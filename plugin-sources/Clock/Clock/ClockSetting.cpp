#include "StdAfx.h"
#include "ClockSetting.h"
#include "json\json.h"
#include <sys/types.h>
#include <sys/stat.h>

extern string g_PluginPath;

ClockSetting::ClockSetting(void)
{
	m_settingPath = g_PluginPath + string("/ClockSetting.json");
}


ClockSetting::~ClockSetting(void)
{
}

bool ClockSetting::load()
{
	FILE* fp = fopen(m_settingPath.c_str(),"r");
	if (fp == NULL)
	{
		return false;
	}

	struct _stat st;
	int ret = _stat(m_settingPath.c_str(),&st);
	if (ret == -1)
	{
		fclose(fp);
		return false;
	}

	char* buffer = new char[st.st_size];
	ret = fread(buffer,1,st.st_size,fp);
	if (ret <= 0)
	{
		delete buffer;
		fclose(fp);
		return false;
	}
	fclose(fp);

	Json::Reader reader;
	Json::Value root;
	if (!reader.parse(buffer,root))
	{
		delete buffer;
		return false;
	}

	Json::Value alarmArray = root["Alarms"];
	for (int i = 0; i < alarmArray.size(); i++)
	{
		Clock item;
		item.name = alarmArray[i]["Name"].asString();
		item.date = alarmArray[i]["Date"].asInt();
		item.time = alarmArray[i]["Time"].asString();
		if (isValidClock(item))
		{
			m_clocks.push_back(item);
		}
	}

	delete buffer;

	return true;
}

bool ClockSetting::save()
{
	FILE* fp = fopen(m_settingPath.c_str(),"w");
	if (fp == NULL)
	{
		return false;
	}

	Json::Value root;
	for (int i = 0; i < m_clocks.size(); i++)
	{
		Json::Value item;
		item["Name"] = m_clocks[i].name;
		item["Date"] = m_clocks[i].date;
		item["Time"] = m_clocks[i].time;

		root["Alarms"].append(item);
	}

	if (m_clocks.size() == 0)
	{
		root["Alarms"].resize(0);
	}

	string strJson = root.toStyledString();

	fwrite(strJson.c_str(),1,strJson.size(),fp);

	fclose(fp);

	return true;
}

bool ClockSetting::isValidClock(Clock& stClock)
{
	SYSTEMTIME stTime;
	GetLocalTime(&stTime);

	int iData = stTime.wYear + stTime.wMonth + stTime.wDay;
	if (stClock.date != iData)
	{
		return false;
	}

	int hour,minites;
	sscanf(stClock.time.c_str(),"%2d:%2d",&hour,&minites);

	int iTime = hour + minites;
	if (iTime < stTime.wHour + stTime.wMinute)
	{
		return false;
	}

	return true;
}

void ClockSetting::delClock(string sTime)
{
	vector<Clock>::iterator Iter;
	for (Iter = m_clocks.begin(); Iter!=m_clocks.end(); Iter++)
	{
		if (Iter->time == sTime)
		{
			m_clocks.erase(Iter);
			break;
		}
	}
}

ClockSetting* GetSettings()
{
	static ClockSetting setting;
	static bool s_initialed = false;

	if (!s_initialed)
	{
		s_initialed = true;
		setting.load();
	}

	return &setting;
}
