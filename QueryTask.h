#ifndef QUERYTASK_H
#define QUERYTASK_H

#include <QThread>
#include "Plugin.h"
#include <QVariant>

struct TaskResult{
    int id;
    QVector<Result> vecResult;
};
Q_DECLARE_METATYPE(TaskResult)

class QueryTask : public QThread
{
	Q_OBJECT
public:
    QueryTask(QObject* parent);
    void run() Q_DECL_OVERRIDE;
public:
    void query(Plugin* plugin,Query& query,bool isDrop = false);
    void query(bool isDrop = false);
    void setQuery(Plugin* plugin,Query& query);
    void stop();
    Plugin* getPlugin() {return m_plugin;}
signals:
    void resultReady(QVariant);
	void resultError();
    void cancel();
private:
    Query m_query;
    Plugin* m_plugin;
    bool m_run;
    bool m_isDrop;
public:
    int m_id;
};

#endif // QUERYTASK_H
