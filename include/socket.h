#ifndef SOCKET_H
#define SOCKET_H

#ifndef HAVE_STRING_H
#define HAVE_STRING_H       1
#include <string.h>
#endif

#ifndef HAVE_FCNTL_H
#define HAVE_FCNTL_H        1
#include <fcntl.h>
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

//int connect_socket(thread *threads);

#endif
