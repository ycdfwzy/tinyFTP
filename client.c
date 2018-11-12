#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <errno.h>

#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <pthread.h>
#include "socketutils.h"
#include "constants.h"
#include "command.h"
#include "datautils.h"

// const int MAXBUFLEN = 8192;
char rec[MAXBUFLEN];
char snd[MAXBUFLEN];
struct ClientUtils client;

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

int toRename = 0;

int cmd_RNFR(char* snd, char* rec, struct ClientUtils* cu) {
    int p = sendMsg(cu->sockfd, snd, strlen(snd));
    int idx;
    if (p < 0) {    // error code!
        printf("sendMsg Error! %d\n", -p);
        return p;
    }

    do{
        p = waitMsg(cu->sockfd, rec, MAXBUFLEN);
        if (p < 0) {    // error code!
            printf("waitMsg Error! %d\n", -p);
            return p;
        }
        idx = indexofDDDS(rec);
    } while (idx < 0);

    printf("From server: %s\n", rec+idx+4);
    if (startWith(rec+idx, "350 ")) {
        toRename = 1;
    }
    return 0;
}

int cmd_RNTO(char* snd, char* rec, struct ClientUtils* cu) {
    int idx;
    if (toRename == 0){
        printf("Please give RNFR cmd first!\n");
        return 0;
    }

    int p = sendMsg(cu->sockfd, snd, strlen(snd));
    if (p < 0) {    // error code!
        printf("sendMsg Error! %d\n", -p);
        return p;
    }

    do{
        p = waitMsg(cu->sockfd, rec, MAXBUFLEN);
        if (p < 0) {    // error code!
            printf("waitMsg Error! %d\n", -p);
            // releCmd(&cmd);
            return p;
        }
        idx = indexofDDDS(rec);
    } while (idx < 0);

    printf("From server: %s\n", rec+idx+4);
    // if (startWith(rec, "250 ")) {
        toRename = 0;
    // }
    return 0;
}

int cmd_PORT(struct Command* cmd, char* snd, char* rec, struct ClientUtils* cu) {
    int idx, p;
    dropOtherConn_Client(cu);

    if (cmd->num_params != 1) {
        printf("Please input ip and port!\n");
        return 0;
    }
    p = crtSer_Client(cmd->params[0], cu);
    if (p < 0){
        printf("Please change ip or port!\n");
        return 0;
    }

    p = sendMsg(cu->sockfd, snd, strlen(snd));
    if (p < 0){
        printf("sendMsg Error! %d\n", -p);
        return p;
    }
    /*nonblock*/
    pthread_t pid;
    pthread_create(&pid, NULL, waitConn_thread, (void*)cu->dataSer);
    pthread_detach(pid);
    // p = waitConn(cu->dataSer);
    // if (p < 0){
    //     printf("waitConn Error! %d\n", -p);
    //     return p;
    // }
    do{
        p = waitMsg(cu->sockfd, rec, MAXBUFLEN);
        if (p < 0) {    // error code!
            printf("waitMsg Error! %d\n", -p);
            return p;
        }
        idx = indexofDDDS(rec);
    } while (idx < 0);
    printf("From server: %s\n", rec+idx+4);

    if (startWith(rec+idx, "200 ")){
        printf("Now you are in PORT mode\nPlease input cmd STOR/RETR/LIST!\n");
    }

    return 0;
}

int cmd_PASV(char* snd, char* rec, struct ClientUtils* cu) {
    int idx, p;
    dropOtherConn_Client(cu);

    p = sendMsg(cu->sockfd, snd, strlen(snd));
    do{
        p = waitMsg(cu->sockfd, rec, MAXBUFLEN);
        if (p < 0) {    // error code!
            printf("waitMsg Error! %d\n", -p);
            return p;
        }
        idx = indexofDDDS(rec);
    } while (idx < 0);
    printf("From server: %s\n", rec+idx+4);

    if (startWith(rec+idx, "227 ")){
        p = conSer_Client(rec+5, cu);
        if (p < 0){
            printf("connect data Server Error! %d\n", -p);
            return p;
        }

        printf("Now you are in PASV mode\nPlease input cmd STOR/RETR/LIST!\n");
    }

    return 0;
}

int cmd_STOR(struct Command* cmd, char* snd, char* rec, struct ClientUtils* cu) {
    int idx, p;
    if (cu->dataSer == NULL && cu->dataCli == NULL){
        printf("Please choose PASV/PORT mode first!\n");
        return 0;
    }
    p = sendMsg(cu->sockfd, snd, strlen(snd));
    if (p < 0) {    // error code!
        printf("sendMsg Error! %d\n", -p);
        dropOtherConn_Client(cu);
        return p;
    }

    do{
        p = waitMsg(cu->sockfd, rec, MAXBUFLEN);
        if (p < 0) {    // error code!
            printf("waitMsg Error! %d\n", -p);
            dropOtherConn_Client(cu);
            return p;
        }
        idx = indexofDDDS(rec);
    } while (idx < 0);
    printf("From server: %s\n", rec+idx+4);

    if (startWith(rec+idx, "150 ")){
        printf("Start transfer...\n");
        if (cu->dataSer != NULL) {
            p = send_file(cmd->params[0], cu->dataSer->conn[0].connfd);
        } else
        {
            p = send_file(cmd->params[0], cu->dataCli->sockfd);
        }

        // if (p == 0){
        //     sleep(1);
        // } else
        // if (p == -ERRORREADFROMDISC){
        //     printf("Error when read from disk!\n");
        // } else
        // if (p == -ERRORDISCONN){
        //     printf("data connection Disconnect!\n");
        // }
        
        dropOtherConn_Client(cu);
        do{
            p = waitMsg(cu->sockfd, rec, MAXBUFLEN);
            if (p < 0) {    // error code!
                printf("waitMsg Error! %d\n", -p);
                return p;
            }
            idx = indexofDDDS(rec);
        } while (idx < 0);
        printf("From server: %s\n", rec+idx+4);

        if (startWith(rec+idx, "226 ")){
            printf("STOR Successfully!\n");
        } else
        {
            printf("some error happend when STOR!\n");
        }
    } else
    {
        printf("STOR failed!\n");
        dropOtherConn_Client(cu);
    }

    return 0;
}

int cmd_RETR(struct Command* cmd, char* snd, char* rec, struct ClientUtils* cu) {
    int idx, p;
    if (cu->dataSer == NULL && cu->dataCli == NULL){
        printf("Please choose PASV/PORT mode first!\n");
        return 0;
    }
    p = sendMsg(cu->sockfd, snd, strlen(snd));
    if (p < 0) {    // error code!
        printf("sendMsg Error! %d\n", -p);
        dropOtherConn_Client(cu);
        return p;
    }

    do{
        p = waitMsg(cu->sockfd, rec, MAXBUFLEN);
        if (p < 0) {    // error code!
            printf("waitMsg Error! %d\n", -p);
            dropOtherConn_Client(cu);
            return p;
        }
        idx = indexofDDDS(rec);
    } while (idx < 0);
    printf("From server: %s\n", rec+idx+4);

    if (startWith(rec+idx, "150 ")){
        printf("Start transfer...\n");
        char tmp[512];
        strcpy(tmp, cmd->params[0]);
        getfilename(tmp);
        if (cu->dataSer != NULL) {
            p = recv_file_append(tmp, cu->dataSer->conn[0].connfd);
        } else
        {
            p = recv_file_append(tmp, cu->dataCli->sockfd);
        }

        // if (p == -ERRORREADFROMDISC){
        //     printf("Error when write to disk!\n");
        // } else
        // if (p == -ERRORDISCONN){
        //     printf("data connection Disconnect!\n");
        // }

        dropOtherConn_Client(cu);
        do{
            p = waitMsg(cu->sockfd, rec, MAXBUFLEN);
            if (p < 0) {    // error code!
                printf("waitMsg Error! %d\n", -p);
                return p;
            }
            idx = indexofDDDS(rec);
        } while (idx < 0);
        printf("From server: %s\n", rec+idx+4);

        if (startWith(rec+idx, "226 ")){
            printf("RETR Successfully!\n");
        } else
        {
            printf("some error happend when RETR!\n");
        }

    } else
    {
        printf("RETR failed!\n");
        dropOtherConn_Client(cu);
    }

    return 0;
}

int cmd_LIST(char* snd, char* rec, struct ClientUtils* cu) {
    int idx, p;
    if (cu->dataSer == NULL && cu->dataCli == NULL){
        printf("Please choose PASV/PORT mode first!\n");
        return 0;
    }

    p = sendMsg(cu->sockfd, snd, strlen(snd));
    if (p < 0) {    // error code!
        printf("sendMsg Error! %d\n", -p);
        dropOtherConn_Client(cu);
        return p;
    }

    do{
        p = waitMsg(cu->sockfd, rec, MAXBUFLEN);
        if (p < 0) {    // error code!
            printf("waitMsg Error! %d\n", -p);
            dropOtherConn_Client(cu);
            return p;
        }
        idx = indexofDDDS(rec);
    } while (idx < 0);
    printf("From server: %s\n", rec+idx+4);

    if (startWith(rec+idx, "150 ")){
        if (cu->dataSer != NULL) {
            p = recv_list(cu->dataSer->conn[0].connfd, NULL);
        } else
        {
            p = recv_list(cu->dataCli->sockfd, NULL);
        }

        dropOtherConn_Client(cu);

        do{
            p = waitMsg(cu->sockfd, rec, MAXBUFLEN);
            if (p < 0) {    // error code!
                printf("waitMsg Error! %d\n", -p);
                return p;
            }
            idx = indexofDDDS(rec);
        } while (idx < 0);
        printf("From server: %s\n", rec+idx+4);

        if (startWith(rec+idx, "226 ")){
            printf("LIST Successfully!\n");
        } else
        {
            printf("some error happend when LIST!\n");
        }
    } else
    {
        printf("LIST failed!\n");
        dropOtherConn_Client(cu);
    }
    return 0;
}

int cmd_NORMAL(char* snd, char* rec, struct ClientUtils* cu) {
    int idx;
    int p = sendMsg(cu->sockfd, snd, strlen(snd));
    if (p < 0) {    // error code!
        printf("sendMsg Error! %d\n", -p);
        return p;
    }

    do{
        p = waitMsg(cu->sockfd, rec, MAXBUFLEN);
        if (p < 0) {    // error code!
            printf("waitMsg Error! %d\n", -p);
            // releCmd(&cmd);
            return p;
        }
        idx = indexofDDDS(rec);
    } while (idx < 0);
    printf("From server: %s\n", rec+idx+4);

    if (isByeMsg(rec)){
        // releCmd(&cmd);
        return -ERRORQUIT;
    }

    return 0;
}

int communicate(struct ClientUtils* cu) {
    // char tmp[8192];
    // int p;

    // dropOtherConn_Client(cu);

    // p = crtSer_Client(tmp, cu);
    // sprintf(snd, "PORT %s\r\n", tmp);
    // printf("to send: %s", snd);
    // p = sendMsg(cu->sockfd, snd, strlen(snd));
    // waitConn(cu->dataSer);
    // p = waitMsg(cu->sockfd, rec, MAXBUFLEN);

    // sprintf(snd, "RETR test.png\r\n");
    // printf("to send: %s", snd);
    // p = sendMsg(cu->sockfd, snd, strlen(snd));
    // p = waitMsg(cu->sockfd, rec, MAXBUFLEN);
    // if (startWith(rec, "150 ")){
    //     recv_file("/home/ycdfwzy/test.png", cu->dataSer->conn[0].connfd);
    //     // dropOtherConn_Client(cu);
    //     p = waitMsg(cu->sockfd, rec, MAXBUFLEN);
    // }

    // dropOtherConn_Client(cu);

    // sprintf(snd, "CWD /home/ycdfwzy/Pictures\r\n");
    // p = sendMsg(cu->sockfd, snd, strlen(snd));
    // p = waitMsg(cu->sockfd, rec, MAXBUFLEN);

    // sprintf(snd, "PASV\r\n");
    // p = sendMsg(cu->sockfd, snd, strlen(snd));
    // p = waitMsg(cu->sockfd, rec, MAXBUFLEN);

    // p = conSer_Client(rec+5, cu);

    // sprintf(snd, "STOR snoopy.png\r\n");
    // p = sendMsg(cu->sockfd, snd, strlen(snd));
    // send_file("/home/ycdfwzy/Pictures/snoopy.png", cu->dataCli->sockfd);
    // sleep(1);
    // dropOtherConn_Client(cu);
    // p = waitMsg(cu->sockfd, rec, MAXBUFLEN);

    // sprintf(snd, "LIST /home/ycdfwzy\r\n");
    // printf("to send: %s", snd);
    // p = sendMsg(cu->sockfd, snd, strlen(snd));
    // recv_list(cu->dataSer->conn[0].connfd);
    // p = waitMsg(cu->sockfd, rec, MAXBUFLEN);

    // dropOtherConn_Client(cu);

    // sprintf(snd, "PASV\r\n");
    // p = sendMsg(cu->sockfd, snd, strlen(snd));
    // p = waitMsg(cu->sockfd, rec, MAXBUFLEN);
    // // struct Command cmd;
    // // initCmd(&cmd);

    // // Msg2Command(rec, &cmd);
    // p = conSer_Client(rec+5, cu);

    // sprintf(snd, "LIST /home/ycdfwzy/oh-my-tuna.py\r\n");
    // p = sendMsg(cu->sockfd, snd, strlen(snd));
    // printf("before recv_list, sockfd=%d\n", cu->dataCli->sockfd);
    // recv_list(cu->dataCli->sockfd);
    // p = waitMsg(cu->sockfd, rec, MAXBUFLEN);
    // dropOtherConn_Client(cu);

    // sprintf(snd, "QUIT\r\n");
    // p = sendMsg(cu->sockfd, snd, strlen(snd));
    // p = waitMsg(cu->sockfd, rec, MAXBUFLEN);

    // int sockfd = cu->sockfd;
    // char tmp[8192];

    struct Command cmd;
    initCmd(&cmd);
    getInput(snd);
    // int len = getInput(snd);
    // printf("%s %d\n", snd, len);

    int p = Msg2Command(snd, &cmd);
    if (p < 0){
        return 0;
    }

    if (strcmp(cmd.cmdName, "RNFR") == 0) {
        p = cmd_RNFR(snd, rec, cu);
    } else
    if (strcmp(cmd.cmdName, "RNTO") == 0) {
        p = cmd_RNTO(snd, rec, cu);
    } else
    if (strcmp(cmd.cmdName, "PORT") == 0) {
        p = cmd_PORT(&cmd, snd, rec, cu);
    } else
    if (strcmp(cmd.cmdName, "PASV") == 0) {
        p = cmd_PASV(snd, rec, cu);
    } else
    if (strcmp(cmd.cmdName, "STOR") == 0) {
        p = cmd_STOR(&cmd, snd, rec, cu);
    } else
    if (strcmp(cmd.cmdName, "RETR") == 0) {
        p = cmd_RETR(&cmd, snd, rec, cu);
    } else
    if (strcmp(cmd.cmdName, "LIST") == 0){
        p = cmd_LIST(snd, rec, cu);
    } else
    {
        p = cmd_NORMAL(snd, rec, cu);
    }

    if (p < 0){
        if (-p != ERRORQUIT)
            printf("cmd handle error! %d\n", p);
        releCmd(&cmd);
        return p;
    }

    releCmd(&cmd);
    return 0;
}

int main(int argc, char **argv) {
	// int sockfd;
	// struct sockaddr_in addr;
    int port = 6789;
    char ipaddr[20] = "127.0.0.1";
    if (argc == 3 || argc == 5){
        for (int i = 1; i < argc; i += 2){
            if (strcmp(argv[i], "-port") == 0){
                sscanf(argv[i+1], "%d", &port);
            } else
            if (strcmp(argv[i], "-ip") == 0){
                strcpy(ipaddr, argv[i+1]);
            }
        }
    }

    char sentence[MAXBUFLEN];
    initClientUtils(&client);
	int p;

    if ((client.sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("Error socket(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }

    memset(&client.addr, 0, sizeof(client.addr));
    client.addr.sin_family = AF_INET;
    client.addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ipaddr, &(client.addr.sin_addr)) <= 0) {
        printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }

    if (connect(client.sockfd, (struct sockaddr*)&(client.addr), sizeof(client.addr)) < 0) {
        printf("Error connect(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }
    printf("sockid=%d\n", client.sockfd);
    p = login(client.sockfd, sentence, MAXBUFLEN);
    if (p < 0) {
        close(client.sockfd);
        printf("login Error! %d\n", -p);
        return -p;
    }
    while (1) {
        p = communicate(&client);
        if (p == -ERRORQUIT){
            printf("Disconnect Successfully!\n");
            break;
        } else
        if (p < 0) {
            close(client.sockfd);
            printf("Error when communicate! %d\n", -p);
            return -p;
        }
        // break;
    }
    close(client.sockfd);
    releClientUtils(&client);
	return 0;
}
