#include "WebSearchPlugin.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include "LogFile.h"
#include <QDir>
#include <QEventLoop>
#include <QTextCodec>

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
# pragma execution_character_set("utf-8")
#endif

WebSearchCfg::WebSearchCfg()
{

}

bool WebSearchCfg::load()
{
    QFile file("Settings\\WebSearch.json");
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
        qLog(QString("Parse WebSearch Settings.json failed, error : %1").arg(parseJsonErr.errorString()));
        return false;
    }

    QJsonObject jsonObject = document.object();
    m_enableSuggest = jsonObject["EnableSuggestion"].toBool();

    QJsonArray pluginArray = jsonObject["SearchSources"].toArray();
    for (int i = 0; i < pluginArray.size(); i++)
    {
        QJsonObject oRecord = pluginArray[i].toObject();
        SearchItem item;
        item.title = oRecord["Title"].toString();
        item.keyword = oRecord["Keyword"].toString();
        item.iconPath = oRecord["Icon"].toString();
        item.url = oRecord["Url"].toString();
        item.enabled = oRecord["Enabled"].toBool();
        m_itemsMap[item.keyword] = item;
    }

    return true;
}

bool WebSearchCfg::save()
{
    QJsonDocument document;
    QJsonObject oRoot;
    QJsonArray recordArray;

    oRoot["EnableSuggestion"] = m_enableSuggest;

    QMapIterator<QString,SearchItem> mapIter(m_itemsMap);
    while (mapIter.hasNext())
    {
        mapIter.next();
        QJsonObject item;
        item["Title"] = mapIter.value().title;
        item["Keyword"] = mapIter.value().keyword;
        item["Icon"] = mapIter.value().iconPath;
        item["Url"] = mapIter.value().url;
        item["Enabled"] = mapIter.value().enabled;
        recordArray.append(item);
    }

    oRoot["SearchSources"] = recordArray;

    document.setObject(oRoot);

    QByteArray byte_array = document.toJson(QJsonDocument::Indented);

    QFile file("Settings\\WebSearch.json");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return false;
    }
    file.write(byte_array);
    file.flush();
    file.close();

    return true;
}

WebSearchCfg* GetWebSearchCfg()
{
    static WebSearchCfg setting;
    static bool s_initialed = false;

    if (!s_initialed)
    {
        s_initialed = true;
        setting.load();
    }

    return &setting;
}

WebSearchPlugin::WebSearchPlugin():Plugin()
{
    WebSearchCfg* cfg = GetWebSearchCfg();

    m_info.id = WEBSEARCH_PLUGIN_ID;
    m_info.name = tr("WebSearch");
    m_info.pluginType = "c++";
    m_info.author = "magicdmer";
    m_info.enableSeparate = 0;
    m_iconPath = QDir::currentPath() + QString("/Images/find.png");

    QMapIterator<QString,SearchItem> mapIter(cfg->m_itemsMap);
    while (mapIter.hasNext())
    {
        mapIter.next();
        m_info.keyword.append(mapIter.key());
    }

    m_guid = m_info.id;
    m_mode = RealMode;
}

bool WebSearchPlugin::query(Query query, QVector<Result> &vecResult)
{
    WebSearchCfg* cfg = GetWebSearchCfg();

    if (!cfg->m_itemsMap.contains(query.keyword))
    {
        return true;
    }

    SearchItem& item = cfg->m_itemsMap[query.keyword];
    if (query.parameter.isEmpty())
    {
        Result result;
        result.id = WEBSEARCH_PLUGIN_ID;
        result.title = tr("打开 %1").arg(item.title);
        result.subTitle = "";
        result.iconPath = item.iconPath;
        result.action.funcName = "Ra_OpenWeb";
        result.action.parameter = item.url;
        vecResult.append(result);
        return true;
    }

    QString searchUrl = item.url;
    searchUrl.replace("{q}",query.parameter);

    Result result;
    result.id = WEBSEARCH_PLUGIN_ID;
    result.title = query.parameter;
    result.subTitle = tr("搜索 %1").arg(item.title);
    result.iconPath = item.iconPath;
    result.action.funcName = "Ra_OpenWeb";
    result.action.parameter = searchUrl;
    vecResult.append(result);

    if (cfg->m_enableSuggest)
    {
        QStringList suggestList;
        if (getSuggest(query.parameter,suggestList))
        {
            foreach (QString sug,suggestList)
            {
                Result result;
                result.id = WEBSEARCH_PLUGIN_ID;
                result.title = sug;
                result.subTitle = tr("搜索 %1").arg(item.title);
                result.iconPath = item.iconPath;
                result.action.funcName = "Ra_OpenWeb";
                searchUrl = item.url;
                searchUrl.replace("{q}",sug);
                result.action.parameter = searchUrl;
                vecResult.append(result);
            }
        }
    }

    return true;
}

void WebSearchPlugin::itemClick(Result &item, QObject *parent)
{
    PluginAction& action = item.action;
    if (action.funcName.startsWith("Ra_"))
    {
        QByteArray ba = action.funcName.toUtf8();
        QMetaObject::invokeMethod(this,ba.data(),Q_ARG(QString,action.parameter),Q_ARG(QObject*,parent));
    }
}

bool WebSearchPlugin::getSuggest(QString &query,QStringList& suggestList)
{
    QString url = QString("http://suggestion.baidu.com/su?json=1&wd=%1").arg(query);
    QNetworkRequest request;

    request.setHeader(QNetworkRequest::UserAgentHeader,
        "Mozilla/5.0 (Windows NT 6.3; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/41.0.2226.0 Safari/537.36");
    request.setUrl(url);

    m_reply = m_manager.get(request);

    QEventLoop loop;
    connect(m_reply,SIGNAL(finished()),&loop,SLOT(quit()));
    loop.exec();

    if (m_reply->error())
    {
        m_reply->deleteLater();
        return false;
    }

    QTextCodec* codec = QTextCodec::codecForName("gbk");
    QString all = codec->toUnicode(m_reply->readAll());
    if (all.isEmpty())
    {
        m_reply->deleteLater();
        return false;
    }

    m_reply->deleteLater();

    int bindex = all.indexOf("(");
    if (bindex == -1)
    {
        return false;
    }

    int eindex = all.lastIndexOf(")");
    if (eindex == -1)
    {
        return false;
    }

    QString strJson = all.mid(bindex+1,eindex - bindex -1);
    if (strJson.isEmpty())
    {
        return false;
    }

    QJsonParseError json_error;
    QJsonObject obj;
    QJsonDocument parse_document = QJsonDocument::fromJson(strJson.toUtf8(),&json_error);
    if (json_error.error != QJsonParseError::NoError)
    {
        return false;
    }

    obj = parse_document.object();

    QJsonArray array = obj["s"].toArray();
    if (array.isEmpty())
    {
        return false;
    }

    for (int i = 0; i < array.size();i++)
    {
        suggestList.append(array[i].toString());
    }

    return true;
}

void WebSearchPlugin::addSource(SearchItem &item)
{
    GetWebSearchCfg()->m_itemsMap[item.keyword] = item;
    m_info.keyword.append(item.keyword);
}

void WebSearchPlugin::delSource(QString keyword)
{
    GetWebSearchCfg()->m_itemsMap.remove(keyword);
    m_info.keyword.removeAll(keyword);
}

QString WebSearchPlugin::getIconPath(QString keyword)
{
    WebSearchCfg* cfg = GetWebSearchCfg();

    if (!cfg->m_itemsMap.contains(keyword))
    {
        return m_iconPath;
    }

    SearchItem& item = cfg->m_itemsMap[keyword];

    return item.iconPath;
}
