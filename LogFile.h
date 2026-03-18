#ifndef LOGFILE_H_
#define LOGFILE_H_
#include <QMutex>
#include <QStringList>

enum LOG_LEVEL{
    LOG_INFO = 0,
    LOG_DEBUG,
    LOG_WARN,
    LOG_ERROR,
};

extern bool g_IsDebug;

class CEasyLog
{
public:
    CEasyLog(/*LPCSTR lpFile = NULL*/);
    virtual ~CEasyLog(void);

public:
    QStringList m_recordList;

private:
    QString m_strFile;
    QMutex m_mutex;

public:
    void writeLog(LOG_LEVEL level,QString strLog);
    void clearLog();
    static CEasyLog &Instance();
};

#define qLog(x)	CEasyLog::Instance().writeLog(LOG_INFO,x);
#define qDbg(x) if(g_IsDebug){CEasyLog::Instance().writeLog(LOG_DEBUG,x);}
#define qWarn(x) CEasyLog::Instance().writeLog(LOG_WARN,x);
#define qError(x) CEasyLog::Instance().writeLog(LOG_ERROR,x);

#define qclearLog() CEasyLog::Instance().clearLog();

#endif
