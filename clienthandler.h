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

struct RetInfo{
    int ErrorCode;
    QString info;

    RetInfo(int ec, QString info_=QString("Nothing")){
        this->ErrorCode = ec;
        this->info = info_;
    }
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

    struct ClientUtils* cu;
};

#endif // CLIENTHANDLER_H
