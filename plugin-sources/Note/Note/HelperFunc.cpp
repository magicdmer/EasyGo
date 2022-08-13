#include "stdafx.h"
#include "HelperFunc.h"
#include "json/json.h"

#ifdef _DEBUG
#pragma comment(lib,"jsoncpp_d.lib")
#else
#pragma comment(lib,"jsoncpp_r.lib")
#endif

typedef void (WINAPI *pRa_ChangeQuery)(const char* pQuery);
typedef void (WINAPI *pRa_ShowMsg)(const char* title, const char* msg);
typedef void (WINAPI *pRa_EditFile)(const char* title, const char* filePath);

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

void Ra_ShowMsg(const char* title, const char* msg)
{
	pRa_ShowMsg pShowMsg = (pRa_ShowMsg)GetProcAddress(GetModuleHandle(NULL), "Ra_ShowMsg");
	if (pShowMsg) {
		pShowMsg(title, msg);
	}
}

void Ra_EditFile(const char* title, const char* filePath)
{
	pRa_EditFile pEditFile = (pRa_EditFile)GetProcAddress(GetModuleHandle(NULL), "Ra_EditFile");
	if (pEditFile) {
		pEditFile(title, filePath);
	}
}


void GetDirFiles(string dir, vector<string>& fileList)
{
	WIN32_FIND_DATAA stFD ;
	HANDLE h;
	string temp;

	temp=dir+"\\*";
	h=FindFirstFileA(temp.c_str(),&stFD);
	while(FindNextFileA(h,&stFD))
	{
		if (stFD.cFileName[0] == '.')
		{
			continue;
		}

		temp=dir+"\\"+stFD.cFileName;
		if(!PathIsDirectoryA(temp.c_str()) )
		{
			fileList.push_back(stFD.cFileName);
		}
	}
	FindClose(h);

	return ;
}
