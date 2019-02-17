CFLAGS= -O3 -ggdb -D_REENTRANT -Wall -pg -I../util
CC= gcc
AR = ar
ARFLAGS = cru
RANLIB = ranlib

#VMBOPTOBJECTS = param.o option.o version.o optiontab.o
VMBOPTOBJECTS = param.o  option.o optiontab.o

.PHONY:  clean
     
all: libvmbopt.a
     
libvmbopt.a: $(VMBOPTOBJECTS)
	rm -f $@
	$(AR) $(ARFLAGS) $@ $(VMBOPTOBJECTS)
	$(RANLIB) $@

install:
	echo nothing to do


clean:
	rm -f *.o *.a *~ 
