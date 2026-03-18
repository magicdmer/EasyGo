#ifndef ICONEXTRACTOR_H
#define ICONEXTRACTOR_H

#include <QObject>
#include <QThread>
#include <QPixmap>
#include "Plugin.h"

class IconExtractor : public QThread
{
    Q_OBJECT
public:
    IconExtractor();
public:
    void processIcons(const QVector<Result>& newItems);
    void run();
    void stop();
signals:
    void iconExtracted(int itemIndex, QPixmap icon);
private:
    QVector<Result> m_results;
    bool m_run;
};

#endif // ICONEXTRACTOR_H
