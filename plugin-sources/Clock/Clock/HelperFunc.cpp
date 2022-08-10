#include "stdafx.h"
#include "HelperFunc.h"
#include "json/json.h"
#include "ClockSetting.h"
#include <mmsystem.h>

#pragma comment(lib,"Winmm.lib")

#ifdef _DEBUG
#pragma comment(lib,"jsoncpp_d.lib")
#else
#pragma comment(lib,"jsoncpp_r.lib")
#endif

typedef void (WINAPI *pRa_ChangeQuery)(const char* pQuery);

bool ParseQuery(string strQuery,QueryText& stQuery)
{
	Json::Reader reader;
	Json::Value value;

	if (!reader.parse(strQuery,value))
	{
		return false;
	}

	stQuery.rawQuery = value["RawQuery"].asString();
	stQuery.keyword = value["Keyword"].asString();
	stQuery.parameter = value["Parameter"].asString();

	return true;
}

bool ParseResult(string strResult, Result& stResult)
{
	Json::Reader reader;
	Json::Value value;

	if (!reader.parse(strResult,value))
	{
		return false;
	}

	stResult.id = value["ID"].asString();
	stResult.title = value["Title"].asString();
	stResult.subTitle = value["SubTitle"].asString();

	return true;
}

string CreateResultJson(vector<Result> resultList)
{
	Json::Value root;

	for (int i = 0; i < resultList.size(); i++)
	{
		Json::Value item;
		item["Title"] = resultList[i].title;
		item["SubTitle"] = resultList[i].subTitle;
		item["IconPath"] = resultList[i].iconPath;
		
		Json::Value action;
		action["FuncName"] = resultList[i].action.funcName;
		action["Parameter"] = resultList[i].action.parameter;
		action["HideWindow"] = resultList[i].action.hideWindow;

		item["Action"] = action;

		root["Results"].append(item);
	}

	string strJson = root.toStyledString();

	return strJson;
}

string strToLower(const string &str)
{
	string strTmp = str;
	transform(strTmp.begin(),strTmp.end(),strTmp.begin(),tolower);
	return strTmp;
}

bool compareNoCase(const string &strA,const string &strB)
{
	string str1 = strToLower(strA);
	string str2 = strToLower(strB);
	return (str1 == str2);
}

void Ra_ChangeQuery(const char* strQuery)
{
	pRa_ChangeQuery pChangeQuery = (pRa_ChangeQuery)GetProcAddress(GetModuleHandle(NULL), "Ra_ChangeQuery");
	if (pChangeQuery) {
		pChangeQuery(strQuery);
	}
}

MMRESULT g_IdTimerEvent = 0;

void CALLBACK TimeProc( 
	UINT uID,       
	UINT uMsg,     
	DWORD dwUser,   
	DWORD dw1,     
	DWORD dw2       
	) 
{ 

	if (GetSettings()->m_clocks.empty())
	{
		StopAlarmTimer();
		return;
	}

	SYSTEMTIME stTime;
	GetLocalTime(&stTime);
	
	int iData = stTime.wYear + stTime.wMonth + stTime.wDay;

	for (int i = 0; i < GetSettings()->m_clocks.size(); i++)
	{
		int hour,minites;
		sscanf(GetSettings()->m_clocks[i].time.c_str(),"%d:%d",&hour,&minites);

		if (GetSettings()->m_clocks[i].date == iData &&
			hour == stTime.wHour && minites == stTime.wMinute)
		{
			MessageBoxA(NULL,"时间到了",GetSettings()->m_clocks[i].name.c_str(),0);
			GetSettings()->m_clocks.erase(begin(GetSettings()->m_clocks)+i);
			GetSettings()->save();
			break;
		}
	}
}

bool StartAlarmTimer()
{
	if (g_IdTimerEvent != 0)
	{
		return true;
	}

	g_IdTimerEvent = timeSetEvent( 
		5000,//延时1秒 
		0, 
		(LPTIMECALLBACK)TimeProc, 
		0, 
		(UINT)TIME_PERIODIC); 
	if (g_IdTimerEvent == 0)
	{
		return false;
	}

	return true;
}

void StopAlarmTimer()
{
	if (g_IdTimerEvent != 0)
	{
		timeKillEvent(g_IdTimerEvent);
		g_IdTimerEvent = 0;
	}
}