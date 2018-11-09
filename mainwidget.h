#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include <QMenu>

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
    void DoCWD();
    void Rename();
    void show_Menu(QPoint);

private:
    Ui::MainWidget *ui;
    MainWindow *mw;
    struct mMenu{
        QMenu *popMenu;
        QAction *download;
        QAction *upload;
        QAction *refresh;
        QAction *rename;
        QString name;
    } menu;
    void removeFileList();
};

#endif // MAINWIDGET_H
