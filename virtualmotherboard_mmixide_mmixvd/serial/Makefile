

PROGS = talk serial utilities
UTILDIR = ../util
INSTALLDIR=../bin

CFLAGS= -ggdb  -D_REENTRANT -Wall  -I$(UTILDIR) 
LDFLAGS= -L$(UTILDIR) -lvmb -lpthread
CC=gcc

all: $(PROGS)

serial: serial.o optiontab.o utilities 
	$(CC) $(CFLAGS)  serial.o optiontab.o  $(LDFLAGS) -o $@

utilities:
	$(MAKE) -C  $(UTILDIR)

install: serial
	install serial $(INSTALLDIR)

clean:
	rm -f *.o *~ $(PROGS)
