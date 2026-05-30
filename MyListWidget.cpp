#include "MyListWidget.h"
#include <QMouseEvent>
#include "ThemeSetting.h"
#include <QListWidgetItem>
#include "Settings.h"
#include "HelperFunc.h"

MyListWidget::MyListWidget(QWidget *parent):
    QListWidget(parent)
{
    setTheme();
}

void MyListWidget::mousePressEvent(QMouseEvent *me)
{
    QListWidget::mousePressEvent(me);

    if (me->button() == Qt::RightButton)
    {
        emit sigMouseRight();
    }
    else if (me->button() == Qt::LeftButton)
    {
        emit sigMouseLeft();
    }
}

int MyListWidget::number()
{
    int num = this->count();

    for (int i = 0; i < this->count(); i++)
    {
        QListWidgetItem* item = this->item(i);
        if (item->isHidden())
        {
            num--;
        }
    }

    return num;
}

void MyListWidget::setTheme(QColor mainColorForImage)
{
    QColor childWinColor = GetThemeSetting()->resolvedChildWinColor(mainColorForImage);
    QColor scrollbarColor = GetThemeSetting()->resolvedScrollbarColor(mainColorForImage);
    QColor selectedColor = GetThemeSetting()->resolvedItemSelectedColor(mainColorForImage);

    // 选中项高亮色（柔和偏移）
    QColor hoverColor = selectedColor;
    hoverColor.setAlpha(128);

    QString hoverStyle;
    if (GetSettings()->m_disableMouse)
    {
        hoverStyle = "";
    }
    else
    {
        hoverStyle = QString("QListWidget::Item:hover{background: %1;}").arg(selectedColor.name());
    }

    QString scrollStyle = ThemeSetting::scrollbarStyleSheet(scrollbarColor, childWinColor);

    QString styleSheet = QString(
        "%1"
        "QListWidget{background-color: %2; border: none; outline: none;}"
        "QListWidget::Item{background-color: %2; border: none; padding-left: 2px;}"
        "QListWidget::Item:selected{background-color: %3; border-left: 3px solid %4;}"
        "%5"
    ).arg(scrollStyle)
     .arg(childWinColor.name())
     .arg(selectedColor.name())
     .arg(GetThemeSetting()->isDarkTheme(mainColorForImage) ? scrollbarColor.lighter(120).name() : scrollbarColor.darker(150).name())
     .arg(hoverStyle);

    setStyleSheet(styleSheet);
}

ResultItem* MyListWidget::getCurrentItem()
{
    QListWidgetItem *curItem = currentItem();
    if (!curItem)
    {
        return NULL;
    }

    ResultItem* item = (ResultItem*)(itemWidget(curItem));
    if (!item)
    {
        return NULL;
    }

    return item;
}

ResultItem* MyListWidget::getItem(int index)
{
    QListWidgetItem* widgetItem = item(index);
    ResultItem* resultItem = (ResultItem*)itemWidget(widgetItem);
    return resultItem;
}

void MyListWidget::addItemWidget(ResultItem *item,bool hide)
{
    QListWidgetItem* widgetItem = new QListWidgetItem();
    widgetItem->setSizeHint(QSize(item->width(),item->height()));
    addItem(widgetItem);
    setItemWidget(widgetItem,item);
    if (hide)
    {
        widgetItem->setHidden(true);
    }
}

void MyListWidget::showItem(int index)
{
    QListWidgetItem* widgetItem = item(index);
     widgetItem->setHidden(false);
}

void MyListWidget::hideItem(int index)
{
    QListWidgetItem* widgetItem = item(index);
    widgetItem->setHidden(true);
}

void MyListWidget::hideAll()
{
    for (int i = 0; i < count(); i++)
    {
        hideItem(i);
    }
}
