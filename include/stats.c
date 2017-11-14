#include "stats.h"

uint64_t time_us()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return (t.tv_sec * 1000000) + t.tv_usec;
}

int compare_fun(const void *param1, const void *param2)
{
    return *(int*)param1 - *(int*)param2;
}
