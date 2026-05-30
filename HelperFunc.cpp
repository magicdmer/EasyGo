#include "HelperFunc.h"
#include <QDir>
#include <QFileIconProvider>
#include "LogFile.h"
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
#include <QProcess>
#include <QDateTime>
#include <QFontDatabase>
#include <QWidget>

#ifdef Q_OS_WIN32
#include "quazip.h"
#include "quazipfile.h"
#include "JlCompress.h"
#endif

#ifdef Q_OS_WIN32
#include <Windows.h>
#include <ShlObj.h>
#include <shellapi.h>
#else
#include <QTextStream>
#include <QUrl>
#endif

namespace {

QString readOsReleaseValue(const QString& key)
{
    QFile file(QStringLiteral("/etc/os-release"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return QString();
    }

    while (!file.atEnd())
    {
        const QString line = QString::fromUtf8(file.readLine()).trimmed();
        if (!line.startsWith(key + QLatin1Char('=')))
        {
            continue;
        }

        QString value = line.mid(key.size() + 1).trimmed();
        if (value.startsWith(QLatin1Char('"')) && value.endsWith(QLatin1Char('"')) && value.size() >= 2)
        {
            value = value.mid(1, value.size() - 2);
        }
        return value;
    }

    return QString();
}

} // namespace

#define USER_AGENT "Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.198 Safari/537.36"

QString GetUiFontFamily()
{
#ifdef Q_OS_WIN32
    return QStringLiteral("微软雅黑");
#else
    const QStringList preferredFonts = {
        QStringLiteral("Source Han Sans SC"),
        QStringLiteral("Noto Sans CJK SC"),
        QStringLiteral("WenQuanYi Micro Hei"),
        QStringLiteral("Noto Sans CJK TC"),
        QStringLiteral("Noto Sans CJK JP")
    };

    QFontDatabase database;
    const QStringList families = database.families();
    for (const QString& family : preferredFonts)
    {
        if (families.contains(family))
        {
            return family;
        }
    }

    QString fallback = QFontDatabase::systemFont(QFontDatabase::GeneralFont).family();
    if (!fallback.isEmpty())
    {
        return fallback;
    }

    return QStringLiteral("Sans Serif");
#endif
}

QFont GetUiFont(int pointSize, int weight)
{
    QFont font(GetUiFontFamily());
    font.setStyleHint(QFont::SansSerif);
    if (pointSize > 0)
    {
        font.setPointSize(pointSize);
    }
    if (weight >= 0)
    {
        font.setWeight(weight);
    }
    return font;
}

#ifdef Q_OS_WIN32
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
    opRecycle.fFlags            = FOF_ALLOWUNDO;
    opRecycle.hNameMappings     = nullptr;

    if(SHFileOperationA(&opRecycle) != 0)
    {
        return false;
    }

    return true;
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
#else
void CopyFileToClipboard(QString filePath)
{
    Q_UNUSED(filePath)
}

bool MvoeFileToRecyclebin(const QString &fileName)
{
    QFileInfo fileInfo(fileName);
    if (!fileInfo.exists())
    {
        return false;
    }

    QString xdgDataHome = QFile::decodeName(qgetenv("XDG_DATA_HOME"));
    QString trashPath = xdgDataHome.isEmpty()
            ? QDir::homePath() + "/.local/share/Trash/"
            : xdgDataHome + "/Trash/";
    QString trashFilesPath = trashPath + "files/";
    QString trashInfoPath = trashPath + "info/";

    QDir dir;
    if (!(dir.mkpath(trashFilesPath) && dir.mkpath(trashInfoPath)))
    {
        return false;
    }

    QString trashFileName = fileInfo.fileName();
    QString trashFilePath = trashFilesPath + trashFileName;
    if (QFile::exists(trashFilePath))
    {
        int suffixNumber = 1;
        do
        {
            trashFileName = QString("%1.%2").arg(fileInfo.fileName()).arg(suffixNumber++);
            trashFilePath = trashFilesPath + trashFileName;
        } while (QFile::exists(trashFilePath));
    }

    QFile file(fileName);
    if (!file.rename(trashFilePath))
    {
        return false;
    }

    QFile infoFile(trashInfoPath + trashFileName + ".trashinfo");
    if (!infoFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QFile::rename(trashFilePath, fileName);
        return false;
    }

    QTextStream stream(&infoFile);

    QByteArray info = "[Trash Info]\n";
    info += "Path=";
    info += QUrl::toPercentEncoding(QFileInfo(fileName).absoluteFilePath(), "~_-./");
    info += '\n';
    info += "DeletionDate=";
    info += QDateTime::currentDateTime().toString(Qt::ISODate).toLatin1();
    info += '\n';

    stream << info;
    infoFile.close();

    return true;
}

void OpenFileFoler(QString& path)
{
    QFileInfo info(path);
    if (!info.exists())
    {
        return;
    }

    QString folder = info.isDir() ? info.absoluteFilePath() : info.absolutePath();
    QProcess::startDetached("/usr/bin/xdg-open", QStringList() << folder);
}
#endif

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

QString NormalizePath(QString path)
{
    // Qt 内部各 API 在所有平台都接受 '/'，故统一转成 '/'，并清理多余分隔符与 . / ..。
    path.replace('\\', '/');
    return QDir::cleanPath(path);
}

QString GetRandomString(int length)
{
    qsrand(QDateTime::currentMSecsSinceEpoch());

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

QColor PixmapMainColor(QPixmap& p, double bright)
{
    QPoint realPosition = RealPointFromAlphaPng(p);

    int step = 20;
    int t = 0;
    QImage image = p.toImage();
    int r = 0, g = 0, b = 0;
    for (int i = 10; i < p.width() - 10; i += step) {
        for (int j = realPosition.y() + 5; j < p.height() - 5; j += step) {
            if (image.valid(i, j)) {

                QColor c = image.pixelColor(i, j);
                if (c.alpha() != 0)
                {
                    t++;

                    r += c.red();
                    b += c.blue();
                    g += c.green();
                }
            }
        }
    }

    return QColor(int(bright * r / t) > 255 ? 255 : int(bright * r / t),
        int(bright * g / t) > 255 ? 255 : int(bright * g / t),
        int(bright * b / t) > 255 ? 255 : int(bright * b / t));
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

double GetPngBrightness(const QColor& color)
{
    double brightness = 0.299 * color.red() + 0.587 * color.green() + 0.114 * color.blue();

    return brightness;
}

QColor ToColor(const QString& strRgb)
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
#if (QT_VERSION >= QT_VERSION_CHECK(5,15,0))
    manager.setTransferTimeout(5000);
#endif
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

QByteArray ReadZipEntry(const QString& zipPath, const QString& entryName)
{
#ifdef Q_OS_WIN32
    QuaZip zip(zipPath);
    if (!zip.open(QuaZip::mdUnzip))
    {
        return QByteArray();
    }

    if (!zip.setCurrentFile(entryName))
    {
        zip.close();
        return QByteArray();
    }

    QuaZipFile zipfile(&zip);
    if (!zipfile.open(QIODevice::ReadOnly))
    {
        zip.close();
        return QByteArray();
    }

    QByteArray content = zipfile.readAll();
    zipfile.close();
    zip.close();
    return content;
#else
    QProcess process;
    process.start("unzip", QStringList() << "-p" << zipPath << entryName);
    if (!process.waitForFinished(-1) || process.exitCode() != 0)
    {
        qLog(QString("Read zip entry failed: %1/%2, %3")
             .arg(zipPath, entryName, QString::fromLocal8Bit(process.readAllStandardError())));
        return QByteArray();
    }

    return process.readAllStandardOutput();
#endif
}

bool ExtractZip(const QString& zipPath, const QString& dstPath)
{
#ifdef Q_OS_WIN32
    return !JlCompress::extractDir(zipPath, dstPath).isEmpty();
#else
    QDir().mkpath(dstPath);

    QProcess process;
    process.start("unzip", QStringList() << "-o" << zipPath << "-d" << dstPath);
    if (!process.waitForFinished(-1) || process.exitCode() != 0)
    {
        qLog(QString("Extract zip failed: %1 -> %2, %3")
             .arg(zipPath, dstPath, QString::fromLocal8Bit(process.readAllStandardError())));
        return false;
    }

    return true;
#endif
}

int CompareVersion(QString& s1, QString& s2)
{
    QStringList sl1 = s1.split('.', QString::SkipEmptyParts);
    QStringList sl2 = s2.split('.', QString::SkipEmptyParts);

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
                contentList.append(tmpStr);
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
    QString repoUrl = "https://ghfast.top/https://raw.githubusercontent.com/magicdmer/EasyGoPlugin/main/repo.json";
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

bool isUos()
{
#ifdef Q_OS_LINUX
    static const bool isUosSystem = []() {
        const QString id = readOsReleaseValue(QStringLiteral("ID")).trimmed().toLower();
        if (id == QStringLiteral("uos"))
        {
            return true;
        }

        const QString name = readOsReleaseValue(QStringLiteral("NAME")).trimmed().toLower();
        return name == QStringLiteral("uos");
    }();

    return isUosSystem;
#else
    return false;
#endif
}
