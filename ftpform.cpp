#include "ftpform.h"
#include "ui_ftpform.h"
#include <QMessageBox>
#include <QDir>
#include <QProgressBar>
#include <QPushButton>
#include<QDebug>
#include<QTimer>
#include <QSettings>

ftpform::ftpform(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ftpform)
{
    ui->setupUi(this);
    loadFtpConfig();
    initFtp();
    initTable();
}

ftpform::~ftpform()
{
    delete ui;
}

void ftpform::initFtp()
{
    ftp = new QFtp(this);
    connect(ftp, &QFtp::commandFinished, this, &ftpform::onCommandFinished);
    connect(ftp, &QFtp::listInfo, this, &ftpform::onListInfo);
    connect(ftp, &QFtp::dataTransferProgress, this, &ftpform::onProgress);
}

void ftpform::initTable()
{
    ui->tableWidget->setColumnCount(6);
    ui->tableWidget->setHorizontalHeaderLabels({"序号", "名称", "日期", "大小", "操作", "进度"});
    ui->tableWidget->verticalHeader()->setVisible(false);
    ui->tableWidget->setColumnWidth(4, 110);
    ui->tableWidget->setColumnWidth(5, 160);
    ui->tableWidget->setAlternatingRowColors(false); // 可选，让界面更干净
    ui->tableWidget->setStyleSheet("QTableWidget::item { alignment: center; }");
}

void ftpform::on_FTPconnect_clicked()
{

    if (m_ftpIP.isEmpty()) {
        QMessageBox::warning(this, "提示", "配置文件中IP不能为空！");
        return;
    }

    ftp->connectToHost(m_ftpIP, m_ftpPort);
    ftp->login(m_ftpUser, m_ftpPwd);
}

void ftpform::onCommandFinished(int, bool error)
{
    if (error) {
        qDebug() << "FTP 错误：" << ftp->errorString();
        return;
    }

    if (ftp->currentCommand() == QFtp::Login) {
        QMessageBox::information(this, "成功", "登录成功！");
        ui->tableWidget->setRowCount(0);
        m_fileList.clear();
        ftp->list("/");
        return;
    }

    if (ftp->currentCommand() == QFtp::List) {
        if (m_isDownloadingFolder) {
            processNextTask();
        }
        return;
    }

    if (ftp->currentCommand() == QFtp::Get) {
        for (auto &item : m_fileList) {
            if (item.btn && item.btn->text() == "下载中...") {
                if (item.file) {
                    item.file->close();
                    item.file->deleteLater();
                    item.file = nullptr;
                }
                item.btn->setText("已完成");
                item.btn->setStyleSheet("border:none; padding:5px 12px; background:#2E8B57; color:white; border-radius:3px;");
                break;
            }
        }
        processNextTask();
    }
}

void ftpform::onProgress(qint64 read, qint64 total)
{
    for (auto &item : m_fileList) {
        if (item.btn && item.btn->text() == "下载中...") {
            item.bar->setMaximum(static_cast<int>(total));
            item.bar->setValue(static_cast<int>(read));
            break;
        }
    }
}

// ==============================
// 递归扫描文件夹（安全模式）
// ==============================
void ftpform::startRecursiveScan(const QString &remoteDir, const QString &localDir)
{
    // ========================
       // 自动重连（修复掉线）
       // ========================
       if (ftp->state() == QFtp::Unconnected) {
           qDebug() << "FTP 已断开，自动重连中...";
           // 重新连接（IP 填你的 FTP 服务器 IP）
           ftp->connectToHost(m_ftpIP, m_ftpPort);
           ftp->login(m_ftpUser, m_ftpPwd);


           // 重连后再下载
           QTimer::singleShot(1000, this, [=]() {
               startRecursiveScan(remoteDir, localDir);
           });
           return;
       }

       qDebug() << "\n=====================================";
       qDebug() << "📂 开始下载文件夹：";
       qDebug() << "远程路径：" << remoteDir;
       qDebug() << "本地路径：" << localDir;
       qDebug() << "=====================================\n";

       m_isDownloadingFolder = true;
       m_currentRemoteDir = remoteDir;
       m_currentLocalDir = localDir;
       QDir().mkpath(localDir);
       ftp->cd(remoteDir);
       ftp->list();
}

// ==============================
// 执行下一个下载任务
// ==============================
void ftpform::processNextTask()
{
    if (m_taskList.isEmpty()) {
        m_isDownloadingFolder = false;
        m_currentRemoteDir.clear();
        m_currentLocalDir.clear();
        qDebug() << " 全部下载完成！";
        return;
    }

    DownloadTask task = m_taskList.first();
    m_taskList.pop_front();

    // 如果是文件夹
    if (task.remotePath.endsWith("/")) {
        qDebug() << "处理文件夹：" << task.remotePath;

        QDir().mkpath(task.localPath);
        m_currentRemoteDir = task.remotePath;
        m_currentLocalDir = task.localPath;

        ftp->cd(task.remotePath);
        ftp->list();
    }
    // 如果是文件
    else {
        qDebug() << "开始下载：" << task.localPath;

        QFile *file = new QFile(task.localPath);
        if (!file->open(QIODevice::WriteOnly)) {
            file->deleteLater();
            processNextTask();
            return;
        }
        ftp->get(task.remotePath, file);

    }
}

void ftpform::loadFtpConfig()
{
    QSettings config("ftpconfig.ini", QSettings::IniFormat);
    config.setIniCodec("UTF-8");

    // 读取配置（没有则自动创建默认值）
    m_ftpIP   = config.value("FTP/IP",   "127.0.0.1").toString();
    m_ftpPort = config.value("FTP/Port", 21).toInt();
    m_ftpUser = config.value("FTP/User", "test").toString();
    m_ftpPwd  = config.value("FTP/Pwd",  "123456").toString();

    qDebug() << "加载FTP配置：" << m_ftpIP << m_ftpPort << m_ftpUser;
}

// ==============================
// 列表信息（完全隔离界面/下载）
// ==============================
void ftpform::onListInfo(QUrlInfo info)
{
    if (info.name() == "." || info.name() == "..")
        return;

    if (m_isDownloadingFolder) {
        if (info.isDir()) {
            DownloadTask task;
            task.remotePath = m_currentRemoteDir + info.name() + "/";
            task.localPath = m_currentLocalDir + info.name() + "/";
            m_taskList.append(task);
        } else {
            DownloadTask task;
            task.remotePath = m_currentRemoteDir + info.name();
            task.localPath = m_currentLocalDir + info.name();
            m_taskList.append(task);

            qDebug() << "加入文件：" << task.localPath;
        }
        return;
    }
    // ================== 正常界面显示 ==================
    int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);
    ui->tableWidget->setItem(row, 0, new QTableWidgetItem(QString::number(row + 1)));
    ui->tableWidget->item(row, 0)->setTextAlignment(Qt::AlignCenter);

    bool isDir = info.isDir();
    ui->tableWidget->setItem(row, 1, new QTableWidgetItem(isDir ? "📁 " + info.name() : info.name()));
    ui->tableWidget->item(row, 1)->setTextAlignment(Qt::AlignCenter);

    ui->tableWidget->setItem(row, 2, new QTableWidgetItem(info.lastModified().toString("yyyy-MM-dd HH:mm")));
    ui->tableWidget->item(row, 2)->setTextAlignment(Qt::AlignCenter);

    if (isDir) {
        ui->tableWidget->setItem(row, 3, new QTableWidgetItem("文件夹"));
        ui->tableWidget->item(row, 3)->setTextAlignment(Qt::AlignCenter);

    } else {
        qint64 s = info.size();
        QString sz;
        if (s >= 1024*1024*1024) sz = QString::number(s/(1024*1024*1024.0),'f',2)+" GB";
        else if (s >= 1024*1024) sz = QString::number(s/(1024*1024.0),'f',2)+" MB";
        else if (s >= 1024) sz = QString::number(s/1024.0,'f',2)+" KB";
        else sz = QString::number(s)+" B";
        ui->tableWidget->setItem(row, 3, new QTableWidgetItem(sz));
        ui->tableWidget->item(row, 3)->setTextAlignment(Qt::AlignCenter);

    }

    QWidget *btnWidget = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(btnWidget);
    layout->setContentsMargins(6, 3, 6, 3); // 边距

    QPushButton *btn = new QPushButton();
    btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout->addWidget(btn);

    QProgressBar *bar = new QProgressBar();
    bar->setValue(0);

    if (!isDir) {
        btn->setText("下载");
        btn->setStyleSheet(R"(
            QPushButton {
                border:none;
                padding:5px 12px;
                background:#409EFF;
                color:white;
                border-radius:3px;
            }
            QPushButton:hover {
                background:#3388EE;
            }
        )");
    } else {
        btn->setText("下载文件夹");
        btn->setStyleSheet("QPushButton{border:none;padding:5px 12px;background:#FF6666;color:white;border-radius:3px;}QPushButton:hover{background:#EE5555;}");
    }

    ui->tableWidget->setCellWidget(row, 4, btnWidget);
    ui->tableWidget->setCellWidget(row, 5, bar);

    FileItem item;
    item.name = info.name();
    item.isDir = isDir;
    item.btn = btn;
    item.bar = bar;
    item.file = nullptr;
    m_fileList.append(item);

    connect(btn, &QPushButton::clicked, this, [=]() {
        if (btn->text() == "已完成" || btn->text() == "下载中...")
            return;

        btn->setText("下载中...");
        btn->setStyleSheet("border:none;padding:5px 12px;background:#FF9500;color:white;border-radius:3px;");

        if (!isDir) {
            QDir().mkpath("D:/FTP_DOWNLOAD/");
            QString path = "D:/FTP_DOWNLOAD/" + info.name();
            QFile *f = new QFile(path);
            if (f->open(QIODevice::WriteOnly)) {
                for (auto &i : m_fileList) {
                    if (i.name == info.name() && !i.isDir)
                        i.file = f;
                }
                ftp->get(info.name(), f);
            }
        } else {
            QString remote = "/" + info.name() + "/";
            QString local = "D:/FTP_DOWNLOAD/" + info.name() + "/";
            startRecursiveScan(remote, local);
        }
    });
}
