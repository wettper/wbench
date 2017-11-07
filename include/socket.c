#include "socket.h"

int connect_socket(thread *threads)
{
    struct sockaddr_in addr = threads->addr;
    int fd;
    fd = socket(AI_FAMILY, AI_SOCKTYPE, AI_PROTOCOL);
    if (fd < 0) {
        threads->errors.connect ++;
        return -1;
    }

    char *str = "hello world";
    char buf[RECVBUF] = {'\0'};

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        threads->errors.connect ++;
        close(fd);
        return -1;
    }

    send(fd, str, strlen(str) + 1, 0);
    recv(fd, buf, RECVBUF, 0);

    close(fd);

    return fd;
}
