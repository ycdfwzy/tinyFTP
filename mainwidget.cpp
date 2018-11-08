#include "mainwidget.h"
#include "mainwindow.h"
#include "ui_mainwidget.h"
#include <QMessageBox>
#include <QStringList>
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
    ui->LocEdt->setText(GetPWD());
    setDirList();

}

MainWidget::~MainWidget()
{
    delete ui;
}

void MainWidget::Logout(){
    ClientHandler *ch = mw->getClientHandler();
    RetInfo p = ch->quit();
    // Connection Error!
    if (p.ErrorCode == -ERRORWRITE || p.ErrorCode == -ERRORREAD){
        emit mw->SIGLogoutOK();
    } else
    // server refused disconnect!
    if (p.ErrorCode < 0){
        QMessageBox::about(this, "Error", p.info);
    } else
    {
        emit mw->SIGLogoutOK();
    }
}

void MainWidget::setDirList(){
    ui->FileTbl->setColumnCount(4);
    ui->FileTbl->horizontalHeader()->setStretchLastSection(true);
    ui->FileTbl->setHorizontalHeaderLabels(QStringList() << "Name" << "Type" << "Size" << "Last Modification");
    ui->FileTbl->setSelectionBehavior(QAbstractItemView::SelectRows);

    ClientHandler *ch = mw->getClientHandler();
    RetInfo p = ch->list();
    if (p.ErrorCode == -ERRORWRITE || p.ErrorCode == -ERRORREAD){
        QMessageBox::about(this, "Error", "Disconnect unexpectedly!");
        emit mw->SIGLogoutOK();
        return;
    }
    QMessageBox::about(this, "FOO", p.info);
//    ui->FileTbl->insertRow(0);
//    ui->FileTbl->setRowHeight(0, 20);
//    ui->FileTbl->insertRow(0);
//    ui->FileTbl->setRowHeight(0, 20);
}

QString MainWidget::GetPWD(){
    ClientHandler *ch = mw->getClientHandler();
    RetInfo p = ch->pwd();
    // Connection Error!
    if (p.ErrorCode == -ERRORWRITE || p.ErrorCode == -ERRORREAD){
        QMessageBox::about(this, "Error", "Disconnect unexpectedly!");
        emit mw->SIGLogoutOK();
    }
    return p.info;
}
