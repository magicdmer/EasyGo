#include "stdafx.h"
#include "HelperFunc.h"
#include "json/json.h"

#ifdef _DEBUG
#pragma comment(lib,"jsoncpp_d.lib")
#else
#pragma comment(lib,"jsoncpp_r.lib")
#endif

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

		item["Action"] = action;

		root["Results"].append(item);
	}

	string strJson = root.toStyledString();

	return strJson;
}

BOOL RunCommandA(char* wCmd)
{
	//createprocess第二个参数不能为readonly的字符串
	char command[MAX_PATH] = {0};
	strcpy(command,wCmd);

	STARTUPINFOA si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	si.cb = sizeof(si);

	BOOL bRet = CreateProcessA(
		NULL,
		command, 
		NULL,
		NULL,
		FALSE,
		DETACHED_PROCESS,
		NULL,
		NULL,
		&si,
		&pi);
	if (!bRet)
	{
		return FALSE;
	}

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	return TRUE;
}