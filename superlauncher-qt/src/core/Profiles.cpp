#include "core/Profiles.h"
#include "core/Paths.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace sl {

Profiles &Profiles::instance()
{
    static Profiles p;
    return p;
}

QString Profiles::filePath() const
{
    return Paths::configDir() + QStringLiteral("/profiles.json");
}

QList<Profile> Profiles::all() const
{
    QList<Profile> out;
    QFile f(filePath());
    if (!f.open(QIODevice::ReadOnly))
        return out;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    const QJsonArray arr = doc.array();
    for (const auto &v : arr) {
        const QJsonObject o = v.toObject();
        Profile p;
        p.name = o[QStringLiteral("name")].toString();
        p.dir = o[QStringLiteral("dir")].toString();
        p.imported = o[QStringLiteral("imported")].toBool();
        if (!p.name.isEmpty() && !p.dir.isEmpty())
            out.append(p);
    }
    return out;
}

void Profiles::add(const Profile &p)
{
    QList<Profile> list = all();
    for (int i = 0; i < list.size(); ++i) {
        if (list[i].name == p.name) {
            list[i] = p;
            goto save;
        }
    }
    list.append(p);
save:
    QJsonArray arr;
    for (const auto &x : list) {
        arr.append(QJsonObject{
            {QStringLiteral("name"), x.name},
            {QStringLiteral("dir"), x.dir},
            {QStringLiteral("imported"), x.imported},
        });
    }
    QFile f(filePath());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
        f.close();
    }
}

void Profiles::remove(const QString &name)
{
    QList<Profile> list = all();
    for (int i = 0; i < list.size(); ++i) {
        if (list[i].name == name) {
            list.removeAt(i);
            break;
        }
    }
    QJsonArray arr;
    for (const auto &x : list) {
        arr.append(QJsonObject{
            {QStringLiteral("name"), x.name},
            {QStringLiteral("dir"), x.dir},
            {QStringLiteral("imported"), x.imported},
        });
    }
    QFile f(filePath());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
        f.close();
    }
}

bool Profiles::exists(const QString &name) const
{
    for (const auto &p : all())
        if (p.name == name)
            return true;
    return false;
}

QString Profiles::defaultVersionDir(const QString &name)
{
    // Standard convention, no prefix: <gameDir>/versions/<name>.
    return Paths::versionsDir() + QStringLiteral("/") + name;
}

} // namespace sl
