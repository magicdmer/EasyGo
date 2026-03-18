#ifndef MYLISTWIDGET_H
#define MYLISTWIDGET_H

#include <QListWidget>
#include "ResultItem.h"

class MyListWidget : public QListWidget
{
    Q_OBJECT
public:
    MyListWidget(QWidget *parent=0);
protected:
    void mousePressEvent(QMouseEvent *me);
public:
    int number();
    void setTheme();
    void setTheme(QColor& color);
    ResultItem* getCurrentItem();
    ResultItem* getItem(int index);
    void addItemWidget(ResultItem* item,bool hide = false);
    void hideItem(int index);
    void showItem(int index);
    void hideAll();
signals:
    void sigMouseLeft();
    void sigMouseRight();
};

#endif // MYLISTWIDGET_H
