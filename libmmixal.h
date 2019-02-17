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

#ifndef LIBMMIXAL_H
#define LIBMMIXAL_H
#include "mmixlib.h"
/* types and definitions from mmixal */

typedef struct sym_tab_struct{
  int file_no;
  int line_no;
int serial;
struct sym_tab_struct*link;
octa equiv;
}sym_node;


typedef struct ternary_trie_struct{
unsigned short ch;
struct ternary_trie_struct*left,*mid,*right;
struct sym_tab_struct*sym;
}trie_node;

extern trie_node *trie_search(trie_node *t, char *s);

//extern file_node file_info[256];

typedef struct{
Char*name;
short code;
int bits;
}op_spec;


extern trie_node *trie_root; 
//extern sym_node*sym_avail;
#define DEFINED (sym_node*)1     /* link value for octabyte equivalents */
#define REGISTER (sym_node*) 2   /* link value for register equivalents */


#endif
