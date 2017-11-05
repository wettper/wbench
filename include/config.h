#ifndef CONFIG_H 
#define CONFIG_H

//版本配置，后期从git分支信息获取
//extern const char *VERSION;
#define VERSION "1.0"

//连接信息默认数据
#define THREADS_DEFAULT         2
#define CONNECTIONS_DEFAULT     10
#define DURATION_DEFAULT        10
#define SOCKET_TIMEOUT_DEFAULT  100

typedef unsigned char       uint8_t;
typedef unsigned short int  uint16_t;
#if __WORDSIZE == 64
typedef unsigned long int   uint64_t;
#else
typedef unsigned long long int  uint64_t;
#endif

typedef int bool;
#define true 1
#define false 0

//连接配置
static struct config {
    uint64_t connections;
    uint64_t duration;
    uint64_t threads;
    uint64_t timeout;
    uint64_t pipeline;
    bool     delay;
    bool     dynamic; 
    bool     latency; 
    char     *host;
    char     *script;
} cfg;

#define RECVBUF     8192

#endif  /*CONFIG_H*/
