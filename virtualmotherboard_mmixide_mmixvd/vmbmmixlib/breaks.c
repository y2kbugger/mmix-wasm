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
#include <stdio.h>
#include "mmix-internals.h"
#include "address.h"
#include "breaks.h"

#define HSIZE 211

typedef struct node {
  struct node *next;
  octa a;
  unsigned int b;
}  node;

static node *table[HSIZE] = {0};

#define hvalue(a) ((a.h + a.l)%HSIZE)
#define panic(x) {fprintf(stderr,"%s (%d) ERROR: %s\n",__FILE__,__LINE__,x);exit(1);}

static node *newnode( octa a, unsigned char b, node *next)
{ node *n;
  n = (node *)malloc(sizeof(*n));
  if (n==NULL) panic("Out of memory"); 
  n->a = a;
  n->b = b;
  n->next=next;
  return n;
}

unsigned char get_break(octa a)
{ node *n = table[hvalue(a)];
  while (n!=NULL)
    if (a.h==n->a.h && a.l==n->a.l)
      return n->b;
    else
      n=n->next;   
  return 0 ;
}

void set_break(octa a, unsigned char b)
{ int i = hvalue(a); 
  node **p = table+i;
  while (*p!=NULL)
    if (a.h==(*p)->a.h && a.l==(*p)->a.l)
    { if (b==0)
      { node *n=*p;
        *p=(*p)->next;
        free(n);
      }
      else
        (*p)->b = b;
      return;
    }
    else
      p=&((*p)->next);   
  table[i] = newnode(a,b,table[i]);
}

void show_breaks(void)
{ int i;
  for (i=0;i<HSIZE;i++)
  { node *n = table[i];
    while (n!=NULL) 
    {
      printf("  %08x%08x %c%c%c%c\n",n->a.h,n->a.l,
             n->b&trace_bit? 't': '-',
             n->b&read_bit? 'r': '-',
             n->b&write_bit? 'w': '-',
             n->b&exec_bit? 'x': '-');
      n=n->next;
    }
  }
}

