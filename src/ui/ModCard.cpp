#include "ui/ModCard.h"
#include "net/HttpClient.h"

#include <QDir>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFile>
#include <QLabel>
#include <QPixmap>
#include <QStandardPaths>
#include <QVariantAnimation>

namespace sl {

namespace {
const char *kCardStyle = R"(
QFrame#card {
    background:#0d0d18; border:1px solid #161626; border-radius:10px;
}
QLabel { color:#ededf2; border:none; background:transparent; }
QLabel#title { font-weight:bold; font-size:14px; }
QLabel#desc { color:#8a8a9e; font-size:12px; }
QLabel#meta { color:#1ad16f; font-size:12px; }
QLabel#icon {
    background:#161626; border-radius:8px;
    min-width:44px; min-height:44px; max-width:44px; max-height:44px;
}
)";
}

ModCard::ModCard(const QString &projectId, const QString &title, const QString &desc,
                 qint64 downloads, const QString &iconUrl, QWidget *parent)
    : QFrame(parent)
    , m_projectId(projectId)
    , m_title(title)
{
    setObjectName(QStringLiteral("card"));
    setStyleSheet(kCardStyle);
    setFixedHeight(72);

    m_icon = new QLabel();
    m_icon->setObjectName(QStringLiteral("icon"));
    m_icon->setAlignment(Qt::AlignCenter);

    m_titleLabel = new QLabel(title);
    m_titleLabel->setObjectName(QStringLiteral("title"));
    m_descLabel = new QLabel(desc.left(90));
    m_descLabel->setObjectName(QStringLiteral("desc"));
    m_metaLabel = new QLabel(QObject::tr("%1K загрузок").arg(downloads / 1000));
    m_metaLabel->setObjectName(QStringLiteral("meta"));

    auto *textCol = new QVBoxLayout();
    textCol->setContentsMargins(0, 0, 0, 0);
    textCol->setSpacing(2);
    textCol->addWidget(m_titleLabel);
    textCol->addWidget(m_descLabel);
    textCol->addWidget(m_metaLabel);

    auto *lay = new QHBoxLayout(this);
    lay->setContentsMargins(12, 12, 12, 12);
    lay->setSpacing(14);
    lay->addWidget(m_icon);
    lay->addLayout(textCol, 1);

    // Hover color animation.
    m_anim = new QVariantAnimation(this);
    m_anim->setDuration(140);
    connect(m_anim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        const QColor c = v.value<QColor>();
        const QString border = c.lightness() > 40 ? QStringLiteral("#1ad16f")
                                                  : QStringLiteral("#161626");
        setStyleSheet(QStringLiteral("QFrame#card { background:%1; border:1px solid %2; "
                                    "border-radius:10px; } %3")
                          .arg(c.name(QColor::HexArgb), border,
                               QString::fromUtf8(kCardStyle)));
    });

    if (!iconUrl.isEmpty())
        requestIcon(iconUrl);
}

void ModCard::requestIcon(const QString &iconUrl)
{
    const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
                             QStringLiteral("/icons");
    QDir().mkpath(cacheDir);
    const QString ext = iconUrl.endsWith(QLatin1String(".png")) ? QStringLiteral(".png")
                                                                 : QStringLiteral(".jpg");
    const QString dest = cacheDir + QStringLiteral("/") + m_projectId + ext;
    if (QFile::exists(dest)) {
        setIconFromPath(dest);
        return;
    }
    HttpClient::instance().downloadFile(iconUrl, dest, [this, dest](bool ok, const QString &) {
        if (ok)
            setIconFromPath(dest);
    });
}

void ModCard::setIconFromPath(const QString &path)
{
    QPixmap pm(path);
    if (pm.isNull())
        return;
    m_icon->setPixmap(pm.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void ModCard::animateTo(const QString &targetHex)
{
    // Current card background: #0d0d18 (base) or whatever the animation left.
    QColor from(0x0d, 0x0d, 0x18);
    QColor to(targetHex);
    m_anim->stop();
    m_anim->setStartValue(from);
    m_anim->setEndValue(to);
    m_anim->start();
}

void ModCard::enterEvent(QEnterEvent *e)
{
    animateTo(QStringLiteral("#161626"));
    QFrame::enterEvent(e);
}

void ModCard::leaveEvent(QEvent *e)
{
    animateTo(QStringLiteral("#0d0d18"));
    QFrame::leaveEvent(e);
}

} // namespace sl
