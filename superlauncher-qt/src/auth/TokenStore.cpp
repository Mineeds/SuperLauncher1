#include "auth/TokenStore.h"
#include "core/Paths.h"

#include <QFile>
#include <QDir>
#include <QSettings>
#include <QFileInfo>

namespace sl {

QString TokenStore::filePath()
{
    return Paths::configDir() + QStringLiteral("/session.dat");
}

void TokenStore::save(const QString &key, const QString &value)
{
    QSettings s(filePath(), QSettings::IniFormat);
    s.setValue(key, value);
    // Restrict permissions to the current user only.
    QFile f(filePath());
    QFileInfo info(f);
    f.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
}

QString TokenStore::load(const QString &key)
{
    QSettings s(filePath(), QSettings::IniFormat);
    return s.value(key).toString();
}

QString TokenStore::msRefreshToken(){ return load(QStringLiteral("ms/refresh")); }
QString TokenStore::msAccessToken() { return load(QStringLiteral("ms/access")); }
QString TokenStore::msUuid()        { return load(QStringLiteral("ms/uuid")); }
QString TokenStore::msPlayerName()  { return load(QStringLiteral("ms/name")); }
QString TokenStore::offlineNick()   { return load(QStringLiteral("offline/nick")); }
QString TokenStore::siteRefreshToken() { return load(QStringLiteral("site/refresh")); }
QString TokenStore::siteNickname()   { return load(QStringLiteral("site/nick")); }

void TokenStore::saveMsAccount(const QString &refresh, const QString &uuid, const QString &name)
{
    save(QStringLiteral("ms/refresh"), refresh);
    save(QStringLiteral("ms/uuid"), uuid);
    save(QStringLiteral("ms/name"), name);
}

void TokenStore::saveMsAccessToken(const QString &token)
{
    save(QStringLiteral("ms/access"), token);
}

void TokenStore::saveOfflineNick(const QString &nick)
{
    save(QStringLiteral("offline/nick"), nick);
}

void TokenStore::clearMsAccount()
{
    save(QStringLiteral("ms/refresh"), QString());
    save(QStringLiteral("ms/uuid"), QString());
    save(QStringLiteral("ms/name"), QString());
}

void TokenStore::saveSiteTokens(const QString &access, const QString &refresh, const QString &nickname)
{
    save(QStringLiteral("site/access"), access);
    save(QStringLiteral("site/refresh"), refresh);
    save(QStringLiteral("site/nick"), nickname);
}

void TokenStore::clearOffline()
{
    save(QStringLiteral("offline/nick"), QString());
}

void TokenStore::clearSite()
{
    save(QStringLiteral("site/access"), QString());
    save(QStringLiteral("site/refresh"), QString());
    save(QStringLiteral("site/nick"), QString());
}

} // namespace sl
