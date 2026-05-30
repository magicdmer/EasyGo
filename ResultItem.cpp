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

void ResultItem::setTheme(QColor mainColorForImage)
{
    Theme* theme = GetThemeSetting()->getSelectTheme();
    if (!theme) return;

    QColor textColor = GetThemeSetting()->resolvedTextColor(mainColorForImage);

    // 副标题颜色比标题略淡
    QColor subTitleColor = textColor;
    subTitleColor.setAlpha(160);

    ui->labelTitle->setStyleSheet(QString("color: %1;").arg(textColor.name()));
    // 副标题使用 180/255 透明度，确保在复杂背景上也有足够对比度
    ui->labelSubTitle->setStyleSheet(QString("color: rgba(%1, %2, %3, 180);")
                                         .arg(textColor.red())
                                         .arg(textColor.green())
                                         .arg(textColor.blue()));

    ui->labelTitle->setFont(GetUiFont(theme->title_font_size, QFont::Bold));
    ui->labelSubTitle->setFont(GetUiFont(theme->subtitle_font_size, QFont::Normal));
}
