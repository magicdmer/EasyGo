#ifndef INDEXDATABASE_H
#define INDEXDATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVector>

struct ProgramInfo{
    QString pyname;
    QString name;
    QString path;
};

class IndexDatabase
{
public:
    IndexDatabase();
    IndexDatabase(QString name);
    IndexDatabase(QString connecName, QString dbName);
    ~IndexDatabase();

public:
    bool load();
    bool beginInsert()
    {
        if (!m_database.transaction())
        {
            return false;
        }

        return true;
    }
    bool insert(ProgramInfo& info);
    bool endInsert()
    {
        if (!m_database.commit())
        {
            return false;
        }

        return true;
    }
    bool insert(QVector<ProgramInfo> &infos);
    bool query(QString query,QVector<ProgramInfo>& vecInfo);
    bool clear();

private:
    QSqlDatabase m_database;
};

IndexDatabase* GetIndexDatabase();

#endif // INDEXDATABASE_H
