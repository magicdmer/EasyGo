#ifndef PROGRAMPLUGIN_H
#define PROGRAMPLUGIN_H

#include "Plugin.h"

struct ProgramQuery{

};

class ProgramPlugin : public Plugin
{
    Q_OBJECT
public:
    ProgramPlugin();
public:
    bool initPlugin(QString pluginPath) { Q_UNUSED(pluginPath); return true;}
    bool query(Query query,QVector<Result>& vecResult);
    bool getMenu(Result& result, QVector<Result>& vecMenu);
    void itemClick(Result &item,QObject* parent);
};

#endif // PROGRAMPLUGIN_H
