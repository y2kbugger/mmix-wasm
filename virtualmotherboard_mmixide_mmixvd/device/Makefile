PROG= devicetemplate
UTILDIR = ../util
CFLAGS= -g -D_REENTRANT -Wall
CC = gcc


all: $(PROG)

utilities:
	$(MAKE) -C  $(UTILDIR)


$(PROG): $(PROG).c  maintemplate.c utilities
	$(CC) $(CFLAGS) -I$(UTILDIR) maintemplate.c $(PROG).c  \
	-L$(UTILDIR) -lvmb -lpthread -o $(PROG)

install: all
	echo nothing to do

clean:
	rm -f $(PROG) *.o *~

