PROG= disk
UTILDIR = ../util
INSTALLDIR=../bin

CFLAGS= -g  -D_REENTRANT -Wall  -I$(UTILDIR) 
LDFLAGS= -L$(UTILDIR) -lvmb -lpthread 
CC = gcc

all: $(PROG) TAGS

utilities:
	$(MAKE) -C  $(UTILDIR)


$(PROG): $(PROG).c  utilities
	$(CC) $(CFLAGS) $(PROG).c $(LDFLAGS) -o $(PROG)


TAGS: *.c *.h $(UTILDIR)/*.c $(UTILDIR)/*.h
	etags *.c *.h $(UTILDIR)/*.c $(UTILDIR)/*.h

install: $(PROG)
	install $(PROG) $(INSTALLDIR)

clean:
	rm -f $(PROG) *.o *~


