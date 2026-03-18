#include "MainDialog.h"
#include <QApplication>
#include <QSharedMemory>
#include <QDir>
#include <QFontDatabase>
#include "LogFile.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
# pragma execution_character_set("utf-8")
#endif

bool g_IsDebug = false;

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#if (QT_VERSION > QT_VERSION_CHECK(5,14,0))
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif
    QApplication a(argc, argv);
    QSharedMemory mem("EasyGo");
	if(!mem.create(1))
	{
		return 0;
	}

    a.setQuitOnLastWindowClosed(false);

	QDir::setCurrent(a.applicationDirPath());

    qclearLog();

    if (GetSettings()->m_debugMode)
    {
        g_IsDebug = true;
        qLog("------Debug Mode------");
    }

    QFont f;
    QFontDatabase database;
    QStringList fontList = database.families(QFontDatabase::SimplifiedChinese);
    if (!fontList.isEmpty())
    {
        if (fontList.indexOf("微软雅黑") != -1)
        {
            f.setFamily("微软雅黑");
            f.setPointSize(9);
            a.setFont(f);
        }
        else
        {
            if (fontList.indexOf(f.defaultFamily()) != -1)
            {
                f.setFamily(f.defaultFamily());
                f.setPointSize(9);
                a.setFont(f);
            }
            else
            {
                f.setFamily(fontList[fontList.size()-1]);
                f.setPointSize(9);
                a.setFont(f);
            }
        }
    }

    HDC hdc = GetDC(NULL);
    int LogicalScreenHeight = GetDeviceCaps(hdc, VERTRES);
    int PhysicalScreenHeight = GetDeviceCaps(hdc, DESKTOPVERTRES);
    qreal dpi = (float)PhysicalScreenHeight / (float)LogicalScreenHeight;

    MainDialog w;
    w.setFactor(dpi);

    int ret = a.exec();
    if (ret == 773)
    {
        mem.detach();
        QProcess::startDetached(qApp->applicationFilePath(), QStringList());
        return 0;
    }

    return ret;
}
