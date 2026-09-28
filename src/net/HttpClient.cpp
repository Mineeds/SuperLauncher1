#include "net/HttpClient.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QEventLoop>
#include <QFile>
#include <QDir>
#include <QFileInfo>

namespace sl {

HttpClient &HttpClient::instance()
{
    static HttpClient client;
    return client;
}

HttpClient::HttpClient(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

void HttpClient::getJson(const QString &url, JsonCallback cb)
{
    QNetworkRequest req{QUrl(url)};
    req.setRawHeader(QByteArrayLiteral("User-Agent"),
                    QByteArrayLiteral("SuperLauncher/1.0 (https://superlauncher.org)"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    handleReply(m_nam->get(req), std::move(cb));
}

void HttpClient::getJsonAuth(const QString &url, const QString &bearer, JsonCallback cb)
{
    QNetworkRequest req{QUrl(url)};
    req.setRawHeader(QByteArrayLiteral("Authorization"),
                    (QStringLiteral("Bearer ") + bearer).toUtf8());
    handleReply(m_nam->get(req), std::move(cb));
}

void HttpClient::postJson(const QString &url, const QJsonDocument &body, JsonCallback cb)
{
    QNetworkRequest req{QUrl(url)};
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    handleReply(m_nam->post(req, body.toJson(QJsonDocument::Compact)), std::move(cb));
}

void HttpClient::postForm(const QString &url, const QMap<QString, QString> &form, JsonCallback cb)
{
    QUrlQuery q;
    for (auto it = form.constBegin(); it != form.constEnd(); ++it)
        q.addQueryItem(it.key(), it.value());

    QNetworkRequest req{QUrl(url)};
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  QStringLiteral("application/x-www-form-urlencoded"));
    handleReply(m_nam->post(req, q.toString(QUrl::FullyEncoded).toUtf8()), std::move(cb));
}

bool HttpClient::downloadFileSync(const QString &url, const QString &destPath)
{
    bool ok = false;
    QEventLoop loop;
    downloadFile(url, destPath, [&](bool success, const QString &) {
        ok = success;
        loop.quit();
    });
    loop.exec();
    return ok;
}

void HttpClient::downloadFile(const QString &url, const QString &destPath, FileCallback cb,
                              ProgressCallback progress)
{
    const QFileInfo info(destPath);
    QDir().mkpath(info.absolutePath());

    QNetworkRequest req{QUrl(url)};
    QNetworkReply *reply = m_nam->get(req);
    if (progress)
        connect(reply, &QNetworkReply::downloadProgress, this,
                [progress](qint64 r, qint64 t) { progress(r, t); });
    connect(reply, &QNetworkReply::finished, this, [reply, destPath, cb]() {
        if (reply->error() != QNetworkReply::NoError) {
            cb(false, reply->errorString());
            reply->deleteLater();
            return;
        }
        QFile f(destPath);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            cb(false, QStringLiteral("Cannot write %1").arg(destPath));
            reply->deleteLater();
            return;
        }
        f.write(reply->readAll());
        f.close();
        cb(true, QString());
        reply->deleteLater();
    });
}

void HttpClient::handleReply(QNetworkReply *reply, JsonCallback cb)
{
    connect(reply, &QNetworkReply::finished, this, [reply, cb = std::move(cb)]() {
        const QByteArray data = reply->readAll();
        QString error;
        if (reply->error() != QNetworkReply::NoError)
            error = reply->errorString();
        QJsonParseError parseErr{};
        const QJsonDocument doc = QJsonDocument::fromJson(data, &parseErr);
        if (error.isEmpty() && parseErr.error != QJsonParseError::NoError)
            error = QStringLiteral("JSON parse error: ") + parseErr.errorString();
        cb(doc, error);
        reply->deleteLater();
    });
}

} // namespace sl
