#ifndef THEMESETTING_H
#define THEMESETTING_H

#include <QObject>
#include <QVector>

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
public:
    QVector<Theme> m_themes;
    QString m_currentSelect;
    int m_roundCorner;
    Theme m_default_theme;
};

ThemeSetting* GetThemeSetting();

#endif // THEMESETTING_H
