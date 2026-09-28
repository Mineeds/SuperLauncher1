#pragma once
#include <QWidget>

class QListWidget;
class QPushButton;
class QProgressBar;
class QLabel;

namespace sl {

// "Версии" page: list of profiles + "+" button.
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
    QPushButton *m_playBtn = nullptr;
    QProgressBar *m_progress = nullptr;
    QLabel *m_progressLabel = nullptr;
};

} // namespace sl
