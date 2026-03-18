#ifndef THEMEPLUGIN_H
#define THEMEPLUGIN_H

#include "Plugin.h"

class ThemePlugin : public Plugin
{
    Q_OBJECT
public:
    ThemePlugin();
public:
    bool initPlugin(QString pluginPath) { Q_UNUSED(pluginPath); return true; }
    bool query(Query query,QVector<Result>& vecResult);
    bool getMenu(Result& result, QVector<Result>& vecMenu);
    void itemClick(Result &item,QObject* parent);
public:
    Q_INVOKABLE void setTheme(QString name, QObject* parent);
    Q_INVOKABLE void setRoundCorner(QString roundCorner, QObject* parent);
};

#endif // THEMEPLUGIN_H
