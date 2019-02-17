PROG= timer
UTILDIR = ../util
INSTALLDIR=../bin

CFLAGS= -g  -D_REENTRANT -Wall  -I$(UTILDIR)
LDFLAGS= -L$(UTILDIR) -lvmb -lpthread
CC = gcc


all: $(PROG)

utilities:
	$(MAKE) -C  $(UTILDIR)



$(PROG): main.o timer.o  utilities
	$(CC) $(CFLAGS)  main.o timer.o  $(LDFLAGS) -o $(PROG)

install: all
	install $(PROG) $(INSTALLDIR)

clean:
	rm -f $(PROG) *.o *~

