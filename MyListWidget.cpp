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

void MyListWidget::setTheme()
{
    QString hoverStyle;
    if (GetSettings()->m_disableMouse)
    {
        hoverStyle = "QListWidget::Item::disabled:hover; }";
    }
    else
    {
        hoverStyle = "QListWidget::Item:hover{background:rgb(@3); }";
    }

    QString styleSheet = QString(
        "QScrollBar:vertical {"
        "    border: 1px solid #999999;"
        "    background:white;"
        "    width: 8px;    "
        "    margin: 0px 0px 0px 0px;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop: 0 @1, stop: 0.5 @1, stop:1 @1);"
        "    min-height: 0px;"
        "}"
        "QScrollBar::add-line:vertical {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop: 0 @1, stop: 0.5 @1,  stop:1 @1);"
        "    height: 0px;"
        "    subcontrol-position: bottom;"
        "    subcontrol-origin: margin;"
        "}"
        "QScrollBar::sub-line:vertical {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop: 0  @1, stop: 0.5 @1,  stop:1 @1);"
        "    height: 0 px;"
        "    subcontrol-position: top;"
        "    subcontrol-origin: margin;"
        "}"
        "QListWidget::Item{background-color: @2;}"
        "QListWidget::Item:selected{background-color: @3;}"
        "%1"
        "QListWidget{background-color: @2;}"
    ).arg(hoverStyle);

    Theme* theme = GetThemeSetting()->getSelectTheme();
    if (theme && !theme->type)
    {
        QColor child_win_color = ToColor(theme->child_win_color);
        if (theme->child_win_color.isEmpty())
        {
            child_win_color = ToColor(theme->win_color);
        }

        QColor scrollbar_color = ToColor(theme->scrollbar_color);
        if (theme->scrollbar_color.isEmpty())
        {
            scrollbar_color = child_win_color.darker(125);
        }

        QColor item_selected_color = ToColor(theme->item_selected_color);
        if (theme->item_selected_color.isEmpty())
        {
            item_selected_color = child_win_color.darker(120);
        }

        styleSheet = styleSheet.replace("@1", scrollbar_color.name());
        styleSheet = styleSheet.replace("@2", child_win_color.name());
        styleSheet = styleSheet.replace("@3", item_selected_color.name());
    }

    setStyleSheet(styleSheet);
}

void MyListWidget::setTheme(QColor& color)
{
    QString hoverStyle;
    if (GetSettings()->m_disableMouse)
    {
        hoverStyle = "QListWidget::Item::disabled:hover; }";
    }
    else
    {
        hoverStyle = "QListWidget::Item:hover{background:rgb(@3); }";
    }

    QString styleSheet = QString(
        "QScrollBar:vertical {"
        "    border: 1px solid #999999;"
        "    background:white;"
        "    width: 8px;    "
        "    margin: 0px 0px 0px 0px;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop: 0 @1, stop: 0.5 @1, stop:1 @1);"
        "    min-height: 0px;"
        "}"
        "QScrollBar::add-line:vertical {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop: 0 @1, stop: 0.5 @1,  stop:1 @1);"
        "    height: 0px;"
        "    subcontrol-position: bottom;"
        "    subcontrol-origin: margin;"
        "}"
        "QScrollBar::sub-line:vertical {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop: 0  @1, stop: 0.5 @1,  stop:1 @1);"
        "    height: 0 px;"
        "    subcontrol-position: top;"
        "    subcontrol-origin: margin;"
        "}"
        "QListWidget::Item{background-color: @2;}"
        "QListWidget::Item:selected{background-color: @3;}"
        "%1"
        "QListWidget{background-color: @2;}"
    ).arg(hoverStyle);

    Theme* theme = GetThemeSetting()->getSelectTheme();
    if (theme)
    {
        QColor child_win_color = ToColor(theme->child_win_color);
        if (theme->child_win_color.isEmpty())
        {
            child_win_color = color;
        }

        QColor scrollbar_color = ToColor(theme->scrollbar_color);
        if (theme->scrollbar_color.isEmpty())
        {
            scrollbar_color = child_win_color.darker(125);
        }

        QColor item_selected_color = ToColor(theme->item_selected_color);
        if (theme->item_selected_color.isEmpty())
        {
            item_selected_color = child_win_color.darker(120);
        }

        styleSheet = styleSheet.replace("@1", scrollbar_color.name());
        styleSheet = styleSheet.replace("@2", child_win_color.name());
        styleSheet = styleSheet.replace("@3", item_selected_color.name());
    }

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
