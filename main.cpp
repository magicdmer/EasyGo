#include "MainDialog.h"
#include "singleapplication.h"
#include <QStyleFactory>
#include <QDir>
#include <QFontDatabase>
#include <QScreen>
#include <QProcess>
#include "LogFile.h"
#include "Settings.h"
#include "HelperFunc.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
# pragma execution_character_set("utf-8")
#endif

#ifdef Q_OS_WIN32
#include <windows.h>
#endif

bool g_IsDebug = false;

int main(int argc, char *argv[])
{
    SingleApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    SingleApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#if (QT_VERSION > QT_VERSION_CHECK(5,14,0))
    SingleApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif
    SingleApplication a(argc, argv);
    if (a.isRunning())
        return 0;

#ifdef Q_OS_LINUX
    // Linux 原生 style 不绘制忙碌进度条动画等效果，全局统一用 Fusion
    a.setStyle(QStyleFactory::create("Fusion"));
#endif

    a.setQuitOnLastWindowClosed(false);

    QDir::setCurrent(a.applicationDirPath());

    qclearLog();

    if (GetSettings()->m_debugMode)
    {
        g_IsDebug = true;
        qLog("------Debug Mode------");
    }

    a.setFont(GetUiFont(9));

#ifdef Q_OS_WIN32
    HDC hdc = GetDC(NULL);
    int LogicalScreenHeight = GetDeviceCaps(hdc, VERTRES);
    int PhysicalScreenHeight = GetDeviceCaps(hdc, DESKTOPVERTRES);
    qreal dpi = (float)PhysicalScreenHeight / (float)LogicalScreenHeight;
#else
    qreal dpi = QApplication::primaryScreen()->devicePixelRatio();
#endif

    MainDialog w;
    w.setFactor(dpi);
    a.w = &w;

    int ret = a.exec();
    if (ret == 773)
    {
        QProcess::startDetached(qApp->applicationFilePath(), QStringList());
        return 0;
    }

    return ret;
}