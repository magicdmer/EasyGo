#pragma once

struct QueryText{
	string rawQuery;
	string keyword;
	string parameter;
};

struct PluginAction{
	string funcName;
	string parameter;
	bool hideWindow;

	PluginAction(){
		hideWindow = true;
	}
};

struct Result{
	string id;
	string title;
	string subTitle;
	string iconPath;
	string extraData;
	PluginAction action;
};

bool ParseQuery(string strQuery,QueryText& stQuery);
bool ParseResult(string strResult, Result& stResult);
string CreateResultJson(vector<Result> resultList);
bool compareNoCase(const string &strA,const string &strB);
void Ra_ChangeQuery(const char* strQuery);
bool StartAlarmTimer();
void StopAlarmTimer();