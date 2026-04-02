#ifndef FTPFORM_H
#define FTPFORM_H
#include<QMap>
#include <QWidget>
#include"qftp/qftp.h"
#include"qftp/qurlinfo.h"
#include <QFileInfo>
#include"qftp/qurlinfo.h"
#include <QProgressBar>
#include<QPushButton>
struct DownloadTask
{
    QString remotePath;   // 完整远程路径
    QString localPath;    // 完整本地路径
};

namespace Ui {
class ftpform;
}

class ftpform : public QWidget
{
    Q_OBJECT

public:
    explicit ftpform(QWidget *parent = nullptr);
    ~ftpform();

private slots:
    void on_FTPconnect_clicked();
    void onCommandFinished(int id, bool error);
    void onListInfo(QUrlInfo info);
    void onProgress(qint64, qint64);

private:
    Ui::ftpform *ui;
    QFtp *ftp;

    void initFtp();
    void initTable();

    // 递归下载
    void startRecursiveScan(const QString &remoteDir, const QString &localDir);
    void processNextTask();

    // 下载状态
    bool m_isDownloadingFolder = false;
    QString m_currentRemoteDir;
    QString m_currentLocalDir;
    QList<DownloadTask> m_taskList;

    // 界面文件项
    struct FileItem
    {
        QString name;
        bool isDir;
        QPushButton *btn;
        QProgressBar *bar;
        QFile *file;
    };
    QList<FileItem> m_fileList;




    void loadFtpConfig();
    QString m_ftpIP;
    int m_ftpPort;
    QString m_ftpUser;
    QString m_ftpPwd;

};
#endif // FTPFORM_H
