#include "ui/CabinetPage.h"
#include "auth/TokenStore.h"
#include "net/HttpClient.h"
#include "ui/LoginDialog.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace sl {

namespace {
const char *kPageStyle = R"(
QWidget { background:#08080f; }
QLabel { color:#ededf2; }
QLabel#title { color:#ededf2; font-size:22px; font-weight:bold; }
QLabel#nick { color:#1ad16f; font-size:26px; font-weight:bold; }
QLabel#hint { color:#8a8a9e; font-size:12px; }
QPushButton {
    background:#161626; color:#ededf2; border:none; border-radius:6px;
    padding:9px 16px; font-weight:bold;
}
QPushButton:hover { background:#1d1d30; }
)";
}

CabinetPage::CabinetPage(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet(kPageStyle);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(32, 28, 32, 28);
    root->setSpacing(10);

    auto *title = new QLabel(tr("Личный кабинет"));
    title->setObjectName(QStringLiteral("title"));
    root->addWidget(title);

    m_name = new QLabel();
    m_name->setObjectName(QStringLiteral("nick"));
    root->addWidget(m_name);

    m_accountType = new QLabel();
    root->addWidget(m_accountType);

    m_uuid = new QLabel();
    m_uuid->setObjectName(QStringLiteral("hint"));
    root->addWidget(m_uuid);

    m_siteInfo = new QLabel();
    m_siteInfo->setObjectName(QStringLiteral("hint"));
    m_siteInfo->setWordWrap(true);
    root->addWidget(m_siteInfo);

    root->addStretch(1);

    auto *row = new QHBoxLayout();
    auto *refreshBtn = new QPushButton(tr("Обновить"));
    connect(refreshBtn, &QPushButton::clicked, this, &CabinetPage::refreshFromSite);
    auto *logoutBtn = new QPushButton(tr("Выйти из аккаунта"));
    connect(logoutBtn, &QPushButton::clicked, this, &CabinetPage::logout);
    row->addWidget(refreshBtn);
    row->addWidget(logoutBtn);
    row->addStretch(1);
    root->addLayout(row);

    reload();
}

void CabinetPage::reload()
{
    if (!TokenStore::msPlayerName().isEmpty()) {
        m_name->setText(TokenStore::msPlayerName());
        m_accountType->setText(tr("Аккаунт Microsoft"));
        m_uuid->setText(tr("UUID: %1").arg(TokenStore::msUuid()));
        m_siteInfo->setText(tr("Сайт: не подключён (можно войти через сайт для синхронизации)."));
    } else if (!TokenStore::siteNickname().isEmpty()) {
        m_name->setText(TokenStore::siteNickname());
        m_accountType->setText(tr("Аккаунт superlauncher.org"));
        m_uuid->setText(tr("Синхронизирован с личным кабинетом на сайте."));
    } else {
        m_name->setText(TokenStore::offlineNick().isEmpty() ? tr("Player")
                                                            : TokenStore::offlineNick());
        m_accountType->setText(tr("Офлайн-режим"));
        m_uuid->setText(tr("Без аккаунта: скин по нику, серверы без авторизации."));
    }
}

void CabinetPage::refreshFromSite()
{
    if (TokenStore::siteRefreshToken().isEmpty()) {
        m_siteInfo->setText(tr("Нет сессии сайта — войди через кнопку на странице входа."));
        return;
    }
    m_siteInfo->setText(tr("Синхронизация с superlauncher.org..."));
    // TODO: switch to the real cabinet endpoint once the backend adds it:
    // GET /api/auth/me with the stored JWT -> nickname, coins, purchases.
    HttpClient::instance().getJson(
        QStringLiteral("https://superlauncher.org/api/auth/me"),
        [this](const QJsonDocument &doc, const QString &error) {
            if (!error.isEmpty()) {
                m_siteInfo->setText(tr("Сайт недоступен: %1").arg(error));
                return;
            }
            const QJsonObject o = doc.object();
            m_siteInfo->setText(tr("Сайт: %1").arg(QString::fromUtf8(
                QJsonDocument(o).toJson(QJsonDocument::Compact))));
        });
}

void CabinetPage::logout()
{
    TokenStore::clearMsAccount();
    TokenStore::clearSite();
    TokenStore::clearOffline();
    reload();
    // Ask for a new login right away, like the first run.
    LoginDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted && !dlg.displayName().isEmpty())
        m_name->setText(dlg.displayName());
    reload();
}

} // namespace sl
