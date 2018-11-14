#include "mainwidget.h"
#include "mainwindow.h"
#include "ui_mainwidget.h"
#include <QMessageBox>
#include <QStringList>
#include <QDebug>
#include <QDialog>
#include <QInputDialog>
#include <QFileDialog>
#include <QProgressDialog>
#include <QProgressBar>
#include <algorithm>
#include <time.h>
#include <QIcon>

MainWidget::MainWidget(
        QString ip, int port, QString name,
        MainWindow *mw_, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainWidget),
    mw(mw_)
{
    ui->setupUi(this);
    this->transfering = false;
    ui->SerEdt->setText(ip+QString(":")+QString::number(port));
    ui->NameEdt->setText(name);
    connect(ui->DisconBtn, SIGNAL(clicked()),
            this, SLOT(Logout()));
    ui->LocEdt->setText(GetPWD());
    mw->getClientHandler()->rootpath = ui->LocEdt->text();
    connect(ui->LocEdt, SIGNAL(returnPressed()),
            this, SLOT(DoCWD()));
    connect(ui->GoBtn, SIGNAL(clicked()),
            this, SLOT(DoCWD()));
    connect(ui->MkdBtn, SIGNAL(clicked()),
            this, SLOT(NewDir()));
    connect(ui->UpBtn, SIGNAL(clicked()),
            this, SLOT(Upload()));
    setDirList();

    menu.popMenu = nullptr;
    ui->FileTbl->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->FileTbl, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(show_Menu(QPoint)));
    connect(ui->FileTbl, SIGNAL(cellDoubleClicked(int, int)),
            this, SLOT(DcCWD(int, int)));
    ui->GoBtn->setIcon(QApplication::style()->standardIcon(QStyle::SP_ToolBarHorizontalExtensionButton));
    ui->MkdBtn->setIcon(QApplication::style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    ui->UpBtn->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowUp));
    ui->GoBtn->setToolTip("Go to Location");
    ui->MkdBtn->setToolTip("New Floder");
    ui->UpBtn->setToolTip("Upload File");

    initRecvFileTbl();
    initSendFileTbl();
}

MainWidget::~MainWidget()
{
    delete ui;
}

void MainWidget::show_Menu(QPoint pos){
    if (this->transfering){
        QMessageBox::about(this, "Error", "You are transfering Files!");
        return;
    }

    qDebug() << "show_Menu";
    QModelIndex index = ui->FileTbl->indexAt(pos);
    QTableWidgetItem *item = ui->FileTbl->item(index.row(), 0);
    if (menu.popMenu != nullptr){
        delete menu.popMenu;
        menu.popMenu = nullptr;
    }

    if (item != nullptr){
        menu.name = item->text();
        menu.type = ui->FileTbl->item(index.row(), 1)->text();
        menu.popMenu = new QMenu(ui->FileTbl);
        menu.download = new QAction("Download", menu.popMenu);
        menu.refresh = new QAction("Refresh", menu.popMenu);
        menu.rename = new QAction("Rename", menu.popMenu);
        menu.remove = new QAction("Delete", menu.popMenu);
        menu.popMenu->addAction(menu.download);
        menu.popMenu->addAction(menu.refresh);
        menu.popMenu->addAction(menu.rename);
        menu.popMenu->addAction(menu.remove);

        if (menu.type == "directory")
            menu.download->setEnabled(false);

        connect(menu.rename, SIGNAL(triggered()),
                this, SLOT(Rename()));
        connect(menu.refresh, SIGNAL(triggered()),
                this, SLOT(Refresh()));
        connect(menu.remove, SIGNAL(triggered()),
                this, SLOT(Remove()));
        connect(menu.download, SIGNAL(triggered()),
                this, SLOT(Download()));
    } else
    {
        menu.popMenu = new QMenu(ui->FileTbl);
        menu.upload = new QAction("Upload", menu.popMenu);
        menu.refresh = new QAction("Refresh", menu.popMenu);
        menu.newfloder = new QAction("New Floder", menu.popMenu);
        menu.popMenu->addAction(menu.upload);
        menu.popMenu->addAction(menu.refresh);
        menu.popMenu->addAction(menu.newfloder);

        connect(menu.upload, SIGNAL(triggered()),
                this, SLOT(Upload()));
        connect(menu.refresh, SIGNAL(triggered()),
                this, SLOT(Refresh()));
        connect(menu.newfloder, SIGNAL(triggered()),
                this, SLOT(NewDir()));
    }
    menu.popMenu->exec(QCursor::pos());
}

void MainWidget::Logout(){
    if (this->transfering){
        QMessageBox::about(this, "Error", "You are transfering Files!");
        return;
    }
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

void MainWidget::removeFileList(){
    int t = ui->FileTbl->rowCount();
    while (t--)
        ui->FileTbl->removeRow(0);
}

void MainWidget::setDirList(){
    if (this->transfering){
        QMessageBox::about(this, "Error", "You are transfering Files!");
        return;
    }
    ui->FileTbl->setColumnCount(4);
    ui->FileTbl->horizontalHeader()->setStretchLastSection(true);
    ui->FileTbl->setHorizontalHeaderLabels(QStringList() << "Name" << "Type" << "Size" << "Last Modification");
    ui->FileTbl->setSelectionBehavior(QAbstractItemView::SelectRows);

    disconnect(ui->FileTbl->horizontalHeader(), SIGNAL(sectionClicked(int)), nullptr, nullptr);
    connect(ui->FileTbl->horizontalHeader(), SIGNAL(sectionClicked(int)),
            this, SLOT(sortRow(int)));

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
        for (int i = 0; i < 4; ++i)
            if (ch->order[i] != 0){
                sortRow(i);
                return;
            }
        sortRow(0);
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
    if (this->transfering){
        QMessageBox::about(this, "Error", "You are transfering Files!");
        return;
    }

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

void MainWidget::DcCWD(int row, int){
    if (this->transfering){
        QMessageBox::about(this, "Error", "You are transfering Files!");
        return;
    }
    if (ui->FileTbl->item(row, 1)->text() != "directory"){
        return;
    }

    ClientHandler *ch = mw->getClientHandler();
    if (ch->curpath.endsWith('/')){
        ui->LocEdt->setText(ch->curpath+ui->FileTbl->item(row, 0)->text());
    } else
        ui->LocEdt->setText(ch->curpath+QString("/")+ui->FileTbl->item(row, 0)->text());
    DoCWD();
}

void MainWidget::Rename(){
    if (this->transfering){
        QMessageBox::about(this, "Error", "You are transfering Files!");
        return;
    }

    QInputDialog input(this);
    input.setWindowTitle("File Name");
    input.setLabelText("Input filename:");
    input.setInputMode(QInputDialog::TextInput);
    input.setOkButtonText("Rename");
    if (input.exec() == QInputDialog::Accepted){
//        qDebug() << menu.name << " " << input.textValue();
        ClientHandler *ch = mw->getClientHandler();
        RetInfo p = ch->rename(menu.name, input.textValue());

        if (p.ErrorCode == -ERRORWRITE || p.ErrorCode == -ERRORREAD){
            QMessageBox::about(this, "Error", "Disconnect unexpectedly!");
            emit mw->SIGLogoutOK();
        } else
        if (p.ErrorCode < 0){
            QMessageBox::about(this, "Error", p.info);
            ui->LocEdt->setText(ch->curpath);
        } else {
            setDirList();
        }
    }
}

void MainWidget::Refresh(){
    if (this->transfering){
        QMessageBox::about(this, "Error", "You are transfering Files!");
        return;
    }

    DoCWD();
}

void MainWidget::NewDir(){
    if (this->transfering){
        QMessageBox::about(this, "Error", "You are transfering Files!");
        return;
    }

    QInputDialog input(this);
    input.setWindowTitle("New Floder");
    input.setLabelText("Floder name:");
    input.setInputMode(QInputDialog::TextInput);
    input.setOkButtonText("Create");
    if (input.exec() == QInputDialog::Accepted){
        ClientHandler *ch = mw->getClientHandler();
        RetInfo p = ch->mkd(input.textValue());

        if (p.ErrorCode == -ERRORWRITE || p.ErrorCode == -ERRORREAD){
            QMessageBox::about(this, "Error", "Disconnect unexpectedly!");
            emit mw->SIGLogoutOK();
        } else
        if (p.ErrorCode < 0){
            QMessageBox::about(this, "Error", p.info);
            ui->LocEdt->setText(ch->curpath);
        } else {
            Refresh();
        }
    }
}

void MainWidget::Remove(){
    if (this->transfering){
        QMessageBox::about(this, "Error", "You are transfering Files!");
        return;
    }

    QMessageBox msg(this);
    msg.setWindowTitle("Warning");
    msg.setText(QString("Are you sure to DELETE ")+menu.name);
    msg.setIcon(QMessageBox::Warning);
    msg.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);

    if (msg.exec() == QMessageBox::Ok){
        ClientHandler *ch = mw->getClientHandler();
        RetInfo p = ch->remove(menu.name, menu.type);
        if (p.ErrorCode == -ERRORWRITE || p.ErrorCode == -ERRORREAD){
            QMessageBox::about(this, "Error", "Disconnect unexpectedly!");
            emit mw->SIGLogoutOK();
        } else
        if (p.ErrorCode < 0){
            QMessageBox::about(this, "Error", p.info);
            ui->LocEdt->setText(ch->curpath);
        } else {
            Refresh();
        }
    }
}

void MainWidget::Upload(){
    if (this->transfering){
        QMessageBox::about(this, "Error", "You are transfering Files!");
        return;
    }

    QFileDialog fd(this);
    fd.setWindowTitle("Choose upload file");
    fd.setAcceptMode(QFileDialog::AcceptOpen);
    fd.setFileMode(QFileDialog::ExistingFile);
    fd.setOption(QFileDialog::DontUseNativeDialog);

    if (fd.exec() == QFileDialog::Accepted){
        QString fullpath = fd.selectedFiles()[0];
        char *fullpath_cstr = fullpath.toUtf8().data();
        getfilename(fullpath_cstr);
        QString filename(fullpath_cstr);
        QString dirname = fullpath.mid(0, fullpath.length()-filename.length());
//        qDebug() << "dirname = " << dirname;

        ClientHandler *ch = mw->getClientHandler();
        transInfo ri;
        ri.name = filename;
        ri.dir_local = dirname;
        ri.dir_remote = ch->curpath;
        ri.start_pos = 0;
        ri.state = 0;
        int idx = appendSendFileTbl(ri);
        QProgressBar* pb = qobject_cast<QProgressBar*>(ui->sendFileTbl->cellWidget(idx, 3));

        this->transfering = true;
        RetInfo p = ch->stor(sendList[idx], pb);
        this->transfering = false;

        if (sendList[idx].state == 2){ // stopped
            removeSendFileTbl(idx);
        } else
        if (sendList[idx].state == 0){
            removeSendFileTbl(idx);
            if (p.ErrorCode == -ERRORWRITE || p.ErrorCode == -ERRORREAD){
                QMessageBox::about(this, "Error", "Disconnect unexpectedly!");
                emit mw->SIGLogoutOK();
            } else
            if (p.ErrorCode < 0){
                QMessageBox::about(this, "Error", p.info);
            } else
            {
                Refresh();
            }
        } else
        {
            qDebug() << "You paused " << sendList[idx].name;
        }

//        QProgressDialog pd;
//        pd.setWindowTitle("Send File");
//        pd.setWindowModality(Qt::NonModal);
//        pd.setCancelButtonText("STOP");
//        pd.setLabelText(QString("Uploading ")+filename);

//        ClientHandler *ch = mw->getClientHandler();
//        this->transfering = true;
//        RetInfo p = ch->stor(filename, pd);
//        this->transfering = false;
//        if (p.ErrorCode == -ERRORWRITE || p.ErrorCode == -ERRORREAD){
//            QMessageBox::about(this, "Error", "Disconnect unexpectedly!");
//            emit mw->SIGLogoutOK();
//        } else
//        if (p.ErrorCode < 0){
//            QMessageBox::about(this, "Error", p.info);
//        } else {
//            Refresh();
//        }
    }
}

void MainWidget::Download(){
    if (this->transfering){
        QMessageBox::about(this, "Error", "You are transfering Files!");
        return;
    }

    QFileDialog fd(this);
    fd.setWindowTitle("Choose download directory");
    fd.setAcceptMode(QFileDialog::AcceptSave);
    fd.setFileMode(QFileDialog::DirectoryOnly);
    fd.setOption(QFileDialog::DontUseNativeDialog);

    if (fd.exec() == QFileDialog::Accepted) {
        QString dirname = fd.selectedFiles()[0];

        ClientHandler *ch = mw->getClientHandler();
        transInfo ri;
        ri.name = menu.name;
        ri.dir_local = dirname;
        ri.dir_remote = ch->curpath;
        ri.start_pos = 0;
        ri.state = 0;
        int idx = appendRecvFileTbl(ri);
        QProgressBar* pb = qobject_cast<QProgressBar*>(ui->recvFileTbl->cellWidget(idx, 3));

        this->transfering = true;
        RetInfo p = ch->retr(recvList[idx], pb);
        this->transfering = false;

        if (recvList[idx].state == 2){ // stopped
            removeRecvFileTbl(idx);
        } else
        if (recvList[idx].state == 0){ // not paused
            removeRecvFileTbl(idx);
            if (p.ErrorCode == -ERRORWRITE || p.ErrorCode == -ERRORREAD){
                QMessageBox::about(this, "Error", "Disconnect unexpectedly!");
                emit mw->SIGLogoutOK();
            } else
            if (p.ErrorCode < 0){
                QMessageBox::about(this, "Error", p.info);
            }
        } else
        {
            qDebug() << "You paused " << recvList[idx].name;
        }
    }
}

void MainWidget::sortRow(int index){
    if (index < 0 || index > 4){
        qDebug() << "index = " << index;
        return;
    }
    ClientHandler *ch = mw->getClientHandler();
    Qt::SortOrder order;
    if (ch->order[index] == 0 || ch->order[index] == 2){
        order = Qt::AscendingOrder;
        ch->order[index] = 1;
    }
    else {
        order = Qt::DescendingOrder;
        ch->order[index] = 2;
    }

    for (int i = 0; i < 4; ++i)
    if (i != index){
        ch->order[i] = 0;
    }
    ui->FileTbl->horizontalHeader()->setSortIndicatorShown(true);
    ui->FileTbl->horizontalHeader()->setSortIndicator(index, order);

    showFileList(index);
}

void MainWidget::showFileList(int index){
    ClientHandler *ch = mw->getClientHandler();
    int ord = ch->order[index];
    switch (index) {
        case 0:
            std::sort(ch->fileList.begin(), ch->fileList.end(),
                  [ord](const ClientHandler::FileInfo& a,
                      const ClientHandler::FileInfo& b){
                        if (ord == 1)
                            return a.name > b.name;
                        return a.name < b.name;
                    });
            break;
        case 1:
            std::sort(ch->fileList.begin(), ch->fileList.end(),
              [ord](const ClientHandler::FileInfo& a,
                  const ClientHandler::FileInfo& b){
                    if (ord == 1)
                        return a.type > b.type;
                    return a.type < b.type;
                });
            break;
        case 2:
            std::sort(ch->fileList.begin(), ch->fileList.end(),
              [ord](const ClientHandler::FileInfo& a,
                  const ClientHandler::FileInfo& b){
                    if (a.size == -1 || b.size == -1)
                        return a.size == -1 && b.size != -1;
                    if (ord == 1){
                        return a.size > b.size;
                    }
                    return a.size < b.size;
                });
            break;
        case 3:
            std::sort(ch->fileList.begin(), ch->fileList.end(),
              [ord](const ClientHandler::FileInfo& a,
                  const ClientHandler::FileInfo& b){
                    if (ord == 1)
                        return a.mtime > b.mtime;
                    return a.mtime < b.mtime;
                });
            break;
    }
    removeFileList();
    for (int i = 0; i < ch->fileList.length(); ++i){
        ui->FileTbl->insertRow(0);
        ui->FileTbl->setRowHeight(0, 20);

        QTableWidgetItem *item0 = new QTableWidgetItem();
        item0->setText(ch->fileList[i].name);
        item0->setFlags(item0->flags() & (~Qt::ItemIsEditable));
        ui->FileTbl->setItem(0, 0, item0);

        QTableWidgetItem *item1 = new QTableWidgetItem();
        item1->setText(ch->fileList[i].type);
        item1->setFlags(item1->flags() & (~Qt::ItemIsEditable));
        ui->FileTbl->setItem(0, 1, item1);

        QTableWidgetItem *item2 = new QTableWidgetItem();
        if (ch->fileList[i].type != "directory"){
            if (ch->fileList[i].size > (1<<30)) //GB
                item2->setText(QString::number(ch->fileList[i].size/1024./1024./1024, 'f', 1)+"Gb");
            else if (ch->fileList[i].size > (1<<20))
                item2->setText(QString::number(ch->fileList[i].size/1024./1024., 'f', 1)+"Mb");
            else if (ch->fileList[i].size > (1<<10))
                item2->setText(QString::number(ch->fileList[i].size/1024., 'f', 1)+"Kb");
            else
                item2->setText(QString::number(ch->fileList[i].size)+"byte");
        }
        item2->setFlags(item2->flags() & (~Qt::ItemIsEditable));
        ui->FileTbl->setItem(0, 2, item2);

        QTableWidgetItem *item3 = new QTableWidgetItem();
        item3->setText(QString(asctime(localtime(&ch->fileList[i].mtime))));
        item3->setFlags(item3->flags() & (~Qt::ItemIsEditable));
        ui->FileTbl->setItem(0, 3, item3);
    }
}

void MainWidget::initRecvFileTbl(){
    recvList.clear();
    ui->recvFileTbl->clear();
    ui->recvFileTbl->setColumnCount(4);
    ui->recvFileTbl->horizontalHeader()->setStretchLastSection(true);
    ui->recvFileTbl->setHorizontalHeaderLabels(QStringList() << "FileName" << "Stop" << "Pause" << "Progress");
    ui->recvFileTbl->setSelectionBehavior(QAbstractItemView::SelectRows);
}

int MainWidget::appendRecvFileTbl(const transInfo& ri){
    int len = recvList.length();
    ui->recvFileTbl->insertRow(len);
    recvList.append(ri);

    QPushButton *stopBtn = new QPushButton("STOP", ui->recvFileTbl);
    QPushButton *pauseBtn = new QPushButton("PAUSE", ui->recvFileTbl);
    QProgressBar *pb = new QProgressBar(ui->recvFileTbl);

    connect(stopBtn, SIGNAL(clicked()), this, SLOT(RECVBTNCLICKED()));
    connect(pauseBtn, SIGNAL(clicked()), this, SLOT(RECVBTNCLICKED()));
    pb->setRange(0, 100);
    pb->setValue(0);

    QTableWidgetItem *item = new QTableWidgetItem();
    item->setText(ri.name);
    item->setFlags(item->flags() & (~Qt::ItemIsEditable));
    ui->recvFileTbl->setItem(len, 0, item);
    ui->recvFileTbl->setCellWidget(len, 1, stopBtn);
    ui->recvFileTbl->setCellWidget(len, 2, pauseBtn);
    ui->recvFileTbl->setCellWidget(len, 3, pb);
    return len;
}

void MainWidget::removeRecvFileTbl(int idx){
    recvList.removeAt(idx);
    ui->recvFileTbl->removeRow(idx);
}

void MainWidget::RECVBTNCLICKED(){
    QPushButton *senderObj = qobject_cast<QPushButton*>(sender());
    if (senderObj == nullptr)
        return;
    QPoint pos(senderObj->frameGeometry().x(), senderObj->frameGeometry().y());
    QModelIndex idx = ui->recvFileTbl->indexAt(pos);
    int row = idx.row();
    int col = idx.column();
    if (col == 1){
        qDebug() << "You Clicked stop! " << ui->recvFileTbl->item(row, 0)->text();
        if (recvList[row].state == 0){
            recvList[row].state = 2;
        }
    }
    else
    if (col == 2){
        qDebug() << "You Clicked pause! " << ui->recvFileTbl->item(row, 0)->text();

        if (recvList[row].state == 0){
            recvList[row].state = 1;
            senderObj->setText("CONTINUE");
        } else
        if (recvList[row].state == 1){
            if (this->transfering){
                QMessageBox::about(this, "Error", "You are transfering files!");
            } else
            {
                recvList[row].state = 0;

                QProgressBar *pb = qobject_cast<QProgressBar*>(ui->recvFileTbl->cellWidget(row, 3));
                ClientHandler *ch = mw->getClientHandler();

                this->transfering = true;
                RetInfo p = ch->retr(recvList[row], pb);
                this->transfering = false;

                if (recvList[row].state == 2){ // stopped
                    removeRecvFileTbl(row);
                } else
                if (recvList[row].state == 0){ // not paused
                    removeRecvFileTbl(row);
                    if (p.ErrorCode == -ERRORWRITE || p.ErrorCode == -ERRORREAD){
                        QMessageBox::about(this, "Error", "Disconnect unexpectedly!");
                        emit mw->SIGLogoutOK();
                    } else
                    if (p.ErrorCode < 0){
                        QMessageBox::about(this, "Error", p.info);
                    }
                } else
                {
                    qDebug() << "You paused " << recvList[row].name;
                }
                senderObj->setText("PAUSE");
            }
        }
    }
}

void MainWidget::initSendFileTbl(){
    sendList.clear();
    ui->sendFileTbl->clear();
    ui->sendFileTbl->setColumnCount(4);
    ui->sendFileTbl->horizontalHeader()->setStretchLastSection(true);
    ui->sendFileTbl->setHorizontalHeaderLabels(QStringList() << "FileName" << "Stop" << "Pause" << "Progress");
    ui->sendFileTbl->setSelectionBehavior(QAbstractItemView::SelectRows);
}

int MainWidget::appendSendFileTbl(const transInfo& ri){
    int len = sendList.length();
    ui->sendFileTbl->insertRow(len);
    sendList.append(ri);

    QPushButton *stopBtn = new QPushButton("STOP", ui->sendFileTbl);
    QPushButton *pauseBtn = new QPushButton("PAUSE", ui->sendFileTbl);
    QProgressBar *pb = new QProgressBar(ui->sendFileTbl);

    connect(stopBtn, SIGNAL(clicked()), this, SLOT(SENDBTNCLICKED()));
    connect(pauseBtn, SIGNAL(clicked()), this, SLOT(SENDBTNCLICKED()));
    pb->setRange(0, 100);
    pb->setValue(0);

    QTableWidgetItem *item = new QTableWidgetItem();
    item->setText(ri.name);
    item->setFlags(item->flags() & (~Qt::ItemIsEditable));
    ui->sendFileTbl->setItem(len, 0, item);
    ui->sendFileTbl->setCellWidget(len, 1, stopBtn);
    ui->sendFileTbl->setCellWidget(len, 2, pauseBtn);
    ui->sendFileTbl->setCellWidget(len, 3, pb);
    return len;
}

void MainWidget::removeSendFileTbl(int idx){
    sendList.removeAt(idx);
    ui->sendFileTbl->removeRow(idx);
}

void MainWidget::SENDBTNCLICKED(){
    QPushButton *senderObj = qobject_cast<QPushButton*>(sender());
    if (senderObj == nullptr)
        return;
    QPoint pos(senderObj->frameGeometry().x(), senderObj->frameGeometry().y());
    QModelIndex idx = ui->sendFileTbl->indexAt(pos);
    int row = idx.row();
    int col = idx.column();
    if (col == 1){
        qDebug() << "You Clicked stop! " << ui->sendFileTbl->item(row, 0)->text();
        if (sendList[row].state == 0){
            sendList[row].state = 2;
        }
    } else
    if (col == 2){
        qDebug() << "You Clicked pause! " << ui->sendFileTbl->item(row, 0)->text();

        if (sendList[row].state == 0){
            sendList[row].state = 1;
            senderObj->setText("CONTINUE");
        } else
        if (sendList[row].state == 1){
            if (this->transfering){
                QMessageBox::about(this, "Error", "You are transfering files!");
            } else
            {
                sendList[row].state = 0;

                QProgressBar *pb = qobject_cast<QProgressBar*>(ui->sendFileTbl->cellWidget(row, 3));
                ClientHandler *ch = mw->getClientHandler();

                this->transfering = true;
                RetInfo p = ch->stor(sendList[row], pb);
                this->transfering = false;

                if (sendList[row].state == 2){ // stopped
                    removeSendFileTbl(row);
                } else
                if (sendList[row].state == 0){ // not paused
                    removeSendFileTbl(row);
                    if (p.ErrorCode == -ERRORWRITE || p.ErrorCode == -ERRORREAD){
                        QMessageBox::about(this, "Error", "Disconnect unexpectedly!");
                        emit mw->SIGLogoutOK();
                    } else
                    if (p.ErrorCode < 0){
                        QMessageBox::about(this, "Error", p.info);
                    } else
                    {
                        Refresh();
                    }
                } else
                {
                    qDebug() << "You paused " << sendList[row].name;
                }
                senderObj->setText("PAUSE");
            }
        }
    }
}

bool MainWidget::allIdle() const{
    for (int i = 0; i < recvList.length(); ++i)
        if (recvList[i].state == 0)
            return false;
    return true;
}
