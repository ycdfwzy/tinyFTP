#include "mainwidget.h"
#include "mainwindow.h"
#include "ui_mainwidget.h"
#include <QMessageBox>
#include <QDebug>

MainWidget::MainWidget(
        QString ip, int port, QString name,
        MainWindow *mw_, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainWidget),
    mw(mw_)
{
    ui->setupUi(this);
    ui->SerEdt->setText(ip+QString(":")+QString::number(port));
    ui->NameEdt->setText(name);
    connect(ui->DisconBtn, SIGNAL(clicked()),
            this, SLOT(Logout()));
}

MainWidget::~MainWidget()
{
    delete ui;
}

void MainWidget::Logout(){
    ClientHandler *ch = mw->getClientHandler();
    int p = ch->quit();
    // Connection Error!
    if (p == -ERRORWRITE || p == -ERRORREAD){
        emit mw->SIGLogoutOK();
    } else
    // server refused disconnect!
    if (p < 0){
        QMessageBox::about(this, "Error", "Quit Failed!");
    } else
    {
        emit mw->SIGLogoutOK();
    }
}
