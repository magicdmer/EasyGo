#ifndef WEBSEARCHPLUGIN_H
#define WEBSEARCHPLUGIN_H

#include "Plugin.h"
#include <QMap>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

struct SearchItem{
    QString title;
    QString keyword;
    QString url;
    QString iconPath;
    bool enabled;
};

class WebSearchCfg{
public:
    WebSearchCfg();
public:
    bool save();
    bool load();
public:
    QMap<QString,SearchItem> m_itemsMap;
    bool m_enableSuggest;
};

WebSearchCfg* GetWebSearchCfg();

class WebSearchPlugin : public Plugin
{
    Q_OBJECT
public:
    WebSearchPlugin();
public:
    bool initPlugin(QString pluginPath) { Q_UNUSED(pluginPath); return true;}
    bool query(Query query,QVector<Result>& vecResult);
    bool getMenu(Result& result, QVector<Result>& vecMenu) { Q_UNUSED(result) Q_UNUSED(vecMenu) return false; }
    void itemClick(Result &item,QObject* parent);
public:
    void addSource(SearchItem& item);
    void delSource(QString keyword);
    QString getIconPath(QString keyword);
private:
    bool getSuggest(QString& query,QStringList& suggestList);
private:
    QNetworkAccessManager m_manager;
    QNetworkReply* m_reply;
};

#endif // WEBSEARCHPLUGIN_H
