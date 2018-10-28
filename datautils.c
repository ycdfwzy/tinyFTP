#include "datautils.h"
#include "errorcode.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <errno.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

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

int endWith(const char* s, const char* t){
	int ls = strlen(s);
	int lt = strlen(t);
	if (ls < lt){
		return 0;
	}
	for (int i = 1; i <= lt; ++i)
	if (s[ls-i] != t[lt-i]){
		return 0;
	}
	return 1;
}

int extract(char* IpPort, char* ipaddr, int* port){
	int h[7];
	// int cnt = 0;
	// while (*IpPort){
	// 	// printf("%c\n", *IpPort);
	// 	if ((*IpPort) < '0' || (*IpPort) > '9') {
	// 		IpPort++;
	// 		continue;
	// 	}
	// 	h[cnt] = 0;
	// 	while ((*IpPort) >= '0' && (*IpPort) <= '9') {
	// 		h[cnt] = h[cnt]*10+((*IpPort)-'0');
	// 		IpPort++;
	// 	}
	// 	cnt++;
	// 	if (cnt > 6)
	// 		break;
	// }
	// if (cnt != 6){
	// 	return -ERRORIPPORT;
	// }
	while ((*IpPort) && ((*IpPort)<'0' || (*IpPort)>'9'))
		IpPort++;
	sscanf(IpPort, "%d,%d,%d,%d,%d,%d",
		&h[0], &h[1], &h[2], &h[3], &h[4], &h[5]);
	sprintf(ipaddr, "%d.%d.%d.%d", h[0], h[1], h[2], h[3]);
	*port = h[4]*256+h[5];
	return 0;
}

int conSer(char* IpPort, struct ClientUtils* datacli){
	int p;
	int port;
	char ipaddr[20];

	p = extract(IpPort, ipaddr, &port);
	if (p < 0) {
		printf("Error extract!\n");
		return p;
	}
	printf("%s:%d\n", ipaddr, port);

	if ((datacli->sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("Error socket(): %s(%d)\n", strerror(errno), errno);
        return -ERRORSOCKET;
    }
    printf("sockfd: %d\n", datacli->sockfd);

	memset(&(datacli->addr), 0, sizeof(datacli->addr));
    datacli->addr.sin_family = AF_INET;
    datacli->addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ipaddr, &(datacli->addr.sin_addr)) <= 0) {
        printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
        return -ERRORBIND;
    }

    int cnt = 0;
    while (connect(datacli->sockfd, (struct sockaddr*)&(datacli->addr), sizeof(datacli->addr)) < 0) {
        printf("%d Error connect(): %s(%d)\n", ++cnt, strerror(errno), errno);
        if (cnt > 100){
        	return -ERRORCONN;
        }
    }
    
	return 0;
}

int conSer_Server(char* IpPort, struct connClient* cc){
	cc->dataCli = malloc(sizeof(struct ClientUtils));
	struct ClientUtils* datacli = cc->dataCli;
	initClientUtils(datacli);

	return conSer(IpPort, datacli);
}

int conSer_Client(char* IpPort, struct ClientUtils* cu){
	cu->dataCli = malloc(sizeof(struct ClientUtils));
	struct ClientUtils* datacli = cu->dataCli;
	initClientUtils(datacli);

	return conSer(IpPort, datacli);
}

int crtSer(char* IpPort, struct ServerUtils* dataser){
	// int p;
	int port = rand()&65535;
	char ipaddr[20];

	while (port < 20000)
		port = rand()&65535;

	if ((dataser->listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return -ERRORSOCKET;
	}

	memset(&(dataser->addr), 0, sizeof(dataser->addr));
	dataser->addr.sin_family = AF_INET;
	dataser->addr.sin_port = htons(port);
	dataser->addr.sin_addr.s_addr = htonl(INADDR_ANY);

	while (bind(dataser->listenfd, (struct sockaddr*)&(dataser->addr), sizeof(dataser->addr)) == -1) {
		printf("Error bind(): %s(%d)\n", strerror(errno), errno);

		port = rand()&65535;
		while (port < 20000)
			port = rand()&65535;

		dataser->addr.sin_port = htons(port);
		continue;
		// return 1;
		// return -ERRORBIND;
	}
	strcpy(ipaddr, "127.0.0.1");
	sprintf(IpPort, "127,0,0,1,%d,%d", port/256, port%256);

	if (listen(dataser->listenfd, 1) == -1) {
		printf("Error listen(): %s(%d)\n", strerror(errno), errno);
		// return 1;
		return ERRORLISTEN;
	}

	return 0;
}

int crtSer_Server(char* IpPort, struct connClient* cc){
	cc->dataSer = malloc(sizeof(struct ServerUtils));
	struct ServerUtils* dataser = cc->dataSer;
	initServerUtils(dataser);

	return crtSer(IpPort, dataser);
}

int crtSer_Client(char* IpPort, struct ClientUtils* cu){
	cu->dataSer = malloc(sizeof(struct ServerUtils));
	struct ServerUtils* dataser = cu->dataSer;
	initServerUtils(dataser);

	return crtSer(IpPort, dataser);
}

int waitConn(struct ServerUtils* su){
	int index = -1;
    for (int i = 0; i < MAXCONN; ++i){
        if (su->conn[i].connfd == -1){
            index = i;
            break;
        }
    }
    if (index == -1)    // no idle conn nearly impossible
        return -1;
    if (index != 0){	// index = 0
    	printf("index != 0!");
    }
    while ((su->conn[index].connfd = accept(su->listenfd, NULL, NULL)) == -1) {
        printf("Error accept(): %s(%d)\n", strerror(errno), errno);
        continue;
    }
    return 0;
}

int cmd_LISTD(int connfd, char* pathname, char* msg, int maxlen) {
    int p;
    struct dirent *dir;
    DIR *dp;
    if ((dp = opendir(pathname)) == NULL) {
        printf("opendir error for %s\n", pathname);
        msg = "550 opendir error!\r\n\0";
        p = sendMsg(connfd, msg, strlen(msg));
        if (p < 0) {
            return -ERRORDISCONN;
        }
        return -ERRORREADFROMDISC;
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
        sprintf(msg, "name: %s; type: %s\n", dir->d_name, type);
        p = sendMsg(connfd, msg, strlen(msg));
        if (p < 0) {
            closedir(dp);
            return -ERRORDISCONN;
        }
    }
    closedir(dp);
    sprintf(msg, "Complete!\n");
    p = sendMsg(connfd, msg, strlen(msg));
    if (p < 0){
        return -ERRORDISCONN;
    }
    return 0;
}

int cmd_LISTF(int connfd, char* curpath, char* msg, int maxlen) {
    int p;
    struct stat path_stat;
    if (stat(curpath, &path_stat) != 0){ //nearly impossible
        printf("File not found!\n");
        msg = "550 File not found!\n\0";
        p = sendMsg(connfd, msg, strlen(msg));
        if (p < 0) {
            return -ERRORDISCONN;
        }
        return -ERRORREADFROMDISC;
    }
    if (S_ISDIR(path_stat.st_mode)){ //nearly impossible
        printf("Not file!\n");
        msg = "550 Not file!\n\0";
        p = sendMsg(connfd, msg, strlen(msg));
        if (p < 0) {
            return -ERRORDISCONN;
        }
        return -ERRORREADFROMDISC;
    }

    int t = strlen(curpath);
    while (t > 0 && curpath[t-1] != '/')
        t--;
    // printf("%x\n", &(path_stat.st_atime));
    // printf("%x\n", &(path_stat.st_mtime));
    sprintf(msg, "name: %s; size: %lu bytes; last access: %s; last modification: %s\n",
                    curpath+t, path_stat.st_size,
                    asctime(localtime(&(path_stat.st_atime))),
                    asctime(localtime(&(path_stat.st_mtime))) );
    p = sendMsg(connfd, msg, strlen(msg));
    if (p < 0){
    	return -ERRORDISCONN;
    }
    sprintf(msg, "Complete!\n");
    p = sendMsg(connfd, msg, strlen(msg));
    if (p < 0){
        return -ERRORDISCONN;
    }
    return 0;
}

int send_list(char* param, int fd) {
	int p;
	char msg[8192];
	if (is_directory(param)){
		p = cmd_LISTD(fd, param, msg, 8192);
	} else
	{
		p = cmd_LISTF(fd, param, msg, 8192);
	}
	return p;
}

int recv_list(int fd) {
	int p;
	char msg[8192];
	do{
        p = waitMsg(fd, msg, 8192);
        if (p < 0) {    // error code!
            printf("waitMsg Error! %d\n", -p);
            return p;
        }
    } while (!endWith(msg, "Complete!"));
    return 0;
}