#include "WebSearchSetDlg.h"
#include "ui_WebSearchSetDlg.h"
#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>
#include "WebSearchPlugin.h"
#include "Settings.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
# pragma execution_character_set("utf-8")
#endif

WebSearchSetDlg::WebSearchSetDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::WebSearchSetDlg)
{
    ui->setupUi(this);

    connect(ui->pushButtonView,SIGNAL(clicked()),this,SLOT(sltViewIconPath()));
    connect(ui->pushButtonOk,SIGNAL(clicked()),this,SLOT(sltOk()));
    connect(ui->pushButtonCancel,SIGNAL(clicked()),this,SLOT(sltCancel()));
}

WebSearchSetDlg::~WebSearchSetDlg()
{
    delete ui;
}

void WebSearchSetDlg::sltViewIconPath()
{
    QFileDialog dialog(0,tr("选择磁盘分区"),
                QDir::currentPath() + QString("/Images"),
                QObject::tr("支持类型 (*.*)"));
    dialog.setViewMode(QFileDialog::Detail);
    dialog.setFileMode(QFileDialog::ExistingFile);
    if(dialog.exec())
    {
        QString filePath = dialog.selectedFiles()[0];
        QPixmap pixmap;
        pixmap.load(filePath);
        if (pixmap.isNull())
        {
            QMessageBox::warning(this, tr("错误"),tr("不支持该图片格式，请选择其他"));
            return;
        }

        ui->labelPic->setPixmap(pixmap.scaled(32,32));

        m_iconPath = filePath;
    }
}

void WebSearchSetDlg::sltOk()
{
    m_title = ui->lineEditTitle->text();
    m_url = ui->lineEditUrl->text();
    m_keyword = ui->lineEditKey->text();

    if (m_url.isEmpty() || m_keyword.isEmpty())
    {
        QMessageBox::information(this,tr("提示"),tr("参数不能为空！"));
        return;
    }

    if (GetWebSearchCfg()->m_itemsMap.contains(m_keyword) ||
        GetSettings()->isKeywordExsit(m_keyword))
    {
        QMessageBox::information(this,tr("提示"),tr("关键字已被占用，请使用其他关键字"));
        return;
    }

    accept();
}

void WebSearchSetDlg::sltCancel()
{
    reject();
}
