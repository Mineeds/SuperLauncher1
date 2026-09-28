#include "core/Launcher.h"
#include "core/Config.h"
#include "core/JavaFinder.h"
#include "core/Paths.h"
#include "auth/TokenStore.h"
#include "net/HttpClient.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QProcess>
#include <QSysInfo>
#include <QtGui/private/qzipreader_p.h>

namespace sl {

Launcher &Launcher::instance()
{
    static Launcher l;
    return l;
}

Launcher::Launcher(QObject *parent)
    : QObject(parent)
{
}

// Offline UUID = MD5 of "OfflinePlayer:<nick>" (nameUUIDFromBytes convention).
static QString offlineUuid(const QString &nick)
{
    const QByteArray hash =
        QCryptographicHash::hash(QStringLiteral("OfflinePlayer:%1").arg(nick).toUtf8(),
                                 QCryptographicHash::Md5);
    QByteArray hex = hash.toHex().toUpper();
    // Set version 3 bits.
    hex[14] = '3';                    // version 3
    char b16 = hex[16];
    hex[16] = ((b16 & 0x3) | '8');    // variant
    QString s = QString::fromLatin1(hex);
    return QStringLiteral("%1-%2-%3-%4-%5")
        .arg(s.left(8), s.mid(8, 4), s.mid(12, 4), s.mid(16, 4), s.mid(20, 12));
}

bool Launcher::resolveVersionJson(const Profile &profile, QString *jsonPath,
                                  QJsonDocument *doc, QString *error)
{
    // Prefer <profile>/<profile>.json; fall back to the first *.json in the dir
    // (covers folders imported from other launchers).
    const QDir dir(profile.dir);
    QStringList candidates{dir.filePath(QFileInfo(profile.dir).fileName() + QStringLiteral(".json"))};
    const QStringList jsons = dir.entryList({QStringLiteral("*.json")}, QDir::Files);
    for (const QString &f : jsons)
        candidates << dir.filePath(f);
    for (const auto &path : candidates) {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly))
            continue;
        const QJsonDocument d = QJsonDocument::fromJson(f.readAll());
        f.close();
        if (d.isObject()) {
            *jsonPath = path;
            *doc = d;
            return true;
        }
    }
    *error = tr("В папке версии нет корректного JSON (установка не завершена?)");
    return false;
}

QString Launcher::nativesClassifier(const QJsonObject &lib) const
{
    const QJsonObject natives = lib[QStringLiteral("natives")].toObject();
    if (natives.isEmpty())
        return QString();
    QString osKey = QStringLiteral("linux");
    if (QSysInfo::productType() == QStringLiteral("windows"))
        osKey = QStringLiteral("windows");
    else if (QSysInfo::productType() == QStringLiteral("osx"))
        osKey = QStringLiteral("osx");
    return natives[osKey].toString();
}

bool Launcher::rulesAllow(const QJsonArray &rules) const
{
    if (rules.isEmpty())
        return true;
    bool allowed = false;
    QString osName = QStringLiteral("linux");
    if (QSysInfo::productType() == QStringLiteral("windows"))
        osName = QStringLiteral("windows");
    else if (QSysInfo::productType() == QStringLiteral("osx"))
        osName = QStringLiteral("osx");
    const QString arch = QSysInfo::currentCpuArchitecture(); // "x86_64" / "arm64"

    for (const auto &r : rules) {
        const QJsonObject o = r.toObject();
        const QString action = o[QStringLiteral("action")].toString();
        const QJsonObject os = o[QStringLiteral("os")].toObject();
        bool match = true;
        if (os.contains(QStringLiteral("name")) && os[QStringLiteral("name")].toString() != osName)
            match = false;
        if (os.contains(QStringLiteral("arch")) && os[QStringLiteral("arch")].toString() != arch)
            match = false;
        // "features" rules (is_quick_play etc.) are never true for us.
        if (o.contains(QStringLiteral("features")))
            match = false;
        if (match)
            allowed = (action == QStringLiteral("allow"));
    }
    return allowed;
}

QString Launcher::substitute(QString arg, const QMap<QString, QString> &vars) const
{
    for (auto it = vars.constBegin(); it != vars.constEnd(); ++it)
        arg.replace(QStringLiteral("${%1}").arg(it.key()), it.value());
    return arg;
}

QStringList Launcher::buildJvmArgs(const QJsonObject &versionRoot, const QString &profileDir,
                                   int majorJava) const
{
    Q_UNUSED(majorJava);
    QStringList args;
    // Sensible defaults (memory from settings).
    args << QStringLiteral("-Xms%1m").arg(Config::instance().minRamMb())
         << QStringLiteral("-Xmx%1m").arg(Config::instance().maxRamMb());

    const QJsonValue jvm = versionRoot[QStringLiteral("arguments")]
                               .toObject()[QStringLiteral("jvm")];
    if (jvm.isArray()) {
        for (const auto &v : jvm.toArray()) {
            if (v.isString()) {
                args << v.toString();
            } else {
                const QJsonObject o = v.toObject();
                if (rulesAllow(o[QStringLiteral("rules")].toArray()))
                    args << o[QStringLiteral("value")].toString();
            }
        }
    } else {
        // Legacy defaults.
        args << QStringLiteral("-Djava.library.path=${natives_directory}")
             << QStringLiteral("-Djna.boot.library.path=${natives_directory}")
             << QStringLiteral("-Djava.io.tmpdir=%1").arg(profileDir + QStringLiteral("/tmp"))
             << QStringLiteral("-cp")
             << QStringLiteral("${classpath}");
    }
    return args;
}

QStringList Launcher::buildGameArgs(const QJsonObject &versionRoot) const
{
    const QJsonObject arguments = versionRoot[QStringLiteral("arguments")].toObject();
    if (arguments.contains(QStringLiteral("game"))) {
        QStringList out;
        for (const auto &v : arguments[QStringLiteral("game")].toArray()) {
            if (v.isString()) {
                out << v.toString();
            } else {
                const QJsonObject o = v.toObject();
                if (rulesAllow(o[QStringLiteral("rules")].toArray()))
                    out << o[QStringLiteral("value")].toString();
            }
        }
        return out;
    }
    // Legacy space-separated string.
    const QString legacy = versionRoot[QStringLiteral("minecraftArguments")].toString();
    return legacy.split(QLatin1Char(' '), Qt::SkipEmptyParts);
}

bool Launcher::ensureNatives(const QJsonObject &versionRoot, const Profile &profile,
                             QString *error)
{
    const QString nativesDir = profile.dir + QStringLiteral("/natives");
    QDir().mkpath(nativesDir);

    for (const auto &l : versionRoot[QStringLiteral("libraries")].toArray()) {
        const QJsonObject lib = l.toObject();
        const QString classifier = nativesClassifier(lib);
        if (classifier.isEmpty())
            continue;

        const QJsonObject natives = lib[QStringLiteral("downloads")]
                                        .toObject()[QStringLiteral("classifiers")]
                                        .toObject()[classifier]
                                        .toObject();
        const QString url = natives[QStringLiteral("url")].toString();
        const QString path = natives[QStringLiteral("path")].toString();
        if (url.isEmpty() || path.isEmpty())
            continue;

        const QString jar = Paths::librariesDir() + QStringLiteral("/") + path;
        if (!QFile::exists(jar)) {
            QDir().mkpath(QFileInfo(jar).absolutePath());
            if (!HttpClient::instance().downloadFileSync(url, jar)) {
                *error = tr("Не удалось скачать нативы: %1").arg(path);
                return false;
            }
        }
        // Extract the natives jar into <version>/natives.
        QZipReader reader(jar);
        if (reader.exists())
            reader.extractAll(nativesDir);
    }
    return true;
}

void Launcher::launch(const Profile &profile)
{
    QString jsonPath;
    QJsonDocument doc;
    QString error;
    if (!resolveVersionJson(profile, &jsonPath, &doc, &error)) {
        emit launchFailed(profile.name, error);
        return;
    }
    const QJsonObject root = doc.object();

    // --- Account ---
    QString playerName, uuid, accessToken, userType = QStringLiteral("legacy");
    if (!TokenStore::msPlayerName().isEmpty()) {
        playerName = TokenStore::msPlayerName();
        uuid = TokenStore::msUuid();
        accessToken = TokenStore::msAccessToken().isEmpty() ? QStringLiteral("0")
                                                            : TokenStore::msAccessToken();
        userType = QStringLiteral("msa");
    } else if (!TokenStore::siteNickname().isEmpty()) {
        playerName = TokenStore::siteNickname();
        uuid = offlineUuid(playerName);
        accessToken = QStringLiteral("0");
        userType = QStringLiteral("mojang");
    } else {
        playerName = TokenStore::offlineNick().isEmpty() ? tr("Player")
                                                         : TokenStore::offlineNick();
        uuid = offlineUuid(playerName);
        accessToken = QStringLiteral("0");
    }

    // --- Java ---
    const int requiredJava = root[QStringLiteral("javaVersion")]
                                 .toObject()[QStringLiteral("majorVersion")]
                                 .toInt(8);
    JavaInfo java;
    const QString manualJava = Config::instance().javaPath();
    if (!manualJava.isEmpty() && QFile::exists(manualJava)) {
        java.path = manualJava;
        java.major = 99; // trust the user
    } else {
        java = JavaFinder::findBest(requiredJava);
    }
    if (java.path.isEmpty()) {
        emit launchFailed(profile.name,
                          tr("Не найдена Java %1+. Установи её или положи в .minecraft/javas.")
                              .arg(requiredJava));
        return;
    }

    // --- Natives ---
    if (!ensureNatives(root, profile, &error)) {
        emit launchFailed(profile.name, error);
        return;
    }

    // --- Classpath ---
    const QString jarPath =
        profile.dir + QStringLiteral("/") + QFileInfo(profile.dir).fileName() + QStringLiteral(".jar");
    QStringList classList;
    for (const auto &l : root[QStringLiteral("libraries")].toArray()) {
        const QJsonObject lib = l.toObject();
        if (lib.contains(QStringLiteral("natives")))
            continue;
        const QString p = lib[QStringLiteral("downloads")]
                              .toObject()[QStringLiteral("artifact")]
                              .toObject()[QStringLiteral("path")]
                              .toString();
        if (!p.isEmpty())
            classList << Paths::librariesDir() + QStringLiteral("/") + p;
    }
    classList << jarPath;
    const bool isWindows = QSysInfo::productType() == QStringLiteral("windows");
    const QString cpSeparator = isWindows ? QStringLiteral(";") : QStringLiteral(":");

    // --- Variables ---
    QMap<QString, QString> vars;
    vars.insert(QStringLiteral("version_name"), profile.name);
    vars.insert(QStringLiteral("profile_name"), profile.name);
    vars.insert(QStringLiteral("version_type"), root[QStringLiteral("type")].toString());
    vars.insert(QStringLiteral("auth_player_name"), playerName);
    vars.insert(QStringLiteral("auth_uuid"), uuid);
    vars.insert(QStringLiteral("auth_access_token"), accessToken);
    vars.insert(QStringLiteral("auth_xuid"), QStringLiteral(""));
    vars.insert(QStringLiteral("clientid"), QStringLiteral("superlauncher"));
    vars.insert(QStringLiteral("user_type"), userType);
    vars.insert(QStringLiteral("assets_root"), Paths::assetsDir());
    vars.insert(QStringLiteral("assets_index_name"),
                root[QStringLiteral("assets")].toString(QStringLiteral("legacy")));
    vars.insert(QStringLiteral("game_directory"), Paths::gameDir());
    vars.insert(QStringLiteral("library_directory"), Paths::librariesDir());
    vars.insert(QStringLiteral("classpath_separator"), cpSeparator);
    vars.insert(QStringLiteral("natives_directory"), profile.dir + QStringLiteral("/natives"));
    vars.insert(QStringLiteral("launcher_name"), QStringLiteral("SuperLauncher"));
    vars.insert(QStringLiteral("launcher_version"), QStringLiteral("1.0"));

    QStringList fullArgs;
    for (const QString &a : buildJvmArgs(root, profile.dir, java.major))
        fullArgs << substitute(a, vars);
    if (!fullArgs.contains(QStringLiteral("-cp")))
        fullArgs << QStringLiteral("-cp") << classList.join(cpSeparator);
    fullArgs << root[QStringLiteral("mainClass")].toString();
    for (const QString &a : buildGameArgs(root))
        fullArgs << substitute(a, vars);

    if (root[QStringLiteral("mainClass")].toString().isEmpty()) {
        emit launchFailed(profile.name, tr("В JSON версии нет mainClass."));
        return;
    }

    // --- Start ---
    QDir::setCurrent(Paths::gameDir());
    const qint64 pid = QProcess::startDetached(java.path, fullArgs, Paths::gameDir());
    if (pid == 0) {
        emit launchFailed(profile.name, tr("Не удалось запустить процесс Java."));
        return;
    }
    emit gameStarted(profile.name);
}

} // namespace sl
