#include "ui/MainWindow.h"
#include "core/Config.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("SuperLauncher"));
    QApplication::setOrganizationName(QStringLiteral("Minebag"));

    sl::MainWindow w;
    w.setWindowIcon(QIcon(QStringLiteral(":/assets/assets/icon.png")));
    w.setWindowTitle(QStringLiteral("SuperLauncher " SUPERLAUNCHER_VERSION));
    w.show();

    const int rc = app.exec();

    // Persist window geometry on exit.
    sl::Config::instance().setWindowGeometry(w.saveGeometry());
    return rc;
}
