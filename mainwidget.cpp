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
    mw->getClientHandler()->rootpath = ui->LocEdt->text();
    connect(ui->LocEdt, SIGNAL(editingFinished()),
            this, SLOT(DoCWD()));
    connect(ui->GoBtn, SIGNAL(clicked()),
            this, SLOT(DoCWD()));
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
    ui->FileTbl->clear();
    ui->FileTbl->setColumnCount(4);
    ui->FileTbl->horizontalHeader()->setStretchLastSection(true);
    ui->FileTbl->setHorizontalHeaderLabels(QStringList() << "Name" << "Type" << "Size" << "Last Modification");
    ui->FileTbl->setSelectionBehavior(QAbstractItemView::SelectRows);

    ClientHandler *ch = mw->getClientHandler();
    RetInfo p = ch->list();
    if (p.ErrorCode == -ERRORWRITE || p.ErrorCode == -ERRORREAD){
        QMessageBox::about(this, "Error", "Disconnect unexpectedly!");
        emit mw->SIGLogoutOK();
    } else
    if (p.ErrorCode < 0){
        QMessageBox::about(this, "Error", "Get file list error!");
    } else
    {
        for (int i = 0; i < ch->fileList.length(); ++i){
            ui->FileTbl->insertRow(0);
            ui->FileTbl->setRowHeight(0, 20);

            QTableWidgetItem *item0 = new QTableWidgetItem();
            item0->setText(ch->fileList[i].name);
            ui->FileTbl->setItem(0, 0, item0);

            QTableWidgetItem *item1 = new QTableWidgetItem();
            item1->setText(ch->fileList[i].type);
            item1->setFlags(item1->flags() & (~Qt::ItemIsEditable));
            ui->FileTbl->setItem(0, 1, item1);

            QTableWidgetItem *item2 = new QTableWidgetItem();
            if (ch->fileList[i].size > (1<<30)) //GB
                item2->setText(QString::number(ch->fileList[i].size/1024./1024./1024, 'f', 1)+"Gb");
            else if (ch->fileList[i].size > (1<<20))
                item2->setText(QString::number(ch->fileList[i].size/1024./1024., 'f', 1)+"Mb");
            else if (ch->fileList[i].size > (1<<10))
                item2->setText(QString::number(ch->fileList[i].size/1024., 'f', 1)+"Kb");
            else
                item2->setText(QString::number(ch->fileList[i].size)+"byte");
            item2->setFlags(item2->flags() & (~Qt::ItemIsEditable));
            ui->FileTbl->setItem(0, 2, item2);

            QTableWidgetItem *item3 = new QTableWidgetItem();
            item3->setText(ch->fileList[i].mtime);
            item3->setFlags(item3->flags() & (~Qt::ItemIsEditable));
            ui->FileTbl->setItem(0, 3, item3);
        }
    }
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

void MainWidget::DoCWD(){
    ClientHandler *ch = mw->getClientHandler();
    RetInfo p = ch->cwd(ui->LocEdt->text());
    // Connection Error!
    if (p.ErrorCode == -ERRORWRITE || p.ErrorCode == -ERRORREAD){
        QMessageBox::about(this, "Error", "Disconnect unexpectedly!");
        emit mw->SIGLogoutOK();
    } else
    if (p.ErrorCode < 0){
        QMessageBox::about(this, "Error", p.info);
        ui->LocEdt->setText(ch->curpath);
    } else
    {
        setDirList();
    }
}
