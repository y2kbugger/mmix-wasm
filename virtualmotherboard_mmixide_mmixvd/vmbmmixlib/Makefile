
CFLAGS= -O3 -ggdb -D_REENTRANT -Wall -pg -I../util
CC= gcc
AR = ar
ARFLAGS = cru
RANLIB = ranlib

VMBMMIXOBJECTS = address.o mmix-bus.o 

.PHONY:  clean
     
all: libvmbmmix.a
     
libvmbmmix.a: $(VMBMMIXOBJECTS)
	rm -f $@
	$(AR) $(ARFLAGS) $@ $(VMBMMIXOBJECTS)
	$(RANLIB) $@

install:
	echo nothing to do


clean:
	rm -f *.o *.a *~ 
