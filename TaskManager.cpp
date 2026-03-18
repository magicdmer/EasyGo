#include "TaskManager.h"

TaskManager::TaskManager()
{

}

TaskManager::~TaskManager()
{

}

QueryTask* TaskManager::getIdleTask()
{
    QueryTask* idleTask = nullptr;

    bool invalid = false;
    foreach(auto task,m_tasks)
    {
        if (task->isFinished())
        {
            invalid = true;
            idleTask = task;
            break;
        }
    }

    if (!invalid)
    {
        QueryTask* newTask = new QueryTask(this);
        m_tasks.append(newTask);
        idleTask = newTask;
    }

    return idleTask;
}

TaskManager* GetTaskManager()
{
    static TaskManager manager;
    static bool s_initialed = false;

    if (!s_initialed)
    {
        s_initialed = true;
    }

    return &manager;
}
