#ifndef SOCKET_H
#define SOCKET_H

#define AI_FAMILY   AF_INET
#define AI_SOCKTYPE SOCK_STREAM
#define AI_PROTOCOL 0

typedef struct {
    char  *buffer;
    size_t length;
    char  *cursor;
} buffer;

typedef struct connection {
    thread *thread; 
    int fd;
    uint64_t start;
    char *request;
    size_t length;
    size_t written;
    uint64_t pending;
    buffer headers;
    buffer body;
    char buf[RECVBUF];

} connection;

int connect_socket(thread *thread)
{
    struct sockaddr_in addr = thread->addr;
    int fd;
    fd = socket(AI_FAMILY, AI_SOCKTYPE, AI_PROTOCOL);

    char *str = "hello world";
    char buf[RECVBUF] = {'\0'};

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        thread->errors.connect ++;
        close(fd);
        return -1;
    }

    send(fd, str, strlen(str) + 1, 0);
    recv(fd, buf, RECVBUF, 0);

    close(fd);
    return fd;
}

#endif
