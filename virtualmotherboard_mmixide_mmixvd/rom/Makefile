PROG= rom
UTILDIR = ../util
INSTALLDIR=../bin
#CFLAGS= -g  -D_REENTRANT -Wall  -I$(UTILDIR) -I$(OPTDIR)
CFLAGS= -O3  -D_REENTRANT -Wall  -I$(UTILDIR)
LDFLAGS= -L$(UTILDIR) -lvmb -lpthread
CC = gcc


all: $(PROG) TAGS

utilities:
	$(MAKE) -C  $(UTILDIR)

$(PROG): $(PROG).c  main.c utilities
	$(CC) $(CFLAGS) $(PROG).c main.c $(LDFLAGS) -o $(PROG)


TAGS: *.c *.h $(UTILDIR)/*.c $(UTILDIR)/*.h
	etags *.c *.h $(UTILDIR)/*.c $(UTILDIR)/*.h

# making the rom bios program cross-compiling on the host with target = mmix

BIN=/usr/local/bin
MAS=$(BIN)/mmix-as
MLD=$(BIN)/mmix-ld
MCC=$(BIN)/mmix-gcc
MCFLAGS= -g -Wall
MLDFLAGS=--entry=_start.
#MLDFLAGS=--section-start .text=0x8000000000000000 



.o: %.mms
	$(MAS) -gdwarf2 --linker-allocated-gregs $^ -o $@

bios.o: bios.s
	$(MCC) $(CFLAGS) -c  $^ -o $@

#io.o: io.c
#	$(MCC) $(CFLAGS) -mabi=mmixware -mno-base-addresses -O2 -c $^



OBJECTS= bios.o 

bios.img: $(OBJECTS)
	$(MLD) -t -T bios.x $(MLDFLAGS) --oformat binary $(OBJECTS) -o bios.img


bios.elf: $(OBJECTS)
	$(MLD) -t -T bios.x $(MLDFLAGS) --oformat elf64-mmix $(OBJECTS) -o bios.elf

%.mmo: %.mms
	mmixal $<

%.img: %.mmo
	../mmix/mmoboot $< 


install: all
	install $(PROG) $(INSTALLDIR)
#	install bios.img $(INSTALLDIR)
#	install bios.elf $(INSTALLDIR)

clean:
	rm -f $(PROG) *.o *~ *.rom *.elf





