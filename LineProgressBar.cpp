#include "LineProgressBar.h"
#include <QPainter>
#include <QTimerEvent>
#include <QLinearGradient>
#include <QPalette>

LineProgressBar::LineProgressBar(QWidget *parent) : QProgressBar(parent), m_pos(-120) {
    startTimer(16); // ~60fps
}

void LineProgressBar::paintEvent(QPaintEvent *e) {
    if (minimum() == 0 && maximum() == 0) {
        QPainter painter(this);
        int chunkWidth = 120;
        
        if (m_pos + chunkWidth < 0 || m_pos > width()) {
            return;
        }
        
        QLinearGradient grad(m_pos, 0, m_pos + chunkWidth, 0);
        QColor color = palette().color(QPalette::Highlight);
        QColor trans = color;
        trans.setAlpha(0);
        
        grad.setColorAt(0.0, trans);
        grad.setColorAt(0.5, color);
        grad.setColorAt(1.0, trans);
        
        painter.fillRect(m_pos, 0, chunkWidth, height(), grad);
    } else {
        QProgressBar::paintEvent(e);
    }
}

void LineProgressBar::timerEvent(QTimerEvent *e) {
    if (minimum() == 0 && maximum() == 0) {
        if (!isVisible()) return;
        
        int chunkWidth = 120;
        m_pos += 4;
        if (m_pos > width()) {
            m_pos = -chunkWidth;
        }
        update();
    } else {
        QProgressBar::timerEvent(e);
    }
}
