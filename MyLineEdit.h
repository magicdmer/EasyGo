#ifndef MYLINEEDIT_H
#define MYLINEEDIT_H

#include <QLineEdit>

class MyLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    MyLineEdit(QWidget *parent=0);
    QString separatorText() const;
protected:
    void keyPressEvent(QKeyEvent *me);
    void mousePressEvent(QMouseEvent *e);
signals:
    void clicked();
private:
    bool isAtStartOfSeparator() const;
    bool isAtEndOfSeparator() const;
    QChar separatorChar() const;
};

#endif // MYLINEEDIT_H
