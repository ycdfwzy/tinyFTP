#ifndef tinyFTP_fileserver_H
#define tinyFTP_fileserver_H
#include "socketutils.h"

int indexofDDDS(const char* st);
int startWithDDDS(const char* st);
int startWith(const char* s, const char* t);
int is_regular_file(const char *path);
int is_directory(const char* path);
int exist(const char* path);
int getFilesize(const char* path);
void getfilename(char* path);
int crtSer_Server(char* IpPort, struct connClient* cc);
int crtSer_Client(char* IpPort, struct ClientUtils* cu);
int waitConn(struct ServerUtils* su);
void* waitConn_thread(void* param);
int conSer_Server(char* IpPort, struct connClient* cc);
int conSer_Client(char* IpPort, struct ClientUtils* cu);
int send_file(char* filename, int fd);
int recv_file(char* filename, int fd);
int send_list(char* param, int fd);
int recv_list(int fd);

#endif
