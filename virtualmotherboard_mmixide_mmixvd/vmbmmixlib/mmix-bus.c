/*
    Copyright 2005 Martin Ruckert
    
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
#include "mmix-internals.h"
#include "address.h"
#include "vmb.h"
#include "message.h"
#include "cache.h"
#include "bus-arith.h"

extern device_info vmb;

/* checking for interrupt */
int get_interrupt(octa *data)
{ if (vmb_get_interrupt(&vmb,&(data->h),&(data->l))==1)
  { g[rQ].h |= data->h; g[rQ].l |= data->l;
    return 1;
  }
  else
    return 0;
}

int wait_time(int ms)
{ return vmb_wait_for_event_timed(&vmb,ms);
}

void cancel_wait(void)
{ vmb_cancel_wait_for_event(&vmb); 
}

/* loading data from physical memory */

static void load_uncached_memory(unsigned char *data, int size, octa address)
     /* load size byte from the given physical address
        into the memory pointed to by data 
        size must be 1,2,4, or n*8 byte, with 1<=n<=256.
*/

{ data_address da;
  da.address_hi = address.h;
  da.address_lo = address.l;
  da.data = data;
  da.size = size;
  vmb_load(&vmb,&da);
  vmb_wait_for_valid(&vmb,&da);
}

static void store_uncached_memory(unsigned char *data, int size, octa address)
     /* store size byte to the given physical address
        from the memory pointed to by data
        size must be 1,2,4, or n*8 byte, with 1<=n<=256.
     */
{ data_address da;
  da.address_hi = address.h;
  da.address_lo = address.l;
  da.data = data;
  da.size = size;
  vmb_store(&vmb,&da);
}


static void char_to_octa(int size, const unsigned char *d,octa *data, int signextension)
{  if (size==8)
  { data->h = chartoint(d);
    data->l = chartoint(d+4);
  }
  else
    { if (signextension && (d[0]&0x80))
      { signed int i;
        data->h = 0xFFFFFFFF;
        i = chartoint(d);
        if (size < 4) i= i>>((4-size)*8);
	data->l = i;
      }
      else
      { unsigned int i;
        data->h = 0;
        i = chartoint(d);
        if (size < 4) i= i>>((4-size)*8);
	data->l = i;
      }
   }
}


/* Functions used in mmix-sim.ch */

void load_uncached_data(int size, octa *data, octa address, int signextension)
/* load size (1,2,4,or 8) byte into data from the given physical address 
*/
{ unsigned char d[8]={0};
  address.l=address.l&~(size-1);  /* round down to next alignment */
  load_uncached_memory(d,size,address);
  char_to_octa(size,d,data,signextension);
}

void store_uncached_data(int size, octa data, octa address)
/* store an octa, tetra, wyde, or byte (depending on size)
   in data to the given physical address 
   raise an interrupt if there is a problem
*/
{ unsigned char d[8]= {0};
  address.l=address.l&~(size-1);  /* round down to next alignment */
  if (size == 8)
  { inttochar(data.h,d);
    inttochar(data.l,d+4);
    store_uncached_memory(d, size, address);
  }
  else
  { inttochar(data.l,d);
    store_uncached_memory(d+(4-size), size, address);
  }
}


void load_cached_data(int size, octa *data, octa address, int signextension)
{
  address.l=address.l&~(size-1);  /* round down to next alignment */
  char_to_octa(size,vmb_cache_read(&vmb,&vmb_d_cache, address.h, address.l),data,signextension);
}

void load_cached_instruction(tetra *instruction, octa address)
{
  address.l=address.l&~0x3;  /* round down to next alignment */
  *instruction = vmb_cache_read_int(&vmb,&vmb_i_cache, address.h, address.l);
}

#include <stdio.h>

void store_cached_data(int size, octa data, octa address)
{ unsigned char *d;
  address.l=address.l&~(size-1);  /* round down to next alignment */
  d = vmb_cache_write(&vmb,&vmb_d_cache, address.h, address.l);
 if (size == 8)
  { inttochar(data.h,d);
    inttochar(data.l,d+4);
  }
  else
  { unsigned char tmp[4];
    inttochar(data.l,tmp);
    memcpy(d,tmp+(4-size),size);
  }
#if 0
 { unsigned char search[8]={0x01,0x73,0x01,0x68,0x01,0x2d,0x01,0x33};
  // check for special store 
   if (strncmp(d+size-8,search,8)==0)
     { fprintf(stderr,"loc: %08x %08x, addr: %08x %08x, size: %d\n", loc.h, loc.l,address.h, address.l, size);
	 g[rQ].l|=B_BIT;
  }
  }
#endif
}

extern octa incr(octa y,int delta);

void write_all_data_cache(void)
{ vmb_cache_flush(&vmb,&vmb_d_cache);
}

void clear_all_data_cache(void)
{ vmb_cache_clear(&vmb_d_cache);
}

void clear_all_instruction_cache(void)
{ vmb_cache_clear(&vmb_i_cache);
}


void write_data_cache(octa address, int size)
     /* flush the cache starting at address for size byte */
{ int i;
  for (i = -(int)(address.l&LINEMASK); i<size;i=i+LINESIZE)
  { vmb_cache_flush_line(&vmb,&vmb_d_cache, address.h, address.l);
    address=incr(address,LINESIZE);
  }
}

void clear_data_cache(octa address, int size)
     /* clear the cache starting at address for size byte */
{ int i;
  for (i = -(int)(address.l&LINEMASK); i<size;i=i+LINESIZE)
  { vmb_cache_clear_line(&vmb_d_cache, address.h, address.l);
    address=incr(address,LINESIZE);
  }
}

void clear_instruction_cache(octa address, int size)
     /* clear the cache starting at address for size byte */
{ int i;
  for (i = -(int)(address.l&LINEMASK); i<size;i=i+LINESIZE)
  { vmb_cache_clear_line(&vmb_i_cache, address.h, address.l);
    address=incr(address,LINESIZE);
  }
}

void prego_instruction_cache(octa address, int size)
/* load the instruction cache starting at address for size byte */
{ int i;
  for (i = -(int)(address.l&LINEMASK); i<size;i=i+LINESIZE)
  { vmb_cache_preload(&vmb,&vmb_i_cache, address.h, address.l);
    address=incr(address,LINESIZE);
  }
}

void preload_data_cache(octa address, int size)
/* load the data cache starting at address for size byte */
{ int i;
  for (i = -(int)(address.l&LINEMASK); i<size;i=i+LINESIZE)
  { vmb_cache_preload(&vmb,&vmb_d_cache, address.h, address.l);
    address=incr(address,LINESIZE);
  }
}

/* sideeffects of special bus messages */

 
static void mmix_buserror(unsigned char type, unsigned char a[8])
{ char hex[17]={0};
  chartohex(a,hex,8);
  vmb_debugs(VMB_DEBUG_ERROR, "Bus error detected at %s\n",hex);
  g[rF].h = chartoint(a);
  g[rF].l = chartoint(a+4);
  g[rQ].l |= NM_BIT;
  new_Q.l |= NM_BIT;
}


void vmb_atexit(void)
{ vmb_disconnect(&vmb);
  vmb_end();
}

void init_mmix_bus(char *host, int port, char *name)
{ vmb_begin();
  vmb_debug_flag = 0;
  vmb_program_name = "MMIX CPU";
  vmb_connect(&vmb,host, port);
  if (!vmb.connected) return;
  vmb_register(&vmb,0,0,0,-1,-1,name,g[rN].h>>24,(g[rN].h>>16)&0xFF);
  vmb_cache_init(&vmb_i_cache);
  vmb_cache_init(&vmb_d_cache);
  vmb.buserror = mmix_buserror;
}


