#ifdef __cplusplus
extern "C"{
#endif
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
#include <pthread.h>

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

// struct CmdHandleInfo{
//     struct Command cmd;
//     struct connClient* cc;
//     char* msg;
// };

// void* RETR_thread(void* arg){
//     struct CmdHandleInfo* argp = (struct CmdHandleInfo*) arg;
//     struct Command cmd = argp->cmd;
//     struct connClient* cc = argp->cc;
//     char* msg = argp->msg;
//     int connfd = cc->connfd;
//     char tmp[MAXBUFLEN];
//     int p;



//     return NULL;
// }

// void* STOR_thread(void* arg){
//     struct CmdHandleInfo* argp = (struct CmdHandleInfo*) arg;
//     struct Command cmd = argp->cmd;
//     struct connClient* cc = argp->cc;
//     char* msg = argp->msg;
//     int connfd = cc->connfd;
//     char tmp[MAXBUFLEN];
//     int p;



//     return NULL;
// }

int CmdHandle(struct Command cmd, struct connClient* cc, char* msg) {
    int p;
    int connfd = cc->connfd;
    char tmp[MAXBUFLEN];
    printf("%s\n", cmd.cmdName);

    if (cc->oldpath[0] != '\0'){ // RNFR to handle
        if (strcmp(cmd.cmdName, "RNTO") != 0){
            msg = "503 Please send target name using RNTO!\r\n\0";
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
            if (cmd.num_params == 1){
                strcpy(tmp, cmd.params[0]);
                toabsPath(tmp, cc->curdir);
                printf("RETR_thread: abspath %s\n", tmp);

                char rootpath[512];
                getcwd(rootpath, 512);
                if (!startWith(tmp, rootpath)){ // permission denied!
                    sprintf(msg, "550 %s: You have no Permission!\r\n", tmp);
                    p = sendMsg(connfd, msg, strlen(msg));
                } else

                if (!exist(tmp) || is_directory(tmp)){
                    printf("RETR_thread: File Not found!\n");
                    msg = "451 File Not found!\r\n\0";
                } else
                {
                    int sz = getFilesize(tmp);
                    sprintf(msg, "150 Start transfer(%d bytes)\r\n", sz);
                    p = sendMsg(connfd, msg, strlen(msg));

                    int fd = -1;
                    if (cc->dataSer != NULL){
                        fd = getfisrtConn(cc->dataSer);
                    } else
                    {
                        fd = cc->dataCli->sockfd;
                    }

                    if (fd == -1) {
                        sprintf(msg, "425 Connection is not established!\r\n");
                        p = sendMsg(connfd, msg, strlen(msg));
                    } else
                    {
                        p = send_file(tmp, fd);
                        dropOtherConn_CONN(cc);
                        
                        if (p == 0){
                            msg = "226 retr file successfully.\r\n\0";
                            p = sendMsg(connfd, msg, strlen(msg));
                        } else
                        if (p == -ERRORREADFROMDISC){
                            printf("RETR_thread: Error when read from disk!\n");
                            msg = "551 Error when read from disk!\r\n\0";
                            p = sendMsg(connfd, msg, strlen(msg));
                        } else
                        if (p == -ERRORDISCONN){
                            printf("RETR_thread: data connection Disconnect!\n");
                            msg = "426 Disconnect!\r\n\0";
                            p = sendMsg(connfd, msg, strlen(msg));
                        }
                    }
                }
            } else
            {
                printf("RETR_thread: RETR parmas Error!\n");
                msg = "451 RETR parmas Error!\r\n\0";
                p = sendMsg(connfd, msg, strlen(msg));
            }
            dropOtherConn_CONN(cc);
        }
    } else

    if (strcmp(cmd.cmdName, "STOR") == 0) {
        if (cc->dataSer == NULL && cc->dataCli == NULL){
            msg = "450 Please choose mode(PORT/PASV)\r\n\0";
            p = sendMsg(connfd, msg, strlen(msg));
        } else
        {

            if (cmd.num_params == 1){
                strcpy(tmp, cmd.params[0]);
                getfilename(tmp);
                toabsPath(tmp, cc->curdir);
                printf("STOR_thread: abspath %s:\n", tmp);

                char rootpath[512];
                getcwd(rootpath, 512);
                if (!startWith(tmp, rootpath)){ // permission denied!
                    sprintf(msg, "550 %s: You have no Permission!\r\n", tmp);
                    p = sendMsg(connfd, msg, strlen(msg));
                } else
                {
                    sprintf(msg, "150 Start recv\r\n");
                    p = sendMsg(connfd, msg, strlen(msg));

                    int fd = -1;
                    if (cc->dataSer != NULL){
                        fd = getfisrtConn(cc->dataSer);
                    } else
                    {
                        fd = cc->dataCli->sockfd;
                    }

                    if (fd == -1) {
                        sprintf(msg, "425 Connection is not established!\r\n");
                        p = sendMsg(connfd, msg, strlen(msg));
                    } else
                    {
                        p = recv_file(tmp, fd);
                        dropOtherConn_CONN(cc);

                        if (p == 0){
                            msg = "226 stor file successfully.\r\n\0";
                            p = sendMsg(connfd, msg, strlen(msg));
                        } else
                        if (p == -ERRORREADFROMDISC){
                            printf("Error when write to disk!\n");
                            msg = "552 Error when write to disk!\r\n\0";
                            p = sendMsg(connfd, msg, strlen(msg));
                        } else
                        if (p == -ERRORDISCONN){
                            printf("data connection Disconnect!\n");
                            msg = "426 Disconnect!\r\n\0";
                            p = sendMsg(connfd, msg, strlen(msg));
                        }
                    }
                }

            } else
            {
                printf("STOR_thread: STOR parmas Error!\n");
                msg = "450 STOR parmas Error!\r\n\0";
                p = sendMsg(connfd, msg, strlen(msg));
            }
            dropOtherConn_CONN(cc);
        }
    } else

    if (strcmp(cmd.cmdName, "QUIT") == 0 ||
        strcmp(cmd.cmdName, "ABOR") == 0) {
        // printf("In QUIT\n");
        strcpy(msg, byeStr);
        p = sendMsg(connfd, msg, strlen(msg));
        if (p < 0) { // error code!
            printf("sendMsg Error! %d\n", -p);
            return p;
        }
        dropOtherConn_CONN(cc);
        return -ERRORQUIT;
    } else

    if (strcmp(cmd.cmdName, "SYST") == 0) {
        strcpy(msg, "215 UNIX Type: L8\r\n\0");
        p = sendMsg(connfd, msg, strlen(msg));
    } else

    if (strcmp(cmd.cmdName, "TYPE") == 0) {
        // printf("IN TYPE\n");
        if (cmd.num_params == 1 &&
            (strcmp(cmd.params[0], "I") == 0 || strcmp(cmd.params[0], "i") == 0)){
            strcpy(msg, "200 Type set to I.\r\n\0");
            p = sendMsg(connfd, msg, strlen(msg));
        } else
        {
            strcpy(msg, "504 No this type!\r\n\0");
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
                    msg = "425 error ip/port of PORT\r\n\0";
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
                    pthread_t pid;
                    pthread_create(&pid, NULL, waitConn_thread, (void*)(cc->dataSer));
                    pthread_detach(pid);
                    // p = waitConn(cc->dataSer);
                    // printf("after waitConn: %d\n", p);
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

            char rootpath[512];
            getcwd(rootpath, 512);
            if (!startWith(tmp, rootpath)){ // permission denied!
                sprintf(msg, "550 %s: You have no Permission!\r\n", tmp);
                p = sendMsg(connfd, msg, strlen(msg));
            } else

            if (mkdir(tmp, S_IRWXU | S_IRWXG | S_IROTH) == 0){
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
                char rootpath[512];
                getcwd(rootpath, 512);
                if (!startWith(tmp, rootpath)){ // permission denied!
                    sprintf(msg, "550 %s: You have no Permission!!\r\n", tmp);
                    p = sendMsg(connfd, msg, strlen(msg));
                } else
                {
                    strcpy(cc->curdir, tmp);
                    // if (chdir(cmd.params[0]) == 0){
                    msg = "250 CWD success!\r\n\0";
                    p = sendMsg(connfd, msg, strlen(msg));
                }
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
            if (cmd.num_params == 0){
                sprintf(msg, "150 Start transfer\r\n");
                p = sendMsg(connfd, msg, strlen(msg));

                int fd = -1;
                if (cc->dataSer != NULL){
                    fd = getfisrtConn(cc->dataSer);
                } else
                {
                    fd = cc->dataCli->sockfd;
                }

                if (fd == -1) {
                    sprintf(msg, "425 Connection is not established!\r\n");
                    p = sendMsg(connfd, msg, strlen(msg));
                } else
                {
                    p = send_list(cc->curdir, fd);
                    if (p == 0){
                        msg = "226 send list successfully.\r\n\0";
                        p = sendMsg(connfd, msg, strlen(msg));
                    }
                }
            } else
            if (cmd.num_params == 1){
                strcpy(tmp, cmd.params[0]);
                toabsPath(tmp, cc->curdir);
                printf("abspath %s:\n", tmp);

                char rootpath[512];
                getcwd(rootpath, 512);
                if (!startWith(tmp, rootpath)){ // permission denied!
                    sprintf(msg, "550 %s: You have no Permission!\r\n", tmp);
                    p = sendMsg(connfd, msg, strlen(msg));
                } else

                if (!exist(tmp)){
                    printf("Path Not found!\n");
                    msg = "451 Path Not found!\r\n\0";
                    p = sendMsg(connfd, msg, strlen(msg));
                } else
                {
                    sprintf(msg, "150 Start transfer\r\n");
                    p = sendMsg(connfd, msg, strlen(msg));
                    sleep(0.5);
                    int fd = -1;
                    if (cc->dataSer != NULL){
                        fd = getfisrtConn(cc->dataSer);
                    } else
                    {
                        fd = cc->dataCli->sockfd;
                    }

                    if (fd == -1) {
                        sprintf(msg, "425 Connection is not established!\r\n");
                        p = sendMsg(connfd, msg, strlen(msg));
                    } else
                    {
                        p = send_list(tmp, fd);
                        sleep(1);
                        dropOtherConn_CONN(cc);
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

            char rootpath[512];
            getcwd(rootpath, 512);
            if (!startWith(tmp, rootpath)){ // permission denied!
                sprintf(msg, "550 %s: You have no Permission!\r\n", tmp);
                p = sendMsg(connfd, msg, strlen(msg));
            } else
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

            char rootpath[512];
            getcwd(rootpath, 512);
            if (!startWith(tmp, rootpath)){ // permission denied!
                sprintf(msg, "550 %s: You have no Permission!\r\n", tmp);
                p = sendMsg(connfd, msg, strlen(msg));
            } else
            if (exist(tmp)){
                msg = "350 Please send target name!\r\n\0";
                p = sendMsg(connfd, msg, strlen(msg));
                if (p == 0)
                    strcpy(cc->oldpath, tmp);
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
        printf("in RNTO\n");
        if (cc->oldpath[0] == '\0'){
            msg = "503 Please source filename!\r\n\0";
            p = sendMsg(connfd, msg, strlen(msg));
        } else
        if (cmd.num_params == 1){
            printf("in RNTO\n");
            strcpy(tmp, cmd.params[0]);
            toabsPath(tmp, cc->curdir);
            printf("tmp=%s\n", tmp);

            char rootpath[512];
            getcwd(rootpath, 512);
            printf("rootpath=%s\n", rootpath);            
            if (!startWith(tmp, rootpath)){ // permission denied!
                sprintf(msg, "550 %s: You have no Permission!\r\n", tmp);
                p = sendMsg(connfd, msg, strlen(msg));
                cc->oldpath[0] = '\0';
            } else
            if (rename(cc->oldpath, tmp) == 0){
                msg = "250 Rename successfully!\r\n\0";
                p = sendMsg(connfd, msg, strlen(msg));
                cc->oldpath[0] = '\0';
            } else
            {
                msg = "550 RNTO Failed!\r\n\0";
                p = sendMsg(connfd, msg, strlen(msg));
                cc->oldpath[0] = '\0';
            }
        } else
        {
            msg = "550 RNTO Failed!\r\n\0";
            p = sendMsg(connfd, msg, strlen(msg));
            cc->oldpath[0] = '\0';
        }
        cc->oldpath[0] = '\0';
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
#ifdef __cplusplus
}
#endif
