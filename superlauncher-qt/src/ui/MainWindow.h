#pragma once
#include <QWidget>

class QListWidget;
class QStackedWidget;
class QLabel;

namespace sl {

// Main launcher window: dark theme, left sidebar with page navigation,
// central stacked pages (placeholder pages will be filled step by step).
class MainWindow : public QWidget {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void switchPage(int row);

private:
    void buildUi();
    void showLogin();
    QWidget *makePlaceholderPage(const QString &title);

    QListWidget *m_sidebar = nullptr;
    QStackedWidget *m_pages = nullptr;
    QLabel *m_accountLabel = nullptr;
};

} // namespace sl
