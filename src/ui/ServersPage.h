#pragma once
#include <QWidget>

class QListWidget;

namespace sl {

// "Сервери" page: a saved-servers list (name + address), matching the
// existing app's header layout ("Оновити" / "+ Додати"). Ping/status
// checking is a later milestone — for now this stores and shows the list.
class ServersPage : public QWidget {
    Q_OBJECT
public:
    explicit ServersPage(QWidget *parent = nullptr);

private:
    void reload();
    void addServer();
    QString filePath() const;

    QListWidget *m_list = nullptr;
};

} // namespace sl
