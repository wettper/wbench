#ifndef WBENCH_H
#define WBENCH_H

#define HAVE_STDIO_H        1
#define HAVE_STDLIB_H       1
#define HAVE_STRING_H       1
#define HAVE_ERRNO_H        1
#define HAVE_PTHREAD_H      1
#define HAVE_SIGNAL_H       1
#define HAVE_INTTYPES_H     1
#define HAVE_SYS_SOCKET_H   1
#define HAVE_ARPA_INET_H    1
#define HAVE_NETINET_IN_H   1
#define HAVE_ASSERT_H       1

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif

#endif  /*WEBNCH_H*/
