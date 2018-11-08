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

class ClientHandler
{
public:
    explicit ClientHandler();
    ~ClientHandler();

    int conncet_login(char* ip, int port, char* username, char* password);

    struct ClientUtils* cc;
};

#endif // CLIENTHANDLER_H
