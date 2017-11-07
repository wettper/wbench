#ifndef WBENCH_H
#define WBENCH_H

#ifndef HAVE_STDIO_H
#define HAVE_STDIO_H        1
#include <stdio.h>
#endif

#ifndef HAVE_STDLIB_H
#define HAVE_STDLIB_H       1
#include <stdlib.h>
#endif

#ifndef HAVE_ERRNO_H
#define HAVE_ERRNO_H        1
#include <errno.h>
#endif

#ifndef HAVE_SIGNAL_H
#define HAVE_SIGNAL_H       1
#include <signal.h>
#endif

#include "config.h"
#include "filter.h"
#include "stats.h"
#include "socket.h"
#include "thread.h"

#endif  /*WEBNCH_H*/
