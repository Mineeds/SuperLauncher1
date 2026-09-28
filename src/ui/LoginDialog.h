#pragma once
#include <QDialog>

class QLineEdit;
class QLabel;
class QPushButton;

namespace sl {

class MsAuth;
class SiteAuth;

// Login screen with three flows:
//  1. Microsoft account (device code flow) -> Minecraft account
//  2. Site account: opens superlauncher.org in the browser, syncs with the
//     user's cabinet (personal account on the site)
//  3. Offline mode (nickname only, no account)
class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(QWidget *parent = nullptr);

    // Player name to show after a successful login (any flow).
    QString displayName() const { return m_displayName; }

private slots:
    void startMicrosoftLogin();
    void startSiteLogin();
    void doOfflineLogin();
    void onMsProgress(const QString &msg);
    void onMsUserCode(const QString &userCode, const QString &uri);
    void onMsFinished(const QString &mcToken, const QString &refreshToken,
                      const QString &uuid, const QString &name);
    void onMsFailed(const QString &error);
    void onSiteProgress(const QString &msg);
    void onSiteLinkReady(const QString &url);
    void onSiteFinished(const QString &access, const QString &refresh, const QString &nickname);
    void onSiteFailed(const QString &error);

private:
    void buildUi();
    void setBusy(bool busy);

    MsAuth *m_msAuth = nullptr;
    SiteAuth *m_siteAuth = nullptr;
    QLineEdit *m_nick = nullptr;
    QLabel *m_status = nullptr;
    QPushButton *m_msButton = nullptr;
    QPushButton *m_siteButton = nullptr;
    QPushButton *m_offlineButton = nullptr;
    QString m_displayName;
};

} // namespace sl
