#include "IndexTask.h"
#include <QDir>
#include <QFileInfo>
#include "ChineseLetterHelper.h"
#include "LogFile.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
# pragma execution_character_set("utf-8")
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
        }
    }
}

void IndexTask::run()
{
    m_indexing = true;

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

    m_indexing = false;
}
