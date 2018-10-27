#include "dataserver.h"
#include "errorcode.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <errno.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int extract(char* IpPort, char* ipaddr, int* port){
	int h[7];
	int cnt = 0;
	while (*IpPort){
		// printf("%c\n", *IpPort);
		if ((*IpPort) < '0' || (*IpPort) > '9') {
			IpPort++;
			continue;
		}
		h[cnt] = 0;
		while ((*IpPort) >= '0' && (*IpPort) <= '9') {
			h[cnt] = h[cnt]*10+((*IpPort)-'0');
			IpPort++;
		}
		cnt++;
		if (cnt > 6)
			break;
	}
	if (cnt != 6){
		return -ERRORIPPORT;
	}
	sprintf(ipaddr, "%d.%d.%d.%d", h[0], h[1], h[2], h[3]);
	*port = h[4]*256+h[5];
	return 0;
}

int conSer(char* IpPort){
	// printf("In port func\n");
	int sockfd;
	struct sockaddr_in addr;
	int p;
	int port;
	char ipaddr[20];

	p = extract(IpPort, ipaddr, &port);
	if (p < 0) {
		printf("Error extract!\n");
		return p;
	}
	printf("%s:%d\n", ipaddr, port);

	if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("Error socket(): %s(%d)\n", strerror(errno), errno);
        return -ERRORSOCKET;
    }

	memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ipaddr, &addr.sin_addr) <= 0) {
        printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
        return -ERRORBIND;
    }

    int cnt = 0;
    while (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        printf("%d Error connect(): %s(%d)\n", ++cnt, strerror(errno), errno);
        if (cnt > 100){
        	return -ERRORCONN;
        }
    }
    
	return 0;
}