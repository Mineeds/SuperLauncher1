#include "ui/DashboardContent.h"
#include "core/Profiles.h"
#include "core/ProfileInfo.h"
#include "core/Launcher.h"
#include "net/HttpClient.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QUrl>
#include <QVBoxLayout>

namespace sl {

namespace {

const char *kStyle = R"(
QWidget { background:#08080f; }
QLabel { color:#ededf2; background:transparent; }
QLabel#sectionTitle { font-size:16px; font-weight:bold; color:#ededf2; }
QLabel#sectionLink { color:#8a8a9e; font-size:12px; }
QLabel#muted { color:#8a8a9e; font-size:12px; }
QFrame#hero {
    border-radius:14px;
    background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
        stop:0 #0c1a12, stop:0.45 #16241a, stop:0.75 #3a3421, stop:1 #6b5326);
}
QLabel#heroTitle { color:#f4f6f2; font-size:30px; font-weight:bold; }
QLabel#heroSub { color:#c9cdd3; font-size:13px; }
QPushButton#play {
    background:#1ad16f; color:#08080f; border:none; border-radius:8px;
    padding:10px 22px; font-weight:bold; font-size:13px;
}
QPushButton#play:hover { background:#17bc63; }
QPushButton#ghostPill {
    background:rgba(255,255,255,18); color:#ededf2; border:1px solid rgba(255,255,255,40);
    border-radius:14px; padding:6px 14px; font-size:12px;
}
QFrame#card {
    background:#0d0d18; border:1px solid #161626; border-radius:10px;
}
QFrame#card:hover { border:1px solid #1ad16f; }
QLabel#cardIcon {
    background:#161626; border-radius:8px;
    min-width:40px; min-height:40px; max-width:40px; max-height:40px;
}
QLabel#cardTitle { font-weight:bold; font-size:13px; }
QLabel#cardMeta { color:#8a8a9e; font-size:11px; }
QPushButton#miniPlay {
    background:#1ad16f; color:#08080f; border:none; border-radius:6px;
    padding:6px 0px; font-weight:bold; font-size:12px;
}
QPushButton#miniPlay:hover { background:#17bc63; }
QPushButton#download {
    background:#1ad16f; color:#08080f; border:none; border-radius:6px;
    padding:6px 14px; font-weight:bold; font-size:12px;
}
QPushButton#download:hover { background:#17bc63; }
QFrame#statTile { background:#0d0d18; border:1px solid #161626; border-radius:8px; }
QLabel#statNum { font-size:15px; font-weight:bold; color:#ededf2; }
QLabel#statLabel { font-size:10px; color:#8a8a9e; }
QFrame#newsCard { background:#0d0d18; border:1px solid #161626; border-radius:10px; }
QFrame#newsStripe { border-radius:2px; min-height:3px; max-height:3px; }
QLabel#newsTitle { font-weight:bold; font-size:13px; }
QLabel#newsRead { color:#1ad16f; font-size:12px; }
)";

QFrame *makeStatTile(const QString &num, const QString &label)
{
    auto *tile = new QFrame();
    tile->setObjectName(QStringLiteral("statTile"));
    tile->setFixedSize(78, 62);
    auto *lay = new QVBoxLayout(tile);
    lay->setContentsMargins(8, 8, 8, 8);
    lay->setSpacing(2);
    auto *n = new QLabel(num);
    n->setObjectName(QStringLiteral("statNum"));
    auto *l = new QLabel(label);
    l->setObjectName(QStringLiteral("statLabel"));
    l->setWordWrap(true);
    lay->addWidget(n);
    lay->addWidget(l);
    lay->addStretch(1);
    return tile;
}

} // namespace

DashboardContent::DashboardContent(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet(kStyle);

    auto *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *inner = new QWidget();
    auto *root = new QVBoxLayout(inner);
    root->setContentsMargins(28, 24, 28, 24);
    root->setSpacing(18);

    root->addWidget(buildHero());
    root->addWidget(buildRecentRow());
    root->addWidget(buildModpacksRow());
    root->addWidget(buildNewsRow());
    root->addStretch(1);

    scroll->setWidget(inner);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(scroll);

    loadPopularModpacks();
}

QWidget *DashboardContent::buildHero()
{
    auto *hero = new QFrame();
    hero->setObjectName(QStringLiteral("hero"));
    hero->setMinimumHeight(200);

    auto *lay = new QVBoxLayout(hero);
    lay->setContentsMargins(28, 22, 28, 22);

    auto *topRow = new QHBoxLayout();
    topRow->addStretch(1);
    auto *announceBtn = new QPushButton(tr("+ анонса"));
    announceBtn->setObjectName(QStringLiteral("ghostPill"));
    topRow->addWidget(announceBtn);
    lay->addLayout(topRow);
    lay->addStretch(1);

    auto *title = new QLabel(tr("Minecraft\nнового поколіня —"));
    title->setObjectName(QStringLiteral("heroTitle"));
    lay->addWidget(title);

    auto *sub = new QLabel(tr("Запускай будь-яку версію. Встановлюй моди.\nГрай так, як хочеш."));
    sub->setObjectName(QStringLiteral("heroSub"));
    lay->addWidget(sub);

    lay->addSpacing(6);
    auto *playBtn = new QPushButton(tr("▶ Грати"));
    playBtn->setObjectName(QStringLiteral("play"));
    playBtn->setCursor(Qt::PointingHandCursor);
    connect(playBtn, &QPushButton::clicked, this, [this]() {
        const auto profiles = Profiles::instance().all();
        if (!profiles.isEmpty())
            Launcher::instance().launch(profiles.first());
    });
    lay->addWidget(playBtn, 0, Qt::AlignLeft);

    return hero;
}

QWidget *DashboardContent::buildStatsPanel()
{
    auto *panel = new QWidget();
    auto *lay = new QGridLayout(panel);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(6);
    lay->addWidget(makeStatTile(QStringLiteral("0"), tr("Скрап")), 0, 0);
    lay->addWidget(makeStatTile(QStringLiteral("0"), tr("Моди")), 0, 1);
    lay->addWidget(makeStatTile(QStringLiteral("0"), tr("Текстур")), 1, 0);
    lay->addWidget(makeStatTile(QStringLiteral("—"), tr("Ресурс паки")), 1, 1);
    return panel;
}

QWidget *DashboardContent::buildRecentRow()
{
    auto *wrap = new QWidget();
    auto *v = new QVBoxLayout(wrap);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(10);

    auto *header = new QHBoxLayout();
    auto *title = new QLabel(tr("▸ Останній запуск"));
    title->setObjectName(QStringLiteral("sectionTitle"));
    header->addWidget(title);
    header->addStretch(1);
    auto *link = new QLabel(tr("Всі версії →"));
    link->setObjectName(QStringLiteral("sectionLink"));
    header->addWidget(link);
    header->addSpacing(16);
    auto *statsLabel = new QLabel(tr("🧩 Ваша статистика"));
    statsLabel->setObjectName(QStringLiteral("sectionLink"));
    header->addWidget(statsLabel);
    v->addLayout(header);

    auto *bodyRow = new QHBoxLayout();
    m_recentRow = new QHBoxLayout();
    m_recentRow->setSpacing(10);
    auto *cardsHolder = new QWidget();
    cardsHolder->setLayout(m_recentRow);
    bodyRow->addWidget(cardsHolder, 1);
    bodyRow->addWidget(buildStatsPanel());
    v->addLayout(bodyRow);

    refreshRecent();
    return wrap;
}

void DashboardContent::refreshRecent()
{
    QLayoutItem *item;
    while ((item = m_recentRow->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    const auto profiles = Profiles::instance().all();
    if (profiles.isEmpty()) {
        auto *empty = new QLabel(tr("Немає версій — створи одну на сторінці «Версії»."));
        empty->setObjectName(QStringLiteral("muted"));
        m_recentRow->addWidget(empty);
        return;
    }

    int shown = 0;
    for (const auto &p : profiles) {
        if (shown++ >= 4)
            break;
        const ProfileInfo info = detectProfileInfo(p.dir, p.name);

        auto *card = new QFrame();
        card->setObjectName(QStringLiteral("card"));
        card->setFixedHeight(120);
        card->setMinimumWidth(150);
        auto *lay = new QVBoxLayout(card);
        lay->setContentsMargins(12, 10, 12, 10);
        lay->setSpacing(4);

        auto *icon = new QLabel();
        icon->setObjectName(QStringLiteral("cardIcon"));
        lay->addWidget(icon);

        auto *name = new QLabel(p.name);
        name->setObjectName(QStringLiteral("cardTitle"));
        name->setWordWrap(true);
        lay->addWidget(name);

        auto *meta = new QLabel(QStringLiteral("%1 • %2").arg(
            info.loader, info.mcVersion.isEmpty() ? tr("?") : info.mcVersion));
        meta->setObjectName(QStringLiteral("cardMeta"));
        lay->addWidget(meta);

        const QString last = p.lastPlayed.isEmpty()
            ? tr("Ніколи")
            : QDateTime::fromString(p.lastPlayed, Qt::ISODate).toString(QStringLiteral("dd.MM HH:mm"));
        auto *lastLabel = new QLabel(tr("Останній запуск: %1").arg(last));
        lastLabel->setObjectName(QStringLiteral("cardMeta"));
        lay->addWidget(lastLabel);

        lay->addStretch(1);
        auto *playBtn = new QPushButton(tr("▶ Грати"));
        playBtn->setObjectName(QStringLiteral("miniPlay"));
        const QString pname = p.name;
        connect(playBtn, &QPushButton::clicked, this, [pname]() {
            for (const auto &pp : Profiles::instance().all()) {
                if (pp.name == pname) {
                    Launcher::instance().launch(pp);
                    return;
                }
            }
        });
        lay->addWidget(playBtn);

        m_recentRow->addWidget(card);
    }
    m_recentRow->addStretch(1);
}

QWidget *DashboardContent::buildModpacksRow()
{
    auto *wrap = new QWidget();
    auto *v = new QVBoxLayout(wrap);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(10);

    auto *header = new QHBoxLayout();
    auto *title = new QLabel(tr("🔥 Популярні модпаки"));
    title->setObjectName(QStringLiteral("sectionTitle"));
    header->addWidget(title);
    header->addStretch(1);
    auto *link = new QLabel(tr("Всі модпаки →"));
    link->setObjectName(QStringLiteral("sectionLink"));
    header->addWidget(link);
    v->addLayout(header);

    m_modpacksRow = new QHBoxLayout();
    m_modpacksRow->setSpacing(10);
    auto *holder = new QWidget();
    holder->setLayout(m_modpacksRow);
    v->addWidget(holder);

    auto *loading = new QLabel(tr("Завантаження..."));
    loading->setObjectName(QStringLiteral("muted"));
    m_modpacksRow->addWidget(loading);

    return wrap;
}

void DashboardContent::loadPopularModpacks()
{
    const QUrl url(QStringLiteral(
        "https://api.modrinth.com/v2/search?limit=4&index=downloads&"
        "facets=%5B%5B%22project_type%3Amodpack%22%5D%5D"));

    HttpClient::instance().getJson(url.toString(), [this](const QJsonDocument &doc, const QString &error) {
        QLayoutItem *item;
        while ((item = m_modpacksRow->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        if (!error.isEmpty()) {
            auto *err = new QLabel(tr("Не вдалося завантажити модпаки."));
            err->setObjectName(QStringLiteral("muted"));
            m_modpacksRow->addWidget(err);
            return;
        }
        const QJsonArray hits = doc.object()[QStringLiteral("hits")].toArray();
        for (const auto &h : hits) {
            const QJsonObject o = h.toObject();
            const QString title = o[QStringLiteral("title")].toString();
            const QString desc = o[QStringLiteral("description")].toString();
            const QString slug = o[QStringLiteral("slug")].toString();
            const qint64 downloads = o[QStringLiteral("downloads")].toInteger();

            auto *card = new QFrame();
            card->setObjectName(QStringLiteral("card"));
            card->setMinimumWidth(190);
            card->setMaximumHeight(150);
            auto *lay = new QVBoxLayout(card);
            lay->setContentsMargins(12, 10, 12, 10);
            lay->setSpacing(4);

            auto *icon = new QLabel();
            icon->setObjectName(QStringLiteral("cardIcon"));
            lay->addWidget(icon);

            auto *name = new QLabel(title);
            name->setObjectName(QStringLiteral("cardTitle"));
            name->setWordWrap(true);
            lay->addWidget(name);

            auto *descLabel = new QLabel(desc.left(70));
            descLabel->setObjectName(QStringLiteral("cardMeta"));
            descLabel->setWordWrap(true);
            lay->addWidget(descLabel);

            auto *meta = new QLabel(tr("⬇ %1K завантажень").arg(downloads / 1000));
            meta->setObjectName(QStringLiteral("cardMeta"));
            lay->addWidget(meta);
            lay->addStretch(1);

            auto *dl = new QPushButton(tr("Завантажити"));
            dl->setObjectName(QStringLiteral("download"));
            connect(dl, &QPushButton::clicked, this, [slug]() {
                QDesktopServices::openUrl(QUrl(QStringLiteral("https://modrinth.com/modpack/%1").arg(slug)));
            });
            lay->addWidget(dl);

            m_modpacksRow->addWidget(card);
        }
        m_modpacksRow->addStretch(1);
    });
}

QWidget *DashboardContent::buildNewsRow()
{
    auto *wrap = new QWidget();
    auto *v = new QVBoxLayout(wrap);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(10);

    auto *title = new QLabel(tr("Новини"));
    title->setObjectName(QStringLiteral("sectionTitle"));
    v->addWidget(title);

    auto *row = new QHBoxLayout();
    row->setSpacing(10);

    struct NewsItem { QString stripe; QString title; };
    const NewsItem items[] = {
        {QStringLiteral("#1ad16f"), tr("Часті запитання")},
        {QStringLiteral("#3a8bff"), tr("Готовий почати?")},
    };
    for (const auto &n : items) {
        auto *card = new QFrame();
        card->setObjectName(QStringLiteral("newsCard"));
        card->setMinimumHeight(74);
        auto *lay = new QVBoxLayout(card);
        lay->setContentsMargins(14, 10, 14, 10);
        lay->setSpacing(6);

        auto *stripe = new QFrame();
        stripe->setObjectName(QStringLiteral("newsStripe"));
        stripe->setStyleSheet(QStringLiteral("background:%1;").arg(n.stripe));
        lay->addWidget(stripe);

        auto *t = new QLabel(n.title);
        t->setObjectName(QStringLiteral("newsTitle"));
        lay->addWidget(t);

        auto *read = new QLabel(tr("Читати →"));
        read->setObjectName(QStringLiteral("newsRead"));
        lay->addWidget(read);

        row->addWidget(card, 1);
    }
    v->addLayout(row);
    return wrap;
}

} // namespace sl
