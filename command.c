#include "command.h"
#include "errorcode.h"
#include "socketutils.h"
#include "constants.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

char oldpath[8192] = "\0";
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

int is_regular_file(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat) != 0){
        return 0;
    }
    return S_ISREG(path_stat.st_mode);
}

int is_directory(const char* path) {
    struct stat path_stat;
    if (stat(path, &path_stat) != 0) {
        return 0;
    }
    return S_ISDIR(path_stat.st_mode);
}

int exist(const char* path) {
    struct stat path_stat;
    return (stat(path, &path_stat) == 0);
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
        strcmp(cmd->cmdName, "RMD") == 0 ||
        strcmp(cmd->cmdName, "RNFR") == 0 ||
        strcmp(cmd->cmdName, "RNTO") == 0){
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

int cmd_LISTD(int connfd, char* pathname, char* msg, int maxlen) {
    int p, len;
    struct dirent *dir;
    DIR *dp;
    if ((dp = opendir(pathname)) == NULL) {
        printf("opendir error for %s\n", pathname);
        msg = "500 opendir error!\r\n\0";
        p = sendMsg(connfd, msg, strlen(msg));
        if (p < 0) {
            return p;
        }
        return 0;
    }

    while ((dir = readdir(dp)) != NULL) {
        char type[20];

        switch (dir->d_type){
            case DT_BLK:
                strcpy(type, "block device");
                break;
            case DT_CHR:
                strcpy(type, "character device");
                break;
            case DT_DIR:
                strcpy(type, "directory");
                break;
            case DT_FIFO:
                strcpy(type, "named pipe (FIFO)");
                break;
            case DT_LNK:
                strcpy(type, "symbolic link");
                break;
            case DT_REG:
                strcpy(type, "regular file");
                break;
            case DT_SOCK:
                strcpy(type, "UNIX domain socket");
                break;
            default:
                strcpy(type, "Unknown");
        }
        sprintf(msg, "200-name: %s; type: %s\r\n", dir->d_name, type);
        p = sendMsg(connfd, msg, strlen(msg));
        if (p < 0) {
            closedir(dp);
            return p;
        }
    }
    closedir(dp);
    sprintf(msg, "200 OK!\r\n");
    p = sendMsg(connfd, msg, strlen(msg));
    if (p < 0){
        return p;
    }
    return 0;
}

int cmd_LISTF(int connfd, char* curpath, char* msg, int maxlen) {
    int p;
    struct stat path_stat;
    if (stat(curpath, &path_stat) != 0){ //nearly impossible
        printf("File not found!\n");
        msg = "500 File not found!\r\n\0";
        p = sendMsg(connfd, msg, strlen(msg));
        return p;
    }
    if (S_ISDIR(path_stat.st_mode)){ //nearly impossible
        printf("Not file!\n");
        msg = "500 Not file!\r\n\0";
        p = sendMsg(connfd, msg, strlen(msg));
        return p;
    }

    int t = strlen(curpath);
    while (t > 0 && curpath[t-1] != '/')
        t--;
    // printf("%x\n", &(path_stat.st_atime));
    // printf("%x\n", &(path_stat.st_mtime));
    sprintf(msg, "200 name: %s; size: %lu bytes; last access: %s; last modification: %s\r\n",
                    curpath+t, path_stat.st_size,
                    asctime(localtime(&(path_stat.st_atime))),
                    asctime(localtime(&(path_stat.st_mtime))) );
    p = sendMsg(connfd, msg, strlen(msg));
    return p;
}

int CmdHandle(struct Command cmd, int connfd, char* msg, int maxlen) {
    int p;
    printf("%s\n", cmd.cmdName);

    if (oldpath[0] != '\0'){ // RNFR to handle
        if (strcmp(cmd.cmdName, "RNTO") != 0){
            msg = "500 Please send target name!\r\n\0";
            p = sendMsg(connfd, msg, strlen(msg));
            if (p < 0) {    // error code!
                printf("sendMsg Error! %d\n", -p);
                return p;
            }
            return 0;
        }
    }

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
        if (cmd.num_params == 0){
            if (getcwd(curpath, 8192) == NULL){
                printf("Error getcwd in LIST");
                msg = "500 Error getcwd!\r\n\0";
                p = sendMsg(connfd, msg, strlen(msg));
            } else
            {
                p = cmd_LISTD(connfd, curpath, msg, maxlen);
            }
        } else
        if (cmd.num_params == 1){
            if (!exist(cmd.params[0])){
                printf("Path Not found!\n");
                msg = "500 Path Not found!\r\n\0";
                p = sendMsg(connfd, msg, strlen(msg));
            } else
            if (is_directory(cmd.params[0])){
                p = cmd_LISTD(connfd, cmd.params[0], msg, maxlen);
            } else
            {
                p = cmd_LISTF(connfd, cmd.params[0], msg, maxlen);
            }
        } else
        {
            printf("LIST parmas Error!\n");
            msg = "500 LIST parmas Error!\r\n\0";
            p = sendMsg(connfd, msg, strlen(msg));
        }
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
        // struct stat path_stat;
        if (cmd.num_params == 1 && exist(cmd.params[0])){
            msg = "200 Please send target name!\r\n\0";
            p = sendMsg(connfd, msg, strlen(msg));
            strcpy(oldpath, cmd.params[0]);
        } else
        {
            msg = "500 RNFR Failed!\r\n\0";
            p = sendMsg(connfd, msg, strlen(msg));
        }
    } else

    if (strcmp(cmd.cmdName, "RNTO") == 0) {
        if (cmd.num_params == 1 && rename(oldpath, cmd.params[0]) == 0){
            msg = "200 Rename successfully!\r\n\0";
            p = sendMsg(connfd, msg, strlen(msg));
            oldpath[0] = '\0';
        } else
        {
            msg = "500 RNTO Failed!\r\n\0";
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
