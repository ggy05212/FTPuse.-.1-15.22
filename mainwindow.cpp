#include "mainwindow.h"
#include "ui_mainwindow.h"
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_ftpform = new ftpform(this);
    setCentralWidget(m_ftpform);
}

MainWindow::~MainWindow()
{
    delete ui;
}
