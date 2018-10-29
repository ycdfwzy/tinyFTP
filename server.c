#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "command.h"
#include "errorcode.h"
#include "socketutils.h"
#include "constants.h"

struct ServerUtils server;

int login(int connfd, char* sentence, int maxlen) {
    struct Command cmd;
    initCmd(&cmd);

    // sentence = "220 Anonymous FTP server ready.\r\n\0";
    strcpy(sentence, readyStr);
    int p = sendMsg(connfd, sentence, strlen(sentence));
    if (p < 0) {    // error code
        printf("Error sendMsg: %d\n", -p);
        return p;
    }

    // login method: only support anonymous
    while (1) {
        int len = waitMsg(connfd, sentence, maxlen);
        if (len < 0) {   // error code
            printf("waitMsg Error: %d\n", -len);
            return len;
        }
        p = Msg2Command(sentence, &cmd);
        if (p < 0) {    // error code
            printf("Msg2Command Error: %d\n", -p);
            return p;
        }
        if (strcmp(cmd.cmdName, "USER") == 0) {
            if (cmd.num_params == 1 && strcmp(cmd.params[0], "anonymous") == 0){
                strcpy(sentence, passwStr);
                // sentence = "331 Guest login ok, send your complete e-mail address as password.\n\0";
                p = sendMsg(connfd, sentence, strlen(sentence));
                if (p < 0) {
                    printf("Error sendMsg: %d\n", -p);
                    return p;
                }
                break;
            } else
            {
                strcpy(sentence, "530 Not supported login method!\r\n\0");
                printf("Not supported login method!\n");
                p = sendMsg(connfd, sentence, strlen(sentence));
                if (p < 0) {
                    printf("Error sendMsg: %d\n", -p);
                    return p;
                }
                releCmd(&cmd);
                continue;
            }
        } else
        {
            strcpy(sentence, "530 You can do nothing without login!\r\n\0");
            printf("You can do nothing without login!\n");
            p = sendMsg(connfd, sentence, strlen(sentence));
            if (p < 0) {
                printf("Error sendMsg: %d\n", -p);
                return p;
            }
            releCmd(&cmd);
            continue;
        }
    }
    printf("login method OK\n");

    // access password
    while (1) {
        int len = waitMsg(connfd, sentence, maxlen);
        if (len < 0) {   // error code
            printf("waitMsg Error: %d\n", -len);
            return len;
        }
        p = Msg2Command(sentence, &cmd);
        if (p < 0) {    // error code
            printf("Msg2Command Error: %d\n", -p);
            return p;
        }

        if (strcmp(cmd.cmdName, "PASS") == 0){
            if (cmd.num_params == 1) {
                printf("password: %s", cmd.params[0]);
                strcpy(sentence, greetStr);
                p = sendMsg(connfd, sentence, strlen(sentence));
                if (p < 0) {    // error code
                    printf("Error sendMsg: %d\n", -p);
                    return p;
                }
                break;
            } else
            if (cmd.num_params == 0) {
                printf("No password get!\n");
                strcpy(sentence, "503 No password get!\r\n\0");
                p = sendMsg(connfd, sentence, strlen(sentence));
                if (p < 0) {    // error code
                    printf("Error sendMsg: %d\n", -p);
                    return p;
                }
                continue;
            } else
            {
                printf("Password shouldn't include space!\n");
                strcpy(sentence, "503 Password shouldn't include space!\r\n\0");
                p = sendMsg(connfd, sentence, strlen(sentence));
                if (p < 0) {    // error code
                    printf("Error sendMsg: %d\n", -p);
                    return p;
                }
                continue;
            }

        } else
        {
            printf("Please send password!\n");
            strcpy(sentence, "332 Please send password!\r\n\0");
            p = sendMsg(connfd, sentence, strlen(sentence));
            if (p < 0) {    // error code
                printf("Error sendMsg: %d\n", -p);
                return p;
            }
            continue;
        }
    }
    printf("access password OK\n");


    printf("login Successful!\n");
    return 0;
}

int comunicate(struct connClient* cc, char* sentence, int maxlen) {
    int p;
    int connfd = cc->connfd;
    struct Command cmd;
    initCmd(&cmd);

    p = waitMsg(connfd, sentence, maxlen);
    if (p < 0) {    // error code!
        printf("waitMsg Error! %d\n", -p);
        releCmd(&cmd);
        return p;
    }
    p = Msg2Command(sentence, &cmd);
    if (p < 0) {    // error code
        printf("Msg2Command Error: %d\n", -p);
        releCmd(&cmd);
        return p;
    }
    p = CmdHandle(cmd, cc, sentence, maxlen);
    if (p < 0) {    // error code
        printf("CmdHandle Error: %d\n", -p);
        releCmd(&cmd);
        return p;
    }

    releCmd(&cmd);
    return 0;
}

int serve_client(struct connClient* cc){
	int p;
	char sentence[MAXBUFLEN];
    int connfd = cc->connfd;
    
    // p = login(connfd, sentence, MAXBUFLEN);
    // if (p < 0) {    // error code
    //     printf("login Error: %d\n", -p);
    //     return -p;
    // }

    while (1) {
        p = comunicate(cc, sentence, MAXBUFLEN);
        if (p < 0) {    // error code
            printf("comunicate Error: %d\n", -p);
            return -p;
        }
    }
	return 0;
}

void* thread_serve_client(void* param){
    struct connClient* cc = (struct connClient*)param;

    int serve_ret = serve_client(cc);

    printf("serve_client %d finished\n", cc->connfd);
    if (serve_ret == ERRORQUIT){
        printf("Client %d disconnect Successfully!\n", cc->connfd);
        close(cc->connfd);
        releconnClient(cc);
        return NULL;//continue;
    } else
    if (serve_ret != 0 && serve_ret != ERRORREAD){
        close(cc->connfd);
        releconnClient(cc);
        return NULL;
    }
    close(cc->connfd);
    releconnClient(cc);

    return NULL;
}

int main(int argc, char **argv) {
    int port = 6789;
    if (argc == 3 || argc == 5){
        for (int i = 1; i < 5; i += 2){
            if (strcmp(argv[i], "-port") == 0){
                sscanf(argv[i+1], "%d", &port);
            } else
            if (strcmp(argv[i], "-root") == 0){
                chdir(argv[i+1]);
            }
        }
    }
    printf("port = %d\n", port);
    char tmp[100];
    getcwd(tmp, 100);
    printf("curdir = %s\n", tmp);

	initServerUtils(&server);
	if ((server.listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		// return 1;
		return ERRORSOCKET;
	}

	memset(&(server.addr), 0, sizeof(server.addr));
	server.addr.sin_family = AF_INET;
	server.addr.sin_port = htons(port);
	server.addr.sin_addr.s_addr = htonl(INADDR_ANY);


	if (bind(server.listenfd, (struct sockaddr*)&(server.addr), sizeof(server.addr)) == -1) {
		printf("Error bind(): %s(%d)\n", strerror(errno), errno);
		// return 1;
		return ERRORBIND;
	}


	if (listen(server.listenfd, MAXCONN) == -1) {
		printf("Error listen(): %s(%d)\n", strerror(errno), errno);
		// return 1;
		return ERRORLISTEN;
	}

	while (1) {
        int index = -1;
        for (int i = 0; i < MAXCONN; ++i){
            if (server.conn[i].connfd == -1){
                index = i;
                break;
            }
        }
        if (index == -1)    // no idle conn
            continue;
        if ((server.conn[index].connfd = accept(server.listenfd, NULL, NULL)) == -1) {
            printf("Error accept(): %s(%d)\n", strerror(errno), errno);
            continue;
        }

        getcwd(server.conn[index].curdir, 512);

        // int serve_ret = serve_client(&server.conn[index]);
        // printf("serve_client %d finished\n", server.conn[index].connfd);
        // if (serve_ret == ERRORQUIT){
        //     printf("Client %d disconnect Successfully!\n", server.conn[index].connfd);
        //     close(server.conn[index].connfd);
        //     releconnClient(&(server.conn[index]));
        //     continue;
        // } else
        // if (serve_ret != 0 && serve_ret != ERRORREAD){
        //     close(server.conn[index].connfd);
        //     releconnClient(&(server.conn[index]));
        //     return serve_ret;
        // }
        // close(server.conn[index].connfd);
        // releconnClient(&(server.conn[index]));

        pthread_t pid;
        pthread_create(&pid, NULL, thread_serve_client, (void*)&(server.conn[index]));
		pthread_detach(pid);
        // break;
	}
    // close(connfd);
	close(server.listenfd);
    releServerUtils(&server);
	return 0;
}
