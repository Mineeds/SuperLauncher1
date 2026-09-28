#pragma once
#include <QWidget>

class QLabel;
class QVBoxLayout;
class QHBoxLayout;

namespace sl {

// The main dashboard content: hero banner, "recent launches" row, a small
// stats panel, "popular modpacks" (live from Modrinth) and a news block.
// Shared by the Home page and embedded to the right of the Versions list.
class DashboardContent : public QWidget {
    Q_OBJECT
public:
    explicit DashboardContent(QWidget *parent = nullptr);

    // Re-reads the profiles list (call after creating/removing a version).
    void refreshRecent();

private:
    QWidget *buildHero();
    QWidget *buildStatsPanel();
    QWidget *buildRecentRow();
    QWidget *buildModpacksRow();
    QWidget *buildNewsRow();
    void loadPopularModpacks();

    QHBoxLayout *m_recentRow = nullptr;
    QHBoxLayout *m_modpacksRow = nullptr;
};

} // namespace sl
