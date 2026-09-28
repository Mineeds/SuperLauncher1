#include "auth/SiteAuth.h"
#include "auth/TokenStore.h"
#include "net/HttpClient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QUrlQuery>

namespace sl {

// TODO: align these endpoints with the backend on the VPS when wiring up the
// cabinet sync. Expected contract:
//   POST /api/auth/launcher/link  -> { code }
//   GET  /api/auth/launcher/poll?code=...  -> pending | { token, refreshToken, nickname }
static const QString kApiBase = QStringLiteral("https://superlauncher.org/api/auth/launcher");

SiteAuth::SiteAuth(QObject *parent)
    : QObject(parent)
{
}

bool SiteAuth::hasStoredSession()
{
    return !TokenStore::siteRefreshToken().isEmpty();
}

void SiteAuth::start()
{
    emit progress(tr("Подключение к superlauncher.org..."));
    requestLinkCode();
}

void SiteAuth::requestLinkCode()
{
    HttpClient::instance().postJson(
        kApiBase + QStringLiteral("/link"), QJsonDocument(QJsonObject()),
        [this](const QJsonDocument &doc, const QString &error) {
            if (!error.isEmpty()) {
                emit failed(tr("Сайт недоступен: %1").arg(error));
                return;
            }
            m_code = doc.object()[QStringLiteral("code")].toString();
            if (m_code.isEmpty()) {
                emit failed(tr("Сайт не выдал код привязки."));
                return;
            }

            // The user logs in on the site; the code binds this launcher to their cabinet.
            QUrl url(QStringLiteral("https://superlauncher.org/login"));
            QUrlQuery q;
            q.addQueryItem(QStringLiteral("launcher"), m_code);
            url.setQuery(q);
            emit linkReady(url.toString());

            poll();
        });
}

void SiteAuth::poll()
{
    m_elapsed += kPollInterval;

    QUrl url(kApiBase + QStringLiteral("/poll"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("code"), m_code);
    url.setQuery(q);

    HttpClient::instance().getJson(
        url.toString(),
        [this](const QJsonDocument &doc, const QString &error) {
            if (!error.isEmpty()) {
                emit failed(tr("Ошибка связи с сайтом: %1").arg(error));
                return;
            }
            const QJsonObject o = doc.object();
            const QString status = o[QStringLiteral("status")].toString();

            if (status == QStringLiteral("pending")) {
                if (m_elapsed < kMaxElapsed) {
                    QTimer::singleShot(kPollInterval * 1000, this, &SiteAuth::poll);
                } else {
                    emit failed(tr("Время привязки истекло."));
                }
                return;
            }
            if (!o.contains(QStringLiteral("token"))) {
                emit failed(tr("Сайт вернул неожиданный ответ."));
                return;
            }

            TokenStore::saveSiteTokens(o[QStringLiteral("token")].toString(),
                                       o[QStringLiteral("refreshToken")].toString(),
                                       o[QStringLiteral("nickname")].toString());
            TokenStore::clearOffline();
            emit finished(o[QStringLiteral("token")].toString(),
                          o[QStringLiteral("refreshToken")].toString(),
                          o[QStringLiteral("nickname")].toString());
        });
}

} // namespace sl
