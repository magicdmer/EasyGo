#ifndef THEMESETTING_H
#define THEMESETTING_H

#include <QObject>
#include <QVector>
#include <QColor>

struct Theme{
    int type;

    QString name;
    QString description;
    double win_opacity;
    QString win_color;
    QString child_win_color;
    QString text_color;
    QString scrollbar_color;
    QString item_selected_color;
    int title_font_size;
    int subtitle_font_size;
    int noframe;
    QString image_path;
};

class ThemeSetting
{
public:
    ThemeSetting();
public:
    bool load();
    bool save();
    Theme* getSelectTheme();

    // 解析后的颜色（含 fallback 逻辑），mainColorForImage 用于 image 主题
    QColor resolvedChildWinColor(QColor mainColorForImage = QColor());
    QColor resolvedTextColor(QColor mainColorForImage = QColor());
    QColor resolvedScrollbarColor(QColor mainColorForImage = QColor());
    QColor resolvedItemSelectedColor(QColor mainColorForImage = QColor());
    bool isDarkTheme(QColor mainColorForImage = QColor());

    // 公共样式生成
    static QString scrollbarStyleSheet(const QColor& handleColor, const QColor& bgColor, int width = 6);

public:
    QVector<Theme> m_themes;
    QString m_currentSelect;
    int m_roundCorner;
    Theme m_default_theme;
};

ThemeSetting* GetThemeSetting();

#endif // THEMESETTING_H
