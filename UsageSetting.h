#ifndef USAGESETTING_H
#define USAGESETTING_H


class UsageSetting
{
public:
    UsageSetting();
public:
    bool load();
    bool save();
public:
    int m_usage;
};

UsageSetting* GetUsageSetting();

#endif // USAGESETTING_H
