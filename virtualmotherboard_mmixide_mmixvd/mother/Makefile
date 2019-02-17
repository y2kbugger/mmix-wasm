PROG= mother
UTILDIR = ../util
INSTALLDIR=../bin

#CFLAGS= -g  -D_REENTRANT -Wall  -I$(UTILDIR) 
CFLAGS= -O3  -D_REENTRANT -Wall  -I$(UTILDIR) 
LDFLAGS= -L$(UTILDIR) -lvmb -lpthread
CC = gcc


all: $(PROG)

utilities:
	$(MAKE) -C  $(UTILDIR)


$(PROG): $(PROG).c optiontab.c utilities
	$(CC) $(CFLAGS) $(PROG).c optiontab.c $(LDFLAGS) -o $(PROG)

install: all
	install $(PROG) $(INSTALLDIR)


TAGS:
	etags *.c *.h $(UTILDIR)/*.c $(UTILDIR)/*.h 

clean:
	rm -f $(PROG) *.o *~

