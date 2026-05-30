#ifndef INDEXTASK_H
#define INDEXTASK_H

#include <QObject>
#include "IndexDatabase.h"
#include <QThread>
#include <QRunnable>
#include <QThreadPool>
#include "Settings.h"

#ifdef Q_OS_WIN32
#include "FastSearch.h"
#endif

#ifdef Q_OS_LINUX
#include <QSet>
class QSettings;
#endif

class IndexTask : public QThread
{
    Q_OBJECT
public:
#ifdef Q_OS_WIN32
    IndexTask(){ fs = new tyrlib::FastSearch(); }
    ~IndexTask() { delete fs; }
#else
    IndexTask(){}
    ~IndexTask() {}
#endif
    void run() Q_DECL_OVERRIDE;
public:
	void index();
	void stop();
    static bool isIndexing() { return m_indexing; }
private:
    void indexProgram(QString strDir);
#ifdef Q_OS_LINUX
    void getProgramInfo(QString desktopFilePath, ProgramInfo& info);
    bool appendProgramInfo(const ProgramInfo& info, const QString& dedupeKey);
    // 依据 freedesktop 规范判断该 desktop 条目是否应被索引过滤掉
    bool shouldSkipDesktopEntry(QSettings& desktopFile);
#endif
private:
#ifdef Q_OS_WIN32
    tyrlib::FastSearch *fs;
#else
    QSet<QString> m_seenProgramKeys;
#endif
    QVector<ProgramInfo> m_programs;
	bool m_run;
    static bool m_indexing;
};

#endif // INDEXTASK_H
