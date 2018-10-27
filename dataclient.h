#ifndef tinyFTP_fileclient_H
#define tinyFTP_fileclient_H

int crtSer(char* IpPort);
int conSer(char* IpPort);
int send_file(char* filename);
int recv_file(char* filename);

#endif