#include "loginwidget.h"
#include "ui_loginwidget.h"
#include <QDebug>

LoginWidget::LoginWidget(MainWindow* mw_, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LoginWidget),
    mw(mw_)
{
    ui->setupUi(this);
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
}
