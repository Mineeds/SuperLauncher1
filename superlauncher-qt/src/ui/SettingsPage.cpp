#include "ui/SettingsPage.h"
#include "core/Config.h"
#include "core/JavaFinder.h"
#include "core/Paths.h"

#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

namespace sl {

namespace {
const char *kPageStyle = R"(
QWidget { background:#08080f; }
QLabel { color:#ededf2; }
QLabel#title { color:#ededf2; font-size:22px; font-weight:bold; }
QLabel#value { color:#1ad16f; font-weight:bold; }
QLabel#hint { color:#8a8a9e; font-size:12px; }
QSlider::groove:horizontal { background:#161626; height:6px; border-radius:3px; }
QSlider::handle:horizontal { background:#1ad16f; width:16px; margin:-6px 0; border-radius:8px; }
QSlider::sub-page:horizontal { background:#1ad16f; border-radius:3px; }
QLineEdit {
    background:#0d0d18; color:#ededf2; border:1px solid #161626;
    border-radius:6px; padding:8px 10px; selection-background-color:#1ad16f;
}
QPushButton {
    background:#161626; color:#ededf2; border:none; border-radius:6px;
    padding:9px 16px; font-weight:bold;
}
QPushButton:hover { background:#1d1d30; }
QPushButton#save { background:#1ad16f; color:#08080f; }
QPushButton#save:hover { background:#17bc63; }
)";
}

SettingsPage::SettingsPage(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet(kPageStyle);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(32, 28, 32, 28);
    root->setSpacing(14);

    auto *title = new QLabel(tr("Настройки"));
    title->setObjectName(QStringLiteral("title"));
    root->addWidget(title);

    // --- RAM ---
    root->addWidget(new QLabel(tr("Память для Minecraft")));

    m_minRam = new QSlider(Qt::Horizontal);
    m_minRam->setRange(512, 16384);
    m_minRam->setSingleStep(512);
    m_minRamLabel = new QLabel();
    m_minRamLabel->setObjectName(QStringLiteral("value"));
    auto *minRow = new QHBoxLayout();
    minRow->addWidget(new QLabel(tr("Минимум:")), 0);
    minRow->addWidget(m_minRamLabel, 0);
    minRow->addStretch(1);
    root->addLayout(minRow);
    root->addWidget(m_minRam);

    m_maxRam = new QSlider(Qt::Horizontal);
    m_maxRam->setRange(1024, 16384);
    m_maxRam->setSingleStep(512);
    m_maxRamLabel = new QLabel();
    m_maxRamLabel->setObjectName(QStringLiteral("value"));
    auto *maxRow = new QHBoxLayout();
    maxRow->addWidget(new QLabel(tr("Максимум:")), 0);
    maxRow->addWidget(m_maxRamLabel, 0);
    maxRow->addStretch(1);
    root->addLayout(maxRow);
    root->addWidget(m_maxRam);

    connect(m_minRam, &QSlider::valueChanged, this, [this](int v) {
        m_minRamLabel->setText(tr("%1 МБ").arg(v));
        if (m_maxRam->value() < v)
            m_maxRam->setValue(v);
    });
    connect(m_maxRam, &QSlider::valueChanged, this, [this](int v) {
        m_maxRamLabel->setText(tr("%1 МБ").arg(v));
        if (m_minRam->value() > v)
            m_minRam->setValue(v);
    });

    // --- Game directory ---
    root->addWidget(new QLabel(tr("Папка игры (.minecraft)")));
    auto *dirRow = new QHBoxLayout();
    m_gameDir = new QLineEdit();
    m_gameDir->setReadOnly(true);
    auto *dirBtn = new QPushButton(tr("Выбрать..."));
    connect(dirBtn, &QPushButton::clicked, this, &SettingsPage::browseGameDir);
    dirRow->addWidget(m_gameDir, 1);
    dirRow->addWidget(dirBtn);
    root->addLayout(dirRow);
    auto *dirHint = new QLabel(tr("По умолчанию: ~/.minecraft. Моды и миры остаются в каждой папке."));
    dirHint->setObjectName(QStringLiteral("hint"));
    root->addWidget(dirHint);

    // --- Java ---
    root->addWidget(new QLabel(tr("Java")));
    auto *javaRow = new QHBoxLayout();
    m_javaPath = new QLineEdit();
    m_javaPath->setPlaceholderText(tr("Авто (найденная система)"));
    auto *javaBtn = new QPushButton(tr("Выбрать..."));
    connect(javaBtn, &QPushButton::clicked, this, &SettingsPage::browseJava);
    javaRow->addWidget(m_javaPath, 1);
    javaRow->addWidget(javaBtn);
    root->addLayout(javaRow);

    const auto javas = JavaFinder::findAll();
    if (javas.isEmpty()) {
        auto *jHint = new QLabel(tr("Java не найдена — лаунчер предложит установить при запуске."));
        jHint->setObjectName(QStringLiteral("hint"));
        root->addWidget(jHint);
    } else {
        for (const auto &j : javas) {
            auto *jHint = new QLabel(tr("Найдена: %1 (%2)").arg(j.path, j.version));
            jHint->setObjectName(QStringLiteral("hint"));
            root->addWidget(jHint);
        }
    }

    root->addStretch(1);

    m_saveBtn = new QPushButton(tr("Сохранить"));
    m_saveBtn->setObjectName(QStringLiteral("save"));
    connect(m_saveBtn, &QPushButton::clicked, this, [this]() {
        save();
        m_saveBtn->setText(tr("Сохранено ✓"));
    });
    auto *saveRow = new QHBoxLayout();
    saveRow->addStretch(1);
    saveRow->addWidget(m_saveBtn);
    root->addLayout(saveRow);

    load();
}

void SettingsPage::load()
{
    m_minRam->setValue(Config::instance().minRamMb());
    m_maxRam->setValue(Config::instance().maxRamMb());
    m_minRamLabel->setText(tr("%1 МБ").arg(m_minRam->value()));
    m_maxRamLabel->setText(tr("%1 МБ").arg(m_maxRam->value()));
    m_gameDir->setText(Paths::gameDir());
    m_javaPath->setText(Config::instance().javaPath());
}

void SettingsPage::save()
{
    Config::instance().setMinRamMb(m_minRam->value());
    Config::instance().setMaxRamMb(m_maxRam->value());
    Config::instance().setJavaPath(m_javaPath->text().trimmed());
}

void SettingsPage::browseGameDir()
{
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Папка игры"),
                                                          Paths::gameDir());
    if (!dir.isEmpty())
        Config::instance().setCustomGameDir(dir);
    m_gameDir->setText(Paths::gameDir());
}

void SettingsPage::browseJava()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Исполняемый файл Java"),
                                                      QStringLiteral("/usr/lib/jvm"));
    if (!path.isEmpty())
        m_javaPath->setText(path);
}

} // namespace sl
