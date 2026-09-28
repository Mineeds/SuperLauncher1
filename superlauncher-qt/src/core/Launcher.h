#pragma once
#include <QObject>
#include "core/Profiles.h"

namespace sl {

// Builds the full Minecraft command line and starts the game.
// Handles: version JSON parsing (new + legacy arguments), rules evaluation,
// classpath, natives extraction, account substitution (Microsoft / site /
// offline), RAM settings from Config.
class Launcher : public QObject {
    Q_OBJECT
public:
    static Launcher &instance();

    // Launch the given profile with the current stored account.
    void launch(const Profile &profile);

signals:
    void launchFailed(const QString &profileName, const QString &error);
    void gameStarted(const QString &profileName);

private:
    explicit Launcher(QObject *parent = nullptr);

    bool resolveVersionJson(const Profile &profile, QString *jsonPath,
                            QJsonDocument *doc, QString *error);
    QStringList buildJvmArgs(const QJsonObject &versionRoot, const QString &profileDir,
                             int majorJava) const;
    QStringList buildGameArgs(const QJsonObject &versionRoot) const;
    bool rulesAllow(const QJsonArray &rules) const;
    QString substitute(QString arg, const QMap<QString, QString> &vars) const;
    QString nativesClassifier(const QJsonObject &lib) const;
    bool ensureNatives(const QJsonObject &versionRoot, const Profile &profile,
                       QString *error);
};

} // namespace sl
