#pragma once
#include <QFrame>

class QLabel;
class QVariantAnimation;

namespace sl {

// A single Modrinth result card: icon, title, description, downloads count.
// Smooth hover animation between background colors.
class ModCard : public QFrame {
    Q_OBJECT
public:
    ModCard(const QString &projectId, const QString &title, const QString &desc,
            qint64 downloads, const QString &iconUrl, QWidget *parent = nullptr);

    QString projectId() const { return m_projectId; }
    QString title() const { return m_title; }

    void setIconFromPath(const QString &path);

protected:
    void enterEvent(QEnterEvent *e) override;
    void leaveEvent(QEvent *e) override;

private:
    void animateTo(const QString &targetRgba);
    void requestIcon(const QString &iconUrl);

    QString m_projectId;
    QString m_title;
    QLabel *m_icon = nullptr;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_descLabel = nullptr;
    QLabel *m_metaLabel = nullptr;
    QVariantAnimation *m_anim = nullptr;
};

} // namespace sl
