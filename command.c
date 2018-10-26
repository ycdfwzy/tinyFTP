#include "command.h"
#include "errorcode.h"
#include <string.h>
#include <stdlib.h>

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

int CmdHandle(struct Command* cmd, int connfd, char* msg, int maxlen) {
    
}
