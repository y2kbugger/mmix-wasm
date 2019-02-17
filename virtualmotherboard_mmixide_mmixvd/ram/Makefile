PROG= ram
UTILDIR = ../util
INSTALLDIR=../bin

#CFLAGS= -g  -D_REENTRANT -Wall  -I$(UTILDIR) -I$(OPTDIR)
CFLAGS= -O3  -D_REENTRANT -Wall  -I$(UTILDIR) 
LDFLAGS=  -L$(UTILDIR) -lvmb -lpthread
CC = gcc


all: $(PROG)

utilities:
	$(MAKE) -C  $(UTILDIR)

$(PROG): $(PROG).c  main.c utilities
	$(CC) $(CFLAGS) $(PROG).c main.c $(LDFLAGS) -o $(PROG)

install: all
	install $(PROG) $(INSTALLDIR)

clean:
	rm -f $(PROG) *.o *~

