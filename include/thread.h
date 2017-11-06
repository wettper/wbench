#ifndef THREAD_H
#define THREAD_H

typedef struct {
    pthread_t thread;
    struct sockaddr_in addr;
    uint64_t connections;
    uint64_t complete;
    uint64_t requests;
    uint64_t bytes;
    uint64_t start;
    uint64_t end;
    errors errors;
} thread;

void *thread_main(void *arg)
{
    connect_socket(arg);
}

#endif
