#ifndef MAINPLUGINDIALOG_H
#define MAINPLUGINDIALOG_H

#include <QDialog>
#include "TaskManager.h"
#include <QVariant>
#include <QSystemTrayIcon>
#include "MyListWidget.h"
#include <QListWidgetItem>
#include "SetDlg.h"
#include <QTimer>
#include "MyLineEdit.h"
#include <QProgressBar>
#include "qxtglobalshortcut.h"
#include "IconExtractor.h"
#include "ShowContentDlg.h"
#include <QColor>
#include <QPainter>
#include <QTextEdit>
#include "notifymanager.h"
#include <QMediaPlayer>
#include <QPropertyAnimation>
#include "CommonTypes.h"


class MainPluginDialog : public QDialog
{
    Q_OBJECT
public:
    MainPluginDialog(Plugin* plugin, QWidget* parent=nullptr);
    ~MainPluginDialog();

signals:
    void resultReady(QVariant);

public slots:
    void sltTextChanged(const QString &inputStr);
    void sltResultReady(QVariant);
    void sltResultError();
    void sltCancelOpr();
    void sltFilterEntries();
    void sltRightOpr();
    void sltLeftOpr();
    void sltIconExtracted(int itemIndex, QPixmap icon);
    void sltLineEditClick();
    void sltMediaStatusChanged(QMediaPlayer::MediaStatus status);

public:
    Q_INVOKABLE void setQueryText(QString queryText);
    Q_INVOKABLE QString getQueryText() { return m_lineEdit->text();}
    void setTheme();
    Q_INVOKABLE void showMsg(QString title, QString msg);
    Q_INVOKABLE void showContent(QString title, QString content);
    Q_INVOKABLE void showTip(QString title, QString msg);
    Q_INVOKABLE void editFile(QString title, QString filePath);
    Q_INVOKABLE void playMusic(QString url);
    Q_INVOKABLE void pauseMusic();
    Q_INVOKABLE void stopMusic();
    void removeSelect() {m_lineEdit->setSelection(m_lineEdit->text().size(),0);}

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void animateResize(int targetHeight, QWidget* targetWidget = nullptr);
    void hideResultUI();
    void resetInputState();
    void createTextEditMenu();
    void copyImageFromTextEdit();

private:
    QueryTask* m_task;
    MyListWidget* m_listWidget;
    MyLineEdit* m_lineEdit;
    QProgressBar* m_progressbar;
    QTextEdit* m_textEdit;
    int m_width;
    QTimer* m_typingTimer;
    QString m_filterText;
    TaskResult m_taskResult;
    int m_itemNum;
    QTimer* m_checkActive;
    QString m_hotKey;
    QxtGlobalShortcut* m_shortcut;
    IconExtractor m_iconExtractor;
    QString m_curPath;
    int m_taskId;
    QPixmap m_pixmap;
    QColor m_mainColor;
    int m_curThemeType;
    int m_initHeight;
    NotifyManager *m_notifyMgr;
    QMediaPlayer m_mediaPlayer;
    ResultItem* m_curClickedItem;
    QPixmap m_playPixmap;
    QPixmap m_pausePixmap;
    bool m_forceQuery;
    Plugin* m_plugin;
    TaskManager m_taskManager;
    QPropertyAnimation* m_resizeAnimation;
    InputState m_inputState;
};

#endif // MAINPLUGINDIALOG_H
