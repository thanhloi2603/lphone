AUTOMAKE_OPTIONS = foreign
LDFLAGS=-Wall -std=c99 -DCONFDIR=$(sysconfdir)

#lutils.o: lutils.c lutils.h
#   $(CC) -DCONFDIR=$(sysconfdir) -o $@ $< $(LIBS)

#call_func.o: call_func.c call_func.h
#   $(CC) -DCONFDIR=$(sysconfdir) -o $@ $< $(LIBS)

#lphone: lphone.c call_func.o lutils.o
#   $(CC) -DCONFDIR=$(sysconfdir) -o $@ $< $(LIBS)

bin_PROGRAMS = lphone
lphone_SOURCES = lphone.c lutils.c lutils.h call_func.c call_func.h
sysconf_DATA = lphone.conf
