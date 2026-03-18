#include "MainDialog.h"
#include "ui_MainDialog.h"
#include "ResultItem.h"
#include <QLibrary>
#include <QMouseEvent>
#include <QMenu>
#include "PluginManager.h"
#include "Settings.h"
#include "LogFile.h"
#include "ThemeSetting.h"
#include <QMessageBox>
#include "AboutDialog.h"
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QUrl>
#include <QTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProgressDialog>
#include <QDir>
#include <QScrollBar>
#include "TaskManager.h"
#include "UsageSetting.h"
#include "quazip.h"
#include "quazipfile.h"
#include "JlCompress.h"
#include "HelperFunc.h"
#include "InstallDlg.h"
#include <QBitmap>
#include <QtConcurrent>
#include <QScreen>
#include <QClipboard>
#include "MainPluginDialog.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
# pragma execution_character_set("utf-8")
#endif

extern QObject* g_fromMainDlg;

MainDialog::MainDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MainDialog),
    m_taskId(0),
    m_curClickedItem(nullptr),
    m_forceQuery(false)
{
    ui->setupUi(this);

#ifdef QT_NO_DEBUG
    setWindowFlags(Qt::FramelessWindowHint|Qt::Dialog|Qt::Popup|Qt::WindowStaysOnTopHint);
#else
    setWindowFlags(Qt::FramelessWindowHint|Qt::Dialog|Qt::Popup);
#endif

    setAttribute(Qt::WA_TranslucentBackground);

    g_fromMainDlg = this;

    m_inputState.primaryMode = NormalInput;
    m_inputState.isSecondary = false;
    m_inputState.additionalData = "";

    m_curThemeType = GetThemeSetting()->getSelectTheme()->type;
    m_roundCorner = GetThemeSetting()->m_roundCorner;

    QString pythonPath = QApplication::applicationDirPath() + QString("/Python");
    if (QFile::exists(pythonPath) && GetSettings()->m_pythonPath.isEmpty())
    {
        GetSettings()->m_pythonPath = pythonPath;
        GetSettings()->save();
    }

    GetPluginMananger();

    QScreen* primary = QApplication::primaryScreen();
    int iwidth = primary->geometry().width();
    int iheight = primary->geometry().height();
    m_width = iwidth/5*2;

    m_initHeight = 70;

    setGeometry(0,0,m_width, m_initHeight);
    setAcceptDrops(true);

    if (GetSettings()->m_remLastPosition)
    {
        move(GetSettings()->m_x,GetSettings()->m_y);
    }
    else
    {
        move ((iwidth - width())/2,(iheight - height())/10);
    }

    connect(this,SIGNAL(resultReady(QVariant)),this,SLOT(sltResultReady(QVariant)));

    //托盘
    m_pTrayIcon = new QSystemTrayIcon(this);
    m_pTrayIcon->setIcon(QIcon(":/Images/app.ico"));
    m_pTrayIcon->setToolTip(tr("EasyGo快捷启动"));

    m_open = new QAction(tr("打开"),this);
    connect(m_open, SIGNAL(triggered()), this,SLOT(sltOpen()));
    m_setting = new QAction(tr("设置"),this);
    connect(m_setting, SIGNAL(triggered()), this,SLOT(sltSet()));
    m_reload = new QAction(tr("重载"),this);
    connect(m_reload,SIGNAL(triggered()),this,SLOT(sltReload()));
    m_about = new QAction(tr("关于"),this);
    connect(m_about, SIGNAL(triggered()), this,SLOT(sltAbout()));
    m_exit = new QAction(tr("退出"),this);
    connect(m_exit, SIGNAL(triggered()), this,SLOT(sltExit()));

    QMenu* trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(m_open);
    trayIconMenu->addAction(m_setting);
    trayIconMenu->addAction(m_reload);
    trayIconMenu->addAction(m_about);
    trayIconMenu->addAction(m_exit);
    m_pTrayIcon->setContextMenu(trayIconMenu);

    connect(m_pTrayIcon,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this,SLOT(sltTrayActived(QSystemTrayIcon::ActivationReason)));


    //输入框
    m_lineEdit = new MyLineEdit(this);
    m_lineEdit->setGeometry(5,5,m_width - 10, 60);
    m_lineEdit->setFrame(false);
    m_lineEdit->setContextMenuPolicy(Qt::NoContextMenu);
    m_lineEdit->setFont(QFont(tr("Arial"),18,QFont::Bold));

    connect(m_lineEdit,SIGNAL(textChanged(QString)),this,SLOT(sltTextChanged(QString)));
    connect(m_lineEdit,SIGNAL(clicked()),this,SLOT(sltLineEditClick()));


    //进度条
    m_progressbar = new QProgressBar(this);
    m_progressbar->setGeometry(5,66,m_width - 10,2);
    m_progressbar->setValue(0);
    m_progressbar->setTextVisible(false);
    m_progressbar->setMinimum(0);
    m_progressbar->setMaximum(0);
    m_progressbar->hide();

    //列表框
    m_listWidget = new MyListWidget(this);
    m_listWidget->setGeometry(5,70,m_width - 10, 60);
    m_listWidget->setFrameShape(QFrame::NoFrame);
    m_listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listWidget->setProperty("showDropIndicator", QVariant(false));
    m_listWidget->setUniformItemSizes(true);
    m_listWidget->hide();

    m_textEdit = new QTextEdit(this);
    m_textEdit->setGeometry(5,70,m_width - 10, GetSettings()->m_maxResultsPerPage * 60);
    m_textEdit->setFrameShape(QFrame::NoFrame);
    m_textEdit->setFrameShadow(QFrame::Plain);
    m_textEdit->setReadOnly(true);
    m_textEdit->setFont(QFont(tr("Arial"),13,QFont::Normal));
    m_textEdit->hide();
    createTextEditMenu();

    if (!GetSettings()->m_disableMouse)
    {
        connect(m_listWidget,SIGNAL(sigMouseRight()),this,SLOT(sltRightOpr()));
        connect(m_listWidget,SIGNAL(sigMouseLeft()),this,SLOT(sltLeftOpr()));
    }

    m_notifyMgr = new NotifyManager(this);
    m_notifyMgr->setMaxCount(5);
    m_notifyMgr->setDisplayTime(5000);
    m_notifyMgr->setNotifyWndSize(300, 100);

    for (int i = 0; i < GetSettings()->m_maxResultsToShow; i++)
    {
        Result result;
        ResultItem* myitem = new ResultItem(this, &result);
        myitem->setProperty("play_state", QMediaPlayer::StoppedState);
        m_listWidget->addItemWidget(myitem, true);
    }

    setTheme();

    m_task = GetTaskManager()->getIdleTask();
    connect(m_task,SIGNAL(resultReady(QVariant)),this,SLOT(sltResultReady(QVariant)));
    connect(m_task,SIGNAL(resultError()),this,SLOT(sltResultError()));
    connect(m_task,SIGNAL(cancel()),this,SLOT(sltCancelOpr()));
    m_task->m_id = m_taskId;

    m_typingTimer = new QTimer( this );
    m_typingTimer->setSingleShot( true );
    connect(m_typingTimer, SIGNAL(timeout()),this, SLOT(sltFilterEntries()));

    connect(&m_iconExtractor,SIGNAL(iconExtracted(int,QPixmap)),this,SLOT(sltIconExtracted(int,QPixmap)));
    m_shortcut = new QxtGlobalShortcut(QKeySequence(),this);
    m_hotKey = GetSettings()->m_hotKey;
    if (m_shortcut->setShortcut(QKeySequence(m_hotKey)))
    {
        connect(m_shortcut,SIGNAL(activated()),this,SLOT(sltHotKey()));
    }

    m_checkActive = new QTimer( this );
    connect(m_checkActive, SIGNAL(timeout()),this, SLOT(sltCheckAtivate()));
    if (GetSettings()->m_hideWhenDeactive)
    {
        m_checkActive->start(1000);
    }

    connect(&m_mediaPlayer,SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)),
            this,SLOT(sltMediaStatusChanged(QMediaPlayer::MediaStatus)));

    m_lineEdit->installEventFilter(this);
    m_listWidget->installEventFilter(this);
    m_textEdit->installEventFilter(this);

    // 初始化大小改变动画
    m_resizeAnimation = new QPropertyAnimation(this, "size");
    m_resizeAnimation->setEasingCurve(QEasingCurve::OutCubic);
    m_resizeAnimation->setDuration(200); // 200ms动画时长

    m_pTrayIcon->show();

    if (GetSettings()->m_checkUpdate)
        QTimer::singleShot(5000, this, SLOT(sltCheckUpdate()));
}

MainDialog::~MainDialog()
{
    delete ui;
}

void MainDialog::sltCheckUpdate()
{
    QString newVersion;
    if (CheckUpdate(newVersion))
    {
        QString currentVersion(EASYGO_VERSION);
        if (CompareVersion(newVersion, currentVersion) > 0)
        {
            showTip(tr("更新提示"),
                    tr("有新版本发布，版本: %1\n请前往关于中的网址更新").arg(newVersion));
        }
    }
}

void MainDialog::setQueryText(QString queryText)
{
    if (m_inputState.primaryMode == MenuInput)
    {
        m_lineEdit->setEnabled(true);
        m_lineEdit->setFocus();

        resetInputState();
    }

    m_forceQuery = true;

    if (m_lineEdit->text() == queryText)
    {
        sltTextChanged(queryText);
    }
    else
    {
        m_lineEdit->setText(queryText);
    }

    m_lineEdit->setFocus();
}

void MainDialog::hideResultUI()
{
    m_listWidget->hide();
    m_listWidget->hideAll();
    m_listWidget->setCurrentRow(-1);

    m_textEdit->clear();
    m_textEdit->hide();

    m_progressbar->hide();
}

void MainDialog::resetInputState()
{
    m_inputState.primaryMode = NormalInput;
    m_inputState.isSecondary = false;
    m_inputState.additionalData.clear();
    m_inputState.additionalQuery = Query();
}

void MainDialog::sltTrayActived(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick)
    {
        sltHotKey();
    }
}

void MainDialog::sltCheckAtivate()
{
    if (!this->isActiveWindow())
    {
        this->hide();
    }
}

void MainDialog::sltFilterEntries()
{
    if (m_lineEdit->text().isEmpty())
    {
        resetInputState();
        return;
    }

    //全部是空格
    if (m_filterText.count(" ") == m_filterText.size())
    {
        resetInputState();
        return;
    }

    m_filterText = m_filterText.remove(QRegExp("^ +\\s*"));

    Query query;
    QStringList split;
    Plugin* plugin = NULL;
    if (m_inputState.primaryMode == DropInput)
    {
        if (m_inputState.isSecondary)
        {
            query.rawQuery = m_filterText;
            query.keyword = m_inputState.additionalQuery.keyword;
            split = m_filterText.split(m_lineEdit->separatorText());
            query.parameter = QString("%1 %2").arg(m_inputState.additionalQuery.parameter).
                    arg(split[1]);
            plugin = GetPluginMananger()->getPlugin(m_inputState.additionalData);
            if (!plugin) return;
        }
        else
        {
            query.rawQuery = m_filterText;
            query.keyword = m_inputState.additionalQuery.keyword;
            query.parameter = m_inputState.additionalQuery.parameter;
        }
    }
    else
    {
        bool pluginMode = true;
        if (m_inputState.isSecondary)
        {
            query.rawQuery = m_filterText;
            split = m_filterText.split(m_lineEdit->separatorText());
            query.keyword = m_inputState.additionalData;
            query.parameter = split[1];
        }
        else
        {
            query.rawQuery = m_filterText;
            query.keyword = query.rawQuery;
            int index = m_filterText.indexOf(" ");
            if (index != -1)
            {
                query.keyword = m_filterText.mid(0,index);
                query.parameter = m_filterText.mid(index+1);
            }
            else
            {
                if (query.keyword != "epm" && query.keyword != "/")
                    pluginMode = false;
            }
        }

        if (pluginMode)
        {
            plugin = GetPluginMananger()->getValidPlugin(query.keyword);
        }
        else
        {
            plugin = GetPluginMananger()->getValidPlugin("*");
        }
        if (!plugin) return;

        int index = GetSettings()->find(plugin->m_guid);
        if (index != -1)
        {
            if (GetSettings()->m_pluginConfig[index].mode)
            {
                m_inputState.primaryMode = EnterInput;
            }
            else
            {
                m_inputState.primaryMode = NormalInput;
            }
        }
        else
        {
            if (plugin->m_mode == EnterMode)
            {
                m_inputState.primaryMode = EnterInput;
            }
            else
            {
                m_inputState.primaryMode = NormalInput;
            }
        }
    }

    m_itemNum = -1;

    if (m_task->isRunning())
    {
        m_task = GetTaskManager()->getIdleTask();
        connect(m_task,SIGNAL(resultReady(QVariant)),this,SLOT(sltResultReady(QVariant)));
        connect(m_task,SIGNAL(resultError()),this,SLOT(sltResultError()));
        connect(m_task,SIGNAL(cancel()),this,SLOT(sltCancelOpr()));
        m_task->m_id = ++m_taskId;
    }

    if (m_inputState.primaryMode != EnterInput || m_forceQuery ||
         query.parameter.split(' ',Qt::SkipEmptyParts).size() < plugin->m_info.argc)
    {
        m_progressbar->show();
        if (m_inputState.primaryMode == DropInput && !m_inputState.isSecondary)
        {
            m_task->setQuery(nullptr, query);
            m_task->query(true);
        }
        else
        {
            m_task->query(plugin,query);
        }
    }
    else
    {
        m_task->setQuery(plugin,query);
    }

    m_forceQuery = false;
}

void MainDialog::sltTextChanged(const QString &inputStr)
{
    if (m_mediaPlayer.state() == QMediaPlayer::PlayingState ||
        m_mediaPlayer.state() == QMediaPlayer::PausedState)
    {
        stopMusic();
    }

    if (m_lineEdit->text().isEmpty())
    {
        m_task->stop();
        resetInputState();
        animateResize(m_initHeight);
        hideResultUI();
        return;
    }

    if (m_inputState.primaryMode == DropInput)
    {
        QFileInfo info(m_inputState.additionalQuery.parameter);
        if (!m_lineEdit->text().contains(m_lineEdit->separatorText()))
        {
            m_inputState.isSecondary = false;
            m_inputState.additionalData.clear();
        }

        if (!m_lineEdit->text().contains(info.fileName()))
        {
            m_inputState.primaryMode = NormalInput;
            m_inputState.additionalQuery = Query();
        }
    }
    else if (m_inputState.primaryMode == NormalInput)
    {
        if (!m_lineEdit->text().contains(m_lineEdit->separatorText()))
        {
            m_inputState.isSecondary = false;
            m_inputState.additionalData.clear();
        }
    }

    m_filterText = inputStr;
    m_typingTimer->start( 200 );
}

void MainDialog::sltIconExtracted(int itemIndex, QPixmap icon)
{
    ResultItem* myitem = m_listWidget->getItem(itemIndex);
    if (myitem->m_result.showType == SHOW_TYPE_MUSIC)
    {
        QMediaPlayer::State state;
        state = (QMediaPlayer::State)myitem->property("play_state").toUInt();
        if (state == QMediaPlayer::PlayingState)
        {
            myitem->updateIcon(m_playPixmap);
        }
        else if(state == QMediaPlayer::PausedState)
        {
            myitem->updateIcon(m_pausePixmap);
        }
        else
        {
            myitem->updateIcon(icon);
        }
    }
    else
    {
        myitem->updateIcon(icon);
    }
}

void MainDialog::sltResultReady(QVariant rInfo)
{
    TaskResult taskResult = rInfo.value<TaskResult>();
    QVector<Result>& resultList = taskResult.vecResult;
    
    if (m_inputState.primaryMode != MenuInput) 
    {
        if (m_inputState.primaryMode != EnterInput && taskResult.id != m_taskId)
        {
            return;
        }

        m_taskResult.vecResult.clear();
        m_taskResult.id = m_taskId;
        m_taskResult.vecResult = resultList;
    }

    if (resultList[0].showType == SHOW_TYPE_RICHTEXT)
    {
        m_textEdit->clear();

        QStringList contentList = SplitContent(resultList[0].subTitle);
        QTextCursor cursor = m_textEdit->textCursor();
        foreach(QString content, contentList)
        {
            if (content.startsWith("[pic="))
            {
                QString picPath = content.mid(5, content.length() - 6);
                if (picPath.startsWith("http"))
                {
                    QDateTime time = QDateTime::currentDateTime();
                    int timeT = time.toTime_t();
                    QString dstFile = QDir::tempPath() + "/" + QString::number(timeT);
                    QFuture<bool> fut = QtConcurrent::run(HttpDownload, picPath, dstFile, false);
                    while (!fut.isFinished())
                    {
                        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
                    }

                    if(fut.result())
                    {
                        QImage image(dstFile);
                        if (image.width() > width() - 20)
                        {
                            image = image.scaledToWidth(width() - 20);
                        }
                        cursor.insertImage(image);
                        QFile::remove(dstFile);
                    }
                }
                else
                {
                    QImage image(picPath);
                    if (image.width() > width() - 20)
                    {
                        image = image.scaledToWidth(width() - 20);
                    }
                    cursor.insertImage(image);
                    QFile::remove(picPath);
                }
            }
            else
            {
                cursor.insertText(content);
            }
        }

        int resizeHeight = GetSettings()->m_maxResultsPerPage * 60;
        animateResize(m_initHeight + resizeHeight + 10, m_textEdit);
        m_textEdit->resize(m_width - 10, resizeHeight);

        m_listWidget->hide();
        m_progressbar->hide();
        m_textEdit->show();
        this->activateWindow();
        return;
    }

    m_textEdit->hide();

    int i = 0;
    for (i = 0; i < resultList.size(); i++)
	{
        ResultItem* myitem;
        if (i < m_listWidget->count())
        {
            m_listWidget->showItem(i);
            myitem = m_listWidget->getItem(i);
            myitem->m_result = resultList[i];
            myitem->update();
        }
        else
        {
            myitem = new ResultItem(this,&resultList[i]);
            myitem->setProperty("play_state", QMediaPlayer::StoppedState);
            myitem->setTheme(m_mainColor);
            m_listWidget->addItemWidget(myitem);
        }
	}

    while (m_listWidget->count() > i)
    {
        m_listWidget->hideItem(i);
        i++;
    }

    if (m_listWidget->number() > GetSettings()->m_maxResultsPerPage)
    {
        int resizeHeight = GetSettings()->m_maxResultsPerPage * 60;
        if (!m_curThemeType)
        {
            m_listWidget->resize(m_width - 10, resizeHeight);
        }
        else
        {
            m_listWidget->resize(m_width, resizeHeight);
        }
        animateResize(m_initHeight + resizeHeight + 10, m_listWidget);
    }
    else
    {
        int resizeHeight = m_listWidget->number() * 60;
        if (!m_curThemeType)
        {
            m_listWidget->resize(m_width - 10, resizeHeight);
        }
        else
        {
            m_listWidget->resize(m_width, resizeHeight);
        }
        animateResize(m_initHeight + resizeHeight + 10, m_listWidget);
    }

    if (m_inputState.primaryMode == MenuInput)
    {
        m_listWidget->setCurrentRow(0);
    }
    else if (m_inputState.primaryMode != EnterInput)
    {
        if (m_itemNum == -1)
        {
            m_listWidget->setCurrentRow(0);
        }
        else
        {
            m_listWidget->setCurrentRow(m_itemNum);
        }
    }
    else
    {
        if (m_itemNum != -1)
        {
            m_listWidget->setFocus();
        }
        m_listWidget->setCurrentRow(m_itemNum);
    }

    m_iconExtractor.processIcons(resultList);

    m_progressbar->hide();
    m_listWidget->show();

    this->activateWindow();
}

void MainDialog::sltResultError()
{
    m_lineEdit->setEnabled(true);
    this->activateWindow();
    m_lineEdit->setFocus();
    animateResize(m_initHeight);
    hideResultUI();
}

void MainDialog::sltCancelOpr()
{
    m_progressbar->hide();
    this->activateWindow();
    m_lineEdit->setFocus();
}

void MainDialog::sltMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::EndOfMedia)
    {
        stopMusic();
    }
}

void MainDialog::sltOpen()
{
    this->show();
    this->activateWindow();

    if (m_inputState.primaryMode != EnterInput)
    {
        m_lineEdit->setFocus();
        m_lineEdit->setSelection(m_lineEdit->text().size(),0);
    }

    if (GetSettings()->m_indexCfg.clearHistory)
    {
        Plugin* plugin = m_task->getPlugin();
        if (plugin && plugin->m_guid == PROGRAM_PLUGIN_ID)
        {
            m_lineEdit->clear();
        }
    }

    GetUsageSetting()->m_usage++;
    GetUsageSetting()->save();
}

void MainDialog::sltSet()
{
    QWidget* setParent = nullptr;
    if (!m_listWidget->isHidden())
    {
        setParent = this;
    }

    SetDlg dlg(setParent);
    dlg.exec();

    if (GetSettings()->m_hideWhenDeactive)
    {
        if (!m_checkActive->isActive())
        {
            m_checkActive->start(1000);
        }
    }
    else
    {
        if (m_checkActive->isActive())
        {
            m_checkActive->stop();
        }
    }

    QString hotKey = GetSettings()->m_hotKey;
    if (hotKey != m_hotKey)
    {
        m_hotKey = hotKey;
        m_shortcut->setShortcut(QKeySequence(m_hotKey));
    }
}

void MainDialog::sltReload()
{
    GetThemeSetting()->load();
    setTheme();
    GetPluginMananger()->loadPlugin();
}

void MainDialog::sltAbout()
{
    QWidget* setParent = nullptr;
    if (!m_listWidget->isHidden())
    {
        setParent = this;
    }

    AboutDialog dlg(setParent);
    dlg.exec();
}

void MainDialog::sltExit()
{
	delete m_pTrayIcon;

    exit(0);
}

void MainDialog::setTheme()
{
    QString scrollStyleSheet = QString("QScrollBar {"
        "    border: 1px solid #999999;"
        "    background:white;"
        "    width:10px;    "
        "    margin: 0px 0px 0px 0px;"
        "}"
        "QScrollBar::handle {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop: 0 %1, stop: 0.5 %1, stop:1 %1);"
        "    min-height: 0px;"
        "}"
        "QScrollBar::add-line {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop: 0 %1, stop: 0.5 %1,  stop:1 %1);"
        "    height: 0px;"
        "    subcontrol-position: bottom;"
        "    subcontrol-origin: margin;"
        "}"
        "QScrollBar::sub-line {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop: 0  %1, stop: 0.5 %1,  stop:1 %1);"
        "    height: 0 px;"
        "    subcontrol-position: top;"
        "    subcontrol-origin: margin;"
        "}"
    );

    Theme* theme = GetThemeSetting()->getSelectTheme();
    if (theme)
    {
        if (m_curThemeType != theme->type || m_roundCorner != GetThemeSetting()->m_roundCorner)
        {
            if (QMessageBox::No == QMessageBox::information(this, tr("提示"), tr("需要重启程序，是否切换？"), QMessageBox::Yes | QMessageBox::No))
            {
                return;
            }

            qApp->exit(773);
        }

        setWindowOpacity(theme->win_opacity);

        if (!theme->type)
        {
            m_mainColor = ToColor(theme->child_win_color);
            if (theme->child_win_color.isEmpty())
            {
                m_mainColor = ToColor(theme->win_color);
            }

            m_initHeight = 70;

            resize(m_width, 70);

            m_lineEdit->setGeometry(5, 5, m_width - 10, 60);
            m_progressbar->setGeometry(5, 66, m_width - 10, 2);
            m_listWidget->setGeometry(5, 70, m_width - 10, 60);
            m_textEdit->setGeometry(5, 70, m_width - 10, 60 * GetSettings()->m_maxResultsPerPage);

            QString styleSheet = QString("MainDialog{background-color: rgb(%1);}").
                arg(theme->win_color);
            setStyleSheet(styleSheet);

            styleSheet.clear();

            QString child_win_color = theme->child_win_color.isEmpty() ? theme->win_color : theme->child_win_color;

            if (theme->text_color.isEmpty())
            {
                double brightness = GetPngBrightness(m_mainColor);
                if (brightness < 100)
                {
                    styleSheet = QString("background-color: rgb(%1);color:white;"
                                         "selection-background-color: #3399FF;"
                                         "selection-color: white;").
                                        arg(child_win_color);
                    QPixmap pixmap(":/Images/play.png");
                    m_playPixmap = ConvertColor(pixmap,QColor(Qt::white));
                    pixmap.load(":/Images/pause.png");
                    m_pausePixmap = ConvertColor(pixmap,QColor(Qt::white));
                }
                else
                {
                    styleSheet = QString("background-color: rgb(%1);color:black;"
                                         "selection-background-color: #3399FF;"
                                         "selection-color: white;").
                                        arg(child_win_color);
                    QPixmap pixmap(":/Images/play.png");
                    m_playPixmap = ConvertColor(pixmap,QColor(Qt::black));
                    pixmap.load(":/Images/pause.png");
                    m_pausePixmap = ConvertColor(pixmap,QColor(Qt::black));
                }
            }
            else
            {
                styleSheet = QString("background-color: rgb(%1);color:rgb(%2);"
                                     "selection-background-color: #3399FF;"
                                     "selection-color: white;").
                                    arg(child_win_color).arg(theme->text_color);
                QPixmap pixmap(":/Images/play.png");
                m_playPixmap = ConvertColor(pixmap,ToColor(theme->text_color));
                pixmap.load(":/Images/pause.png");
                m_pausePixmap = ConvertColor(pixmap,ToColor(theme->text_color));
            }

            m_lineEdit->setStyleSheet(styleSheet);
            m_textEdit->setStyleSheet(styleSheet);

            QColor scrollbar_color = ToColor(theme->scrollbar_color);
            if (theme->scrollbar_color.isEmpty())
            {
                scrollbar_color = ToColor(child_win_color).darker(125);
            }
            scrollStyleSheet = scrollStyleSheet.replace("%1", scrollbar_color.name());
            m_textEdit->verticalScrollBar()->setStyleSheet(scrollStyleSheet);
            m_textEdit->horizontalScrollBar()->setStyleSheet(scrollStyleSheet);

            m_listWidget->setTheme();

            for (int i = 0; i < m_listWidget->count(); i++)
            {
                ResultItem* rItem = m_listWidget->getItem(i);
                if (rItem)
                {
                    rItem->setTheme();
                }
            }
        }
        else
        {
            QPixmap pixmap(theme->image_path);
            m_pixmap = pixmap.scaledToWidth(m_width);
            resize(m_pixmap.size());
            m_initHeight = m_pixmap.height();

            m_mainColor = PixmapMainColor(m_pixmap, 1);
            QPoint realPosition = RealPointFromAlphaPng(m_pixmap);

            m_lineEdit->setGeometry(10, realPosition.y() + 5, m_width - 20, height() - realPosition.y() - 10);
            m_progressbar->setGeometry(10, realPosition.y() + 5 + m_lineEdit->height() + 1, m_width - 20, 2);
            m_listWidget->setGeometry(0, height() - 1, m_width, 60);
            m_textEdit->setGeometry(0, height() - 1, m_width, 60 * GetSettings()->m_maxResultsPerPage);

            setStyleSheet(tr("MainDialog{background-color: %1;}").arg(m_mainColor.name()));

            if (theme->text_color.isEmpty())
            {
                double brightness = GetPngBrightness(m_mainColor);
                if (brightness < 100)
                {
                    m_lineEdit->setStyleSheet("background-color: transparent;color:white");
                    m_textEdit->setStyleSheet(tr("background-color: %1;color:white;"
                                                 "selection-background-color: #3399FF;"
                                                 "selection-color: white;").
                                                arg(m_mainColor.name()));
                    QPixmap pixmap(":/Images/play.png");
                    m_playPixmap = ConvertColor(pixmap,QColor(Qt::white));
                    pixmap.load(":/Images/pause.png");
                    m_pausePixmap = ConvertColor(pixmap,QColor(Qt::white));
                }
                else
                {
                    m_lineEdit->setStyleSheet("background-color: transparent;color:black");
                    m_textEdit->setStyleSheet(tr("background-color: %1;color:black;"
                                                 "selection-background-color: #3399FF;"
                                                 "selection-color: white;").
                                                arg(m_mainColor.name()));
                    QPixmap pixmap(":/Images/play.png");
                    m_playPixmap = ConvertColor(pixmap,QColor(Qt::black));
                    pixmap.load(":/Images/pause.png");
                    m_pausePixmap = ConvertColor(pixmap,QColor(Qt::black));
                }
            }
            else
            {
                m_lineEdit->setStyleSheet(tr("background-color: transparent;color:rgb(%1)").arg(theme->text_color));
                m_textEdit->setStyleSheet(tr("background-color: %1;color:rgb(%2);"
                                             "selection-background-color: #3399FF;"
                                             "selection-color: white;").
                                            arg(m_mainColor.name()).arg(theme->text_color));
                QPixmap pixmap(":/Images/play.png");
                m_playPixmap = ConvertColor(pixmap,ToColor(theme->text_color));
                pixmap.load(":/Images/pause.png");
                m_pausePixmap = ConvertColor(pixmap,ToColor(theme->text_color));
            }

            QColor scrollbar_color = ToColor(theme->scrollbar_color);
            if (theme->scrollbar_color.isEmpty())
            {
                scrollbar_color = m_mainColor.darker(125);
            }
            scrollStyleSheet = scrollStyleSheet.replace("%1", scrollbar_color.name());
            m_textEdit->verticalScrollBar()->setStyleSheet(scrollStyleSheet);
            m_textEdit->horizontalScrollBar()->setStyleSheet(scrollStyleSheet);

            m_listWidget->setTheme(m_mainColor);

            for (int i = 0; i < m_listWidget->count(); i++)
            {
                ResultItem* rItem = m_listWidget->getItem(i);
                if (rItem)
                {
                    rItem->setTheme(m_mainColor);
                }
            }

        }

    }
}

void MainDialog::showMsg(QString title, QString msg)
{
    QMessageBox* box = new QMessageBox(this);
    box->setAttribute(Qt::WA_DeleteOnClose,true);
    box->setModal(false);
    box->setWindowTitle(title);
    box->setText(msg);
    box->setIcon(QMessageBox::Information);
    box->show();
}

void MainDialog::showContent(QString title, QString content)
{
    ShowContentDlg* dlg = new ShowContentDlg(this);
    dlg->resize(width(), 480);
    dlg->setAttribute(Qt::WA_DeleteOnClose,true);
    dlg->setReadOnly(true);
    if (title.isEmpty())
        dlg->setContent(content);
    else
        dlg->setContent(title, content);
    dlg->show();
}

void MainDialog::showTip(QString title, QString content)
{
    m_notifyMgr->notify(title, content);
}

void MainDialog::editFile(QString title, QString filePath)
{
    ShowContentDlg* dlg = new ShowContentDlg(filePath, this);
    dlg->resize(width(), 480);
    dlg->setAttribute(Qt::WA_DeleteOnClose,true);
    dlg->setReadOnly(false);
    dlg->setWindowTitle(title);
    dlg->show();
}

void MainDialog::playMusic(QString url)
{
    if (!m_curClickedItem) return;

    QMediaPlayer::State state;
    state = (QMediaPlayer::State)m_curClickedItem->property("play_state").toUInt();

    if (state == QMediaPlayer::StoppedState)
    {
        m_mediaPlayer.stop();

        QUrl music_url(url);
        if (!url.startsWith("http"))
        {
            music_url = QUrl::fromLocalFile(url);
        }

        m_mediaPlayer.setMedia(music_url);
        m_mediaPlayer.play();

        QMediaPlayer::MediaStatus status = m_mediaPlayer.mediaStatus();
        while (status == QMediaPlayer::LoadingMedia || status == QMediaPlayer::BufferingMedia)
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
            status = m_mediaPlayer.mediaStatus();
        }

        if (status == QMediaPlayer::InvalidMedia)
        {
            qDbg(tr("Invalid url %1").arg(url));
            showMsg(tr("提示"),tr("无效播放地址"));
            return;
        }

        m_curClickedItem->setProperty("play_state", QMediaPlayer::PlayingState);
        if (m_inputState.primaryMode != MenuInput)
            m_curClickedItem->updateIcon(m_playPixmap);
    }
    else if (state == QMediaPlayer::PausedState)
    {
        m_mediaPlayer.play();
        m_curClickedItem->setProperty("play_state", QMediaPlayer::PlayingState);
        if (m_inputState.primaryMode != MenuInput)
            m_curClickedItem->updateIcon(m_playPixmap);
    }
    else
    {
        m_mediaPlayer.pause();
        m_curClickedItem->setProperty("play_state", QMediaPlayer::PausedState);
        if (m_inputState.primaryMode != MenuInput)
            m_curClickedItem->updateIcon(m_pausePixmap);
    }
}

void MainDialog::pauseMusic()
{
    m_mediaPlayer.pause();
    if (m_curClickedItem)
    {
        m_curClickedItem->setProperty("play_state", QMediaPlayer::PausedState);
        if (m_inputState.primaryMode != MenuInput)
            m_curClickedItem->updateIcon(m_pausePixmap);
    }
}

void MainDialog::stopMusic()
{
    m_mediaPlayer.stop();
    if (m_curClickedItem)
    {
        m_curClickedItem->setProperty("play_state", QMediaPlayer::StoppedState);
        if (m_inputState.primaryMode != MenuInput)
        {
            QPixmap pixmap(m_curClickedItem->m_result.iconPath);
            m_curClickedItem->updateIcon(pixmap);
        }
    }
}

void MainDialog::sltHotKey()
{
    if (this->isHidden())
    {
        if (!GetSettings()->m_remLastPosition)
        {
            QPoint point = QCursor::pos();

            QScreen* primary = QApplication::primaryScreen();
            int priWidth = primary->geometry().width();

            QScreen* screen = QApplication::screenAt(point);
            if (!screen)
            {
                screen = primary;
            }
            int curWidth = screen->geometry().width();
            int curHeight = screen->geometry().height();

            if (screen->name() != primary->name())
            {
                int org_width = (int)(priWidth * m_factor);
                move ((curWidth - width())/2 + org_width, (curHeight - m_initHeight)/10);
            }
            else
            {
                move ((curWidth - width())/2, (curHeight - m_initHeight)/10);
            }
        }

        this->show();
        this->activateWindow();
        if (m_inputState.primaryMode != EnterInput)
        {
            m_lineEdit->setFocus();
            m_lineEdit->setSelection(m_lineEdit->text().size(),0);
        }
        else
        {
            m_lineEdit->setSelection(m_lineEdit->text().size(),0);
        }

        if (GetSettings()->m_indexCfg.clearHistory)
        {
            Plugin* plugin = m_task->getPlugin();
            if (plugin && plugin->m_guid == PROGRAM_PLUGIN_ID)
            {
                m_lineEdit->clear();
            }
        }

        GetUsageSetting()->m_usage++;
        GetUsageSetting()->save();
    }
    else
    {
        this->hide();

        if (GetSettings()->m_remLastPosition &&
            (GetSettings()->m_x != this->x() ||
            GetSettings()->m_y != this->y()))
        {
            GetSettings()->m_x = this->x();
            GetSettings()->m_y = this->y();
            GetSettings()->save();
        }

    }
}

void MainDialog::sltLeftOpr()
{
    ResultItem* itemWidget = m_listWidget->getCurrentItem();
    if (!itemWidget)
    {
        return;
    }

    QString funcName = itemWidget->m_result.action.funcName;
    if (funcName.isEmpty())
    {
        return;
    }

    Plugin* plugin = GetPluginMananger()->getPlugin(itemWidget->m_uuid);
    if (!plugin)
    {
        return;
    }

    if (itemWidget->m_result.showType == SHOW_TYPE_MUSIC)
    {
        if (m_curClickedItem != itemWidget)
        {
            stopMusic();
        }

        m_curClickedItem = itemWidget;
    }
    else
    {
        if (m_inputState.primaryMode == MenuInput)
        {
            ResultItem* item = m_listWidget->getItem(m_itemNum);
            if (item && item->m_result.showType == SHOW_TYPE_MUSIC)
            {
                if (m_curClickedItem != item)
                {
                    stopMusic();
                }

                m_curClickedItem = item;
            }
        }
    }

    //之所以在这里取值是因为itemClick里面切换到主界面导致result发生变化，下面hideWindow也会重置为True
    bool isHideWindow = itemWidget->m_result.action.hideWindow;

    m_progressbar->show();
    g_fromMainDlg = this;
    plugin->itemClick(itemWidget->m_result,this);
    m_progressbar->hide();

    if (m_inputState.primaryMode != EnterInput)
    {
        m_lineEdit->setFocus();
    }

    if (isHideWindow)
    {
        this->hide();
    }
}

void MainDialog::sltRightOpr()
{
    ResultItem* itemWidget = m_listWidget->getCurrentItem();
    if (!itemWidget)
    {
        return;
    }

    Plugin* plugin = GetPluginMananger()->getPlugin(itemWidget->m_uuid);
    if (!plugin)
    {
        return;
    }

    if (m_inputState.primaryMode != MenuInput)
    {
        QVector<Result> menus;
        if (!plugin->getMenu(itemWidget->m_result,menus))
        {
            return;
        }

        if (menus.isEmpty())
        {
            return;
        }

        m_lineEdit->setEnabled(false);
        m_inputState.primaryMode = MenuInput;
        m_itemNum = m_listWidget->currentRow();

        QVariant variant;
        TaskResult taskResult;
        taskResult.id = m_taskId;
        taskResult.vecResult.swap(menus);
        variant.setValue(taskResult);
        emit resultReady(variant);
    }
    else
    {
        m_lineEdit->setEnabled(true);
        m_lineEdit->setFocus();

        resetInputState();

        QVariant variant;
        variant.setValue(m_taskResult);
        emit resultReady(variant);
    }
}

void MainDialog::sltLineEditClick()
{
    if (m_inputState.primaryMode == EnterInput)
    {
        m_listWidget->setCurrentRow(-1);
    }
}

void MainDialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
    {
        return;
    }

    if (event->pos().x() >= 0 && event->pos().x() <= this->width()
        && event->pos().y() >= 0 && event->pos().y() <= 10
        ) {
            m_move = true;
            m_lastPoint = event->pos();
    } else {
        m_move = false;
    }
}

void MainDialog::mouseMoveEvent(QMouseEvent *event)
{
	if (m_move) {
        this->move(this->pos() + event->pos() - m_lastPoint);
	}
}

void MainDialog::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
	m_move = false;
}

bool MainDialog::eventFilter(QObject *obj, QEvent *event)
{
    bool handled = false;

    if(event->type() == QEvent::KeyPress)
    {
        QKeyEvent *me = static_cast<QKeyEvent*>(event);
        if (me->key() == Qt::Key_Tab)
        {
            if (m_inputState.primaryMode == EnterInput)
            {
                if (m_lineEdit->hasFocus())
                {
                    if (m_listWidget->isVisible())
                    {
                        m_listWidget->setFocus();
                        m_listWidget->setCurrentRow(0);
                    }
                    else
                    {
                        m_textEdit->setFocus();
                    }
                }
                else
                {
                    m_lineEdit->setFocus();
                    m_lineEdit->setSelection(m_lineEdit->text().size(),0);
                    if (m_listWidget->isVisible())
                        m_listWidget->setCurrentRow(-1);
                }
            }
            else
            {
                ResultItem* item = m_listWidget->getCurrentItem();
                if (item && m_inputState.primaryMode != MenuInput)
                {
                    if (m_inputState.primaryMode == DropInput &&
                        !m_lineEdit->text().contains(m_lineEdit->separatorText()) &&
                        !item->m_result.id.isEmpty())
                    {
                        m_inputState.isSecondary = true;
                        m_inputState.additionalData = item->m_uuid;
                        m_lineEdit->setText(m_lineEdit->text() + m_lineEdit->separatorText());
                    }
                    else
                    {
                        if (item->m_uuid == PROGRAM_PLUGIN_ID &&
                            !item->m_result.action.funcName.isEmpty() &&
                            item->m_result.action.funcName != "Ra_ActivePlugin" &&
                            !m_lineEdit->text().contains(m_lineEdit->separatorText()))
                        {
                            m_inputState.isSecondary = true;
                            m_inputState.additionalData = item->m_result.subTitle;
                            m_lineEdit->setText(item->m_result.title + m_lineEdit->separatorText());
                        }
                    }
                }

            }

            if (m_curClickedItem && m_mediaPlayer.state() == QMediaPlayer::PlayingState)
            {
                pauseMusic();
            }

            handled = true;
        }
        else if (me->key() == Qt::Key_Escape)
        {
            this->hide();
            handled = true;
        }
        else if (me->key() == Qt::Key_Down)
        {
            if (m_listWidget->isVisible() &&
                m_listWidget->number() != 0)
            {
                int index = m_listWidget->currentRow();
                if (m_inputState.primaryMode == EnterInput && !m_listWidget->hasFocus())
                {
                    m_listWidget->setFocus();
                }

                if (index+1 < m_listWidget->number())
                {
                    m_listWidget->setCurrentRow(index+1);
                }
                else
                {
                    m_listWidget->setCurrentRow(0);
                }

                handled = true;
            }
        }
        else if (me->key() == Qt::Key_Up)
        {
            if (m_listWidget->isVisible() &&
                m_listWidget->number() != 0)
            {
                int index = m_listWidget->currentRow();
                if (-1 != index && index-1 >= 0)
                {
                    m_listWidget->setCurrentRow(index-1);
                }
                else
                {
                    if (m_inputState.primaryMode == EnterInput)
                    {
                        m_lineEdit->setFocus();
                        m_listWidget->setCurrentRow(-1);
                    }
                    else
                    {
                        m_listWidget->setCurrentRow(m_listWidget->number()-1);
                    }
                }

                handled = true;
            }
        }
        else if(me->key() == Qt::Key_Enter || me->key() == Qt::Key_Return)
        {
            if (m_inputState.primaryMode == EnterInput && m_lineEdit->hasFocus())
            {
                m_progressbar->show();
                m_task->query();
            }
            else
            {
                sltLeftOpr();
            }
            handled = true;
        }
        else if(me->key() == Qt::Key_Right)
        {
            if (m_lineEdit->cursorPosition() == m_lineEdit->text().length())
            {
                sltRightOpr();
                handled = true;
            }
        }
        else if (me->key() == Qt::Key_Y &&
                 me->modifiers() & Qt::ControlModifier)
        {
            Plugin* plugin = m_task->getPlugin();
            if (plugin)
            {
                if (plugin->m_info.enableSeparate)
                {
                    m_lineEdit->clear();
                    this->hide();

                    MainPluginDialog* pluginDialog = new MainPluginDialog(plugin, this);
                    pluginDialog->setAttribute(Qt::WA_DeleteOnClose,true);
                    int index = m_filterText.indexOf(" ");
                    if (index != -1)
                    {
                        QString parameter = m_filterText.mid(index+1);
                        pluginDialog->setQueryText(parameter);
                    }
                    pluginDialog->show();
                    pluginDialog->removeSelect();
                }
                else
                {
                    showMsg(tr("提示"),tr("该插件不支持分离窗口"));
                }
            }

            handled = true;
        }
    }
    else if (event->type() == QEvent::HoverMove || event->type() == QEvent::HoverEnter)
    {
        if (obj == m_listWidget)
        {
            QHoverEvent* hover_event = static_cast<QHoverEvent*>(event);
            QListWidgetItem* item = m_listWidget->itemAt(hover_event->pos());
            if (item)
            {
                m_listWidget->setCurrentItem(item);
                handled = true;
            }
        }
    }

    return handled;
}

void MainDialog::installPlugin(QString& filePath)
{
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
        if (GetSettings()->m_pythonPath.isEmpty())
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
            QWidget* progressParent = nullptr;
            if (!m_listWidget->isHidden())
            {
                progressParent = this;
            }
            InstallTask task;
            QProgressDialog dlg(progressParent);
            dlg.resize(300,200);
            dlg.setWindowTitle(tr("安装"));
            dlg.setLabelText(tr("安装插件依赖中,请不要关闭对话框..."));
            dlg.setRange(0,0);
            dlg.setMinimumDuration(0);
            dlg.setCancelButton(0);

            task.m_pythonPath = GetSettings()->m_pythonPath;
            task.m_requirePath = requirePath;
            connect(&task,SIGNAL(finished()),&dlg,SLOT(accept()));
            task.start();

            if (QDialog::Rejected == dlg.exec())
            {
                task.terminate();
                task.m_exitCode = 1;
            }

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

    if (plugin->m_info.keyword[0].isEmpty() && plugin->m_info.acceptType.isEmpty())
    {
        QMessageBox::information(this,tr("提示"),tr("安装插件成功，请设置关键字后使用！"));
    }
    else
    {
        QMessageBox::information(this,tr("提示"),tr("安装插件成功！"));
    }

}

void MainDialog::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    if (m_curThemeType)
    {
        QPainter painter(this);
        painter.drawPixmap(0, 0, m_pixmap);
    }
    else
    {
        if (GetThemeSetting()->m_roundCorner)
        {
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing); // 反锯齿;

            QColor win_color;
            Theme* theme = GetThemeSetting()->getSelectTheme();
            if (theme)
            {
                win_color = ToColor(theme->win_color);

            }
            else
            {
                win_color = QColor(240, 240, 240);
            }

            painter.setBrush(QBrush(win_color));//窗体背景色
            painter.setPen(Qt::transparent);
            QRect rect = this->rect();//rect为绘制大小
            rect.setWidth(rect.width() - 1);
            rect.setHeight(rect.height() - 1);
            painter.drawRoundedRect(rect, 8, 8);//15为圆角角度
        }
    }
}

void MainDialog::dragEnterEvent(QDragEnterEvent *event)
{
    if (m_lineEdit->text().isEmpty())
    {
        event->acceptProposedAction();
    }
}

void MainDialog::dropEvent(QDropEvent *event)
{
    QString filePath = event->mimeData()->urls().first().toString();
    QUrl url(filePath);
    QString nativeFilePath = url.toLocalFile();
    QFileInfo info(nativeFilePath);

    QString fileType = info.suffix();
    if (fileType.compare("plugin",Qt::CaseInsensitive) == 0)
    {
        m_lineEdit->blockSignals(true);
        m_lineEdit->setText(info.fileName());
        m_lineEdit->blockSignals(false);

        m_lineEdit->setEnabled(false);

        if (QMessageBox::Yes == QMessageBox::information(this,tr("提示"),tr("是否要安装此插件？"),QMessageBox::Yes | QMessageBox::No))
        {
            QUrl url(filePath);
            QString nativeFilePath = url.toLocalFile();
            installPlugin(nativeFilePath);
        }

        m_lineEdit->setEnabled(true);
        m_lineEdit->clear();
        this->activateWindow();
        m_lineEdit->setFocus();
    }
    else
    {
        m_inputState.primaryMode = DropInput;

        if (info.isDir())
        {
            m_inputState.additionalQuery.keyword = "folder";
        }
        else
        {
            m_inputState.additionalQuery.keyword = fileType;
        }

        m_inputState.additionalQuery.parameter = nativeFilePath;

        m_lineEdit->setText(info.fileName());
    }
}

void MainDialog::animateResize(int targetHeight, QWidget* targetWidget)
{
    m_resizeAnimation->stop();
    m_resizeAnimation->setStartValue(size());
    m_resizeAnimation->setEndValue(QSize(m_width, targetHeight));
    
    if (targetWidget) {
        targetWidget->resize(targetWidget->width(), targetHeight - m_initHeight - 10);
    }
    
    m_resizeAnimation->start();
}

void MainDialog::createTextEditMenu()
{
    m_textEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_textEdit, &QTextEdit::customContextMenuRequested, [this](const QPoint &pos) {
        QMenu *menu = new QMenu(this);

        QTextCursor cursor = m_textEdit->cursorForPosition(pos);
        QTextCharFormat format = cursor.charFormat();
        bool hasSelection = m_textEdit->textCursor().hasSelection();
        bool isImage = format.isImageFormat();

        QAction *copyAct = menu->addAction(tr("复制"));
        copyAct->setEnabled(hasSelection || isImage);

        connect(copyAct, &QAction::triggered, [this, isImage, cursor]() mutable {
            if (isImage) {
                copyImageFromTextEdit();
            } else {
                m_textEdit->copy();
            }
        });

        menu->exec(m_textEdit->mapToGlobal(pos));
        delete menu;
    });
}

void MainDialog::copyImageFromTextEdit()
{
    QTextCursor cursor = m_textEdit->textCursor();
    if (cursor.charFormat().isImageFormat()) {
        QTextImageFormat imageFormat = cursor.charFormat().toImageFormat();
        QString imageId = imageFormat.name();
        QVariant imgVar = m_textEdit->document()->resource(QTextDocument::ImageResource, QUrl(imageId));
        if (imgVar.isValid()) {
            QImage image = imgVar.value<QImage>();
            if (!image.isNull()) {
                QApplication::clipboard()->setImage(image);
            }
        }
    }
}
