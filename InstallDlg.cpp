#include "InstallDlg.h"
#include "ui_InstallDlg.h"

InstallDlg::InstallDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::InstallDlg)
{
    ui->setupUi(this);
    m_bReplaceCfg = false;

    connect(ui->okButton,SIGNAL(clicked()),this,SLOT(sltOk()));
    connect(ui->cancelButton,SIGNAL(clicked()),this,SLOT(sltCancel()));
}

InstallDlg::~InstallDlg()
{
    delete ui;
}

void InstallDlg::setMsg(QString msg)
{
    ui->label->setText(msg);
}

void InstallDlg::sltOk()
{
    m_bReplaceCfg = ui->checkBox->isChecked();
    accept();
}

void InstallDlg::sltCancel()
{
    reject();
}
