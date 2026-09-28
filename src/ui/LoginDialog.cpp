#include "ui/LoginDialog.h"
#include "auth/MsAuth.h"
#include "auth/SiteAuth.h"
#include "auth/TokenStore.h"

#include <QDesktopServices>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

namespace sl {

namespace {
const char *kStyle = R"(
QDialog { background:#08080f; }
QLabel { color:#ededf2; }
QLabel#status { color:#1ad16f; }
QLabel#brand { color:#1ad16f; font-size:20px; font-weight:bold; letter-spacing:2px; }
QLabel#hint { color:#8a8a9e; font-size:12px; }
QLineEdit {
    background:#0d0d18; color:#ededf2; border:1px solid #161626;
    border-radius:6px; padding:8px 10px; selection-background-color:#1ad16f;
}
QPushButton {
    background:#161626; color:#ededf2; border:none; border-radius:6px;
    padding:10px 16px; font-weight:bold;
}
QPushButton:hover { background:#1d1d30; }
QPushButton:disabled { color:#5a5a6e; }
QPushButton#primary { background:#1ad16f; color:#08080f; }
QPushButton#primary:hover { background:#17bc63; }
)";
}

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
{
    m_msAuth = new MsAuth(this);
    m_siteAuth = new SiteAuth(this);
    buildUi();

    connect(m_msAuth, &MsAuth::progress, this, &LoginDialog::onMsProgress);
    connect(m_msAuth, &MsAuth::userCodeReady, this, &LoginDialog::onMsUserCode);
    connect(m_msAuth, &MsAuth::finished, this, &LoginDialog::onMsFinished);
    connect(m_msAuth, &MsAuth::failed, this, &LoginDialog::onMsFailed);

    connect(m_siteAuth, &SiteAuth::progress, this, &LoginDialog::onSiteProgress);
    connect(m_siteAuth, &SiteAuth::linkReady, this, &LoginDialog::onSiteLinkReady);
    connect(m_siteAuth, &SiteAuth::finished, this, &LoginDialog::onSiteFinished);
    connect(m_siteAuth, &SiteAuth::failed, this, &LoginDialog::onSiteFailed);
}

void LoginDialog::buildUi()
{
    setWindowTitle(tr("Вход — SuperLauncher"));
    setModal(true);
    setMinimumWidth(420);
    setStyleSheet(kStyle);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(32, 28, 32, 28);
    root->setSpacing(14);

    auto *brand = new QLabel(QStringLiteral("SUPERLAUNCHER"));
    brand->setObjectName(QStringLiteral("brand"));
    root->addWidget(brand);

    m_msButton = new QPushButton(tr("Войти через Microsoft"));
    m_msButton->setObjectName(QStringLiteral("primary"));
    connect(m_msButton, &QPushButton::clicked, this, &LoginDialog::startMicrosoftLogin);
    root->addWidget(m_msButton);

    m_siteButton = new QPushButton(tr("Войти через сайт (личный кабинет)"));
    connect(m_siteButton, &QPushButton::clicked, this, &LoginDialog::startSiteLogin);
    root->addWidget(m_siteButton);

    root->addWidget(new QLabel(tr("— или офлайн-режим —")));

    auto *nickRow = new QHBoxLayout();
    m_nick = new QLineEdit();
    m_nick->setPlaceholderText(tr("Ник"));
    m_offlineButton = new QPushButton(tr("Играть"));
    m_offlineButton->setDefault(true);
    connect(m_offlineButton, &QPushButton::clicked, this, &LoginDialog::doOfflineLogin);
    connect(m_nick, &QLineEdit::returnPressed, this, &LoginDialog::doOfflineLogin);
    nickRow->addWidget(m_nick, 1);
    nickRow->addWidget(m_offlineButton);
    root->addLayout(nickRow);

    auto *hint = new QLabel(tr("Офлайн: игра без аккаунта. Microsoft: доступ к серверам "
                               "с авторизацией. Сайт: вход в браузере и синхронизация "
                               "с твоим личным кабинетом на superlauncher.org."));
    hint->setObjectName(QStringLiteral("hint"));
    hint->setWordWrap(true);
    root->addWidget(hint);

    m_status = new QLabel();
    m_status->setObjectName(QStringLiteral("status"));
    m_status->setWordWrap(true);
    root->addWidget(m_status);

    root->addStretch(1);
}

void LoginDialog::setBusy(bool busy)
{
    m_msButton->setEnabled(!busy);
    m_siteButton->setEnabled(!busy);
    m_offlineButton->setEnabled(!busy);
    m_nick->setEnabled(!busy);
}

void LoginDialog::startMicrosoftLogin()
{
    setBusy(true);
    m_status->setText(tr("Ожидание кода Microsoft..."));
    m_msAuth->start();
}

void LoginDialog::onMsProgress(const QString &msg)
{
    m_status->setText(msg);
}

void LoginDialog::onMsUserCode(const QString &userCode, const QString &uri)
{
    m_status->setText(tr("Открой %1 и введи код: %2")
                          .arg(QStringLiteral("<a href='%1'>%1</a>").arg(uri), userCode));
    // Auto-open the verification page in the browser.
    QDesktopServices::openUrl(QUrl(uri));
}

void LoginDialog::onMsFinished(const QString &mcToken, const QString &refreshToken,
                               const QString &uuid, const QString &name)
{
    TokenStore::saveMsAccount(refreshToken, uuid, name);
    TokenStore::saveMsAccessToken(mcToken); // short-lived; used until it expires
    TokenStore::clearOffline();
    TokenStore::clearSite();
    m_displayName = name;
    accept();
}

void LoginDialog::onMsFailed(const QString &error)
{
    setBusy(false);
    m_status->setText(error);
}

void LoginDialog::startSiteLogin()
{
    setBusy(true);
    m_status->setText(tr("Подключение к superlauncher.org..."));
    m_siteAuth->start();
}

void LoginDialog::onSiteProgress(const QString &msg)
{
    m_status->setText(msg);
}

void LoginDialog::onSiteLinkReady(const QString &url)
{
    m_status->setText(tr("Открываю сайт для входа... код привязки активен 15 минут."));
    QDesktopServices::openUrl(QUrl(url));
}

void LoginDialog::onSiteFinished(const QString &access, const QString &refresh, const QString &nickname)
{
    Q_UNUSED(access);
    Q_UNUSED(refresh);
    m_displayName = nickname;
    accept();
}

void LoginDialog::onSiteFailed(const QString &error)
{
    setBusy(false);
    m_status->setText(error);
}

void LoginDialog::doOfflineLogin()
{
    const QString nick = m_nick->text().trimmed();
    if (nick.isEmpty()) {
        m_status->setText(tr("Введи ник."));
        return;
    }
    TokenStore::saveOfflineNick(nick);
    TokenStore::clearMsAccount();
    TokenStore::clearSite();
    m_displayName = nick;
    accept();
}

} // namespace sl
