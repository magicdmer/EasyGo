#ifndef RESULTITEM_H
#define RESULTITEM_H

#include <QWidget>
#include "Plugin.h"

namespace Ui {
class ResultItem;
}

class ResultItem : public QWidget
{
    Q_OBJECT

public:
    explicit ResultItem(QWidget *parent = 0,Result* result = 0);
    ~ResultItem();
public:
    void update();
    void updateIcon(QPixmap& pixmap);
    void setTheme();
    void setTheme(QColor& color);
public:
    QString m_uuid;
    Result m_result;
private:
    Ui::ResultItem *ui;
};

#endif // RESULTITEM_H
