#ifndef THREAD_H
#define THREAD_H

#ifndef HAVE_PTHREAD_H
#define HAVE_PTHREAD_H      1
#include <pthread.h>
#endif

#ifndef HAVE_STDLIB_H
#define HAVE_STDLIB_H       1
#include <stdlib.h>
#endif

#include "config.h"
#include "stats.h"
#include "socket.h"

typedef struct thread {
    pthread_t thread;
    struct sockaddr_in addr;
    char     *host;
    char     *port;
    char     params[REQUBUF];
    uint64_t timeout;
    uint64_t connections;
    uint64_t complete;
    uint64_t requests;
    uint64_t bytes;
    uint64_t start;
    uint64_t end;
    struct socket_info *socket_info;
    errors errors;
} thread;

void *thread_main(void *arg);

#endif
