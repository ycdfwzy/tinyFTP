#include "command.h"
#include "errorcode.h"
#include "socketutils.h"
#include "constants.h"
#include "datautils.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

// char oldpath[8192] = "\0";
char curpath[8192];

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

void format(char* s, char* t) {
    int len = strlen(s);
    int i, j;
    for (i = 0, j = 0; i < len; ++i){
        if (s[i] == '"'){
            t[j++] = '"';
            t[j++] = '"';
        } else
            t[j++] = s[i];
        }
    t[j] = '\0';
}

void getfilename(char* path){
    int len = strlen(path);
    char tmp[MAXBUFLEN];
    if (len == 0) return;
    for (int j = len-1; j >= 0; j--){
        if (path[j] == '/'){
            strcpy(tmp, path+j+1);
            strcpy(path, tmp);
            return;
        }
    }
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
    while (len > 0 && (cmd->cmdName[len-1] == '\n' || cmd->cmdName[len-1] == '\r'))
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
        strcmp(cmd->cmdName, "RMD") == 0 ||
        strcmp(cmd->cmdName, "RNFR") == 0 ||
        strcmp(cmd->cmdName, "RNTO") == 0 ||
        strcmp(cmd->cmdName, "PORT") == 0){
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

void toabsPath(char* oripath, char* curdir){
    if (strlen(oripath) > 0 && oripath[0] == '/')
        return;
    char* tmp = malloc(sizeof(char)*8192);
    strcpy(tmp, oripath);
    if (curdir[strlen(curdir)-1]=='/')
        sprintf(oripath, "%s%s", curdir, tmp);
    else
        sprintf(oripath, "%s/%s", curdir, tmp);
    free(tmp);
}

int CmdHandle(struct Command cmd, struct connClient* cc, char* msg, int maxlen) {
    int p;
    int connfd = cc->connfd;
    char tmp[8192];
    printf("%s\n", cmd.cmdName);

    if (cc->oldpath[0] != '\0'){ // RNFR to handle
        if (strcmp(cmd.cmdName, "RNTO") != 0){
            msg = "503 Please send target name!\r\n\0";
            p = sendMsg(connfd, msg, strlen(msg));
            if (p < 0) {    // error code!
                printf("sendMsg Error! %d\n", -p);
                return p;
            }
            return 0;
        }
    }

    if (strcmp(cmd.cmdName, "RETR") == 0) {
        if (cc->dataSer == NULL && cc->dataCli == NULL){
            msg = "425 Please choose mode(PORT/PASV)\r\n\0";
            p = sendMsg(connfd, msg, strlen(msg));
        } else
        {
            int fd;
            if (cc->dataSer != NULL){
                fd = getfisrtConn(cc->dataSer);
            } else
            {
                fd = cc->dataCli->sockfd;
            }

            if (cmd.num_params == 1){
                strcpy(tmp, cmd.params[0]);
                toabsPath(tmp, cc->curdir);
                printf("abspath %s:\n", tmp);

                if (!exist(tmp)){
                    printf("File Not found!\n");
                    msg = "451 File Not found!\r\n\0";
                } else
                {
                    p = send_file(tmp, fd);

                    dropOtherConn_CONN(cc);
                    if (p == 0){
                        msg = "226 retr file successfully.\r\n\0";
                        p = sendMsg(connfd, msg, strlen(msg));
                    } else
                    if (p == -ERRORREADFROMDISC){
                        printf("Error when read from disk!\n");
                        msg = "551 Error when read from disk!\r\n\0";
                        p = sendMsg(connfd, msg, strlen(msg));
                    } else
                    if (p == -ERRORDISCONN){
                        printf("File Not found!\n");
                        msg = "426 Disconnect!\r\n\0";
                        p = sendMsg(connfd, msg, strlen(msg));
                    }
                }
            } else
            {
                printf("RETR parmas Error!\n");
                msg = "451 RETR parmas Error!\r\n\0";
                p = sendMsg(connfd, msg, strlen(msg));
            }
            dropOtherConn_CONN(cc);
        }
    } else

    if (strcmp(cmd.cmdName, "STOR") == 0) {
        if (cc->dataSer == NULL && cc->dataCli == NULL){
            msg = "425 Please choose mode(PORT/PASV)\r\n\0";
            p = sendMsg(connfd, msg, strlen(msg));
        } else
        {
            int fd;
            if (cc->dataSer != NULL){
                fd = getfisrtConn(cc->dataSer);
            } else
            {
                fd = cc->dataCli->sockfd;
            }

            if (cmd.num_params == 1){
                strcpy(tmp, cmd.params[0]);
                // getfilename(tmp);
                toabsPath(tmp, cc->curdir);
                printf("abspath %s:\n", tmp);

                p = recv_file(tmp, fd);

                dropOtherConn_CONN(cc);
                if (p == 0){
                    msg = "226 stor file successfully.\r\n\0";
                    p = sendMsg(connfd, msg, strlen(msg));
                } else
                if (p == -ERRORREADFROMDISC){
                    printf("Error when read from disk!\n");
                    msg = "552 Error when read from disk!\r\n\0";
                    p = sendMsg(connfd, msg, strlen(msg));
                } else
                if (p == -ERRORDISCONN){
                    printf("File Not found!\n");
                    msg = "426 Disconnect!\r\n\0";
                    p = sendMsg(connfd, msg, strlen(msg));
                }

            } else
            {
                printf("STOR parmas Error!\n");
                msg = "452 STOR parmas Error!\r\n\0";
                p = sendMsg(connfd, msg, strlen(msg));
            }
            dropOtherConn_CONN(cc);
        }
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
        strcpy(msg, "215 UNIX Type: L8\r\n\0");
        p = sendMsg(connfd, msg, strlen(msg));
    } else

    if (strcmp(cmd.cmdName, "TYPE") == 0) {
        // printf("IN TYPE\n");
        if (cmd.num_params == 1 &&
            (strcmp(cmd.params[0], "I") == 0 || strcmp(cmd.params[0], "i"))){
            strcpy(msg, "200 Type set to I.\r\n\0");
            p = sendMsg(connfd, msg, strlen(msg));
        } else
        {
            strcpy(msg, "500 No this type!\r\n\0");
            p = sendMsg(connfd, msg, strlen(msg));
        }
    } else

    if (strcmp(cmd.cmdName, "PORT") == 0) {
        // printf("in PORT\n");
        dropOtherConn_CONN(cc);
        if (cmd.num_params == 1){
            p = conSer_Server(cmd.params[0], cc);
            switch (-p){
                case 0:
                    msg = "200 PORT command successful.\r\n\0";
                    p = sendMsg(connfd, msg, strlen(msg));
                    break;
                case ERRORIPPORT:
                    printf("Error IP/PORT in PORT!\n");
                    msg = "425 error ip/port in PORT\r\n\0";
                    p = sendMsg(connfd, msg, strlen(msg));
                    break;
                default:
                    printf("Connect Error in PORT!\n");
                    msg = "425 connect error in PORT\r\n\0";
                    p = sendMsg(connfd, msg, strlen(msg));
            }
        } else
        {
            msg = "425 error params in PORT!\r\n\0";
            p = sendMsg(connfd, msg, strlen(msg));
        }
    } else

    if (strcmp(cmd.cmdName, "PASV") == 0) {
        dropOtherConn_CONN(cc);
        if (cmd.num_params == 0){
            p = crtSer_Server(tmp, cc);
            if (p == 0){
                sprintf(msg, "227 Entering Passive Mode (%s)\r\n", tmp);
                p = sendMsg(connfd, msg, strlen(msg));
                if (p == 0){
                    p = waitConn(cc->dataSer);
                    printf("after waitConn: %d\n", p);
                }
            } else
            {
                msg = "425 error ip/port in PORT\r\n\0";
                p = sendMsg(connfd, msg, strlen(msg));
            }
        } else
        {
            msg = "425 error params in PASV!\r\n\0";
            p = sendMsg(connfd, msg, strlen(msg));
        }
    } else

    if (strcmp(cmd.cmdName, "MKD") == 0) {
        if (cmd.num_params == 1) {
            strcpy(tmp, cmd.params[0]);
            toabsPath(tmp, cc->curdir);
            if (mkdir(tmp, S_IRWXU | S_IRWXG | S_IRWXO) == 0){
                sprintf(msg, "250 MKD success.\r\n");
                p = sendMsg(connfd, msg, strlen(msg));
            } else
            {
                msg = "550 MKD Failed!\r\n\0";
                p = sendMsg(connfd, msg, strlen(msg));
            }
        } else
        {
            msg = "550 error please check pramas of MKD!\r\n\0";
            p = sendMsg(connfd, msg, strlen(msg));
        }
    } else

    if (strcmp(cmd.cmdName, "CWD") == 0) {
        if (cmd.num_params == 1){
            strcpy(tmp, cmd.params[0]);
            toabsPath(tmp, cc->curdir);
            if (is_directory(tmp)){
                strcpy(cc->curdir, tmp);
                // if (chdir(cmd.params[0]) == 0){
                msg = "250 CWD success!\r\n\0";
                p = sendMsg(connfd, msg, strlen(msg));
            } else
            {
                sprintf(msg, "550 %s: No such file or directory.\r\n", tmp);
                p = sendMsg(connfd, msg, strlen(msg));
            }
        } else
        {
            strcpy(tmp, msg);
            sprintf(msg, "550 %s: No such file or directory.\r\n", tmp);
            // msg = "550 CWD Failed!\r\n\0";
            p = sendMsg(connfd, msg, strlen(msg));
        }
    } else

    if (strcmp(cmd.cmdName, "PWD") == 0) {
        strcpy(msg, cc->curdir);
        // if (getcwd(msg, maxlen) == NULL){
        //     printf("Error getcwd");
        //     msg = "550 PWD failed\r\n\0";
        //     p = sendMsg(connfd, msg, strlen(msg));
        // } else
        {
            format(msg, tmp);
            sprintf(msg, "257 \"%s\" PWD OK.\r\n", tmp);
            p = sendMsg(connfd, msg, strlen(msg));
        }
    } else

    if (strcmp(cmd.cmdName, "LIST") == 0) {
        if (cc->dataSer == NULL && cc->dataCli == NULL){
            msg = "425 Please choose mode(PORT/PASV)\r\n\0";
            p = sendMsg(connfd, msg, strlen(msg));
        } else
        {
            int fd;
            if (cc->dataSer != NULL){
                fd = getfisrtConn(cc->dataSer);
            } else
            {
                fd = cc->dataCli->sockfd;
            }

            if (cmd.num_params == 0){
                p = send_list(cc->curdir, fd);
                if (p == 0){
                    msg = "226 send successfully.\r\n\0";
                    p = sendMsg(connfd, msg, strlen(msg));
                }
            } else
            if (cmd.num_params == 1){
                strcpy(tmp, cmd.params[0]);
                toabsPath(tmp, cc->curdir);
                printf("abspath %s:\n", tmp);

                if (!exist(tmp)){
                    printf("Path Not found!\n");
                    msg = "451 Path Not found!\r\n\0";
                } else
                {
                    p = send_list(tmp, fd);
                    // dropOtherConn_CONN(cc);
                    if (p == 0){
                        msg = "226 send list successfully.\r\n\0";
                        p = sendMsg(connfd, msg, strlen(msg));
                    } else
                    if (p == -ERRORREADFROMDISC){
                        printf("Error when read from disk!\n");
                        msg = "451 Error when read from disk!\r\n\0";
                        p = sendMsg(connfd, msg, strlen(msg));
                    } else
                    if (p == -ERRORDISCONN){
                        printf("Path Not found!\n");
                        msg = "426 Disconnect!\r\n\0";
                        p = sendMsg(connfd, msg, strlen(msg));
                    }
                }
            } else
            {
                printf("LIST parmas Error!\n");
                msg = "451 LIST parmas Error!\r\n\0";
                p = sendMsg(connfd, msg, strlen(msg));
            }
            dropOtherConn_CONN(cc);
        }

    } else

    if (strcmp(cmd.cmdName, "RMD") == 0) {
        if (cmd.num_params == 1){
            strcpy(tmp, cmd.params[0]);
            toabsPath(tmp, cc->curdir);
            if (rmdir(tmp) == 0){
                msg = "250 RMD success!\r\n\0";
                p = sendMsg(connfd, msg, strlen(msg));
            } else
            {
                msg = "550 RMD Failed!\r\n\0";
                p = sendMsg(connfd, msg, strlen(msg));
            }
        } else
        {
            msg = "550 RMD Failed!\r\n\0";
            p = sendMsg(connfd, msg, strlen(msg));
        }
    } else

    if (strcmp(cmd.cmdName, "RNFR") == 0) {
        // struct stat path_stat;
        if (cmd.num_params == 1){
            strcpy(tmp, cmd.params[0]);
            toabsPath(tmp, cc->curdir);
            if (exist(tmp)){
                msg = "350 Please send target name!\r\n\0";
                p = sendMsg(connfd, msg, strlen(msg));
                strcpy(cc->oldpath, cmd.params[0]);
            } else
            {
                msg = "550 RNFR Failed!\r\n\0";
                p = sendMsg(connfd, msg, strlen(msg));
            }
        } else
        {
            msg = "550 RNFR Failed!\r\n\0";
            p = sendMsg(connfd, msg, strlen(msg));
        }
    } else

    if (strcmp(cmd.cmdName, "RNTO") == 0) {
        if (cc->oldpath[0] == '\0'){
            msg = "503 Please source filename!\r\n\0";
            p = sendMsg(connfd, msg, strlen(msg));
        } else
        if (cmd.num_params == 1){
            strcpy(tmp, cmd.params[0]);
            toabsPath(tmp, cc->curdir);
            if (rename(cc->oldpath, tmp) == 0){
                msg = "250 Rename successfully!\r\n\0";
                p = sendMsg(connfd, msg, strlen(msg));
                cc->oldpath[0] = '\0';
            }
        } else
        {
            msg = "550 RNTO Failed!\r\n\0";
            p = sendMsg(connfd, msg, strlen(msg));
        }
    } else

    {
        printf("Undefined cmd!\n");
        strcpy(msg, nocmdStr);
        p = sendMsg(connfd, msg, strlen(msg));
    }

    if (p < 0) {    // error code!
        printf("sendMsg Error! %d\n", -p);
        return p;
    }
    return 0;
}
