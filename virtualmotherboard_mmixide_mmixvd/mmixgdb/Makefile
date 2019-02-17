PROG= mmixgdb
UTILDIR = ../util
INSTALLDIR=../bin
MMIXLIBDIR=./mmixlib
VMBMMIXLIBDIR=../vmbmmixlib

GDBOBJ= buffers.o gdb.o remote-utils.o

#CFLAGS= -g  -D_REENTRANT -Wall  -I$(UTILDIR) -I$(VMBMMIXLIBDIR)
CFLAGS= -O3  -D_REENTRANT -Wall  -I$(UTILDIR)  -I$(VMBMMIXLIBDIR)
LDFLAGS=  -L$(MMIXLIBDIR) -lmmix  -L$(VMBMMIXLIBDIR) -lvmbmmix  -L$(UTILDIR) -lvmb -lpthread
CC = gcc


all: $(PROG)

libraries: $(UTILDIR)/libvmb.a $(MMIXLIBDIR)/libmmix.a $(VMBMMIXLIBDIR)/libvmbmmix.a

$(UTILDIR)/libvmb.a:
	$(MAKE) -C  $(UTILDIR)

$(MMIXLIBDIR)/libmmix.a: $(MMIXLIBDIR)/libconfig.h  $(MMIXLIBDIR)/libimport.h
	$(MAKE) -C $(MMIXLIBDIR) libmmix.a

$(MMIXLIBDIR)/libconfig.h: $(MMIXLIBDIR)/libconfig.gdb
	cp $< $@

$(MMIXLIBDIR)/libimport.h: $(MMIXLIBDIR)/libimport.gdb
	cp $< $@



$(PROG): $(PROG).c libraries  $(GDBOBJ)
	$(CC) $(CFLAGS) $(PROG).c $(GDBOBJ) $(LDFLAGS) -o $(PROG)

install: all
	install $(PROG) $(INSTALLDIR)

clean:
	rm -f $(PROG) *.o *~

