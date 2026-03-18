#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include "Plugin.h"
#include <QMap>

class PluginManager
{
public:
	PluginManager(void);
	~PluginManager(void);

public:
	bool loadPlugin();
    bool addPlugin(QString& pluginPath);
	bool deletePlugin(QString& pluginId);
	bool disablePlugin(QString& pluginId);
    Plugin* getPlugin(QString pluginId);
	bool getValidPlugin(const QString& keyword,QVector<Plugin*>& vecPlugin);
    Plugin* getValidPlugin(const QString& keyword);
    bool getValidTypePlugin(const QString& fileType,QVector<Plugin*>& vecPlugin);
    QString getPluginsPath(){ return m_pluginsPath;}
    bool search(const QString& query,QVector<Plugin*>& vecPlugin);

private:
    PluginType GetPluginType(QString pluginPath);
    void loadDefaultPlugin();
private:
	QMap<QString,Plugin*> m_plugins;
    QString m_pluginsPath;
};

PluginManager* GetPluginMananger();

#endif
