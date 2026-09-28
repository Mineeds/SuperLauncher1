#include "auth/MsAuth.h"
#include "net/HttpClient.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonObject>
#include <QTimer>
#include <QUrl>

namespace sl {

// Minecraft's default public client ID (same as in the Python implementation).
static const QString kClientId = QStringLiteral("00000000402b5328");

MsAuth::MsAuth(QObject *parent)
    : QObject(parent)
{
}

void MsAuth::start()
{
    emit progress(tr("Запуск авторизации Microsoft..."));

    QMap<QString, QString> form{
        {QStringLiteral("client_id"), kClientId},
        {QStringLiteral("scope"), QStringLiteral("XboxLive.signin offline_access")},
        {QStringLiteral("response_type"), QStringLiteral("device_code")},
    };
    HttpClient::instance().postForm(
        QStringLiteral("https://login.live.com/oauth20_connect.srf"), form,
        [this](const QJsonDocument &doc, const QString &error) {
            if (!error.isEmpty()) {
                emit failed(tr("Ошибка device auth: %1").arg(error));
                return;
            }
            const QJsonObject o = doc.object();
            m_deviceCode = o[QStringLiteral("device_code")].toString();
            m_interval = o[QStringLiteral("interval")].toInt(5);
            emit userCodeReady(o[QStringLiteral("user_code")].toString(),
                               o[QStringLiteral("verification_uri")].toString());
            pollForToken();
        });
}

void MsAuth::pollForToken()
{
    QMap<QString, QString> form{
        {QStringLiteral("grant_type"),
         QStringLiteral("urn:ietf:params:oauth:grant-type:device_code")},
        {QStringLiteral("client_id"), kClientId},
        {QStringLiteral("device_code"), m_deviceCode},
    };
    HttpClient::instance().postForm(
        QStringLiteral("https://login.live.com/oauth20_token.srf"), form,
        [this](const QJsonDocument &doc, const QString &error) {
            m_elapsed += m_interval;
            const QJsonObject o = doc.object();

            if (!error.isEmpty() && o.isEmpty()) {
                emit failed(tr("Ошибка получения токена: %1").arg(error));
                return;
            }
            if (o.contains(QStringLiteral("error"))) {
                const QString err = o[QStringLiteral("error")].toString();
                if (err == QStringLiteral("authorization_pending")) {
                    if (m_elapsed < kMaxElapsed) {
                        QTimer::singleShot(m_interval * 1000, this, &MsAuth::pollForToken);
                    } else {
                        emit failed(tr("Время авторизации истекло."));
                    }
                    return;
                }
                emit failed(tr("OAuth ошибка: %1").arg(err));
                return;
            }

            m_refreshToken = o[QStringLiteral("refresh_token")].toString();
            emit progress(tr("Вход в Xbox Live..."));
            doXblAuth(o[QStringLiteral("access_token")].toString());
        });
}

void MsAuth::doXblAuth(const QString &accessToken)
{
    const QJsonObject body{
        {QStringLiteral("Properties"),
         QJsonObject{
             {QStringLiteral("AuthMethod"), QStringLiteral("RPS")},
             {QStringLiteral("SiteName"), QStringLiteral("user.auth.xboxlive.com")},
             {QStringLiteral("RpsTicket"), QStringLiteral("d=") + accessToken},
         }},
        {QStringLiteral("RelyingParty"), QStringLiteral("http://auth.xboxlive.com")},
        {QStringLiteral("TokenType"), QStringLiteral("JWT")},
    };
    HttpClient::instance().postJson(
        QStringLiteral("https://user.auth.xboxlive.com/user/authenticate"),
        QJsonDocument(body),
        [this](const QJsonDocument &doc, const QString &error) {
            onXblFinished(doc, error);
        });
}

void MsAuth::onXblFinished(const QJsonDocument &doc, const QString &error)
{
    if (!error.isEmpty()) {
        emit failed(tr("Ошибка XBL: %1").arg(error));
        return;
    }
    const QString xblToken = doc.object()[QStringLiteral("Token")].toString();
    emit progress(tr("Авторизация XSTS..."));
    doXstsAuth(xblToken);
}

void MsAuth::doXstsAuth(const QString &xblToken)
{
    const QJsonObject body{
        {QStringLiteral("Properties"),
         QJsonObject{
             {QStringLiteral("SandboxId"), QStringLiteral("RETAIL")},
             {QStringLiteral("UserTokens"), QJsonArray{xblToken}},
         }},
        {QStringLiteral("RelyingParty"), QStringLiteral("rp://api.minecraftservices.com/")},
        {QStringLiteral("TokenType"), QStringLiteral("JWT")},
    };
    HttpClient::instance().postJson(
        QStringLiteral("https://xsts.auth.xboxlive.com/xsts/authorize"), QJsonDocument(body),
        [this](const QJsonDocument &doc, const QString &error) {
            if (!error.isEmpty()) {
                emit failed(tr("Ошибка XSTS: %1").arg(error));
                return;
            }
            const QJsonObject o = doc.object();
            const QString uhs = o[QStringLiteral("DisplayClaims")]
                                    .toObject()[QStringLiteral("xui")]
                                    .toArray()
                                    .at(0)
                                    .toObject()[QStringLiteral("uhs")]
                                    .toString();
            emit progress(tr("Вход в Minecraft..."));
            doMinecraftAuth(uhs, o[QStringLiteral("Token")].toString());
        });
}

void MsAuth::doMinecraftAuth(const QString &uhs, const QString &xstsToken)
{
    const QJsonObject body{
        {QStringLiteral("identityToken"),
         QStringLiteral("XBL3.0 x=%1;%2").arg(uhs, xstsToken)},
    };
    HttpClient::instance().postJson(
        QStringLiteral("https://api.minecraftservices.com/authentication/login_with_xbox"),
        QJsonDocument(body),
        [this](const QJsonDocument &doc, const QString &error) {
            if (!error.isEmpty()) {
                emit failed(tr("Ошибка Minecraft auth: %1").arg(error));
                return;
            }
            doOwnershipAndProfile(
                doc.object()[QStringLiteral("access_token")].toString());
        });
}

void MsAuth::doOwnershipAndProfile(const QString &mcToken)
{
    emit progress(tr("Проверка владения игрой..."));
    HttpClient::instance().getJsonAuth(
        QStringLiteral("https://api.minecraftservices.com/entitlements/mcstore"), mcToken,
        [this, mcToken](const QJsonDocument &, const QString &) {
            // Entitlements check is soft: proceed to profile either way.
            emit progress(tr("Загрузка профиля..."));
            HttpClient::instance().getJsonAuth(
                QStringLiteral("https://api.minecraftservices.com/minecraft/profile"),
                mcToken, [this, mcToken](const QJsonDocument &doc, const QString &error) {
                    if (!error.isEmpty()) {
                        emit failed(tr("Ошибка профиля: %1").arg(error));
                        return;
                    }
                    const QJsonObject o = doc.object();
                    emit finished(mcToken, m_refreshToken,
                                  o[QStringLiteral("id")].toString(),
                                  o[QStringLiteral("name")].toString());
                });
        });
}

} // namespace sl
