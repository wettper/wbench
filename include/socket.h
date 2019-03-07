#ifndef SOCKET_H
#define SOCKET_H

#ifndef HAVE_STDLIB_H
#define HAVE_STDLIB_H       1
#include <stdlib.h>
#endif

#ifndef HAVE_STRING_H
#define HAVE_STRING_H       1
#include <string.h>
#endif

#ifndef HAVE_ERRNO_H
#define HAVE_ERRNO_H        1
#include <errno.h>
#endif

#ifndef HAVE_FCNTL_H
#define HAVE_FCNTL_H        1
#include <fcntl.h>
#endif

#ifndef HAVE_NETDB_H
#define HAVE_NETDB_H        1
#include <netdb.h>
#endif

#ifndef HAVE_SYS_SOCKET_H
#define HAVE_SYS_SOCKET_H   1
#include <sys/socket.h>
#endif

#ifndef HAVE_ARPA_INET_H
#define HAVE_ARPA_INET_H    1
#include <arpa/inet.h>
#endif

#ifndef HAVE_NETINET_IN_H
#define HAVE_NETINET_IN_H   1
#include <netinet/in.h>
#endif

#define AI_FAMILY   AF_INET
#define AI_SOCKTYPE SOCK_STREAM
#define AI_PROTOCOL 0

#include "config.h"
#include "thread.h"

typedef struct socket_info {
    int fd;
    uint64_t start;
    uint64_t end;
    char buf[RECVBUF];
} socket_info;

typedef struct thread {
    pthread_t thread;
    struct sockaddr_in addr;
    char     *protocol;
    char     *host;
    char     *port;
    char     *uri;
    char     *script;
    uint64_t timeout;
    uint64_t connections;
    uint64_t complete;
    uint64_t requests;
    uint64_t bytes;
    uint64_t start;
    uint64_t end;
    struct socket_info *socket_info;
    struct data_queue *params;
    errors errors;
} thread;


//websocket根据data[0]判别数据包类型    比如0x81 = 0x80 | 0x1 为一个txt类型数据包
typedef enum {
    // 0x0：标识一个中间数据包
    WCT_MINDATA = -20,
    // 0x1：标识一个txt类型数据包
    WCT_TXTDATA = -19,
    // 0x2：标识一个bin类型数据包
    WCT_BINDATA = -18,
    // 0x8：标识一个断开连接类型数据包
    WCT_DISCONN = -17,
    // 0x8：标识一个断开连接类型数据包
    WCT_PING = -16,
    // 0xA：表示一个pong类型数据包
    WCT_PONG = -15,
    WCT_ERR = -1,
    WCT_NULL = 0
} w_com_type;

uint16_t connect_socket(thread *threads, socket_info *socketinfo);

void get_host_by_name(char **host);

#endif
