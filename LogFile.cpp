#include "LogFile.h"
#include <QDate>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QFileInfo>

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
# pragma execution_character_set("utf-8")
#endif

CEasyLog::CEasyLog()
{
    QDir dir;
    if (!dir.exists("Log"))
    {
        dir.mkdir("Log");
    }
}

CEasyLog::~CEasyLog(void)
{
}

CEasyLog &CEasyLog::Instance()
{
    static CEasyLog CEasyLog_Instance;
    return CEasyLog_Instance;
}

void CEasyLog::writeLog(LOG_LEVEL level, QString strLog)
{
    m_mutex.lock();
    QDate date = QDate::currentDate();
    QString strFileName = date.toString("yyyy-MM-dd");
    strFileName = "log/" + strFileName + ".log";

    QFile file(strFileName);
    if (!file.open(QIODevice::ReadWrite | QIODevice::Append))
    {
        m_mutex.unlock();
        return;
    }

    QString szLevel;
    switch (level)
    {
    case LOG_INFO:
        szLevel = "[INFO ]";
        break;
    case LOG_DEBUG:
        szLevel = "[DEBUG]";
        break;
    case LOG_WARN:
        szLevel = "[WARN ]";
        break;
    case LOG_ERROR:
        szLevel = "[ERROR]";
        break;
    default:
        szLevel = "[INFO ]";
        break;
    }

    QTextStream out(&file);
    QString strTime = QString("[%1]").arg(QDateTime::currentDateTime().toString("hh:mm:ss"));
#ifdef Q_OS_WIN32
    out << szLevel << strTime << "\t" << strLog << "\r\n";
#else
    out << szLevel << strTime << "\t" << strLog << "\n";
#endif
    file.flush();
    file.close();
    m_mutex.unlock();
}

void CEasyLog::clearLog()
{
    QDateTime now = QDateTime::currentDateTime();
    QDateTime dateTime1 = now.addDays(-7);
    QDateTime dateTime2;

    QDir dir("Log");
    QStringList filters;    //设置过滤器
    filters<<"*.log";   //过滤留下log文件
    dir.setNameFilters(filters);
    foreach (QFileInfo mItem, dir.entryInfoList())
    {
        dateTime2 = QDateTime::fromString(mItem.baseName(), "yyyy-MM-dd");
        if (dateTime2 < dateTime1)
        {
            dir.remove(mItem.absoluteFilePath());
        }
    }
}
