#include "MainPluginDialog.h"
#include "ResultItem.h"
#include <QLibrary>
#include <QMouseEvent>
#include <QMenu>
#include "PluginManager.h"
#include "Settings.h"
#include "LogFile.h"
#include "ThemeSetting.h"
#include <QMessageBox>
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
#include "HelperFunc.h"
#include <QBitmap>
#include <QtConcurrent>
#include <QApplication>
#include <QScreen>
#include <QClipboard>

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
# pragma execution_character_set("utf-8")
#endif

extern QObject* g_fromMainDlg;

MainPluginDialog::MainPluginDialog(Plugin* plugin, QWidget *parent) :
    QDialog(parent),
    m_taskId(0),
    m_curThemeType(0),
    m_curClickedItem(nullptr),
    m_forceQuery(false),
    m_plugin(plugin)
{
    setWindowTitle(m_plugin->m_info.name);

    setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);

    m_inputState.primaryMode = NormalInput;
    m_inputState.isSecondary = false;
    m_inputState.additionalData = "";

    QScreen* primary = QApplication::primaryScreen();
    int iwidth = primary->geometry().width();
    int iheight = primary->geometry().height();
    m_width = iwidth/5*2;

    m_initHeight = 70;

    setGeometry(0,0,m_width, m_initHeight);
    setAcceptDrops(false);

    move ((iwidth - width())/2,(iheight - height())/3);

    connect(this,SIGNAL(resultReady(QVariant)),this,SLOT(sltResultReady(QVariant)));

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

    connect(m_listWidget,SIGNAL(sigMouseRight()),this,SLOT(sltRightOpr()));
    connect(m_listWidget,SIGNAL(sigMouseLeft()),this,SLOT(sltLeftOpr()));

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

    m_task = m_taskManager.getIdleTask();

    connect(m_task,SIGNAL(resultReady(QVariant)),this,SLOT(sltResultReady(QVariant)));
    connect(m_task,SIGNAL(resultError()),this,SLOT(sltResultError()));
    connect(m_task,SIGNAL(cancel()),this,SLOT(sltCancelOpr()));
    m_task->m_id = m_taskId;

    m_typingTimer = new QTimer( this );
    m_typingTimer->setSingleShot( true );
    connect(m_typingTimer, SIGNAL(timeout()),this, SLOT(sltFilterEntries()));

    connect(&m_iconExtractor,SIGNAL(iconExtracted(int,QPixmap)),this,SLOT(sltIconExtracted(int,QPixmap)));

    connect(&m_mediaPlayer,SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)),
            this,SLOT(sltMediaStatusChanged(QMediaPlayer::MediaStatus)));

    m_lineEdit->installEventFilter(this);
    m_listWidget->installEventFilter(this);
    m_textEdit->installEventFilter(this);

    // 初始化大小改变动画
    m_resizeAnimation = new QPropertyAnimation(this, "size");
    m_resizeAnimation->setEasingCurve(QEasingCurve::OutCubic);
    m_resizeAnimation->setDuration(200); // 200ms动画时长
}

MainPluginDialog::~MainPluginDialog()
{

}

void MainPluginDialog::setQueryText(QString queryText)
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

void MainPluginDialog::hideResultUI()
{
    m_listWidget->hide();
    m_listWidget->hideAll();
    m_listWidget->setCurrentRow(-1);

    m_textEdit->clear();
    m_textEdit->hide();

    m_progressbar->hide();
}

void MainPluginDialog::resetInputState()
{
    m_inputState.primaryMode = NormalInput;
    m_inputState.isSecondary = false;
    m_inputState.additionalData.clear();
    m_inputState.additionalQuery = Query();
}

void MainPluginDialog::sltFilterEntries()
{
    if (m_lineEdit->text().isEmpty())
    {
        return;
    }

    //全部是空格
    if (m_filterText.count(" ") == m_filterText.size())
    {
        return;
    }

    m_filterText = m_filterText.remove(QRegExp("^ +\\s*"));

    Query query;
    QStringList split;

    PluginConfig cfg = GetSettings()->getPluginCfg(m_plugin->m_guid);

    query.rawQuery = QString("%1 %2").arg(cfg.keyword[0]).arg(m_filterText);
    query.keyword = cfg.keyword[0];
    query.parameter = m_filterText;

    int index = GetSettings()->find(m_plugin->m_guid);
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
        if (m_plugin->m_mode == EnterMode)
        {
            m_inputState.primaryMode = EnterInput;
        }
        else
        {
            m_inputState.primaryMode = NormalInput;
        }
    }

    m_itemNum = -1;

    if (m_task->isRunning())
    {
        m_task = m_taskManager.getIdleTask();
        connect(m_task,SIGNAL(resultReady(QVariant)),this,SLOT(sltResultReady(QVariant)));
        connect(m_task,SIGNAL(resultError()),this,SLOT(sltResultError()));
        connect(m_task,SIGNAL(cancel()),this,SLOT(sltCancelOpr()));
        m_task->m_id = ++m_taskId;
    }

    if (m_inputState.primaryMode != EnterInput || m_forceQuery ||
         query.parameter.split(' ',Qt::SkipEmptyParts).size() < m_plugin->m_info.argc)
    {
        m_progressbar->show();
        m_task->query(m_plugin,query);
    }
    else
    {
        m_task->setQuery(m_plugin,query);
    }

    m_forceQuery = false;
}

void MainPluginDialog::sltTextChanged(const QString &inputStr)
{
    if (m_lineEdit->text().isEmpty())
    {
        m_task->stop();
        resetInputState();
        animateResize(m_initHeight);
        hideResultUI();
        return;
    }

    if (m_mediaPlayer.state() == QMediaPlayer::PlayingState ||
        m_mediaPlayer.state() == QMediaPlayer::PausedState)
    {
        stopMusic();
    }

    m_filterText = inputStr;
    m_typingTimer->start( 200 );
}

void MainPluginDialog::sltIconExtracted(int itemIndex, QPixmap icon)
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

void MainPluginDialog::sltResultReady(QVariant rInfo)
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

void MainPluginDialog::sltResultError()
{
    m_lineEdit->setEnabled(true);
    this->activateWindow();
    m_lineEdit->setFocus();
    animateResize(m_initHeight);
    hideResultUI();
}

void MainPluginDialog::sltCancelOpr()
{
    m_progressbar->hide();
    this->activateWindow();
    m_lineEdit->setFocus();
}

void MainPluginDialog::sltMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::EndOfMedia)
    {
        stopMusic();
    }
}

void MainPluginDialog::setTheme()
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
        setWindowOpacity(theme->win_opacity);

        m_curThemeType = theme->type;

        if (!theme->type)
        {
            m_mainColor = ToColor(theme->child_win_color);
            if (theme->child_win_color.isEmpty())
            {
                m_mainColor = ToColor(theme->win_color);
            }
        }
        else
        {
            QPixmap pixmap(theme->image_path);
            m_pixmap = pixmap.scaledToWidth(m_width);
            resize(m_pixmap.size());
            m_initHeight = m_pixmap.height();

            m_mainColor = PixmapMainColor(m_pixmap, 1);
        }

        m_initHeight = 70;

        resize(m_width, 70);

        m_lineEdit->setGeometry(5, 5, m_width - 10, 60);
        m_progressbar->setGeometry(5, 66, m_width - 10, 2);
        m_listWidget->setGeometry(5, 70, m_width - 10, 60);
        m_textEdit->setGeometry(5, 70, m_width - 10, 60 * GetSettings()->m_maxResultsPerPage);

        QString styleSheet = QString("MainPluginDialog{background-color: rgb(%1);}").
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
}

void MainPluginDialog::showMsg(QString title, QString msg)
{
    QMessageBox* box = new QMessageBox(this);
    box->setAttribute(Qt::WA_DeleteOnClose,true);
    box->setModal(false);
    box->setWindowTitle(title);
    box->setText(msg);
    box->setIcon(QMessageBox::Information);
    box->show();
}

void MainPluginDialog::showContent(QString title, QString content)
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

void MainPluginDialog::showTip(QString title, QString content)
{
    m_notifyMgr->notify(title, content);
}

void MainPluginDialog::editFile(QString title, QString filePath)
{
    ShowContentDlg* dlg = new ShowContentDlg(filePath, this);
    dlg->resize(width(), 480);
    dlg->setAttribute(Qt::WA_DeleteOnClose,true);
    dlg->setReadOnly(false);
    dlg->setWindowTitle(title);
    dlg->show();
}

void MainPluginDialog::playMusic(QString url)
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

void MainPluginDialog::pauseMusic()
{
    m_mediaPlayer.pause();
    if (m_curClickedItem)
    {
        m_curClickedItem->setProperty("play_state", QMediaPlayer::PausedState);
        if (m_inputState.primaryMode != MenuInput)
            m_curClickedItem->updateIcon(m_pausePixmap);
    }
}

void MainPluginDialog::stopMusic()
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

void MainPluginDialog::sltLeftOpr()
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

void MainPluginDialog::sltRightOpr()
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

void MainPluginDialog::sltLineEditClick()
{
    if (m_inputState.primaryMode == EnterInput)
    {
        m_listWidget->setCurrentRow(-1);
    }
}

bool MainPluginDialog::eventFilter(QObject *obj, QEvent *event)
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
                    if (!m_lineEdit->text().contains(m_lineEdit->separatorText()) &&
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

// 添加动画调整大小的辅助函数
void MainPluginDialog::animateResize(int targetHeight, QWidget* targetWidget) 
{
    m_resizeAnimation->stop();
    m_resizeAnimation->setStartValue(size());
    m_resizeAnimation->setEndValue(QSize(m_width, targetHeight));
    
    if (targetWidget) {
        targetWidget->resize(targetWidget->width(), targetHeight - m_initHeight - 10);
    }
    
    m_resizeAnimation->start();
}

void MainPluginDialog::createTextEditMenu()
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

void MainPluginDialog::copyImageFromTextEdit()
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
