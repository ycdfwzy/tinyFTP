#include "socketutils.h"
#include "errorcode.h"
#include <stdio.h>
#include <stdlib.h>

void closeSer(struct ServerUtils* su){
	// for (int i = 0; i < MAXCONN; ++i)
	// if (su->conn[i].connfd != -1){
	// 	close(su->conn[i].connfd);
	// 	releconnClient(&(su->conn[i]));
	// }
	releServerUtils(su);
	close(su->listenfd);
}

void closeCli(struct ClientUtils* cu){
	releClientUtils(cu);
	close(cu->sockfd);
}

void initconnClient(struct connClient* cc){
	cc->connfd = -1;
	cc->oldpath[0] = '\0';
	cc->dataSer = NULL;
	cc->dataCli = NULL;
}

void releconnClient(struct connClient* cc){
	cc->connfd = -1;
	if (cc->dataSer != NULL){
		releServerUtils(cc->dataSer);
		free(cc->dataSer);
		cc->dataSer = NULL;
	}
	if (cc->dataCli != NULL){
		releClientUtils(cc->dataCli);
		free(cc->dataCli);
		cc->dataCli = NULL;
	}
}

void initServerUtils(struct ServerUtils* su){
	for (int i = 0; i < MAXCONN; ++i){
		initconnClient(&(su->conn[i]));
	}
}

void releServerUtils(struct ServerUtils* su){
	for (int i = 0; i < MAXCONN; ++i)
	if (su->conn[i].connfd != -1){
		close(su->conn[i].connfd);
		releconnClient(&(su->conn[i]));
	}
}

void initClientUtils(struct ClientUtils* cu){
	cu->dataSer = NULL;
	cu->dataCli = NULL;
}

void releClientUtils(struct ClientUtils* cu){
	if (cu->dataSer != NULL){
		releServerUtils(cu->dataSer);
		free(cu->dataSer);
		cu->dataSer = NULL;
	}
	if (cu->dataCli != NULL){
		releClientUtils(cu->dataCli);
		free(cu->dataCli);
		cu->dataCli = NULL;
	}
}

void dropOtherConn_Client(struct ClientUtils* cu){
    if (cu->dataSer != NULL){
        closeSer(cu->dataSer);
        releServerUtils(cu->dataSer);
        cu->dataSer = NULL;
    }
    if (cu->dataCli != NULL){
        closeCli(cu->dataCli);
        releClientUtils(cu->dataCli);
        cu->dataCli = NULL;
    }
}

void dropOtherConn_CONN(struct connClient* cc){
    if (cc->dataSer != NULL){
        closeSer(cc->dataSer);
        releServerUtils(cc->dataSer);
        cc->dataSer = NULL;
    }
    if (cc->dataCli != NULL){
        closeCli(cc->dataCli);
        releClientUtils(cc->dataCli);
        cc->dataCli = NULL;
    }
}

int getfisrtConn(struct ServerUtils* su){
	for (int i = 0; i < MAXCONN; ++i)
		if (su->conn[i].connfd != -1)
			return su->conn[i].connfd;
	return -1;
}

int waitMsg(int connfd, char* msg, int MAXLEN) {
	int len, p;
	p = 0;
	while (1) {
		int n = read(connfd, msg + p, MAXLEN - p);
		printf("n=%d\n", n);
		if (n < 0) {
			printf("Error read(): %s(%d)\n", strerror(errno), errno);
			return -ERRORREAD;
		} else if (n == 0) {
			break;
		} else {
			p += n;
			if (p >= MAXLEN ||  msg[p - 1] == '\n') {
				break;
			}
		}
	}

	if (p == 0){
		msg[0] = '\0';
		len = p;
	} else
	{
		if (msg[p - 1] == '\n'){
    		msg[p - 1] = '\0';
			len = p - 1;
		} else
		{
			msg[p] = '\0';
			len = p;
		}
	}

	printf("receive: %s\n", msg);
	return len;
}

int sendMsg(int connfd, char* msg, int len){
    int p = 0;
    // printf("before finished!\n");
    while (p < len) {
        // printf("%d %d\n", p, len);
        int n = write(connfd, msg + p, len - p);
        if (n < 0) {
            printf("Error write(): %s(%d)\n", strerror(errno), errno);
            return -ERRORWRITE;
        } else {
            p += n;
        }
    }
    printf("send finished: %s", msg);
    // printf("write finished!\n");
    return 0;
}
