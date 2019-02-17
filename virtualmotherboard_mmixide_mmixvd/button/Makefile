PROG= button
UTILDIR = ../util
INSTALLDIR=../bin

#CFLAGS= -g  -D_REENTRANT -Wall  -I$(UTILDIR) 
CFLAGS= -O3  -D_REENTRANT -Wall  -I$(UTILDIR) 
LDFLAGS= -L$(UTILDIR) -lvmb -lpthread
CC = gcc


all: $(PROG) TAGS

utilities:
	$(MAKE) -C  $(UTILDIR)



$(PROG): $(PROG).c  utilities
	$(CC) $(CFLAGS) $(PROG).c optiontab.c $(LDFLAGS) -o $(PROG)


TAGS: *.c *.h $(UTILDIR)/*.c $(UTILDIR)/*.h 
	etags *.c *.h $(UTILDIR)/*.c $(UTILDIR)/*.h


install: all
	install $(PROG) $(INSTALLDIR)

clean:
	rm -f $(PROG) *.o *~ *.elf TAGS





