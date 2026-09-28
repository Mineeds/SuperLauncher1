#include "ui/MainWindow.h"
#include "ui/LoginDialog.h"
#include "ui/VersionsPage.h"
#include "ui/SettingsPage.h"
#include "ui/CabinetPage.h"
#include "ui/ModsPage.h"
#include "auth/TokenStore.h"
#include "core/Paths.h"
#include "core/Config.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QListWidget>
#include <QStackedWidget>
#include <QFrame>

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
    const QString player = TokenStore::msPlayerName();
    if (!player.isEmpty())
        m_accountLabel->setText(player);
    else if (!TokenStore::siteNickname().isEmpty())
        m_accountLabel->setText(TokenStore::siteNickname());
    else if (!TokenStore::offlineNick().isEmpty())
        m_accountLabel->setText(TokenStore::offlineNick());
    else
        showLogin();
}

void MainWindow::buildUi()
{
    setWindowTitle(QStringLiteral("SuperLauncher"));
    resize(1100, 700);

    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // Sidebar
    auto *side = new QVBoxLayout();
    auto *brand = new QLabel(QStringLiteral("SUPERLAUNCHER"));
    brand->setStyleSheet(QStringLiteral(
        "color:#1ad16f; font-size:15px; font-weight:bold; padding:14px 12px; letter-spacing:2px;"));

    m_sidebar = new QListWidget();
    m_sidebar->setFixedWidth(200);
    m_sidebar->setStyleSheet(QStringLiteral(
        "QListWidget{background:#0d0d18; color:#ededf2; border:none; outline:0;}"
        "QListWidget::item{padding:12px 14px;}"
        "QListWidget::item:selected{background:#161626; color:#1ad16f; border-left:3px solid #1ad16f;}"));

    const QStringList pages{
        tr("Личный кабинет"),
        tr("Версии"),
        tr("Моды"),
        tr("Настройки"),
    };
    for (const QString &p : pages)
        m_sidebar->addItem(p);

    m_accountLabel = new QLabel(tr("Не авторизован"));
    m_accountLabel->setStyleSheet(
        QStringLiteral("color:#8a8a9e; padding:10px 12px; border-top:1px solid #161626;"));

    side->setContentsMargins(0, 0, 0, 0);
    side->setSpacing(0);
    side->addWidget(brand);
    side->addWidget(m_sidebar, 1);
    side->addWidget(m_accountLabel);

    auto *sideFrame = new QFrame();
    sideFrame->setLayout(side);
    sideFrame->setStyleSheet(QStringLiteral("QFrame{background:#0d0d18;}"));

    // Pages
    m_pages = new QStackedWidget();
    m_pages->addWidget(new CabinetPage());
    m_pages->addWidget(new VersionsPage());
    m_pages->addWidget(new ModsPage());
    m_pages->addWidget(new SettingsPage());

    root->addWidget(sideFrame);
    root->addWidget(m_pages, 1);

    connect(m_sidebar, &QListWidget::currentRowChanged, this, &MainWindow::switchPage);
    m_sidebar->setCurrentRow(0);
}

QWidget *MainWindow::makePlaceholderPage(const QString &title)
{
    auto *page = new QWidget();
    auto *lay = new QVBoxLayout(page);
    lay->setContentsMargins(40, 40, 40, 40);
    auto *label = new QLabel(title);
    label->setStyleSheet(QStringLiteral("color:#ededf2; font-size:22px; font-weight:bold;"));
    lay->addWidget(label);
    lay->addStretch(1);
    return page;
}

void MainWindow::showLogin()
{
    LoginDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted && !dlg.displayName().isEmpty())
        m_accountLabel->setText(dlg.displayName());
}

void MainWindow::switchPage(int row)
{
    m_pages->setCurrentIndex(row);
}

} // namespace sl
