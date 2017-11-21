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

void *thread_main(void *arg);

#endif
