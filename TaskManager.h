#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <QObject>
#include "QueryTask.h"

class TaskManager:public QObject
{
    Q_OBJECT
public:
    TaskManager();
    ~TaskManager();
public:
    QueryTask* getIdleTask();
private:
    QVector<QueryTask*> m_tasks;
};


TaskManager* GetTaskManager();

#endif // TASKMANAGER_H
