#include "TopMostRecord.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include "LogFile.h"

TopMostRecord::TopMostRecord()
{

}

bool TopMostRecord::load()
{
    QFile file("Settings\\TopMostRecord.json");
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
        qLog(QString("Parse Settings.json failed, error : %1").arg(parseJsonErr.errorString()));
        return false;
    }

    QJsonObject jsonObject = document.object();
    QJsonArray pluginArray = jsonObject["Records"].toArray();
    for (int i = 0; i < pluginArray.size(); i++)
    {
        QJsonObject oRecord = pluginArray[i].toObject();
        QString query = oRecord.keys()[0];
        QString path = oRecord[query].toString();
        m_records[query] = path;
    }

    return true;
}

bool TopMostRecord::save()
{
    QJsonDocument document;
    QJsonObject oRoot;
    QJsonArray recordArray;

    QMapIterator<QString,QString> mapIter(m_records);
    while (mapIter.hasNext())
    {
        mapIter.next();
        QJsonObject record;
        record[mapIter.key()] = mapIter.value();
        recordArray.append(record);
    }

    oRoot["Records"] = recordArray;

    document.setObject(oRoot);

    QByteArray byte_array = document.toJson(QJsonDocument::Indented);

    QFile file("Settings\\TopMostRecord.json");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return false;
    }
    file.write(byte_array);
    file.flush();
    file.close();

    return true;
}

bool TopMostRecord::isTopMost(Result &result)
{
    QString path = m_records[result.extraData];

    if (result.action.funcName == "Ra_ActivePlugin")
    {
        if (path != result.title)
        {
            return false;
        }
    }
    else
    {
        if (path != result.subTitle)
        {
            return false;
        }
    }

    return true;
}

void TopMostRecord::addOrUpdate(QString& query,QString& path)
{
    m_records[query] = path;
}

void TopMostRecord::remove(QString& query)
{
    m_records.remove(query);
}

TopMostRecord* GetTopMostRecord()
{
    static TopMostRecord setting;
    static bool s_initialed = false;

    if (!s_initialed)
    {
        s_initialed = true;
        setting.load();
    }

    return &setting;
}
