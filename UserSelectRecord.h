#ifndef USERSELECTRECORD_H
#define USERSELECTRECORD_H

#include <QObject>
#include <QMap>

class UserSelectRecord
{
public:
    UserSelectRecord();
public:
    bool load();
    bool save();
    int getScore(const QString& key);
    void addScore(const QString& key);
private:
    QMap<QString,int> m_records;
};

UserSelectRecord* GetUserSelectRecord();

#endif // USERSELECTRECORD_H
