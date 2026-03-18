#include "PluginSetDlg.h"
#include "ui_PluginSetDlg.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include "LogFile.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
# pragma execution_character_set("utf-8")
#endif

PluginSetDlg::PluginSetDlg(QString filePath, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PluginSetDlg),
    m_filePath(filePath)
{
    ui->setupUi(this);

    QStringList labels;
    labels << tr("配置项") << tr("数值");

    ui->treeWidget->setHeaderLabels(labels);
    ui->treeWidget->header()->setSectionResizeMode(QHeaderView::Interactive);
    ui->treeWidget->header()->setSectionsMovable(false);
    ui->treeWidget->header()->setStretchLastSection(false);
    ui->treeWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    ui->treeWidget->setColumnWidth(0,100);
    ui->treeWidget->setEditTriggers(QAbstractItemView::DoubleClicked);
    ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    initSettings();

    connect(ui->treeWidget,SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
            this,SLOT(sltItemDoubleClicked(QTreeWidgetItem*,int)));
    connect(ui->treeWidget,SIGNAL(itemChanged(QTreeWidgetItem*,int)),
            this,SLOT(sltItemChanged(QTreeWidgetItem*,int)));
}

PluginSetDlg::~PluginSetDlg()
{
    saveSettings();

    delete ui;
}

void PluginSetDlg::initSettings()
{
    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qLog(tr("Open %1 failed").arg(m_filePath));
        return;
    }

    QString jsonData = file.readAll();
    file.close();

    QJsonParseError parseJsonErr;
    QJsonDocument document = QJsonDocument::fromJson(jsonData.toUtf8(), &parseJsonErr);
    if (parseJsonErr.error != QJsonParseError::NoError)
    {
        qLog(tr("Parser %1 failed").arg(m_filePath));
        return;
    }

    QJsonObject jsonObject = document.object();
    QStringList keyList = jsonObject.keys();
    for(int i = 0; i < keyList.size(); i++)
    {
        QTreeWidgetItem *childItem = new QTreeWidgetItem;
        childItem->setText(0, keyList[i]);
        QString value = jsonObject[keyList[i]].toString();
        childItem->setText(1, value);
        ui->treeWidget->addTopLevelItem(childItem);
    }
}

void PluginSetDlg::saveSettings()
{
    QJsonDocument document;
    QJsonObject oRoot;

    QTreeWidgetItemIterator it(ui->treeWidget);
    while (*it)
    {
        oRoot[(*it)->text(0)] = (*it)->text(1);
        ++it;
    }

    document.setObject(oRoot);

    QByteArray byte_array = document.toJson(QJsonDocument::Indented);
    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return;
    }
    file.write(byte_array);
    file.flush();
    file.close();
}

void PluginSetDlg::sltItemDoubleClicked(QTreeWidgetItem * item, int column)
{
    Qt::ItemFlags tmp = item->flags();
    if (column == 1) {
        item->setFlags(tmp | Qt::ItemIsEditable);
    } else if (tmp & Qt::ItemIsEditable) {
        item->setFlags(tmp ^ Qt::ItemIsEditable);
    }
}

void PluginSetDlg::sltItemChanged(QTreeWidgetItem * item, int column)
{
    QString strKey = item->text(column);

}

