#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "loginwidget.h"
#include "clienthandler.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    ClientHandler* getClientHandler(){
        return ch;
    }
private:
    Ui::MainWindow *ui;
    LoginWidget *lw;
    ClientHandler *ch;
    bool isLogin;
};

#endif // MAINWINDOW_H
