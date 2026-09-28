#include "core/Paths.h"
#include "core/Config.h"

#include <QStandardPaths>

namespace sl {

QString Paths::gameDir()
{
    const QString custom = Config::instance().customGameDir();
    if (!custom.isEmpty() && QDir(custom).exists())
        return custom;
    return QDir::home().filePath(QStringLiteral(".minecraft"));
}

QString Paths::versionsDir()    { return gameDir() + QStringLiteral("/versions"); }
QString Paths::assetsDir()      { return gameDir() + QStringLiteral("/assets"); }
QString Paths::librariesDir()  { return gameDir() + QStringLiteral("/libraries"); }
QString Paths::configDir()     { return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation); }

} // namespace sl
