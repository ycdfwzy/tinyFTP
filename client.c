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

#define isDigit(c) ((c)>='0' && (c)<='9')
#define isSpace(c) ((c)==' ')
// const int MAXBUFLEN = 8192;
char rec[8192];
char snd[8192];

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
    for (int i = 0; i < lt; ++i)
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

int communicate(int sockfd) {
    int len = getInput(snd);
    int p = sendMsg(sockfd, snd, len);
    if (p < 0) {    // error code!
        printf("sendMsg Error! %d\n", -p);
        return p;
    }

    p = waitMsg(sockfd, rec, MAXBUFLEN);
    if (p < 0) {    // error code!
        printf("waitMsg Error! %d\n", -p);
        return p;
    }

    if (isByeMsg(rec)){
        return -ERRORQUIT;
    }
    return 0;
}

int main(int argc, char **argv) {
	int sockfd;
	struct sockaddr_in addr;
	int p;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("Error socket(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = 6789;
    if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) <= 0) {
        printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }


    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
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
        p = communicate(sockfd);
        if (p == -ERRORQUIT){
            printf("Disconnect Successfully!\n");
            break;
        } else
        if (p < 0) {
            close(sockfd);
            printf("login Error! %d\n", -p);
            return -p;
        }
    }
    close(sockfd);
	return 0;
}
