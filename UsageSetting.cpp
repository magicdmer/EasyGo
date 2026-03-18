#include "UsageSetting.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include "LogFile.h"

UsageSetting::UsageSetting():m_usage(0)
{

}

bool UsageSetting::load()
{
    QFile file("Settings\\Usage.json");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qLog("Open Usage.json failed");
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
    m_usage = jsonObject["ActivateTimes"].toInt();

    return true;
}

bool UsageSetting::save()
{
    QJsonDocument document;
    QJsonObject oRoot;

    oRoot["ActivateTimes"] = m_usage;

    document.setObject(oRoot);

    QByteArray byte_array = document.toJson(QJsonDocument::Indented);

    QFile file("Settings\\Usage.json");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return false;
    }
    file.write(byte_array);
    file.flush();
    file.close();

    return true;
}

UsageSetting* GetUsageSetting()
{
    static UsageSetting setting;
    static bool s_initialed = false;

    if (!s_initialed)
    {
        s_initialed = true;
        setting.load();
    }

    return &setting;
}
