#pragma once

struct Clock{
	string name;
	int date;
	string time;
};

class ClockSetting
{
public:
	ClockSetting(void);
	~ClockSetting(void);
public:
	bool load();
	bool save();
	bool isValidClock(Clock& stClock);
	void addClock(Clock& clock) { m_clocks.push_back(clock); }
	void delClock(string sTime);
public:
	vector<Clock> m_clocks;
private:
	string m_settingPath;
};

ClockSetting* GetSettings();
