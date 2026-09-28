#pragma once
#include <QWidget>

class QListWidget;
class QStackedWidget;
class QLabel;

namespace sl {

// Main launcher window: dark theme, left sidebar (Головна/Версії/Ресурси/
// Сервери/Себе) + a mini account card and settings link at the bottom,
// central stacked pages.
class MainWindow : public QWidget {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void switchPage(int row);

private:
    void buildUi();
    void showLogin();
    void openSettings();

    QListWidget *m_sidebar = nullptr;
    QStackedWidget *m_pages = nullptr;
    QLabel *m_accountLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
};

} // namespace sl
