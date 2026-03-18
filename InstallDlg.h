#ifndef INSTALLDLG_H
#define INSTALLDLG_H

#include <QDialog>

namespace Ui {
class InstallDlg;
}

class InstallDlg : public QDialog
{
    Q_OBJECT

public:
    explicit InstallDlg(QWidget *parent = nullptr);
    ~InstallDlg();
public:
    void setMsg(QString msg);
public slots:
    void sltOk();
    void sltCancel();
public:
    bool m_bReplaceCfg;
private:
    Ui::InstallDlg *ui;
};

#endif // INSTALLDLG_H
