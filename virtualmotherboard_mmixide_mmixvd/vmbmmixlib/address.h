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

#ifndef ADDRESS_H
#define ADDRESS_H




/* these instructions return 0 (false) if a page fault occurs
   they return 1 (true) otherwise even if a protection violation
   occurs in this case they will not alter memory nor deliver a value
   but raise an interrupt 
*/

extern int load_instruction(tetra *instruction, octa address);
/* load a tetra into data from the given virtual address 
   raise an interrupt if there is a problem and load 0.
*/

extern int load_data(int size, octa *data, octa address,int signextension);
/* load an octa into data from the given virtual address 
   raise an interrupt if there is a problem and load 0
*/
extern int load_data_uncached(int size, octa *data, octa address,int signextension);
/* load an octa into data from the given virtual address 
   raise an interrupt if there is a problem and load 0
*/

extern int store_data(int size,octa data, octa address);
/* store an octa into data to the given virtual address 
   raise an interrupt if there is a problem
*/

extern int store_data_uncached(int size,octa data, octa address);
/* store an octa into data to the given virtual address 
   raise an interrupt if there is a problem
*/

/* functions for the virtual translation cache */
extern void clear_all_data_vtc(void);
extern void clear_all_instruction_vtc(void);
extern octa update_vtc(octa key);
extern void store_data_translation(octa *virt, octa *phys);
extern void store_exec_translation(octa *virt, octa *phys);


/* functions to access the cache */
extern void write_data(octa address,int size);
extern void delete_data(octa address,int size);
extern void delete_instruction(octa address,int size);
extern void read_instruction(octa address,int size);
extern void prego_instruction(octa address,int size);

extern int valid_address(octa address);
/* return 1 if valid, zero otherwise */


#endif
