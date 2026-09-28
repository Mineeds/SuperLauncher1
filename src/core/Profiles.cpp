#include "core/Profiles.h"
#include "core/Paths.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace sl {

namespace {
QJsonArray serialize(const QList<Profile> &list)
{
    QJsonArray arr;
    for (const auto &x : list) {
        arr.append(QJsonObject{
            {QStringLiteral("name"), x.name},
            {QStringLiteral("dir"), x.dir},
            {QStringLiteral("imported"), x.imported},
            {QStringLiteral("lastPlayed"), x.lastPlayed},
        });
    }
    return arr;
}
} // namespace

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
        p.lastPlayed = o[QStringLiteral("lastPlayed")].toString();
        if (!p.name.isEmpty() && !p.dir.isEmpty())
            out.append(p);
    }
    return out;
}

void Profiles::add(const Profile &p)
{
    QList<Profile> list = all();
    bool replaced = false;
    for (int i = 0; i < list.size(); ++i) {
        if (list[i].name == p.name) {
            list[i] = p;
            replaced = true;
            break;
        }
    }
    if (!replaced)
        list.append(p);
    QFile f(filePath());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(QJsonDocument(serialize(list)).toJson(QJsonDocument::Compact));
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
    QFile f(filePath());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(QJsonDocument(serialize(list)).toJson(QJsonDocument::Compact));
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

void Profiles::touchLastPlayed(const QString &name)
{
    QList<Profile> list = all();
    for (auto &p : list) {
        if (p.name == name) {
            p.lastPlayed = QDateTime::currentDateTime().toString(Qt::ISODate);
            break;
        }
    }
    QFile f(filePath());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(QJsonDocument(serialize(list)).toJson(QJsonDocument::Compact));
        f.close();
    }
}

QString Profiles::defaultVersionDir(const QString &name)
{
    // Standard convention, no prefix: <gameDir>/versions/<name>.
    return Paths::versionsDir() + QStringLiteral("/") + name;
}

} // namespace sl
