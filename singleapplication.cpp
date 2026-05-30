#include "singleapplication.h"
#include "HelperFunc.h"
#include <QtNetwork/QLocalSocket>
#include <QFileInfo>
#include <QMetaObject>

#define TIME_OUT   500

SingleApplication::SingleApplication(int &argc, char *argv[])
    :QApplication(argc, argv),
    w(NULL),
    localserver(NULL),
    isRunnings(false)
{
    //取应用程序名作为localServer的名字
    QString filePath = QApplication::applicationFilePath();
    QByteArray bFilePath = filePath.toUtf8();
    serverName = bFilePath.toBase64();

    InitLocalConnection();
}


/*****************************************************************
* 检查是否已有一个实例在运行, true有实例运行, false没有实例运行.
******************************************************************/
bool SingleApplication::isRunning()
{
    return isRunnings;
}


/*****************************************************************
* 通过socket通讯实现程序单实例运行,监听到新的连接时触发该函数.
******************************************************************/
void SingleApplication::NewLocalConnection()
{
    QLocalSocket *localSocket = localserver->nextPendingConnection();
    if(localSocket)
    {
        localSocket->waitForReadyRead(TIME_OUT * 2);
        const QString message = QString::fromUtf8(localSocket->readAll()).trimmed();
        delete localSocket;

        if (message == QStringLiteral("toggle"))
        {
            ToggleWindow();
        }
        else
        {
            ActivateWindow();
        }
    }
}

QString SingleApplication::startupAction() const
{
    if (isUos())
    {
        return QStringLiteral("toggle");
    }

    return arguments().contains(QStringLiteral("--toggle-window"))
            ? QStringLiteral("toggle")
            : QStringLiteral("activate");
}


/*****************************************************************
* 通过socket通讯实现程序单实例运行，
* 初始化本地连接，如果连接不上server，则创建，否则退出
******************************************************************/
void SingleApplication::InitLocalConnection()
{
    isRunnings = false;
    QLocalSocket socket;
    socket.connectToServer(serverName);
    if(socket.waitForConnected(TIME_OUT))
    {
        isRunnings = true;
        socket.write(startupAction().toUtf8());
        socket.flush();
        socket.waitForBytesWritten(TIME_OUT);
        return;
    }

    //连接不上服务器,就创建一个
    NewLocalServer();
}

/*****************************************************************
* 创建localserver
******************************************************************/
void SingleApplication::NewLocalServer()
{
    localserver = new QLocalServer(this);
    connect(localserver, &QLocalServer::newConnection, this, &SingleApplication::NewLocalConnection);
    if(!localserver->listen(serverName))
    {
        // 此时监听失败,可能是程序崩溃时,残留进程服务导致的,移除
        if(localserver->serverError() == QAbstractSocket::AddressInUseError)
        {
            localserver->removeServer(serverName);
            localserver->listen(serverName); //重新监听
        }
    }
}

/*****************************************************************
* 激活主窗口
******************************************************************/
void SingleApplication::ActivateWindow()
{
    if(w)
    {
        w->show();
        w->setWindowState(w->windowState() & ~Qt::WindowMinimized | Qt::WindowActive);
    }
}

void SingleApplication::ToggleWindow()
{
    if (w)
    {
        QMetaObject::invokeMethod(w, "sltHotKey", Qt::DirectConnection);
    }
}
