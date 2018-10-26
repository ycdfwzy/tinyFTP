#include "socketutils.h"
#include "errorcode.h"
#include <stdio.h>

int waitMsg(int connfd, char* msg, int MAXLEN) {
	int len, p;
	p = 0;
	while (1) {
		int n = read(connfd, msg + p, MAXLEN - p);
		if (n < 0) {
			printf("Error read(): %s(%d)\n", strerror(errno), errno);
			return -ERRORREAD;
		} else if (n == 0) {
			break;
		} else {
			p += n;
			if (msg[p - 1] == '\n') {
				break;
			}
		}
	}
    msg[p - 1] = '\0';
	len = p-1;
	printf("receive: %s\n", msg);
	return len;
}

int sendMsg(int connfd, char* msg, int len){
    int p = 0;
    printf("%s\n", msg);
    printf("before finished!\n");
    while (p < len) {
        printf("%d %d\n", p, len);
        int n = write(connfd, msg + p, len - p);
        if (n < 0) {
            printf("Error write(): %s(%d)\n", strerror(errno), errno);
            return -ERRORWRITE;
        } else {
            p += n;
        }
    }
    printf("write finished!\n");
    return 0;
}
