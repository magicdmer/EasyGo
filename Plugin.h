#ifndef PLUGIN_H
#define PLUGIN_H

#include <QObject>
#include <QVector>
#include <QLibrary>

enum PluginType{
    PLUGIN_PRIVATE,
    PLUGIN_CPP,
    PLUGIN_PYTHON,
    PLUGIN_E,
    PLUGIN_POWERSHELL,
    PLUGIN_SCRIPT,
    PLUGIN_UNKOWN
};

enum PluginMode{
    RealMode,
    EnterMode
};

enum ShowType{
    SHOW_TYPE_DEFAULT,
    SHOW_TYPE_RICHTEXT,
    SHOW_TYPE_MUSIC
};

#define PROGRAM_PLUGIN_ID "00000000000000000000000000000001"
#define WEBSEARCH_PLUGIN_ID "00000000000000000000000000000002"
#define EPM_PLUGIN_ID "00000000000000000000000000000003"
#define OPTION_PLUGIN_ID "00000000000000000000000000000004"
#define THEME_PLUGIN_ID "00000000000000000000000000000005"

typedef bool (*pInitFunc)(char* pPluginPath);
typedef bool (*pMenuFunc)(char* result,char* pMenu, int* length);
typedef bool (*pQueryFunc)(char* pQuery, char* pResult, int* length);
typedef void (*pUpdateSetting)();
typedef void (*pClickFunc)(char* pData);

typedef bool (__stdcall *pEInitFunc)(char* pPluginPath);
typedef const char* (__stdcall *pEMenuFunc)(char* result);
typedef const char* (__stdcall *pEQueryFunc)(char* pQuery);
typedef void (__stdcall *pEUpdateSetting)();
typedef void (__stdcall *pEClickFunc)(char* pData);

struct Query{
    QString rawQuery;
    QString keyword;
    QString parameter;
};

struct PluginAction{
    QString funcName;
    QString parameter;
    bool hideWindow;

    PluginAction(){
        hideWindow = true;
    }
};

struct Result{
    QString id;
    int showType;
    QString title;
    QString subTitle;
    QString iconPath;
    QString extraData;
    PluginAction action;
};
Q_DECLARE_METATYPE(Result)

struct PluginInfo{
    QString id;
    QStringList keyword;
    int argc;
    QString name;
    QString description;
    QString author;
    QString version;
    QString pluginType;
    QString pluginMode;  //EnterMode,RealMode
    QString acceptType;
    int enableSeparate;
    QString webSite;
    QString iconPath;
    QString interpreter;
    QString interpreterArgv;
    QString exeName;
    QString cfgPath;
};

#ifdef Q_OS_WIN32
#define PLUGIN_API extern "C" __declspec(dllexport)
#define EASYGO_API  __stdcall
#pragma comment(linker, "/export:Ra_ChangeQuery=_Ra_ChangeQuery@4")
#pragma comment(linker, "/export:Ra_Reload=_Ra_Reload@0")
#pragma comment(linker, "/export:Ra_ReQuery=_Ra_ReQuery@0")
#pragma comment(linker, "/export:Ra_ShowMsg=_Ra_ShowMsg@8")
#pragma comment(linker, "/export:Ra_ShowTip=_Ra_ShowTip@8")
#pragma comment(linker, "/export:Ra_ShowContent=_Ra_ShowContent@8")
#pragma comment(linker, "/export:Ra_EditFile=_Ra_EditFile@8")
#pragma comment(linker, "/export:Ra_PlayMusic=_Ra_PlayMusic@4")
#else
#define PLUGIN_API extern "C" __attribute__((visibility("default")))
#define EASYGO_API
#endif

PLUGIN_API void EASYGO_API Ra_ChangeQuery(const char* strQuery);
PLUGIN_API void EASYGO_API Ra_Reload();
PLUGIN_API void EASYGO_API Ra_ReQuery();
PLUGIN_API void EASYGO_API Ra_ShowMsg(const char* title,const char* msg);
PLUGIN_API void EASYGO_API Ra_ShowTip(const char* title , const char* msg);
PLUGIN_API void EASYGO_API Ra_ShowContent(const char* title,const char* msg);
PLUGIN_API void EASYGO_API Ra_EditFile(const char* title,const char* filePath);
PLUGIN_API void EASYGO_API Ra_PlayMusic(const char* musicPath);

class Plugin : public QObject
{
    Q_OBJECT
public:
    Plugin():m_initok(true),m_type(PLUGIN_PRIVATE),m_mode(RealMode) {}
    Plugin(QString& pluginPath):m_path(pluginPath),m_initok(true)
    {
        load();
    }
    virtual ~Plugin() {}

public:
    //EasyGo启动的时候调用
    virtual bool initPlugin(QString pluginPath) = 0;
    virtual bool query(Query query,QVector<Result>& vecResult)  = 0;
    virtual bool getMenu(Result& result, QVector<Result>& vecMenu)  = 0;
    virtual void updateSetting() {}
    virtual void itemClick(Result& item,QObject* parent = nullptr) = 0;

public:
    bool m_initok;
    QString m_path;
    PluginInfo m_info;
    QString m_guid;
    QString m_iconPath;
    PluginType m_type;
    PluginMode m_mode;
    QString m_cfgPath;

private:
    bool load();

protected:
    Q_INVOKABLE void Ra_TopResult(QString parameter,QObject* parent = nullptr);
    Q_INVOKABLE void Ra_DownResult(QString parameter,QObject* parent = nullptr);

    Q_INVOKABLE void Ra_ActivePlugin(QString parameter,QObject* parent = nullptr);
    Q_INVOKABLE void Ra_ShowMsg(QString parameter,QObject* parent = nullptr);
    Q_INVOKABLE void Ra_ShowTip(QString parameter,QObject* parent = nullptr);
    Q_INVOKABLE void Ra_ChangeQuery(QString parameter,QObject* parent = nullptr);
    Q_INVOKABLE void Ra_ChangeQueryPara(QString parameter,QObject* parent = nullptr);
    Q_INVOKABLE void Ra_Reload(QString parameter,QObject* parent = nullptr);
    Q_INVOKABLE void Ra_ReQuery(QString parameter,QObject* parent = nullptr);
    Q_INVOKABLE void Ra_ShowContent(QString parameter,QObject* parent = nullptr);

    Q_INVOKABLE void Ra_CopyPath(QString data,QObject* parent = nullptr);
    Q_INVOKABLE void Ra_Copy(QString data,QObject* parent = nullptr);
    Q_INVOKABLE void Ra_CopyImage(QString data,QObject* parent = nullptr);
    Q_INVOKABLE void Ra_Delete(QString data,QObject* parent = nullptr);
    Q_INVOKABLE void Ra_OpenFileFolder(QString data,QObject* parent = nullptr);
    Q_INVOKABLE void Ra_Open(QString data,QObject* parent = nullptr);
    Q_INVOKABLE void Ra_OpenWeb(QString data,QObject* parent = nullptr);
    Q_INVOKABLE void Ra_Recycle(QString data,QObject* parent = nullptr);
    Q_INVOKABLE void Ra_EditFile(QString data,QObject* parent = nullptr);

    Q_INVOKABLE void Ra_Install(QString data,QObject* parent = nullptr);
    Q_INVOKABLE void Ra_Uninstall(QString data,QObject* parent = nullptr);

    Q_INVOKABLE void Ra_PlayMusic(QString musicPath,QObject* parent = nullptr);
    Q_INVOKABLE void Ra_PauseMusic(QString parameter,QObject* parent = nullptr);
    Q_INVOKABLE void Ra_StopMusic(QString parameter,QObject* parent = nullptr);
};


class CPlusPlugin : public Plugin
{
public:
    CPlusPlugin(QString& pluginPath):Plugin(pluginPath) {}

public:
    //EasyGo启动的时候调用
    bool initPlugin(QString pluginPath);
    bool query(Query query,QVector<Result>& vecResult);
    bool getMenu(Result& result, QVector<Result>& vecMenu);
    void updateSetting();
    void itemClick(Result& item,QObject* parent);
    void dettach() {m_library.unload();}

private:
    pQueryFunc m_query;
    pMenuFunc m_menuFunc;
    pUpdateSetting m_updateSetting;
    QLibrary m_library;
};

class PythonPlugin : public Plugin
{
public:
    PythonPlugin(QString& pluginPath);

public:
    //EasyGo启动的时候调用
    bool initPlugin(QString pluginPath);
    bool query(Query query,QVector<Result>& vecResult);
    bool getMenu(Result& result,QVector<Result>& vecMenu);
    void itemClick(Result& item,QObject* parent);
    QString execute(QString funcName,QString parameter);

private:
    QString m_pythonPath;
    QString m_jsonRpcPath;
};

class EPlugin : public Plugin
{
public:
    EPlugin(QString& pluginPath):Plugin(pluginPath) {}

public:
    //EasyGo启动的时候调用
    bool initPlugin(QString pluginPath);
    bool query(Query query,QVector<Result>& vecResult);
    bool getMenu(Result& result, QVector<Result>& vecMenu);
    void updateSetting();
    void itemClick(Result& item,QObject* parent);
    void dettach() {m_library.unload();}

private:
    pEQueryFunc m_query;
    pEMenuFunc m_menuFunc;
    pEUpdateSetting m_updateSetting;
    QLibrary m_library;
};

class PowerShellPlugin : public Plugin
{
public:
    PowerShellPlugin(QString& pluginPath);

public:
    //EasyGo启动的时候调用
    bool initPlugin(QString pluginPath);
    bool query(Query query,QVector<Result>& vecResult);
    bool getMenu(Result& result,QVector<Result>& vecMenu);
    void itemClick(Result& item,QObject* parent);
    QString execute(QString funcName,QString parameter);
};

class ScriptPlugin : public Plugin
{
public:
    ScriptPlugin(QString& pluginPath);

public:
    //EasyGo启动的时候调用
    bool initPlugin(QString pluginPath);
    bool query(Query query,QVector<Result>& vecResult);
    bool getMenu(Result& result,QVector<Result>& vecMenu);
    void itemClick(Result& item,QObject* parent);
    QString execute(QString funcName,QString parameter);
};
#endif // PLUGIN_H
