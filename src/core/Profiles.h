#pragma once
#include <QString>
#include <QList>

namespace sl {

// A launcher profile: points at a version folder inside .minecraft (or a custom
// location). Version folders keep their original names — the launcher stores
// only paths in its own config, so other launchers (TLauncher, SKLauncher,
// vanilla) keep working and never conflict with us.
struct Profile {
    QString name;      // display name (any, user's choice)
    QString dir;      // absolute path to the version folder
    bool imported = false; // true if the folder was created by another launcher
    QString lastPlayed;    // ISO date-time, empty = never launched
};

class Profiles {
public:
    static Profiles &instance();

    QList<Profile> all() const;
    // Add or update a profile by name.
    void add(const Profile &p);
    void remove(const QString &name);
    bool exists(const QString &name) const;
    // Stamps "now" as lastPlayed for the given profile and persists it.
    void touchLastPlayed(const QString &name);

    // Default location for a NEW version folder: <gameDir>/versions/<name>.
    // Standard name, no prefix — same convention as every other launcher,
    // so imported and native versions live side by side.
    static QString defaultVersionDir(const QString &name);

private:
    Profiles() = default;
    QString filePath() const;
};

} // namespace sl
