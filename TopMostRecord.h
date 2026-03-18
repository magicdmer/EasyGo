#ifndef TOPMOSTRECORD_H
#define TOPMOSTRECORD_H

#include "Plugin.h"
#include <QMap>


class TopMostRecord
{
public:
    TopMostRecord();
public:
    bool load();
    bool save();
    bool isTopMost(Result& result);
    void remove(QString& query);
    void addOrUpdate(QString& query,QString& path);
public:
    QMap<QString,QString> m_records;
};

TopMostRecord* GetTopMostRecord();

#endif // TOPMOSTRECORD_H
