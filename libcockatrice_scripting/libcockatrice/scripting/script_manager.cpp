#include "script_manager.h"

#include <QDirIterator>
#include <QDebug>

#include <sol/sol.hpp>


ScriptManager::ScriptManager(QObject *parent) : QObject(parent)
{
    fileWatcher = new QFileSystemWatcher(this);
    connect(fileWatcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString &path) {
        refreshScripts();
    });
    connect(fileWatcher, &QFileSystemWatcher::fileChanged, this, [this](const QString &path) {
        qDebug() << "ScriptManager file changed:" << path;
        lua.script_file(path.toStdString());
    });
    lua.open_libraries(sol::lib::math);
    lua.open_libraries(sol::lib::base);
    qDebug() << "ScriptManager initialized";
    refreshScripts();
}

double ScriptManager::evaluateLegacyExpression(const QString &expression, double x) {
    sol::environment env(lua, sol::create, lua.globals());
    env["x"] = x;

    env["abs"] = [](double a) { return qFabs(a); };
    env["ceil"] = [](double a) { return qCeil(a); };
    env["cos"] = [](double a) { return qCos(a); };
    env["floor"] = [](double a) { return qFloor(a); };
    env["log"] = [](double a) { return qLn(a); };
    env["log10"] = [](double a) { return qLn(a); };
    env["round"] = [](double a) { return qRound(a); };
    env["sin"] = [](double a) { return qSin(a); };
    env["sqrt"] = [](double a) { return qSqrt(a); };
    env["tan"] = [](double a) { return qTan(a); };
    env["trunc"] = [](double a) { return std::trunc(a); };


    std::string funcDef = "return " + expression.toStdString();
    sol::load_result script = lua.load(funcDef);

    if (!script.valid()) {
        sol::error err = script;
        qWarning("Lua script load error: %s", err.what());
        return 0.0;
    }

    sol::protected_function func = script;
    env.set_on(func);

    sol::protected_function_result result = func();

    if (!result.valid()) {
        sol::error err = result;
        qWarning("Lua script execution error: %s", err.what());
        return 0.0;
    }

    return result.get<double>();
}

void ScriptManager::refreshScripts() {
    // Implementation for refreshing scripts goes here
    qDebug() << "ScriptManager refreshed";

    QStringList allFiles;

    // QDirIterator with QDir::Files ensures only files are listed (no directories)
    QDirIterator it(scriptsPath, {"*.lua"}, QDir::Files,  QDirIterator::NoIteratorFlags);

    while (it.hasNext()) {
        allFiles << it.next(); // Add each file path to the list
        if (!fileWatcher->files().contains(it.filePath())) {
            fileWatcher->addPath(it.filePath());
            lua.script_file(it.filePath().toStdString());
        }
    }

    qDebug() << "ScriptManager Loaded files:" << allFiles;
}