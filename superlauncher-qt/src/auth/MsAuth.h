#pragma once
#include <QObject>

namespace sl {

// Microsoft account authentication via OAuth 2.0 Device Authorization Grant,
// followed by Xbox Live (XBL) -> XSTS -> Minecraft Services flow.
// Ported from the Python implementation (launcher/auth.py).
class MsAuth : public QObject {
    Q_OBJECT
public:
    explicit MsAuth(QObject *parent = nullptr);

    // Starts the device-code flow. Emits userCodeReady(userCode, verificationUri).
    void start();

signals:
    void progress(const QString &message);
    void userCodeReady(const QString &userCode, const QString &verificationUri);
    void finished(const QString &mcAccessToken,
                  const QString &refreshToken,
                  const QString &uuid,
                  const QString &playerName);
    void failed(const QString &error);

private slots:
    void pollForToken();
    void onXblFinished(const class QJsonDocument &doc, const QString &error);

private:
    QString m_deviceCode;
    QString m_refreshToken;
    int m_interval = 5;
    int m_elapsed = 0;
    static const int kMaxElapsed = 900; // 15 minutes

    void doXblAuth(const QString &accessToken);
    void doXstsAuth(const QString &xblToken);
    void doMinecraftAuth(const QString &uhs, const QString &xstsToken);
    void doOwnershipAndProfile(const QString &mcToken);
};

} // namespace sl
