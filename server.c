#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include "command.h"
#include "errorcode.h"
#include "socketutils.h"
#include "constants.h"

// const int MAXBUFLEN = 8192;

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
            if (strcmp(cmd.params[0], "anonymous") == 0){
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
                strcpy(sentence, "332 Not supported login method!\r\n\0");
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
            strcpy(sentence, "332 You can do nothing without login!\r\n\0");
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
                strcpy(sentence, "332 No password get!\r\n\0");
                p = sendMsg(connfd, sentence, strlen(sentence));
                if (p < 0) {    // error code
                    printf("Error sendMsg: %d\n", -p);
                    return p;
                }
                continue;
            } else
            {
                printf("Password shouldn't include space!\n");
                strcpy(sentence, "332 Password shouldn't include space!\r\n\0");
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

int comunicate(int connfd, char* sentence, int maxlen) {
    int p;
    struct Command cmd;
    initCmd(&cmd);

    p = waitMsg(connfd, sentence, maxlen);
    if (p < 0) {    // error code!
        printf("waitMsg Error! %d\n", -p);
        return p;
    }
    p = Msg2Command(sentence, &cmd);
    if (p < 0) {    // error code
        printf("Msg2Command Error: %d\n", -p);
        return p;
    }
    p = CmdHandle(cmd, connfd, sentence, maxlen);
    if (p < 0) {    // error code
        printf("CmdHandle Error: %d\n", -p);
        return p;
    }

    releCmd(&cmd);
    return 0;
    
}

int serve_client(int connfd){
	int p, len, i;
	char sentence[MAXBUFLEN];
    

    // p = login(connfd, sentence, MAXBUFLEN);
    // if (p < 0) {    // error code
    //     printf("login Error: %d\n", -p);
    //     return -p;
    // }

    while (1) {
        p = comunicate(connfd, sentence, MAXBUFLEN);
        if (p < 0) {    // error code
            printf("comunicate Error: %d\n", -p);
            return -p;
        }
    }
	return 0;
}

int main(int argc, char **argv) {
	int listenfd, connfd;
	struct sockaddr_in addr;
	// int p;
	// int len;
	if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		// return 1;
		return ERRORSOCKET;
	}


	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(6789);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);


	if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		printf("Error bind(): %s(%d)\n", strerror(errno), errno);
		// return 1;
		return ERRORBIND;
	}


	if (listen(listenfd, 10) == -1) {
		printf("Error listen(): %s(%d)\n", strerror(errno), errno);
		// return 1;
		return ERRORLISTEN;
	}

	while (1) {
        if ((connfd = accept(listenfd, NULL, NULL)) == -1) {
            printf("Error accept(): %s(%d)\n", strerror(errno), errno);
            continue;
        }

		int serve_ret = serve_client(connfd);
		printf("serve_client finished\n");
        if (serve_ret == ERRORQUIT){
            printf("Client disconnect Successfully!\n");
            close(connfd);
            continue;
        } else
		if (serve_ret != 0 && serve_ret != ERRORREAD){
            close(connfd);
			return serve_ret;
		}
        break;
	}
    close(connfd);
	close(listenfd);
	return 0;
}
