#include "HotKeyEdit.h"
#include <QMessageBox>
#include <QKeyEvent>

HotKeyEdit::HotKeyEdit(QWidget *parent):
    QLineEdit (parent)
{

}

void HotKeyEdit::keyPressEvent(QKeyEvent *me)
{
    int uKey = me->key();
    if (uKey == Qt::Key_unknown)
    {
        return;
    }

    if (uKey == Qt::Key_Control
        || uKey == Qt::Key_Shift
        || uKey == Qt::Key_Alt
        || uKey == Qt::Key_Enter
        || uKey == Qt::Key_Return
        || uKey == Qt::Key_Tab
        || uKey == Qt::Key_CapsLock
        || uKey == Qt::Key_Escape)
    {
        return;
    }

    if (uKey == Qt::Key_Backspace)
    {
        clear();
        return;
    }

    bool bModifiers = false;
    Qt::KeyboardModifiers modifiers = me->modifiers();
    if (modifiers & Qt::ShiftModifier)
    {
        uKey += Qt::SHIFT;
        bModifiers = true;
    }
    if (modifiers & Qt::ControlModifier)
    {
        uKey += Qt::CTRL;
        bModifiers = true;
    }
    if (modifiers & Qt::AltModifier)
    {
        uKey += Qt::ALT;
        bModifiers = true;
    }

    if (!bModifiers)
    {
        return;
    }

    QString qsKey = QKeySequence(uKey).toString();

    //除去Windows常用快捷键
    QStringList blackList;
    blackList << "CTRL+S" << "CTRL+C" << "CTRL+V" << "CTRL+A" << "CTRL+D" << "CTRL+Z" << "CTRL+X";
    if (blackList.contains(qsKey, Qt::CaseInsensitive))
    {
        return;
    }

    setText(qsKey);
}
