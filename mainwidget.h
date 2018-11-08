#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>

namespace Ui {
class MainWidget;
}

class MainWindow;

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainWidget(QString ip, int port, QString name, MainWindow *mw_, QWidget *parent = nullptr);
    ~MainWidget();

    QString GetPWD();
    void setDirList();

public slots:
    void Logout();

private:
    Ui::MainWidget *ui;
    MainWindow *mw;
};

#endif // MAINWIDGET_H
