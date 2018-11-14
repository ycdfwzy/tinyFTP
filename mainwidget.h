#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include <QMenu>
#include <QPushButton>
#include <QVector>
#include "clienthandler.h"

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

    void setTransfering(bool transfering)
        { this->transfering = transfering; }
    MainWindow* getMainWindow() const { return mw; }

public slots:
    void Logout();
    void DoCWD();
    void DcCWD(int, int);   // double-clicked
    void Rename();
    void Refresh();
    void NewDir();
    void Remove();
    void Upload();
    void Download();
    void sortRow(int);
    void showFileList(int);
    void show_Menu(QPoint);
    void RECVBTNCLICKED();
    void SENDBTNCLICKED();

private:
    Ui::MainWidget *ui;
    MainWindow *mw;
    struct mMenu{
        QMenu *popMenu;
        QAction *download;
        QAction *upload;
        QAction *refresh;
        QAction *rename;
        QAction *remove;
        QAction *newfloder;
        QString name;
        QString type;
    } menu;
    bool transfering;
    QVector<transInfo> recvList;
    QVector<transInfo> sendList;
    void removeFileList();

    void initRecvFileTbl();
    int appendRecvFileTbl(const transInfo&);
    void removeRecvFileTbl(int idx);

    void initSendFileTbl();
    int appendSendFileTbl(const transInfo&);
    void removeSendFileTbl(int idx);

    bool allIdle() const;
};

#endif // MAINWIDGET_H
