#ifndef HELPERFUNC_H
#define HELPERFUNC_H
#include <QObject>

void CopyFileToClipboard(QString filePath);
QPixmap GetFileIcon(QString& path);
bool MvoeFileToRecyclebin(const QString &filename);
void OpenFileFoler(QString& path);
QString GetRandomString(int length);
QColor PixmapMainColor(QPixmap& p, double bright);
QPoint RealPointFromAlphaPng(QPixmap& p);
double GetPngBrightness(QColor& color);
QColor ToColor(QString& strRgb);
QString ToRgb(QColor color);
QString HttpGet(QString& url);
bool HttpDownload(QString& url , QString& dst, bool autoproxy);
int CompareVersion(QString& s1, QString& s2);
QStringList SplitContent(QString& content);
QPixmap ConvertColor(QPixmap& pixmap, QColor color);
bool CheckUpdate(QString& version);

#endif // HELPERFUNC_H
