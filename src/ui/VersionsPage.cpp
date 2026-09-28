#include "ui/VersionsPage.h"
#include "ui/DashboardContent.h"
#include "core/Profiles.h"
#include "core/ProfileInfo.h"
#include "core/VersionManager.h"
#include "core/Launcher.h"
#include "core/Paths.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QMenu>
#include <QProgressBar>
#include <QPointer>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

namespace sl {

namespace {
const char *kPageStyle = R"(
QWidget { background:#08080f; }
QLabel { color:#ededf2; }
QLabel#title { color:#ededf2; font-size:16px; font-weight:bold; }
QLabel#hint { color:#8a8a9e; font-size:11px; }
QListWidget {
    background:#0d0d18; color:#ededf2; border:1px solid #161626;
    border-radius:8px; outline:0;
}
QListWidget::item { padding:2px; border-bottom:1px solid #12121e; }
QListWidget::item:selected { background:#161626; border-left:3px solid #1ad16f; }
QPushButton {
    background:#161626; color:#ededf2; border:none; border-radius:6px;
    padding:9px 16px; font-weight:bold;
}
QPushButton:hover { background:#1d1d30; }
QPushButton:disabled { color:#5a5a6e; }
QPushButton#iconBtn {
    background:#1ad16f; color:#08080f; border-radius:6px;
    min-width:26px; max-width:26px; min-height:26px; max-height:26px;
    padding:0px; font-size:14px;
}
QPushButton#iconBtn:hover { background:#17bc63; }
QPushButton#closeBtn {
    background:#161626; color:#8a8a9e; border-radius:6px;
    min-width:26px; max-width:26px; min-height:26px; max-height:26px;
    padding:0px; font-size:12px;
}
QPushButton#closeBtn:hover { background:#1d1d30; color:#ededf2; }
QLineEdit {
    background:#0d0d18; color:#ededf2; border:1px solid #161626;
    border-radius:6px; padding:8px 10px; selection-background-color:#1ad16f;
}
QRadioButton { color:#ededf2; }
)";

QColor loaderColor(const QString &loader)
{
    if (loader == QStringLiteral("Fabric")) return QColor(0x3a, 0x8b, 0xff);
    if (loader == QStringLiteral("Forge")) return QColor(0xb0, 0x6a, 0x3a);
    if (loader == QStringLiteral("NeoForge")) return QColor(0xe0, 0x7a, 0x2a);
    if (loader == QStringLiteral("Quilt")) return QColor(0x8a, 0x4a, 0xd1);
    return QColor(0x6a, 0x6a, 0x7a); // Vanilla
}

QWidget *makeVersionRow(const Profile &p)
{
    const ProfileInfo info = detectProfileInfo(p.dir, p.name);
    auto *row = new QWidget();
    auto *lay = new QVBoxLayout(row);
    lay->setContentsMargins(10, 8, 10, 8);
    lay->setSpacing(3);

    auto *name = new QLabel(p.name);
    name->setStyleSheet(QStringLiteral("color:#ededf2; font-weight:bold; font-size:13px; background:transparent;"));
    lay->addWidget(name);

    auto *metaRow = new QHBoxLayout();
    metaRow->setSpacing(6);
    auto *badge = new QLabel(info.loader);
    const QColor c = loaderColor(info.loader);
    badge->setStyleSheet(QStringLiteral(
        "background:%1; color:#08080f; font-size:10px; font-weight:bold; "
        "border-radius:4px; padding:2px 6px;").arg(c.name()));
    metaRow->addWidget(badge);
    auto *mc = new QLabel(QStringLiteral("MC %1").arg(info.mcVersion.isEmpty() ? QStringLiteral("?") : info.mcVersion));
    mc->setStyleSheet(QStringLiteral("color:#8a8a9e; font-size:11px; background:transparent;"));
    metaRow->addWidget(mc);
    if (p.imported) {
        auto *imp = new QLabel(QChar(0x21D3)); // import glyph hint
        imp->setToolTip(QObject::tr("Імпортована папка"));
        imp->setStyleSheet(QStringLiteral("color:#1ad16f; font-size:12px; background:transparent;"));
        metaRow->addWidget(imp);
    }
    metaRow->addStretch(1);
    lay->addLayout(metaRow);
    return row;
}
} // namespace

VersionsPage::VersionsPage(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet(kPageStyle);

    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // Left slide-out panel: version list.
    auto *panel = new QWidget();
    panel->setFixedWidth(230);
    panel->setStyleSheet(QStringLiteral("background:#08080f; border-right:1px solid #161626;"));
    auto *panelLay = new QVBoxLayout(panel);
    panelLay->setContentsMargins(16, 18, 16, 18);
    panelLay->setSpacing(10);

    auto *header = new QHBoxLayout();
    auto *title = new QLabel(tr("Версії"));
    title->setObjectName(QStringLiteral("title"));
    header->addWidget(title);
    header->addStretch(1);
    auto *addBtn = new QPushButton(QStringLiteral("+"));
    addBtn->setObjectName(QStringLiteral("iconBtn"));
    addBtn->setToolTip(tr("Створити або імпортувати версію"));
    connect(addBtn, &QPushButton::clicked, this, &VersionsPage::createVersion);
    header->addWidget(addBtn);
    auto *closeBtn = new QPushButton(QStringLiteral("×"));
    closeBtn->setObjectName(QStringLiteral("closeBtn"));
    closeBtn->setToolTip(tr("Видалити вибрану версію зі списку"));
    connect(closeBtn, &QPushButton::clicked, this, &VersionsPage::removeSelected);
    header->addWidget(closeBtn);
    panelLay->addLayout(header);

    auto *hint = new QLabel(tr("Дублюй клац — грати. «+» створює нову версію або "
                               "імпортує папку від іншого лаунчера без змін."));
    hint->setObjectName(QStringLiteral("hint"));
    hint->setWordWrap(true);
    panelLay->addWidget(hint);

    m_list = new QListWidget();
    m_list->setContextMenuPolicy(Qt::CustomContextMenu);
    panelLay->addWidget(m_list, 1);

    m_progress = new QProgressBar();
    m_progress->setVisible(false);
    m_progress->setTextVisible(true);
    panelLay->addWidget(m_progress);

    m_progressLabel = new QLabel();
    m_progressLabel->setObjectName(QStringLiteral("hint"));
    m_progressLabel->setWordWrap(true);
    m_progressLabel->setVisible(false);
    panelLay->addWidget(m_progressLabel);

    // Right side: same dashboard as Home, so the app never feels "empty"
    // while browsing versions — matches the current app's layout.
    m_dashboard = new DashboardContent();

    root->addWidget(panel);
    root->addWidget(m_dashboard, 1);

    auto *vm = &VersionManager::instance();
    connect(vm, &VersionManager::installProgress, this,
            [this](int pct, const QString &msg) {
                m_progress->setVisible(true);
                m_progressLabel->setVisible(true);
                m_progress->setValue(pct);
                m_progressLabel->setText(msg);
            });
    connect(vm, &VersionManager::installFinished, this,
            [this](const QString &, const QString &) {
                m_progress->setValue(100);
                m_progressLabel->setText(tr("Версію встановлено!"));
                reload();
            });
    connect(vm, &VersionManager::installFailed, this,
            [this](const QString &, const QString &err) {
                m_progressLabel->setVisible(true);
                m_progressLabel->setText(tr("Помилка: %1").arg(err));
            });
    connect(&Launcher::instance(), &Launcher::launchFailed, this,
            [this](const QString &, const QString &err) {
                m_progressLabel->setVisible(true);
                m_progressLabel->setText(tr("Помилка запуску: %1").arg(err));
            });
    connect(&Launcher::instance(), &Launcher::gameStarted, this,
            [this](const QString &name) {
                Profiles::instance().touchLastPlayed(name);
                m_progressLabel->setVisible(true);
                m_progressLabel->setText(tr("Гру запущено: %1").arg(name));
                if (m_dashboard)
                    m_dashboard->refreshRecent();
                reload();
            });
    connect(m_list, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        const QString name = item->data(Qt::UserRole).toString();
        for (const auto &p : Profiles::instance().all()) {
            if (p.name == name) {
                Launcher::instance().launch(p);
                return;
            }
        }
    });
    connect(m_list, &QListWidget::customContextMenuRequested, this, [this](const QPoint &pt) {
        auto *item = m_list->itemAt(pt);
        if (!item)
            return;
        QMenu menu(this);
        QAction *play = menu.addAction(tr("▶ Грати"));
        QAction *remove = menu.addAction(tr("Видалити зі списку"));
        QAction *chosen = menu.exec(m_list->mapToGlobal(pt));
        const QString name = item->data(Qt::UserRole).toString();
        if (chosen == play) {
            for (const auto &p : Profiles::instance().all())
                if (p.name == name) { Launcher::instance().launch(p); return; }
        } else if (chosen == remove) {
            Profiles::instance().remove(name);
            reload();
        }
    });

    reload();
}

void VersionsPage::reload()
{
    m_list->clear();
    const auto profiles = Profiles::instance().all();
    for (const auto &p : profiles) {
        auto *item = new QListWidgetItem(m_list);
        item->setData(Qt::UserRole, p.name);
        auto *row = makeVersionRow(p);
        item->setSizeHint(row->sizeHint());
        m_list->setItemWidget(item, row);
    }
    if (m_dashboard)
        m_dashboard->refreshRecent();
}

void VersionsPage::createVersion()
{
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Нова версія"));
    dlg.setMinimumWidth(460);
    dlg.setStyleSheet(kPageStyle);

    auto *lay = new QVBoxLayout(&dlg);
    lay->setSpacing(10);

    auto *newRadio = new QRadioButton(tr("Створити нову версію"));
    newRadio->setChecked(true);
    auto *importRadio = new QRadioButton(tr("Імпортувати папку від іншого лаунчера"));
    lay->addWidget(newRadio);
    lay->addWidget(importRadio);

    auto *nameEdit = new QLineEdit();
    nameEdit->setPlaceholderText(tr("Назва версії"));
    lay->addWidget(nameEdit);

    auto *verCombo = new QComboBox();
    verCombo->addItem(tr("Завантаження списку версій..."));
    lay->addWidget(verCombo);
    auto *verComboGuard = new QPointer<QComboBox>(verCombo);
    VersionManager::instance().fetchManifest(
        this, [verComboGuard](const QList<VersionManager::ManifestEntry> &entries, const QString &) {
            auto *combo = verComboGuard->data();
            if (!combo)
                return;
            combo->clear();
            int releases = 0;
            for (const auto &e : entries) {
                if (e.type != QStringLiteral("release"))
                    continue;
                combo->addItem(e.id);
                ++releases;
                if (releases >= 30)
                    break;
            }
            if (releases == 0)
                combo->addItem(QObject::tr("немає з'єднання"));
        });

    auto *status = new QLabel();
    status->setStyleSheet(QStringLiteral("color:#8a8a9e;"));
    status->setWordWrap(true);
    lay->addWidget(status);

    auto *row = new QHBoxLayout();
    auto *browseBtn = new QPushButton(tr("Вибрати папку версії..."));
    browseBtn->setEnabled(false);
    auto *okBtn = new QPushButton(tr("Зберегти"));
    okBtn->setStyleSheet(QStringLiteral("background:#1ad16f; color:#08080f; font-weight:bold; border:none; border-radius:6px; padding:9px 16px;"));
    row->addWidget(browseBtn);
    row->addStretch(1);
    row->addWidget(okBtn);
    lay->addLayout(row);

    QString importDir;
    QString importSuggestedName;

    connect(importRadio, &QRadioButton::toggled, this, [&](bool on) {
        nameEdit->setEnabled(!on);
        browseBtn->setEnabled(on);
        if (on) {
            status->setText(tr("Папка не копіюється і не перейменовується — "
                               "моди і світи залишаться на місці."));
        } else {
            status->clear();
            importDir.clear();
            importSuggestedName.clear();
        }
    });
    connect(browseBtn, &QPushButton::clicked, this, [&]() {
        const QString dir = QFileDialog::getExistingDirectory(
            this, tr("Папка версії (наприклад .minecraft/versions/...)"),
            Paths::versionsDir());
        if (!dir.isEmpty()) {
            importDir = dir;
            importSuggestedName = QFileInfo(dir).fileName();
            status->setText(tr("Вибрано: %1").arg(dir));
        }
    });
    connect(okBtn, &QPushButton::clicked, this, [&]() {
        if (newRadio->isChecked()) {
            const QString name = nameEdit->text().trimmed();
            if (name.isEmpty()) {
                status->setText(tr("Введи назву."));
                return;
            }
            if (Profiles::instance().exists(name)) {
                status->setText(tr("Версія з такою назвою вже є."));
                return;
            }
            Profile p;
            p.name = name;
            p.dir = Profiles::defaultVersionDir(name);
            p.imported = false;
            QDir().mkpath(p.dir);
            Profiles::instance().add(p);
            const QString mc = verCombo->currentText();
            if (!mc.isEmpty() && mc != tr("немає з'єднання"))
                VersionManager::instance().installVanilla(mc, name);
        } else {
            if (importDir.isEmpty()) {
                status->setText(tr("Спочатку вибери папку версії."));
                return;
            }
            Profile p;
            p.name = nameEdit->text().trimmed();
            if (p.name.isEmpty())
                p.name = importSuggestedName; // keep the folder's own name
            if (p.name.isEmpty() || Profiles::instance().exists(p.name)) {
                status->setText(tr("Введи унікальну назву."));
                return;
            }
            p.dir = importDir;
            p.imported = true;
            Profiles::instance().add(p);
        }
        dlg.accept();
    });

    dlg.exec();
    reload();
}

void VersionsPage::removeSelected()
{
    auto *item = m_list->currentItem();
    if (!item)
        return;
    // Removes only the launcher's list entry; the folder itself stays untouched.
    Profiles::instance().remove(item->data(Qt::UserRole).toString());
    reload();
}

} // namespace sl
