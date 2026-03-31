#ifndef FTPFORM_H
#define FTPFORM_H
#include<QMap>
#include <QWidget>
#include"qftp/qftp.h"
#include"qftp/qurlinfo.h"
#include <QFileInfo>
#include"qftp/qurlinfo.h"
#include <QProgressBar>
namespace Ui {
class ftpform;
}

class ftpform : public QWidget
{
    Q_OBJECT

public:
    void initftp();
    void initTable();
    explicit ftpform(QWidget *parent = nullptr);
    ~ftpform();

    void downloadFile(QString remoteFile);
    void scanDirRecursive(QString path);
    void startDownloadDir(QString dirPath);

private slots:
    void on_FTPconnect_clicked();
    void onCommandFinished(int, bool);
    void onListInfo(QUrlInfo);
    void onProgress(qint64 read, qint64 total);

private:
    Ui::ftpform *ui;
    QFtp *ftp;
    QString localRootDir;   // 本地保存根目录
    QString remoteRootDir; // 远程FTP目录
    QMap<QString, QUrlInfo> fileInfoMap; // 文件信息
    qint64 totalBytes;     // 总大小
    qint64 finishedBytes;  // 已下载大小
    bool isDownloading;    // 是否正在下载

    QStringList downloadQueue;
    int currentFileIndex;

    QString currentDownloadRemoteDir;  // 当前正在下载的远程目录
    QString currentDownloadLocalDir;   // 对应本地目录
    void downloadFolder(const QString &remoteFolder, const QString &localFolder);
};

#endif // FTPFORM_H
