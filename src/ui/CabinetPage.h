#pragma once
#include <QWidget>

class QLabel;

namespace sl {

// "Личный кабинет" page: current account (Microsoft / site / offline),
// synced with superlauncher.org when a site session exists.
class CabinetPage : public QWidget {
    Q_OBJECT
public:
    explicit CabinetPage(QWidget *parent = nullptr);

private slots:
    void logout();
    void refreshFromSite();

private:
    void reload();
    QLabel *m_name = nullptr;
    QLabel *m_uuid = nullptr;
    QLabel *m_accountType = nullptr;
    QLabel *m_siteInfo = nullptr;
};

} // namespace sl
