#ifndef SETDLG_H
#define SETDLG_H

#include <QDialog>
#include <QTreeWidgetItem>
#include <QMenu>
#include <QAction>
#include "IndexTask.h"
#include "HotKeyEdit.h"
#include <QProcess>

namespace Ui {
class SetDlg;
}

class InstallTask : public QThread
{
    Q_OBJECT
public:
    InstallTask(){}
    ~InstallTask() {}
protected:
    void run()
    {
        QProcess process;

        QString pythonPath;
#ifdef Q_OS_WIN32
        if (m_pythonPath.isEmpty())
        {
            pythonPath = "python.exe";
        }
        else
        {
            pythonPath = m_pythonPath + QString("/python.exe");
        }
#else
        if (m_pythonPath.isEmpty())
        {
            pythonPath = "python3";
        }
        else
        {
            pythonPath = m_pythonPath + QString("/python3");
        }
#endif

        QStringList args;
        args.append("-m");
        args.append("pip");
        args.append("install");
        args.append("-i");
        args.append("http://mirrors.aliyun.com/pypi/simple");
        args.append("--trusted-host");
        args.append("mirrors.aliyun.com");
        args.append("-r");
        args.append(m_requirePath);

        process.setProgram(pythonPath);
        process.setArguments(args);
        process.start();

        if (!process.waitForStarted())
        {
            m_exitCode = 1;
            return;
        }

        process.waitForFinished(-1);
        m_exitCode = process.exitCode();
    }
public:
    QString m_pythonPath;
    QString m_requirePath;
    int m_exitCode;
};

class SetDlg : public QDialog
{
    Q_OBJECT

public:
    explicit SetDlg(QWidget *parent = 0);
    ~SetDlg();
protected:
    void closeEvent(QCloseEvent *event);
public slots:
    void sltItemDoubleClicked(QTreeWidgetItem * item, int column);
    void sltItemChanged(QTreeWidgetItem * item, int column);
    void sltWsItemChanged(QTreeWidgetItem* item,int column);
    void sltBrowserPath();
    void sltOpenFolder();
    void sltInstallPlugin();
    void sltUinstallPlugin();
    void sltOpenUrl();
    void sltOpenDetail();
    void sltPluginSet();
    void sltCustomContextMenuRequested(const QPoint &pos);
    void sltPushButtonAdd();
    void sltPushButtonDelete();
    void sltPushButtonIndex();
    void sltPushButtonAddUrl();
    void sltPushButtonDelUrl();
    void sltTaskFinished();
    void sltPushButtonReset();
    void sltPushButtonCheckUpdate();

public:
    void LoadPlugin();
    void AddPlugin(QString& pluginId);
    void RemovePlugin(QString& pluginId);
    void LoadPath();
    void LoadWebSearch();
private:
    Ui::SetDlg *ui;
    QMenu* m_popMenu;
    QAction* m_actionFolder;
    QAction* m_actionDetail;
    QAction* m_actionInstall;
    QAction* m_actionUninstall;
    QAction* m_actionSet;
    QAction* m_actionOpenUrl;
    IndexTask* m_task;
};

#endif // SETDLG_H
