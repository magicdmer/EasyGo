#include "ThemeSetting.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include "LogFile.h"

ThemeSetting::ThemeSetting()
{
    m_default_theme.name = "";
    m_default_theme.type = 0;
    m_default_theme.description = QObject::tr("");
    m_default_theme.win_color = "240, 240, 240";
    m_default_theme.text_color = "0, 0, 0";
    m_default_theme.win_opacity = 0.8;
    m_default_theme.scrollbar_color = "220, 220, 220";
    m_default_theme.item_selected_color = "190, 190, 190";
    m_default_theme.title_font_size = 12;
    m_default_theme.subtitle_font_size = 10;
}

bool ThemeSetting::load()
{
    if (!m_themes.isEmpty())
    {
        m_themes.clear();
    }

    QFile file("Settings\\ThemeSetting.json");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return false;
    }

    QString value = file.readAll();
    file.close();

    QJsonParseError parseJsonErr;
    QJsonDocument document = QJsonDocument::fromJson(value.toUtf8(), &parseJsonErr);
    if (parseJsonErr.error != QJsonParseError::NoError)
    {
        qLog(QString("Parse ThemeSetting.json failed, error : %1").arg(parseJsonErr.errorString()));
        return false;
    }

    QJsonObject jsonObject = document.object();
    m_currentSelect = jsonObject["CurrentSelect"].toString();

    if (jsonObject.contains("RoundCorner"))
    {
        m_roundCorner = jsonObject["RoundCorner"].toInt();
    }
    else
    {
        m_roundCorner = 0;
    }

    QJsonArray themeArray = jsonObject["Themes"].toArray();
    for (int i = 0; i < themeArray.size(); i++)
    {
        Theme theme;
        QJsonObject oTheme = themeArray[i].toObject();
        if (oTheme.contains("Type"))
        {
            theme.type = oTheme["Type"].toInt();
            if (theme.type > 1) continue;
        }
        else
        {
            theme.type = 0;
        }

        if (!theme.type)
        {
            theme.win_color = oTheme["WinColor"].toString();
          
            if (oTheme.contains("NoFrame"))
            {
                theme.noframe = oTheme["NoFrame"].toInt();
            }
            else
            {
                theme.noframe = 0;
            }
        }
        else
        {
            theme.image_path = oTheme["ImagePath"].toString();
        }

        theme.name = oTheme["Name"].toString();

        if (oTheme.contains("Description"))
        {
            theme.description = oTheme["Description"].toString();
        }

        theme.win_opacity = oTheme["WinOpacity"].toDouble();

        if (oTheme.contains("TextColor"))
        {
            theme.text_color = oTheme["TextColor"].toString();
        }
        
        if (oTheme.contains("TitleFontSize"))
        {
            theme.title_font_size = oTheme["TitleFontSize"].toInt();
        }
        else
        {
            theme.title_font_size = 12;
        }

        if (oTheme.contains("SubTitleFontSize"))
        {
            theme.subtitle_font_size = oTheme["SubTitleFontSize"].toInt();
        }
        else
        {
            theme.subtitle_font_size = 10;
        }

        if (oTheme.contains("ChildWinColor"))
        {
            theme.child_win_color = oTheme["ChildWinColor"].toString();
        }

        if (oTheme.contains("ScrollBarColor"))
        {
            theme.scrollbar_color = oTheme["ScrollBarColor"].toString();
        }

        if (oTheme.contains("ItemSelectedBgColor"))
        {
            theme.item_selected_color = oTheme["ItemSelectedBgColor"].toString();
        }

        m_themes.append(theme);
    }

    return true;
}

bool ThemeSetting::save()
{
    QJsonDocument document;
    QJsonObject oRoot;
    QJsonArray themeArray;

    oRoot["CurrentSelect"] = m_currentSelect;
    oRoot["RoundCorner"] = m_roundCorner;

    for (int i = 0; i < m_themes.size(); i++)
    {
        QJsonObject obj;
        obj["Name"] = m_themes[i].name;
        obj["Description"] = m_themes[i].description;
        obj["WinOpacity"] = m_themes[i].win_opacity;
        obj["WinColor"] = m_themes[i].win_color;
        obj["ChildWinColor"] = m_themes[i].child_win_color;
        obj["TextColor"] = m_themes[i].text_color;
        obj["ScrollBarColor"] = m_themes[i].scrollbar_color;
        obj["ItemSelectedBgColor"] = m_themes[i].item_selected_color;
        obj["TitleFontSize"] = m_themes[i].title_font_size;
        obj["SubTitleFontSize"] = m_themes[i].subtitle_font_size;
        themeArray.append(obj);
    }

    oRoot["Themes"] = themeArray;

    document.setObject(oRoot);

    QByteArray byte_array = document.toJson(QJsonDocument::Indented);

    QFile file("Settings\\ThemeSetting.json");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return false;
    }
    file.write(byte_array);
    file.flush();
    file.close();

    return true;
}

Theme* ThemeSetting::getSelectTheme()
{
    Theme* theme = nullptr;

    for (int i = 0; i < m_themes.size(); i++)
    {
        if (m_themes[i].name == m_currentSelect)
        {
            theme = &m_themes[i];
            break;
        }
    }

    if (!theme)
    {
        theme = &m_default_theme;
    }

    return theme;
}

ThemeSetting* GetThemeSetting()
{
    static ThemeSetting setting;
    static bool s_initialed = false;

    if (!s_initialed)
    {
        s_initialed = true;
        setting.load();
    }

    return &setting;
}
