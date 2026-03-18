#-------------------------------------------------
#
# Project created by QtCreator 2019-06-15T15:53:56
#
#-------------------------------------------------

QT       += core gui sql network concurrent multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = EasyGo
TEMPLATE = app
TRANSLATIONS  = EasyGo.en.ts

win32 {
    #QMAKE_LFLAGS += /MANIFESTUAC:\"level=\'requireAdministrator\' uiAccess=\'false\'\"
    QMAKE_CXXFLAGS += /MP
    RC_FILE = EasyGo.rc
}

win32:CONFIG(release, debug|release): {

    contains(QT_ARCH, x86_64){
        INCLUDEPATH += QuaZIP/x64/include
        LIBS += -LQuaZIP/x64/lib -lQt5Quazip1
    }else{
        INCLUDEPATH += QuaZIP/x86/include
        LIBS += -LQuaZIP/x86/lib -lQt5Quazip1
    }

    LIBS += -lAdvapi32 -lshell32 -lole32 -luser32
}
else:win32:CONFIG(debug, debug|release): {

    contains(QT_ARCH, x86_64){
        INCLUDEPATH += QuaZIP/x64/include
        LIBS += -LQuaZIP/x64/lib -lQt5Quazip1
    }else{
        INCLUDEPATH += QuaZIP/x86/include
        LIBS += -LQuaZIP/x86/lib -lQt5Quazip1
    }
    LIBS += -lAdvapi32 -lshell32 -lole32 -luser32
}

include(QxtGobalShortcut/qxtglobalshortcut.pri)
include(QtNotify2/qtnotify2.pri)

SOURCES += main.cpp\
    EpmPlugin.cpp \
    FastSearch.cpp \
    HotKeyEdit.cpp \
    IconExtractor.cpp \
    IndexTask.cpp \
    InstallDlg.cpp \
    MainDialog.cpp \
    MainPluginDialog.cpp \
    MyLineEdit.cpp \
    MyListWidget.cpp \
    OptionPlugin.cpp \
    PluginManager.cpp \
    PluginSetDlg.cpp \
    QueryTask.cpp \
    ResultItem.cpp \
    Plugin.cpp \
    Settings.cpp \
    ShowContentDlg.cpp \
    TaskManager.cpp \
    ThemePlugin.cpp \
    ThemeSetting.cpp \
    TopMostRecord.cpp \
    UsageSetting.cpp \
    UserSelectRecord.cpp \
    LogFile.cpp \
    SetDlg.cpp \
    ChineseLetterHelper.cpp \
    IndexDatabase.cpp \
    HelperFunc.cpp \
    ProgramPlugin.cpp \
    WebSearchPlugin.cpp \
    WebSearchSetDlg.cpp \
    AboutDialog.cpp

HEADERS  += MainDialog.h \
    CommonTypes.h \
    EpmPlugin.h \
    FastSearch.h \
    HotKeyEdit.h \
    IconExtractor.h \
    IndexTask.h \
    InstallDlg.h \
    MainPluginDialog.h \
    MyLineEdit.h \
    MyListWidget.h \
    OptionPlugin.h \
    PluginManager.h \
    PluginSetDlg.h \
    QueryTask.h \
    ResultItem.h \
    Plugin.h \
    Settings.h \
    ShowContentDlg.h \
    TaskManager.h \
    ThemePlugin.h \
    ThemeSetting.h \
    TopMostRecord.h \
    UsageSetting.h \
    UserSelectRecord.h \
    LogFile.h \
    SetDlg.h \
    ChineseLetterHelper.h \
    IndexDatabase.h \
    HelperFunc.h \
    ProgramPlugin.h \
    WebSearchPlugin.h \
    WebSearchSetDlg.h \
    AboutDialog.h

FORMS    += MainDialog.ui \
    InstallDlg.ui \
    PluginSetDlg.ui \
    ResultItem.ui \
    SetDlg.ui \
    ShowContentDlg.ui \
    WebSearchSetDlg.ui \
    AboutDialog.ui

RESOURCES += \
    EasyGo.qrc
