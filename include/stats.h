#ifndef STATS_H
#define STATS_H

#ifndef HAVE_STDIO_H
#define HAVE_STDIO_H        1
#include <stdio.h>
#endif

#ifndef HAVE_SYS_TIME_H
#define HAVE_SYS_TIME_H        1
#include <sys/time.h>
#endif

#include "config.h"

typedef struct {
    uint32_t connect;
    uint32_t read;
    uint32_t write;
    uint32_t status;
    uint32_t timeout;
} errors;

uint64_t time_us();

int compare_fun(const void *param1, const void *param2);

#endif
