AUTOMAKE_OPTIONS = foreign
INCLUDES = -I $(top_srcdir)/include/
noinst_LIBRARIES = libfilter.a libsocket.a libstats.a libthread.a
libfilter_a_SOURCES = config.h filter.h filter.c
libsocket_a_SOURCES = config.h thread.h socket.h socket.c
libstats_a_SOURCES = config.h stats.h stats.c
libthread_a_SOURCES = config.h stats.h socket.h socket.c thread.h thread.c
