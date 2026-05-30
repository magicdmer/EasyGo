#include "IconExtractor.h"
#include "HelperFunc.h"
#include <QFileIconProvider>
#include <QDir>
#include <QFile>

#ifdef Q_OS_WIN32
#include <Windows.h>
#else
#include <QMimeDatabase>
#endif

IconExtractor::IconExtractor()
{

}

void IconExtractor::processIcons(const QVector<Result>& newItems)
{
    m_run = false;
    wait();
    m_run = true;
    m_results = newItems;
    start();
}

void IconExtractor::run()
{
#ifdef Q_OS_WIN32
    PVOID OldValue;
    Wow64DisableWow64FsRedirection (&OldValue);

    for (int i = 0; i < m_results.size() && m_run; i++)
    {
        QPixmap pixmap;
        QFileIconProvider icon;

        if (m_results[i].iconPath == "Ra_FileIcon")
        {
            pixmap = icon.icon(QFileIconProvider::File).pixmap(32,32);
        }
        else if (m_results[i].iconPath == "Ra_FolderIcon")
        {
            pixmap = icon.icon(QFileIconProvider::Folder).pixmap(32,32);
        }
        else if (m_results[i].iconPath == "Ra_DriveIcon")
        {
            pixmap = icon.icon(QFileIconProvider::Drive).pixmap(32,32);
        }
        else if (m_results[i].iconPath == "Ra_NativeIcon")
        {
            pixmap = GetFileIcon(m_results[i].subTitle);
        }
        else if (m_results[i].iconPath == "Ra_NativeIconExtra")
        {
            pixmap = GetFileIcon(m_results[i].extraData);
        }
        else
        {
            pixmap.load(m_results[i].iconPath);
        }

        emit iconExtracted(i,pixmap);
    }

    Wow64RevertWow64FsRedirection (OldValue);
#else
    for (int i = 0; i < m_results.size() && m_run; i++)
    {
        QPixmap pixmap;
        QFileIconProvider icon;

        if (m_results[i].iconPath == "Ra_FileIcon")
        {
            pixmap = icon.icon(QFileIconProvider::File).pixmap(32,32);
        }
        else if (m_results[i].iconPath == "Ra_FolderIcon")
        {
            pixmap = icon.icon(QFileIconProvider::Folder).pixmap(32,32);
        }
        else if (m_results[i].iconPath == "Ra_DriveIcon")
        {
            pixmap = icon.icon(QFileIconProvider::Drive).pixmap(32,32);
        }
        else if (m_results[i].iconPath == "Ra_NativeIcon" ||
                 m_results[i].iconPath == "Ra_NativeIconExtra")
        {
            QString filePath = (m_results[i].iconPath == "Ra_NativeIcon")
                               ? m_results[i].subTitle
                               : m_results[i].extraData;

            QMimeDatabase mimeDataBase;
            QMimeType mimeType = mimeDataBase.mimeTypeForFile(filePath);
            QString strMime = mimeType.name();
            strMime.replace("/", "-");
            QIcon appIcon = QIcon::fromTheme(strMime);
            if (!appIcon.isNull())
            {
                pixmap = appIcon.pixmap(32, 32);
            }
            else
            {
                pixmap = icon.icon(QFileIconProvider::File).pixmap(32, 32);
            }
        }
        else
        {
            QIcon appIcon = QIcon::fromTheme(m_results[i].iconPath);
            pixmap = appIcon.pixmap(32, 32);

            // Fallback: search in hicolor theme directories
            if (pixmap.isNull())
            {
                QStringList iconSizes = {"256x256@2", "256x256", "128x128", "64x64", "48x48", "32x32", "22x22", "16x16"};
                QStringList searchPaths = {"/usr/share/icons", "/usr/local/share/icons"};

                foreach(const QString& basePath, searchPaths)
                {
                    foreach(const QString& size, iconSizes)
                    {
                        // Check apps subdirectory
                        QString iconPath = QString("%1/hicolor/%2/apps/%3.png")
                            .arg(basePath, size, m_results[i].iconPath);
                        if (QFile::exists(iconPath))
                        {
                            pixmap.load(iconPath);
                            break;
                        }

                        // Also check SVG
                        iconPath = QString("%1/hicolor/%2/apps/%3.svg")
                            .arg(basePath, "scalable", m_results[i].iconPath);
                        if (QFile::exists(iconPath))
                        {
                            pixmap.load(iconPath);
                            break;
                        }
                    }
                    if (!pixmap.isNull())
                        break;
                }
            }

            if (pixmap.isNull())
            {
                pixmap.load(m_results[i].iconPath);
            }
            if (pixmap.isNull())
            {
                pixmap = icon.icon(QFileIconProvider::File).pixmap(32, 32);
            }
        }

        emit iconExtracted(i, pixmap);
    }
#endif
}

void IconExtractor::stop()
{
    m_run = false;
}
