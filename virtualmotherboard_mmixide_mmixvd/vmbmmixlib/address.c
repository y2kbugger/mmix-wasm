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
#include "message.h"
#include "mmix-internals.h"
#include "address.h"
#include "mmix-bus.h"

/* CONSTANTS */
#define EXEC_BIT 0x1
#define WRITE_BIT 0x2
#define READ_BIT 0x4
#define PAGE_TABLE_ERROR 0x8


/* TYPES */

/* Page Table Entry */
typedef struct {
  /* int x;  not implemented */
  octa base;    /* base address with x, y, n, p fields set to zero */
  /* int y;  not implemented */
  unsigned int n;
  int p;
} PTE;



/* Translation Cache Entry */
#define TCSIZE 64

typedef struct {
  int i;
  int n;
  octa base;
  octa phys;
  int p;
} TCE;


/* Tranclation Cache */
typedef struct { int last; TCE tce[TCSIZE];} TC;


/* GLOBAL VARIABLES */

/* the two translation caches for instructions and data */
static TC exec_tc={0,{{0}}}, data_tc={0,{{0}}}; 

/* two inline caches for  instruction and data */

static octa last_i_addr={0x80000000,0}, 
            last_i_trans={0,0};

static octa last_d_addr={0x80000000,0}, 
            last_d_trans={0,0};


void clear_all_data_vtc(void)
{ int i;
  for (i=0;i<TCSIZE;i++)
    data_tc.tce[i].p=0; 
  last_d_addr.h=0x80000000; last_d_addr.l=0; 
  last_d_trans.h=0; last_d_trans.l =0;
}


void clear_all_instruction_vtc(void)
{ int i;
  for (i=0;i<TCSIZE;i++)
    exec_tc.tce[i].p=0; 
  last_i_addr.h=0x80000000; last_i_addr.l=0; 
  last_i_trans.h=0; last_i_trans.l =0;
}



/* AUXILIAR FUNCTIONS */


static void unpack_pte(PTE *e,octa *a, int s)
     /* unpack a PTE from an octa a */
{ e->p = a->l& 0x07;
  e->n = (a->l>>3)& 0x3FF;
  if (s<32)
    { unsigned int mask = ~((1<<s)-1); 
      e->base.l = a->l & mask;
      e->base.h = a->h & 0xFFFF;
    }
  else if (s==32)
    { e->base.l = 0;
      e->base.h = a->h & 0xFFFF;
    }
  else
    { unsigned int mask = ~((1<<(s-32))-1); 
      e->base.l = 0;
      e->base.h = a->h & 0xFFFF & mask;
    }
}




/* Handling a page table entry */

static int translate_v2p(int b[5], int s, octa *r, unsigned int n, int i, octa *address)
/* given the components b[i], s,r,n, and i from rV,
   translate the virtual address into 
   a pysical address and return the protection code p.
   return PAGE_TABLE_ERROR if there is no translation.
   use the alorithm as given by Don Knuth in section 47 of mmix-doc.
   modified according to my letter to don.
     */
{  octa ptp, pte;
   int limit;
   int a[5];
   PTE e;

   ptp.h = r->h;
   ptp.l = r->l + (b[i]<<13); /* address of first page table */
   limit = (b[i+1]-b[i])*2;

   address->h = address->h&0x1FFFFFFF; /*remove sign and segment */ 
   *address = shift_right(*address,s,1); /* remove offset get a4a3a2a1a0 */

   if (address->h==0 && address->l==0)
     limit=limit+1;

   if (limit <= 0) goto pagetable_error;
   limit = limit-2;

   a[0] = address->l&0x3FF;
   *address = shift_right(*address,10,1); /* remove a0 */   
   if (address->h!=0 || address->l!=0)
     { if (limit <= 0) goto pagetable_error;
       limit = limit-2;
       ptp.l = ptp.l+0x2000;
       a[1] =  address->l&0x3FF;
       *address = shift_right(*address,10,1); /* remove a1 */
       if (address->h!=0 || address->l!=0)
	 { if (limit <= 0) goto pagetable_error;
           limit = limit-2;
           ptp.l = ptp.l+0x2000;
           a[2] =  address->l&0x3FF;
           *address = shift_right(*address,10,1); /* remove a2 */
           if (address->h!=0 || address->l!=0)
	     { if (limit <= 0) goto pagetable_error;
               limit = limit-2;
               ptp.l = ptp.l+0x2000;
               a[3] =  address->l&0x3FF;
               *address = shift_right(*address,10,1); /* remove a3 */
               a[4] =  address->l&0x3FF;
	       if (a[4] != 0)
		 { if (limit <= 0) goto pagetable_error;
                   limit = limit-2;
                   ptp.l = ptp.l+0x2000;
	           ptp.l = ptp.l+8*a[4];
                   load_cached_data(8,&ptp,ptp,0);
                   if (ptp.h==0 && ptp.l==0) return 0;
                   if (((ptp.h&0x80000000)==0) || ((ptp.l>>3)&0x3FF)!=n)
		     goto pagetable_error;
                   ptp.l = ptp.l & (~0x1FFF);
                   ptp.h = ptp.h & 0x7FFFFFFF;
		 }
	         ptp.l = ptp.l+8*a[3];
                 load_cached_data(8,&ptp,ptp,0);
                 if (ptp.h==0 && ptp.l==0) return 0;
                 if (((ptp.h&0x80000000)==0) || ((ptp.l>>3)&0x3FF)!=n)
		   goto pagetable_error;
                 ptp.l = ptp.l & (~0x1FFF);
                 ptp.h = ptp.h & 0x7FFFFFFF;
	     }
             ptp.l = ptp.l+8*a[2];
             load_cached_data(8,&ptp,ptp,0);
             if (ptp.h==0 && ptp.l==0) return 0;
             if (((ptp.h&0x80000000)==0) || ((ptp.l>>3)&0x3FF)!=n)
	       goto pagetable_error;
             ptp.l = ptp.l & (~0x1FFF);
             ptp.h = ptp.h & 0x7FFFFFFF;
	 }
         ptp.l = ptp.l+8*a[1];
         load_cached_data(8,&ptp,ptp,0);
         if (ptp.h==0 && ptp.l==0) return 0;
         if (((ptp.h&0x80000000)==0) || ((ptp.l>>3)&0x3FF)!=n)
           goto pagetable_error;
         ptp.l = ptp.l & (~0x1FFF);
         ptp.h = ptp.h & 0x7FFFFFFF;
     }
   ptp.l = ptp.l+8*a[0];
   load_cached_data(8,&pte,ptp,0);
   if (pte.h==0 && pte.l==0)
     return 0;
   unpack_pte(&e,&pte,s);
   if (e.n != n) /* this is an error in the page table structure */
     goto pagetable_error;
   *address = e.base;
   return e.p;
 
pagetable_error:
   return PAGE_TABLE_ERROR;
}

/* Handling the Translation Cache */

TCE *TCE_lookup(TC *tc, int i, int n, octa *base)
/* returns NULL if there is no corresponding pte in tc */
{ int k;
  TCE *tce;
  for (k=0;k<TCSIZE;k++)
  { tce = tc->tce + ((tc->last +k) & (TCSIZE-1));
    if ( tce->p != 0 &&
         tce->i == i && 
         tce->n == n &&
         tce->base.h == base->h &&
         tce->base.l == base->l )
      return tce;
  }    
  return NULL;
} 

void TC_store(TC *tc, int i, int n, octa *base, octa *phys, int p)
{ tc->last = (tc->last +1) & (TCSIZE-1);
  tc->tce[tc->last].p = p;
  tc->tce[tc->last].i = i;
  tc->tce[tc->last].n = n;
  tc->tce[tc->last].base = *base;
  tc->tce[tc->last].phys = *phys;
}


static int TC_update(TC *tc, int i, int n, octa *base, int p)
/* updates the access code p in the translation cache.
   if p == 0 it will delete a corresponding entry
*/
{ TCE *tce = TCE_lookup(tc,i,n,base);
  if (tce==NULL) return 0;
  tce->p = p;
  return 1;
} 


int TC_translate(TC *tc, int s, int i, int n, octa *base)
     /* return PAGE_TABLE_ERROR if not found; (the rQ bit is then already set)
        return protection on success and replace the virtual base by the pysical
        if p!=0 store the result in the TLB
     */

{ TCE *tce;
  tce = TCE_lookup(tc,i,n,base);
  if (tce==NULL)
  { int b[5];
    octa r,phys;
    int p;
    b[0] = 0;
    b[1] = (g[rV].h>>28)&0xF;
    b[2] = (g[rV].h>>24)&0xF;
    b[3] = (g[rV].h>>20)&0xF;
    b[4] = (g[rV].h>>16)&0xF;
    r.h  = g[rV].h&0xFF;
    r.l  = g[rV].l&~0x1FFF;
    phys=*base;
    p=translate_v2p(b,s,&r,n,i,&phys);
    if (p==PAGE_TABLE_ERROR)
      return p;
    else if (p==0)
      return 0;
    TC_store(tc,i, n, base, &phys, p);
    *base = phys;
    return p;
  }
  *base = tce->phys;
  return tce->p;
}



#if 1

octa update_vtc(octa key)
/* implements the LDVTS instruction according to mmix-doc */
{  int i;
   int n;
   int p;
   octa x;
   i = (key.h>>29) & 0x03;
   n = (key.l>>3) & 0x3FF;
   p = key.l & 0x07;
   key.h = key.h &0x1FFFFFFF;
   key.l = key.l & ~0x01FF;
   x.h=0;
   x.l = TC_update(&data_tc,i,n, &key,p)<< 1;
   x.l = x.l | TC_update(&exec_tc,i,n, &key,p);
   return x;
}

#else
octa update_vtc(octa key)
/* implements the LDVTS instruction
   according to LDTV.txt proposal 2
 */
{  int i;
   int n;
   int p;
   int s; 
   octa x;
   s = (g[rV].h>>8)&0xFF;  /* extract the page size from rV */
   i = (key.h>>29) & 0x03;
   n = (key.l>>3) & 0x3FF;	
   p = key.l & 0x07;
   key.h = key.h &0x1FFFFFFF;
   key.l = key.l & ~0x01FF;
   x.h=x.l=-1;

   if (p==0) /*delete from both caches*/
     { TC_update(&data_tc,i,n, &key,0);
       TC_update(&exec_tc,i,n, &key,0);
     }
   else if (p == 1) /* check data VT cache, provide translation if present */
     { TCE *tce;
       tce = TCE_lookup(&data_tc,i,n,&key);
        if (tce!=NULL)
	{  x.h = tce->phys.h;
	   x.l = tce->phys.l | tce->p; 
        }
     }
   else if (p == 2) /* check instruction VT cache, provide translation if present */
     { TCE *tce;
       tce = TCE_lookup(&exec_tc,i,n,&key);
        if (tce!=NULL)
	{  x.h = tce->phys.h;
	   x.l = tce->phys.l | tce->p; 
        }
     }
   else if (p == 3) 
      /* lazy update both caches.
         if a matching entry is present in one of the caches, reread the translation
         from memory and replace the matching enties in both caches. */
     {
     }
  else if (p == 4) 
      /* update of data VT cache. reread translation from memory and place
         into data VT cache. replace an existing instruction VT cache entry. */
     {
     }
  else if (p == 5) 
      /* update of instruction VT cache. reread translation from memory and place
         into instruction VT cache. replace an existing data VT cache entry. */
     {
     }
  else if (p == 6) 
      /* read data translation, read a translation from the data cache, if not present
         read the translation from memory and keep the result in the data VT cache */
     { int vn    = (g[rV].l>>3)&0x3FF;
	   p= TC_translate(&data_tc, s, i, vn, &key);
       if (!(p&PAGE_TABLE_ERROR) && p!=0)
       { x.h = key.h;
         x.l = key.l | (n<<3) | p; /*add in protection bits and page offset */
       }
     }
  else if (p == 7) 
      /* read instruction translation, read a translation from the instruction cache,
         if not present, read the translation from memory 
         and keep the result in the instruction VT cache */
     { int vn    = (g[rV].l>>3)&0x3FF;
       p= TC_translate(&exec_tc, s, i, vn, &key);
       if (!(p&PAGE_TABLE_ERROR) && p!=0)
       { x.h = key.h;
         x.l = key.l | (n<<3) | p;
       }
     }
    return x;
}


#endif



static void unpack_address(octa *virt, int s, int *i, octa *base, octa *offset)
     /* unpack a virtual address into segment i, base and offset */
{ *i = (unsigned)virt->h >> 29;
  if (s > 32 )
  { unsigned int mask;
     mask = (1 << (s-32))-1;
     base->h = virt->h & ~mask & 0x1FFFFFFF;
     base->l = 0;
     offset->h = virt->h & mask;
     offset->l = virt->l;
  }
  else if (s == 32)
  { base->h = virt->h & 0x1FFFFFFF;
     base->l = 0;
     offset->h = 0;
     offset->l = virt->l;
  }
  else
  { unsigned int mask;
     mask = (1 << s)-1;
     base->h = virt->h & 0x1FFFFFFF;
     base->l = virt->l & ~mask;
     offset->h = 0;
     offset->l = virt->l & mask;
  }
}


static void store_translation(TC *tc,octa *virt, octa *phys)
{ int vi, pi;
 int n; 
 int s;
 octa vbase, pbase;
 octa voffset, poffset;
 n    = (g[rV].l>>3)&0x3FF;
 s    = (g[rV].h>>8)&0xFF;  /* extract the page size from rV */

 unpack_address(virt, s, &vi, &vbase, &voffset);
 unpack_address(phys, s, &pi, &pbase, &poffset);
 TC_store(tc, vi, n, &vbase, &pbase,poffset.l&0x7);
}

void store_data_translation(octa *virt, octa *phys)
{
  store_translation(&data_tc,virt, phys);
}

void store_exec_translation(octa *virt, octa *phys)
{
  store_translation(&exec_tc,virt, phys);
}


static int translate_address(octa *address, TC *tc)
     /* the virtual address is repaced be the physical address.
	return the protection p (can be PAGE_TABLE_ERROR or 0 or other)
     */
{ /* Use octa g[rV] for the special rV register */
  int s,i,p;
  octa base, offset;
  int n;
  n    = (g[rV].l>>3)&0x3FF;
  s    = (g[rV].h>>8)&0xFF;  /* extract the page size from rV */
  unpack_address(address, s, &i, &base, &offset);
  p= TC_translate(tc, s, i, n, &base);
  if (!(p&PAGE_TABLE_ERROR) && p!=0)
     *address = oplus(base,offset);
  return p;
}

int valid_address(octa address)
/* return 1 if valid, zero otherwise */
{
  if (address.h&0x80000000) return 1;
  else 
  { int p = translate_address(&address, &data_tc);
  if ((p&PAGE_TABLE_ERROR) || p==0) return 0;
  }
  return 1;
}


int load_instruction(tetra *instruction, octa address)
/* load a tetra into instruction from the given virtual address 
   raise an interrupt if there is a problem and load 0.
*/
{ if (address.h==last_i_addr.h &&
      (address.l&0xFFFFE000)==last_i_addr.l)
      address.h=last_i_trans.h,
      address.l=last_i_trans.l|(address.l&0x1FFF);
  else if (address.h&0x80000000)
    /* negative addresses are maped to physical adresses by suppressing the sign bit */
  {  last_i_addr.h = address.h;
     last_i_addr.l = address.l&0xFFFFE000;
     address.h= address.h&0x7FFFFFFF;
     last_i_trans.h = address.h;
     last_i_trans.l = address.l&0xFFFFE000;
  }
  else
  { octa last= address;
    int p = translate_address(&address, &exec_tc);
    if (p&PAGE_TABLE_ERROR)
    { *instruction = 0;
      g[rQ].l |= PT_BIT;
      new_Q.l |= PT_BIT;
      g[rF]=address; 
      return 0;
    } 
    else if (!(p&EXEC_BIT)) 
    { *instruction = 0;
      g[rQ].h |=  PX_BIT;
      new_Q.h |=  PX_BIT;
      g[rF]=address; 
      return 0;
    }
    last_i_addr.h = last.h;
    last_i_addr.l = last.l&0xFFFFE000;
    last_i_trans.h=address.h;
    last_i_trans.l=address.l&0xFFFFE000;
  }
  if (address.h & 0xFFFF0000)
  { /* load uncached */
    octa data;
    load_uncached_data(4,&data,address,0);
    *instruction = data.l;
  }
  else
    load_cached_instruction(instruction,address);
  return 1;
}


int load_data(int size, octa *data, octa address,int signextension)
/* size may be 1, 2, 4 or 8 
   load a byte, wyde, tetra, or octa into data from the given virtual address 
   raise an interrupt if there is a problem
*/

{ if (address.h==last_d_addr.h &&
      (address.l&0xFFFFE000)==last_d_addr.l)
      address.h=last_d_trans.h,
      address.l=last_d_trans.l|(address.l&0x1FFF);
  else if (address.h&0x80000000)
    /* negative addresses are maped to physical adresses by suppressing the sign bit */
  {  last_d_addr.h = address.h;
     last_d_addr.l = address.l&0xFFFFE000;
     address.h= address.h&0x7FFFFFFF;
     last_d_trans.h = address.h;
     last_d_trans.l = address.l&0xFFFFE000;
  }
  else 
  { octa last= address;
    int p = translate_address(&address, &data_tc);
    if (p&PAGE_TABLE_ERROR)
    { data->h=data->l = 0;
      g[rQ].l |= PT_BIT;
      new_Q.l |= PT_BIT; 
      g[rF]=address; 
      return 0;
    }
   else if (!(p&READ_BIT)) 
    { data->h=data->l = 0;
      g[rQ].h |= PR_BIT;
      new_Q.h |= PR_BIT;
      g[rF]=address; 
      return 0;
    }
    last_d_addr.h = last.h;
    last_d_addr.l = last.l&0xFFFFE000;
    last_d_trans.h=address.h;
    last_d_trans.l=address.l&0xFFFFE000;
  }
  if (address.h & 0xFFFF0000)
    load_uncached_data(size,data,address,signextension);
  else
    load_cached_data(size,data,address,signextension);
  return 1;
}


int store_data(int size,octa data, octa address)
/* store an octa from data into the given virtual address 
   raise an interrupt and return 0 if there is a problem
*/
{
  if (address.h==last_d_addr.h &&
      (address.l&0xFFFFE000)==last_d_addr.l)
      address.h=last_d_trans.h,
      address.l=last_d_trans.l|(address.l&0x1FFF);
  else if (address.h&0x80000000)
    /* negative addresses are maped to physical adresses by suppressing the sign bit */
  {  last_d_addr.h = address.h;
     last_d_addr.l = address.l&0xFFFFE000;
     address.h= address.h&0x7FFFFFFF;
     last_d_trans.h = address.h;
     last_d_trans.l = address.l&0xFFFFE000;
  }
  else
  { octa last= address;
    int p = translate_address(&address, &data_tc);
    if (p&PAGE_TABLE_ERROR)
    { g[rQ].l |= PT_BIT;
      new_Q.l |= PT_BIT; 
      g[rF]=address; 
      return 0;
    }
    else if (!(p&WRITE_BIT)) 
    { g[rQ].h |= PW_BIT;
      new_Q.h |= PW_BIT;
      g[rF]=address; 
      return 0;
    }
    last_d_addr.h = last.h;
    last_d_addr.l = last.l&0xFFFFE000;
    last_d_trans.h=address.h;
    last_d_trans.l=address.l&0xFFFFE000;
  }
 if (address.h & 0xFFFF0000)
    store_uncached_data(size,data,address);
  else
     store_cached_data(size,data,address);
  return 1;
}


int load_data_uncached(int size, octa *data, octa address,int signextension)
/* size may be 1, 2, 4 or 8 
   load a byte, wyde, tetra, or octa into data from the given virtual address 
   raise an interrupt if there is a problem
*/

{ if (address.h==last_d_addr.h &&
      (address.l&0xFFFFE000)==last_d_addr.l)
      address.h=last_d_trans.h,
      address.l=last_d_trans.l|(address.l&0x1FFF);
  else if (address.h&0x80000000)
    /* negative addresses are maped to physical adresses by suppressing the sign bit */
  {  last_d_addr.h = address.h;
     last_d_addr.l = address.l&0xFFFFE000;
     address.h= address.h&0x7FFFFFFF;
     last_d_trans.h = address.h;
     last_d_trans.l = address.l&0xFFFFE000;
  }
  else 
  { octa last= address;
    int p = translate_address(&address, &data_tc);
    if (p&PAGE_TABLE_ERROR)
    { data->h=data->l = 0;
      g[rQ].l |= PT_BIT;
      new_Q.l |= PT_BIT; 
      g[rF]=address; 
      return 0;
    }
   else if (!(p&READ_BIT)) 
    { data->h=data->l = 0;
      g[rQ].h |= PR_BIT;
      new_Q.h |= PR_BIT;
      g[rF]=address; 
      return 0;
    }
    last_d_addr.h = last.h;
    last_d_addr.l = last.l&0xFFFFE000;
    last_d_trans.h=address.h;
    last_d_trans.l=address.l&0xFFFFE000;
  }
  load_uncached_data(size,data,address,signextension);
  return 1;
}


int store_data_uncached(int size,octa data, octa address)
/* store an octa from data into the given virtual address 
   raise an interrupt and return 0 if there is a problem
*/
{
  if (address.h==last_d_addr.h &&
      (address.l&0xFFFFE000)==last_d_addr.l)
      address.h=last_d_trans.h,
      address.l=last_d_trans.l|(address.l&0x1FFF);
  else if (address.h&0x80000000)
    /* negative addresses are maped to physical adresses by suppressing the sign bit */
  {  last_d_addr.h = address.h;
     last_d_addr.l = address.l&0xFFFFE000;
     address.h= address.h&0x7FFFFFFF;
     last_d_trans.h = address.h;
     last_d_trans.l = address.l&0xFFFFE000;
  }
  else
  { octa last= address;
    int p = translate_address(&address, &data_tc);
    if (p&PAGE_TABLE_ERROR)
    { g[rQ].l |= PT_BIT;
      new_Q.l |= PT_BIT; 
      g[rF]=address; 
      return 0;
    }
    else if (!(p&WRITE_BIT)) 
    { g[rQ].h |= PW_BIT;
      new_Q.h |= PW_BIT;
      g[rF]=address; 
      return 0;
    }
    last_d_addr.h = last.h;
    last_d_addr.l = last.l&0xFFFFE000;
    last_d_trans.h=address.h;
    last_d_trans.l=address.l&0xFFFFE000;
  }
  store_uncached_data(size,data,address);
  return 1;
}


void write_data(octa address,int size)
/* make sure size bytes starting at address are writen
   from the data cache to memory 
   no exceptions are raised if writing is not possible
*/
{ if (address.h&0x80000000)
    /* negative addresses are maped to physical adresses by suppressing the sign bit */
    address.h= address.h&0x7FFFFFFF;
  else
  { int p = translate_address(&address, &data_tc);
    if (p&PAGE_TABLE_ERROR ||p==0) 
      return;
  }
  if (address.h & 0xFFFF0000)
    return; /* these addresses are not cached */
  else
    write_data_cache(address, size);
}

void delete_data(octa address,int size)
     /* make sure size bytes starting at address are deleted
        from the data cache */
{ if (address.h&0x80000000)
    /* negative addresses are maped to physical adresses by suppressing the sign bit */
    address.h= address.h&0x7FFFFFFF;
  else
  { int p = translate_address(&address, &data_tc);
    if (p&PAGE_TABLE_ERROR ||p==0) 
      return;
  }
  if (address.h & 0xFFFF0000)
    return; /* these addresses are not cached */
  else
    clear_data_cache(address, size);
}

void delete_instruction(octa address,int size)
     /* make sure size bytes starting at address are deleted
        from the instruction cache */
{ if (address.h&0x80000000)
    /* negative addresses are maped to physical adresses by suppressing the sign bit */
    address.h= address.h&0x7FFFFFFF;
  else
  { int p = translate_address(&address, &data_tc);
    if (p&PAGE_TABLE_ERROR ||p==0) 
      return;
  }
  if (address.h & 0xFFFF0000)
    return; /* these addresses are not cached */
  else
    clear_instruction_cache(address, size);
}

void prego_instruction(octa address,int size)
     /* make sure size bytes starting at address are read into
        the instruction cache */
{ if (address.h&0x80000000)
    /* negative addresses are maped to physical adresses by suppressing the sign bit */
    address.h= address.h&0x7FFFFFFF;
  else
  { int p = translate_address(&address, &data_tc);
    if (p&PAGE_TABLE_ERROR || !(p&EXEC_BIT)) 
      return;
  }
  if (address.h & 0xFFFF0000)
    return; /* these addresses are not cached */
  else
    prego_instruction_cache(address, size);
}
