#include "core/VersionManager.h"
#include "core/Paths.h"
#include "core/Profiles.h"
#include "net/HttpClient.h"

#include <QDir>
#include <QFile>
#include <memory>
#include <QJsonArray>
#include <QJsonObject>

namespace sl {

static const QString kManifestUrl =
    QStringLiteral("https://launchermeta.mojang.com/mc/game/version_manifest_v2.json");

VersionManager &VersionManager::instance()
{
    static VersionManager vm;
    return vm;
}

VersionManager::VersionManager(QObject *parent)
    : QObject(parent)
{
}

void VersionManager::fetchManifest(
    QObject *context, std::function<void(const QList<ManifestEntry> &, const QString &)> cb)
{
    HttpClient::instance().getJson(
        kManifestUrl, [cb, context](const QJsonDocument &doc, const QString &error) {
            QList<ManifestEntry> out;
            if (error.isEmpty()) {
                const QJsonArray versions = doc.object()[QStringLiteral("versions")].toArray();
                for (const auto &v : versions) {
                    const QJsonObject o = v.toObject();
                    ManifestEntry e;
                    e.id = o[QStringLiteral("id")].toString();
                    e.url = o[QStringLiteral("url")].toString();
                    e.type = o[QStringLiteral("type")].toString();
                    e.releaseTime = o[QStringLiteral("releaseTime")].toString();
                    if (!e.id.isEmpty() && !e.url.isEmpty())
                        out.append(e);
                }
            }
            if (cb && context)
                QMetaObject::invokeMethod(context, [cb, out, error]() { cb(out, error); },
                                          Qt::QueuedConnection);
        });
}

void VersionManager::installVanilla(const QString &versionId, const QString &profileName)
{
    m_versionId = versionId;
    m_profileName = profileName;
    m_failError.clear();
    m_pendingFiles = 0;
    m_totalBytes = 0;

    emit installProgress(2, tr("Загрузка манифеста версий..."));
    fetchManifest(this, [this](const QList<ManifestEntry> &entries, const QString &error) {
        if (!error.isEmpty()) {
            fail(tr("Манифест недоступен: %1").arg(error));
            return;
        }
        QString versionUrl;
        for (const auto &e : entries) {
            if (e.id == m_versionId) {
                versionUrl = e.url;
                break;
            }
        }
        if (versionUrl.isEmpty()) {
            fail(tr("Версия %1 не найдена в манифесте.").arg(m_versionId));
            return;
        }

        emit installProgress(6, tr("Загрузка данных версии %1...").arg(m_versionId));
        HttpClient::instance().getJson(
            versionUrl, [this](const QJsonDocument &doc, const QString &err) {
                if (!err.isEmpty()) {
                    fail(tr("Не удалось загрузить данные версии: %1").arg(err));
                    return;
                }
                m_versionJson = doc;
                const QJsonObject o = doc.object();

                // 1) Save the version JSON into the profile folder (standard layout).
                const QString profileDir = Profiles::defaultVersionDir(m_profileName);
                QDir().mkpath(profileDir);
                QFile jsonFile(profileDir + QStringLiteral("/%1.json").arg(m_profileName));
                if (jsonFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                    jsonFile.write(doc.toJson(QJsonDocument::Compact));
                    jsonFile.close();
                }

                // 2) Download client JAR.
                const QJsonObject client =
                    o[QStringLiteral("downloads")].toObject()[QStringLiteral("client")].toObject();
                const QString clientUrl = client[QStringLiteral("url")].toString();

                emit installProgress(10, tr("Загрузка клиента..."));
                HttpClient::instance().downloadFile(
                    clientUrl,
                    profileDir + QStringLiteral("/%1.jar").arg(m_profileName),
                    [this](bool ok, const QString &err) {
                        if (!ok) {
                            fail(tr("Клиент не скачался: %1").arg(err));
                            return;
                        }
                        downloadLibraries();
                    });

                // 3) Libraries + assets are collected in the profile's flow below.
                m_versionJson = doc; // keep alive for later stages
            });
    });
}

void VersionManager::downloadLibraries()
{
    const QJsonObject o = m_versionJson.object();
    const QJsonArray libs = o[QStringLiteral("libraries")].toArray();

    // Pairs of <url, destPath>.
    QList<QPair<QString, QString>> files;
    for (const auto &l : libs) {
        const QJsonObject lib = l.toObject();
        const QJsonObject artifact = lib[QStringLiteral("downloads")]
                                        .toObject()[QStringLiteral("artifact")]
                                        .toObject();
        const QString url = artifact[QStringLiteral("url")].toString();
        const QString path = artifact[QStringLiteral("path")].toString();
        if (url.isEmpty() || path.isEmpty())
            continue; // natives and rules-free entries are skipped for now
        files.append({url, Paths::librariesDir() + QStringLiteral("/") + path});
    }

    emit installProgress(15, tr("Загрузка библиотек (%1)...").arg(files.size()));
    downloadIndexed(files, tr("библиотеки"), 15, 55, [this]() { downloadAssets(); });
}

void VersionManager::downloadAssets()
{
    const QJsonObject assetIndex = m_versionJson.object()[QStringLiteral("assetIndex")].toObject();
    const QString indexUrl = assetIndex[QStringLiteral("url")].toString();
    const QString indexId = assetIndex[QStringLiteral("id")].toString();

    if (indexUrl.isEmpty()) {
        maybeFinished();
        return;
    }

    emit installProgress(57, tr("Загрузка индекса ассетов..."));
    HttpClient::instance().downloadFile(
        indexUrl, Paths::assetsDir() + QStringLiteral("/indexes/%1.json").arg(indexId),
        [this, indexId](bool ok, const QString &err) {
            if (!ok) {
                fail(tr("Индекс ассетов не скачался: %1").arg(err));
                return;
            }
            // Parse the index and download every object.
            QFile f(Paths::assetsDir() + QStringLiteral("/indexes/%1.json").arg(indexId));
            if (!f.open(QIODevice::ReadOnly)) {
                fail(tr("Не удалось открыть индекс ассетов."));
                return;
            }
            const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
            f.close();

            const QJsonObject objects = doc.object()[QStringLiteral("objects")].toObject();
            QList<QPair<QString, QString>> files;
            for (auto it = objects.constBegin(); it != objects.constEnd(); ++it) {
                const QString hash = it.value().toObject()[QStringLiteral("hash")].toString();
                if (hash.size() < 2)
                    continue;
                const QString url = QStringLiteral(
                    "https://resources.download.minecraft.net/%1/%2")
                                        .arg(hash.left(2), hash);
                const QString dest = Paths::assetsDir() + QStringLiteral("/objects/%1/%2")
                                         .arg(hash.left(2), hash);
                // Skip already-downloaded objects (other launchers share this layout).
                if (QFile::exists(dest) && QFile(dest).size() ==
                        it.value().toObject()[QStringLiteral("size")].toInt()) {
                    continue;
                }
                files.append({url, dest});
            }

            emit installProgress(60, tr("Загрузка ассетов (%1 файлов)...").arg(files.size()));
            downloadIndexed(files, tr("ассеты"), 60, 99, [this]() { maybeFinished(); });
        });
}

void VersionManager::downloadIndexed(const QList<QPair<QString, QString>> &files,
                                      const QString &stageMsg, int fromPercent, int toPercent,
                                      const std::function<void()> &onDone)
{
    m_totalFiles = files.size();
    m_pendingFiles = files.size();
    if (m_totalFiles == 0) {
        onDone();
        return;
    }
    // Counter must live on the heap: callbacks fire after this function returns.
    auto done = std::make_shared<int>(0);
    const int total = m_totalFiles;
    for (const auto &pair : files) {
        HttpClient::instance().downloadFile(
            pair.first, pair.second,
            [this, onDone, fromPercent, toPercent, done, total, stageMsg](bool ok, const QString &err) {
                ++(*done);
                if (!ok && m_failError.isEmpty())
                    m_failError = err; // remember the first error, keep draining
                emit installProgress(
                    fromPercent + (toPercent - fromPercent) * (*done) / total,
                    tr("Загрузка %1: %2/%3").arg(stageMsg).arg(*done).arg(total));
                if (*done == total) {
                    if (!m_failError.isEmpty()) {
                        fail(tr("Ошибка загрузки %1: %2").arg(stageMsg, m_failError));
                        return;
                    }
                    onDone();
                }
            });
    }
}

void VersionManager::fail(const QString &error)
{
    emit installFailed(m_versionId, error);
}

void VersionManager::maybeFinished()
{
    emit installProgress(100, tr("Готово"));
    emit installFinished(m_versionId, m_profileName);
}

} // namespace sl
