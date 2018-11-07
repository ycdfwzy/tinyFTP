#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->isLogin = false;
    this->lw = new LoginWidget(this);
    this->setCentralWidget(this->lw);
    this->setWindowTitle("Please log in first!");
    this->setFixedSize(380, 310);
}

MainWindow::~MainWindow()
{
    delete ui;
}

