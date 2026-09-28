#include "ui/ResourcesPage.h"
#include "ui/ResourceRow.h"
#include "core/Paths.h"
#include "core/Profiles.h"
#include "net/HttpClient.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFrame>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QVBoxLayout>

namespace sl {

namespace {
const char *kStyle = R"(
QWidget { background:#08080f; }
QLabel { color:#ededf2; background:transparent; }
QLabel#pageTitle { font-size:20px; font-weight:bold; }
QLabel#hint { color:#8a8a9e; font-size:11px; }
QLabel#filterHeader { color:#ededf2; font-size:12px; font-weight:bold; }
QLineEdit, QComboBox {
    background:#0d0d18; color:#ededf2; border:1px solid #161626;
    border-radius:6px; padding:9px 12px; selection-background-color:#1ad16f;
}
QPushButton#tab {
    background:transparent; color:#8a8a9e; border:none; border-bottom:2px solid transparent;
    padding:8px 4px; font-weight:bold; font-size:13px; margin-right:14px;
}
QPushButton#tab:checked, QPushButton#tabActive {
    color:#1ad16f; border-bottom:2px solid #1ad16f;
}
QPushButton#chip {
    background:#0d0d18; color:#ededf2; border:1px solid #161626; border-radius:14px;
    padding:6px 12px; font-size:12px;
}
QPushButton#chip:checked { background:#1ad16f; color:#08080f; border:1px solid #1ad16f; }
QCheckBox { color:#ededf2; font-size:12px; }
QFrame#sidePanel { background:#0d0d18; border:1px solid #161626; border-radius:10px; }
QPushButton#reset {
    background:#e0473a; color:#ededf2; border:none; border-radius:6px;
    padding:8px 0px; font-weight:bold; font-size:12px;
}
QPushButton#reset:hover { background:#c73e33; }
QPushButton#pager {
    background:#161626; color:#ededf2; border:none; border-radius:6px; padding:6px 12px;
}
QPushButton#pager:disabled { color:#4a4a5a; }
)";

struct TabDef { const char *label; const char *type; };
const TabDef kTabs[5] = {
    {"Модпаки", "modpack"},
    {"Моди", "mod"},
    {"Ресурс-паки", "resourcepack"},
    {"Дата-паки", "datapack"},
    {"Шейдери", "shader"},
};

struct ChipDef { const char *label; const char *category; };
const ChipDef kChips[8] = {
    {"Усі", ""}, {"Популярні", ""}, {"Пригода", "adventure"}, {"Виживання", "survival"},
    {"Технічні", "technology"}, {"RPG", "rpg"}, {"Квести", "quests"}, {"PvP", "pvp"},
};
} // namespace

ResourcesPage::ResourcesPage(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet(kStyle);

    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(28, 24, 28, 24);
    root->setSpacing(20);

    // ---- Main column ----
    auto *mainCol = new QVBoxLayout();
    mainCol->setSpacing(12);

    auto *title = new QLabel(tr("Ресурси"));
    title->setObjectName(QStringLiteral("pageTitle"));
    mainCol->addWidget(title);

    auto *tabRow = new QHBoxLayout();
    for (int i = 0; i < 5; ++i) {
        auto *btn = new QPushButton(tr(kTabs[i].label));
        btn->setObjectName(QStringLiteral("tab"));
        btn->setCheckable(true);
        btn->setChecked(i == 0);
        btn->setCursor(Qt::PointingHandCursor);
        connect(btn, &QPushButton::clicked, this, [this, i]() { selectTab(i); });
        m_tabButtons[i] = btn;
        tabRow->addWidget(btn);
    }
    tabRow->addStretch(1);
    mainCol->addLayout(tabRow);

    auto *searchRow = new QHBoxLayout();
    m_search = new QLineEdit();
    m_search->setPlaceholderText(tr("Пошук: введіть хоча б одну букву..."));
    m_search->setClearButtonEnabled(true);
    searchRow->addWidget(m_search, 1);
    mainCol->addLayout(searchRow);

    auto *chipRow = new QHBoxLayout();
    chipRow->setSpacing(6);
    for (int i = 0; i < 8; ++i) {
        auto *chip = new QPushButton(tr(kChips[i].label));
        chip->setObjectName(QStringLiteral("chip"));
        chip->setCheckable(true);
        chip->setChecked(i == 0);
        chip->setCursor(Qt::PointingHandCursor);
        connect(chip, &QPushButton::clicked, this, [this, i]() {
            for (int j = 0; j < 8; ++j)
                m_category->blockSignals(true);
            m_category->setCurrentIndex(i < m_category->count() ? i : 0);
            m_category->blockSignals(false);
            search();
        });
        chipRow->addWidget(chip);
    }
    chipRow->addStretch(1);
    mainCol->addLayout(chipRow);

    m_status = new QLabel();
    m_status->setObjectName(QStringLiteral("hint"));
    mainCol->addWidget(m_status);

    auto *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *resultsHolder = new QWidget();
    m_resultsLayout = new QVBoxLayout(resultsHolder);
    m_resultsLayout->setSpacing(8);
    m_resultsLayout->setContentsMargins(0, 0, 0, 0);
    m_resultsLayout->addStretch(1);
    scroll->setWidget(resultsHolder);
    mainCol->addWidget(scroll, 1);

    auto *pageRow = new QHBoxLayout();
    auto *prevBtn = new QPushButton(tr("← Назад"));
    prevBtn->setObjectName(QStringLiteral("pager"));
    connect(prevBtn, &QPushButton::clicked, this, [this]() {
        m_offset = qMax(0, m_offset - m_perPage->currentText().toInt());
        search();
    });
    m_pageLabel = new QLabel();
    m_pageLabel->setObjectName(QStringLiteral("hint"));
    auto *nextBtn = new QPushButton(tr("Вперед →"));
    nextBtn->setObjectName(QStringLiteral("pager"));
    connect(nextBtn, &QPushButton::clicked, this, [this]() {
        m_offset += m_perPage->currentText().toInt();
        search();
    });
    pageRow->addWidget(prevBtn);
    pageRow->addWidget(m_pageLabel);
    pageRow->addWidget(nextBtn);
    pageRow->addStretch(1);
    mainCol->addLayout(pageRow);

    root->addLayout(mainCol, 1);

    // ---- Right filters panel ----
    auto *side = new QFrame();
    side->setObjectName(QStringLiteral("sidePanel"));
    side->setFixedWidth(220);
    auto *sideLay = new QVBoxLayout(side);
    sideLay->setContentsMargins(16, 16, 16, 16);
    sideLay->setSpacing(10);

    auto *sortHeader = new QLabel(tr("Сортування"));
    sortHeader->setObjectName(QStringLiteral("filterHeader"));
    sideLay->addWidget(sortHeader);
    m_sort = new QComboBox();
    m_sort->addItem(tr("За релевантністю"), QStringLiteral("relevance"));
    m_sort->addItem(tr("За завантаженнями"), QStringLiteral("downloads"));
    m_sort->addItem(tr("Новизна"), QStringLiteral("newest"));
    connect(m_sort, &QComboBox::currentIndexChanged, this, [this]() { m_offset = 0; search(); });
    sideLay->addWidget(m_sort);

    auto *verHeader = new QLabel(tr("Версія Minecraft"));
    verHeader->setObjectName(QStringLiteral("filterHeader"));
    sideLay->addWidget(verHeader);
    const QStringList versions{"1.21.1", "1.20.1", "1.19.4", "1.18.2"};
    for (const QString &v : versions) {
        auto *cb = new QCheckBox(v);
        connect(cb, &QCheckBox::toggled, this, [this, v](bool on) {
            if (on) m_mcVersionFilters.insert(v); else m_mcVersionFilters.remove(v);
            m_offset = 0;
            search();
        });
        sideLay->addWidget(cb);
    }

    auto *typeHeader = new QLabel(tr("Тип"));
    typeHeader->setObjectName(QStringLiteral("filterHeader"));
    sideLay->addWidget(typeHeader);
    const QStringList loaders{"Fabric", "Forge", "Quilt", "NeoForge"};
    for (const QString &l : loaders) {
        auto *cb = new QCheckBox(l);
        const QString slug = l.toLower();
        connect(cb, &QCheckBox::toggled, this, [this, slug](bool on) {
            if (on) m_loaderFilters.insert(slug); else m_loaderFilters.remove(slug);
            m_offset = 0;
            search();
        });
        sideLay->addWidget(cb);
    }

    auto *catHeader = new QLabel(tr("Категорія"));
    catHeader->setObjectName(QStringLiteral("filterHeader"));
    sideLay->addWidget(catHeader);
    m_category = new QComboBox();
    for (const auto &c : kChips)
        m_category->addItem(tr(c.label), QString::fromLatin1(c.category));
    connect(m_category, &QComboBox::currentIndexChanged, this, [this]() { m_offset = 0; search(); });
    sideLay->addWidget(m_category);

    auto *resetBtn = new QPushButton(tr("Скинути фільтри"));
    resetBtn->setObjectName(QStringLiteral("reset"));
    connect(resetBtn, &QPushButton::clicked, this, &ResourcesPage::resetFilters);
    sideLay->addWidget(resetBtn);

    auto *perPageHeader = new QLabel(tr("Відображати"));
    perPageHeader->setObjectName(QStringLiteral("filterHeader"));
    sideLay->addWidget(perPageHeader);
    m_perPage = new QComboBox();
    m_perPage->addItems({"10", "20", "30"});
    m_perPage->setCurrentText(QStringLiteral("10"));
    connect(m_perPage, &QComboBox::currentIndexChanged, this, [this]() { m_offset = 0; search(); });
    sideLay->addWidget(m_perPage);
    sideLay->addStretch(1);

    root->addWidget(side);

    // Instant search: debounce, one letter is enough.
    m_debounce = new QTimer(this);
    m_debounce->setSingleShot(true);
    m_debounce->setInterval(350);
    connect(m_debounce, &QTimer::timeout, this, [this]() { m_offset = 0; search(); });
    connect(m_search, &QLineEdit::textChanged, m_debounce, qOverload<>(&QTimer::start));

    search();
}

void ResourcesPage::selectTab(int index)
{
    m_tabIndex = index;
    for (int i = 0; i < 5; ++i)
        m_tabButtons[i]->setChecked(i == index);
    m_offset = 0;
    search();
}

QString ResourcesPage::projectType() const
{
    return QString::fromLatin1(kTabs[m_tabIndex].type);
}

QString ResourcesPage::targetFolder() const
{
    const QString type = projectType();
    if (type == QStringLiteral("shader"))
        return Paths::gameDir() + QStringLiteral("/shaderpacks");
    if (type == QStringLiteral("resourcepack"))
        return Paths::gameDir() + QStringLiteral("/resourcepacks");
    if (type == QStringLiteral("datapack"))
        return Paths::gameDir() + QStringLiteral("/datapacks");
    return Paths::gameDir() + QStringLiteral("/mods");
}

QString ResourcesPage::selectedProfileMcVersion() const
{
    if (!m_mcVersionFilters.isEmpty())
        return *m_mcVersionFilters.begin();
    QString best;
    for (const auto &p : Profiles::instance().all()) {
        const QDir dir(p.dir);
        const QStringList jsons = dir.entryList({QStringLiteral("*.json")}, QDir::Files);
        for (const QString &f : jsons) {
            QFile file(dir.filePath(f));
            if (!file.open(QIODevice::ReadOnly))
                continue;
            const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            file.close();
            const QString id = doc.object()[QStringLiteral("id")].toString();
            static const QRegularExpression re(QStringLiteral("^(\\d+)\\.(\\d+)(\\.(\\d+))?$"));
            if (re.match(id).hasMatch() && (best.isEmpty() || id > best))
                best = id;
        }
    }
    return best;
}

QString ResourcesPage::buildFacets() const
{
    // Modrinth facets: outer array = AND, inner array = OR.
    QStringList groups;
    groups << QStringLiteral("[\"project_type:%1\"]").arg(projectType());

    if (!m_mcVersionFilters.isEmpty()) {
        QStringList ors;
        for (const QString &v : m_mcVersionFilters)
            ors << QStringLiteral("\"versions:%1\"").arg(v);
        groups << QStringLiteral("[%1]").arg(ors.join(QStringLiteral(",")));
    }
    if (!m_loaderFilters.isEmpty()) {
        QStringList ors;
        for (const QString &l : m_loaderFilters)
            ors << QStringLiteral("\"categories:%1\"").arg(l);
        groups << QStringLiteral("[%1]").arg(ors.join(QStringLiteral(",")));
    }
    const QString cat = m_category ? m_category->currentData().toString() : QString();
    if (!cat.isEmpty())
        groups << QStringLiteral("[\"categories:%1\"]").arg(cat);

    return QStringLiteral("[%1]").arg(groups.join(QStringLiteral(",")));
}

void ResourcesPage::search()
{
    QLayoutItem *item;
    while ((item = m_resultsLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    const QString query = m_search->text().trimmed();
    const QString sortIndex = m_sort ? m_sort->currentData().toString() : QStringLiteral("relevance");
    const int perPage = m_perPage ? m_perPage->currentText().toInt() : 10;

    m_status->setText(query.isEmpty() ? tr("Показую популярне...") : tr("Шукаю \"%1\"...").arg(query));

    QUrlQuery params;
    if (!query.isEmpty())
        params.addQueryItem(QStringLiteral("query"), query);
    params.addQueryItem(QStringLiteral("limit"), QString::number(perPage));
    params.addQueryItem(QStringLiteral("offset"), QString::number(m_offset));
    params.addQueryItem(QStringLiteral("index"), sortIndex);
    params.addQueryItem(QStringLiteral("facets"), buildFacets());

    const QUrl url(QStringLiteral("https://api.modrinth.com/v2/search?") +
                   QUrlQuery(params).toString(QUrl::FullyEncoded));

    HttpClient::instance().getJson(url.toString(), [this](const QJsonDocument &doc, const QString &error) {
        if (!error.isEmpty()) {
            m_status->setText(tr("Modrinth недоступний: %1").arg(error));
            return;
        }
        const QJsonObject root = doc.object();
        const QJsonArray hits = root[QStringLiteral("hits")].toArray();
        const int total = root[QStringLiteral("total_hits")].toInt();

        for (const auto &h : hits) {
            const QJsonObject o = h.toObject();
            const QString title = o[QStringLiteral("title")].toString();
            const QString projectId = o[QStringLiteral("project_id")].toString();
            if (title.isEmpty() || projectId.isEmpty())
                continue;
            const QString slug = o[QStringLiteral("slug")].toString();
            const QString author = o[QStringLiteral("author")].toString();
            const QString desc = o[QStringLiteral("description")].toString();
            const qint64 downloads = o[QStringLiteral("downloads")].toInteger();
            const QString iconUrl = o[QStringLiteral("icon_url")].toString();
            QStringList tags;
            for (const auto &c : o[QStringLiteral("categories")].toArray()) {
                tags << c.toString();
                if (tags.size() >= 3)
                    break;
            }

            auto *row = new ResourceRow(projectId, slug, title, author, desc, downloads,
                                        tags, iconUrl);
            connect(row, &ResourceRow::downloadRequested, this,
                    [this, projectId, slug, title]() { install(projectId, slug, title); });
            m_resultsLayout->insertWidget(m_resultsLayout->count() - 1, row);
        }
        if (hits.isEmpty()) {
            auto *empty = new QLabel(tr("Нічого не знайдено."));
            empty->setObjectName(QStringLiteral("hint"));
            m_resultsLayout->insertWidget(m_resultsLayout->count() - 1, empty);
        }
        m_status->setText(tr("Знайдено: %1").arg(total));
        m_pageLabel->setText(tr("Показано %1–%2 з %3")
                                 .arg(m_offset + 1)
                                 .arg(m_offset + hits.size())
                                 .arg(total));
    });
}

void ResourcesPage::resetFilters()
{
    m_search->clear();
    m_mcVersionFilters.clear();
    m_loaderFilters.clear();
    m_offset = 0;
    for (auto *cb : findChildren<QCheckBox *>())
        cb->setChecked(false);
    m_category->setCurrentIndex(0);
    m_sort->setCurrentIndex(0);
    m_perPage->setCurrentIndex(0);
    search();
}

void ResourcesPage::install(const QString &projectId, const QString &slug, const QString &title)
{
    if (projectType() == QStringLiteral("modpack")) {
        // Full modpack import (overrides, manifests) is a later milestone.
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://modrinth.com/modpack/%1").arg(slug)));
        m_status->setText(tr("Модпаки поки встановлюються через сайт — відкрив у браузері."));
        return;
    }

    const QString mcVersion = selectedProfileMcVersion();
    QUrlQuery params;
    if (!mcVersion.isEmpty())
        params.addQueryItem(QStringLiteral("game_versions"), QStringLiteral("[\"%1\"]").arg(mcVersion));
    const QUrl url = QStringLiteral("https://api.modrinth.com/v2/project/%1/version?%2")
                         .arg(projectId, QUrlQuery(params).toString(QUrl::FullyEncoded));

    m_status->setText(tr("Перевіряю сумісність..."));
    HttpClient::instance().getJson(url.toString(), [this, title, mcVersion](const QJsonDocument &doc,
                                                                            const QString &error) {
        if (!error.isEmpty()) {
            m_status->setText(tr("Не вдалося отримати збірки: %1").arg(error));
            return;
        }
        const QJsonArray versions = doc.array();
        if (versions.isEmpty()) {
            m_status->setText(tr("⛔ \"%1\" не сумісний з версією %2").arg(title, mcVersion));
            return;
        }
        const QJsonObject v = versions[0].toObject();
        const QJsonArray files = v[QStringLiteral("files")].toArray();
        QString fileUrl, filename;
        for (const auto &f : files) {
            const QJsonObject fo = f.toObject();
            if (fo[QStringLiteral("primary")].toBool()) {
                fileUrl = fo[QStringLiteral("url")].toString();
                filename = fo[QStringLiteral("filename")].toString();
                break;
            }
        }
        if (fileUrl.isEmpty() || filename.isEmpty()) {
            m_status->setText(tr("У збірки немає придатного файлу."));
            return;
        }

        const QString dest = targetFolder() + QStringLiteral("/") + filename;
        QDir().mkpath(targetFolder());
        m_status->setText(tr("Завантажую %1...").arg(filename));
        HttpClient::instance().downloadFile(fileUrl, dest, [this, filename](bool ok, const QString &err) {
            m_status->setText(ok ? tr("✓ Встановлено: %1").arg(filename)
                                 : tr("Помилка завантаження: %1").arg(err));
        });
    });
}

} // namespace sl
