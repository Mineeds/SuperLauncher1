#include "ui/ServersPage.h"
#include "core/Paths.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>

namespace sl {

namespace {
const char *kStyle = R"(
QWidget { background:#08080f; }
QLabel { color:#ededf2; background:transparent; }
QLabel#pageTitle { font-size:20px; font-weight:bold; }
QLabel#hint { color:#8a8a9e; font-size:12px; }
QPushButton {
    background:#161626; color:#ededf2; border:none; border-radius:6px;
    padding:8px 16px; font-weight:bold; font-size:12px;
}
QPushButton:hover { background:#1d1d30; }
QPushButton#add { background:#1ad16f; color:#08080f; }
QPushButton#add:hover { background:#17bc63; }
QListWidget {
    background:#0d0d18; color:#ededf2; border:1px solid #161626;
    border-radius:8px; outline:0;
}
QListWidget::item { padding:12px; border-bottom:1px solid #12121e; }
QListWidget::item:selected { background:#161626; }
QLineEdit {
    background:#0d0d18; color:#ededf2; border:1px solid #161626;
    border-radius:6px; padding:8px 10px; selection-background-color:#1ad16f;
}
)";
}

ServersPage::ServersPage(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet(kStyle);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(28, 24, 28, 24);
    root->setSpacing(14);

    auto *header = new QHBoxLayout();
    auto *title = new QLabel(tr("Сервери"));
    title->setObjectName(QStringLiteral("pageTitle"));
    header->addWidget(title);
    header->addStretch(1);
    auto *refreshBtn = new QPushButton(tr("↻ Оновити"));
    connect(refreshBtn, &QPushButton::clicked, this, &ServersPage::reload);
    header->addWidget(refreshBtn);
    auto *addBtn = new QPushButton(tr("+ Додати"));
    addBtn->setObjectName(QStringLiteral("add"));
    connect(addBtn, &QPushButton::clicked, this, &ServersPage::addServer);
    header->addWidget(addBtn);
    root->addLayout(header);

    m_list = new QListWidget();
    m_list->setContextMenuPolicy(Qt::CustomContextMenu);
    root->addWidget(m_list, 1);

    connect(m_list, &QListWidget::customContextMenuRequested, this, [this](const QPoint &pt) {
        auto *item = m_list->itemAt(pt);
        if (!item)
            return;
        QMenu menu(this);
        QAction *remove = menu.addAction(tr("Видалити сервер"));
        if (menu.exec(m_list->mapToGlobal(pt)) == remove) {
            QFile f(filePath());
            QJsonArray arr;
            if (f.open(QIODevice::ReadOnly)) {
                arr = QJsonDocument::fromJson(f.readAll()).array();
                f.close();
            }
            QJsonArray next;
            for (const auto &v : arr)
                if (v.toObject()[QStringLiteral("name")].toString() != item->data(Qt::UserRole).toString())
                    next.append(v);
            if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                f.write(QJsonDocument(next).toJson(QJsonDocument::Compact));
                f.close();
            }
            reload();
        }
    });

    reload();
}

QString ServersPage::filePath() const
{
    return Paths::configDir() + QStringLiteral("/servers.json");
}

void ServersPage::reload()
{
    m_list->clear();
    QFile f(filePath());
    if (!f.open(QIODevice::ReadOnly))
        return;
    const QJsonArray arr = QJsonDocument::fromJson(f.readAll()).array();
    f.close();
    for (const auto &v : arr) {
        const QJsonObject o = v.toObject();
        const QString name = o[QStringLiteral("name")].toString();
        const QString address = o[QStringLiteral("address")].toString();
        auto *item = new QListWidgetItem(QStringLiteral("%1\n%2").arg(name, address));
        item->setData(Qt::UserRole, name);
        m_list->addItem(item);
    }
}

void ServersPage::addServer()
{
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Додати сервер"));
    dlg.setStyleSheet(kStyle);
    dlg.setMinimumWidth(360);

    auto *form = new QFormLayout(&dlg);
    auto *nameEdit = new QLineEdit();
    auto *addressEdit = new QLineEdit();
    addressEdit->setPlaceholderText(tr("IP:порт, наприклад play.example.com:25565"));
    form->addRow(tr("Назва:"), nameEdit);
    form->addRow(tr("Адреса:"), addressEdit);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    form->addRow(buttons);

    if (dlg.exec() != QDialog::Accepted)
        return;
    const QString name = nameEdit->text().trimmed();
    const QString address = addressEdit->text().trimmed();
    if (name.isEmpty() || address.isEmpty())
        return;

    QFile f(filePath());
    QJsonArray arr;
    if (f.open(QIODevice::ReadOnly)) {
        arr = QJsonDocument::fromJson(f.readAll()).array();
        f.close();
    }
    arr.append(QJsonObject{{QStringLiteral("name"), name}, {QStringLiteral("address"), address}});
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
        f.close();
    }
    reload();
}

} // namespace sl
