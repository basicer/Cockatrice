#ifndef SCRIPT_MANAGER_H
#define SCRIPT_MANAGER_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QFilesystemWatcher>

#include <QDebug>

#include <sol/sol.hpp>

class ScriptManager : public QObject
{
    Q_OBJECT
public:
    ScriptManager() = delete;
    explicit ScriptManager(QObject *parent = nullptr);
    
    void setScriptsPath(const QString &path)
    {
        scriptsPath = path;
        fileWatcher->removePaths(fileWatcher->files());
        fileWatcher->addPath(path);
        qDebug() << "ScriptManager path set to:" << path;
    }
    double evaluateLegacyExpression(const QString &expression, double x);
private:
    sol::state lua;
    QString scriptsPath;
    void refreshScripts();

    QFileSystemWatcher *fileWatcher;
};

#endif // SCRIPT_MANAGER_H