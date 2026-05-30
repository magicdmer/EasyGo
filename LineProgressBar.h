#ifndef LINEPROGRESSBAR_H
#define LINEPROGRESSBAR_H

#include <QProgressBar>

class LineProgressBar : public QProgressBar {
    Q_OBJECT
public:
    explicit LineProgressBar(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *e) override;
    void timerEvent(QTimerEvent *e) override;

private:
    int m_pos;
};

#endif // LINEPROGRESSBAR_H
