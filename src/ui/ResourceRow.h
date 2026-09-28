#pragma once
#include <QFrame>
#include <QStringList>

class QLabel;
class QPushButton;

namespace sl {

// A single Modrinth search result row for the "Ресурси" page: thumbnail,
// title + author, description, tag pills, downloads and a download button.
// Wider/list-style (vs. the compact ModCard used elsewhere).
class ResourceRow : public QFrame {
    Q_OBJECT
public:
    ResourceRow(const QString &projectId, const QString &slug, const QString &title,
                const QString &author, const QString &desc, qint64 downloads,
                const QStringList &tags, const QString &iconUrl, QWidget *parent = nullptr);

    QString projectId() const { return m_projectId; }
    QString slug() const { return m_slug; }
    QString title() const { return m_title; }

signals:
    void downloadRequested();

private:
    void requestIcon(const QString &iconUrl);
    void setIconFromPath(const QString &path);

    QString m_projectId;
    QString m_slug;
    QString m_title;
    QLabel *m_icon = nullptr;
};

} // namespace sl
