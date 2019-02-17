




#include <stdlib.h>
#include <string.h>
#include "vmb.h"
#include "cache.h"
#include "bus-arith.h"


/* providing an instruction and a data cache */
vmb_cache vmb_i_cache = {{{{{0}}}}};
vmb_cache vmb_d_cache = {{{{{0}}}}};


void vmb_cache_init(vmb_cache *c)
     /* initialize the cache structure */
{ int i,k;
  for (i=0;i<CACHESIZE;i++)
    for (k=0;k<WAYS;k++)
      vmb_init_data_address(&c->line[i][k].content,LINESIZE);
  vmb_cache_clear(c);
}


void vmb_cache_clear(vmb_cache *c)
/* initialize the cache to be completely empty */
{ int i,k;
  c->access = 0;
  for (i=0;i<CACHESIZE;i++)
    for (k=0;k<WAYS;k++)
       c->line[i][k].content.status=STATUS_INVALID;
}

static vmb_cache_line *cache_lookup(vmb_cache *c, int address_hi, int address_lo)
/* given a start address of a cache line, return a matching cache line or NULL */
{ int i = CACHEINDEX(address_lo);
  vmb_cache_line *line = c->line[i];
  int w;
  for (w=0;w<WAYS;w++)
    if (line[w].content.status != STATUS_INVALID &&
        line[w].content.address_hi == address_hi &&
        line[w].content.address_lo == address_lo)
        { line[w].access = ++(c->access);
          return line+w;
	}
   return NULL;
}


void vmb_cache_clear_line(vmb_cache *c, int address_hi, int address_lo)
/* initialize the line to be completely empty */
{ vmb_cache_line *line = cache_lookup(c, address_hi, LINESTART(address_lo));
  if (line!=NULL)
    line->content.status=STATUS_INVALID;
}


static vmb_cache_line *cache_lru(vmb_cache *c,  int address_hi, int address_lo)
/* The given address is not in the cache,
   return an invalid or the least recently used cache line */
{
  int i = CACHEINDEX(address_lo);
  vmb_cache_line *line = c->line[i];
  int access = c->access;
  int lru = 0;
  unsigned int age = access-line[lru].access;
  int w;
  for (w=0;w<WAYS;w++)
    if (line[w].content.status == STATUS_INVALID)
    { lru = w; break;}
    else if ((unsigned)(access-line[w].access) >= age)
    { age = access-line[w].access; lru=w; }
  return line+lru;
}


static vmb_cache_line *cache_load(device_info *vmb, vmb_cache *c,  int address_hi, int address_lo)
/* The given start address it not in the cache. 
   load data at the given start address into the cache and return a line */
{ vmb_cache_line *line = cache_lru(c,address_hi, address_lo);
  if (line->content.status == STATUS_INVALID)
    line->content.status = STATUS_VALID;
  else if (line->content.status == STATUS_VALID && line->dirty)
  { vmb_store(vmb, &line->content);
    line->dirty = 0;
  }
  if (line->content.status != STATUS_VALID)
    vmb_wait_for_valid(vmb, &line->content);

  line->access = ++(c->access);
  line->content.address_hi = address_hi;
  line->content.address_lo = address_lo;
  vmb_load(vmb, &line->content);
  line->dirty = 0;
  return line;
}


/* Now the functions to access the cache */

/* Reading Bytes and Integers */

static vmb_cache_line *cache_preload(device_info *vmb, vmb_cache *c, int address_hi, int address_lo) 
/* load the cache line for the given address. Dont wait for completion */

{ vmb_cache_line *line = cache_lookup(c, address_hi, LINESTART(address_lo));
  if (line==NULL)
    line = cache_load(vmb, c, address_hi, LINESTART(address_lo));
  return line;
}

 void vmb_cache_preload(device_info *vmb, vmb_cache *c, unsigned int hi_address,
			      unsigned int lo_address)
/* this is the external version of the previous function, it does not return a cache line */
{ cache_preload(vmb, c, hi_address, LINESTART(lo_address));
}

static vmb_cache_line *get_valid_line(device_info *vmb, vmb_cache *c, int address_hi, int address_lo) 
/* get a valid cache line containing the requested address */
{ vmb_cache_line *line = cache_preload(vmb, c, address_hi, LINESTART(address_lo));
  if (line->content.status != STATUS_VALID)
    vmb_wait_for_valid(vmb, &line->content);
  return line;
}


const unsigned char *vmb_cache_read(device_info *vmb, vmb_cache *c, unsigned int address_hi, unsigned int address_lo) 
/* return a pointer to the requested byte.
   there is no alignment check or enforcement 
   and access must not cross line boundaries
   make sure alignment is propper and linesizes are multiples of the maximum
   alignment requirements.
*/
{ vmb_cache_line *line = get_valid_line(vmb, c, address_hi, LINESTART(address_lo));
  return line->content.data+LINEOFFSET(address_lo);
}

unsigned int vmb_cache_read_int(device_info *vmb, 
								vmb_cache *c, 
                   unsigned int address_hi,
                   unsigned int address_lo)
/* return a four byte integer from the cache.
   there is no alignment check or enforcement 
   and access must not cross line boundaries
   make sure alignment is propper and linesizes are multiples of the maximum
   alignment requirements.
*/
{ vmb_cache_line *line = get_valid_line(vmb,c, address_hi, LINESTART(address_lo));
  return chartoint(line->content.data+LINEOFFSET(address_lo));
}


/* Writing Bytes */

unsigned char *vmb_cache_write(device_info *vmb, vmb_cache *c, unsigned  int address_hi,  unsigned int address_lo) 
/* return a pointer to n byte where the data can be written
   there is no alignment check or enforcement 
   and access must not cross line boundaries
   make sure alignment is propper and linesizes ar multiples of the maximum
   alignment requirements.
*/
{ vmb_cache_line *line = get_valid_line(vmb, c, address_hi, LINESTART(address_lo));
  line->dirty = 1;
  return line->content.data+LINEOFFSET(address_lo);
}

void vmb_cache_flush(device_info *vmb, vmb_cache *c)
     /* write all the dirty lines to memory */
{ int i,k;
  for (i=0;i<CACHESIZE;i++)
    for (k=0;k<WAYS;k++)
      if (c->line[i][k].content.status==STATUS_VALID && c->line[i][k].dirty)
        { vmb_store(vmb, &c->line[i][k].content);
          c->line[i][k].dirty = 0;
        }       
}


void vmb_cache_flush_line(device_info *vmb, vmb_cache *c,int address_hi,int address_lo)
     /* less drastic flush of just one dirty line */
{ vmb_cache_line *line;
  line = cache_lookup(c,address_hi, LINESTART(address_lo));
  if (line!=NULL && line->content.status == STATUS_VALID && line->dirty)
        { vmb_store(vmb, &line->content);
          line->dirty = 0;
        }       
}



void vmb_cache_zero_line(device_info *vmb, vmb_cache *c,int address_hi,int address_lo)
/* zero out one line, its faster than writing because no load is needed */
{ vmb_cache_line *line;
  line = cache_lookup(c,address_hi, LINESTART(address_lo));
  if (line==NULL)
  { line = cache_lru(c,address_hi, address_lo);
    if (line->content.status == STATUS_INVALID)
      line->content.status = STATUS_VALID;
    else if (line->content.status == STATUS_VALID && line->dirty)
    { vmb_store(vmb,&line->content);
      line->dirty = 0;
    }
    if (line->content.status != STATUS_VALID)
      vmb_wait_for_valid(vmb, &line->content);
    line->content.address_hi = address_hi;
    line->content.address_lo = address_lo;
  }       
  line->access = ++(c->access);
  memset(line->content.data,0,LINESIZE);
  line->dirty = 1;
}
