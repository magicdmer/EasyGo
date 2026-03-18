#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include <QDialog>
#include "QueryTask.h"
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
#include "ThemeSetting.h"

namespace Ui {
class MainDialog;
}

class MainDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MainDialog(QWidget *parent = 0);
    ~MainDialog();

signals:
    void resultReady(QVariant);

public slots:
    void sltTextChanged(const QString &inputStr);
    void sltResultReady(QVariant);
	void sltResultError();
    void sltCancelOpr();
    void sltExit();
    void sltOpen();
    void sltSet();
    Q_INVOKABLE void sltReload();
    void sltAbout();
    void sltHotKey();
    void sltFilterEntries();
    void sltRightOpr();
    void sltLeftOpr();
    void sltCheckAtivate();
    void sltIconExtracted(int itemIndex, QPixmap icon);
    void sltTrayActived(QSystemTrayIcon::ActivationReason reason);
    void sltLineEditClick();
    void sltMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void sltCheckUpdate();

public:
    Q_INVOKABLE void setQueryText(QString queryText);
    Q_INVOKABLE QString getQueryText() { return m_lineEdit->text();}
    void setTheme();
    Q_INVOKABLE void showMsg(QString title, QString msg);
    Q_INVOKABLE void showContent(QString title, QString content);
    Q_INVOKABLE void showTip(QString title, QString msg);
    void installPlugin(QString& filePath);
    Q_INVOKABLE void editFile(QString title, QString filePath);
    Q_INVOKABLE void playMusic(QString url);
    Q_INVOKABLE void pauseMusic();
    Q_INVOKABLE void stopMusic();
    void setFactor(qreal factor) { m_factor = factor;}

protected:
    void mousePressEvent(QMouseEvent *me) override;
    void mouseMoveEvent(QMouseEvent *me) override;
    void mouseReleaseEvent(QMouseEvent *me) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void animateResize(int targetHeight, QWidget* targetWidget = nullptr);
    void hideResultUI();
    void resetInputState();
    void createTextEditMenu();
    void copyImageFromTextEdit();

private:
    Ui::MainDialog *ui;
    QueryTask* m_task;
    QSystemTrayIcon* m_pTrayIcon;
    QAction* m_setting;
    QAction* m_reload;
    QAction* m_open;
    QAction* m_about;
    QAction* m_exit;
    MyListWidget* m_listWidget;
    MyLineEdit* m_lineEdit;
    QProgressBar* m_progressbar;
    QTextEdit* m_textEdit;
	bool m_move;
    QPoint m_lastPoint;
	int m_width;
    QTimer* m_typingTimer;
    QString m_filterText;
    TaskResult m_taskResult;
    int m_itemNum;
    QTimer* m_checkActive;
    QString m_hotKey;
    QxtGlobalShortcut* m_shortcut;
    IconExtractor m_iconExtractor;
    int m_taskId;
    QPixmap m_pixmap;
    QColor m_mainColor;
    int m_curThemeType;
    int m_roundCorner;
    int m_initHeight;
    NotifyManager *m_notifyMgr;
    QMediaPlayer m_mediaPlayer;
    ResultItem* m_curClickedItem;
    QPixmap m_playPixmap;
    QPixmap m_pausePixmap;
    bool m_forceQuery;
    qreal m_factor;
    QPropertyAnimation* m_resizeAnimation;
    InputState m_inputState;
};

#endif // MAINDIALOG_H
