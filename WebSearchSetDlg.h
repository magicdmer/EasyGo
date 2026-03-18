#ifndef WEBSEARCHSETDLG_H
#define WEBSEARCHSETDLG_H

#include <QDialog>

namespace Ui {
class WebSearchSetDlg;
}

class WebSearchSetDlg : public QDialog
{
    Q_OBJECT

public:
    explicit WebSearchSetDlg(QWidget *parent = 0);
    ~WebSearchSetDlg();
public slots:
    void sltViewIconPath();
    void sltOk();
    void sltCancel();
public:
    QString m_iconPath;
    QString m_title;
    QString m_keyword;
    QString m_url;
private:
    Ui::WebSearchSetDlg *ui;
};

#endif // WEBSEARCHSETDLG_H
