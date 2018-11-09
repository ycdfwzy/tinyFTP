#include "clienthandler.h"
#include "command.h"
#include "constants.h"
#include "datautils.h"
#include "socketutils.h"
#include <string>
#include <cstdio>
#include <QDebug>

char snd[MAXBUFLEN];
char rec[MAXBUFLEN];

ClientHandler::ClientHandler()
{
    this->cu = nullptr;
}

ClientHandler::~ClientHandler(){
    if (this->cu != nullptr){
        delete this->cu;
        this->cu = nullptr;
    }
}

RetInfo login(
        struct ClientUtils* cu,
        char* username,
        char* password){
    int sockfd = cu->sockfd;
    int p, idx;
    char msg[MAXBUFLEN];

    do {
        p = waitMsg(sockfd, msg, MAXBUFLEN);
        if (p < 0) {    // error code!
            qDebug() << "waitMsg Error! " << -p;
            return RetInfo(p);
        }
        idx = indexofDDDS(msg);
    } while (idx < 0);

    qDebug() << "From Server: " << QString(msg+idx+4);
    if (!startWith(msg+idx, "220 ")){
        qDebug() << "Login Failed: No 220 get!";
        return RetInfo(-ERRORLOGIN, QString(msg+idx+4));
    }

    do {
        sprintf(msg, "USER %s\r\n", username);
        int len = strlen(msg);
        p = sendMsg(sockfd, msg, len);
        if (p < 0) {
            qDebug() << "sendMsg Error! " << -p;
            return RetInfo(p);
        }

        p = waitMsg(sockfd, msg, MAXBUFLEN);
        if (p < 0) {    // error code!
            qDebug() << "waitMsg Error! " << -p;
            return RetInfo(p);
        }
        idx = indexofDDDS(msg);
    } while (idx < 0);

    qDebug() << "From Server: " << QString(msg+idx+4);
    if (!startWith(msg+idx, "331 ")){
        qDebug() << "Login Failed: No 331 get!";
        return RetInfo(-ERRORLOGIN, QString(msg+idx+4));
    }

    do {
        sprintf(msg, "PASS %s\r\n", password);
        int len = strlen(msg);
        p = sendMsg(sockfd, msg, len);
        if (p < 0) {
            qDebug() << "snedMsg Error! " << -p;
            return RetInfo(p);
        }

        p = waitMsg(sockfd, msg, MAXBUFLEN);
        if (p < 0) {    // error code!
            qDebug() << "waitMsg Error! " << -p;
            return RetInfo(p);
        }
        idx = indexofDDDS(msg);
    } while (idx < 0);

    qDebug() << "From Server: " << QString(msg+idx+4);
    if (!startWith(msg+idx, "230 ")){
        qDebug() << "Login Failed: No 230 get!";
        return RetInfo(-ERRORLOGIN, QString(msg+idx+4));
    }

    return RetInfo(NOERROR, QString(msg+idx+4));
}

RetInfo ClientHandler::conncet_login(
        char* ip,
        int port,
        char* username,
        char* password){
    this->cu = new ClientUtils();
    initClientUtils(this->cu);

    if ((this->cu->sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
//        printf("Error socket(): %s(%d)\nPlease", strerror(errno), errno);
        return RetInfo(-ERRORSOCKET);
    }

    memset(&(this->cu->addr), 0, sizeof(this->cu->addr));
    this->cu->addr.sin_family = AF_INET;
    this->cu->addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &(this->cu->addr.sin_addr)) <= 0) {
//        printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
        return RetInfo(-ERRORINETPTON);
    }

    if (connect(this->cu->sockfd, (struct sockaddr*)&(this->cu->addr), sizeof(this->cu->addr)) < 0) {
//        printf("Error connect(): %s(%d)\n", strerror(errno), errno);
        return RetInfo(-ERRORCONN);
    }

    qDebug() << this->cu->sockfd;
    RetInfo ret = login(this->cu, username, password);
    if (ret.ErrorCode < 0)
        return RetInfo(-ERRORLOGIN);
    return ret;
}

QString getPath(QString path){
    int len = path.length();
    QString ret = "";
    for (int i = 1; i < len; ++i){
        if (path[i] == '"' && path[i-1] != '"')
            break;
        ret += path[i];
    }
    return ret;
}

RetInfo ClientHandler::quit(){
    int p, idx;
    char msg[MAXBUFLEN];

    sprintf(msg, "QUIT\r\n");
    p = sendMsg(cu->sockfd, msg, strlen(msg));
    if (p < 0){
        qDebug() << "sendMsg Error! " << -p;
        return RetInfo(p);
    }

    do {
        p = waitMsg(cu->sockfd, msg, MAXBUFLEN);
        if (p < 0) {    // error code!
            qDebug() << "waitMsg Error! " << -p;
            return RetInfo(p);
        }
        idx = indexofDDDS(msg);
    } while (idx < 0);

    qDebug() << "From Server: " << QString(msg+idx+4);
    if (!startWith(msg+idx, "221 ")){
        return RetInfo(-1, QString(msg+idx+4));
    }
    return RetInfo(NOERROR, QString(msg+idx+4));
}

RetInfo ClientHandler::pwd(){
    int p, idx;
    char msg[MAXBUFLEN];

    sprintf(msg, "PWD\r\n");
    p = sendMsg(cu->sockfd, msg, strlen(msg));
    if (p < 0){
        qDebug() << "sendMsg Error! " << -p;
        return RetInfo(p);
    }

    do {
        p = waitMsg(cu->sockfd, msg, MAXBUFLEN);
        if (p < 0) {    // error code!
            qDebug() << "waitMsg Error! " << -p;
            return RetInfo(p);
        }
        idx = indexofDDDS(msg);
    } while (idx < 0);

    qDebug() << "From Server: " << QString(msg+idx+4);

    if (!startWith(msg+idx, "257 ")){
        return RetInfo(-1, QString(msg+idx+4));
    }
    curpath = getPath(QString(msg+idx+4));
    return RetInfo(NOERROR, curpath);
}

RetInfo ClientHandler::pasv(){
    int idx, p;

    dropOtherConn_Client(cu);

    sprintf(snd, "PASV\r\n");
    p = sendMsg(cu->sockfd, snd, strlen(snd));
    do{
        p = waitMsg(cu->sockfd, rec, MAXBUFLEN);
        if (p < 0) {    // error code!
            qDebug() << "waitMsg Error! " << -p;
            return RetInfo(p);
        }
        idx = indexofDDDS(rec);
    } while (idx < 0);
    qDebug() << "From server: " << QString(rec+idx+4);

    if (startWith(rec+idx, "227 ")){
        p = conSer_Client(rec+5, cu);
        if (p < 0){
            qDebug() << "connect data Server Error! " << -p;
            return RetInfo(p, QString("connect data Server Error!"));
        }

        qDebug() << "Now you are in PASV mode" << endl << "Please input cmd STOR/RETR/LIST!";
        return RetInfo(NOERROR, QString("Now you are in PASV mode\nPlease input cmd STOR/RETR/LIST!\n"));
    }

    return RetInfo(-1, QString(rec+idx));
}

int getContent(const QString& mlist, int x){
    int ret = 0;
    while (x+ret+1 < mlist.length()){
        if (mlist[x+ret] == '"' && mlist[x+ret+1] != '"')
            return ret;
        ret++;
    }
    return mlist.length();
}

void ClientHandler::extract_fileList(const QString& mlist){
    int idx = 0;
    FileInfo fileInfo;
    fileList.clear();
    while ((idx = mlist.indexOf("name: \"", idx)) >= 0){
        int l = getContent(mlist, idx+7);
        fileInfo.name = mlist.mid(idx+7, l);
        idx += 7+l;

        idx = mlist.indexOf("type: \"", idx);
        l = getContent(mlist, idx+7);
        fileInfo.type = mlist.mid(idx+7, l);
        idx += 7+l;

        idx = mlist.indexOf("size: \"", idx);
        l = getContent(mlist, idx+7);
        fileInfo.size = mlist.mid(idx+7, l).toLongLong();
        idx += 7+l;

        idx = mlist.indexOf("last modification: \"", idx);
        l = getContent(mlist, idx+20);
        fileInfo.mtime = mlist.mid(idx+20, l);
        idx += 20+l;

        fileList.append(fileInfo);
    }
}

RetInfo ClientHandler::list(){
    int p, idx;
    char mlist[MAXBUFLEN];

    RetInfo ret = this->pasv();
    if (ret.ErrorCode < 0){
        return ret;
    }

    sprintf(snd, "LIST\r\n");
    p = sendMsg(cu->sockfd, snd, strlen(snd));
    if (p < 0) {    // error code!
        qDebug() << "sendMsg Error!" << -p;
        dropOtherConn_Client(cu);
        return RetInfo(p);
    }
    do{
        p = waitMsg(cu->sockfd, rec, MAXBUFLEN);
        if (p < 0) {    // error code!
            qDebug() << "waitMsg Error!" << -p;
            dropOtherConn_Client(cu);
            return RetInfo(p);
        }
        idx = indexofDDDS(rec);
    } while (idx < 0);
    qDebug() << "From server: " << QString(rec+idx+4);

    if (startWith(rec+idx, "150 ")){
        qDebug() << "Before recv_list";
        p = recv_list(cu->dataCli->sockfd, mlist);
        qDebug() << "After recv_list";
        dropOtherConn_Client(cu);

        do{
            p = waitMsg(cu->sockfd, rec, MAXBUFLEN);
            if (p < 0) {    // error code!
                qDebug() << "waitMsg Error! " << -p;
                return RetInfo(p);
            }
            idx = indexofDDDS(rec);
        } while (idx < 0);
        qDebug() << "From server:  " << QString(rec+idx+4);

        if (startWith(rec+idx, "226 ")){
            qDebug() << "LIST Successfully!";
            extract_fileList(QString(mlist));
            return RetInfo(NOERROR, QString("LIST Successfully!"));
        } else
        {
            qDebug() << "some error happend when LIST!";
            return RetInfo(-1, QString(rec+idx+4));
        }
    } else
    {
        qDebug() << "LIST Refused!";
        dropOtherConn_Client(cu);
        return RetInfo(-1, QString(rec+idx+4));
    }
}

RetInfo ClientHandler::cwd(const QString& path){
    int p, idx;

    sprintf(snd, "CWD %s\r\n", path.toUtf8().data());
    p = sendMsg(cu->sockfd, snd, strlen(snd));
    if (p < 0) {    // error code!
        qDebug() << "sendMsg Error!" << -p;
        dropOtherConn_Client(cu);
        return RetInfo(p);
    }

    do{
        p = waitMsg(cu->sockfd, rec, MAXBUFLEN);
        if (p < 0) {    // error code!
            qDebug() << "waitMsg Error!" << -p;
            dropOtherConn_Client(cu);
            return RetInfo(p);
        }
        idx = indexofDDDS(rec);
    } while (idx < 0);

    qDebug() << "From server: " << QString(rec+idx+4);
    if (startWith(rec+idx, "250 ")){
        curpath = path;
        return RetInfo(NOERROR, QString(rec+idx+4));
    }

    return RetInfo(-1, QString(rec+idx+4));
}
