PROG= vram
UTILDIR = ../util
INSTALLDIR=../bin

CFLAGS= -g  -D_REENTRANT -Wall  -I$(UTILDIR) -I/usr/X11R6/include
LDFLAGS= -L/usr/X11R6/lib -lX11 -L$(UTILDIR) -lvmb -lpthread
CC = gcc


all: $(PROG) TAGS

utilities:
	$(MAKE) -C  $(UTILDIR)

$(PROG): $(PROG).c utilities
	$(CC) $(CFLAGS) $(PROG).c $(LDFLAGS) -o $(PROG)

install: all
	install $(PROG) $(INSTALLDIR)


TAGS: *.c $(UTILDIR)/*.c $(UTILDIR)/*.h 
	etags *.c $(UTILDIR)/*.c $(UTILDIR)/*.h

clean:
	rm -f $(PROG) *.o *~ TAGS


