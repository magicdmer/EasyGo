#include "IndexTask.h"
#include <QDir>
#include <QFileInfo>
#include <QCryptographicHash>
#include "ChineseLetterHelper.h"
#include "LogFile.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
# pragma execution_character_set("utf-8")
#endif

#ifdef Q_OS_LINUX
#include <QSettings>
#include <QStandardPaths>
#include <libintl.h>
#endif

bool IndexTask::m_indexing = false;

void IndexTask::index()
{
	m_run = true;
	start();
}

void IndexTask::stop()
{
	m_run = false;
	wait();
}

#ifdef Q_OS_LINUX
static QStringList getDefaultLinuxIndexPaths()
{
    QStringList paths;
    paths.append("/usr/share/applications");
    paths.append(QString("%1/.local/share/applications").arg(QDir::homePath()));
    paths.append("/usr/share/ubuntu/applications");
    paths.append("/var/lib/snapd/desktop/applications");

    return paths;
}

static bool hasAppImageSuffix(const QString& filePath)
{
    return QFileInfo(filePath).suffix().compare("AppImage", Qt::CaseInsensitive) == 0;
}

static bool isAppImageLauncher(const QFileInfo& fileInfo)
{
    if (hasAppImageSuffix(fileInfo.filePath()))
    {
        return fileInfo.isExecutable();
    }

    if (fileInfo.isSymLink())
    {
        QFileInfo targetInfo(fileInfo.symLinkTarget());
        return hasAppImageSuffix(targetInfo.filePath()) && targetInfo.isExecutable();
    }

    return false;
}

static QString normalizedDesktopExec(QSettings& desktopFile)
{
    QString exec = desktopFile.value("/Desktop Entry/Exec").toString();
    exec = exec.replace(QRegExp("%[a-zA-Z]"), "").trimmed();
    return exec.split(' ', QString::SkipEmptyParts).join(' ');
}

static QString firstCommandToken(const QString& command)
{
    QString trimmed = command.trimmed();
    if (trimmed.isEmpty())
    {
        return QString();
    }

    if (trimmed.startsWith('"') || trimmed.startsWith('\''))
    {
        const QChar quote = trimmed[0];
        const int end = trimmed.indexOf(quote, 1);
        if (end > 0)
        {
            return trimmed.mid(1, end - 1);
        }
    }

    return trimmed.split(' ', QString::SkipEmptyParts).first();
}

static QString appImageDedupeKeyFromPath(const QString& filePath)
{
    const QString canonicalPath = QFileInfo(filePath).canonicalFilePath();
    return QString("appimage:%1").arg(canonicalPath.isEmpty() ? filePath : canonicalPath);
}

static QString desktopDedupeKey(const QString& desktopFilePath)
{
    QSettings desktopFile(desktopFilePath, QSettings::IniFormat);
    desktopFile.setIniCodec("UTF8");

    const QString exec = normalizedDesktopExec(desktopFile);
    const QString execPath = firstCommandToken(exec);
    if (hasAppImageSuffix(execPath))
    {
        return appImageDedupeKeyFromPath(execPath);
    }

    if (!exec.isEmpty())
    {
        return QString("desktop:%1").arg(exec);
    }

    const QString canonicalPath = QFileInfo(desktopFilePath).canonicalFilePath();
    return QString("desktop-file:%1").arg(canonicalPath.isEmpty() ? desktopFilePath : canonicalPath);
}

static QString appImageDedupeKey(const QFileInfo& fileInfo)
{
    const QString sourcePath = fileInfo.isSymLink() ? fileInfo.symLinkTarget() : fileInfo.filePath();
    return appImageDedupeKeyFromPath(sourcePath);
}

static QString appImageThumbnailPath(const QString& filePath)
{
    const QString uri = QString("file://%1").arg(filePath);
    const QByteArray hash = QCryptographicHash::hash(uri.toUtf8(), QCryptographicHash::Md5).toHex();
    const QString cacheRoot = QString("%1/.cache/thumbnails").arg(QDir::homePath());
    QStringList thumbnailPaths;
    thumbnailPaths.append(QString("%1/large/%2.png").arg(cacheRoot, QString::fromLatin1(hash)));
    thumbnailPaths.append(QString("%1/normal/%2.png").arg(cacheRoot, QString::fromLatin1(hash)));

    foreach (const QString& thumbnailPath, thumbnailPaths)
    {
        if (QFileInfo::exists(thumbnailPath))
        {
            return thumbnailPath;
        }
    }

    return QString();
}

static void getAppImageProgramInfo(const QFileInfo& fileInfo, ProgramInfo& info)
{
    info.name = fileInfo.completeBaseName();
    if (info.name.isEmpty())
    {
        info.name = fileInfo.fileName();
    }

    info.pyname = ChineseLetterHelper::GetFirstLetters(info.name);
    info.path = fileInfo.filePath();
    const QString iconSourcePath = fileInfo.isSymLink() ? fileInfo.symLinkTarget() : fileInfo.filePath();
    info.iconPath = appImageThumbnailPath(iconSourcePath);
    if (info.iconPath.isEmpty())
    {
        info.iconPath = "application-x-executable";
    }
    info.desktopPath.clear();
}

bool IndexTask::shouldSkipDesktopEntry(QSettings &desktopFile)
{
    // 无 Exec 无法启动
    if (!desktopFile.contains("/Desktop Entry/Exec"))
    {
        return true;
    }

    // 仅索引应用类型；Type 缺省视为 Application
    QString type = desktopFile.value("/Desktop Entry/Type").toString();
    if (!type.isEmpty() && type.compare("Application", Qt::CaseInsensitive) != 0)
    {
        return true;
    }

    // NoDisplay / Hidden：不应展示或已被用户“删除”的条目
    if (desktopFile.value("/Desktop Entry/NoDisplay").toString()
            .compare("true", Qt::CaseInsensitive) == 0)
    {
        return true;
    }
    if (desktopFile.value("/Desktop Entry/Hidden").toString()
            .compare("true", Qt::CaseInsensitive) == 0)
    {
        return true;
    }

    // OnlyShowIn / NotShowIn：按当前桌面环境过滤（如 xscreensaver 的 OnlyShowIn=MATE）
    QStringList currentDesktops =
        QString::fromLocal8Bit(qgetenv("XDG_CURRENT_DESKTOP"))
            .split(':', QString::SkipEmptyParts);
    QString onlyShowIn = desktopFile.value("/Desktop Entry/OnlyShowIn").toString();
    if (!onlyShowIn.isEmpty())
    {
        bool match = false;
        foreach (const QString &env, onlyShowIn.split(';', QString::SkipEmptyParts))
        {
            if (currentDesktops.contains(env, Qt::CaseInsensitive))
            {
                match = true;
                break;
            }
        }
        if (!match)
        {
            return true;
        }
    }
    QString notShowIn = desktopFile.value("/Desktop Entry/NotShowIn").toString();
    if (!notShowIn.isEmpty())
    {
        foreach (const QString &env, notShowIn.split(';', QString::SkipEmptyParts))
        {
            if (currentDesktops.contains(env, Qt::CaseInsensitive))
            {
                return true;
            }
        }
    }

    // 屏保等非日常应用类别
    QStringList categories = desktopFile.value("/Desktop Entry/Categories")
            .toString().split(';', QString::SkipEmptyParts);
    if (categories.contains("Screensaver", Qt::CaseInsensitive))
    {
        return true;
    }

    // TryExec：指定的可执行文件不存在则不索引
    QString tryExec = desktopFile.value("/Desktop Entry/TryExec").toString().trimmed();
    if (!tryExec.isEmpty())
    {
        bool exists = tryExec.startsWith('/')
                ? QFileInfo::exists(tryExec)
                : !QStandardPaths::findExecutable(tryExec).isEmpty();
        if (!exists)
        {
            return true;
        }
    }

    return false;
}

bool IndexTask::appendProgramInfo(const ProgramInfo& info, const QString& dedupeKey)
{
    if (info.name.isEmpty())
    {
        return false;
    }

    if (!dedupeKey.isEmpty())
    {
        if (m_seenProgramKeys.contains(dedupeKey))
        {
            return false;
        }
        m_seenProgramKeys.insert(dedupeKey);
    }

    m_programs.append(info);
    return true;
}

void IndexTask::getProgramInfo(QString desktopFilePath, ProgramInfo &info)
{
    QSettings desktopFile(desktopFilePath, QSettings::IniFormat);
    desktopFile.setIniCodec("UTF8");

    if (shouldSkipDesktopEntry(desktopFile))
    {
        return;
    }

    QString appName = desktopFile.value("/Desktop Entry/Name").toString();
    if (desktopFile.contains("/Desktop Entry/X-Ubuntu-Gettext-Domain"))
    {
        QString domain = desktopFile.value("/Desktop Entry/X-Ubuntu-Gettext-Domain").toString();
        QByteArray bDomain = domain.toUtf8();
        QByteArray bAppName = appName.toUtf8();
        appName = QString::fromUtf8(dgettext(bDomain.data(), bAppName.data()));
    }
    else if (desktopFile.contains("/Desktop Entry/Name[zh_CN]"))
    {
        appName = desktopFile.value("/Desktop Entry/Name[zh_CN]").toString();
    }

    info.name = appName;
    info.pyname = ChineseLetterHelper::GetFirstLetters(info.name);
    info.desktopPath = desktopFilePath;     // 启动时交给 gio launch 按规范处理

    QString exec = normalizedDesktopExec(desktopFile);
    QString execPath = firstCommandToken(exec);
    if (execPath.startsWith("/"))
    {
        info.path = execPath;
    }
    else
    {
        info.path = QString("/usr/bin/") + execPath;
    }

    QString icon = desktopFile.value("/Desktop Entry/Icon").toString();
    if (icon.isEmpty())
    {
        info.iconPath = "application-x-executable";
    }
    else
    {
        info.iconPath = icon;
    }
}

#endif

void IndexTask::indexProgram(QString strDir)
{
	if (!m_run)
	{
		return;
	}

    QDir sourceDir(strDir);
    if(!sourceDir.exists()){
        return;
    }
    sourceDir.setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);

    QFileInfoList fileInfoList = sourceDir.entryInfoList();
    foreach(QFileInfo fileInfo, fileInfoList){
        if(fileInfo.isDir()){
            indexProgram(fileInfo.filePath());
        }
        else{
#ifdef Q_OS_WIN32
            if (fileInfo.suffix() == "exe" ||
                fileInfo.suffix() == "lnk" ||
                fileInfo.suffix() == "bat")
            {
                ProgramInfo info;
                info.name = fileInfo.fileName();
                info.path = fileInfo.filePath();
                info.pyname = ChineseLetterHelper::GetFirstLetters(info.name);
                m_programs.append(info);
            }
#else
            if (fileInfo.suffix() == "desktop")
            {
                ProgramInfo info;
                getProgramInfo(fileInfo.filePath(), info);
                appendProgramInfo(info, desktopDedupeKey(fileInfo.filePath()));
            }
            else if (isAppImageLauncher(fileInfo))
            {
                ProgramInfo info;
                getAppImageProgramInfo(fileInfo, info);
                appendProgramInfo(info, appImageDedupeKey(fileInfo));
            }
#endif
        }
    }
}

void IndexTask::run()
{
    m_indexing = true;

#ifdef Q_OS_WIN32
    IndexDatabase database("IndexTask", "IndexCacheTemp.db");
    database.load();
    database.clear();

    QStringList& indexPath = GetSettings()->m_indexCfg.indexPath;
    for (int i = 0; i < indexPath.size() && m_run; i++)
    {
        wchar_t drive[10] = {0};
        indexPath[i].toWCharArray(drive);

        qLog(QString("Index drive %1 ...").arg(indexPath[i]));

        if (GetSettings()->m_indexCfg.enableNormalIndex)
        {
            indexProgram(indexPath[i]);
            if (!m_programs.isEmpty())
            {
                database.insert(m_programs);
                m_programs.clear();
            }
        }
        else
        {
            USN nextusn;
            if (!fs->Traverse(drive,&nextusn))
            {
                qLog("Index from USN failed,use normal traverse");
                indexProgram(indexPath[i]);
                if (!m_programs.isEmpty())
                {
                    database.insert(m_programs);
                }
            }
            else
            {
                fs->SaveToDataBase(&database);
            }
        }
    }
#else
    IndexDatabase database("IndexTask");
    database.load();
    database.clear();
    m_seenProgramKeys.clear();

    QStringList indexPath = GetSettings()->m_indexCfg.indexPath;
    if (indexPath.isEmpty())
    {
        indexPath = getDefaultLinuxIndexPaths();
    }

    for (int i = 0; i < indexPath.size() && m_run; i++)
    {
        qLog(QString("Index path %1 ...").arg(indexPath[i]));
        indexProgram(indexPath[i]);
    }

    if (!m_programs.isEmpty())
    {
        database.insert(m_programs);
        m_programs.clear();
    }
#endif

    m_indexing = false;
}
