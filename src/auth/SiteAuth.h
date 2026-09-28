#pragma once
#include <QObject>

namespace sl {

// Site (superlauncher.org) account linking.
// Flow: the launcher asks the site for a one-time link code, opens the browser
// at the site login/cabinet page with that code, then polls the site until the
// user completes login there. The site then hands back JWT + refresh tokens,
// which are stored securely (TokenStore). This is like the Microsoft device
// flow, but against superlauncher.org and synced with the user's cabinet.
class SiteAuth : public QObject {
    Q_OBJECT
public:
    explicit SiteAuth(QObject *parent = nullptr);

    // Start linking: emits linkReady(browserUrl) once the code is issued,
    // then finished() after the user logs in on the site.
    void start();

    static bool hasStoredSession();

signals:
    void progress(const QString &message);
    void linkReady(const QString &browserUrl);
    void finished(const QString &accessToken, const QString &refreshToken,
                  const QString &nickname);
    void failed(const QString &error);

private slots:
    void poll();

private:
    QString m_code;
    int m_elapsed = 0;
    static const int kPollInterval = 5;   // seconds
    static const int kMaxElapsed = 900;   // 15 minutes

    void requestLinkCode();
};

} // namespace sl
