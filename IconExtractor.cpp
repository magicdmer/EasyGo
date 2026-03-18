#include "IconExtractor.h"
#include "HelperFunc.h"
#include <QFileIconProvider>
#include <Windows.h>

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
}

void IconExtractor::stop()
{
    m_run = false;
}
