#ifndef HELPERFUNC_H
#define HELPERFUNC_H
#include <QObject>
#include <QByteArray>
#include <QFont>

class QWidget;

void CopyFileToClipboard(QString filePath);
QPixmap GetFileIcon(QString& path);
bool MvoeFileToRecyclebin(const QString &filename);
void OpenFileFoler(QString& path);
QString GetRandomString(int length);
// 把外部读入的路径（可能含 Windows 反斜杠）归一化为 Qt 内部统一使用的 '/' 形式。
// 注意 QDir::toNativeSeparators 在非 Windows 平台不会转换 '\\'，不能用于此目的。
QString NormalizePath(QString path);
QColor PixmapMainColor(QPixmap& p, double bright);
QPoint RealPointFromAlphaPng(QPixmap& p);
double GetPngBrightness(const QColor& color);
QColor ToColor(const QString& strRgb);
QString ToRgb(QColor color);
QString HttpGet(QString& url);
bool HttpDownload(QString& url , QString& dst, bool autoproxy);
int CompareVersion(QString& s1, QString& s2);
QStringList SplitContent(QString& content);
QPixmap ConvertColor(QPixmap& pixmap, QColor color);
bool CheckUpdate(QString& version);
QByteArray ReadZipEntry(const QString& zipPath, const QString& entryName);
bool ExtractZip(const QString& zipPath, const QString& dstPath);
QString GetUiFontFamily();
QFont GetUiFont(int pointSize = -1, int weight = -1);
bool isUos();

#endif // HELPERFUNC_H
