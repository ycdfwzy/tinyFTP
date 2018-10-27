#ifndef tinyFTP_socketuitls_H
#define tinyFTP_socketuitls_H
#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <stdlib.h>
#include <string.h>
#include "errorcode.h"

struct ClientUtils;

struct ServerUtils{
	int listenfd;
	int connfd;
	struct sockaddr_in addr;
	struct ServerUtils* dataSer;
	struct ClientUtils* dataCli;
};

struct ClientUtils{
	int sockfd;
	struct sockaddr_in addr;
	struct ServerUtils* dataSer;
	struct ClientUtils* dataCli;
};

int waitMsg(int connfd, char* msg, int MAXLEN);
int sendMsg(int connfd, char* msg, int len);

#endif