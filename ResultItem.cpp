#include "ResultItem.h"
#include "ui_ResultItem.h"
#include <QDesktopWidget>
#include <QFileIconProvider>
#include "ThemeSetting.h"
#include "HelperFunc.h"

ResultItem::ResultItem(QWidget *parent,Result* result) :
    QWidget(parent),
    m_result(*result),
    ui(new Ui::ResultItem)
{
    ui->setupUi(this);

	int primary = QApplication::desktop()->primaryScreen();
	int iwidth = QApplication::desktop()->screen(primary)->width();
	setGeometry(0,0,iwidth/5*2 - 20, 60);

    setTheme();

    ui->labelTitle->setText(m_result.title);
    ui->labelSubTitle->setText(m_result.subTitle);

    m_uuid = m_result.id;
}

ResultItem::~ResultItem()
{
    delete ui;
}

void ResultItem::updateIcon(QPixmap& pixmap)
{
    ui->labelAppIcon->setPixmap(pixmap);
    ui->labelAppIcon->setScaledContents(true);
}

void ResultItem::update()
{
    ui->labelTitle->setText(m_result.title);
    ui->labelSubTitle->setText(m_result.subTitle);

    m_uuid = m_result.id;
}

void ResultItem::setTheme()
{
    Theme* theme = GetThemeSetting()->getSelectTheme();
    if (theme && !theme->type)
    {
        if (!theme->text_color.isEmpty())
        {
            ui->labelTitle->setStyleSheet(QString("color:rgb(%1)").arg(theme->text_color));
            ui->labelSubTitle->setStyleSheet(QString("color:rgb(%1)").arg(theme->text_color));
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
                ui->labelTitle->setStyleSheet("color: white");
                ui->labelSubTitle->setStyleSheet("color: white");
            }
            else
            {
                ui->labelTitle->setStyleSheet("color: black");
                ui->labelSubTitle->setStyleSheet("color: black");
            }
        }

        ui->labelTitle->setFont(QFont(tr("Arial"), theme->title_font_size, QFont::Bold));
        ui->labelSubTitle->setFont(QFont(tr("Arial"), theme->subtitle_font_size, QFont::Bold));
    }
}


void ResultItem::setTheme(QColor& color)
{
    Theme* theme = GetThemeSetting()->getSelectTheme();
    if (theme)
    {
        if (!theme->text_color.isEmpty())
        {
            ui->labelTitle->setStyleSheet(QString("color:rgb(%1)").arg(theme->text_color));
            ui->labelSubTitle->setStyleSheet(QString("color:rgb(%1)").arg(theme->text_color));
        }
        else
        {
            double brightness = GetPngBrightness(color);
            if (brightness < 100)
            {
                ui->labelTitle->setStyleSheet("color: white");
                ui->labelSubTitle->setStyleSheet("color: white");
            }
            else
            {
                ui->labelTitle->setStyleSheet("color: black");
                ui->labelSubTitle->setStyleSheet("color: black");
            }
        }

        ui->labelTitle->setFont(QFont(tr("Arial"), theme->title_font_size, QFont::Bold));
        ui->labelSubTitle->setFont(QFont(tr("Arial"), theme->subtitle_font_size, QFont::Bold));
    }
}
