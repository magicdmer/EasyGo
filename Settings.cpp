#include "Settings.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include "LogFile.h"
#include <QDir>

Settings::Settings(void):
    m_startOnSystemStartup(false),
    m_hideWhenDeactive(false),
    m_remLastPosition(false),
    m_disableMouse(false),
    m_maxResultsPerPage(6),
    m_maxResultsToShow(50),
    m_debugMode(0),
    m_checkUpdate(0)
{
    m_indexCfg.enableNormalIndex = false;
    m_indexCfg.indexPath.clear();
    m_indexCfg.clearHistory = false;

    m_hotKey = "Alt+Space";

    QDir dir;
    dir.mkdir("Settings");
}


Settings::~Settings(void)
{
}

bool Settings::load()
{
	QFile file("Settings\\Settings.json");
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
        qLog("Open Settings.json failed");
		return false;
	}

	QString value = file.readAll();
	file.close();

	QJsonParseError parseJsonErr;
	QJsonDocument document = QJsonDocument::fromJson(value.toUtf8(), &parseJsonErr);
	if (parseJsonErr.error != QJsonParseError::NoError) 
	{
        qLog(QString("Parse Settings.json failed, error : %1").arg(parseJsonErr.errorString()));
		return false;
	}

    m_pluginConfig.clear();
    m_indexCfg.indexPath.clear();

	QJsonObject jsonObject = document.object();

    m_startOnSystemStartup = jsonObject["StartOnSystemStartup"].toBool();
    m_hideWhenDeactive = jsonObject["HideWhenDeactive"].toBool();
    m_remLastPosition = jsonObject["RememberLastLaunchLocation"].toBool();
    if(m_remLastPosition)
    {
        m_x = jsonObject["WindowLeft"].toInt();
        m_y = jsonObject["WindowTop"].toInt();
    }

    m_maxResultsPerPage = jsonObject["MaxResultsPerPage"].toInt();
    m_maxResultsToShow = jsonObject["MaxResultsToShow"].toInt();

    m_hotKey = jsonObject["HotKey"].toString();

    m_disableMouse = jsonObject["DisableMouse"].toBool();

    m_checkUpdate = jsonObject["CheckUpdate"].toInt();

    m_repo_url = jsonObject["Repo"].toString();

    QJsonObject oPluginSettings = jsonObject["PluginSettings"].toObject();
    m_pythonPath = oPluginSettings["PythonDirectory"].toString();
    QJsonArray pluginArray = oPluginSettings["Plugins"].toArray();
	for (int i = 0; i < pluginArray.size(); i++)
	{
		QJsonObject pluginObj = pluginArray[i].toObject();

		PluginConfig cfg;
        cfg.id = pluginObj["ID"].toString();
        cfg.name = pluginObj["Name"].toString();

        if (pluginObj["Keyword"].isArray())
        {
            QJsonArray keysObj = pluginObj["Keyword"].toArray();
            for (int i = 0; i < keysObj.size(); i++)
            {
                QString key = keysObj[i].toString();
                cfg.keyword.append(key);
            }
        }
        else
        {
            QString key = pluginObj["Keyword"].toString();
            cfg.keyword.append(key);
        }

        cfg.mode = pluginObj["PluginMode"].toInt();

        cfg.disabled = pluginObj["Disabled"].toInt();

        if (pluginObj.contains("AcceptType"))
        {
            cfg.acceptType = pluginObj["AcceptType"].toString();
        }
		
		m_pluginConfig.append(cfg);
	}

    QJsonObject oIndexSetting = jsonObject["IndexSetting"].toObject();
    if (oIndexSetting.contains("EnableNormalIndex"))
    {
        m_indexCfg.enableNormalIndex = oIndexSetting["EnableNormalIndex"].toBool();
    }

    if (oIndexSetting.contains("ClearHistory"))
    {
        m_indexCfg.clearHistory = oIndexSetting["ClearHistory"].toBool();
    }

    QJsonArray oPathArray = oIndexSetting["ProgramSources"].toArray();
    for (int i = 0; i < oPathArray.size();i++)
    {
        QJsonObject oPath = oPathArray[i].toObject();
        QString path = oPath["Location"].toString();
        m_indexCfg.indexPath.append(path);
    }

    QJsonObject oEpmSetting = jsonObject["EpmSetting"].toObject();
    m_proxy_ip = oEpmSetting["proxy_ip"].toString();
    m_proxy_port = oEpmSetting["proxy_port"].toInt();
	return true;
}

bool Settings::save()
{
	QJsonDocument document;
    QJsonObject oRoot;
    QJsonObject oPluginSettings;
    QJsonObject oIndexSetting;
    QJsonObject oEpmSetting;
	QJsonArray array;
    QJsonArray pathArray;

    oRoot["StartOnSystemStartup"] = m_startOnSystemStartup;
    oRoot["HideWhenDeactive"] = m_hideWhenDeactive;
    oRoot["RememberLastLaunchLocation"] = m_remLastPosition;
    if (m_remLastPosition)
    {
        oRoot["WindowLeft"] = m_x;
        oRoot["WindowTop"] = m_y;
    }
    oRoot["MaxResultsToShow"] = m_maxResultsToShow;
    oRoot["MaxResultsPerPage"] = m_maxResultsPerPage;

    oRoot["HotKey"] = m_hotKey;

    oRoot["DisableMouse"] = m_disableMouse;

    oRoot["CheckUpdate"] = m_checkUpdate;

    oRoot["Repo"] = m_repo_url;

    oPluginSettings["PythonDirectory"] = m_pythonPath;

	for (int i = 0; i< m_pluginConfig.size(); i++)
	{
		QJsonObject pluginObj;
        pluginObj.insert("ID",m_pluginConfig[i].id);
        pluginObj.insert("Name",m_pluginConfig[i].name);

        if (m_pluginConfig[i].keyword.size() == 1)
        {
            pluginObj.insert("Keyword",m_pluginConfig[i].keyword[0]);
        }
        else
        {
            QJsonArray keysObj;
            for (int i = 0; i < m_pluginConfig.size(); i++)
            {
                keysObj.append(m_pluginConfig[i].keyword[i]);
            }

            pluginObj.insert("Keyword",keysObj);
        }

        pluginObj.insert("PluginMode",m_pluginConfig[i].mode);
        pluginObj.insert("Disabled",m_pluginConfig[i].disabled);

        if (!m_pluginConfig[i].acceptType.isEmpty())
        {
            pluginObj.insert("AcceptType",m_pluginConfig[i].acceptType);
        }

		array.append(pluginObj);
	}
    oPluginSettings["Plugins"] = array;
    oRoot["PluginSettings"] = oPluginSettings;

    for (int i = 0; i< m_indexCfg.indexPath.size(); i++)
    {
        QJsonObject oPath;
        oPath["Location"] = m_indexCfg.indexPath[i];
        pathArray.append(oPath);
    }
    oIndexSetting["ProgramSources"] = pathArray;
    oIndexSetting["EnableNormalIndex"] = m_indexCfg.enableNormalIndex;
    oIndexSetting["ClearHistory"] = m_indexCfg.clearHistory;
    oRoot["IndexSetting"] = oIndexSetting;

    oEpmSetting["proxy_ip"] = m_proxy_ip;
    oEpmSetting["proxy_port"] = m_proxy_port;
    oRoot["EpmSetting"] = oEpmSetting;

    document.setObject(oRoot);

	QByteArray byte_array = document.toJson(QJsonDocument::Indented);

    QFile file("Settings\\Settings.json");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return false;
    }
    file.write(byte_array);
    file.flush();
	file.close();
	
	return true;
}

int Settings::find(const QString &uuid)
{
    if (m_pluginConfig.isEmpty()) return -1;

    for (int i = 0; i < m_pluginConfig.size(); i++)
    {
        if (m_pluginConfig[i].id.compare(uuid,Qt::CaseInsensitive) == 0)
        {
            return i;
        }
    }

    return -1;
}

bool Settings::isKeywordExsit(const QString &keyword)
{
    if (m_pluginConfig.isEmpty()) return false;

    for (int i = 0; i < m_pluginConfig.size(); i++)
    {
        QStringList& keyList = m_pluginConfig[i].keyword;
        if (keyList[0].isEmpty())
        {
            continue;
        }

        foreach (QString key,keyList)
        {
            if (key.compare(keyword,Qt::CaseInsensitive) == 0)
            {
                return true;
            }
        }
    }

    return false;
}

PluginConfig Settings::getPluginCfg(const QString &uuid)
{
    if (m_pluginConfig.isEmpty()) return PluginConfig();

    for (int i = 0; i < m_pluginConfig.size(); i++)
    {
        if (m_pluginConfig[i].id.compare(uuid,Qt::CaseInsensitive) == 0)
        {
            return m_pluginConfig[i];
        }
    }

    return PluginConfig();
}

Settings* GetSettings()
{
	static Settings setting;
	static bool s_initialed = false;

	if (!s_initialed)
	{
		s_initialed = true;
		setting.load();
	}

	return &setting;
}
