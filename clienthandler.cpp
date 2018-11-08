#include "clienthandler.h"
#include "command.h"
#include "constants.h"
#include "datautils.h"
#include "socketutils.h"
#include <string>
#include <cstdio>
#include <QDebug>

ClientHandler::ClientHandler()
{
    this->cc = nullptr;
}

ClientHandler::~ClientHandler(){
    if (this->cc != nullptr){
        delete this->cc;
        this->cc = nullptr;
    }
}

int login(
        struct ClientUtils* cc,
        char* username,
        char* password){
    int sockfd = cc->sockfd;
    int p, idx;
    char msg[MAXBUFLEN];

    do {
        p = waitMsg(sockfd, msg, MAXBUFLEN);
        if (p < 0) {    // error code!
            qDebug() << "waitMsg Error! " << -p;
            return p;
        }
        idx = indexofDDDS(msg);
    } while (idx < 0);

    qDebug() << "From Server: " << QString(msg+idx+4);
    if (!startWith(msg+idx, "220 ")){
        qDebug() << "Login Failed: No 220 get!";
        return -ERRORLOGIN;
    }

    do {
        sprintf(msg, "USER %s\r\n", username);
        int len = strlen(msg);
        p = sendMsg(sockfd, msg, len);
        if (p < 0) {
            qDebug() << "sendMsg Error! " << -p;
            return p;
        }

        p = waitMsg(sockfd, msg, MAXBUFLEN);
        if (p < 0) {    // error code!
            qDebug() << "waitMsg Error! " << -p;
            return p;
        }
        idx = indexofDDDS(msg);
    } while (idx < 0);

    qDebug() << "From Server: " << QString(msg+idx+4);
    if (!startWith(msg+idx, "331 ")){
        qDebug() << "Login Failed: No 331 get!";
        return -ERRORLOGIN;
    }

    do {
        sprintf(msg, "PASS %s\r\n", password);
        int len = strlen(msg);
        p = sendMsg(sockfd, msg, len);
        if (p < 0) {
            qDebug() << "snedMsg Error! " << -p;
            return p;
        }

        p = waitMsg(sockfd, msg, MAXBUFLEN);
        if (p < 0) {    // error code!
            qDebug() << "waitMsg Error! " << -p;
            return p;
        }
        idx = indexofDDDS(msg);
    } while (idx < 0);

    qDebug() << "From Server: " << QString(msg+idx+4);
    if (!startWith(msg+idx, "230 ")){
        qDebug() << "Login Failed: No 230 get!";
        return -ERRORLOGIN;
    }

    return NOERROR;
}

int ClientHandler::conncet_login(
        char* ip,
        int port,
        char* username,
        char* password){
    this->cc = new ClientUtils();
    initClientUtils(this->cc);

    if ((this->cc->sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
//        printf("Error socket(): %s(%d)\nPlease", strerror(errno), errno);
        return -ERRORSOCKET;
    }

    memset(&(this->cc->addr), 0, sizeof(this->cc->addr));
    this->cc->addr.sin_family = AF_INET;
    this->cc->addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &(this->cc->addr.sin_addr)) <= 0) {
//        printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
        return -ERRORINETPTON;
    }

    if (connect(this->cc->sockfd, (struct sockaddr*)&(this->cc->addr), sizeof(this->cc->addr)) < 0) {
//        printf("Error connect(): %s(%d)\n", strerror(errno), errno);
        return -ERRORCONN;
    }

    qDebug() << this->cc->sockfd;
    int p = login(this->cc, username, password);
    if (p < 0)
        return -ERRORLOGIN;
    return p;
}
