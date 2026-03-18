#ifndef INDEXTASK_H
#define INDEXTASK_H

#include <QObject>
#include "IndexDatabase.h"
#include <QThread>
#include <QRunnable>
#include <QThreadPool>
#include "Settings.h"
#include "FastSearch.h"

class IndexTask : public QThread
{
    Q_OBJECT
public:
    IndexTask(){ fs = new tyrlib::FastSearch(); }
    ~IndexTask() { delete fs; }
    void run() Q_DECL_OVERRIDE;
public:
	void index();
	void stop();
    static bool isIndexing() { return m_indexing; }
private:
    void indexProgram(QString strDir);
private:
    tyrlib::FastSearch *fs;
    QVector<ProgramInfo> m_programs;
	bool m_run;
    static bool m_indexing;
};

#endif // INDEXTASK_H
