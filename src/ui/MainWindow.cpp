#include "ui/MainWindow.h"
#include "ui/LoginDialog.h"
#include "ui/VersionsPage.h"
#include "ui/ResourcesPage.h"
#include "ui/ServersPage.h"
#include "ui/SettingsPage.h"
#include "ui/CabinetPage.h"
#include "ui/DashboardContent.h"
#include "auth/TokenStore.h"
#include "core/Paths.h"
#include "core/Config.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QListWidget>
#include <QStackedWidget>
#include <QFrame>
#include <QPushButton>
#include <QVBoxLayout>

namespace sl {

namespace {

void applyDarkPalette(QWidget *w)
{
    // SuperLauncher style: dark background #08080f with emerald accents,
    // matching the site's Minecraft-styled look.
    QPalette pal = w->palette();
    pal.setColor(QPalette::Window, QColor(0x08, 0x08, 0x0f));
    pal.setColor(QPalette::Base, QColor(0x0d, 0x0d, 0x18));
    pal.setColor(QPalette::Text, QColor(0xed, 0xed, 0xf2));
    pal.setColor(QPalette::WindowText, QColor(0xed, 0xed, 0xf2));
    pal.setColor(QPalette::Button, QColor(0x16, 0x16, 0x26));
    pal.setColor(QPalette::Highlight, QColor(0x1a, 0xd1, 0x6f));
    pal.setColor(QPalette::HighlightedText, QColor(0x08, 0x08, 0x0f));
    w->setPalette(pal);
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    applyDarkPalette(this);
    setAutoFillBackground(true);
    buildUi();
    const QByteArray geo = Config::instance().windowGeometry();
    if (!geo.isEmpty())
        restoreGeometry(geo);

    // Restore or request a session.
    QString player;
    if (!TokenStore::msPlayerName().isEmpty())
        player = TokenStore::msPlayerName();
    else if (!TokenStore::siteNickname().isEmpty())
        player = TokenStore::siteNickname();
    else if (!TokenStore::offlineNick().isEmpty())
        player = TokenStore::offlineNick();

    if (!player.isEmpty()) {
        m_accountLabel->setText(player);
        m_statusLabel->setText(tr("Online"));
    } else {
        showLogin();
    }
}

void MainWindow::buildUi()
{
    setWindowTitle(QStringLiteral("SuperLauncher"));
    resize(1180, 720);

    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // Sidebar
    auto *side = new QVBoxLayout();
    side->setContentsMargins(0, 0, 0, 0);
    side->setSpacing(0);

    auto *brandRow = new QHBoxLayout();
    brandRow->setContentsMargins(14, 14, 14, 14);
    auto *logo = new QLabel(QStringLiteral("A"));
    logo->setFixedSize(26, 26);
    logo->setAlignment(Qt::AlignCenter);
    logo->setStyleSheet(QStringLiteral(
        "background:#1ad16f; color:#08080f; font-weight:bold; border-radius:6px;"));
    brandRow->addWidget(logo);
    auto *brand = new QLabel(QStringLiteral("SuperLauncher"));
    brand->setStyleSheet(QStringLiteral("color:#ededf2; font-size:14px; font-weight:bold;"));
    brandRow->addWidget(brand);
    auto *ver = new QLabel(QStringLiteral("v1.0"));
    ver->setStyleSheet(QStringLiteral("color:#8a8a9e; font-size:11px;"));
    brandRow->addWidget(ver);
    brandRow->addStretch(1);
    auto *brandFrame = new QFrame();
    brandFrame->setLayout(brandRow);

    m_sidebar = new QListWidget();
    m_sidebar->setFixedWidth(200);
    m_sidebar->setStyleSheet(QStringLiteral(
        "QListWidget{background:#0d0d18; color:#ededf2; border:none; outline:0;}"
        "QListWidget::item{padding:12px 14px; font-size:13px;}"
        "QListWidget::item:selected{background:#161626; color:#1ad16f; border-left:3px solid #1ad16f;}"));

    const QStringList pages{
        tr("Головна"),
        tr("Версії"),
        tr("Ресурси"),
        tr("Сервери"),
        tr("Себе"),
    };
    for (const QString &p : pages)
        m_sidebar->addItem(p);

    // Bottom mini account card + settings link.
    auto *accountFrame = new QFrame();
    accountFrame->setStyleSheet(QStringLiteral("QFrame{border-top:1px solid #161626;}"));
    auto *accountLay = new QVBoxLayout(accountFrame);
    accountLay->setContentsMargins(14, 10, 14, 10);
    accountLay->setSpacing(6);

    auto *accountRow = new QHBoxLayout();
    auto *avatar = new QLabel();
    avatar->setFixedSize(28, 28);
    avatar->setStyleSheet(QStringLiteral("background:#6a4a2a; border-radius:6px;"));
    accountRow->addWidget(avatar);
    auto *accountCol = new QVBoxLayout();
    accountCol->setSpacing(0);
    m_accountLabel = new QLabel(tr("Не авторизовано"));
    m_accountLabel->setStyleSheet(QStringLiteral("color:#ededf2; font-size:12px; font-weight:bold;"));
    m_statusLabel = new QLabel(tr("Offline"));
    m_statusLabel->setStyleSheet(QStringLiteral("color:#1ad16f; font-size:10px;"));
    accountCol->addWidget(m_accountLabel);
    accountCol->addWidget(m_statusLabel);
    accountRow->addLayout(accountCol, 1);
    accountLay->addLayout(accountRow);

    auto *settingsBtn = new QPushButton(tr("⚙ Налаштування"));
    settingsBtn->setCursor(Qt::PointingHandCursor);
    settingsBtn->setStyleSheet(QStringLiteral(
        "QPushButton{background:transparent; color:#8a8a9e; border:none; text-align:left; "
        "font-size:12px; padding:4px 0px;} QPushButton:hover{color:#ededf2;}"));
    connect(settingsBtn, &QPushButton::clicked, this, &MainWindow::openSettings);
    accountLay->addWidget(settingsBtn);

    side->addWidget(brandFrame);
    side->addWidget(m_sidebar, 1);
    side->addWidget(accountFrame);

    auto *sideFrame = new QFrame();
    sideFrame->setLayout(side);
    sideFrame->setStyleSheet(QStringLiteral("QFrame{background:#0d0d18;}"));

    // Pages
    m_pages = new QStackedWidget();
    m_pages->addWidget(new DashboardContent());   // Головна
    m_pages->addWidget(new VersionsPage());       // Версії
    m_pages->addWidget(new ResourcesPage());      // Ресурси
    m_pages->addWidget(new ServersPage());        // Сервери
    m_pages->addWidget(new CabinetPage());        // Себе

    root->addWidget(sideFrame);
    root->addWidget(m_pages, 1);

    connect(m_sidebar, &QListWidget::currentRowChanged, this, &MainWindow::switchPage);
    m_sidebar->setCurrentRow(0);
}

void MainWindow::showLogin()
{
    LoginDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted && !dlg.displayName().isEmpty()) {
        m_accountLabel->setText(dlg.displayName());
        m_statusLabel->setText(tr("Online"));
    }
}

void MainWindow::openSettings()
{
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Налаштування"));
    dlg.resize(560, 480);
    auto *lay = new QVBoxLayout(&dlg);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(new SettingsPage());
    dlg.exec();
}

void MainWindow::switchPage(int row)
{
    m_pages->setCurrentIndex(row);
}

} // namespace sl
