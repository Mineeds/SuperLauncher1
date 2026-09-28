#include "ui/ResourceRow.h"
#include "net/HttpClient.h"

#include <QDir>
#include <QFile>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QStandardPaths>

namespace sl {

namespace {
const char *kRowStyle = R"(
QFrame#row {
    background:#0d0d18; border:1px solid #161626; border-radius:10px;
}
QFrame#row:hover { border:1px solid #1ad16f; }
QLabel { color:#ededf2; border:none; background:transparent; }
QLabel#title { font-weight:bold; font-size:14px; }
QLabel#author { color:#8a8a9e; font-size:11px; }
QLabel#desc { color:#8a8a9e; font-size:12px; }
QLabel#downloads { color:#1ad16f; font-size:11px; font-weight:bold; }
QLabel#tag {
    background:#161626; color:#8a8a9e; font-size:10px; border-radius:4px; padding:2px 7px;
}
QLabel#icon {
    background:#161626; border-radius:8px;
    min-width:52px; min-height:52px; max-width:52px; max-height:52px;
}
QPushButton#dl {
    background:#1ad16f; color:#08080f; border:none; border-radius:6px;
    padding:8px 16px; font-weight:bold; font-size:12px;
}
QPushButton#dl:hover { background:#17bc63; }
)";
}

ResourceRow::ResourceRow(const QString &projectId, const QString &slug, const QString &title,
                         const QString &author, const QString &desc, qint64 downloads,
                         const QStringList &tags, const QString &iconUrl, QWidget *parent)
    : QFrame(parent)
    , m_projectId(projectId)
    , m_slug(slug)
    , m_title(title)
{
    setObjectName(QStringLiteral("row"));
    setStyleSheet(kRowStyle);
    setFixedHeight(96);

    m_icon = new QLabel();
    m_icon->setObjectName(QStringLiteral("icon"));
    m_icon->setAlignment(Qt::AlignCenter);

    auto *titleRow = new QHBoxLayout();
    auto *titleLabel = new QLabel(title);
    titleLabel->setObjectName(QStringLiteral("title"));
    titleRow->addWidget(titleLabel);
    if (!author.isEmpty()) {
        auto *authorLabel = new QLabel(tr("by %1").arg(author));
        authorLabel->setObjectName(QStringLiteral("author"));
        titleRow->addWidget(authorLabel);
    }
    titleRow->addStretch(1);

    auto *descLabel = new QLabel(desc.left(120));
    descLabel->setObjectName(QStringLiteral("desc"));
    descLabel->setWordWrap(true);

    auto *tagRow = new QHBoxLayout();
    tagRow->setSpacing(5);
    for (const QString &t : tags) {
        auto *tag = new QLabel(t);
        tag->setObjectName(QStringLiteral("tag"));
        tagRow->addWidget(tag);
    }
    auto *downloadsLabel = new QLabel(tr("⬇ %1").arg(
        downloads >= 1000 ? QStringLiteral("%1K").arg(downloads / 1000) : QString::number(downloads)));
    downloadsLabel->setObjectName(QStringLiteral("downloads"));
    tagRow->addWidget(downloadsLabel);
    tagRow->addStretch(1);

    auto *textCol = new QVBoxLayout();
    textCol->setContentsMargins(0, 0, 0, 0);
    textCol->setSpacing(4);
    textCol->addLayout(titleRow);
    textCol->addWidget(descLabel);
    textCol->addLayout(tagRow);

    auto *dl = new QPushButton(tr("Завантажити"));
    dl->setObjectName(QStringLiteral("dl"));
    connect(dl, &QPushButton::clicked, this, &ResourceRow::downloadRequested);

    auto *lay = new QHBoxLayout(this);
    lay->setContentsMargins(14, 12, 14, 12);
    lay->setSpacing(14);
    lay->addWidget(m_icon);
    lay->addLayout(textCol, 1);
    lay->addWidget(dl, 0, Qt::AlignVCenter);

    if (!iconUrl.isEmpty())
        requestIcon(iconUrl);
}

void ResourceRow::requestIcon(const QString &iconUrl)
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

void ResourceRow::setIconFromPath(const QString &path)
{
    QPixmap pm(path);
    if (pm.isNull())
        return;
    m_icon->setPixmap(pm.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

} // namespace sl
