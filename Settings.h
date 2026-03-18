#ifndef SETTINGS_H
#define SETTINGS_H

#include <QMap>
#include <QVector>

#define EASYGO_VERSION "1.9.8.4"

struct PluginConfig{
    QString id;
    QString name;
    QStringList keyword;
    QString acceptType;
    int mode;
    int disabled;
};

struct IndexConfig{
    bool clearHistory;
    QStringList indexPath;
    bool enableNormalIndex;
};

class Settings
{
public:
	Settings();
	~Settings();
public:
    bool load();
    bool save();
    int find(const QString& uuid);
    bool isKeywordExsit(const QString& keyword);
    PluginConfig getPluginCfg(const QString& uuid);
public:
	QVector<PluginConfig> m_pluginConfig;
    IndexConfig m_indexCfg;
    QString m_pythonPath;
    int m_maxResultsPerPage;
    int m_maxResultsToShow;
    bool m_startOnSystemStartup;
    bool m_hideWhenDeactive;
    bool m_remLastPosition;
    bool m_disableMouse;
    QString m_hotKey;
    int m_x;
    int m_y;
    QString m_proxy_ip;
    int m_proxy_port;
    int m_debugMode;
    int m_checkUpdate;
    QString m_repo_url;
};

Settings* GetSettings();

#endif
