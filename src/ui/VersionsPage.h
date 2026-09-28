#pragma once
#include <QWidget>

class QListWidget;
class QPushButton;
class QProgressBar;
class QLabel;

namespace sl {

class DashboardContent;

// "Версії" page: a slide-out list of profiles (left) next to the same
// dashboard content shown on Home (right) — matches the existing app's
// layout. "+" creates/imports a version, "X" removes the selected one.
// Create dialog offers two options:
//  - New version: pick a name; folder <gameDir>/versions/<name> (standard, no prefix)
//  - Import: pick an existing version folder (e.g. from TLauncher/SKLauncher);
//    the folder is NOT renamed or moved — mods/worlds stay in place.
class VersionsPage : public QWidget {
    Q_OBJECT
public:
    explicit VersionsPage(QWidget *parent = nullptr);

private slots:
    void createVersion();
    void removeSelected();

private:
    void reload();
    QListWidget *m_list = nullptr;
    QPushButton *m_removeBtn = nullptr;
    QProgressBar *m_progress = nullptr;
    QLabel *m_progressLabel = nullptr;
    DashboardContent *m_dashboard = nullptr;
};

} // namespace sl
