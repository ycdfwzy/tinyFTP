#ifndef tinyFTP_command_H
#define tinyFTP_command_H
#include <string.h>
enum curmode{
    NONE = 0,
    PASV,
    PORT
};
struct Command{
    char* cmdName;
    char** params;
    int num_params;
};

void initCmd(struct Command* cmd);
void releCmd(struct Command* cmd);
int Msg2Command(char* msg, struct Command* cmd);
int CmdHandle(struct Command cmd, int connfd, char* msg, int maxlen);

#endif