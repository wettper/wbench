AUTOMAKE_OPTIONS = foreign
INCLUDES = -I $(top_srcdir)/include/
bin_PROGRAMS = wbench
wbench_SOURCES = wbench.c
wbench_LDADD = $(top_srcdir)/include/libfilter.a\
			  	$(top_srcdir)/include/libsocket.a\
			   	$(top_srcdir)/include/libstats.a\
			   	$(top_srcdir)/include/libthread.a
LIBS= -l pthread
