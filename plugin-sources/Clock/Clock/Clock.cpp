// Clock.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "Clock.h"
#include "HelperFunc.h"
#include "ClockSetting.h"

string g_PluginPath;
bool g_setclock = false;

// 这是导出函数的一个示例。
bool InitPlugin(char* pPluginPath)
{
	g_PluginPath = pPluginPath;

	if (GetSettings()->m_clocks.size() != 0)
	{
		StartAlarmTimer();
	}

	return true;
}

void AddQuery(string& strQuery,vector<Result>& vecResult)
{
	Result result;
	if (strQuery.empty())
	{
		if (g_setclock)
		{
			result.title = "设置成功";
			g_setclock = false;
		}
		else
		{
			result.title = "添加闹钟";
			result.subTitle = "添加: 时间格式 HH:MM,闹钟名字任何字符串";
		}
	}
	else
	{
		string timeParameter;
		string nameParameter;
		int index = strQuery.find(" ");
		if (index != -1)
		{
			timeParameter = strQuery.substr(0,index);
			nameParameter = strQuery.substr(index+1);
		}
		else
		{
			timeParameter = strQuery;
		}

		result.title = "添加闹钟";

		char fmtBuf[512] = {0};
		index = timeParameter.find(":");
		if (index == -1 || index == timeParameter.size() - 1)
		{
			result.subTitle = "添加: 时间格式 HH:MM,闹钟名字任何字符串";
		}
		else if (nameParameter.empty())
		{
			regex reg("^(0[0-9]|1[0-9]|2[0-3]|[0-9]):[0-5][0-9]$");
			bool is_match = regex_match(timeParameter,reg);
			if (is_match)
			{
				sprintf(fmtBuf,"添加: 时间 %s 闹钟",timeParameter.c_str());
				result.subTitle = fmtBuf;
				result.action.funcName = "SetClock";
				result.action.parameter = strQuery;
				result.action.hideWindow = false;
			}
			else
			{
				result.subTitle = "添加: 时间格式 HH:MM,闹钟名字任何字符串";
			}
		}
		else
		{
			regex reg("^(0[0-9]|1[0-9]|2[0-3]|[0-9]):[0-5][0-9]$");
			bool is_match = regex_match(timeParameter,reg);
			if (is_match)
			{
				sprintf(fmtBuf,"添加: 时间 %s %s",timeParameter.c_str(),nameParameter.c_str());
				result.subTitle = fmtBuf;
				result.action.funcName = "SetClock";
				result.action.parameter = strQuery;
				result.action.hideWindow = false;
			}
			else
			{
				result.subTitle = "添加: 时间格式 HH:MM,闹钟名字任何字符串";
			}
		}	
	}

	vecResult.push_back(result);
}


bool Query(char* pQuery,char* pResult,int* length)
{
	static string strJson;

	if (pResult == NULL)
	{
		strJson.clear();
	}
	else
	{
		if (!strJson.empty())
		{
			strcpy(pResult,strJson.c_str());
			return true;
		}
	}

	QueryText stQuery;
	if (!ParseQuery(pQuery,stQuery))
	{
		return false;
	}

	vector<Result> vecResult;

	string queryStr = stQuery.parameter;
	if (queryStr.empty())
	{
		Result result;
		result.title = "Add a clock";
		result.subTitle = "添加一个闹钟";
		result.action.funcName = "Ra_ChangeQuery";
		result.action.parameter = "clock add ";
		result.action.hideWindow = false;
		vecResult.push_back(result);

		result.title = "Delete a clock";
		result.subTitle = "删除一个闹钟";
		result.action.funcName = "Ra_ChangeQuery";
		result.action.parameter = "clock delete ";
		result.action.hideWindow = false;
		vecResult.push_back(result);

		result.title = "Show all clocks";
		result.subTitle = "显示所有闹钟";
		result.action.funcName = "Ra_ChangeQuery";
		result.action.parameter = "clock show ";
		result.action.hideWindow = false;
		vecResult.push_back(result);

		strJson = CreateResultJson(vecResult);
		if (NULL == pResult)
		{
			*length = strJson.size();
		}
		else
		{
			strcpy(pResult,strJson.c_str());
		}

		return true;
	}

	string subKeyword;
	string parameter;
	int index = queryStr.find(" ");
	if (index != -1)
	{
		subKeyword = queryStr.substr(0,index);
		parameter = queryStr.substr(index+1);
	}
	else
	{
		subKeyword = queryStr;
	}

	if (compareNoCase(subKeyword,"add"))
	{
		AddQuery(parameter,vecResult);
	}
	else if (compareNoCase(subKeyword,"delete"))
	{
		Result result;
		if (GetSettings()->m_clocks.empty())
		{
			result.title = "没有闹钟";
			vecResult.push_back(result);
		}
		else
		{
			for (int i = 0; i < GetSettings()->m_clocks.size(); i++)
			{
				Clock& clock = GetSettings()->m_clocks[i];
				result.title = clock.name;
				result.subTitle = clock.time;
				result.action.funcName = "DeleteClock";
				result.action.parameter = clock.time;
				result.action.hideWindow = false;
				vecResult.push_back(result);
			}
		}
	}
	else if (compareNoCase(subKeyword,"show"))
	{
		Result result;
		if (GetSettings()->m_clocks.empty())
		{
			result.title = "没有闹钟";
			vecResult.push_back(result);
		}
		else
		{
			for (int i = 0; i < GetSettings()->m_clocks.size(); i++)
			{
				Clock& clock = GetSettings()->m_clocks[i];
				result.title = clock.name;
				result.subTitle = clock.time;
				vecResult.push_back(result);
			}
		}
	}
	else
	{
		Result result;
		result.title = "clock";
		result.subTitle = "一款简单的闹钟程序";
		vecResult.push_back(result);
	}

	strJson = CreateResultJson(vecResult);
	if (NULL == pResult)
	{
		*length = strJson.size();
	}
	else
	{
		strcpy(pResult,strJson.c_str());
	}

	return true;
}

bool GetContextMenu(char* result, char* pMenu,int* length)
{
	return false;
}

void SetClock(char* pParam)
{
	string strQuery = pParam;
	string timeParameter;
	string nameParameter;
	int index = strQuery.find(" ");
	if (index != -1)
	{
		timeParameter = strQuery.substr(0,index);
		nameParameter = strQuery.substr(index+1);
	}
	else
	{
		timeParameter = strQuery;
	}

	if (nameParameter.empty())
	{
		nameParameter = "闹钟";
	}

	SYSTEMTIME stTime;
	GetLocalTime(&stTime);

	int iData = stTime.wYear + stTime.wMonth + stTime.wDay;

	Clock ck;
	ck.name = nameParameter;
	ck.time = timeParameter;
	ck.date = iData;

	GetSettings()->addClock(ck);
	GetSettings()->save();

	StartAlarmTimer();

	g_setclock = true;

	Ra_ChangeQuery("clock add ");
}

void DeleteClock(char* pTime)
{
	GetSettings()->delClock(pTime);
	GetSettings()->save();

	Ra_ChangeQuery("clock delete");
}