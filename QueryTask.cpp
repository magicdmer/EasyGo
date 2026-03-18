#include "QueryTask.h"
#include "MainDialog.h"
#include "PluginManager.h"
#include "LogFile.h"
#include "ProgramPlugin.h"
#include "Settings.h"
#include <QDir>

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
# pragma execution_character_set("utf-8")
#endif

QueryTask::QueryTask(QObject* parent):
    QThread(parent),
    m_run(true),
    m_isDrop(false),
    m_plugin(nullptr)
{
    qRegisterMetaType<QVariant>("QVariant");
}

void QueryTask::query(Plugin* plugin,Query& query,bool isDrop)
{
    m_run = true;

    m_plugin = plugin;
    m_query = query;
    m_isDrop = isDrop;

    start();
}

void QueryTask::query(bool isDrop)
{
    m_run = true;

    m_isDrop = isDrop;

    start();
}

void QueryTask::setQuery(Plugin *plugin, Query &query)
{
    m_run = true;

    m_plugin = plugin;
    m_query = query;
}

void QueryTask::stop()
{
    m_run = false;
    wait();
}

void QueryTask::run()
{
    QVector<Result> resultList;

    if (m_isDrop)
    {
        if (m_query.keyword.isEmpty())
        {
            Result result;
            result.title = "文件类型为空";
            result.subTitle = "";
            result.iconPath = QDir::currentPath() + QString("/Images/app.ico");
            resultList.append(result);
        }
        else
        {
            QVector<Plugin*> vecPlugin;
            if (GetPluginMananger()->getValidTypePlugin(m_query.keyword,vecPlugin))
            {
                for (int i = 0; i < vecPlugin.size(); i++)
                {
                    if (!vecPlugin[i]->query(m_query,resultList))
                    {
                        qLog(QString("[%1] 查询结果失败!").arg(vecPlugin[i]->m_info.name));
                    }
                }
            }
            else
            {
                Result result;
                result.title = "未找到合适的处理插件";
                result.subTitle = "";
                result.iconPath = QDir::currentPath() + QString("/Images/app.ico");
                resultList.append(result);
            }
        }
    }
    else
    {
        if (!m_plugin->query(m_query,resultList))
        {
            qLog(QString("[%1] 查询结果失败!").arg(m_plugin->m_info.name));
        }
    }

    if (!m_run)
    {
        emit cancel();
        return;
    }

    if (resultList.isEmpty())
    {
        emit resultError();
    }
    else
    {
        int maxResults = GetSettings()->m_maxResultsToShow;
        if (resultList.size() > maxResults && resultList[0].id != EPM_PLUGIN_ID)
        {
            int rCount = resultList.size() - maxResults;
            resultList.remove(maxResults,rCount);
        }

        TaskResult task;
        task.id = m_id;
        task.vecResult.swap(resultList);

        QVariant variant;
        variant.setValue(task);
        emit resultReady(variant);
    }

}
