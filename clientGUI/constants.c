#ifdef __cplusplus
extern "C"{
#endif
#include "constants.h"

const char* readyStr = "220 Anonymous FTP server ready.\r\n\0";
const char* passwStr = "331 Guest login ok, send your complete e-mail address as password.\r\n\0";
const char* greetStr = "230 Guest login ok, access restrictions apply.\r\n\0";
const char* byeStr = "221 Goodbye.\r\n\0";
const char* nocmdStr = "500 Undefined cmd!\r\n\0";
#ifdef __cplusplus
}
#endif
