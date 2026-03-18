#include "ThemePlugin.h"
#include <QDir>
#include "ThemeSetting.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
# pragma execution_character_set("utf-8")
#endif

ThemePlugin::ThemePlugin()
{
    m_info.id = OPTION_PLUGIN_ID;
    m_info.name = tr("EasyGo主题设置");
    m_info.keyword.append("/theme");
    m_info.pluginType = "c++";
    m_info.author = "magicdmer";
    m_info.enableSeparate = 0;
    m_iconPath = QDir::currentPath() + QString("/Images/color.png");
    m_guid = m_info.id;
    m_mode = RealMode;
}

bool ThemePlugin::query(Query query,QVector<Result>& vecResult)
{
    Q_UNUSED(query);


    Result result;
    result.id = THEME_PLUGIN_ID;
    result.title = tr("目前主题是: %1").arg(
                GetThemeSetting()->m_currentSelect.isEmpty() ? "默认主题" : GetThemeSetting()->m_currentSelect);
    result.subTitle = tr("点击/回车选择列表中主题后重启/重载软件才生效");
    result.iconPath = m_iconPath;
    vecResult.append(result);

    if (GetThemeSetting()->m_roundCorner)
    {
        result.title = tr("关闭圆角");
        result.subTitle = tr("关闭圆角，win10风格");
        result.iconPath = m_iconPath;
        result.action.funcName = "setRoundCorner";
        result.action.parameter = "0";
        result.action.hideWindow = false;
        vecResult.append(result);
    }
    else
    {
        result.title = tr("开启圆角");
        result.subTitle = tr("开启圆角，win11风格");
        result.iconPath = m_iconPath;
        result.action.funcName = "setRoundCorner";
        result.action.parameter = "1";
        result.action.hideWindow = false;
        vecResult.append(result);
    }

    result.title = tr("默认主题");
    result.subTitle = tr("默认主题");
    result.iconPath = m_iconPath;
    result.action.funcName = "setTheme";
    result.action.parameter = "";
    result.action.hideWindow = false;
    vecResult.append(result);

    foreach (Theme theme, GetThemeSetting()->m_themes)
    {
        result.title = theme.name;
        result.subTitle = theme.description;
        result.iconPath = m_iconPath;
        result.action.funcName = "setTheme";
        result.action.parameter = theme.name;
        result.action.hideWindow = false;
        vecResult.append(result);
    }

    return true;
}

bool ThemePlugin::getMenu(Result &result, QVector<Result> &vecMenu)
{
    Q_UNUSED(result);
    Q_UNUSED(vecMenu);

    return false;
}

void ThemePlugin::itemClick(Result &item, QObject *parent)
{
    PluginAction& action = item.action;
    QByteArray ba = action.funcName.toUtf8();
    QMetaObject::invokeMethod(this,ba.data(),Q_ARG(QString,action.parameter),Q_ARG(QObject*,parent));
}

void ThemePlugin::setTheme(QString name, QObject* parent)
{
    GetThemeSetting()->m_currentSelect = name;
    GetThemeSetting()->save();
    Ra_ReQuery("",parent);
    Ra_Reload("", parent);
}

void ThemePlugin::setRoundCorner(QString roundCorner, QObject* parent)
{
    GetThemeSetting()->m_roundCorner = roundCorner.toInt();
    GetThemeSetting()->save();
    Ra_ReQuery("",parent);
    Ra_Reload("", parent);
}
