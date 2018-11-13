#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <pthread.h>

#include "command.h"
#include "constants.h"
#include "datautils.h"
#include "socketutils.h"
#include <QString>
#include <QVector>
#include <QProgressDialog>
#include <QProgressBar>
#include <time.h>

struct RetInfo{
    int ErrorCode;
    QString info;

    RetInfo(int ec, QString info_=QString("Nothing")){
        this->ErrorCode = ec;
        this->info = info_;
    }
};

class RecvInfo{
public:
    QString name;
    QString dir_local;
    QString dir_remote;
    long long start_pos;
    int state; // 0 running
               // 1 paused
               // 2 stoped
};

class ClientHandler
{
public:
    explicit ClientHandler();
    ~ClientHandler();

    RetInfo conncet_login(char* ip, int port, char* username, char* password);
    RetInfo quit();
    RetInfo pwd();
    RetInfo pasv();
    RetInfo list();
    RetInfo rest(long long);
    RetInfo stor(const QString&,
                 QProgressDialog&);
    RetInfo retr(RecvInfo& ri,
                 QProgressBar* pb);
    RetInfo cwd(const QString&);
    RetInfo rename(const QString&,
                   const QString&);
    RetInfo mkd(const QString&);
    RetInfo remove(const QString&,
                   const QString&);

    struct ClientUtils* cu;
    struct FileInfo{
        QString name;
        QString type;
        long long size;
//        QString mtime;
        time_t mtime;
    };
    int order[4]; // 0 disorder
                  // 1 AscendingOrder
                  // 2 DescendingOrder
    QVector<FileInfo> fileList;

    QString rootpath;
    QString curpath;

    private:
        void extract_fileList(const QString&);
        long long extract_size(const QString&);
};

#endif // CLIENTHANDLER_H
