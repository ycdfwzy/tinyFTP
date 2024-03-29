#ifndef tinyFTP_command_H
#define tinyFTP_command_H

#ifdef __cplusplus
extern "C"{
#endif

#include "socketutils.h"

struct Command{
    char* cmdName;
    char** params;
    int num_params;
};

void initCmd(struct Command* cmd);
void releCmd(struct Command* cmd);
int Msg2Command(char* msg, struct Command* cmd);
int CmdHandle(struct Command cmd, struct connClient* cc, char* msg);

#ifdef __cplusplus
}
#endif

#endif
