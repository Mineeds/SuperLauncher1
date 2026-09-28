#pragma once
#include <QString>
#include <QDir>

namespace sl {

class Paths {
public:
    // Root game data directory. Default: <home>/.minecraft (like TLauncher/SKLauncher).
    // If a custom directory is configured in settings, it is used instead.
    static QString gameDir();

    static QString versionsDir();
    static QString assetsDir();
    static QString librariesDir();
    static QString configDir();
};

} // namespace sl
