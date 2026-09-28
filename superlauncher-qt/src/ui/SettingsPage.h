#pragma once
#include <QWidget>

class QSlider;
class QLabel;
class QComboBox;
class QLineEdit;
class QPushButton;

namespace sl {

// "Настройки" page: RAM, game directory, Java runtime.
class SettingsPage : public QWidget {
    Q_OBJECT
public:
    explicit SettingsPage(QWidget *parent = nullptr);

private slots:
    void browseGameDir();
    void browseJava();

private:
    void load();
    void save();

    QSlider *m_minRam = nullptr;
    QSlider *m_maxRam = nullptr;
    QLabel *m_minRamLabel = nullptr;
    QLabel *m_maxRamLabel = nullptr;
    QLineEdit *m_gameDir = nullptr;
    QLineEdit *m_javaPath = nullptr;
    QPushButton *m_saveBtn = nullptr;
};

} // namespace sl
