#include "PluginManager.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include "Settings.h"
#include "ProgramPlugin.h"
#include <QMapIterator>
#include "LogFile.h"
#include "WebSearchPlugin.h"
#include "EpmPlugin.h"
#include "OptionPlugin.h"
#include "ThemePlugin.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
# pragma execution_character_set("utf-8")
#endif

PluginManager::PluginManager(void)
{
    m_pluginsPath = QDir::currentPath() + QString("/Plugins");
    QDir dir;
    dir.mkdir("Plugins");
}

PluginManager::~PluginManager(void)
{
	qDeleteAll(m_plugins);
    m_plugins.clear();
}

PluginType PluginManager::GetPluginType(QString pluginPath)
{
    QString filePath = pluginPath + QString("/plugin.json");
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return PLUGIN_UNKOWN;
    }
    QString value = file.readAll();
    file.close();

    QJsonParseError parseJsonErr;
    QJsonDocument document = QJsonDocument::fromJson(value.toUtf8(), &parseJsonErr);
    if (parseJsonErr.error != QJsonParseError::NoError)
    {
        return PLUGIN_UNKOWN;
    }

    PluginType pType = PLUGIN_UNKOWN;
    QJsonObject jsonObject = document.object();
    QString pluginType = jsonObject["PluginType"].toString();
    if (pluginType.compare("c++",Qt::CaseInsensitive) == 0)
    {
        pType = PLUGIN_CPP;
    }
    else if (pluginType.compare("python",Qt::CaseInsensitive) == 0)
    {
        pType = PLUGIN_PYTHON;
    }
    else if (pluginType.compare("e", Qt::CaseInsensitive) == 0)
    {
        pType = PLUGIN_E;
    }
    else if (pluginType.compare("powershell", Qt::CaseInsensitive) == 0)
    {
        pType = PLUGIN_POWERSHELL;
    }
    else if (pluginType.compare("script",Qt::CaseInsensitive) == 0)
    {
        pType = PLUGIN_SCRIPT;
    }
    else
    {
        pType = PLUGIN_UNKOWN;
    }

    return pType;
}

bool PluginManager::loadPlugin()
{
    if (!m_plugins.isEmpty())
    {
        qDeleteAll(m_plugins);
        m_plugins.clear();
        GetSettings()->load();
    }

    loadDefaultPlugin();

    Settings* setting = GetSettings();

    QDir dir(m_pluginsPath);
	QStringList folders = dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
	for (int i = 0; i < folders.size(); i++)
	{
        QString pluginPath = m_pluginsPath + QString("/%1").arg(folders[i]);
        PluginType pType = GetPluginType(pluginPath);
        if (pType == PLUGIN_UNKOWN)
        {
            continue;
        }

        Plugin* plugin = nullptr;
        if (pType == PLUGIN_CPP)
        {
            plugin = new CPlusPlugin(pluginPath);
        }
        else if (pType == PLUGIN_PYTHON)
        {
            plugin = new PythonPlugin(pluginPath);
        }
        else if (pType == PLUGIN_E)
        {
            plugin = new EPlugin(pluginPath);
        }
        else if (pType == PLUGIN_POWERSHELL)
        {
            plugin = new PowerShellPlugin(pluginPath);
        }
        else if (pType == PLUGIN_SCRIPT)
        {
            plugin = new ScriptPlugin(pluginPath);
        }

        //这里给plugin类加了一个m_initok字段，因为SetDlg里面需要管理插件，如果Init失败就不加入的话，就没法获取相关插件信息了
        if (setting->find(plugin->m_guid) == -1)
        {
            PluginConfig config;
            config.id = plugin->m_guid;
            config.name = plugin->m_info.name;
            config.keyword = plugin->m_info.keyword;
            config.mode = plugin->m_mode;
            config.disabled = 0;
            config.acceptType = plugin->m_info.acceptType;
            setting->m_pluginConfig.append(config);
        }

        if ((plugin->m_type == PLUGIN_CPP || plugin->m_type == PLUGIN_E)
            && !plugin->initPlugin(pluginPath))
        {
            plugin->m_initok = false;
            qLog(QString("初始化插件[%1]失败...").arg(plugin->m_info.name));
        }

        m_plugins[plugin->m_guid] = plugin;
	}

    for (int i = 0; i < setting->m_pluginConfig.size(); i++)
    {
        QString id = setting->m_pluginConfig[i].id;
        Plugin* plugin = m_plugins[id];
        if (!plugin)
        {
            setting->m_pluginConfig.remove(i);
        }
    }

    setting->save();

	return true;
}

void PluginManager::loadDefaultPlugin()
{
    Plugin* plugin = new ProgramPlugin();
    plugin->initPlugin("");
    m_plugins[PROGRAM_PLUGIN_ID] = plugin;
    
    Plugin* webPlugin = new WebSearchPlugin();
    webPlugin->initPlugin("");
    m_plugins[WEBSEARCH_PLUGIN_ID] = webPlugin;

    Plugin* epmPlugin = new EpmPlugin();
    epmPlugin->initPlugin("");
    m_plugins[EPM_PLUGIN_ID] = epmPlugin;

    Plugin* optionPlugin = new OptionPlugin();
    optionPlugin->initPlugin("");
    m_plugins[OPTION_PLUGIN_ID] = optionPlugin;

    Plugin* themePlugin = new ThemePlugin();
    themePlugin->initPlugin("");
    m_plugins[THEME_PLUGIN_ID] = themePlugin;
}

bool PluginManager::addPlugin(QString &pluginPath)
{
    Settings* setting = GetSettings();

    PluginType pType = GetPluginType(pluginPath);
    if (pType == PLUGIN_UNKOWN)
    {
        return false;
    }

    Plugin* plugin = nullptr;
    if (pType == PLUGIN_CPP)
    {
        plugin = new CPlusPlugin(pluginPath);
    }
    else if (pType == PLUGIN_PYTHON)
    {
        plugin = new PythonPlugin(pluginPath);
    }
    else if (pType == PLUGIN_E)
    {
        plugin = new EPlugin(pluginPath);
    }
    else if (pType == PLUGIN_POWERSHELL)
    {
        plugin = new PowerShellPlugin(pluginPath);
    }
    else if (pType == PLUGIN_SCRIPT)
    {
        plugin = new ScriptPlugin(pluginPath);
    }

    if (setting->find(plugin->m_guid) == -1)
    {
        PluginConfig config;
        config.id = plugin->m_guid;
        config.name = plugin->m_info.name;

        if (setting->isKeywordExsit(plugin->m_info.keyword[0]) ||
            GetWebSearchCfg()->m_itemsMap.contains(plugin->m_info.keyword[0]))
        {
            config.keyword.append("");
        }
        else
        {
            config.keyword = plugin->m_info.keyword;
        }

        config.disabled = 0;
        config.mode = plugin->m_mode;
        config.acceptType = plugin->m_info.acceptType;
        setting->m_pluginConfig.append(config);
        setting->save();
    }

    if (!plugin->initPlugin(pluginPath))
    {
        plugin->m_initok = false;
    }

    m_plugins[plugin->m_guid] = plugin;

    return true;
}

bool PluginManager::deletePlugin(QString &pluginId)
{
    if (m_plugins[pluginId]->m_type == PLUGIN_CPP)
    {
        CPlusPlugin* plugin = (CPlusPlugin*)m_plugins[pluginId];
        plugin->dettach();
    }
    else if (m_plugins[pluginId]->m_type == PLUGIN_E)
    {
        EPlugin* plugin = (EPlugin*)m_plugins[pluginId];
        plugin->dettach();
    }

    QDir dir(m_plugins[pluginId]->m_path);
    if (!dir.removeRecursively())
    {
        return false;
    }

    m_plugins.remove(pluginId);
    int index = GetSettings()->find(pluginId);
    if (index == -1)
    {
        return false;
    }

    GetSettings()->m_pluginConfig.remove(index);
    GetSettings()->save();

    return true;
}

Plugin* PluginManager::getPlugin(QString pluginId)
{
	return m_plugins[pluginId];
}

bool PluginManager::getValidPlugin(const QString& keyword,QVector<Plugin*>& pluginVec)
{
	Settings* cfg = GetSettings();

	bool find = false;

    QMapIterator<QString,Plugin*> mapIter(m_plugins);
    while(mapIter.hasNext())
    {
        mapIter.next();


        int index = cfg->find(mapIter.key());
        if (index != -1)
        {
            if (cfg->m_pluginConfig[index].disabled)
            {
                continue;
            }
        }

        Plugin* plugin = mapIter.value();
        if (!plugin || !plugin->m_initok)
        {
            continue;
        }

        QStringList pluginKeyword;
        if (index != -1)
        {
            pluginKeyword = cfg->m_pluginConfig[index].keyword;
        }
        else
        {
            pluginKeyword = plugin->m_info.keyword;
        }

        for (int i = 0; i < pluginKeyword.size(); i++)
        {
            if (pluginKeyword[i].compare(keyword,Qt::CaseInsensitive) == 0)
            {
                if (plugin->m_guid == WEBSEARCH_PLUGIN_ID)
                {
                    if (GetWebSearchCfg()->m_itemsMap[keyword].enabled)
                    {
                        find = true;
                        pluginVec.append(plugin);
                    }
                    break;
                }
                else
                {
                    find = true;
                    pluginVec.append(plugin);
                    break;
                }
            }
        }
    }

    if (!find)
    {
        QString pgId = PROGRAM_PLUGIN_ID;
        Plugin* plugin = m_plugins[pgId];
        if (plugin)
        {
            pluginVec.append(plugin);
            find = true;
        }
    }

	return find;
}

bool PluginManager::getValidTypePlugin(const QString& fileType,QVector<Plugin*>& vecPlugin)
{
    Settings* cfg = GetSettings();

    bool find = false;

    QMapIterator<QString,Plugin*> mapIter(m_plugins);
    while(mapIter.hasNext())
    {
        mapIter.next();


        int index = cfg->find(mapIter.key());
        if (index != -1)
        {
            if (cfg->m_pluginConfig[index].disabled)
            {
                continue;
            }
        }

        Plugin* plugin = mapIter.value();
        if (!plugin || !plugin->m_initok)
        {
            continue;
        }

        QString acceptType;
        if (index != -1)
        {
            acceptType = cfg->m_pluginConfig[index].acceptType;
        }
        else
        {
            acceptType = plugin->m_info.acceptType;
        }

        if (acceptType.isEmpty())
        {
            continue;
        }

        QStringList acceptList = acceptType.split(",");

        for (int i = 0; i < acceptList.size(); i++)
        {
            if (acceptList[i].compare(fileType,Qt::CaseInsensitive) == 0)
            {
                find = true;
                vecPlugin.append(plugin);
                break;
            }
        }
    }

    return find;
}

Plugin* PluginManager::getValidPlugin(const QString& keyword)
{
    Settings* cfg = GetSettings();

    QMapIterator<QString,Plugin*> mapIter(m_plugins);
    while(mapIter.hasNext())
    {
        mapIter.next();

        int index = cfg->find(mapIter.key());
        if (index != -1)
        {
            if (cfg->m_pluginConfig[index].disabled)
            {
                continue;
            }
        }

        Plugin* plugin = mapIter.value();
        if (!plugin || !plugin->m_initok)
        {
            continue;
        }

        QStringList pluginKeyword;
        if (index != -1)
        {
            pluginKeyword = cfg->m_pluginConfig[index].keyword;
        }
        else
        {
            pluginKeyword = plugin->m_info.keyword;
        }

        for (int i = 0; i < pluginKeyword.size(); i++)
        {
            if (!pluginKeyword[i].isEmpty() &&
                pluginKeyword[i].compare(keyword,Qt::CaseInsensitive) == 0)
            {
                if (plugin->m_guid == WEBSEARCH_PLUGIN_ID)
                {
                    if (GetWebSearchCfg()->m_itemsMap[pluginKeyword[i]].enabled)
                    {
                        return plugin;
                    }
                    break;
                }
                else
                {
                    return plugin;
                }
            }
        }

    }

    QString pgId = PROGRAM_PLUGIN_ID;
    Plugin* plugin = m_plugins[pgId];
    return plugin;
}


bool PluginManager::search(const QString &keyword, QVector<Plugin*>& vecPlugin)
{
    Settings* cfg = GetSettings();

    bool find = false;

    QMapIterator<QString,Plugin*> mapIter(m_plugins);
    while(mapIter.hasNext())
    {
        mapIter.next();

        int index = cfg->find(mapIter.key());
        if (index != -1)
        {
            if (cfg->m_pluginConfig[index].disabled)
            {
                continue;
            }
        }

        Plugin* plugin = mapIter.value();
        if (!plugin || !plugin->m_initok)
        {
            continue;
        }

        QStringList pluginKeyword;
        if (index != -1)
        {
            pluginKeyword = cfg->m_pluginConfig[index].keyword;
        }
        else
        {
            pluginKeyword = plugin->m_info.keyword;
        }

        for (int i = 0; i < pluginKeyword.size(); i++)
        {
            if (pluginKeyword[i] != "*" &&
                pluginKeyword[i].startsWith(keyword,Qt::CaseInsensitive))
            {
                if (plugin->m_guid == WEBSEARCH_PLUGIN_ID)
                {
                    if (GetWebSearchCfg()->m_itemsMap[pluginKeyword[i]].enabled)
                    {
                        find = true;
                        vecPlugin.append(plugin);
                    }
                    break;
                }
                else
                {
                    find = true;
                    vecPlugin.append(plugin);
                    break;
                }
            }
        }
    }

    return find;
}

PluginManager* GetPluginMananger()
{
	static PluginManager manager;
	static bool s_initialed =  false;
	if (!s_initialed)
	{
		s_initialed = true;
		manager.loadPlugin();
	}

	return &manager;
}
