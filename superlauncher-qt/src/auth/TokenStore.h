#pragma once
#include <QString>

namespace sl {

// Secure storage for session data: Microsoft + site tokens and the offline profile.
// Data is written to a file with owner-only permissions (0600 on Unix)
// inside the app config directory, never logged, never printed.
class TokenStore {
public:
    static TokenStore &instance();

    static QString msRefreshToken();   // Microsoft account refresh token
    static QString msAccessToken();    // short-lived Minecraft access token
    static QString msUuid();
    static QString msPlayerName();
    static QString offlineNick();      // offline profile nickname
    static QString siteRefreshToken(); // superlauncher.org session
    static QString siteNickname();

    static void saveMsAccount(const QString &refresh, const QString &uuid, const QString &name);
    static void saveMsAccessToken(const QString &token);
    static void saveOfflineNick(const QString &nick);
    static void saveSiteTokens(const QString &access, const QString &refresh, const QString &nickname);
    static void clearMsAccount();
    static void clearOffline();
    static void clearSite();

private:
    static QString filePath();
    static void save(const QString &key, const QString &value);
    static QString load(const QString &key);
};

} // namespace sl
