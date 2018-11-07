#ifndef LOGINWIDGET_H
#define LOGINWIDGET_H

#include <QWidget>

namespace Ui {
class LoginWidget;
}

class MainWindow;

class LoginWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LoginWidget(MainWindow* mw_, QWidget *parent = nullptr);
    ~LoginWidget();

public slots:
    void Login();

private:
    Ui::LoginWidget *ui;
    MainWindow *mw;
};

#endif // LOGINWIDGET_H
