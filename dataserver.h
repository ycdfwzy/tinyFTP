#ifndef tinyFTP_fileserver_H
#define tinyFTP_fileserver_H

int crtSer(char* IpPort);
int conSer(char* IpPort);
int send_file(char* filename);
int recv_file(char* filename);

#endif
