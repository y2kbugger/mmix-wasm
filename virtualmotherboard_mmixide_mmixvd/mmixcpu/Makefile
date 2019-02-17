PROG= mmixcpu
UTILDIR = ../util
INSTALLDIR=../bin
MMIXLIBDIR=./mmixlib
VMBMMIXLIBDIR=../vmbmmixlib


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

$(MMIXLIBDIR)/libconfig.h: $(MMIXLIBDIR)/libconfig.cpu
	cp $< $@

$(MMIXLIBDIR)/libimport.h: $(MMIXLIBDIR)/libimport.cpu
	cp $< $@

$(VMBMMIXLIBDIR)/libvmbmmix.a:
	$(MAKE) -C  $(VMBMMIXLIBDIR) libvmbmmix.a

$(PROG): $(PROG).c libraries
	$(CC) $(CFLAGS) $(PROG).c $(LDFLAGS) -o $(PROG)

install: all
	install $(PROG) $(INSTALLDIR)

clean:
	rm -f $(PROG) *.o *~

