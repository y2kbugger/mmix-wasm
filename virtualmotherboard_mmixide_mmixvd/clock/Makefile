PROG= clock
UTILDIR = ../util
OPTDIR = ../opt
INSTALLDIR=../bin

CFLAGS= -g  -D_REENTRANT -Wall  -I$(UTILDIR) -I$(OPTDIR)
LDFLAGS= -L$(OPTDIR) -lvmbopt -L$(UTILDIR) -lvmb -lpthread
CC = gcc


all: $(PROG)

utilities:
	$(MAKE) -C  $(UTILDIR)


$(PROG): main.c  utilities
	$(CC) $(CFLAGS)  main.c  $(LDFLAGS) -o $(PROG)

install: all
	install $(PROG) $(INSTALLDIR)

clean:
	rm -f $(PROG) *.o *~

