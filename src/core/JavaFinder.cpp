#include "core/JavaFinder.h"
#include "core/Paths.h"

#include <QDir>
#include <QProcess>
#include <QFile>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QSysInfo>

namespace sl {

JavaInfo JavaFinder::probe(const QString &exePath)
{
    JavaInfo info;
    info.path = exePath;
    QProcess p;
    p.start(exePath, {QStringLiteral("-version")});
    if (!p.waitForFinished(5000)) {
        info.path.clear();
        return info;
    }
    const QString out = QString::fromUtf8(p.readAllStandardError())
                           .remove(QLatin1Char('\r'))
                           .replace(QLatin1Char('\n'), QLatin1Char(' '));
    // e.g. 'openjdk version "17.0.9" 2023-10-17'
    static const QRegularExpression re(QStringLiteral("version \"(\\d+)(?:\\.(\\d+))?"));
    const auto m = re.match(out);
    if (m.hasMatch()) {
        info.version = m.captured(1) + (m.hasCaptured(2)
                                            ? QStringLiteral(".") + m.captured(2)
                                            : QString());
        const int first = m.captured(1).toInt();
        info.major = first == 1 ? m.captured(2).toInt() : first; // "1.8" => 8
    } else {
        info.path.clear();
    }
    return info;
}

QList<JavaInfo> JavaFinder::findAll()
{
    QList<JavaInfo> out;
    const bool isWindows = QSysInfo::productType() == QStringLiteral("windows");
    const QString exe = isWindows ? QStringLiteral("javaw.exe") : QStringLiteral("java");
    QStringList candidates;

    // JAVA_HOME
    const QString javaHome = qEnvironmentVariable("JAVA_HOME");
    if (!javaHome.isEmpty())
        candidates << javaHome + QStringLiteral("/bin/") + exe;

    // Shared runtimes inside the game dir (TLauncher-style "javas").
    const QDir javasDir(Paths::gameDir() + QStringLiteral("/javas"));
    for (const auto &entry : javasDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        candidates << javasDir.filePath(entry) + QStringLiteral("/bin/") + exe
                   << javasDir.filePath(entry) + QStringLiteral("/bin/java");
    }

    // PATH
    const QString inPath = QStandardPaths::findExecutable(isWindows ? QStringLiteral("javaw")
                                                                    : QStringLiteral("java"));
    if (!inPath.isEmpty())
        candidates << inPath;

    if (isWindows) {
        for (const auto &root : {QStringLiteral("C:/Program Files/Java"),
                                 QStringLiteral("C:/Program Files (x86)/Java"),
                                 QStringLiteral("C:/Program Files/Eclipse Adoptium"),
                                 QStringLiteral("C:/Program Files/Microsoft")}) {
            QDir d(root);
            for (const auto &entry : d.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                candidates << d.filePath(entry) + QStringLiteral("/bin/") + exe;
            }
        }
    } else if (QSysInfo::productType() == QStringLiteral("osx")) {
        QDir d(QStringLiteral("/Library/Java/JavaVirtualMachines"));
        for (const auto &entry : d.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
            candidates << d.filePath(entry) + QStringLiteral("/Contents/Home/bin/java");
    } else {
        QDir d(QStringLiteral("/usr/lib/jvm"));
        for (const auto &entry : d.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            candidates << d.filePath(entry) + QStringLiteral("/bin/java");
        }
    }

    for (const auto &c : candidates) {
        if (QFile::exists(c)) {
            JavaInfo info = probe(c);
            if (!info.path.isEmpty())
                out.append(info);
        }
    }
    return out;
}

JavaInfo JavaFinder::findBest(int requiredMajor)
{
    const QList<JavaInfo> all = findAll();
    JavaInfo best;
    for (const auto &j : all) {
        if (requiredMajor > 0 && j.major < requiredMajor)
            continue;
        if (best.path.isEmpty() || j.major < best.major) // smallest suitable version
            best = j;
    }
    return best;
}

} // namespace sl
