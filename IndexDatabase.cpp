#include "IndexDatabase.h"
#include <QVariant>
#include "ChineseLetterHelper.h"

IndexDatabase::IndexDatabase(QString name)
{
    m_database = QSqlDatabase::addDatabase("QSQLITE",name);
    m_database.setDatabaseName("IndexCache.db");
}

IndexDatabase::IndexDatabase(QString conName, QString dbName)
{
    m_database = QSqlDatabase::addDatabase("QSQLITE",conName);
    m_database.setDatabaseName(dbName);
}

IndexDatabase::~IndexDatabase()
{
    QString connectionName = m_database.connectionName();
    m_database.close();
    m_database = QSqlDatabase();
    QSqlDatabase::removeDatabase(connectionName);
}

bool IndexDatabase::load()
{
    if (!m_database.open())
    {
        return false;
    }

    QStringList tableList = m_database.tables();
    if (tableList.isEmpty())
    {
        QSqlQuery query(m_database);
        // Windows 沿用发布版的原始结构，升级零影响；Linux 为新平台，额外存图标与
        // .desktop 路径，两套结构互不干扰。
#ifdef Q_OS_LINUX
        const char* createSql = "create table program(id integer primary key autoincrement,pyname text,name text,path text,iconpath text,desktoppath text)";
#else
        const char* createSql = "create table program(id integer primary key autoincrement,pyname text,name text,path text)";
#endif
        if (!query.exec(createSql))
        {
            return false;
        }
    }

    return true;
}

bool IndexDatabase::insert(ProgramInfo& info)
{
    QSqlQuery query(m_database);

#ifdef Q_OS_LINUX
    QString strQuery = QString("insert into program values(null,\"%1\",\"%2\",\"%3\",\"%4\",\"%5\")").
            arg(info.pyname).arg(info.name).arg(info.path).arg(info.iconPath).arg(info.desktopPath);
#else
    QString strQuery = QString("insert into program values(null,\"%1\",\"%2\",\"%3\")").
            arg(info.pyname).arg(info.name).arg(info.path);
#endif

    if (!query.exec(strQuery))
    {
        return false;
    }

    return true;
}

bool IndexDatabase::insert(QVector<ProgramInfo> &infos)
{
    if (!m_database.transaction())
    {
        return false;
    }

    foreach (ProgramInfo info,infos)
    {
        insert(info);
    }

    if (!m_database.commit())
    {
        return false;
    }

    return true;
}

bool IndexDatabase::query(QString querystr, QVector<ProgramInfo> &vecInfo)
{
    QString pyStr = ChineseLetterHelper::GetFirstLetters(querystr);

    QSqlQuery query(m_database);

    query.setForwardOnly(true);

    QString strQuery = "select * from program where pyname like \"";
    for (int i = 0; i <pyStr.length(); i++)
    {
        QChar c = pyStr[i];
        strQuery = strQuery + QString("%%%1").arg(c);
    }
    strQuery = strQuery + "%%\"";

    if (!query.exec(strQuery))
    {
        return false;
    }

    while(query.next())
    {
        ProgramInfo info;
        info.pyname = query.value(1).toString();
        info.name = query.value(2).toString();
        info.path = query.value(3).toString();
#ifdef Q_OS_LINUX
        info.iconPath = query.value(4).toString();
        info.desktopPath = query.value(5).toString();
#endif
        vecInfo.append(info);
    }

    return true;
}

bool IndexDatabase::clear()
{
    QSqlQuery query(m_database);

    QString strQuery = "delete from program";
    if (!query.exec(strQuery))
    {
        return false;
    }

    strQuery = "update sqlite_sequence set seq = 0 where name ='program'";
    if (!query.exec(strQuery))
    {
        return false;
    }

    return true;
}

IndexDatabase* GetIndexDatabase()
{
    static IndexDatabase* indexdb = nullptr;
    static bool s_initialed = false;

    if (!s_initialed)
    {
        s_initialed = true;
        indexdb = new IndexDatabase("MainQuery");
        indexdb->load();
    }

    return indexdb;
}
