#ifndef tinyFTP_constants_H
#define tinyFTP_constants_H

#ifdef __cplusplus
extern "C"{
#endif

#define MAXCONN 64
#define MAXBUFLEN 8192
extern const char* readyStr;
extern const char* passwStr;
extern const char* greetStr;
extern const char* byeStr;
extern const char* nocmdStr;

#ifdef __cplusplus
}
#endif

#endif
