#ifndef tinyFTP_socketuitls_H
#define tinyFTP_socketuitls_H
#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <stdlib.h>
#include <string.h>
#include "errorcode.h"

int waitMsg(int connfd, char* msg, int MAXLEN);
int sendMsg(int connfd, char* msg, int len);

#endif