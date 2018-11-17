#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    connect(this, SIGNAL(SIGLoginOK(QString, int, QString)),
            this, SLOT(LoginOK(QString, int, QString)));
    connect(this, SIGNAL(SIGLogoutOK()),
            this, SLOT(LogoutOK()));
    ui->setupUi(this);
    this->ch = new ClientHandler();
    this->qw = new LoginWidget(this);
    this->setCentralWidget(this->qw);
    this->setWindowTitle("Please log in");
    this->setFixedSize(380, 310);
    this->isLogin = false;
}

MainWindow::~MainWindow()
{
    delete ui;
    if (ch != nullptr){
        delete ch;
    }
    if (qw != nullptr){
        delete qw;
    }
}

void MainWindow::LoginOK(QString ip, int port, QString name){
    if (this->qw != nullptr){
        delete this->qw;
    }
    this->qw = new MainWidget(ip, port, name, this);
    this->setCentralWidget(this->qw);
    this->setWindowTitle(name+QString('@')+ip+QString(':')+QString::number(port));
    this->setFixedSize(590, 710);
    this->isLogin = true;
}

void MainWindow::LogoutOK(){
    if (this->qw != nullptr){
        delete this->qw;
    }

    this->qw = new LoginWidget(this);
    this->setCentralWidget(this->qw);
    this->setWindowTitle("Please log in");
    this->setFixedSize(380, 310);
    this->isLogin = false;
}
