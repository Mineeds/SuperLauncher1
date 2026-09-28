#include "ui/ModsPage.h"
#include "ui/ModCard.h"
#include "core/Paths.h"
#include "core/Profiles.h"
#include "net/HttpClient.h"

#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QVBoxLayout>

namespace sl {

namespace {
const char *kPageStyle = R"(
QWidget { background:#08080f; }
QLabel { color:#ededf2; }
QLabel#title { color:#ededf2; font-size:22px; font-weight:bold; }
QLabel#hint { color:#8a8a9e; font-size:12px; }
QLineEdit, QComboBox {
    background:#0d0d18; color:#ededf2; border:1px solid #161626;
    border-radius:6px; padding:9px 12px; selection-background-color:#1ad16f;
}
QListWidget {
    background:#0d0d18; color:#ededf2; border:1px solid #161626;
    border-radius:8px; outline:0;
}
QListWidget::item { padding:10px 12px; }
QListWidget::item:selected { background:#161626; color:#1ad16f; }
QPushButton {
    background:#161626; color:#ededf2; border:none; border-radius:6px;
    padding:9px 16px; font-weight:bold;
}
QPushButton:hover { background:#1d1d30; }
)";
}

ModsPage::ModsPage(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet(kPageStyle);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(32, 28, 32, 28);
    root->setSpacing(12);

    auto *title = new QLabel(tr("Моды и ресурсы"));
    title->setObjectName(QStringLiteral("title"));
    root->addWidget(title);

    auto *topRow = new QHBoxLayout();
    m_search = new QLineEdit();
    m_search->setPlaceholderText(tr("Поиск: введите хотя бы одну букву..."));
    m_search->setClearButtonEnabled(true);
    m_type = new QComboBox();
    m_type->addItems({tr("Моды"), tr("Шейдеры"), tr("Ресурспаки"), tr("Модпаки")});
    topRow->addWidget(m_search, 1);
    topRow->addWidget(m_type);
    root->addLayout(topRow);

    m_status = new QLabel();
    m_status->setObjectName(QStringLiteral("hint"));
    root->addWidget(m_status);

    m_results = new QListWidget();
    m_results->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel); // smooth scroll
    root->addWidget(m_results, 1);

    auto *hint = new QLabel(tr("Двойной клик — установить (совместимость с версией игры "
                               "проверяется автоматически)."));
    hint->setObjectName(QStringLiteral("hint"));
    root->addWidget(hint);

    // Instant search: debounce, one letter is enough (as in the roadmap).
    m_debounce = new QTimer(this);
    m_debounce->setSingleShot(true);
    m_debounce->setInterval(350);
    connect(m_debounce, &QTimer::timeout, this, &ModsPage::search);
    connect(m_search, &QLineEdit::textChanged, m_debounce, qOverload<>(&QTimer::start));
    connect(m_type, &QComboBox::currentIndexChanged, this, [this]() {
        if (!m_search->text().isEmpty())
            m_debounce->start();
    });
    connect(m_results, &QListWidget::itemDoubleClicked, this,
            [this](QListWidgetItem *) { onItemActivated(); });
}

QString ModsPage::projectType() const
{
    switch (m_type->currentIndex()) {
    case 1: return QStringLiteral("shader");
    case 2: return QStringLiteral("resourcepack");
    case 3: return QStringLiteral("modpack");
    default: return QStringLiteral("mod");
    }
}

QString ModsPage::targetFolder() const
{
    switch (m_type->currentIndex()) {
    case 1: return Paths::gameDir() + QStringLiteral("/shaderpacks");
    case 2: return Paths::gameDir() + QStringLiteral("/resourcepacks");
    default: return Paths::gameDir() + QStringLiteral("/mods");
    }
}

QString ModsPage::selectedProfileMcVersion() const
{
    // Take the newest profile with a readable version id ("1.20.1", not custom names).
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
            const QRegularExpression re(QStringLiteral("^(\\d+)\\.(\\d+)(\\.(\\d+))?$"));
            if (re.match(id).hasMatch() && (best.isEmpty() || id > best))
                best = id;
        }
    }
    return best;
}

void ModsPage::search()
{
    const QString query = m_search->text().trimmed();
    m_results->clear();
    if (query.isEmpty()) {
        m_status->setText(tr("Введи название для поиска."));
        return;
    }

    const QString type = projectType();
    m_status->setText(tr("Ищу \"%1\"...").arg(query));

    QUrlQuery params;
    params.addQueryItem(QStringLiteral("query"), query);
    params.addQueryItem(QStringLiteral("limit"), QStringLiteral("30"));
    params.addQueryItem(QStringLiteral("index"), QStringLiteral("relevance"));
    // facets are JSON arrays: [["project_type:mod"]]
    params.addQueryItem(QStringLiteral("facets"),
                         QStringLiteral("[[\"project_type:%1\"]]").arg(type));

    const QUrl url(QStringLiteral("https://api.modrinth.com/v2/search?") +
                   QUrlQuery(params).toString(QUrl::FullyEncoded));

    HttpClient::instance().getJson(url.toString(), [this](const QJsonDocument &doc, const QString &error) {
        if (!error.isEmpty()) {
            m_status->setText(tr("Modrinth недоступен: %1").arg(error));
            return;
        }
        const QJsonArray hits = doc.object()[QStringLiteral("hits")].toArray();
        m_results->clear();
        for (const auto &h : hits) {
            const QJsonObject o = h.toObject();
            const QString title = o[QStringLiteral("title")].toString();
            const QString projectId = o[QStringLiteral("project_id")].toString();
            if (title.isEmpty() || projectId.isEmpty())
                continue;
            const QString desc = o[QStringLiteral("description")].toString();
            const qint64 downloads = o[QStringLiteral("downloads")].toInteger();
            const QString iconUrl = o[QStringLiteral("icon_url")].toString();

            auto *card = new ModCard(projectId, title, desc, downloads, iconUrl);
            auto *item = new QListWidgetItem(m_results);
            item->setSizeHint(card->sizeHint());
            // Keep ids for the install step.
            item->setData(Qt::UserRole, projectId);
            item->setData(Qt::UserRole + 1, o[QStringLiteral("slug")].toString());
            item->setData(Qt::UserRole + 2, title);
            m_results->setItemWidget(item, card);
            m_cards.insert(projectId, card);
        }
        m_status->setText(tr("Найдено: %1").arg(m_results->count()));
    });
}

void ModsPage::onItemActivated()
{
    auto *item = m_results->currentItem();
    if (!item)
        return;
    const QString projectId = item->data(Qt::UserRole).toString();
    const QString slug = item->data(Qt::UserRole + 1).toString();
    const QString title = item->data(Qt::UserRole + 2).toString();

    if (projectType() == QStringLiteral("modpack")) {
        // Full modpack import (overrides, manifests) is a later milestone.
        QDesktopServices::openUrl(
            QUrl(QStringLiteral("https://modrinth.com/modpack/%1").arg(slug)));
        m_status->setText(tr("Модпаки пока ставятся через сайт — открыл в браузере."));
        return;
    }

    // Find a build compatible with the installed game version.
    const QString mcVersion = selectedProfileMcVersion();
    QUrlQuery params;
    if (!mcVersion.isEmpty())
        params.addQueryItem(QStringLiteral("game_versions"),
                            QStringLiteral("[\"%1\"]").arg(mcVersion));
    const QUrl url =
        QStringLiteral("https://api.modrinth.com/v2/project/%1/version?%2")
            .arg(projectId, QUrlQuery(params).toString(QUrl::FullyEncoded));

    m_status->setText(tr("Проверяю совместимость..."));
    HttpClient::instance().getJson(url.toString(), [this, title, mcVersion](const QJsonDocument &doc,
                                                                 const QString &error) {
        if (!error.isEmpty()) {
            m_status->setText(tr("Не удалось получить сборки: %1").arg(error));
            return;
        }
        const QJsonArray versions = doc.array();
        if (versions.isEmpty()) {
            // Roadmap rule: block incompatible mods with a clear reason.
            m_status->setText(
                tr("⛔ \"%1\" не совместим с версией %2").arg(title, mcVersion));
            return;
        }
        // First entry = newest compatible build.
        const QJsonObject v = versions[0].toObject();
        const QJsonArray files = v[QStringLiteral("files")].toArray();
        QString url2;
        QString filename;
        for (const auto &f : files) {
            const QJsonObject fo = f.toObject();
            if (fo[QStringLiteral("primary")].toBool()) {
                url2 = fo[QStringLiteral("url")].toString();
                filename = fo[QStringLiteral("filename")].toString();
                break;
            }
        }
        if (url2.isEmpty() || filename.isEmpty()) {
            m_status->setText(tr("У сборки нет подходящего файла."));
            return;
        }

        const QString dest = targetFolder() + QStringLiteral("/") + filename;
        QDir().mkpath(targetFolder());
        m_status->setText(tr("Скачиваю %1...").arg(filename));
        HttpClient::instance().downloadFile(
            url2, dest,
            [this, filename](bool ok, const QString &err) {
                if (ok) {
                    m_status->setText(tr("✓ Установлено: %1").arg(filename));
                } else {
                    m_status->setText(tr("Ошибка скачивания: %1").arg(err));
                }
            });
    });
}

} // namespace sl
