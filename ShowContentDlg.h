#ifndef SHOWCONTENTDLG_H
#define SHOWCONTENTDLG_H

#include <QDialog>
#include <QTimer>

namespace Ui {
class ShowContentDlg;
}

class ShowContentDlg : public QDialog
{
    Q_OBJECT

public:
    explicit ShowContentDlg(QWidget *parent = nullptr);
    ShowContentDlg(QString filePath,QWidget *parent = nullptr);
    ShowContentDlg(QString filePath, bool transparent ,QWidget *parent = nullptr);
    ~ShowContentDlg();
protected:
    void mousePressEvent(QMouseEvent *me);
    void mouseMoveEvent(QMouseEvent *me);
    void mouseReleaseEvent(QMouseEvent *me);
public:
    void setTheme();
    void setTheme(QColor& color);
    void setContent(QString& content);
    void setContent(QString& title, QString& content);
    void setReadOnly(bool ro);
    bool save();
    bool load();
public slots:
    void sltTextChanged();
    void sltFilterEntries();
private:
    Ui::ShowContentDlg *ui;
    bool m_move;
    QPoint m_lastPoint;
    QTimer* m_typingTimer;

    QString m_filePath;
    QString m_noteName;
    bool m_textChanged;
    QString m_filterText;
};

#endif // SHOWCONTENTDLG_H
