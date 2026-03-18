#ifndef HOTKEYEDIT_H
#define HOTKEYEDIT_H

#include <QLineEdit>

class HotKeyEdit : public QLineEdit
{
    Q_OBJECT
public:
    HotKeyEdit(QWidget *parent=0);
protected:
    void keyPressEvent(QKeyEvent *event);
};

#endif // HOTKEYEDIT_H
