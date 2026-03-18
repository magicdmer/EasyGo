#include "MyLineEdit.h"
#include <QKeyEvent>
#include <QMouseEvent>

MyLineEdit::MyLineEdit(QWidget *parent):
    QLineEdit (parent)
{

}

void MyLineEdit::keyPressEvent(QKeyEvent *me)
{
    bool handled = false;

    if (selectionStart() == -1)
    {
        switch (me->key())
        {
        case Qt::Key_Backspace:
            if (isAtEndOfSeparator())
            {
                backspace();
                backspace();
                backspace();
                handled = true;
            }
            break;

        case Qt::Key_Delete:
            if (isAtStartOfSeparator())
            {
                del();
                del();
                del();
                handled = true;
            }
            break;

        case Qt::Key_Left:
            if (isAtEndOfSeparator())
            {
                cursorBackward(false, 3);
                handled = true;
            }
            break;
        case Qt::Key_Right:
            if (isAtStartOfSeparator())
            {
                cursorForward(false,3);
                handled = true;
            }
        }
    }

    if (handled)
    {
        return;
    }
    else
    {
        QLineEdit::keyPressEvent(me);
    }
}

void MyLineEdit::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        emit clicked();
    }

    QLineEdit::mousePressEvent(event);
}

QChar MyLineEdit::separatorChar() const
{
    QFontMetrics met = fontMetrics();
    QChar arrow(0x25ba);
    if (met.inFont(arrow))
        return arrow;
    else
        return QChar('|');
}


QString MyLineEdit::separatorText() const
{
    return QString(" ") + separatorChar() + " ";
}


bool MyLineEdit::isAtStartOfSeparator() const
{
    return text().mid(cursorPosition(), 3) == separatorText();
}


bool MyLineEdit::isAtEndOfSeparator() const
{
    return text().mid(cursorPosition() - 3, 3) == separatorText();
}
