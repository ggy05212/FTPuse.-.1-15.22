#include "ftpform.h"
#include "ui_ftpform.h"
#include <QMessageBox>
#include <QDebug>
#include <QFile>
#include <QPushButton>
#include <QProgressBar>
#include <QDir>

struct FileItem {
    QString name;
    QPushButton *btn;
    QProgressBar *bar;
    QFile *file;
    bool isDir;
    char padding[3] = {0};
};

static QList<FileItem> g_fileList;
static QString g_currentRemotePath = "/";

void ftpform::initftp()
{
    ftp = new QFtp(this);
    connect(ftp, SIGNAL(commandFinished(int,bool)),this, SLOT(onCommandFinished(int,bool)));
    connect(ftp, SIGNAL(listInfo(QUrlInfo)),this, SLOT(onListInfo(QUrlInfo)));
    connect(ftp, SIGNAL(dataTransferProgress(qint64,qint64)),this,SLOT(onProgress(qint64,qint64)));

}

void ftpform::initTable()
{
    ui->tableWidget->setColumnCount(6);
    QStringList headers;
    headers << "序号" << "名称" << "日期" << "大小" << "操作" << "进度";
    ui->tableWidget->setHorizontalHeaderLabels(headers);
    ui->tableWidget->verticalHeader()->setVisible(false);
    ui->tableWidget->setColumnWidth(4, 110);
    ui->tableWidget->setColumnWidth(5, 160);
}

ftpform::ftpform(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ftpform)
{
    ui->setupUi(this);
    initftp();
    initTable();
}

ftpform::~ftpform()
{
    delete ui;
}

void ftpform::on_FTPconnect_clicked()
{
    QString ip = ui->widget->getIP();
    if (ip.isEmpty() || ip == "...") {
        QMessageBox::warning(this, "提示", "IP不能为空！");
        return;
    }

    ftp->connectToHost(ip, 21);
    ftp->login("test", "123456");
}

void ftpform::onCommandFinished(int, bool error)
{
    if (error) return;

    if (ftp->currentCommand() == QFtp::Login) {
        QMessageBox::information(this, "成功", "登录成功！");
        ui->tableWidget->setRowCount(0);
        g_fileList.clear();
        ftp->list(g_currentRemotePath);
    }
    else if (ftp->currentCommand() == QFtp::Get) {
        // 下载完成逻辑移到这里
        for (auto &item : g_fileList) {
            if (item.btn && item.btn->text() == "下载中...") {
                if (item.file) {
                    item.file->close();
                    item.file->deleteLater();
                    item.file = nullptr;
                }
                item.btn->setText("已完成");
                item.btn->setStyleSheet(R"(
                    border:none; padding:5px 12px; background:#2E8B57; color:white; border-radius:3px;
                )");
                break;
            }
        }
    }
}

void ftpform::onProgress(qint64 read, qint64 total)
{
    for (auto &item : g_fileList) {
        if (item.btn && item.btn->text() == "下载中...") {
            item.bar->setMaximum(static_cast<int>(total));
                        item.bar->setValue(static_cast<int>(read));
            break;
        }
    }
}



// ==============================================
// 核心：显示文件夹 + 文件
// ==============================================
void ftpform::onListInfo(QUrlInfo info)
{
    if (info.name() == "." || info.name() == "..") return;

    int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);

    ui->tableWidget->setItem(row, 0, new QTableWidgetItem(QString::number(row + 1)));

    QString showName = info.name();
    bool isDir = info.isDir();

    if (isDir) {
        showName = "📁 " + showName;
    }

    ui->tableWidget->setItem(row, 1, new QTableWidgetItem(showName));
    ui->tableWidget->setItem(row, 2, new QTableWidgetItem(info.lastModified().toString("yyyy-MM-dd HH:mm")));

    if (isDir) {
        ui->tableWidget->setItem(row, 3, new QTableWidgetItem("文件夹"));
    } else {
        qint64 size = info.size();
        QString sizeStr;
        if (size >= 1024*1024*1024) sizeStr = QString::number(size/(1024*1024*1024.0),'f',2)+" GB";
        else if (size >= 1024*1024) sizeStr = QString::number(size/(1024*1024.0),'f',2)+" MB";
        else if (size >= 1024)sizeStr = QString::number(size/1024.0, 'f', 2) + " KB";
        else sizeStr = QString::number(size)+" B";
        ui->tableWidget->setItem(row, 3, new QTableWidgetItem(sizeStr));
    }

    QPushButton *btn = nullptr;
    QProgressBar *bar = new QProgressBar();
    bar->setValue(0);

    if (!isDir) {
        btn = new QPushButton("下载");
        btn->setStyleSheet(R"(
            QPushButton {
                border:none; padding:5px 12px; background:#409EFF; color:white; border-radius:3px;
            }
            QPushButton:hover { background:#3388EE; }
        )");
    }

    ui->tableWidget->setCellWidget(row, 4, btn);
    ui->tableWidget->setCellWidget(row, 5, bar);

    FileItem item;
    item.name = info.name();
    item.isDir = isDir;
    item.btn = btn;
    item.bar = bar;
    item.file = nullptr;
    g_fileList.append(item);

    if (btn) {
        connect(btn, &QPushButton::clicked, this, [=]() {
            if (btn->text() == "已完成" || btn->text() == "下载中...") return;

            btn->setText("下载中...");
            btn->setStyleSheet(R"(
                border:none; padding:5px 12px; background:#FF9500; color:white; border-radius:3px;
            )");

            QDir dir;
            QString savePath = "D:/FTP_DOWNLOAD/";
            dir.mkpath(savePath);
            QString fullPath = savePath + info.name();

            QFile *file = new QFile(fullPath);
            if (!file->open(QIODevice::WriteOnly)) {
                QMessageBox::warning(this, "错误", "保存失败");
                file->deleteLater();
                return;
            }

            for (auto &i : g_fileList) {
                if (i.name == info.name() && !i.isDir) {
                    i.file = file;
                    break;
                }
            }

            ftp->get(info.name(), file);
        });
    }
}

