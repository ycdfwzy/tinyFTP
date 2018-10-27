#ifndef tinyFTP_errorcode_H
#define tinyFTP_errorcode_H
enum ERRORC{
    ERRORSOCKET=1,
    ERRORLISTEN,
    ERRORBIND,
    ERRORCONN,
    ERRORREAD,
    ERRORACCEPT,
    ERRORWRITE,
    ERRORNOCMD,
    ERRORINVALIDRES,
    ERRORQUIT,
    ERRORIPPORT,
};

#endif