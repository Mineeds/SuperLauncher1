#pragma once
#include <QWidget>
#include <QSet>
#include <QString>

class QLineEdit;
class QComboBox;
class QVBoxLayout;
class QLabel;
class QTimer;
class QPushButton;

namespace sl {

// "Ресурси" page: tabs (Модпаки/Моди/Ресурс-паки/Дата-паки/Шейдери), instant
// search, and a right-hand filters panel (Minecraft version, loader,
// category, sort). Backed by the Modrinth API — same install pipeline as
// before (download to the right .minecraft subfolder, or open modpacks on
// the site until full modpack import lands).
class ResourcesPage : public QWidget {
    Q_OBJECT
public:
    explicit ResourcesPage(QWidget *parent = nullptr);

private:
    void selectTab(int index);
    void search();
    void resetFilters();
    void install(const QString &projectId, const QString &slug, const QString &title);
    QString projectType() const;
    QString targetFolder() const;
    QString selectedProfileMcVersion() const;
    QString buildFacets() const;

    int m_tabIndex = 0;
    QPushButton *m_tabButtons[5] = {nullptr};

    QLineEdit *m_search = nullptr;
    QComboBox *m_sort = nullptr;
    QComboBox *m_category = nullptr;
    QComboBox *m_perPage = nullptr;
    QLabel *m_status = nullptr;
    QLabel *m_pageLabel = nullptr;
    QVBoxLayout *m_resultsLayout = nullptr;
    QTimer *m_debounce = nullptr;

    QSet<QString> m_mcVersionFilters;
    QSet<QString> m_loaderFilters;
    int m_offset = 0;
};

} // namespace sl
