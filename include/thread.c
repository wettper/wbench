#include "thread.h"

void *thread_main(void *arg)
{
    thread *thread_info = arg;

    socket_info *socketinfo = calloc(1, thread_info->connections * sizeof(socket_info));

    uint64_t i = 0;
    for (i; i < thread_info->connections; i ++) {
        socket_info *s = &socketinfo[i];
        s->start = time_us();
        connect_socket(thread_info, s);
        s->end = time_us();
        thread_info->socket_info = s;
    }
}
