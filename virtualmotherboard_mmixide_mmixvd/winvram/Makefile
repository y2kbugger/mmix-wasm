PROG= ram
UTILDIR = ../util
INSTALLDIR=../bin
CFLAGS= -g -D_REENTRANT -Wall
CC = gcc


all: $(PROG)

utilities:
	$(MAKE) -C  $(UTILDIR)


$(PROG): $(PROG).c  utilities
	$(CC) $(CFLAGS) -I$(UTILDIR) $(PROG).c  -L$(UTILDIR) -lvmb -lvmbopt -lpthread  -o $(PROG)

install: all
	install $(PROG) $(INSTALLDIR)

clean:
	rm -f $(PROG) *.o *~

