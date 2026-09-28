#pragma once
#include <QString>
#include <QList>

namespace sl {

// Locates installed Java runtimes.
struct JavaInfo {
    QString path;     // executable
    QString version;  // "17.0.9" or empty
    int major = 0;     // 8, 17, 21...
};

class JavaFinder {
public:
    // All runtimes found on the system.
    static QList<JavaInfo> findAll();

    // Best runtime with major >= required (0 = any).
    // Returns an empty path if nothing suitable was found.
    static JavaInfo findBest(int requiredMajor);

private:
    static JavaInfo probe(const QString &exePath);
};

} // namespace sl
