#include "command.h"
#include "errorcode.h"
#include "socketutils.h"
#include "constants.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>

void initCmd(struct Command* cmd) {
    cmd->cmdName = NULL;
    cmd->params = NULL;
    cmd->num_params = 0;
}

void releCmd(struct Command* cmd) {
    int i;
    if (cmd->cmdName != NULL)
        free(cmd->cmdName);
    cmd->cmdName = NULL;
    for (i = 0; i < cmd->num_params; i++)
        if (cmd->params[i] != NULL){
            free(cmd->params[i]);
        }
    if (cmd->num_params > 0) {
        free(cmd->params);
        cmd->params = NULL;
        cmd->num_params = 0;
    }
}

// void sampleOsType(char* buffer){
//     int f=0;
//     // char buffer[80]="";
//     char *file="/proc/sys/kernel/ostype";
//     f = open(file, O_RDONLY);
//     if (f == 0) {
//         printf("error to open: %s\n", file);
//         exit(EXIT_FAILURE);
//     }
//     read(f, (void *)buffer, 80);
//     int len = strlen(buffer);
//     buffer[len-1]='\r';
//     buffer[len++]='\n';
//     buffer[len]='\0';
//     close(f);
// }

int getlen(char* msg){
    int len = 0;
    while ((*msg) != 0 && (*msg) != ' '){
        len++;
        msg++;
    }
    return len;
}

void insertScode(char* msg){
    size_t len = strlen(msg);
    for (int i = len; i > 0; --i){
        msg[i+3] = msg[i-1];
    }
    msg[0] = '2', msg[1] = '0';
    msg[2] = '0', msg[3] = ' ';
    len = strlen(msg);
    while (len > 0 && (msg[len-1] == '\n' || msg[len-1] == '\r')){
        msg[--len] = '\0';
    }
    msg[len++] = '\r';
    msg[len++] = '\n';
    msg[len] = '\0';
}

int Msg2Command(char* msg, struct Command* cmd) {
    char* tmp = msg;
    int cnt = 0, i, len;
    cmd->cmdName = NULL;
    while (*tmp){
        if ((*tmp) == ' '){
            tmp++;
            continue;
        }
        while ((*tmp) != 0 && *tmp != ' '){
            tmp++;
        }
        cnt++;
    }

    if (cnt == 0) {
        cmd->cmdName = NULL;
        cmd->params = NULL;
        return -ERRORNOCMD;
    }

    while ((*msg) == ' '){
        msg++;
        continue;
    }
    len = getlen(msg);
    cmd->cmdName = malloc((len+1)* sizeof(char));
    for (i = 0; i < len; ++i)
        cmd->cmdName[i] = msg[i];
    cmd->cmdName[len] = '\0';
    msg += len;
    while (cmd->cmdName[len-1] == '\n' || cmd->cmdName[len-1] == '\r')
        cmd->cmdName[--len] = '\0';

    if (cnt == 1) {
        cmd->num_params = 0;
        cmd->params = NULL;
        return 0;
    }

    // get full path
    if (strcmp(cmd->cmdName, "LIST") == 0 ||
        strcmp(cmd->cmdName, "CWD") == 0 ||
        strcmp(cmd->cmdName, "MKD") == 0 ||
        strcmp(cmd->cmdName, "RMD") == 0){
        cmd->num_params = 1;
        cmd->params = malloc(cmd->num_params*sizeof(char*));
        while ((*msg) == ' '){
            msg++;
            continue;
        }
        len = strlen(msg);
        cmd->params[0] = malloc((len+1)*sizeof(char));
        for (i = 0; i < len; ++i)
            cmd->params[0][i] = msg[i];
        while (len > 0 &&
            (cmd->params[0][len-1] == '\n' ||
                cmd->params[0][len-1] == '\r'))
            cmd->params[0][--len] = '\0';
        return 0;
    }

    cmd->num_params = cnt-1;
    cmd->params = malloc(cmd->num_params*sizeof(char*));
    for (i = 0; i < cnt-1; ++i)
        cmd->params[i] = NULL;
    cnt = 0;
    while (*msg){
        if ((*msg) == ' '){
            msg++;
            continue;
        }
        len = getlen(msg);
        cmd->params[cnt] = malloc((len+1)* sizeof(char));
        for (i = 0; i < len; ++i)
            cmd->params[cnt][i] = msg[i];
        cmd->params[cnt][len] = '\0';
        msg += len;
        while (len > 0 &&
            (cmd->params[cnt][len-1] == '\n' ||
                cmd->params[cnt][len-1] == '\r'))
            cmd->params[cnt][--len] = '\0';
        cnt++;
    }
    return 0;
}

int CmdHandle(struct Command cmd, int connfd, char* msg, int maxlen) {
    int p;
    printf("%s\n", cmd.cmdName);

    if (strcmp(cmd.cmdName, "RETR") == 0) {

    } else
    if (strcmp(cmd.cmdName, "STOR") == 0) {

    } else
    if (strcmp(cmd.cmdName, "QUIT") == 0 ||
        strcmp(cmd.cmdName, "ABOR") == 0) {
        printf("In QUIT\n");
        strcpy(msg, byeStr);
        p = sendMsg(connfd, msg, strlen(msg));
        if (p < 0) { // error code!
            printf("sendMsg Error! %d\n", -p);
            return p;
        }
        return -ERRORQUIT;
    } else
    if (strcmp(cmd.cmdName, "SYST") == 0) {
        printf("In SYST\n");
        // sampleOsType(msg);
        strcpy(msg, "215 Linux\r\n\0");
        // printf("%s", msg);
        p = sendMsg(connfd, msg, strlen(msg));
    } else
    if (strcmp(cmd.cmdName, "TYPE") == 0) {
        printf("IN TYPE\n");
        if (cmd.num_params >= 1 && strcmp(cmd.params[0], "I") == 0){
            strcpy(msg, "200 Type set to I.\r\n\0");
            p = sendMsg(connfd, msg, strlen(msg));
        } else
        {
            strcpy(msg, "201 No this type!\r\n\0");
            p = sendMsg(connfd, msg, strlen(msg));
        }
    } else
    if (strcmp(cmd.cmdName, "PORT") == 0) {

    } else
    if (strcmp(cmd.cmdName, "PASV") == 0) {

    } else
    if (strcmp(cmd.cmdName, "MKD") == 0) {
        if (cmd.num_params == 1 && mkdir(cmd.params[0], S_IRWXU | S_IRWXG | S_IRWXO) == 0){
            msg = "200 MKD success!\r\n\0";
            p = sendMsg(connfd, msg, strlen(msg));
        } else
        {
            msg = "500 MKD Failed!\r\n\0";
            p = sendMsg(connfd, msg, strlen(msg));
        }
    } else
    if (strcmp(cmd.cmdName, "CWD") == 0) {
        if (cmd.num_params == 1 && chdir(cmd.params[0]) == 0){
            msg = "200 CWD success!\r\n\0";
            p = sendMsg(connfd, msg, strlen(msg));
        } else
        {
            msg = "500 CWD Failed!\r\n\0";
            p = sendMsg(connfd, msg, strlen(msg));
        }
    } else
    if (strcmp(cmd.cmdName, "PWD") == 0) {
        if (getcwd(msg, maxlen) == NULL){
            printf("Error getcwd");
            msg = "500 PWD Failed!\r\n\0";
            p = sendMsg(connfd, msg, strlen(msg));
        } else
        {
            insertScode(msg);
            p = sendMsg(connfd, msg, strlen(msg));
        }
    } else
    if (strcmp(cmd.cmdName, "LIST") == 0) {

    } else
    if (strcmp(cmd.cmdName, "RMD") == 0) {
        if (cmd.num_params == 1 && rmdir(cmd.params[0]) == 0){
            msg = "200 RMD success!\r\n\0";
            p = sendMsg(connfd, msg, strlen(msg));
        } else
        {
            msg = "500 RMD Failed!\r\n\0";
            p = sendMsg(connfd, msg, strlen(msg));
        }
    } else
    if (strcmp(cmd.cmdName, "RNFR") == 0) {

    } else
    if (strcmp(cmd.cmdName, "RNTO") == 0) {

    } else
    {
        printf("No this cmd!\n");
        strcpy(msg, nocmdStr);
        p = sendMsg(connfd, msg, strlen(msg));
    }

    if (p < 0) {    // error code!
        printf("sendMsg Error! %d\n", -p);
        return p;
    }
    return 0;
}
