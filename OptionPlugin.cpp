#include "OptionPlugin.h"
#include <QDir>
#include <QMessageBox>
#include "MainDialog.h"
#include "HelperFunc.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
# pragma execution_character_set("utf-8")
#endif

OptionPlugin::OptionPlugin()
{
    m_info.id = OPTION_PLUGIN_ID;
    m_info.name = tr("EasyGo快捷设置");
    m_info.keyword.append("/");
    m_info.pluginType = "c++";
    m_info.author = "magicdmer";
    m_info.enableSeparate = 0;
    m_iconPath = QDir::currentPath() + QString("/Images/settings.png");
    m_guid = m_info.id;
    m_mode = RealMode;
}

bool OptionPlugin::query(Query query,QVector<Result>& vecResult)
{
    Q_UNUSED(query);

    Result result;
    result.id = OPTION_PLUGIN_ID;
    result.title = tr("设置");
    result.subTitle = tr("打开EasyGo的设置界面");
    result.iconPath = m_iconPath;
    result.action.funcName = "setOption";
    result.action.parameter = "";
    result.action.hideWindow = false;
    vecResult.append(result);

    result.title = tr("插件管理");
    result.subTitle = tr("打开Epm管理器");
    result.iconPath = m_iconPath;
    result.action.funcName = "Ra_ChangeQuery";
    result.action.parameter = "epm ";
    result.action.hideWindow = false;
    vecResult.append(result);

    result.title = tr("主题设置");
    result.subTitle = tr("打开EasyGo的主题设置");
    result.iconPath = m_iconPath;
    result.action.funcName = "Ra_ChangeQuery";
    result.action.parameter = "/theme ";
    result.action.hideWindow = false;
    vecResult.append(result);

    result.title = tr("重载");
    result.subTitle = tr("重载主题和插件列表");
    result.iconPath = m_iconPath;
    result.action.funcName = "Ra_Reload";
    result.action.parameter = "";
    result.action.hideWindow = false;
    vecResult.append(result);

    result.title = tr("检查更新");
    result.subTitle = tr("检查EasyGo更新");
    result.iconPath = m_iconPath;
    result.action.funcName = "checkUpdate";
    result.action.parameter = "";
    result.action.hideWindow = false;
    vecResult.append(result);

    result.title = tr("重启程序");
    result.subTitle = tr("重启EasyGo");
    result.iconPath = m_iconPath;
    result.action.funcName = "restartApp";
    result.action.parameter = "";
    result.action.hideWindow = false;
    vecResult.append(result);

    result.title = tr("重建索引");
    result.subTitle = tr("后台重新索引数据库");
    result.iconPath = m_iconPath;
    result.action.funcName = "reIndex";
    result.action.parameter = "";
    result.action.hideWindow = false;
    vecResult.append(result);

    result.title = tr("退出程序");
    result.subTitle = tr("关闭EasyGo");
    result.iconPath = m_iconPath;
    result.action.funcName = "exitApp";
    result.action.parameter = "";
    result.action.hideWindow = false;
    vecResult.append(result);

    return true;
}

bool OptionPlugin::getMenu(Result &result, QVector<Result> &vecMenu)
{
    Q_UNUSED(result);
    Q_UNUSED(vecMenu);

    return false;
}

void OptionPlugin::itemClick(Result &item, QObject *parent)
{
    m_parent = parent;
    PluginAction& action = item.action;
    if (action.funcName == "Ra_ChangeQuery")
    {
        Ra_ChangeQuery(action.parameter, parent);
    }
    else
    {
        QByteArray ba = action.funcName.toUtf8();
        QMetaObject::invokeMethod(this,ba.data());
    }
}

void OptionPlugin::setOption()
{
    MainDialog* dlg = qobject_cast<MainDialog*>(m_parent);
    dlg->sltSet();
}

void OptionPlugin::checkUpdate()
{
    MainDialog* dlg = qobject_cast<MainDialog*>(m_parent);

    QString newVersion;
    if (CheckUpdate(newVersion))
    {
        QString currentVersion(EASYGO_VERSION);
        if (CompareVersion(newVersion, currentVersion) > 0)
        {
            QMessageBox::information(dlg, tr("更新提示"),
                    tr("有新版本发布，版本: %1\n请前往关于中的网址更新").arg(newVersion));
        }
        else
        {
            QMessageBox::information(dlg, tr("提示"), tr("你已经是最新版本！"));
        }
    }
    else
    {
        QMessageBox::warning(dlg, tr("提示"), tr("无法获取版本信息，请检查网络！"));
    }
}

void OptionPlugin::restartApp()
{
    qApp->exit(773);
}

void OptionPlugin::exitApp()
{
    MainDialog* dlg = qobject_cast<MainDialog*>(m_parent);
    dlg->sltExit();
}

void OptionPlugin::sltTaskFinished()
{
    MainDialog* dlg = qobject_cast<MainDialog*>(m_parent);
    dlg->showTip(tr("提示"),tr("索引完毕"));
    delete m_task;
    m_task = nullptr;
}

void OptionPlugin::reIndex()
{
    m_task = new IndexTask();
    connect(m_task,SIGNAL(finished()),this,SLOT(sltTaskFinished()));
    m_task->index();
}



