#ifndef CONFIG_H 
#define CONFIG_H

#ifndef HAVE_INTTYPES_H
#define HAVE_INTTYPES_H     1
#include <inttypes.h>
#endif

//版本配置，后期从git分支信息获取
//extern const char *VERSION;
#define VERSION "1.0"

//连接信息默认数据
#define THREADS_DEFAULT         2
#define CONNECTIONS_DEFAULT     10
#define SOCKET_TIMEOUT_DEFAULT  100

#define SOCKET_SHAKE_KEY_LEN    16 

typedef unsigned char       uint8_t;
typedef unsigned short int  uint16_t;
typedef unsigned int        uint32_t;
#if __WORDSIZE == 64
typedef unsigned long int   uint64_t;
#elif __WORDSIZE == 32
typedef unsigned long long int  uint64_t;
#else
#endif

typedef int bool;
#define true 1
#define false 0

#define RECVBUF     8192
#define REQUBUF     8192

//连接配置
static struct config {
    uint64_t connections;
    uint64_t threads;
    uint64_t timeout;
    char     data[REQUBUF];
    char     *protocol;
    char     *host;
    char     *port;
} cfg;

#endif  /*CONFIG_H*/
