#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "loginwidget.h"
#include "clienthandler.h"
#include "mainwidget.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    ClientHandler* getClientHandler(){ return ch;}

signals:
    void SIGLoginOK(QString, int, QString);
    void SIGLogoutOK();

public slots:
    void LoginOK(QString, int, QString);
    void LogoutOK();
private:
    Ui::MainWindow *ui;
    QWidget *qw;
    ClientHandler *ch;
    bool isLogin;
};

#endif // MAINWINDOW_H
