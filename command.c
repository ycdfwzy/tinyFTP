#include "command.h"
#include "errorcode.h"
#include "socketutils.h"
#include "constants.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
// #include <fcntl.h>

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
        cmd->params = NULL;
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
        while (cmd->params[cnt][len-1] == '\n' || cmd->params[cnt][len-1] == '\r')
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
        printf("%s", msg);
        p = sendMsg(connfd, msg, strlen(msg));
    } else
    if (strcmp(cmd.cmdName, "TYPE") == 0) {

    } else
    if (strcmp(cmd.cmdName, "PORT") == 0) {

    } else
    if (strcmp(cmd.cmdName, "PASV") == 0) {

    } else
    if (strcmp(cmd.cmdName, "MKD") == 0) {

    } else
    if (strcmp(cmd.cmdName, "CWD") == 0) {

    } else
    if (strcmp(cmd.cmdName, "PWD") == 0) {

    } else
    if (strcmp(cmd.cmdName, "LIST") == 0) {

    } else
    if (strcmp(cmd.cmdName, "RMD") == 0) {

    } else
    if (strcmp(cmd.cmdName, "RNFR") == 0) {

    } else
    if (strcmp(cmd.cmdName, "RNTO") == 0) {

    } else
    {

    }
    if (p < 0) {    // error code!
        printf("sendMsg Error! %d\n", -p);
        return p;
    }
    return 0;
}
