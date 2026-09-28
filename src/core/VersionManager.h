#pragma once
#include <QObject>
#include <QJsonDocument>

namespace sl {

// Vanilla Minecraft version management:
//  - fetches the Mojang version manifest,
//  - installs a vanilla version (version JSON, client JAR, libraries, assets)
//    into the standard .minecraft layout (no prefixes, standard folder names).
class VersionManager : public QObject {
    Q_OBJECT
public:
    struct ManifestEntry {
        QString id;          // e.g. "1.20.1"
        QString url;        // version JSON url
        QString type;       // release / snapshot / old_beta ...
        QString releaseTime;
    };

    static VersionManager &instance();

    // Fetch the public version manifest. cb(entries, error).
    void fetchManifest(QObject *context,
                        std::function<void(const QList<ManifestEntry> &, const QString &)> cb);

    // Full vanilla install of versionId into the profile folder
    // <gameDir>/versions/<profileName> (standard naming).
    void installVanilla(const QString &versionId, const QString &profileName);

signals:
    void installProgress(int percent, const QString &message);
    void installFinished(const QString &versionId, const QString &profileName);
    void installFailed(const QString &versionId, const QString &error);

private:
    explicit VersionManager(QObject *parent = nullptr);

    QString m_versionId;
    QString m_profileName;
    QJsonDocument m_versionJson;   // Mojang version JSON
    int m_pendingFiles = 0;
    int m_totalFiles = 0;
    qint64 m_totalBytes = 0;
    QString m_failError;

    void downloadLibraries();
    void downloadAssets();
    void fail(const QString &error);
    void maybeFinished();
    void downloadIndexed(const QList<QPair<QString, QString>> &files,
                          const QString &stageMsg, int fromPercent, int toPercent,
                          const std::function<void()> &onDone);
};

} // namespace sl
