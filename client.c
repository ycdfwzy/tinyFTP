#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <errno.h>

#include <string.h>
#include <memory.h>
#include <stdio.h>
#include "socketutils.h"
#include "constants.h"
#include "command.h"
#include "datautils.h"

#define isDigit(c) ((c)>='0' && (c)<='9')
#define isSpace(c) ((c)==' ')
// const int MAXBUFLEN = 8192;
char rec[8192];
char snd[8192];
struct ClientUtils client;

// check if string strat with Digit-Digit-Digit-Space
int startWithDDDS(const char* st){
    size_t len = strlen(st);
    if (len < 4){
        return 0;
    }
    return (isDigit(st[0]) && isDigit(st[1]) &&
            isDigit(st[2]) && isSpace(st[3]));
}

int startWith(const char* s, const char* t) {
    size_t ls = strlen(s);
    size_t lt = strlen(t);
    if (ls < lt) return 0;
    for (unsigned i = 0; i < lt; ++i)
        if (s[i] != t[i])
            return 0;
    return 1;
}

int getInput(char* s){
    fgets(s, 4096, stdin);
    int len = strlen(s);
    s[len++] = '\n';
    s[len] = '\0';
    return len;
}

int isByeMsg(char* msg){
    int len = strlen(msg);
    while (len > 0 && (msg[len-1] == '\n' || msg[len-1] == '\r')){
        msg[--len] = '\0';
    }
    msg[len++] = '\r';
    msg[len++] = '\n';
    msg[len] = '\0';
    return strcmp(msg, byeStr) == 0;
}

int login(int sockfd, char* sentence, int maxlen){
    int p, len;

    printf("Wait Server response!\n");
    do {
        p = waitMsg(sockfd, sentence, maxlen);
        if (p < 0) {    // error code!
            printf("waitMsg Error! %d\n", -p);
            return p;
        }
    } while (!startWith(sentence, "220 "));
    printf("Server connected!\n");

    // user anonymous
    do {
        len = getInput(sentence);
        p = sendMsg(sockfd, sentence, len);
        if (p < 0) {    // error code!
            printf("sendMsg Error! %d\n", -p);
            return p;
        }

        p = waitMsg(sockfd, sentence, maxlen);
        if (p < 0) {    // error code!
            printf("waitMsg Error! %d\n", -p);
            return p;
        }
    } while (!startWith(sentence, "331 "));

    // PASS password
    do {
        len = getInput(sentence);
        p = sendMsg(sockfd, sentence, len);
        if (p < 0) {    // error code!
            printf("sendMsg Error! %d\n", -p);
            return p;
        }

        p = waitMsg(sockfd, sentence, maxlen);
        if (p < 0) {    // error code!
            printf("waitMsg Error! %d\n", -p);
            return p;
        }
    } while (!startWith(sentence, "230"));

    printf("Login Successful!\n");
    return 0;
}

int communicate(struct ClientUtils* cu) {
    char tmp[8192];
    int p;

    dropOtherConn_Client(cu);

    p = crtSer_Client(tmp, cu);
    sprintf(snd, "PORT %s\r\n", tmp);
    printf("to send: %s", snd);
    p = sendMsg(cu->sockfd, snd, strlen(snd));
    waitConn(cu->dataSer);
    p = waitMsg(cu->sockfd, rec, MAXBUFLEN);


    sprintf(snd, "LIST /home/ycdfwzy\r\n");
    printf("to send: %s", snd);
    p = sendMsg(cu->sockfd, snd, strlen(snd));
    recv_list(cu->dataSer->conn[0].connfd);
    p = waitMsg(cu->sockfd, rec, MAXBUFLEN);

    dropOtherConn_Client(cu);

    sprintf(snd, "PASV\r\n");
    p = sendMsg(cu->sockfd, snd, strlen(snd));
    p = waitMsg(cu->sockfd, rec, MAXBUFLEN);
    // struct Command cmd;
    // initCmd(&cmd);

    // Msg2Command(rec, &cmd);
    p = conSer_Client(rec+5, cu);

    sprintf(snd, "LIST /home/ycdfwzy/oh-my-tuna.py\r\n");
    p = sendMsg(cu->sockfd, snd, strlen(snd));
    printf("before recv_list, sockfd=%d\n", cu->dataCli->sockfd);
    recv_list(cu->dataCli->sockfd);
    p = waitMsg(cu->sockfd, rec, MAXBUFLEN);
    dropOtherConn_Client(cu);

    sprintf(snd, "QUIT\r\n");
    p = sendMsg(cu->sockfd, snd, strlen(snd));
    p = waitMsg(cu->sockfd, rec, MAXBUFLEN);
    // int sockfd = cu->sockfd;
    // char tmp[8192];

    // struct Command cmd;
    // initCmd(&cmd);
    // int len = getInput(snd);

    // Msg2Command(snd, &cmd);

    // if (strcmp(cmd->cmdName, "PORT") == 0 &&
    //     cmd->num_params == 1){

    //     p = crtSer(tmp, cu);
    // }

    // int p = sendMsg(sockfd, snd, len);
    // if (p < 0) {    // error code!
    //     printf("sendMsg Error! %d\n", -p);
    //     releCmd(&cmd);
    //     return p;
    // }

    // do{
    //     p = waitMsg(sockfd, rec, MAXBUFLEN);
    //     if (p < 0) {    // error code!
    //         printf("waitMsg Error! %d\n", -p);
    //         releCmd(&cmd);
    //         return p;
    //     }
    // } while (!startWithDDDS(rec));

    // if (isByeMsg(rec)){
    //     releCmd(&cmd);
    //     return -ERRORQUIT;
    // }
    // releCmd(&cmd);
    return 0;
}

int main(int argc, char **argv) {
	// int sockfd;
	// struct sockaddr_in addr;
    initClientUtils(&client);
	int p;

    if ((client.sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("Error socket(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }

    memset(&client.addr, 0, sizeof(client.addr));
    client.addr.sin_family = AF_INET;
    client.addr.sin_port = htons(6789);
    if (inet_pton(AF_INET, "127.0.0.1", &(client.addr.sin_addr)) <= 0) {
        printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }

    if (connect(client.sockfd, (struct sockaddr*)&(client.addr), sizeof(client.addr)) < 0) {
        printf("Error connect(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }

    // p = login(sockfd, sentence, 8192);
    // if (p < 0) {
    //     close(sockfd);
    //     printf("login Error! %d\n", -p);
    //     return -p;
    // }
    while (1) {
        p = communicate(&client);
        if (p == -ERRORQUIT){
            printf("Disconnect Successfully!\n");
            break;
        } else
        if (p < 0) {
            close(client.sockfd);
            printf("login Error! %d\n", -p);
            return -p;
        }
        break;
    }
    close(client.sockfd);
    releClientUtils(&client);
	return 0;
}
