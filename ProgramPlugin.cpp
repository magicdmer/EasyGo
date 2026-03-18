#include "ProgramPlugin.h"
#include "IndexDatabase.h"
#include <QVariant>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "ChineseLetterHelper.h"
#include "Settings.h"
#include "TopMostRecord.h"
#include "UserSelectRecord.h"
#include "PluginManager.h"
#include <QTime>
#include <Windows.h>
#include "WebSearchPlugin.h"
#include "HelperFunc.h"
#include "IndexTask.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
# pragma execution_character_set("utf-8")
#endif

ProgramPlugin::ProgramPlugin():Plugin()
{
    m_info.id = PROGRAM_PLUGIN_ID;
    m_info.name = tr("Program");
    m_info.keyword.append("*");
    m_info.pluginType = "c++";
    m_info.author = "magicdmer";
    m_info.enableSeparate = 0;
    m_iconPath = QDir::currentPath() + QString("/Images/exe.png");
    m_guid = m_info.id;
    m_mode = RealMode;
}

QString getFirstWord(QString& name)
{
    QString result;
    QStringList wordlist;

    if (name.contains(' '))
    {
        wordlist = name.split(' ');
    }
    else if (name.contains('_'))
    {
        wordlist = name.split('_');
    }
    else if (name.contains('.'))
    {
        wordlist = name.split('.');
    }
    else
    {
        for (int i = 0; i < name.length(); i++)
        {
            if (name[i].isUpper())
            {
                wordlist.append(QString(name[i]));
            }
        }
    }

    for (int i = 0; i < wordlist.length(); i++)
    {
        result += wordlist[i][0];
    }

    return result.toLower();
}

bool sortResult(Result& r1,Result& r2)
{
    TopMostRecord* topRecord = GetTopMostRecord();
    if (topRecord->isTopMost(r1) && !topRecord->isTopMost(r2))
    {
        return true;
    }
    else if (!topRecord->isTopMost(r1) && topRecord->isTopMost(r2))
    {
        return false;
    }

    UserSelectRecord* selectRecord = GetUserSelectRecord();
    QString recordKey1,recordKey2;
    if (r1.action.funcName == "Ra_ActivePlugin")
    {
        recordKey1 = r1.title;
    }
    else
    {
        recordKey1 = r1.subTitle;
    }
    if (r2.action.funcName == "Ra_ActivePlugin")
    {
        recordKey2 = r2.title;
    }
    else
    {
        recordKey2 = r2.subTitle;
    }

    if (selectRecord->getScore(recordKey1) > selectRecord->getScore(recordKey2))
    {
        return true;
    }
    else if (selectRecord->getScore(recordKey1) < selectRecord->getScore(recordKey2))
    {
        return false;
    }

    if (r1.action.funcName == "Ra_ActivePlugin" &&
        r2.action.funcName != "Ra_ActivePlugin")
    {
        return true;
    }
    else if (r1.action.funcName != "Ra_ActivePlugin" &&
        r2.action.funcName == "Ra_ActivePlugin")
    {
        return false;
    }
    else
    {
        QString strQuery = r1.extraData;

        QString lTitle = r1.title;
        QString rTitle = r2.title;
        int lindex = lTitle.lastIndexOf(".");
        int rindex = rTitle.lastIndexOf(".");

        if (-1 != lindex)
        {
            lTitle.truncate(lindex);
        }

        if (-1 != rindex)
        {
            rTitle.truncate(rindex);
        }

        if (lTitle.compare(strQuery,Qt::CaseInsensitive) == 0 &&
            rTitle.compare(strQuery,Qt::CaseInsensitive) != 0)
        {
            return true;
        }
        else if (lTitle.compare(strQuery,Qt::CaseInsensitive) != 0 &&
                 rTitle.compare(strQuery,Qt::CaseInsensitive) == 0)
        {
            return false;
        }

        QString lShortTitle = getFirstWord(lTitle);
        QString rShortTitle = getFirstWord(rTitle);

        if (lShortTitle.compare(strQuery,Qt::CaseInsensitive) == 0 &&
            rShortTitle.compare(strQuery,Qt::CaseInsensitive) != 0)
        {
            return true;
        }
        else if (lShortTitle.compare(strQuery,Qt::CaseInsensitive) != 0 &&
                 rShortTitle.compare(strQuery,Qt::CaseInsensitive) == 0)
        {
            return false;
        }

        if (lTitle.startsWith(strQuery,Qt::CaseInsensitive) &&
            !rTitle.startsWith(strQuery,Qt::CaseInsensitive))
        {
            return true;
        }
        else if (!lTitle.startsWith(strQuery,Qt::CaseInsensitive) &&
                 rTitle.startsWith(strQuery,Qt::CaseInsensitive))
        {
            return false;
        }

        if (lShortTitle.startsWith(strQuery,Qt::CaseInsensitive) &&
            !rShortTitle.startsWith(strQuery,Qt::CaseInsensitive))
        {
            return true;
        }
        else if (!lShortTitle.startsWith(strQuery,Qt::CaseInsensitive) &&
                 rShortTitle.startsWith(strQuery,Qt::CaseInsensitive))
        {
            return false;
        }

        if (lTitle.contains(strQuery,Qt::CaseInsensitive) &&
            !rTitle.contains(strQuery,Qt::CaseInsensitive))
        {
            return true;
        }
        else if (!lTitle.contains(strQuery,Qt::CaseInsensitive) &&
                 rTitle.contains(strQuery,Qt::CaseInsensitive))
        {
            return false;
        }

        QString pyQuery = ChineseLetterHelper::GetFirstLetters(strQuery);

        if (lTitle.compare(pyQuery,Qt::CaseInsensitive) == 0 &&
            rTitle.compare(pyQuery,Qt::CaseInsensitive) != 0)
        {
            return true;
        }
        else if (lTitle.compare(pyQuery,Qt::CaseInsensitive) != 0 &&
                 rTitle.compare(pyQuery,Qt::CaseInsensitive) == 0)
        {
            return false;
        }

        if (lTitle.startsWith(pyQuery,Qt::CaseInsensitive) &&
            !rTitle.startsWith(pyQuery,Qt::CaseInsensitive))
        {
            return true;
        }
        else if (!lTitle.startsWith(pyQuery,Qt::CaseInsensitive) &&
                 rTitle.startsWith(pyQuery,Qt::CaseInsensitive))
        {
            return false;
        }

        if (lTitle.contains(pyQuery,Qt::CaseInsensitive) &&
            !rTitle.contains(pyQuery,Qt::CaseInsensitive))
        {
            return true;
        }
        else if (!lTitle.contains(pyQuery,Qt::CaseInsensitive) &&
                 rTitle.contains(pyQuery,Qt::CaseInsensitive))
        {
            return false;
        }
    }

    return false;
}

bool ProgramPlugin::query(Query query,QVector<Result>& vecResult)
{
    QVector<ProgramInfo> pgList;
    QString rawQuery = query.rawQuery;

    if (rawQuery.contains(QString(" %1 ").arg(QChar(0x25ba))))
    {
        Result result;
        result.id = PROGRAM_PLUGIN_ID;
        result.title = QString("执行 %1").arg(query.keyword);
        result.subTitle = QString("参数 %1").arg(query.parameter);
        result.iconPath = "Ra_NativeIconExtra";
        result.extraData = query.keyword;
        result.action.funcName = "Ra_Open";
        result.action.parameter = QString("\"%1\" %2").arg(query.keyword).arg(query.parameter);
        vecResult.append(result);
        return true;
    }

    QVector<Plugin*> vecPlugin;
    if (GetPluginMananger()->search(rawQuery,vecPlugin))
    {
        foreach (Plugin* info,vecPlugin)
        {
            Result result;
            QString keyword;
            QStringList keysList;

            int index = GetSettings()->find(info->m_guid);
            if (index == -1)
            {
                keysList  = info->m_info.keyword;
            }
            else
            {
                keysList = GetSettings()->m_pluginConfig[index].keyword;
            }

            for (int i = 0; i < keysList.size(); i++)
            {
                if (keysList[i].startsWith(rawQuery,Qt::CaseInsensitive))
                {
                    keyword = keysList[i];
                    break;
                }
            }

            result.id = PROGRAM_PLUGIN_ID;
            result.title = keyword;
            result.subTitle = QString("激活%1插件").arg(info->m_info.name);
            if (info->m_guid == WEBSEARCH_PLUGIN_ID)
            {
                result.iconPath = ((WebSearchPlugin*)info)->getIconPath(keyword);
            }
            else
            {
                result.iconPath = info->m_iconPath;
            }
            result.action.funcName = "Ra_ActivePlugin";
            result.action.parameter = keyword + " ";
            result.action.hideWindow = false;
            result.extraData = rawQuery;

            vecResult.append(result);
        }
    }

    if (!IndexTask::isIndexing() &&
        QFile::exists("IndexCacheTemp.db"))
    {
        QFile::remove("IndexCache.db");
        QFile::rename("IndexCacheTemp.db","IndexCache.db");
    }

    QString con_name = GetRandomString(10);
    IndexDatabase database(con_name);
    database.load();

    if (!database.query(query.rawQuery,pgList))
    {
        return false;
    }

    foreach (ProgramInfo pg,pgList)
    {
        Result result;
        result.id = PROGRAM_PLUGIN_ID;
        result.title = pg.name;
        result.subTitle = pg.path;
        result.iconPath = "Ra_NativeIcon";
        result.action.funcName = "Ra_Open";
        result.action.parameter = pg.path;
        result.extraData = rawQuery;
        vecResult.append(result);
    }

    if (vecResult.isEmpty())
    {
        Result result;
        result.id = PROGRAM_PLUGIN_ID;
        result.title = tr("程序");
        result.subTitle = tr("用来检索程序");
        result.iconPath = m_iconPath;

        vecResult.append(result);

        return true;
    }

    std::sort(vecResult.begin(),vecResult.end(),sortResult);

    int maxResults = GetSettings()->m_maxResultsToShow;
    if (vecResult.size() > maxResults)
    {
        int rCount = vecResult.size() - maxResults;
        vecResult.remove(maxResults,rCount);
    }

    return true;
}

bool ProgramPlugin::getMenu(Result& result, QVector<Result>& vecMenu)
{
    if (result.action.funcName != "Ra_ActivePlugin")
    {
        PVOID OldValue;
        Wow64DisableWow64FsRedirection (&OldValue);

        QFileInfo info(result.subTitle);
        if (!info.isExecutable() && !info.isSymLink())
        {
            Wow64RevertWow64FsRedirection (OldValue);
            return false;
        }

        Result menu;
        menu.id = PROGRAM_PLUGIN_ID;
        menu.title = tr("打开");
        menu.iconPath = QDir::currentPath() + QString("/Images/cmd.png");
        menu.action.funcName = "Ra_Open";
        menu.action.parameter = info.filePath();
        vecMenu.append(menu);

        menu.title = tr("打开所在文件夹");
        menu.iconPath = QDir::currentPath() + QString("/Images/folder.png");
        menu.action.funcName = "Ra_OpenFileFolder";
        menu.action.parameter = info.filePath();
        vecMenu.append(menu);

        Wow64RevertWow64FsRedirection (OldValue);
    }


    Result topMenu;
    topMenu.id = PROGRAM_PLUGIN_ID;

    if (GetTopMostRecord()->isTopMost(result))
    {
        topMenu.title = tr("取消置顶");
        topMenu.iconPath = QDir::currentPath() + QString("/Images/down.png");
        topMenu.action.funcName = "Ra_DownResult";

        QJsonObject json;
        json.insert("Query",result.extraData);
        if (result.action.funcName == "Ra_ActivePlugin")
        {
            json.insert("FilePath",result.title);
        }
        else
        {
            json.insert("FilePath",result.subTitle);
        }

        QJsonDocument document;
        document.setObject(json);
        QByteArray byte_array = document.toJson(QJsonDocument::Compact);
        topMenu.action.parameter = byte_array;
    }
    else
    {
        topMenu.title = tr("在当前查询中置顶");
        topMenu.iconPath = QDir::currentPath() + QString("/Images/up.png");
        topMenu.action.funcName = "Ra_TopResult";

        QJsonObject json;
        json.insert("Query",result.extraData);
        if (result.action.funcName == "Ra_ActivePlugin")
        {
            json.insert("FilePath",result.title);
        }
        else
        {
            json.insert("FilePath",result.subTitle);
        }

        QJsonDocument document;
        document.setObject(json);
        QByteArray byte_array = document.toJson(QJsonDocument::Compact);
        topMenu.action.parameter = byte_array;
    }

    vecMenu.append(topMenu);

    Result authorMenu;
    authorMenu.id = PROGRAM_PLUGIN_ID;

    if (result.action.funcName == "Ra_ActivePlugin")
    {
        Plugin* plugin = GetPluginMananger()->getValidPlugin(result.title);
        if (plugin)
        {
            authorMenu.title = plugin->m_info.name;
            authorMenu.subTitle = QString("作者:%1").arg(plugin->m_info.author);
            authorMenu.iconPath = plugin->m_iconPath;
        }
    }
    else
    {
        Plugin* plugin = GetPluginMananger()->getPlugin(result.id);
        if (plugin)
        {
            authorMenu.title = plugin->m_info.name;
            authorMenu.subTitle = tr("作者:%1").arg(plugin->m_info.author);
            authorMenu.iconPath = m_iconPath;
        }
    }

    vecMenu.append(authorMenu);

    return true;
}

void ProgramPlugin::itemClick(Result &item,QObject* parent)
{
    PluginAction& action = item.action;

    if (action.funcName == "Ra_Open")
    {
        if (item.iconPath == "Ra_NativeIcon")
        {
            GetUserSelectRecord()->addScore(item.subTitle);
        }
        else
        {
            //带参数的
            QString filePath = item.title.split(" ")[1];
            GetUserSelectRecord()->addScore(filePath);
        }
    }
    else if (action.funcName == "Ra_ActivePlugin")
    {
        GetUserSelectRecord()->addScore(item.title);
    }

    GetUserSelectRecord()->save();

    if (action.funcName.startsWith("Ra_"))
    {
        PVOID OldValue;
        Wow64DisableWow64FsRedirection (&OldValue);
        QByteArray ba = action.funcName.toUtf8();
        QMetaObject::invokeMethod(this,ba.data(),Q_ARG(QString,action.parameter),Q_ARG(QObject*,parent));
        Wow64RevertWow64FsRedirection (OldValue);
    }
}
