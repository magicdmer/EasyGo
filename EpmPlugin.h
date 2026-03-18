#ifndef EPMPLUGIN_H
#define EPMPLUGIN_H

#include "Plugin.h"
#include <QVector>

struct EpmPluginInfo{
    QString id;
    QString name;
    QString description;
    QString author;
    QString version;
    QString min_require;
    int timestamp;
};

class EpmPlugin : public Plugin
{
    Q_OBJECT
public:
    explicit EpmPlugin();
public:
    bool initPlugin(QString pluginPath) { Q_UNUSED(pluginPath); return true; }
    bool query(Query query,QVector<Result>& vecResult);
    bool getMenu(Result& result, QVector<Result>& vecMenu);
    void itemClick(Result &item,QObject* parent);
    bool parsePluginList(QString& strJson);
    EpmPluginInfo* getPluginInfo(QString name);

public:
    Q_INVOKABLE void installPlugin(QString url, QObject* parent);
    Q_INVOKABLE void setProxy(QString proxy, QObject* parent);

private:
    QVector<EpmPluginInfo> m_vecPluginInfo;
};

#endif // EPMPLUGIN_H
