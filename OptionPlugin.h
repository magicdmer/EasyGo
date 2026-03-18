#ifndef OPTIONPLUGIN_H
#define OPTIONPLUGIN_H

#include "Plugin.h"
#include "IndexTask.h"

class OptionPlugin : public Plugin
{
    Q_OBJECT
public:
    OptionPlugin();

public:
    bool initPlugin(QString pluginPath) { Q_UNUSED(pluginPath); return true;}
    bool query(Query query,QVector<Result>& vecResult);
    bool getMenu(Result& result, QVector<Result>& vecMenu);
    void itemClick(Result &item,QObject* parent);

public slots:
    void sltTaskFinished();

public:
    Q_INVOKABLE void setOption();
    Q_INVOKABLE void checkUpdate();
    Q_INVOKABLE void restartApp();
    Q_INVOKABLE void exitApp();
    Q_INVOKABLE void reIndex();

private:
    IndexTask* m_task;
    QObject* m_parent;
};

#endif // OPTIONPLUGIN_H
