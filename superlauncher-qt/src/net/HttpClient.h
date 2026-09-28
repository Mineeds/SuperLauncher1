#pragma once
#include <QObject>
#include <QJsonDocument>
#include <QJsonValue>
#include <functional>

class QNetworkAccessManager;
class QNetworkReply;

namespace sl {

// Thin async HTTP/HTTPS client on top of QNetworkAccessManager.
// All requests run asynchronously; results are delivered via callbacks on the main thread.
class HttpClient : public QObject {
    Q_OBJECT
public:
    using JsonCallback = std::function<void(const QJsonDocument &, const QString &error)>;
    using FileCallback = std::function<void(bool ok, const QString &error)>;
    using ProgressCallback = std::function<void(qint64 received, qint64 total)>;

    static HttpClient &instance();

    // GET a URL, parse the body as JSON, call cb(reply, error).
    void getJson(const QString &url, JsonCallback cb);
    // GET with a Bearer token header.
    void getJsonAuth(const QString &url, const QString &bearer, JsonCallback cb);
    // POST a JSON body.
    void postJson(const QString &url, const QJsonDocument &body, JsonCallback cb);
    // POST url-encoded form data.
    void postForm(const QString &url, const QMap<QString, QString> &form, JsonCallback cb);
    // Blocking download (event loop), for small pre-launch files. Prefer downloadFile().
    bool downloadFileSync(const QString &url, const QString &destPath);
    // Download a file to destPath (parent dirs are created). Progress is optional.
    void downloadFile(const QString &url, const QString &destPath, FileCallback cb,
                      ProgressCallback progress = nullptr);

private:
    explicit HttpClient(QObject *parent = nullptr);
    void handleReply(QNetworkReply *reply, JsonCallback cb);

    QNetworkAccessManager *m_nam;
};

} // namespace sl
