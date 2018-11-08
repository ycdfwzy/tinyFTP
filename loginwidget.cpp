#include "loginwidget.h"
#include "ui_loginwidget.h"
#include "clienthandler.h"
#include "mainwindow.h"
#include <QValidator>
#include <QMessageBox>
#include <QRegExp>
#include <QDebug>

LoginWidget::LoginWidget(MainWindow* mw_, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LoginWidget),
    mw(mw_)
{
    ui->setupUi(this);
    ui->PortEdt->setValidator(new QIntValidator(0, 65535));
    ui->PassEdt->setEchoMode(QLineEdit::Password);
//    QRegExp rx("^((2[0-4]\\d|25[0-5]|[01]?\\d\\d?)\\.){3}(2[0-4]\\d|25[0-5]|[01]?\\d\\d?)$");
//    ui->IPEdt->setValidator(new QRegExpValidator(rx, this));

    connect(ui->Btn, SIGNAL(clicked()), this, SLOT(Login()));
}

LoginWidget::~LoginWidget()
{
    delete ui;
}

void LoginWidget::Login(){
    qDebug() << this->ui->IPEdt->text();
    qDebug() << this->ui->PortEdt->text();
    qDebug() << this->ui->NameEdt->text();
    qDebug() << this->ui->PassEdt->text();

    ClientHandler* ch = this->mw->getClientHandler();
    int p = ch->conncet_login(this->ui->IPEdt->text().toUtf8().data(),
                              this->ui->PortEdt->text().toInt(),
                              this->ui->NameEdt->text().toUtf8().data(),
                              this->ui->PassEdt->text().toUtf8().data());
    if (p == -ERRORLOGIN){
        QMessageBox::about(this, "Login Failed", "Wrong Username or Password!");
    } else
    if (p < 0){
        QMessageBox::about(this, "Connect Failed",
                           QString("Can't connect to ")+ui->IPEdt->text()+QString(":")+ui->PortEdt->text());
    } else
    {
        emit mw->SIGLoginOK(this->ui->IPEdt->text(),
                            this->ui->PortEdt->text().toInt(),
                            this->ui->NameEdt->text());
    }
}
