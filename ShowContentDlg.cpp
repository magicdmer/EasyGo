#include "ShowContentDlg.h"
#include "ui_ShowContentDlg.h"
#include "ThemeSetting.h"
#include <QScrollBar>
#include <QMouseEvent>
#include <QApplication>
#include <QDesktopWidget>
#include "HelperFunc.h"
#include <QDateTime>
#include <QDir>
#include <QFuture>
#include <QtConcurrent>

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
# pragma execution_character_set("utf-8")
#endif

ShowContentDlg::ShowContentDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ShowContentDlg)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags()&~Qt::WindowContextHelpButtonHint);
    setGeometry(0,0,480,450);

    int primary = QApplication::desktop()->primaryScreen();
    int iwidth = QApplication::desktop()->screen(primary)->width();
    int iheight = QApplication::desktop()->screen(primary)->height();

    move ((iwidth - width())/2,(iheight - height())/2);

    ui->textEdit->setFontPointSize(13);

    setTheme();
}

ShowContentDlg::ShowContentDlg(QString filePath,QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ShowContentDlg)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags()&~Qt::WindowContextHelpButtonHint);
    setGeometry(0,0,480,450);

    int primary = QApplication::desktop()->primaryScreen();
    int iwidth = QApplication::desktop()->screen(primary)->width();
    int iheight = QApplication::desktop()->screen(primary)->height();

    move ((iwidth - width())/2,(iheight - height())/2);

    ui->textEdit->setFontPointSize(13);

    setTheme();

    m_textChanged = false;
    m_filePath = filePath;

    load();

    m_typingTimer = new QTimer( this );
    m_typingTimer->setSingleShot( true );
    connect(m_typingTimer, SIGNAL(timeout()),this, SLOT(sltFilterEntries()));

    connect(ui->textEdit,SIGNAL(textChanged()),this,SLOT(sltTextChanged()));
}

ShowContentDlg::ShowContentDlg(QString filePath, bool transparent, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ShowContentDlg)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags()&~Qt::WindowContextHelpButtonHint);
    setGeometry(0,0,480,450);

    int primary = QApplication::desktop()->primaryScreen();
    int iwidth = QApplication::desktop()->screen(primary)->width();
    int iheight = QApplication::desktop()->screen(primary)->height();

    move ((iwidth - width())/2,(iheight - height())/2);

    ui->textEdit->setFontPointSize(13);

    QString styleSheet = QString("QScrollBar {"
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

    QColor scrollbar_color(240, 240, 240);

    styleSheet = styleSheet.replace("%1", scrollbar_color.name());

    ui->textEdit->verticalScrollBar()->setStyleSheet(styleSheet);
    ui->textEdit->horizontalScrollBar()->setStyleSheet(styleSheet);

    m_textChanged = false;
    m_filePath = filePath;

    load();

    m_typingTimer = new QTimer( this );
    m_typingTimer->setSingleShot( true );
    connect(m_typingTimer, SIGNAL(timeout()),this, SLOT(sltFilterEntries()));

    connect(ui->textEdit,SIGNAL(textChanged()),this,SLOT(sltTextChanged()));
}

ShowContentDlg::~ShowContentDlg()
{
    save();
    delete ui;
}

void ShowContentDlg::sltFilterEntries()
{
    if (ui->textEdit->toPlainText().isEmpty())
    {
        m_textChanged = false;
        return;
    }

    if (m_filterText.count(" ") == m_filterText.size())
    {
        m_textChanged = false;
        return;
    }

    save();

    m_textChanged = false;
}

void ShowContentDlg::sltTextChanged()
{
    m_textChanged = true;
    m_filterText = ui->textEdit->toPlainText();
    m_typingTimer->start( 10000 );
}

bool ShowContentDlg::load()
{
    if (m_filePath.isEmpty())
    {
        return false;
    }

    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        return false;
    }
    QString fileContent = file.readAll();
    file.close();

    ui->textEdit->setPlainText(fileContent);

    return true;
}

bool ShowContentDlg::save()
{
    if (m_filePath.isEmpty())
    {
        return false;
    }

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly))
    {
        return false;
    }

    QString fileContent = ui->textEdit->toPlainText();
    file.write(fileContent.toUtf8());
    file.close();

    return true;
}

void ShowContentDlg::setContent(QString& content)
{
    ui->textEdit->clear();

    QStringList contentList = SplitContent(content);
    QTextCursor cursor = ui->textEdit->textCursor();
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

    ui->textEdit->moveCursor(QTextCursor::Start, QTextCursor::MoveAnchor);
}

void ShowContentDlg::setContent(QString &title, QString &content)
{
    setWindowTitle(title);
    ui->textEdit->clear();

    QStringList contentList = SplitContent(content);
    QTextCursor cursor = ui->textEdit->textCursor();
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

    ui->textEdit->moveCursor(QTextCursor::Start, QTextCursor::MoveAnchor);
}

void ShowContentDlg::setReadOnly(bool ro)
{
    ui->textEdit->setReadOnly(ro);
}

void ShowContentDlg::setTheme()
{
    QString styleSheet = QString("QScrollBar {"
            "    border: 1px solid #999999;"
            "    background:white;"
            "    width:10px;    "
            "    margin: 0px 0px 0px 0px;"
            "}"
            "QScrollBar::handle {"
            "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
            "    stop: 0 %1, stop: 0.5 %1, stop:1 %1);"
            "    min-height: 0 px;"
            "}"
            "QScrollBar::handle:vertical {"
            "    min-height: 30px;"
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

    if (GetThemeSetting()->m_currentSelect.isEmpty())
    {
        setWindowOpacity(0.8);

        QColor scrollbar_color(220, 220, 220);

        styleSheet = styleSheet.replace("%1", scrollbar_color.name());

        setStyleSheet("background-color: rgb(240,240,240);");
        ui->textEdit->setStyleSheet("color:black");
    }
    else
    {
        Theme* theme = GetThemeSetting()->getSelectTheme();
        if (theme && !theme->type)
        {
            setWindowOpacity(theme->win_opacity);

            QColor child_win_color = ToColor(theme->child_win_color);
            if (theme->child_win_color.isEmpty())
            {
                child_win_color = ToColor(theme->win_color);
            }

            QColor scrollbar_color = ToColor(theme->scrollbar_color);
            if (theme->scrollbar_color.isEmpty())
            {
                scrollbar_color = child_win_color.darker(125);
            }

            styleSheet = styleSheet.replace("%1", scrollbar_color.name());
            setStyleSheet(QString("background-color: rgb(%1);").arg(theme->child_win_color));

            if (!theme->text_color.isEmpty())
            {
                ui->textEdit->setStyleSheet(QString("color:rgb(%1)").arg(theme->text_color));
            }
            else
            {
                QColor child_win_color = ToColor(theme->child_win_color);
                if (theme->child_win_color.isEmpty())
                {
                    child_win_color = ToColor(theme->win_color);
                }

                double brightness = GetPngBrightness(child_win_color);
                if (brightness < 100)
                {
                    ui->textEdit->setStyleSheet("color:white");
                }
                else
                {
                    ui->textEdit->setStyleSheet("color:black");
                }
            }
        }
        else
        {
            setWindowOpacity(0.8);
            QColor scrollbar_color(220, 220, 220);
            styleSheet = styleSheet.replace("%1", scrollbar_color.name());
            setStyleSheet("background-color: rgb(240,240,240);");
            ui->textEdit->setStyleSheet("color:black");
        }
    }

    ui->textEdit->verticalScrollBar()->setStyleSheet(styleSheet);
    ui->textEdit->horizontalScrollBar()->setStyleSheet(styleSheet);
}

void ShowContentDlg::setTheme(QColor& color)
{
    QString styleSheet = QString("QScrollBar {"
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

        QColor child_win_color = ToColor(theme->child_win_color);
        if (theme->child_win_color.isEmpty())
        {
            child_win_color = color;
        }

        QColor scrollbar_color = ToColor(theme->scrollbar_color);
        if (theme->scrollbar_color.isEmpty())
        {
            scrollbar_color = child_win_color.darker(125);
        }

        styleSheet = styleSheet.replace("%1", scrollbar_color.name());
        setStyleSheet(QString("background-color: rgb(%1);").arg(theme->child_win_color));

        if (!theme->text_color.isEmpty())
        {
            ui->textEdit->setStyleSheet(QString("color:rgb(%1)").arg(theme->text_color));
        }
        else
        {
            double brightness = GetPngBrightness(child_win_color);
            if (brightness < 100)
            {
                ui->textEdit->setStyleSheet("color:white");
            }
            else
            {
                ui->textEdit->setStyleSheet("color:black");
            }
        }
    }
    else
    {
        setWindowOpacity(0.8);
        QColor scrollbar_color(220, 220, 220);
        styleSheet = styleSheet.replace("%1", scrollbar_color.name());
        setStyleSheet("background-color: rgb(240,240,240);");
        ui->textEdit->setStyleSheet("color:black");
    }

    ui->textEdit->verticalScrollBar()->setStyleSheet(styleSheet);
    ui->textEdit->horizontalScrollBar()->setStyleSheet(styleSheet);
}

void ShowContentDlg::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
    {
        return;
    }

    if (event->pos().x() >= 0 && event->pos().x() <= this->width()
        && event->pos().y() >= 0 && event->pos().y() <= 30
        ) {
            m_move = true;
            m_lastPoint = event->pos();
    } else {
        m_move = false;
    }
}

void ShowContentDlg::mouseMoveEvent(QMouseEvent *event)
{
    if (m_move) {
        this->move(this->pos() + event->pos() - m_lastPoint);
    }
}

void ShowContentDlg::mouseReleaseEvent(QMouseEvent *event)
{
    m_move = false;
}
