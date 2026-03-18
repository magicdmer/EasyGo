#include "Plugin.h"
#include <QLibrary>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include "Settings.h"
#include "LogFile.h"
#include <QClipboard>
#include <QApplication>
#include <QImage>
#include "HelperFunc.h"
#include <QProcess>
#include "TopMostRecord.h"
#include "PluginManager.h"
#include "MainDialog.h"
#include <QTextCodec>
#include <QMessageBox>
#include "PluginManager.h"
#include <QtConcurrent>
#include <QFuture>

QObject* g_fromMainDlg = nullptr;

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
# pragma execution_character_set("utf-8")
#endif

void EASYGO_API Ra_ChangeQuery(const char* strQuery)
{
    QString query = strQuery;
    QMetaObject::invokeMethod(g_fromMainDlg, "setQueryText", Q_ARG(QString,query));
}

void EASYGO_API Ra_Reload()
{
    QMetaObject::invokeMethod(g_fromMainDlg, "sltReload");
}

void EASYGO_API Ra_ReQuery()
{
    QString query;
    QMetaObject::invokeMethod(g_fromMainDlg,
                            "getQueryText",Qt::AutoConnection,Q_RETURN_ARG(QString,query));
    QMetaObject::invokeMethod(g_fromMainDlg, "setQueryText", Q_ARG(QString,query));
}

void EASYGO_API Ra_ShowMsg(const char* title, const char* msg)
{
    QMetaObject::invokeMethod(g_fromMainDlg, "showMsg", Q_ARG(QString,title),Q_ARG(QString,msg));
}

void EASYGO_API Ra_ShowTip(const char* title , const char* msg)
{
    MainDialog* dlg = qobject_cast<MainDialog*>(g_fromMainDlg);
    QMetaObject::invokeMethod(dlg, "showTip", Q_ARG(QString,title),Q_ARG(QString,msg));
}

void EASYGO_API Ra_ShowContent(const char* title, const char* msg)
{
    QMetaObject::invokeMethod(g_fromMainDlg, "showContent", Q_ARG(QString,title),Q_ARG(QString,msg));
}

void EASYGO_API Ra_EditFile(const char* title, const char* filePath)
{
    QMetaObject::invokeMethod(g_fromMainDlg, "editFile", Q_ARG(QString,title),Q_ARG(QString,filePath));
}

void EASYGO_API Ra_PlayMusic(const char* musicPath)
{
    QMetaObject::invokeMethod(g_fromMainDlg, "playMusic", Q_ARG(QString,musicPath));
}

void EASYGO_API Ra_PauseMusic()
{
    QMetaObject::invokeMethod(g_fromMainDlg, "pauseMusic");
}

void EASYGO_API Ra_StopMusic()
{
    QMetaObject::invokeMethod(g_fromMainDlg, "stopMusic");
}

bool Plugin::load()
{
    QString filePath = m_path + QString("/plugin.json");
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return false;
    }
    QString value = file.readAll();
    file.close();

    QJsonParseError parseJsonErr;
    QJsonDocument document = QJsonDocument::fromJson(value.toUtf8(), &parseJsonErr);
    if (parseJsonErr.error != QJsonParseError::NoError)
    {
        return false;
    }

    QJsonObject jsonObject = document.object();

    m_info.id = jsonObject["ID"].toString();
    m_guid = m_info.id;

    m_info.author = jsonObject["Author"].toString();

    if (jsonObject["Keyword"].isArray())
    {
        QJsonArray keysObj = jsonObject["Keyword"].toArray();
        for (int i = 0; i < keysObj.size(); i++)
        {
            QString keyword = keysObj[i].toString();
            m_info.keyword.append(keyword);
        }
    }
    else
    {
        QString keyword = jsonObject["Keyword"].toString();
        m_info.keyword.append(keyword);
    }

    m_info.argc = jsonObject["Argc"].toInt(1);
    m_info.name = jsonObject["Name"].toString();
    m_info.description = jsonObject["Description"].toString();
    m_info.version = jsonObject["Version"].toString();

    m_info.pluginType = jsonObject["PluginType"].toString();
    if (m_info.pluginType == "c++")
    {
        m_type = PLUGIN_CPP;
    }
    else if (m_info.pluginType == "python")
    {
        m_type = PLUGIN_PYTHON;
    }
    else if (m_info.pluginType == "e")
    {
        m_type = PLUGIN_E;
    }
    else if (m_info.pluginType == "powershell")
    {
        m_type = PLUGIN_POWERSHELL;
    }
    else if (m_info.pluginType == "script")
    {
        m_type = PLUGIN_SCRIPT;
    }
    else
    {
        m_type = PLUGIN_UNKOWN;
    }

    m_info.enableSeparate = jsonObject["EnableSeparate"].toInt(1);

    m_info.webSite = jsonObject["Website"].toString();
    m_info.exeName = jsonObject["ExeName"].toString();
    m_info.iconPath = jsonObject["IconPath"].toString();
    if (jsonObject.contains("PluginMode"))
    {
        m_info.pluginMode = jsonObject["PluginMode"].toString();
    }
    else
    {
        m_info.pluginMode = "RealMode";
    }

    if (m_info.pluginMode == "RealMode")
    {
        m_mode = RealMode;
    }
    else if (m_info.pluginMode == "EnterMode")
    {
        m_mode = EnterMode;
    }
    else
    {
        m_mode = RealMode;
    }

    m_iconPath = m_path + QString("/%1").arg(m_info.iconPath);

    if (jsonObject.contains("AcceptType"))
    {
        m_info.acceptType = jsonObject["AcceptType"].toString();
    }

    if (jsonObject.contains("CfgPath"))
    {
        m_info.cfgPath = jsonObject["CfgPath"].toString();
        m_cfgPath = m_path + QString("/%1").arg(m_info.cfgPath);
    }

    if (jsonObject.contains("Interpreter"))
    {
        m_info.interpreter = jsonObject["Interpreter"].toString();
    }

    if (jsonObject.contains("InterpreterArgv"))
    {
        m_info.interpreterArgv = jsonObject["InterpreterArgv"].toString();
    }

    return true;
}

void Plugin::Ra_TopResult(QString parameter,QObject* parent)
{
    Q_UNUSED(parent);

    QJsonParseError json_error;
    QJsonDocument resultDoc = QJsonDocument::fromJson(parameter.toUtf8(),&json_error);
    if (json_error.error != QJsonParseError::NoError)
    {
        return;
    }

    QJsonObject obj = resultDoc.object();
    QString query = obj["Query"].toString();
    QString path = obj["FilePath"].toString();

    GetTopMostRecord()->addOrUpdate(query,path);
    GetTopMostRecord()->save();
}

void Plugin::Ra_DownResult(QString parameter,QObject* parent)
{
    Q_UNUSED(parent);

    QJsonParseError json_error;
    QJsonDocument resultDoc = QJsonDocument::fromJson(parameter.toUtf8(),&json_error);
    if (json_error.error != QJsonParseError::NoError)
    {
        return;
    }

    QJsonObject obj = resultDoc.object();
    QString query = obj["Query"].toString();
    QString path = obj["FilePath"].toString();

    GetTopMostRecord()->remove(query);
    GetTopMostRecord()->save();
}

void Plugin::Ra_ActivePlugin(QString parameter,QObject* parent)
{
    MainDialog* dlg = qobject_cast<MainDialog*>(parent);
    dlg->setQueryText(parameter);
}

void Plugin::Ra_ShowMsg(QString parameter,QObject* parent)
{
    MainDialog* dlg = qobject_cast<MainDialog*>(parent);

    QString title;
    QString msg;

    QJsonParseError json_error;
    QJsonDocument resultDoc = QJsonDocument::fromJson(parameter.toUtf8(),&json_error);
    if (json_error.error != QJsonParseError::NoError)
    {
        title = tr("提示 - %1").arg(m_info.name);
        msg = parameter;
    }
    else
    {
        QJsonObject obj = resultDoc.object();
        title = obj["Title"].toString();
        title = tr("%1 - %2").arg(title).arg(m_info.name);
        msg = obj["Msg"].toString();
    }

    dlg->showMsg(title, msg);
}

void Plugin::Ra_ShowTip(QString parameter,QObject* parent)
{
    MainDialog* dlg = qobject_cast<MainDialog*>(parent);

    QString title;
    QString msg;

    QJsonParseError json_error;
    QJsonDocument resultDoc = QJsonDocument::fromJson(parameter.toUtf8(),&json_error);
    if (json_error.error != QJsonParseError::NoError)
    {
        title = tr("提示 - %1").arg(m_info.name);
        msg = parameter;
    }
    else
    {
        QJsonObject obj = resultDoc.object();
        title = obj["Title"].toString();
        title = tr("%1 - %2").arg(title).arg(m_info.name);
        msg = obj["Msg"].toString();
    }

    dlg->showTip(title, msg);
}

void Plugin::Ra_ChangeQuery(QString parameter, QObject *parent)
{
    MainDialog* dlg = qobject_cast<MainDialog*>(parent);
    dlg->setQueryText(parameter);
}

void Plugin::Ra_ChangeQueryPara(QString parameter, QObject *parent)
{
    MainDialog* dlg = qobject_cast<MainDialog*>(parent);

    PluginConfig cfg = GetSettings()->getPluginCfg(m_info.id);
    QString queryText = QString("%1 %2").arg(cfg.keyword[0]).arg(parameter);
    dlg->setQueryText(queryText);
}

void Plugin::Ra_Reload(QString parameter, QObject *parent)
{
    Q_UNUSED(parameter);

    MainDialog* dlg = qobject_cast<MainDialog*>(parent);
    dlg->sltReload();
}

void Plugin::Ra_ReQuery(QString parameter, QObject *parent)
{
    Q_UNUSED(parameter);

    MainDialog* dlg = qobject_cast<MainDialog*>(parent);

    QString query = dlg->getQueryText();
    dlg->setQueryText(query);
}

void Plugin::Ra_ShowContent(QString parameter,QObject* parent)
{
    MainDialog* dlg = qobject_cast<MainDialog*>(parent);

    QString title;
    QString msg;

    QJsonParseError json_error;
    QJsonDocument resultDoc = QJsonDocument::fromJson(parameter.toUtf8(),&json_error);
    if (json_error.error != QJsonParseError::NoError)
    {
        title = tr("提示 - %1").arg(m_info.name);
        msg = parameter;
    }
    else
    {
        QJsonObject obj = resultDoc.object();
        title = obj["Title"].toString();
        title = tr("%1 - %2").arg(title).arg(m_info.name);
        msg = obj["Msg"].toString();
    }

    dlg->showContent(title, msg);
}

void Plugin::Ra_Copy(QString data,QObject* parent)
{
    Q_UNUSED(parent);

    CopyFileToClipboard(data);
}

void Plugin::Ra_CopyPath(QString data,QObject* parent)
{
    Q_UNUSED(parent);

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(data);
}

void Plugin::Ra_CopyImage(QString data,QObject* parent)
{
    Q_UNUSED(parent);

    QClipboard *clipboard = QApplication::clipboard();
    QImage img(data);
    clipboard->setImage(img);
}

void Plugin::Ra_Delete(QString data,QObject* parent)
{
    Q_UNUSED(parent);

    QFileInfo info(data);
    if (info.isDir())
    {
        QDir dir(data);
        dir.removeRecursively();
    }
    else
    {
        QFile::remove(data);
    }
}

void Plugin::Ra_Recycle(QString data,QObject* parent)
{
    Q_UNUSED(parent);

#if (QT_VERSION > QT_VERSION_CHECK(5,15,0))
    QFile::moveToTrash(data);
#else
    MvoeFileToRecyclebin(data);
#endif
}

void Plugin::Ra_OpenFileFolder(QString data,QObject* parent)
{
    Q_UNUSED(parent);

    //OpenFileFoler(data);

    QProcess process;
    QString filePath = QDir::toNativeSeparators(data);
    QString cmdLine = QString("explorer /select,%1").arg(filePath);
    process.startDetached(cmdLine);

}

void Plugin::Ra_Open(QString data,QObject* parent)
{
	Q_UNUSED(parent);

	QString exePath,para;
	exePath = data;

	int num = data.count("\"");
	if (num == 2)
	{
		int index = data.lastIndexOf("\"");
		para = data.mid(index+2);
		exePath = data.mid(0,index+1);
		exePath = exePath.remove("\"");
	}

    QFileInfo fileInfo(exePath);

    if (fileInfo.isExecutable() &&
        (0 == fileInfo.suffix().compare("exe",Qt::CaseInsensitive) ||
        0 == fileInfo.suffix().compare("bat",Qt::CaseInsensitive)))
	{
        QTextCodec* gbk = QTextCodec::codecForName("gbk");
        QByteArray gbkExePath = gbk->fromUnicode(exePath);
        QByteArray gbkPara = gbk->fromUnicode(para);
        QByteArray gbkWorkDir = gbk->fromUnicode(fileInfo.path());

        if (gbkPara.isEmpty())
        {
            ShellExecuteA(0,"open",gbkExePath.data(),0,gbkWorkDir.data(),SW_SHOWNORMAL);
        }
        else
        {
            ShellExecuteA(0,"open",gbkExePath.data(),gbkPara.data(),gbkWorkDir.data(),SW_SHOWNORMAL);
        }
	}
    else if (fileInfo.isDir())
    {
        QTextCodec* gbk = QTextCodec::codecForName("gbk");
        QByteArray gbkFolderPath = gbk->fromUnicode(exePath);
        ShellExecuteA(0,"open",gbkFolderPath.data(),0,0,SW_SHOWNORMAL);
    }
	else
	{
        //lnk快捷方式自带workdir，然后用ShellExecuteA可能要等会才能打开，开进程就能造成秒开效果，所以用explorer
		QProcess process;
		QString cmdLine;
		cmdLine = QString("explorer %1").arg(data);
		cmdLine = cmdLine.replace("/","\\");
		process.startDetached(cmdLine);
	}

}

void Plugin::Ra_OpenWeb(QString data,QObject* parent)
{
    Q_UNUSED(parent);

    QTextCodec* gbk = QTextCodec::codecForName("gbk");
    QByteArray gbkByteArray = gbk->fromUnicode(data);

    ShellExecuteA(0,"open",gbkByteArray.data(),0,0,SW_SHOWNORMAL);
}

void Plugin::Ra_EditFile(QString parameter, QObject* parent)
{
    MainDialog* dlg = qobject_cast<MainDialog*>(parent);

    QString title;
    QString filePath;

    QJsonParseError json_error;
    QJsonDocument resultDoc = QJsonDocument::fromJson(parameter.toUtf8(),&json_error);
    if (json_error.error != QJsonParseError::NoError)
    {
        filePath = parameter;

        QFileInfo info(filePath);

        title = tr("%1 - %2").arg(info.baseName()).arg(m_info.name);
    }
    else
    {
        QJsonObject obj = resultDoc.object();
        title = obj["Title"].toString();
        title = tr("%1 - %2").arg(title).arg(m_info.name);
        filePath = obj["FilePath"].toString();
    }

    dlg->editFile(title, filePath);
}

void Plugin::Ra_Install(QString parameter, QObject* parent)
{
    MainDialog* dlg = qobject_cast<MainDialog*>(parent);

    QUrl url(parameter);
    QString dstPath = QDir::tempPath() + "/" + url.fileName();
    if(!HttpDownload(parameter,dstPath,true))
    {
        QMessageBox::information(dlg,tr("提示"),tr("下载安装包失败"));
    }
    else
    {
        dlg->installPlugin(dstPath);
        QFile::remove(dstPath);
    }
}

void Plugin::Ra_Uninstall(QString parameter, QObject* parent)
{
    MainDialog* dlg = qobject_cast<MainDialog*>(parent);

    if (GetPluginMananger()->deletePlugin(parameter))
    {
        QMessageBox::information(dlg,tr("提示"),tr("卸载成功"));
    }
    else
    {
        QMessageBox::information(dlg,tr("提示"),tr("卸载失败"));
    }
}

void Plugin::Ra_PlayMusic(QString musicPath, QObject* parent)
{
    MainDialog* dlg = qobject_cast<MainDialog*>(parent);
    dlg->playMusic(musicPath);
}

void Plugin::Ra_PauseMusic(QString parameter,QObject* parent)
{
    Q_UNUSED(parameter);

    MainDialog* dlg = qobject_cast<MainDialog*>(parent);
    dlg->pauseMusic();
}

void Plugin::Ra_StopMusic(QString parameter,QObject* parent)
{
    Q_UNUSED(parameter);

    MainDialog* dlg = qobject_cast<MainDialog*>(parent);
    dlg->stopMusic();
}

bool CPlusPlugin::initPlugin(QString pluginPath)
{
    QString exePath = m_path + QString("/%1").arg(m_info.exeName);

    int index = exePath.lastIndexOf(".");
    QString libraryName = exePath.mid(0,index);

    m_library.setFileName(libraryName);
    if (!m_library.load())
    {
        return false;
    }

    pInitFunc init = (pInitFunc)m_library.resolve("InitPlugin");
    if (!init)
    {
        return false;
    }

    m_query = (pQueryFunc)m_library.resolve("Query");
    if (!m_query)
    {
        return false;
    }

    m_menuFunc = (pMenuFunc)m_library.resolve("GetContextMenu");
    if (!m_menuFunc)
    {
        return false;
    }

    m_updateSetting = (pUpdateSetting)m_library.resolve("UpdateSetting");

    pluginPath = QDir::toNativeSeparators(pluginPath);
    QByteArray ba = pluginPath.toLocal8Bit();
    if (!init(ba.data()))
    {
        return false;
    }

    return true;
}

bool CPlusPlugin::query(Query query,QVector<Result>& vecResult)
{
    Settings* cfg = GetSettings();

    QJsonObject json;
    json.insert("RawQuery", query.rawQuery);
    json.insert("Keyword", query.keyword);
    json.insert("Parameter", query.parameter);

    QJsonDocument document;
    document.setObject(json);
    QByteArray byte_array = document.toJson(QJsonDocument::Compact);
    QString strUtf8 = QString::fromUtf8(byte_array);
    QTextCodec* gbk = QTextCodec::codecForName("gbk");
    QByteArray gbkByteArray = gbk->fromUnicode(strUtf8);
    char* jsonStr = gbkByteArray.data();

    int length = 0;

    if (!m_query(jsonStr,NULL,&length))
    {
        return false;
    }

    char* jsonResult = new char[length + 10];
    if (!jsonResult)
    {
        return false;
    }

    if (!m_query(jsonStr,jsonResult,&length))
    {
        delete[] jsonResult;
        return false;
    }

    QJsonParseError json_error;
    QString strJson = QString::fromLocal8Bit(jsonResult);
    delete[] jsonResult;

    QJsonDocument resultDoc = QJsonDocument::fromJson(strJson.toUtf8(),&json_error);
    if (json_error.error != QJsonParseError::NoError)
    {
        return false;
    }

    QJsonObject obj = resultDoc.object();
    QJsonArray resultsObj = obj["Results"].toArray();

    int maxCout = resultsObj.size() > cfg->m_maxResultsToShow ? cfg->m_maxResultsToShow:resultsObj.size();
    for (int i = 0; i < maxCout;i++)
    {
        QJsonObject resultObj = resultsObj[i].toObject();

        Result result;
        result.id = m_guid;
        if (resultObj.contains("ShowType"))
        {
            result.showType = resultObj["ShowType"].toInt();
        }
        else
        {
            result.showType = SHOW_TYPE_DEFAULT;
        }
        result.title = resultObj["Title"].toString();
        result.subTitle = resultObj["SubTitle"].toString();
        QString iconPath = resultObj["IconPath"].toString();
        if (iconPath.isEmpty())
        {
            result.iconPath = m_path + QString("/%1").arg(m_info.iconPath);
        }
        else
        {
            if (iconPath.startsWith("Ra_") || QDir::isAbsolutePath(iconPath))
            {
                result.iconPath = iconPath;
            }
            else
            {
                result.iconPath = m_path + QString("/%1").arg(iconPath);
            }
        }

        if (resultObj.contains("ExtraData"))
        {
            result.extraData = resultObj["ExtraData"].toString();
        }

        if (resultObj.contains("Action"))
        {
            QJsonObject actionObj = resultObj["Action"].toObject();
            result.action.funcName = actionObj["FuncName"].toString();

            if (actionObj.contains("Parameter"))
            {
               result.action.parameter = actionObj["Parameter"].toString();
            }

            if (actionObj.contains("HideWindow"))
            {
                result.action.hideWindow = actionObj["HideWindow"].toBool();
            }
        }

        vecResult.append(result);
    }

    return true;
}

bool CPlusPlugin::getMenu(Result& result ,QVector<Result>& vecMenu)
{
    QJsonObject json;
    json.insert("ID",result.id);
    json.insert("Title",result.title);
    json.insert("SubTitle", result.subTitle);
    json.insert("ExtraData", result.extraData);

    QJsonDocument document;
    document.setObject(json);
    QByteArray byte_array = document.toJson(QJsonDocument::Compact);
    QString strUtf8 = QString::fromUtf8(byte_array);
    QTextCodec* gbk = QTextCodec::codecForName("gbk");
    QByteArray gbkByteArray = gbk->fromUnicode(strUtf8);
    char* jsonStr = gbkByteArray.data();

    int length = 0;

    if (!m_menuFunc(jsonStr,NULL,&length))
    {
        return false;
    }

    char* retJson = new char[length + 10];
    if (!retJson)
    {
        return false;
    }

    if (!m_menuFunc(jsonStr,retJson,&length))
    {
        delete[] retJson;
        return false;
    }

    QJsonParseError json_error;
    QString strJson = QString::fromLocal8Bit(retJson);
    delete[] retJson;

    QJsonDocument resultDocument = QJsonDocument::fromJson(strJson.toUtf8(),&json_error);
    if (json_error.error != QJsonParseError::NoError)
    {
        return false;
    }

    QJsonObject obj = resultDocument.object();
    QJsonArray menuArray = obj["Results"].toArray();

    for (int i = 0; i< menuArray.size();i++)
    {
        QJsonObject menuObj = menuArray[i].toObject();

        Result menu;
        menu.id = m_guid;
        menu.title = menuObj["Title"].toString();
        menu.subTitle = menuObj["SubTitle"].toString();
        QString iconPath = menuObj["IconPath"].toString();
        if (iconPath.isEmpty())
        {
            menu.iconPath = m_path + QString("/%1").arg(m_info.iconPath);
        }
        else
        {
            if (iconPath.startsWith("Ra_") || QDir::isAbsolutePath(iconPath))
            {
                menu.iconPath = iconPath;
            }
            else
            {
                menu.iconPath = m_path + QString("/%1").arg(iconPath);
            }
        }

        if (menuObj.contains("Action"))
        {
            QJsonObject actionObj = menuObj["Action"].toObject();
            menu.action.funcName = actionObj["FuncName"].toString();

            if (actionObj.contains("Parameter"))
            {
               menu.action.parameter = actionObj["Parameter"].toString();
            }

            if (actionObj.contains("HideWindow"))
            {
                menu.action.hideWindow = actionObj["HideWindow"].toBool();
            }
        }

        vecMenu.append(menu);
    }

    return true;
}

void CPlusPlugin::updateSetting()
{
    if (!m_updateSetting) return;
    m_updateSetting();
}

void CPlusPlugin::itemClick(Result& item,QObject* parent)
{
    PluginAction& action = item.action;

    if (action.funcName.startsWith("Ra_"))
    {
        PVOID OldValue;
        Wow64DisableWow64FsRedirection (&OldValue);

        QByteArray ba = action.funcName.toUtf8();
        QMetaObject::invokeMethod(this,ba.data(),Q_ARG(QString,action.parameter),Q_ARG(QObject*,parent));

        Wow64RevertWow64FsRedirection (OldValue);
    }
    else
    {
        QByteArray ba = action.funcName.toLocal8Bit();
        char* funcName = ba.data();
        pClickFunc func = (pClickFunc)m_library.resolve(funcName);
        if (!func)
        {
            return;
        }

        ba = action.parameter.toLocal8Bit();
        char* pPara = ba.data();

        QFuture<void> fut = QtConcurrent::run(func, pPara);
        while (!fut.isFinished())
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        }
    }
}

PythonPlugin::PythonPlugin(QString &pluginPath):Plugin(pluginPath)
{
    QString programPath = QDir::currentPath();
    m_jsonRpcPath = programPath + QString("/JsonRPC");
    m_jsonRpcPath = m_jsonRpcPath.replace("/","\\");

    if (GetSettings()->m_pythonPath.isEmpty())
    {
        m_pythonPath = "pythonw.exe";
    }
    else
    {
        m_pythonPath = GetSettings()->m_pythonPath + QString("/pythonw.exe");
    }
}

bool PythonPlugin::initPlugin(QString pluginPath)
{
    QString output = execute("InitPlugin",pluginPath);

    QJsonParseError json_error;
    QJsonDocument resultDocument = QJsonDocument::fromJson(output.toUtf8(),&json_error);
    if (json_error.error != QJsonParseError::NoError)
    {
        return false;
    }

    QJsonObject obj = resultDocument.object();
    bool result = obj["Result"].toBool();
    if (!result)
    {
        return false;
    }

    return true;
}

bool PythonPlugin::query(Query query,QVector<Result>& vecResult)
{
    Settings* cfg = GetSettings();

    QString output = execute("Query",query.parameter);
    if (output.isEmpty())
    {
        return false;
    }

    QJsonParseError json_error;
    QJsonDocument resultDoc = QJsonDocument::fromJson(output.toUtf8(),&json_error);
    if (json_error.error != QJsonParseError::NoError)
    {
        qLog(QString("[%1] parse json failed, error:%2").
             arg(m_info.name).arg(json_error.errorString()));
        return false;
    }

    QJsonObject obj = resultDoc.object();
    QJsonArray resultsObj = obj["Results"].toArray();

    int maxCout = resultsObj.size() > cfg->m_maxResultsToShow ? cfg->m_maxResultsToShow:resultsObj.size();
    for (int i = 0; i < maxCout;i++)
    {
        QJsonObject resultObj = resultsObj[i].toObject();

        Result result;
        result.id = m_info.id;
        if (resultObj.contains("ShowType"))
        {
            result.showType = resultObj["ShowType"].toInt();
        }
        else
        {
            result.showType = SHOW_TYPE_DEFAULT;
        }
        result.title = resultObj["Title"].toString();
        result.subTitle = resultObj["SubTitle"].toString();
        QString iconPath = resultObj["IconPath"].toString();
        if (iconPath.isEmpty())
        {
            result.iconPath = m_path + QString("/%1").arg(m_info.iconPath);
        }
        else
        {
            if (iconPath.startsWith("Ra_") || QDir::isAbsolutePath(iconPath))
            {
                result.iconPath = iconPath;
            }
            else
            {
                result.iconPath = m_path + QString("/%1").arg(iconPath);
            }
        }

        if (resultObj.contains("ExtraData"))
        {
            result.extraData = resultObj["ExtraData"].toString();
        }

        if (resultObj.contains("Action"))
        {
            QJsonObject actionObj = resultObj["Action"].toObject();
            result.action.funcName = actionObj["FuncName"].toString();

            if (actionObj.contains("Parameter"))
            {
               result.action.parameter = actionObj["Parameter"].toString();
            }

            if (actionObj.contains("HideWindow"))
            {
                result.action.hideWindow = actionObj["HideWindow"].toBool();
            }
        }

        vecResult.append(result);
    }

    return true;
}

bool PythonPlugin::getMenu(Result& result,QVector<Result>& vecMenu)
{
    QJsonObject oParameter;
    oParameter.insert("ID",result.id);
    oParameter.insert("Title",result.title);
    oParameter.insert("SubTitle", result.subTitle);
    oParameter.insert("ExtraData", result.extraData);

    QJsonDocument document;
    document.setObject(oParameter);
    QByteArray byte_array = document.toJson(QJsonDocument::Compact);

    QString output = execute("GetContextMenu",byte_array);
    if (output.isEmpty())
    {
        qLog(QString("[%1] getMenu error,result empty").arg(m_info.name));
        return false;
    }

    QJsonParseError json_error;
    QJsonDocument resultDocument = QJsonDocument::fromJson(output.toUtf8(),&json_error);
    if (json_error.error != QJsonParseError::NoError)
    {
        return false;
    }

    QJsonObject obj = resultDocument.object();
    QJsonArray menuArray = obj["Results"].toArray();

    for (int i = 0; i< menuArray.size();i++)
    {
        QJsonObject menuObj = menuArray[i].toObject();

        Result menu;
        menu.id = m_info.id;
        menu.title = menuObj["Title"].toString();
        menu.subTitle = menuObj["SubTitle"].toString();
        QString iconPath = menuObj["IconPath"].toString();
        if (iconPath.isEmpty())
        {
            menu.iconPath = m_path + QString("/%1").arg(m_info.iconPath);
        }
        else
        {
            if (iconPath.startsWith("Ra_") || QDir::isAbsolutePath(iconPath))
            {
                menu.iconPath = iconPath;
            }
            else
            {
                menu.iconPath = m_path + QString("/%1").arg(iconPath);
            }
        }

        if (menuObj.contains("Action"))
        {
            QJsonObject actionObj = menuObj["Action"].toObject();
            menu.action.funcName = actionObj["FuncName"].toString();

            if (actionObj.contains("Parameter"))
            {
                menu.action.parameter = actionObj["Parameter"].toString();
            }

            if (actionObj.contains("HideWindow"))
            {
                menu.action.hideWindow = actionObj["HideWindow"].toBool();
            }
        }

        vecMenu.append(menu);
    }

    return true;
}

void PythonPlugin::itemClick(Result &item,QObject* parent)
{
    PluginAction& action = item.action;

    if (action.funcName.startsWith("Ra_"))
    {
        PVOID OldValue;
        Wow64DisableWow64FsRedirection (&OldValue);

        QByteArray ba = action.funcName.toUtf8();
        QMetaObject::invokeMethod(this,ba.data(),Q_ARG(QString,action.parameter),Q_ARG(QObject*,parent));

        Wow64RevertWow64FsRedirection (OldValue);

    }
    else
    {
        QString output = execute(action.funcName,action.parameter);
        if (!output.isEmpty())
        {
            QStringList funcList = output.split("\r\n",QString::SkipEmptyParts);

            foreach (QString func,funcList)
            {
                QJsonParseError json_error;
                QJsonDocument resultDocument = QJsonDocument::fromJson(func.toUtf8(),&json_error);
                if (json_error.error != QJsonParseError::NoError)
                {
                    return;
                }

                QJsonObject obj = resultDocument.object();
                QString funcName = obj["FuncName"].toString();
                QString parameter = obj["Parameter"].toString();

                QByteArray ba = funcName.toUtf8();
                QMetaObject::invokeMethod(this,ba.data(),Q_ARG(QString,parameter),Q_ARG(QObject*,parent));

            }
        }
    }
}

QString PythonPlugin::execute(QString funcName,QString parameter)
{
    QProcess process;

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("PYTHONPATH", m_jsonRpcPath);
    process.setProcessEnvironment(env);
    process.setProgram(m_pythonPath);
    process.setWorkingDirectory(m_path);

    QJsonObject json;
    json.insert("FuncName", funcName);
    json.insert("Parameter", parameter);

    QJsonDocument document;
    document.setObject(json);
    QByteArray byte_array = document.toJson(QJsonDocument::Compact);

    QString modPath = m_path + QString("/%1").arg(m_info.exeName);

    QStringList args;
    args.append("-B");
    args.append(modPath);
    args.append(byte_array);
    process.setArguments(args);

    process.start();
    process.waitForFinished();

    QByteArray output = process.readAllStandardOutput();
    if (output.isEmpty())
    {
        QString strError = process.readAllStandardError();
        qLog(QString("[%1] query error : %2").arg(m_info.name).arg(strError));
    }

    return output;
}

bool EPlugin::initPlugin(QString pluginPath)
{
    QString exePath = m_path + QString("/%1").arg(m_info.exeName);

    int index = exePath.lastIndexOf(".");
    QString libraryName = exePath.mid(0,index);

    m_library.setFileName(libraryName);
    if (!m_library.load())
    {
        return false;
    }

    pEInitFunc init = (pEInitFunc)m_library.resolve("InitPlugin");
    if (!init)
    {
        return false;
    }

    m_query = (pEQueryFunc)m_library.resolve("Query");
    if (!m_query)
    {
        return false;
    }

    m_menuFunc = (pEMenuFunc)m_library.resolve("GetContextMenu");
    if (!m_menuFunc)
    {
        return false;
    }

    m_updateSetting = (pEUpdateSetting)m_library.resolve("UpdateSetting");

    pluginPath = QDir::toNativeSeparators(pluginPath);
    QByteArray ba = pluginPath.toLocal8Bit();
    if (!init(ba.data()))
    {
        return false;
    }

    return true;
}

bool EPlugin::query(Query query,QVector<Result>& vecResult)
{
    Settings* cfg = GetSettings();

    QJsonObject json;
    json.insert("RawQuery", query.rawQuery);
    json.insert("Keyword", query.keyword);
    json.insert("Parameter", query.parameter);

    QJsonDocument document;
    document.setObject(json);
    QByteArray byte_array = document.toJson(QJsonDocument::Compact);
    QString strUtf8 = QString::fromUtf8(byte_array);
    QTextCodec* gbk = QTextCodec::codecForName("gbk");
    QByteArray gbkByteArray = gbk->fromUnicode(strUtf8);
    char* jsonStr = gbkByteArray.data();

    const char* jsonResult = m_query(jsonStr);
    if(jsonResult[0] == '\0')
    {
        return false;
    }

    QJsonParseError json_error;
    QString strJson = QString::fromLocal8Bit(jsonResult);

    QJsonDocument resultDoc = QJsonDocument::fromJson(strJson.toUtf8(),&json_error);
    if (json_error.error != QJsonParseError::NoError)
    {
        return false;
    }

    QJsonObject obj = resultDoc.object();
    QJsonArray resultsObj = obj["Results"].toArray();

    int maxCout = resultsObj.size() > cfg->m_maxResultsToShow ? cfg->m_maxResultsToShow:resultsObj.size();
    for (int i = 0; i < maxCout;i++)
    {
        QJsonObject resultObj = resultsObj[i].toObject();

        Result result;
        result.id = m_guid;
        if (resultObj.contains("ShowType"))
        {
            result.showType = resultObj["ShowType"].toInt();
        }
        else
        {
            result.showType = SHOW_TYPE_DEFAULT;
        }
        result.title = resultObj["Title"].toString();
        result.subTitle = resultObj["SubTitle"].toString();
        QString iconPath = resultObj["IconPath"].toString();
        if (iconPath.isEmpty())
        {
            result.iconPath = m_path + QString("/%1").arg(m_info.iconPath);
        }
        else
        {
            if (iconPath.startsWith("Ra_") || QDir::isAbsolutePath(iconPath))
            {
                result.iconPath = iconPath;
            }
            else
            {
                result.iconPath = m_path + QString("/%1").arg(iconPath);
            }
        }

        if (resultObj.contains("ExtraData"))
        {
            result.extraData = resultObj["ExtraData"].toString();
        }

        if (resultObj.contains("Action"))
        {
            QJsonObject actionObj = resultObj["Action"].toObject();
            result.action.funcName = actionObj["FuncName"].toString();

            if (actionObj.contains("Parameter"))
            {
               result.action.parameter = actionObj["Parameter"].toString();
            }

            if (actionObj.contains("HideWindow"))
            {
                result.action.hideWindow = actionObj["HideWindow"].toBool();
            }
        }

        vecResult.append(result);
    }

    return true;
}

bool EPlugin::getMenu(Result& result ,QVector<Result>& vecMenu)
{
    QJsonObject json;
    json.insert("ID",result.id);
    json.insert("Title",result.title);
    json.insert("SubTitle", result.subTitle);
    json.insert("ExtraData", result.extraData);

    QJsonDocument document;
    document.setObject(json);
    QByteArray byte_array = document.toJson(QJsonDocument::Compact);
    QString strUtf8 = QString::fromUtf8(byte_array);
    QTextCodec* gbk = QTextCodec::codecForName("gbk");
    QByteArray gbkByteArray = gbk->fromUnicode(strUtf8);
    char* jsonStr = gbkByteArray.data();

    const char* retJson = m_menuFunc(jsonStr);
    if (retJson[0] == '\0')
    {
        return false;
    }

    QJsonParseError json_error;
    QString strJson = QString::fromLocal8Bit(retJson);

    QJsonDocument resultDocument = QJsonDocument::fromJson(strJson.toUtf8(),&json_error);
    if (json_error.error != QJsonParseError::NoError)
    {
        return false;
    }

    QJsonObject obj = resultDocument.object();
    QJsonArray menuArray = obj["Results"].toArray();

    for (int i = 0; i< menuArray.size();i++)
    {
        QJsonObject menuObj = menuArray[i].toObject();

        Result menu;
        menu.id = m_guid;
        menu.title = menuObj["Title"].toString();
        menu.subTitle = menuObj["SubTitle"].toString();
        QString iconPath = menuObj["IconPath"].toString();
        if (iconPath.isEmpty())
        {
            menu.iconPath = m_path + QString("/%1").arg(m_info.iconPath);
        }
        else
        {
            if (iconPath.startsWith("Ra_") || QDir::isAbsolutePath(iconPath))
            {
                menu.iconPath = iconPath;
            }
            else
            {
                menu.iconPath = m_path + QString("/%1").arg(iconPath);
            }
        }

        if (menuObj.contains("Action"))
        {
            QJsonObject actionObj = menuObj["Action"].toObject();
            menu.action.funcName = actionObj["FuncName"].toString();

            if (actionObj.contains("Parameter"))
            {
               menu.action.parameter = actionObj["Parameter"].toString();
            }

            if (actionObj.contains("HideWindow"))
            {
                menu.action.hideWindow = actionObj["HideWindow"].toBool();
            }
        }

        vecMenu.append(menu);
    }

    return true;
}

void EPlugin::updateSetting()
{
    if (!m_updateSetting) return;
    m_updateSetting();
}

void EPlugin::itemClick(Result& item,QObject* parent)
{
    PluginAction& action = item.action;

    if (action.funcName.startsWith("Ra_"))
    {
        PVOID OldValue;
        Wow64DisableWow64FsRedirection (&OldValue);

        QByteArray ba = action.funcName.toUtf8();
        QMetaObject::invokeMethod(this,ba.data(),Q_ARG(QString,action.parameter),Q_ARG(QObject*,parent));

        Wow64RevertWow64FsRedirection (OldValue);
    }
    else
    {
        QByteArray ba = action.funcName.toLocal8Bit();
        char* funcName = ba.data();
        pEClickFunc func = (pEClickFunc)m_library.resolve(funcName);
        if (!func)
        {
            return;
        }

        ba = action.parameter.toLocal8Bit();
        char* pPara = ba.data();

        QFuture<void> fut = QtConcurrent::run(func, pPara);
        while (!fut.isFinished())
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        }
    }
}

PowerShellPlugin::PowerShellPlugin(QString &pluginPath):Plugin(pluginPath)
{

}

bool PowerShellPlugin::initPlugin(QString pluginPath)
{
    QString output = execute("InitPlugin",pluginPath);

    QJsonParseError json_error;
    QJsonDocument resultDocument = QJsonDocument::fromJson(output.toUtf8(),&json_error);
    if (json_error.error != QJsonParseError::NoError)
    {
        return false;
    }

    QJsonObject obj = resultDocument.object();
    bool result = obj["Result"].toBool();
    if (!result)
    {
        return false;
    }

    return true;
}

bool PowerShellPlugin::query(Query query,QVector<Result>& vecResult)
{
    Settings* cfg = GetSettings();

    QString output = execute("Query",query.parameter);
    if (output.isEmpty())
    {
        return false;
    }

    QJsonParseError json_error;
    QJsonDocument resultDoc = QJsonDocument::fromJson(output.toUtf8(),&json_error);
    if (json_error.error != QJsonParseError::NoError)
    {
        qLog(QString("[%1] parse json failed, error:%2").
             arg(m_info.name).arg(json_error.errorString()));
        return false;
    }

    QJsonObject obj = resultDoc.object();
    QJsonArray resultsObj = obj["Results"].toArray();

    int maxCout = resultsObj.size() > cfg->m_maxResultsToShow ? cfg->m_maxResultsToShow:resultsObj.size();
    for (int i = 0; i < maxCout;i++)
    {
        QJsonObject resultObj = resultsObj[i].toObject();

        Result result;
        result.id = m_info.id;
        if (resultObj.contains("ShowType"))
        {
            result.showType = resultObj["ShowType"].toInt();
        }
        else
        {
            result.showType = SHOW_TYPE_DEFAULT;
        }
        result.title = resultObj["Title"].toString();
        result.subTitle = resultObj["SubTitle"].toString();
        QString iconPath = resultObj["IconPath"].toString();
        if (iconPath.isEmpty())
        {
            result.iconPath = m_path + QString("/%1").arg(m_info.iconPath);
        }
        else
        {
            if (iconPath.startsWith("Ra_") || QDir::isAbsolutePath(iconPath))
            {
                result.iconPath = iconPath;
            }
            else
            {
                result.iconPath = m_path + QString("/%1").arg(iconPath);
            }
        }

        if (resultObj.contains("ExtraData"))
        {
            result.extraData = resultObj["ExtraData"].toString();
        }

        if (resultObj.contains("Action"))
        {
            QJsonObject actionObj = resultObj["Action"].toObject();
            result.action.funcName = actionObj["FuncName"].toString();

            if (actionObj.contains("Parameter"))
            {
               result.action.parameter = actionObj["Parameter"].toString();
            }

            if (actionObj.contains("HideWindow"))
            {
                result.action.hideWindow = actionObj["HideWindow"].toBool();
            }
        }

        vecResult.append(result);
    }

    return true;
}

bool PowerShellPlugin::getMenu(Result& result,QVector<Result>& vecMenu)
{
    QJsonObject oParameter;
    oParameter.insert("ID",result.id);
    oParameter.insert("Title",result.title);
    oParameter.insert("SubTitle", result.subTitle);
    oParameter.insert("ExtraData", result.extraData);

    QJsonDocument document;
    document.setObject(oParameter);
    QByteArray byte_array = document.toJson(QJsonDocument::Compact);

    QString output = execute("GetContextMenu",byte_array);
    if (output.isEmpty())
    {
        qLog(QString("[%1] getMenu error,result empty").arg(m_info.name));
        return false;
    }

    QJsonParseError json_error;
    QJsonDocument resultDocument = QJsonDocument::fromJson(output.toUtf8(),&json_error);
    if (json_error.error != QJsonParseError::NoError)
    {
        return false;
    }

    QJsonObject obj = resultDocument.object();
    QJsonArray menuArray = obj["Results"].toArray();

    for (int i = 0; i< menuArray.size();i++)
    {
        QJsonObject menuObj = menuArray[i].toObject();

        Result menu;
        menu.id = m_info.id;
        menu.title = menuObj["Title"].toString();
        menu.subTitle = menuObj["SubTitle"].toString();
        QString iconPath = menuObj["IconPath"].toString();
        if (iconPath.isEmpty())
        {
            menu.iconPath = m_path + QString("/%1").arg(m_info.iconPath);
        }
        else
        {
            if (iconPath.startsWith("Ra_") || QDir::isAbsolutePath(iconPath))
            {
                menu.iconPath = iconPath;
            }
            else
            {
                menu.iconPath = m_path + QString("/%1").arg(iconPath);
            }
        }

        if (menuObj.contains("Action"))
        {
            QJsonObject actionObj = menuObj["Action"].toObject();
            menu.action.funcName = actionObj["FuncName"].toString();

            if (actionObj.contains("Parameter"))
            {
                menu.action.parameter = actionObj["Parameter"].toString();
            }

            if (actionObj.contains("HideWindow"))
            {
                menu.action.hideWindow = actionObj["HideWindow"].toBool();
            }
        }

        vecMenu.append(menu);
    }

    return true;
}

void PowerShellPlugin::itemClick(Result &item,QObject* parent)
{
    PluginAction& action = item.action;

    if (action.funcName.startsWith("Ra_"))
    {
        PVOID OldValue;
        Wow64DisableWow64FsRedirection (&OldValue);

        QByteArray ba = action.funcName.toUtf8();
        QMetaObject::invokeMethod(this,ba.data(),Q_ARG(QString,action.parameter),Q_ARG(QObject*,parent));

        Wow64RevertWow64FsRedirection (OldValue);

    }
    else
    {
        QString output = execute(action.funcName,action.parameter);
        if (!output.isEmpty())
        {
            QStringList funcList = output.split("\r\n",QString::SkipEmptyParts);

            foreach (QString func,funcList)
            {
                QJsonParseError json_error;
                QJsonDocument resultDocument = QJsonDocument::fromJson(func.toUtf8(),&json_error);
                if (json_error.error != QJsonParseError::NoError)
                {
                    return;
                }

                QJsonObject obj = resultDocument.object();
                QString funcName = obj["FuncName"].toString();
                QString parameter = obj["Parameter"].toString();

                QByteArray ba = funcName.toUtf8();
                QMetaObject::invokeMethod(this,ba.data(),Q_ARG(QString,parameter),Q_ARG(QObject*,parent));

            }
        }
    }
}

QString PowerShellPlugin::execute(QString funcName,QString parameter)
{
    QProcess process;
    process.setProgram("powershell");
    process.setWorkingDirectory(m_path);

    QJsonObject json;
    json.insert("FuncName", funcName);
    json.insert("Parameter", parameter);

    QJsonDocument document;
    document.setObject(json);
    QByteArray byte_array = document.toJson(QJsonDocument::Compact);

    QStringList args;
    args.append("./" + m_info.exeName);
    args.append("'" + byte_array + "'");
    process.setArguments(args);

    process.start();
    process.waitForFinished();

    QByteArray output = process.readAllStandardOutput();
    if (output.isEmpty())
    {
        QString strError = QString::fromLocal8Bit(process.readAllStandardError());
        qLog(QString("[%1] query error : %2").arg(m_info.name).arg(strError));
        return "";
    }

    QString gbk_output = QString::fromLocal8Bit(output);
    return gbk_output;
}

ScriptPlugin::ScriptPlugin(QString &pluginPath):Plugin(pluginPath)
{

}

bool ScriptPlugin::initPlugin(QString pluginPath)
{
    QString output = execute("InitPlugin",pluginPath);

    QJsonParseError json_error;
    QJsonDocument resultDocument = QJsonDocument::fromJson(output.toUtf8(),&json_error);
    if (json_error.error != QJsonParseError::NoError)
    {
        return false;
    }

    QJsonObject obj = resultDocument.object();
    bool result = obj["Result"].toBool();
    if (!result)
    {
        return false;
    }

    return true;
}

bool ScriptPlugin::query(Query query,QVector<Result>& vecResult)
{
    Settings* cfg = GetSettings();

    QString output = execute("Query",query.parameter);
    if (output.isEmpty())
    {
        return false;
    }

    QJsonParseError json_error;
    QJsonDocument resultDoc = QJsonDocument::fromJson(output.toUtf8(),&json_error);
    if (json_error.error != QJsonParseError::NoError)
    {
        qLog(QString("[%1] parse json failed, error:%2").
             arg(m_info.name).arg(json_error.errorString()));
        return false;
    }

    QJsonObject obj = resultDoc.object();
    QJsonArray resultsObj = obj["Results"].toArray();

    int maxCout = resultsObj.size() > cfg->m_maxResultsToShow ? cfg->m_maxResultsToShow:resultsObj.size();
    for (int i = 0; i < maxCout;i++)
    {
        QJsonObject resultObj = resultsObj[i].toObject();

        Result result;
        result.id = m_info.id;
        if (resultObj.contains("ShowType"))
        {
            result.showType = resultObj["ShowType"].toInt();
        }
        else
        {
            result.showType = SHOW_TYPE_DEFAULT;
        }
        result.title = resultObj["Title"].toString();
        result.subTitle = resultObj["SubTitle"].toString();
        QString iconPath = resultObj["IconPath"].toString();
        if (iconPath.isEmpty())
        {
            result.iconPath = m_path + QString("/%1").arg(m_info.iconPath);
        }
        else
        {
            if (iconPath.startsWith("Ra_") || QDir::isAbsolutePath(iconPath))
            {
                result.iconPath = iconPath;
            }
            else
            {
                result.iconPath = m_path + QString("/%1").arg(iconPath);
            }
        }

        if (resultObj.contains("ExtraData"))
        {
            result.extraData = resultObj["ExtraData"].toString();
        }

        if (resultObj.contains("Action"))
        {
            QJsonObject actionObj = resultObj["Action"].toObject();
            result.action.funcName = actionObj["FuncName"].toString();

            if (actionObj.contains("Parameter"))
            {
               result.action.parameter = actionObj["Parameter"].toString();
            }

            if (actionObj.contains("HideWindow"))
            {
                result.action.hideWindow = actionObj["HideWindow"].toBool();
            }
        }

        vecResult.append(result);
    }

    return true;
}

bool ScriptPlugin::getMenu(Result& result,QVector<Result>& vecMenu)
{
    QJsonObject oParameter;
    oParameter.insert("ID",result.id);
    oParameter.insert("Title",result.title);
    oParameter.insert("SubTitle", result.subTitle);
    oParameter.insert("ExtraData", result.extraData);

    QJsonDocument document;
    document.setObject(oParameter);
    QByteArray byte_array = document.toJson(QJsonDocument::Compact);

    QString output = execute("GetContextMenu",byte_array);
    if (output.isEmpty())
    {
        qLog(QString("[%1] getMenu error,result empty").arg(m_info.name));
        return false;
    }

    QJsonParseError json_error;
    QJsonDocument resultDocument = QJsonDocument::fromJson(output.toUtf8(),&json_error);
    if (json_error.error != QJsonParseError::NoError)
    {
        return false;
    }

    QJsonObject obj = resultDocument.object();
    QJsonArray menuArray = obj["Results"].toArray();

    for (int i = 0; i< menuArray.size();i++)
    {
        QJsonObject menuObj = menuArray[i].toObject();

        Result menu;
        menu.id = m_info.id;
        menu.title = menuObj["Title"].toString();
        menu.subTitle = menuObj["SubTitle"].toString();
        QString iconPath = menuObj["IconPath"].toString();
        if (iconPath.isEmpty())
        {
            menu.iconPath = m_path + QString("/%1").arg(m_info.iconPath);
        }
        else
        {
            if (iconPath.startsWith("Ra_") || QDir::isAbsolutePath(iconPath))
            {
                menu.iconPath = iconPath;
            }
            else
            {
                menu.iconPath = m_path + QString("/%1").arg(iconPath);
            }
        }

        if (menuObj.contains("Action"))
        {
            QJsonObject actionObj = menuObj["Action"].toObject();
            menu.action.funcName = actionObj["FuncName"].toString();

            if (actionObj.contains("Parameter"))
            {
                menu.action.parameter = actionObj["Parameter"].toString();
            }

            if (actionObj.contains("HideWindow"))
            {
                menu.action.hideWindow = actionObj["HideWindow"].toBool();
            }
        }

        vecMenu.append(menu);
    }

    return true;
}

void ScriptPlugin::itemClick(Result &item,QObject* parent)
{
    PluginAction& action = item.action;

    if (action.funcName.startsWith("Ra_"))
    {
        PVOID OldValue;
        Wow64DisableWow64FsRedirection (&OldValue);

        QByteArray ba = action.funcName.toUtf8();
        QMetaObject::invokeMethod(this,ba.data(),Q_ARG(QString,action.parameter),Q_ARG(QObject*,parent));

        Wow64RevertWow64FsRedirection (OldValue);

    }
    else
    {
        QString output = execute(action.funcName,action.parameter);
        if (!output.isEmpty())
        {
            QStringList funcList = output.split("\r\n",QString::SkipEmptyParts);

            foreach (QString func,funcList)
            {
                QJsonParseError json_error;
                QJsonDocument resultDocument = QJsonDocument::fromJson(func.toUtf8(),&json_error);
                if (json_error.error != QJsonParseError::NoError)
                {
                    return;
                }

                QJsonObject obj = resultDocument.object();
                QString funcName = obj["FuncName"].toString();
                QString parameter = obj["Parameter"].toString();

                QByteArray ba = funcName.toUtf8();
                QMetaObject::invokeMethod(this,ba.data(),Q_ARG(QString,parameter),Q_ARG(QObject*,parent));

            }
        }
    }
}

QString ScriptPlugin::execute(QString funcName,QString parameter)
{
    QProcess process;
    process.setProgram(m_info.interpreter);
    process.setWorkingDirectory(m_path);

    QJsonObject json;
    json.insert("FuncName", funcName);
    json.insert("Parameter", parameter);

    QJsonDocument document;
    document.setObject(json);
    QByteArray byte_array = document.toJson(QJsonDocument::Compact);

    QStringList args;
    if (!m_info.interpreterArgv.isEmpty())
        args.append(m_info.interpreterArgv.split(" ", Qt::SkipEmptyParts));
    args.append("./" + m_info.exeName);
    args.append(byte_array);
    process.setArguments(args);

    process.start();
    process.waitForFinished();

    QByteArray output = process.readAllStandardOutput();
    if (output.isEmpty())
    {
        QString strError = process.readAllStandardError();
        qLog(QString("[%1] query error : %2").arg(m_info.name).arg(strError));
    }

    return output;
}
