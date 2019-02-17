/*
    Copyright 2005 Alexander Ukas, Martin Ruckert
    
    ruckertm@acm.org

    This file is part of the MMIX Motherboard project

    This file is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This software is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this software; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#include <windows.h>
extern HWND hMainWnd;
#include "winopt.h"
#include "inspect.h"
#else
#include <unistd.h>
#endif
#include "bus-arith.h"
#include "vmb.h"
#include "param.h"

int major_version=2, minor_version=1;
char version[]="$Revision: 664 $ $Date: 2018-12-10 18:10:14 +0100 (Mon, 10 Dec 2018) $";
char title[] ="VMB RAM";

char howto[] =
"\n"
"The program will contact the motherboard at [host:]port\n"
"and register itself with the given start address.\n"
"Then, the program will answer read and write requests from the bus.\n"
"\n"
;

#define BITS 32

#define ROOTBITS 8
#define ROOTSIZE (1<<ROOTBITS)
#define ROOTMASK (ROOTSIZE-1)

#define MIDBITS 8
#define MIDSIZE (1<<MIDBITS)
#define MIDMASK (MIDSIZE-1)

#define PAGEBITS 16
#define PAGESIZE (1<<PAGEBITS)
#define PAGEMASK (PAGESIZE-1)


/* simulated 32bit ram (4GByte) as 0x100 * 0x100 * 0x10000 byte */
/* page level functions */
static int ram_write_page(unsigned char *page[], int j, int offset,int size,unsigned char *payload)
/* write size byte from payload into the jth page at offset.
   return the number of byte written */
{ if (page[j]==NULL) {
    page[j] = calloc(PAGESIZE,sizeof(unsigned char));
    if (page[j]==NULL) {
      vmb_error(__LINE__,"Out of memory");
      return size;
    }
  }
  if (offset>=PAGESIZE)
    return 0;
  if (size+offset > PAGESIZE)
    size = PAGESIZE-offset;
  memmove(page[j]+offset,payload,size);
  return size;
}

static int ram_read_page(unsigned char *page[], int j, int offset,int size,unsigned char *payload)
/* read size byte from the jth page at offset into payload.
   return the number of byte read */
{ if (offset>=PAGESIZE)
    return 0;
  if (size+offset > PAGESIZE)
    size = PAGESIZE-offset;
  if (page==NULL || page[j]==NULL)
    memset(payload,0,size);
  else
    memmove(payload,page[j]+offset,size);
  return size;
}

/* mid level functions */
static unsigned char **root[ROOTSIZE];

static int ram_write_mid(int i, int offset,int size,unsigned char *payload)
/* write size byte from payload into the ith root entry at offset.
   return the number of byte written */
{ int j,n;
  if (root[i]==NULL) {
    root[i] = calloc(MIDSIZE,sizeof(unsigned char *));
    if (root[i]==NULL) { 
      vmb_error(__LINE__,"Out of memory");
      return size;
    }
  }
  j = (offset>>(BITS-ROOTBITS-MIDBITS))&MIDMASK;
  offset = offset & PAGEMASK;
  n = ram_write_page(root[i],j,offset,size,payload);
  if (n<size && j+1 < MIDSIZE)
    n = n + ram_write_page(root[i],j+1,0,size-n,payload+n); 
  return n;
}

static int ram_read_mid(int i, int offset,int size,unsigned char *payload)
/* read size byte from the ith root entry at offset into payload
   return the number of byte read */
{ int j,n;
  j = (offset>>(BITS-ROOTBITS-MIDBITS))&MIDMASK;
  offset = offset & PAGEMASK;
  n = ram_read_page(root[i],j,offset,size,payload);
  if (n<size && j+1 < MIDSIZE)
    n = n + ram_read_page(root[i],j+1,0,size-n,payload+n); 
  return n;
}

/* root level functions */

static int ram_write(unsigned int offset,int size,unsigned char *payload)
/* write size byte from payload at offset into the ram */
{ int i,n;
  i = (offset>>(BITS-ROOTBITS))&ROOTMASK;
  offset = offset &  ((MIDMASK<<PAGEBITS)|PAGEMASK);
  n = ram_write_mid(i,offset,size,payload);
  if (n<size && i+1 < ROOTSIZE)
    n = n + ram_write_mid(i+1,0,size-n,payload+n);
#ifdef WIN32
  mem_update(offset, size);
#endif
  return n;
}

static int ram_read(unsigned int offset,int size,unsigned char *payload)
/* read size byte from the ram at offset into payload */
{ int i,n;
  i = (offset>>(BITS-ROOTBITS))&ROOTMASK;
  offset = offset & ((MIDMASK<<PAGEBITS)|PAGEMASK);
  n = ram_read_mid(i,offset,size,payload);
  if (n<size && i+1 < ROOTSIZE)
    n = n + ram_read_mid(i+1,0,size-n,payload+n);
  return n;
}


static void ram_clean(void)
{ int i,j;
  for (i=0;i<ROOTSIZE;i++)
    if (root[i]!=NULL)
    { for (j=0;j<MIDSIZE;j++)
        if (root[i][j]!=NULL) {
          free(root[i][j]);
          root[i][j]=NULL;
        }
      root[i]=NULL;
    }
#ifdef WIN32
   inspector[0].address=vmb_address;
   inspector[0].size=vmb_size;
   mem_update(0, vmb_size);
#endif
}

/* Interface to the virtual motherboard */

unsigned char *ram_get_payload(unsigned int offset,int size){
  static unsigned char payload[258*8];
  if (ram_read(offset,size,payload)<size)
    vmb_debugi(VMB_DEBUG_ERROR, "Inclomplete read from ram at offset %08X",offset);
  return payload;
}

void ram_put_payload(unsigned int offset,int size, unsigned char *payload){
  if (ram_write(offset,size,payload)<size)
    vmb_debugi(VMB_DEBUG_ERROR, "Inclomplete write to ram at offset %08X",offset);
}

void ram_poweron(void)
{ ram_clean();
#ifdef WIN32
   PostMessage(hMainWnd,WM_VMB_ON,0,0);
#endif
}

void ram_poweroff(void)
{ ram_clean();
#ifdef WIN32
   PostMessage(hMainWnd,WM_VMB_OFF,0,0);
#endif
}


void ram_reset(void)
{ 
  ram_clean();
}

#ifdef WIN32
struct inspector_def inspector[2] = {
    /* name size get_mem address num_regs regs */
	{"Memory",0,ram_read,ram_get_payload,ram_put_payload,0,0, 8,0,NULL},
	{0}
};
#endif

void init_device(device_info *vmb)
{ ram_clean();
  vmb->poweron=ram_poweron;
  vmb->poweroff=ram_poweroff;
  vmb->disconnected=vmb_disconnected;
  vmb->reset=ram_reset;
  vmb->terminate=vmb_terminate;
  vmb->put_payload=ram_put_payload;
  vmb->get_payload=ram_get_payload;
#ifdef WIN32
  inspector[0].address=vmb_address;
  inspector[0].size=vmb_size;
#endif
}
