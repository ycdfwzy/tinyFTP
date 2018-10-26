#ifndef tinyFTP_command_H
#define tinyFTP_command_H
#include <string.h>
#include "errorcode.h"
struct Command{
    char* cmdName;
    char** params;
    int num_params;
};

void initCmd(struct Command* cmd);
void releCmd(struct Command* cmd);
int Msg2Command(char* msg, struct Command* cmd);
int CmdHandle(struct Command* cmd, int connfd, char* msg, int maxlen);

#endif