#ifndef PLUGINSETDLG_H
#define PLUGINSETDLG_H

#include <QDialog>
#include <QTreeWidget>

namespace Ui {
class PluginSetDlg;
}

class PluginSetDlg : public QDialog
{
    Q_OBJECT

public:
    explicit PluginSetDlg(QString filePath, QWidget *parent = nullptr);
    ~PluginSetDlg();
public:
    void initSettings();
    void saveSettings();
public slots:
    void sltItemDoubleClicked(QTreeWidgetItem * item, int column);
    void sltItemChanged(QTreeWidgetItem * item, int column);
private:
    Ui::PluginSetDlg *ui;
    QString m_filePath;
};

#endif // PLUGINSETDLG_H
