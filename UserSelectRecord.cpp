#include "UserSelectRecord.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include "LogFile.h"
#include <QFile>

UserSelectRecord::UserSelectRecord()
{

}

bool UserSelectRecord::load()
{
    QFile file("Settings\\UserSelectRecord.json");
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
        int count = oRecord[query].toInt();
        m_records[query] = count;
    }

    return true;
}

bool UserSelectRecord::save()
{
    QJsonDocument document;
    QJsonObject oRoot;
    QJsonArray recordArray;

    QMapIterator<QString,int> mapIter(m_records);
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

    QFile file("Settings\\UserSelectRecord.json");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return false;
    }
    file.write(byte_array);
    file.flush();
    file.close();

    return true;
}

int UserSelectRecord::getScore(QString &key)
{
    if (m_records.contains(key))
    {
        return m_records[key];
    }

    return 0;
}

void UserSelectRecord::addScore(QString &key)
{
    int score = m_records[key];
    m_records[key] = score + 1;
}

UserSelectRecord* GetUserSelectRecord()
{
    static UserSelectRecord setting;
    static bool s_initialed = false;

    if (!s_initialed)
    {
        s_initialed = true;
        setting.load();
    }

    return &setting;
}
