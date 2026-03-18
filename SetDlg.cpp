#include "SetDlg.h"
#include "ui_SetDlg.h"
#include "PluginManager.h"
#include <QDesktopWidget>
#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QCloseEvent>
#include <QSettings>
#include "WebSearchPlugin.h"
#include "WebSearchSetDlg.h"
#include <QProgressDialog>
#include <QTextCodec>
#include "UsageSetting.h"
#include "quazip.h"
#include "quazipfile.h"
#include "JlCompress.h"
#include "HelperFunc.h"
#include "PluginSetDlg.h"
#include "InstallDlg.h"
#include "ShowContentDlg.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
# pragma execution_character_set("utf-8")
#endif


SetDlg::SetDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SetDlg),
    m_task(nullptr)
{
    ui->setupUi(this);

    setWindowFlags(windowFlags()&~Qt::WindowContextHelpButtonHint);

    setFixedSize(742,523);

    int primary = QApplication::desktop()->primaryScreen();
    int iwidth = QApplication::desktop()->screen(primary)->width();
    int iheight = QApplication::desktop()->screen(primary)->height();

    move ((iwidth - width())/2,(iheight - height())/2);

    QStringList labels;
    labels << tr("") << tr("名字") << tr("描述") << tr("作者")
           << tr("版本") << tr("关键字") << tr("回车模式") << tr("是否禁止");
    
    ui->treeWidget->setHeaderLabels(labels);
    ui->treeWidget->header()->setSectionResizeMode(QHeaderView::Interactive);
    ui->treeWidget->header()->setSectionsMovable(false);
    ui->treeWidget->header()->setStretchLastSection(false);
    ui->treeWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    ui->treeWidget->setIconSize(QSize(24,24));
    ui->treeWidget->setColumnWidth(0,50);
    ui->treeWidget->setEditTriggers(QAbstractItemView::DoubleClicked);
    ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    

    labels.clear();
    labels << tr("标题") << tr("关键字") << tr("网址") << tr("启用");
    ui->wsTreeWidget->header()->setSectionResizeMode(QHeaderView::Interactive);
    ui->wsTreeWidget->header()->setSectionsMovable(false);
    ui->wsTreeWidget->setHeaderLabels(labels);
    ui->wsTreeWidget->setColumnWidth(0,100);
    ui->wsTreeWidget->setColumnWidth(1,100);
    ui->wsTreeWidget->setColumnWidth(2,300);

    m_popMenu = new QMenu(this);
    m_actionFolder = new QAction(tr("打开"),this);
    m_actionSet = new QAction(tr("设置"),this);
    m_actionInstall = new QAction(tr("安装"),this);
    m_actionUninstall = new QAction(tr("卸载"),this);
    m_actionOpenUrl = new QAction(tr("网址"),this);
    m_actionDetail = new QAction(tr("帮助"),this);

    m_popMenu->addAction(m_actionFolder);
    m_popMenu->addAction(m_actionSet);
    m_popMenu->addAction(m_actionInstall);
    m_popMenu->addAction(m_actionUninstall);
    m_popMenu->addAction(m_actionOpenUrl);
    m_popMenu->addAction(m_actionDetail);

    ui->progressBar->setValue(0);
    ui->progressBar->setTextVisible(false);
    ui->progressBar->setMinimum(0);
    ui->progressBar->setMaximum(0);
    ui->progressBar->hide();

    ui->keySequenceEdit->setKeySequence(QKeySequence(GetSettings()->m_hotKey));

    for (int i = 6,j=0; i <= 10; i++,j++)
    {
        ui->comBoxDisplayNum->insertItem(j,QString::number(i));
    }

    for (int i = 20,j=0;i <= 100; i+=10,j++)
    {
        ui->comboBoxMaxCount->insertItem(j,QString::number(i));
    }

    Settings* cfg = GetSettings();

    ui->chkBoxBootRun->setChecked(cfg->m_startOnSystemStartup);
    ui->chkBoxAutoHide->setChecked(cfg->m_hideWhenDeactive);
    ui->comBoxDisplayNum->setCurrentText(QString::number(cfg->m_maxResultsPerPage));
    ui->comboBoxMaxCount->setCurrentText(QString::number(cfg->m_maxResultsToShow));
    ui->chkRemPosition->setChecked(cfg->m_remLastPosition);
    ui->lineEditPythonPath->setText(cfg->m_pythonPath);
    ui->chkDisableMouse->setChecked(cfg->m_disableMouse);
    ui->labelUsage->setText(tr("你已经激活了EasyGo %1 次").arg(GetUsageSetting()->m_usage));
    ui->lineEditRepoUrl->setText(cfg->m_repo_url);

    ui->tabWidget->setCurrentIndex(0);

    connect(m_actionFolder,SIGNAL(triggered()),this,SLOT(sltOpenFolder()));
    connect(m_actionSet,SIGNAL(triggered()),this,SLOT(sltPluginSet()));
    connect(m_actionOpenUrl,SIGNAL(triggered()),this,SLOT(sltOpenUrl()));
    connect(m_actionInstall,SIGNAL(triggered()),this,SLOT(sltInstallPlugin()));
    connect(m_actionUninstall,SIGNAL(triggered()),this,SLOT(sltUinstallPlugin()));
    connect(m_actionDetail,SIGNAL(triggered()),this,SLOT(sltOpenDetail()));

    connect(ui->treeWidget,SIGNAL(customContextMenuRequested(const QPoint&)),
            this,SLOT(sltCustomContextMenuRequested(const QPoint&)));
    connect(ui->treeWidget,SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
            this,SLOT(sltItemDoubleClicked(QTreeWidgetItem*,int)));
    connect(ui->pushButtonView,SIGNAL(clicked()),this,SLOT(sltBrowserPath()));
    connect(ui->treeWidget,SIGNAL(itemChanged(QTreeWidgetItem*,int)),
            this,SLOT(sltItemChanged(QTreeWidgetItem*,int)));

    connect(ui->wsTreeWidget,SIGNAL(itemChanged(QTreeWidgetItem*,int)),
            this,SLOT(sltWsItemChanged(QTreeWidgetItem*,int)));

    connect(ui->pushButtonAdd,SIGNAL(clicked()),this,SLOT(sltPushButtonAdd()));
    connect(ui->pushButtonDelete,SIGNAL(clicked()),this,SLOT(sltPushButtonDelete()));
    connect(ui->pushButtonIndex,SIGNAL(clicked()),this,SLOT(sltPushButtonIndex()));

    connect(ui->pushButtonAddUrl,SIGNAL(clicked()),this,SLOT(sltPushButtonAddUrl()));
    connect(ui->pushButtonDelUrl,SIGNAL(clicked()),this,SLOT(sltPushButtonDelUrl()));

    connect(ui->pushButtonReset,SIGNAL(clicked()),this,SLOT(sltPushButtonReset()));

    connect(ui->pushButtonCheckUpdate, SIGNAL(clicked()), this, SLOT(sltPushButtonCheckUpdate()));

    LoadPlugin();
    LoadPath();
    LoadWebSearch();
}

SetDlg::~SetDlg()
{
    delete ui;
}

void SetDlg::LoadPlugin()
{
    ui->treeWidget->clear();

    Settings* setting = GetSettings();
    for (int i = 0; i < setting->m_pluginConfig.size(); i++)
    {
        PluginConfig& cfg = setting->m_pluginConfig[i];
        Plugin* info = GetPluginMananger()->getPlugin(cfg.id);
        if (!info)
        {
            continue;
        }

        QString iconPath = info->m_iconPath;

        QTreeWidgetItem *childItem = new QTreeWidgetItem;
        childItem->setSizeHint(0,QSize(32,32));
        childItem->setSizeHint(6,QSize(32,32));
        childItem->setSizeHint(7,QSize(32,32));

        childItem->setIcon(0,QIcon(iconPath));
        childItem->setText(1,info->m_info.name);
        childItem->setToolTip(1,info->m_info.name);
        childItem->setText(2,info->m_info.description);
        childItem->setToolTip(2,info->m_info.description);
        childItem->setText(3,info->m_info.author);
        childItem->setToolTip(3,info->m_info.author);
        childItem->setText(4,info->m_info.version);
        childItem->setText(5,cfg.keyword[0]);

        if (cfg.mode == RealMode)
        {
            childItem->setCheckState(6,Qt::Unchecked);
        }
        else
        {
            childItem->setCheckState(6,Qt::Checked);
        }

        if (cfg.disabled)
        {
            childItem->setCheckState(7,Qt::Checked);
        }
        else
        {
            childItem->setCheckState(7,Qt::Unchecked);
        }

        childItem->setData(0,Qt::UserRole,info->m_guid);
        childItem->setData(5,Qt::UserRole,info->m_info.keyword[0]);

        ui->treeWidget->insertTopLevelItem(0,childItem);
    }
}

void SetDlg::LoadPath()
{
    Settings* setting = GetSettings();
    ui->chkEnableNormalIndex->setChecked(setting->m_indexCfg.enableNormalIndex);
    ui->chkClearHistory->setChecked(setting->m_indexCfg.clearHistory);
    QStringList& indexPath = setting->m_indexCfg.indexPath;
    for (int i = 0; i < indexPath.size(); i++)
    {
        ui->listWidget->addItem(indexPath[i]);
    }
}

void SetDlg::LoadWebSearch()
{
    WebSearchCfg* cfg = GetWebSearchCfg();

    if (cfg->m_enableSuggest)
    {
        ui->chkBoxSuggest->setChecked(true);
    }

    QMapIterator<QString,SearchItem> mapIter(cfg->m_itemsMap);
    while (mapIter.hasNext())
    {
        mapIter.next();

        if (mapIter.value().url.isEmpty() || mapIter.value().keyword.isEmpty())
        {
            continue;
        }

        QTreeWidgetItem *childItem = new QTreeWidgetItem;
        childItem->setText(0,mapIter.value().title);
        childItem->setText(1,mapIter.value().keyword);
        childItem->setText(2,mapIter.value().url);
        childItem->setCheckState(3,mapIter.value().enabled?Qt::Checked:Qt::Unchecked);

        childItem->setData(1,Qt::UserRole,mapIter.value().keyword);

        Qt::ItemFlags tmp = childItem->flags();
        childItem->setFlags(tmp | Qt::ItemIsEditable);

        ui->wsTreeWidget->insertTopLevelItem(0,childItem);
    }
}

void SetDlg::AddPlugin(QString& pluginId)
{
    Plugin* info = GetPluginMananger()->getPlugin(pluginId);

    QString iconPath = info->m_iconPath;

    QTreeWidgetItem *childItem = new QTreeWidgetItem;
    childItem->setIcon(0,QIcon(iconPath));
    childItem->setText(1,info->m_info.name);
    childItem->setText(2,info->m_info.description);
    childItem->setText(3,info->m_info.author);
    childItem->setText(4,info->m_info.version);
    childItem->setText(5,info->m_info.keyword[0]);
    childItem->setCheckState(6,info->m_mode?Qt::Checked:Qt::Unchecked);
    childItem->setCheckState(7,Qt::Unchecked);

    childItem->setData(0,Qt::UserRole,info->m_guid);
    childItem->setData(5,Qt::UserRole,info->m_info.keyword[0]);

    ui->treeWidget->insertTopLevelItem(0,childItem);
}

void SetDlg::RemovePlugin(QString& pluginId)
{
    for (int i = 0; i < ui->treeWidget->topLevelItemCount();i++)
    {
        QTreeWidgetItem* item = ui->treeWidget->topLevelItem(i);
        QString id = item->data(0,Qt::UserRole).toString();
        if (id == pluginId)
        {
            ui->treeWidget->takeTopLevelItem(i);
            delete item;
        }
    }
}

void SetDlg::sltItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Qt::ItemFlags tmp = item->flags();
    if (column == 5) {
        item->setFlags(tmp | Qt::ItemIsEditable);
    } else if (tmp & Qt::ItemIsEditable) {
        item->setFlags(tmp ^ Qt::ItemIsEditable);
    }
}

void SetDlg::sltItemChanged(QTreeWidgetItem *item, int column)
{
    if (column == 5)
    {
        QString strKey = item->text(5);

        if (GetSettings()->isKeywordExsit(strKey) ||
            GetWebSearchCfg()->m_itemsMap.contains(strKey))
        {
            QMessageBox::information(this,tr("提示"),tr("关键字已被占用，请使用其他关键字"));
            QString orgKey = item->data(column,Qt::UserRole).toString();
            ui->treeWidget->blockSignals(true);
            item->setText(column,orgKey);
            ui->treeWidget->blockSignals(false);
        }
        else
        {
            QString strId = item->data(0,Qt::UserRole).toString();
            int index = GetSettings()->find(strId);
            ui->wsTreeWidget->blockSignals(true);
            item->setData(column,Qt::UserRole + 1,0);
            ui->wsTreeWidget->blockSignals(false);
            GetSettings()->m_pluginConfig[index].keyword[0] = strKey;
        }
    }
    else if (column == 6)
    {
        QString strId = item->data(0,Qt::UserRole).toString();
        int index = GetSettings()->find(strId);
        GetSettings()->m_pluginConfig[index].mode =
                item->checkState(column) == Qt::Checked ? EnterMode:RealMode;
    }
    else if (column == 7)
    {
        QString strId = item->data(0,Qt::UserRole).toString();
        int index = GetSettings()->find(strId);
        GetSettings()->m_pluginConfig[index].disabled =
                item->checkState(column) == Qt::Checked ? true:false;
    }
}

void SetDlg::sltWsItemChanged(QTreeWidgetItem *item, int column)
{
    QString key = item->text(1);
    if (column == 1)
    {
        QString orgKey = item->data(1,Qt::UserRole).toString();

        if (GetSettings()->isKeywordExsit(key) ||
            GetWebSearchCfg()->m_itemsMap.contains(key))
        {
            QMessageBox::information(this,tr("提示"),tr("关键字已被占用，请使用其他关键字"));
            ui->wsTreeWidget->blockSignals(true);
            item->setText(column,orgKey);
            ui->wsTreeWidget->blockSignals(false);
            return;
        }

        WebSearchPlugin* plugin = (WebSearchPlugin*)GetPluginMananger()->getPlugin(WEBSEARCH_PLUGIN_ID);
        if (plugin)
        {
            SearchItem sItem = GetWebSearchCfg()->m_itemsMap[orgKey];
            plugin->delSource(orgKey);
            sItem.keyword = key;
            plugin->addSource(sItem);
        }

        ui->wsTreeWidget->blockSignals(true);
        item->setData(column,Qt::UserRole,key);
        ui->wsTreeWidget->blockSignals(false);
    }
    else
    {
        if (column == 0)
            GetWebSearchCfg()->m_itemsMap[key].title = item->text(column);
        else if(column == 2)
            GetWebSearchCfg()->m_itemsMap[key].url = item->text(column);
        else if (column == 3)
            GetWebSearchCfg()->m_itemsMap[key].enabled = item->checkState(column) == Qt::Checked ? true:false;
    }
}

void SetDlg::sltBrowserPath()
{
    QFileDialog dialog(0,tr("选择Python3文件夹"),
                                   QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
                                   QObject::tr("支持类型 (*.*)"));
    dialog.setViewMode(QFileDialog::Detail);
    dialog.setFileMode(QFileDialog::DirectoryOnly);
    if(dialog.exec())
    {
        QString filePath = dialog.selectedFiles()[0];
        ui->lineEditPythonPath->setText(filePath);
        QMessageBox::information(this,tr("提示"),tr("请重启 EasyGo 生效!"));
    }
}

void SetDlg::sltCustomContextMenuRequested(const QPoint &pos)
{
    QTreeWidgetItem  *curItem =  ui->treeWidget->itemAt(pos);
    if (curItem == NULL)
    {
        QMenu menu;
        menu.addAction(m_actionInstall);
        menu.exec(QCursor::pos());
        return;
    }

    QString pluginId = curItem->data(0,Qt::UserRole).toString();
    QString webPath = GetPluginMananger()->getPlugin(pluginId)->m_info.webSite;
    QString cfgPath = GetPluginMananger()->getPlugin(pluginId)->m_cfgPath;
    QString pluginPath = GetPluginMananger()->getPlugin(pluginId)->m_path;
    QString info = pluginPath + QString("/readme.txt");

    if (webPath.isEmpty())
    {
        m_actionOpenUrl->setEnabled(false);
    }
    else
    {
        m_actionOpenUrl->setEnabled(true);
    }

    if (cfgPath.isEmpty())
    {
        m_actionSet->setEnabled(false);
    }
    else
    {
        if (QFile::exists(cfgPath))
        {
            m_actionSet->setEnabled(true);
        }
        else
        {
            m_actionSet->setEnabled(false);
        }
    }

    if (QFile::exists(info))
    {
        m_actionDetail->setEnabled(true);
    }
    else
    {
        m_actionDetail->setEnabled(false);
    }

    m_popMenu->exec(QCursor::pos());
}

void SetDlg::sltOpenFolder()
{
    QTreeWidgetItem *curItem = ui->treeWidget->currentItem();
    if (!curItem)
    {
        return;
    }

    QString pluginId = curItem->data(0,Qt::UserRole).toString();
    QString pluginPath = GetPluginMananger()->getPlugin(pluginId)->m_path;

    QProcess process;
    QString cmdLine = QString("explorer %1").arg(pluginPath);
    cmdLine = cmdLine.replace("/","\\");
    process.startDetached(cmdLine);
}

void SetDlg::sltInstallPlugin()
{
    QFileDialog dialog(0,tr("选择插件"),
                                   QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
                                   QObject::tr("支持类型 (*.Plugin)"));
                //dialog.setOption(QFileDialog::DontUseNativeDialog);
    dialog.setViewMode(QFileDialog::Detail);
    dialog.setFileMode(QFileDialog::ExistingFile);
    if(dialog.exec())
    {
        QString filePath = dialog.selectedFiles()[0];
        filePath = filePath.replace("/","\\");

        QuaZip zip(filePath);
        if (!zip.open(QuaZip::mdUnzip))
        {
            QMessageBox::information(this,tr("提示"),tr("打开插件失败"));
            return;
        }

        zip.setCurrentFile("plugin.json");

        QuaZipFile zipfile(&zip);
        if (!zipfile.open(QIODevice::ReadOnly))
        {
            zip.close();
            QMessageBox::information(this,tr("提示"),tr("找不到插件信息"));
            return;
        }

        QByteArray pluginContent = zipfile.readAll();

        zipfile.close();
        zip.close();

        QJsonParseError parseJsonErr;
        QJsonDocument document = QJsonDocument::fromJson(pluginContent, &parseJsonErr);
        if (parseJsonErr.error != QJsonParseError::NoError)
        {
            QMessageBox::information(this,tr("提示"),tr("插件格式不正确"));
            return;
        }

        QJsonObject jsonObject = document.object();
        QString pluginName = jsonObject["Name"].toString();
        QString pluginId = jsonObject["ID"].toString();
        QString pluginType = jsonObject["PluginType"].toString();
        QString pluginPath = GetPluginMananger()->getPluginsPath() +
                QString("/%1").arg(pluginName);
        QString keyword = jsonObject["Keyword"].toString();
        QString pluginVersion = jsonObject["Version"].toString();

        if (pluginType == "python")
        {
            if (ui->lineEditPythonPath->text().isEmpty())
            {
                QMessageBox::information(this,tr("提示"),tr("该插件需要依赖python环境，请设置后安装"));
                return;
            }
        }

        InstallDlg dlg(this);
        QDir dir;
        Plugin* plugin = GetPluginMananger()->getPlugin(pluginId);
        if (plugin)
        {
            int iRet = CompareVersion(plugin->m_info.version, pluginVersion);
            if (iRet > 0)
            {
                dlg.setMsg(tr("已安装更新版本，是否覆盖降级？"));
                if (QDialog::Rejected == dlg.exec())
                {
                    return;
                }
            }
            else if (iRet < 0)
            {
                dlg.setMsg(tr("已安装老版本，是否覆盖升级？"));
                if (QDialog::Rejected == dlg.exec())
                {
                    return;
                }
            }
            else
            {
                dlg.setMsg(tr("已安装同版本，是否覆盖？"));
                if (QDialog::Rejected == dlg.exec())
                {
                    return;
                }
            }

            if (!plugin->m_cfgPath.isEmpty() && !dlg.m_bReplaceCfg)
            {
                QFile::rename(plugin->m_cfgPath, plugin->m_cfgPath + ".bak");
            }

            if (plugin->m_type == PLUGIN_CPP)
            {
                CPlusPlugin* c_plugin = (CPlusPlugin*)plugin;
                c_plugin->dettach();
            }
            else if (plugin->m_type == PLUGIN_E)
            {
                EPlugin* e_plugin = (EPlugin*)plugin;
                e_plugin->dettach();
            }

            RemovePlugin(pluginId);
        }
        else
        {
            if (dir.exists(pluginPath))
            {
                pluginPath = pluginPath + QString("-%1").arg(pluginId);
            }

            if (!dir.mkdir(pluginPath))
            {
                QMessageBox::information(this,tr("提示"),tr("无法创建插件目录"));
                return;
            }
        }

        QStringList extractFiles = JlCompress::extractDir(filePath,pluginPath);
        if (extractFiles.isEmpty())
        {
            QMessageBox::information(this,tr("提示"),tr("解压插件失败"));
            return;
        }

        if (plugin && !dlg.m_bReplaceCfg && !plugin->m_cfgPath.isEmpty())
        {
            QFile::remove(plugin->m_cfgPath);
            QFile::rename(plugin->m_cfgPath+".bak", plugin->m_cfgPath);
        }

        if (!GetPluginMananger()->addPlugin(pluginPath))
        {
            dir.setPath(pluginPath);
            dir.removeRecursively();
            QMessageBox::information(this,tr("提示"),tr("安装插件失败"));
            return;
        }

        plugin = GetPluginMananger()->getPlugin(pluginId);
        if (plugin->m_type == PLUGIN_PYTHON)
        {
            QString requirePath = plugin->m_path + QString("/requirements.txt");
            QFile file(requirePath);
            if (file.exists())
            {
                InstallTask task;

                QProgressDialog dlg(this);
                dlg.resize(300,200);
                dlg.setWindowTitle(tr("安装"));
                dlg.setLabelText(tr("安装插件依赖中,请不要关闭对话框..."));
                dlg.setRange(0,0);
                dlg.setMinimumDuration(0);
                dlg.setCancelButton(0);

                task.m_pythonPath = ui->lineEditPythonPath->text();
                task.m_requirePath = requirePath;
                connect(&task,SIGNAL(finished()),&dlg,SLOT(accept()));
                task.start();

                if (QDialog::Rejected == dlg.exec())
                {
                    task.terminate();
                    task.m_exitCode = 1;
                }

                AddPlugin(pluginId);

                if (task.m_exitCode == 0)
                {
                    plugin->m_initok = true;
                    if (plugin->m_info.keyword[0].isEmpty() && plugin->m_info.acceptType.isEmpty())
                    {
                        QMessageBox::information(this,tr("提示"),tr("安装插件成功，请设置关键字后使用！"));
                    }
                    else
                    {
                        QMessageBox::information(this,tr("提示"),tr("安装插件成功！"));
                    }
                }
                else
                {
                    QMessageBox::information(this,tr("提示"),tr("安装插件成功，但是依赖安装失败，请手工安装相关依赖，确保插件正常运行"));
                }

                return;
            }
        }

        AddPlugin(pluginId);

        if (plugin->m_info.keyword[0].isEmpty() && plugin->m_info.acceptType.isEmpty())
        {
            QMessageBox::information(this,tr("提示"),tr("安装插件成功，请设置关键字后使用！"));
        }
        else
        {
            QMessageBox::information(this,tr("提示"),tr("安装插件成功！"));
        }
    }
}

void SetDlg::sltUinstallPlugin()
{
    QTreeWidgetItem *curItem = ui->treeWidget->currentItem();
    if (!curItem)
    {
        return;
    }

    QString pluginId = curItem->data(0,Qt::UserRole).toString();
    int index = ui->treeWidget->indexOfTopLevelItem(curItem);

    QTreeWidgetItem* item = ui->treeWidget->takeTopLevelItem(index);
    if (!item)
    {
        QMessageBox::critical(this,tr("提示"),tr("卸载失败"));
        return;
    }

    delete item;

    if (GetPluginMananger()->deletePlugin(pluginId))
    {
        QMessageBox::information(this,tr("提示"),tr("卸载成功"));
    }
    else
    {
        QMessageBox::critical(this,tr("提示"),tr("卸载失败"));
    }
}

void SetDlg::sltOpenUrl()
{
    QTreeWidgetItem *curItem = ui->treeWidget->currentItem();
    if (!curItem)
    {
        return;
    }

    QString pluginId = curItem->data(0,Qt::UserRole).toString();
    QString webPath = GetPluginMananger()->getPlugin(pluginId)->m_info.webSite;
    if (webPath.isEmpty())
    {
        QMessageBox::information(this,tr("提示"),tr("该插件无网址信息"));
        return;
    }

    QProcess process;
    QString cmdLine = QString("explorer %1").arg(webPath);
    cmdLine = cmdLine.replace("/","\\");
    process.startDetached(cmdLine);
}

void SetDlg::sltOpenDetail()
{
    QTreeWidgetItem *curItem = ui->treeWidget->currentItem();
    if (!curItem)
    {
        return;
    }

    QString pluginId = curItem->data(0,Qt::UserRole).toString();
    QString pluginPath = GetPluginMananger()->getPlugin(pluginId)->m_path;
    QString info = pluginPath + QString("/readme.txt");

    ShowContentDlg* dlg = new ShowContentDlg(info, false, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose,true);
    dlg->setStyleSheet("background-color: rgb(255,255,255);");
    dlg->setReadOnly(false);
    dlg->setWindowTitle("帮助");
    dlg->show();
}

void SetDlg::sltPluginSet()
{
    QTreeWidgetItem *curItem = ui->treeWidget->currentItem();
    if (!curItem)
    {
        return;
    }

    QString pluginId = curItem->data(0,Qt::UserRole).toString();
    QString cfgPath = GetPluginMananger()->getPlugin(pluginId)->m_cfgPath;

    PluginSetDlg pluginSetDlg(cfgPath,this);
    pluginSetDlg.exec();
    GetPluginMananger()->getPlugin(pluginId)->updateSetting();
}

void SetDlg::closeEvent(QCloseEvent *event)
{
    if (m_task && m_task->isRunning())
    {
        QMessageBox::information(this,tr("提示"),tr("正在索引中，请稍后关闭..."));
        event->ignore();
        return;
    }

    GetSettings()->m_startOnSystemStartup = ui->chkBoxBootRun->isChecked();
    GetSettings()->m_hideWhenDeactive = ui->chkBoxAutoHide->isChecked();
    GetSettings()->m_maxResultsToShow = ui->comboBoxMaxCount->currentText().toInt();
    GetSettings()->m_maxResultsPerPage = ui->comBoxDisplayNum->currentText().toInt();
    GetSettings()->m_remLastPosition = ui->chkRemPosition->isChecked();
    GetSettings()->m_pythonPath = ui->lineEditPythonPath->text();
    GetSettings()->m_hotKey = ui->keySequenceEdit->keySequence().toString();
    GetSettings()->m_disableMouse = ui->chkDisableMouse->isChecked();
    GetSettings()->m_indexCfg.enableNormalIndex = ui->chkEnableNormalIndex->isChecked();
    GetSettings()->m_indexCfg.clearHistory = ui->chkClearHistory->isChecked();
    GetSettings()->m_checkUpdate = ui->chkBoxCheckUpdate->isChecked();
    GetSettings()->m_repo_url = ui->lineEditRepoUrl->text();
    GetSettings()->save();

    GetWebSearchCfg()->m_enableSuggest = ui->chkBoxSuggest->isChecked();
    GetWebSearchCfg()->save();

    if (ui->chkBoxBootRun->isChecked())
    {
        QSettings regSet("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",QSettings::NativeFormat);
        QString value_get = regSet.value("EasyGo").toString();

        QString appPath = QApplication::applicationFilePath();
        appPath = QDir::toNativeSeparators(appPath);
        QString value_set = QString("\"%1\"").arg(appPath);

        if (value_get.isEmpty() || value_set != value_get)
        {
            regSet.setValue("EasyGo",value_set);
        }
    }
}

void SetDlg::sltPushButtonAdd()
{
    QFileDialog dialog(0,tr("选择磁盘分区"),
                                   QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
                                   QObject::tr("支持类型 (*.*)"));
                //dialog.setOption(QFileDialog::DontUseNativeDialog);
    dialog.setViewMode(QFileDialog::Detail);
    dialog.setFileMode(QFileDialog::Directory);

    if(dialog.exec())
    {
        QString filePath = dialog.selectedFiles()[0];
        QFileInfo fileInfo(filePath);
        if (!fileInfo.isRoot())
        {
            QMessageBox::information(this,tr("提示"),tr("请选择磁盘分区"));
            return;
        }

        QStringList& pathList = GetSettings()->m_indexCfg.indexPath;
        if (pathList.contains(filePath))
        {
            QMessageBox::information(this,tr("提示"),tr("路径已存在"));
        }
        else
        {
            pathList.append(filePath);
            ui->listWidget->addItem(filePath);
        }
    }
}

void SetDlg::sltPushButtonDelete()
{
    int index = ui->listWidget->currentRow();
    if (index < 0)
    {
        QMessageBox::information(this,tr("提示"),tr("请选择一个路径删除"));
        return;
    }

    QListWidgetItem* item = ui->listWidget->takeItem(index);
    if (!item)
    {
        return;
    }

    GetSettings()->m_indexCfg.indexPath.removeAll(item->text());

    delete item;
}

void SetDlg::sltPushButtonIndex()
{
    if (ui->pushButtonAdd->isEnabled())
    {
        ui->pushButtonIndex->setText(tr("停止"));
        ui->pushButtonAdd->setEnabled(false);
        ui->pushButtonDelete->setEnabled(false);
        ui->progressBar->show();

        GetSettings()->m_indexCfg.enableNormalIndex = ui->chkEnableNormalIndex->isChecked();

        m_task = new IndexTask();
        connect(m_task,SIGNAL(finished()),this,SLOT(sltTaskFinished()));
        m_task->index();
    }
    else
    {
        m_task->stop();
    }
}

void SetDlg::sltTaskFinished()
{
    ui->progressBar->hide();
    ui->pushButtonIndex->setText(tr("索引"));
    ui->pushButtonAdd->setEnabled(true);
    ui->pushButtonDelete->setEnabled(true);

    delete m_task;
    m_task = nullptr;
}

void SetDlg::sltPushButtonAddUrl()
{
    WebSearchSetDlg dlg(this);
    if (dlg.exec() == QDialog::Accepted)
    {
        SearchItem item;

        item.title = dlg.m_title;
        item.url = dlg.m_url;
        item.iconPath = dlg.m_iconPath;
        item.keyword = dlg.m_keyword;
        item.enabled = false;

        QTreeWidgetItem *childItem = new QTreeWidgetItem;
        childItem->setText(0,item.title);
        childItem->setText(1,item.keyword);
        childItem->setText(2,item.url);
        childItem->setCheckState(3,item.enabled?Qt::Checked:Qt::Unchecked);

        Qt::ItemFlags tmp = childItem->flags();
        childItem->setFlags(tmp | Qt::ItemIsEditable);

        ui->wsTreeWidget->insertTopLevelItem(0,childItem);

        WebSearchPlugin* plugin = (WebSearchPlugin*)GetPluginMananger()->getPlugin(WEBSEARCH_PLUGIN_ID);
        if (plugin)
        {
            plugin->addSource(item);
        }
    }
}

void SetDlg::sltPushButtonDelUrl()
{
    QTreeWidgetItem *curItem = ui->wsTreeWidget->currentItem();
    if (!curItem)
    {
        return;
    }

    WebSearchPlugin* plugin = (WebSearchPlugin*)GetPluginMananger()->getPlugin(WEBSEARCH_PLUGIN_ID);
    if (plugin)
    {
        plugin->delSource(curItem->text(1));
    }

    int index = ui->wsTreeWidget->indexOfTopLevelItem(curItem);
    QTreeWidgetItem* item = ui->wsTreeWidget->takeTopLevelItem(index);
    if (!item)
    {
        return;
    }

    delete item;
}

void SetDlg::sltPushButtonReset()
{
    ui->keySequenceEdit->setKeySequence(QKeySequence("Alt+Space"));
}

void SetDlg::sltPushButtonCheckUpdate()
{
    QString newVersion;
    if (CheckUpdate(newVersion))
    {
        QString currentVersion(EASYGO_VERSION);
        if (CompareVersion(newVersion, currentVersion) > 0)
        {
            QMessageBox::information(this, tr("更新提示"),
                    tr("有新版本发布，版本: %1\n请前往关于中的网址更新").arg(newVersion));
        }
        else
        {
            QMessageBox::information(this, tr("提示"), tr("你已经是最新版本！"));
        }
    }
    else
    {
        QMessageBox::warning(this, tr("提示"), tr("无法获取版本信息，请检查网络！"));
    }
}
