#ifndef tinyFTP_socketuitls_H
#define tinyFTP_socketuitls_H
#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <stdlib.h>
#include <string.h>
#include "errorcode.h"
#include "constants.h"

struct ServerUtils;
struct ClientUtils;

struct connClient{
	char curdir[512];
	char oldpath[512];
	int connfd;
	struct ServerUtils* dataSer;
	struct ClientUtils* dataCli;
};

struct ServerUtils{
	int listenfd;
	struct sockaddr_in addr;
	struct connClient conn[MAXCONN];
};

struct ClientUtils{
	int sockfd;
	struct sockaddr_in addr;
	struct ServerUtils* dataSer;
	struct ClientUtils* dataCli;
};


void closeSer(struct ServerUtils*);
void closeCli(struct ClientUtils*);
void initconnClient(struct connClient*);
void releconnClient(struct connClient*);
void initServerUtils(struct ServerUtils*);
void releServerUtils(struct ServerUtils*);
void initClientUtils(struct ClientUtils*);
void releClientUtils(struct ClientUtils*);
void dropOtherConn_Client(struct ClientUtils* cu);
void dropOtherConn_CONN(struct connClient* cc);
int getfisrtConn(struct ServerUtils*);
int waitMsg(int connfd, char* msg, int MAXLEN);
int waitData(int connfd, char* msg, int MAXLEN);
int sendMsg(int connfd, char* msg, int len);

#endif