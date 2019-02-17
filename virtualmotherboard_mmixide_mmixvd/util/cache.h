/*
    Copyright 2007 Martin Ruckert
    
    ruckertm@acm.org

    This file is part of the Virtual Motherboard project

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

#ifndef CACHE_H
#define CACHE_H

/* Change the following values to configure the cache */

#define LINEBITS  8      /* the n low-order bits to index the content of a cache line */
#define CACHEBITS 16     /* the bits used to index the cache (including linebits,*/
			 /* ignoring the (64-CACHEBITS) high-order bits. Must be <= 32.*/
#define WAYS      4      /* a k way cache, k ist the number of lines per set */


/* There should be no configuration necessary below this line. */

#define CACHESIZE (1<<(CACHEBITS-LINEBITS))
#define CACHEMASK ((1<<CACHEBITS)-1)  /* mask to get cache bits */
#define LINESIZE (1<<LINEBITS) /* bytes per cache line */
#define LINEMASK (LINESIZE-1) /* mask to get line bits */
#define LINEOFFSET(address) (address & LINEMASK)
#define LINESTART(address) (address & ~LINEMASK)
#define CACHEINDEX(address)  ((address & CACHEMASK)>>LINEBITS)

typedef struct {
  data_address content;
  unsigned char dirty;
  unsigned int access;
} vmb_cache_line;

typedef struct {
  vmb_cache_line line[CACHESIZE][WAYS];
  unsigned int access;
} vmb_cache;


/* providing an instruction and a data cache */
extern vmb_cache  vmb_d_cache;
extern vmb_cache  vmb_i_cache;

extern const unsigned char *
vmb_cache_read(device_info *vmb, 
			   vmb_cache *c, 
               unsigned int hi_address,
               unsigned int lo_address);

extern unsigned int 
vmb_cache_read_int(device_info *vmb, vmb_cache *c, 
                   unsigned int hi_address,
                   unsigned int lo_address);

extern unsigned char *
vmb_cache_write(device_info *vmb, vmb_cache *c, 
               unsigned int hi_address,
               unsigned int lo_address);

extern void vmb_cache_preload(device_info *vmb, vmb_cache *c, unsigned int hi_address,
			      unsigned int lo_address);


extern void vmb_cache_init(vmb_cache *c); 
extern void vmb_cache_clear(vmb_cache *c);
extern void vmb_cache_clear_line(vmb_cache *c, int address_hi, int address_lo);
extern void vmb_cache_flush(device_info *vmb, vmb_cache *c);
extern void vmb_cache_flush_line(device_info *vmb, vmb_cache *c,int address_hi,int address_lo);
extern void vmb_cache_zero_line(device_info *vmb, vmb_cache *c,int address_hi,int address_lo);

#endif
