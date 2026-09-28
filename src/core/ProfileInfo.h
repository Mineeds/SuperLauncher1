#pragma once
#include <QString>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

namespace sl {

// Best-effort loader/Minecraft-version detection from a version folder's
// JSON, so the UI can show "Fabric • 1.20.1" style badges without the user
// having to tag anything manually. Works for native and imported folders.
struct ProfileInfo {
    QString loader = QStringLiteral("Vanilla");
    QString mcVersion;
};

inline ProfileInfo detectProfileInfo(const QString &profileDir, const QString &profileName)
{
    ProfileInfo info;
    QDir dir(profileDir);
    QString jsonPath = dir.filePath(profileName + QStringLiteral(".json"));
    if (!QFile::exists(jsonPath)) {
        const QStringList jsons = dir.entryList({QStringLiteral("*.json")}, QDir::Files);
        if (!jsons.isEmpty())
            jsonPath = dir.filePath(jsons.first());
    }
    QFile f(jsonPath);
    if (!f.open(QIODevice::ReadOnly))
        return info;
    const QByteArray raw = f.readAll();
    f.close();
    const QJsonObject root = QJsonDocument::fromJson(raw).object();
    info.mcVersion = root[QStringLiteral("id")].toString();

    const QString whole = QString::fromUtf8(raw);
    if (whole.contains(QStringLiteral("fabricmc"), Qt::CaseInsensitive))
        info.loader = QStringLiteral("Fabric");
    else if (whole.contains(QStringLiteral("neoforge"), Qt::CaseInsensitive))
        info.loader = QStringLiteral("NeoForge");
    else if (whole.contains(QStringLiteral("quiltmc"), Qt::CaseInsensitive))
        info.loader = QStringLiteral("Quilt");
    else if (whole.contains(QStringLiteral("minecraftforge"), Qt::CaseInsensitive))
        info.loader = QStringLiteral("Forge");

    static const QRegularExpression re(QStringLiteral("(\\d+\\.\\d+(?:\\.\\d+)?)"));
    const auto m = re.match(info.mcVersion);
    if (m.hasMatch())
        info.mcVersion = m.captured(1);
    return info;
}

} // namespace sl
