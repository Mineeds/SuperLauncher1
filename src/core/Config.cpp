#include "core/Config.h"
#include "core/Paths.h"

#include <QSettings>
#include <QStandardPaths>
#include <QDir>

namespace sl {

Config &Config::instance()
{
    static Config cfg;
    return cfg;
}

Config::Config(QObject *parent)
    : QObject(parent)
{
    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    m_filePath = Paths::configDir() + QStringLiteral("/superlauncher.ini");
}

QString Config::customGameDir() const
{
    QSettings s(m_filePath, QSettings::IniFormat);
    return s.value(QStringLiteral("paths/customGameDir")).toString();
}

void Config::setCustomGameDir(const QString &dir)
{
    QSettings s(m_filePath, QSettings::IniFormat);
    s.setValue(QStringLiteral("paths/customGameDir"), dir);
}

int Config::minRamMb() const
{
    QSettings s(m_filePath, QSettings::IniFormat);
    return s.value(QStringLiteral("game/minRamMb"), 512).toInt();
}

int Config::maxRamMb() const
{
    QSettings s(m_filePath, QSettings::IniFormat);
    return s.value(QStringLiteral("game/maxRamMb"), 2048).toInt();
}

void Config::setMinRamMb(int mb)
{
    QSettings s(m_filePath, QSettings::IniFormat);
    s.setValue(QStringLiteral("game/minRamMb"), mb);
}

void Config::setMaxRamMb(int mb)
{
    QSettings s(m_filePath, QSettings::IniFormat);
    s.setValue(QStringLiteral("game/maxRamMb"), mb);
}

QString Config::javaPath() const
{
    QSettings s(m_filePath, QSettings::IniFormat);
    return s.value(QStringLiteral("java/path")).toString();
}

void Config::setJavaPath(const QString &path)
{
    QSettings s(m_filePath, QSettings::IniFormat);
    s.setValue(QStringLiteral("java/path"), path);
}

QByteArray Config::windowGeometry() const
{
    QSettings s(m_filePath, QSettings::IniFormat);
    return s.value(QStringLiteral("window/geometry")).toByteArray();
}

void Config::setWindowGeometry(const QByteArray &geometry)
{
    QSettings s(m_filePath, QSettings::IniFormat);
    s.setValue(QStringLiteral("window/geometry"), geometry);
}

} // namespace sl
