#include "HelperFunc.h"
#include <QDir>
#include <Windows.h>
#include <ShlObj.h>
#include <QFileIconProvider>
#include <shellapi.h>
#include "LogFile.h"
#include <QDateTime>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkProxy>
#include <QEventLoop>
#include <QTextCodec>
#include "Settings.h"
#include <QPainter>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#define USER_AGENT "Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.198 Safari/537.36"

void CopyFileToClipboard(QString filePath)
{
    DROPFILES* dropFiles = NULL;
    QByteArray ba = filePath.toLocal8Bit();
    char* fPath = ba.data();
    HGLOBAL hGlobal = GlobalAlloc(GHND, sizeof(DROPFILES) + filePath.size());
    if (hGlobal != 0)
    {
        dropFiles = (DROPFILES*)GlobalLock(hGlobal);
        dropFiles->pFiles = sizeof(DROPFILES);
        dropFiles->fWide = FALSE;
        memcpy((PCHAR*)(dropFiles + 1), fPath, strlen(fPath));
        GlobalUnlock(hGlobal);
        if (OpenClipboard(NULL))
        {
            SetClipboardData(CF_HDROP, hGlobal);
            CloseClipboard();
        }
        else
        {
            GlobalFree(hGlobal);
        }
    }
}

bool MvoeFileToRecyclebin(const QString &filename)
{
    SHFILEOPSTRUCTA opRecycle;

    QByteArray ba = filename.toLocal8Bit();

    opRecycle.hwnd              = nullptr;
    opRecycle.wFunc             = FO_DELETE;
    opRecycle.pFrom             = ba.data();
    opRecycle.pTo               = nullptr;
    opRecycle.fFlags            = FOF_ALLOWUNDO; //此Flag表示送进回收站
    opRecycle.hNameMappings     = nullptr;

    if(SHFileOperationA(&opRecycle) != 0)
    {
        return false;
    }

    return true;
}

QPixmap GetFileIcon(QString& path)
{
    QFileInfo fileInfo(path);
    QFileIconProvider provider;

    QPixmap pixmap;

    if (!fileInfo.exists())
    {
        pixmap = provider.icon(QFileIconProvider::File).pixmap(32,32);
        return pixmap;
    }

    if (fileInfo.isDir())
    {
        pixmap = provider.icon(QFileIconProvider::Folder).pixmap(32,32);
    }
    else
    {
        pixmap = provider.icon(fileInfo).pixmap(32,32);
        if (pixmap.isNull())
        {
            pixmap = provider.icon(QFileIconProvider::File).pixmap(32,32);
        }
    }
 
    return pixmap;
}

void OpenFileFoler(QString& path)
{
    QString fileNativePath = QDir::toNativeSeparators(path);
    WCHAR filePath[MAX_PATH] = {0};
    fileNativePath.toWCharArray(filePath);
    LPITEMIDLIST pItemIdList = NULL;
    SHILCreateFromPath(filePath, &pItemIdList, NULL);
    if (pItemIdList != NULL) {
       CoInitialize(NULL);
       SHOpenFolderAndSelectItems(pItemIdList, 0, NULL, 0);
       CoTaskMemFree(pItemIdList);
       CoUninitialize();
    }
}

QString GetRandomString(int length)
{
    qsrand(QDateTime::currentMSecsSinceEpoch());//为随机值设定一个seed

    const char chrs[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    int chrs_size = sizeof(chrs);

    char* ch = new char[length + 1];
    memset(ch, 0, length + 1);
    int randomx = 0;
    for (int i = 0; i < length; ++i)
    {
        randomx= rand() % (chrs_size - 1);
        ch[i] = chrs[randomx];
    }

    QString ret(ch);
    delete[] ch;
    return ret;
}

QColor PixmapMainColor(QPixmap& p, double bright) //p为目标图片 bright为亮度系数，为1则表示保持不变
{
    QPoint realPosition = RealPointFromAlphaPng(p);

    int step = 20; //步长：在图片中取点时两点之间的间隔，若为1,则取所有点，适当将此值调大有利于提高运行速度
    int t = 0; //点数：记录一共取了多少个点，用于做计算平均数的分母
    QImage image = p.toImage(); //将Pixmap类型转为QImage类型
    int r = 0, g = 0, b = 0; //三元色的值，分别用于记录各个点的rgb值的和
    for (int i = 10; i < p.width() - 10; i += step) {
        for (int j = realPosition.y() + 5; j < p.height() - 5; j += step) {
            if (image.valid(i, j)) { //判断该点是否有效

                QColor c = image.pixelColor(i, j);
                if (c.alpha() != 0)
                {
                    t++; //点数加一

                    r += c.red(); //将获取到的各个颜色值分别累加到rgb三个变量中
                    b += c.blue();
                    g += c.green();
                }
            }
        }
    }

    return QColor(int(bright * r / t) > 255 ? 255 : int(bright * r / t),
        int(bright * g / t) > 255 ? 255 : int(bright * g / t),
        int(bright * b / t) > 255 ? 255 : int(bright * b / t)); //最后返回的值是亮度系数×平均数,若超出255则设置为255也就是最大值，防止乘与亮度系数后导致某些值大于255的情况。
}

QPoint RealPointFromAlphaPng(QPixmap& p)
{
    QPoint first_valid_point;
    QImage image = p.toImage();

    for (int i = 0; i < image.width(); i++)
    {
        for (int j = 0; j < image.height(); j++)
        {
            if (image.valid(i, j))
            {
                QColor c = image.pixelColor(i, j);
                if (c.alpha() != 0)
                {
                    first_valid_point = QPoint(i, j);
                    break;
                }
            }
        }

        if (!first_valid_point.isNull()) break;
    }

    return first_valid_point;
}

double GetPngBrightness(QColor& color)
{
    /*
    int r, g, b;

    int mc = color.name().remove("#").toInt(nullptr, 16);
    r = mc >> 16;
    g = mc >> 8 & 0xff;
    b = mc & 0xff;
    */

    double brightness = 0.299 * color.red() + 0.587 * color.green() + 0.114 * color.blue();

    return brightness;
}

QColor ToColor(QString& strRgb)
{
    if (strRgb.isEmpty()) return QColor();

    QStringList rgblist = strRgb.split(',', QString::SkipEmptyParts);
    if (rgblist.count() != 3)
    {
        return QColor();
    }

    return QColor(rgblist[0].toInt(), rgblist[1].toInt(), rgblist[2].toInt());
}

QString ToRgb(QColor color)
{
    QString str = "";
    str = QString::number(color.red())+","+QString::number(color.green())+","+QString::number(color.blue());
    return str;
}

QString HttpGet(QString& url)
{
    QNetworkRequest request;
    QNetworkAccessManager manager;
    QNetworkProxy proxy;

    if (url.startsWith("https"))
    {
        QSslConfiguration config = QSslConfiguration::defaultConfiguration();
        config.setProtocol(QSsl::AnyProtocol);
        request.setSslConfiguration(config);
    }

    Settings* cfg = GetSettings();
    if (!cfg->m_proxy_ip.isEmpty())
    {
        proxy.setType(QNetworkProxy::HttpProxy);
        proxy.setHostName(cfg->m_proxy_ip);
        proxy.setPort(cfg->m_proxy_port);
    }
    else
    {
        proxy.setType(QNetworkProxy::NoProxy);
    }

    manager.setProxy(proxy);

    request.setUrl(QUrl(url));
    request.setRawHeader("User-Agent", USER_AGENT);

    QNetworkReply* reply = manager.get(request);
    manager.setTransferTimeout(5000);
    reply->ignoreSslErrors();

    QEventLoop loop;
    QObject::connect(reply,SIGNAL(finished()),&loop,SLOT(quit()));
    loop.exec();

    QTextCodec* codec = QTextCodec::codecForName("utf8");
    QString all = codec->toUnicode(reply->readAll());
    if (all.isEmpty())
    {
        qLog(QString("HttpGet error : %1").arg(reply->errorString()));
    }
    reply->deleteLater();

    return all;
}

bool HttpDownload(QString& url , QString& dst, bool autoproxy)
{
    QNetworkRequest request;
    QNetworkAccessManager manager;
    QNetworkProxy proxy;

    if (url.startsWith("https"))
    {
        QSslConfiguration config = QSslConfiguration::defaultConfiguration();
        config.setProtocol(QSsl::AnyProtocol);
        request.setSslConfiguration(config);
    }

    if (autoproxy)
    {
        Settings* cfg = GetSettings();
        if (!cfg->m_proxy_ip.isEmpty())
        {
            proxy.setType(QNetworkProxy::HttpProxy);
            proxy.setHostName(cfg->m_proxy_ip);
            proxy.setPort(cfg->m_proxy_port);
        }
        else
        {
            proxy.setType(QNetworkProxy::NoProxy);
        }
    }

    manager.setProxy(proxy);

    request.setUrl(QUrl(url));
    request.setRawHeader("User-Agent", USER_AGENT);

    QNetworkReply* reply = manager.get(request);
    reply->ignoreSslErrors();

    QEventLoop loop;
    QObject::connect(reply,SIGNAL(finished()),&loop,SLOT(quit()));
    loop.exec();

    QByteArray arry = reply->readAll();
    reply->deleteLater();

    if (arry.isEmpty())
    {
        return false;
    }

    QFile file(dst);
    if (!file.open(QIODevice::WriteOnly))
    {
        return false;
    }
    file.write(arry);
    file.flush();
    file.close();

    return true;
}

int CompareVersion(QString& s1, QString& s2)
{
    QStringList sl1 = s1.split('.', Qt::SkipEmptyParts);
    QStringList sl2 = s2.split('.', Qt::SkipEmptyParts);

    int count = sl1.size() > sl2.size() ? sl2.size() : sl1.size();
    for (int i = 0; i < count; i++)
    {
        if (sl1[i].toInt() > sl2[i].toInt())
        {
            return 1;
        }
        else if (sl1[i].toInt() < sl2[i].toInt())
        {
            return -1;
        }
    }

    return 0;
}

QStringList SplitContent(QString& content)
{
    QStringList contentList;
    QString tmpStr;
    for(int i = 0; i < content.size(); i++)
    {
        if (content.mid(i,5) == "[pic=")
        {
            if (!tmpStr.isEmpty())
            {
                contentList.append(tmpStr);\
                tmpStr.clear();
            }

            int index = content.indexOf(']',i);
            if (index != -1)
            {
                QString picPath = content.mid(i,index - i + 1);
                contentList.append(picPath);
                i = index;
            }
        }
        else
        {
            tmpStr.append(content[i]);
        }
    }

    if (!tmpStr.isEmpty())
        contentList.append(tmpStr);

    return contentList;
}

QPixmap ConvertColor(QPixmap& pixmap, QColor color)
{
    QPixmap new_image = pixmap;
    QPainter painter(&new_image);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(new_image.rect(), color);
    painter.end();

    return new_image;
}

bool CheckUpdate(QString& version)
{
    QString repoUrl = "https://ghfast.top/https://raw.githubusercontent.com/magicdmer/EasyGo/main/repo.json";
    if (!GetSettings()->m_repo_url.isEmpty())
    {
        repoUrl = QString("%1/repo.json").arg(GetSettings()->m_repo_url);
    }

    QString jsonData = HttpGet(repoUrl);
    if (jsonData.isEmpty())
    {
        return false;
    }

    QJsonParseError parseJsonErr;
    QJsonDocument document = QJsonDocument::fromJson(jsonData.toUtf8(), &parseJsonErr);
    if (parseJsonErr.error != QJsonParseError::NoError)
    {
        return false;
    }

    QJsonObject jsonObject = document.object();
    version = jsonObject["version"].toString();

    return true;
}
