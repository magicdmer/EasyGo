#include "EpmPlugin.h"
#include <QDir>
#include "Settings.h"
#include "PluginManager.h"
#include "HelperFunc.h"
#include "MainDialog.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QtConcurrent>
#include <QFuture>
#include <QUrl>
#include <QMessageBox>

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
# pragma execution_character_set("utf-8")
#endif

EpmPlugin::EpmPlugin() : Plugin()
{
    m_info.id = EPM_PLUGIN_ID;
    m_info.name = tr("Epm");
    m_info.keyword.append("epm");
    m_info.pluginType = "c++";
    m_info.author = "magicdmer";
    m_iconPath = QDir::currentPath() + QString("/Images/plugin.png");
    m_guid = m_info.id;
    m_mode = RealMode;
}

bool EpmPlugin::query(Query query,QVector<Result>& vecResult)
{
    QString strQuery = query.parameter.trimmed();
    if (strQuery.isEmpty())
    {
        Result result;
        result.id = EPM_PLUGIN_ID;
        result.title = tr("list");
        result.subTitle = tr("列举出本地安装的插件列表");
        result.iconPath = m_iconPath;
        result.action.funcName = "Ra_ChangeQuery";
        result.action.parameter = "epm list ";
        result.action.hideWindow = false;
        vecResult.append(result);

        result.title = tr("install");
        result.subTitle = tr("通过github在线插件仓库安装插件");
        result.iconPath = m_iconPath;
        result.action.funcName = "Ra_ChangeQuery";
        result.action.parameter = "epm install ";
        result.action.hideWindow = false;
        vecResult.append(result);

        result.title = tr("uninstall");
        result.subTitle = tr("卸载本地插件");
        result.iconPath = m_iconPath;
        result.action.funcName = "Ra_ChangeQuery";
        result.action.parameter = "epm uninstall ";
        result.action.hideWindow = false;
        vecResult.append(result);

        result.title = tr("proxy");
        result.subTitle = tr("设置本地代理，因为访问的github获取插件，所以酌情设置");
        result.iconPath = m_iconPath;
        result.action.funcName = "Ra_ChangeQuery";
        result.action.parameter = "epm proxy ";
        result.action.hideWindow = false;
        vecResult.append(result);

        return true;
    }

    QStringList paraList = strQuery.split(' ',QString::SkipEmptyParts);
    if(paraList.size() == 2)
    {
        strQuery = paraList[0];
    }

    if (strQuery == "list")
    {
        Settings* setting = GetSettings();
        for (int i = 0; i < setting->m_pluginConfig.size(); i++)
        {
            PluginConfig& cfg = setting->m_pluginConfig[i];
            Plugin* info = GetPluginMananger()->getPlugin(cfg.id);
            if (!info)
            {
                continue;
            }

            if (paraList.size() == 2)
            {
                QString para = paraList[1];
                if (!info->m_info.name.startsWith(para,Qt::CaseInsensitive) &&
                    !info->m_info.keyword[0].startsWith(para,Qt::CaseInsensitive))
                {
                    continue;
                }
            }

            Result result;
            result.id = EPM_PLUGIN_ID;
            if (info->m_info.keyword[0].isEmpty())
            {
                result.title = info->m_info.name + " - " +
                               info->m_info.author + " | " + info->m_info.acceptType;
            }
            else
            {
                QString keyword = cfg.keyword[0];
                if (keyword.isEmpty())
                {
                    keyword = info->m_info.keyword[0];
                }

                result.title = info->m_info.name + " - " +
                               info->m_info.author + " | " + keyword;

                result.action.funcName = "Ra_ChangeQuery";
                result.action.parameter = keyword + " ";
                result.action.hideWindow = false;
            }
            result.subTitle = info->m_info.description;
            result.iconPath = info->m_iconPath;
            vecResult.append(result);
        }
    }
    else if (strQuery == "install")
    {
        QString jsonData;
        QString repoUrl = "https://ghfast.top/https://raw.githubusercontent.com/magicdmer/EasyGo/main/repo.json";
        if (!GetSettings()->m_repo_url.isEmpty())
        {
            repoUrl = QString("%1/repo.json").arg(GetSettings()->m_repo_url);
        }
        if (m_vecPluginInfo.isEmpty() || paraList.size() == 1)
        {
            jsonData = HttpGet(repoUrl);
        }
        if (jsonData.isEmpty() && m_vecPluginInfo.isEmpty())
        {
            Result result;
            result.id = EPM_PLUGIN_ID;
            result.title = tr("获取插件列表失败");
            result.subTitle = tr("请检查网络:%1").arg(repoUrl);
            result.iconPath = m_iconPath;
            result.action.funcName = "Ra_CopyPath";
            result.action.parameter = result.subTitle;
            result.action.hideWindow = false;
            vecResult.append(result);
        }
        else
        {
            if (!jsonData.isEmpty())
            {
                parsePluginList(jsonData);
            }

            QString host_url = "https://ghfast.top/https://raw.githubusercontent.com/magicdmer/EasyGo/main/plugin";
            if (!GetSettings()->m_repo_url.isEmpty())
            {
                host_url = QString("%1/plugin").arg(GetSettings()->m_repo_url);
            }

            if (!m_vecPluginInfo.isEmpty())
            {
                for (int i = 0; i < m_vecPluginInfo.size(); i++)
                {
                    EpmPluginInfo &info = m_vecPluginInfo[i];

                    if (paraList.size() == 2)
                    {
                        QString para = paraList[1];
                        if (!info.name.startsWith(para,Qt::CaseInsensitive))
                        {
                            continue;
                        }
                    }

                    QString d_url = QString("%1/%2-%3.plugin").arg(host_url).
                            arg(info.name).arg(info.version);
                    Result result;
                    result.id = EPM_PLUGIN_ID;
                    Plugin* pPlugin = GetPluginMananger()->getPlugin(info.id);
                    if (pPlugin)
                    {
                        if (CompareVersion(info.version, pPlugin->m_info.version) > 0)
                        {
                            result.title = QString("[有更新] %1 %2 - %3").arg(info.name).
                                    arg(info.version).arg(info.author);
                        }
                        else
                        {
                            result.title = QString("[已安装] %1 %2 - %3").arg(info.name).
                                    arg(info.version).arg(info.author);
                        }
                    }
                    else
                    {
                        result.title = QString("[未安装] %1 %2 - %3").arg(info.name).
                                arg(info.version).arg(info.author);
                    }
                    result.subTitle = info.description;
                    result.iconPath = m_iconPath;
                    result.action.funcName = "installPlugin";
                    result.action.parameter = d_url;
                    result.action.hideWindow = false;

                    vecResult.append(result);
                }
            }
            else
            {
                Result result;
                result.id = EPM_PLUGIN_ID;
                result.title = tr("获取插件列表失败");
                result.subTitle = tr("请检查网络:%1").arg(host_url);
                result.iconPath = m_iconPath;
                result.action.funcName = "Ra_CopyPath";
                result.action.parameter = result.subTitle;
                result.action.hideWindow = false;
                vecResult.append(result);
            }
        }
    }
    else if (strQuery == "uninstall")
    {
        Settings* setting = GetSettings();
        for (int i = 0; i < setting->m_pluginConfig.size(); i++)
        {
            PluginConfig& cfg = setting->m_pluginConfig[i];
            Plugin* info = GetPluginMananger()->getPlugin(cfg.id);
            if (!info)
            {
                continue;
            }

            if (paraList.size() == 2)
            {
                QString para = paraList[1];
                if (!info->m_info.name.startsWith(para,Qt::CaseInsensitive) &&
                    !info->m_info.keyword[0].startsWith(para,Qt::CaseInsensitive))
                {
                    continue;
                }
            }

            Result result;
            result.id = EPM_PLUGIN_ID;
            if (info->m_info.keyword[0].isEmpty())
            {
                result.title = info->m_info.name + " - " +
                               info->m_info.author + " | " + info->m_info.acceptType;
            }
            else
            {
                result.title = info->m_info.name + " - " +
                               info->m_info.author + " | " + info->m_info.keyword[0];
            }
            result.subTitle = info->m_info.description;
            result.iconPath = info->m_iconPath;
            result.action.funcName = "Ra_Uninstall";
            result.action.parameter = info->m_guid;
            result.action.hideWindow = false;
            vecResult.append(result);
        }
    }
    else if (strQuery == "proxy")
    {
        if (paraList.size() == 1)
        {
            Result result;
            result.id = EPM_PLUGIN_ID;

            if (GetSettings()->m_proxy_ip.isEmpty())
            {
                result.title = tr("当前代理为空");
                result.subTitle = tr("HTTP代理设置，示例:127.0.0.1:1080");
                result.iconPath = m_iconPath;
            }
            else
            {
                result.title = tr("清除HTTP代理");
                result.subTitle = tr("当前的代理设置为: %1:%2").arg(GetSettings()->m_proxy_ip).
                        arg(GetSettings()->m_proxy_port);
                result.iconPath = m_iconPath;
                result.action.funcName = "setProxy";
                result.action.parameter = "";
                result.action.hideWindow = false;
            }
            vecResult.append(result);
        }
        else
        {
            QString para = paraList[1];
            Result result;
            result.id = EPM_PLUGIN_ID;
            result.title = tr("设置HTTP代理: %1").arg(para);
            result.subTitle = tr("HTTP代理设置，示例:127.0.0.1:1080");
            result.iconPath = m_iconPath;
            result.action.funcName = "setProxy";
            result.action.parameter = para;
            result.action.hideWindow = false;
            vecResult.append(result);
        }
    }
    else
    {
        Result result;
        result.id = EPM_PLUGIN_ID;
        result.title = strQuery;
        result.subTitle = tr("指令输入错误");
        result.iconPath = m_iconPath;
        vecResult.append(result);
    }

    return true;
}

bool EpmPlugin::getMenu(Result& result, QVector<Result>& vecMenu)
{
    Q_UNUSED(result);
    Q_UNUSED(vecMenu);
    return false;
}

void EpmPlugin::itemClick(Result &item,QObject* parent)
{
    PluginAction& action = item.action;
    QByteArray ba = action.funcName.toUtf8();
    QMetaObject::invokeMethod(this,ba.data(),Q_ARG(QString,action.parameter),Q_ARG(QObject*,parent));

    if (action.funcName == "Ra_Uninstall")
    {
        Ra_ChangeQuery("epm uninstall ", parent);
    }
    else if (action.funcName == "installPlugin")
    {
        Ra_ChangeQuery("epm install ", parent);
    }
}

bool sortResult(EpmPluginInfo& r1,EpmPluginInfo& r2)
{
    if (r1.timestamp > r2.timestamp)
    {
        return true;
    }

    return false;
}

bool EpmPlugin::parsePluginList(QString& strJson)
{
    QJsonParseError parseJsonErr;
    QJsonDocument document = QJsonDocument::fromJson(strJson.toUtf8(), &parseJsonErr);
    if (parseJsonErr.error != QJsonParseError::NoError)
    {
        return false;
    }

    m_vecPluginInfo.clear();
    QJsonObject jsonObject = document.object();
    QJsonArray keysObj = jsonObject["plugins"].toArray();
    for (int i = 0; i < keysObj.size(); i++)
    {
        EpmPluginInfo info;
        QJsonObject obj = keysObj[i].toObject();
        info.id = obj["id"].toString();
        info.name = obj["name"].toString();
        info.description = obj["description"].toString();
        info.version = obj["version"].toString();
        info.min_require = obj["min_require"].toString();
        info.author = obj["author"].toString();
        info.timestamp = obj["timestamp"].toInt();
        m_vecPluginInfo.append(info);
    }

    std::sort(m_vecPluginInfo.begin(),m_vecPluginInfo.end(),sortResult);

    return true;
}

EpmPluginInfo* EpmPlugin::getPluginInfo(QString name)
{
    for(int i = 0; i< m_vecPluginInfo.size(); i++)
    {
        EpmPluginInfo& info = m_vecPluginInfo[i];
        QString fileName = QString("%1-%2.plugin").arg(info.name).arg(info.version);
        if (fileName == name)
        {
            return &info;
        }
    }

    return nullptr;
}

void EpmPlugin::installPlugin(QString parameter, QObject* parent)
{
    MainDialog* dlg = qobject_cast<MainDialog*>(parent);

    QUrl url(parameter);
    QString dstPath = QDir::tempPath() + "/" + url.fileName();

    EpmPluginInfo* info = getPluginInfo(url.fileName());
    if (info)
    {
        QString curVersion = EASYGO_VERSION;
        if (CompareVersion(info->min_require, curVersion) > 0)
        {
            QMessageBox::information(dlg, tr("提示"),
                        tr("插件依赖 EasyGo %1 以上的版本\n请更新后再安装").arg(info->min_require));
            return;
        }
    }

    QFuture<bool> fut = QtConcurrent::run(HttpDownload, parameter, dstPath, true);
    while (!fut.isFinished())
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }

    if(!fut.result())
    {
        QMessageBox::information(dlg,tr("提示"),tr("下载安装包失败"));
    }
    else
    {
        dlg->installPlugin(dstPath);
        QFile::remove(dstPath);
    }
}

void EpmPlugin::setProxy(QString proxy, QObject* parent)
{
    QStringList proxyList = proxy.split(':');
    if (proxy.isEmpty())
    {
        GetSettings()->m_proxy_ip.clear();
        GetSettings()->save();
        Ra_ReQuery("",parent);
    }
    else if (proxyList.size() == 1)
    {
        Ra_ShowMsg(tr("请输入正确的代理格式"),parent);
    }
    else
    {
        GetSettings()->m_proxy_ip = proxyList[0];
        GetSettings()->m_proxy_port = proxyList[1].toInt();
        GetSettings()->save();
        Ra_ShowMsg(tr("设置成功"),parent);
    }
}
