#include "ui/VersionsPage.h"
#include "core/Profiles.h"
#include "core/VersionManager.h"
#include "core/Launcher.h"
#include "core/Paths.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QComboBox>
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
QLabel#title { color:#ededf2; font-size:22px; font-weight:bold; }
QLabel#hint { color:#8a8a9e; font-size:12px; }
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
QPushButton:disabled { color:#5a5a6e; }
QPushButton#add { background:#1ad16f; color:#08080f; }
QPushButton#add:hover { background:#17bc63; }
QLineEdit {
    background:#0d0d18; color:#ededf2; border:1px solid #161626;
    border-radius:6px; padding:8px 10px; selection-background-color:#1ad16f;
}
QRadioButton { color:#ededf2; }
)";
}

VersionsPage::VersionsPage(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet(kPageStyle);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(32, 28, 32, 28);
    root->setSpacing(12);

    auto *title = new QLabel(tr("Версии"));
    title->setObjectName(QStringLiteral("title"));
    root->addWidget(title);

    auto *hint = new QLabel(tr("«+» — создать версию или импортировать папку от другого лаунчера "
                               "(ничего не переименовывается, моды и миры остаются на месте)."));
    hint->setObjectName(QStringLiteral("hint"));
    hint->setWordWrap(true);
    root->addWidget(hint);

    m_list = new QListWidget();
    root->addWidget(m_list, 1);

    m_progress = new QProgressBar();
    m_progress->setVisible(false);
    m_progress->setTextVisible(true);
    root->addWidget(m_progress);

    m_progressLabel = new QLabel();
    m_progressLabel->setObjectName(QStringLiteral("hint"));
    m_progressLabel->setVisible(false);
    root->addWidget(m_progressLabel);

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
                m_progressLabel->setText(tr("Версия установлена!"));
                reload();
            });
    connect(vm, &VersionManager::installFailed, this,
            [this](const QString &, const QString &err) {
                m_progressLabel->setText(tr("Ошибка: %1").arg(err));
            });

    auto *row = new QHBoxLayout();
    m_playBtn = new QPushButton(tr("Играть"));
    m_playBtn->setObjectName(QStringLiteral("add"));
    m_playBtn->setEnabled(false);
    connect(m_playBtn, &QPushButton::clicked, this, [this]() {
        auto *item = m_list->currentItem();
        if (!item)
            return;
        const QString name = item->data(Qt::UserRole).toString();
        for (const auto &p : Profiles::instance().all()) {
            if (p.name == name) {
                Launcher::instance().launch(p);
                return;
            }
        }
    });
    auto *addBtn = new QPushButton(tr("+ Создать версию"));
    addBtn->setObjectName(QStringLiteral("add"));
    connect(addBtn, &QPushButton::clicked, this, &VersionsPage::createVersion);
    m_removeBtn = new QPushButton(tr("Удалить из списка"));
    m_removeBtn->setEnabled(false);
    connect(m_removeBtn, &QPushButton::clicked, this, &VersionsPage::removeSelected);
    connect(m_list, &QListWidget::itemSelectionChanged, this, [this]() {
        const bool has = m_list->currentItem() != nullptr;
        m_removeBtn->setEnabled(has);
        m_playBtn->setEnabled(has);
    });
    connect(&Launcher::instance(), &Launcher::launchFailed, this,
            [this](const QString &, const QString &err) {
                m_progressLabel->setVisible(true);
                m_progressLabel->setText(tr("Ошибка запуска: %1").arg(err));
            });
    connect(&Launcher::instance(), &Launcher::gameStarted, this,
            [this](const QString &name) {
                m_progressLabel->setVisible(true);
                m_progressLabel->setText(tr("Игра запущена: %1").arg(name));
            });
    row->addWidget(m_playBtn);
    row->addWidget(addBtn);
    row->addWidget(m_removeBtn);
    row->addStretch(1);
    root->addLayout(row);

    reload();
}

void VersionsPage::reload()
{
    m_list->clear();
    const auto profiles = Profiles::instance().all();
    for (const auto &p : profiles) {
        auto *item = new QListWidgetItem(
            p.name + (p.imported ? tr("  (импортирована)") : QString()));
        item->setData(Qt::UserRole, p.name);
        m_list->addItem(item);
    }
}

void VersionsPage::createVersion()
{
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Новая версия"));
    dlg.setMinimumWidth(460);
    dlg.setStyleSheet(kPageStyle);

    auto *lay = new QVBoxLayout(&dlg);
    lay->setSpacing(10);

    auto *newRadio = new QRadioButton(tr("Создать новую версию"));
    newRadio->setChecked(true);
    auto *importRadio = new QRadioButton(tr("Импортировать папку от другого лаунчера"));
    lay->addWidget(newRadio);
    lay->addWidget(importRadio);

    auto *nameEdit = new QLineEdit();
    nameEdit->setPlaceholderText(tr("Название версии"));
    lay->addWidget(nameEdit);

    auto *verCombo = new QComboBox();
    verCombo->addItem(tr("Загрузка списка версий..."));
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
                combo->addItem(QObject::tr("нет соединения"));
        });

    auto *status = new QLabel();
    status->setStyleSheet(QStringLiteral("color:#8a8a9e;"));
    status->setWordWrap(true);
    lay->addWidget(status);

    auto *row = new QHBoxLayout();
    auto *browseBtn = new QPushButton(tr("Выбрать папку версии..."));
    browseBtn->setEnabled(false);
    auto *okBtn = new QPushButton(tr("Сохранить"));
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
            status->setText(tr("Папка не копируется и не переименовывается — "
                               "моды и миры останутся на месте."));
        } else {
            status->clear();
            importDir.clear();
            importSuggestedName.clear();
        }
    });
    connect(browseBtn, &QPushButton::clicked, this, [&]() {
        const QString dir = QFileDialog::getExistingDirectory(
            this, tr("Папка версии (например .minecraft/versions/...)"),
            Paths::versionsDir());
        if (!dir.isEmpty()) {
            importDir = dir;
            importSuggestedName = QFileInfo(dir).fileName();
            status->setText(tr("Выбрано: %1").arg(dir));
        }
    });
    connect(okBtn, &QPushButton::clicked, this, [&]() {
        if (newRadio->isChecked()) {
            const QString name = nameEdit->text().trimmed();
            if (name.isEmpty()) {
                status->setText(tr("Введи название."));
                return;
            }
            if (Profiles::instance().exists(name)) {
                status->setText(tr("Версия с таким названием уже есть."));
                return;
            }
            Profile p;
            p.name = name;
            p.dir = Profiles::defaultVersionDir(name);
            p.imported = false;
            QDir().mkpath(p.dir);
            Profiles::instance().add(p);
            // Kick off the vanilla install for the selected Minecraft version.
            const QString mc = verCombo->currentText();
            if (!mc.isEmpty() && mc != tr("нет соединения"))
                VersionManager::instance().installVanilla(mc, name);
        } else {
            if (importDir.isEmpty()) {
                status->setText(tr("Сначала выбери папку версии."));
                return;
            }
            Profile p;
            p.name = nameEdit->text().trimmed();
            if (p.name.isEmpty())
                p.name = importSuggestedName; // keep the folder's own name
            if (p.name.isEmpty() || Profiles::instance().exists(p.name)) {
                status->setText(tr("Введи уникальное название."));
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
