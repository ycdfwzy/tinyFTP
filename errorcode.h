#ifndef tinyFTP_errorcode_H
#define tinyFTP_errorcode_H

#ifdef __cplusplus
extern "C"{
#endif

enum ERRORC{
    NOERROR=0,
    ERRORSOCKET=1,
    ERRORLISTEN,
    ERRORBIND,
    ERRORCONN,
    ERRORINETPTON,
    ERRORLOGIN,
    ERRORREAD,
    ERRORACCEPT,
    ERRORWRITE,
    ERRORNOCMD,
    ERRORINVALIDRES,
    ERRORQUIT,
    ERRORIPPORT,
    ERRORREADFROMDISC,
    ERRORDISCONN,
    ERROREXT,
};

#ifdef __cplusplus
}
#endif

#endif
