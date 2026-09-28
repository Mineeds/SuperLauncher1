#pragma once
#include <QWidget>
#include <QMap>

class QLineEdit;
class QComboBox;
class QListWidget;
class QPushButton;
class QLabel;
class QTimer;

namespace sl {

// "Моды" page: instant search + install from Modrinth (no API key needed).
// Types: mods -> .minecraft/mods, shaders -> shaderpacks, resource packs -> resourcepacks.
// Modpacks open in the browser for now.
class ModsPage : public QWidget {
    Q_OBJECT
public:
    explicit ModsPage(QWidget *parent = nullptr);

private slots:
    void search();
    void onItemActivated();

private:
    QString projectType() const;
    QString targetFolder() const;
    QString selectedProfileMcVersion() const;

    QLineEdit *m_search = nullptr;
    QComboBox *m_type = nullptr;
    QListWidget *m_results = nullptr;
    QLabel *m_status = nullptr;
    QTimer *m_debounce = nullptr;
    QMap<QString, class ModCard *> m_cards;
};

} // namespace sl
