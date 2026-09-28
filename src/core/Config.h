#pragma once
#include <QObject>
#include <QString>

namespace sl {

// Application-wide settings, persisted as an INI file.
class Config : public QObject {
    Q_OBJECT
public:
    static Config &instance();

    // Custom game data directory. Empty string => default ~/.minecraft.
    QString customGameDir() const;
    void setCustomGameDir(const QString &dir);

    // RAM settings in MB.
    int minRamMb() const;
    int maxRamMb() const;
    void setMinRamMb(int mb);
    void setMaxRamMb(int mb);

    // Manual Java executable. Empty => auto-detect.
    QString javaPath() const;
    void setJavaPath(const QString &path);

    // Window geometry.
    QByteArray windowGeometry() const;
    void setWindowGeometry(const QByteArray &geometry);

private:
    explicit Config(QObject *parent = nullptr);
    QString m_filePath;
};

} // namespace sl
