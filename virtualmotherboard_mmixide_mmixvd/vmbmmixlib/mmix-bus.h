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
extern int get_interrupt(octa *data);
extern int wait_time(int ms);
extern void cancel_wait(void);
extern void load_uncached_data(int size, octa *data, octa address, int signextension);
extern void store_uncached_data(int size, octa data, octa address);
extern void load_cached_data(int size, octa *data, octa address, int signextension);
extern void store_cached_data(int size, octa data, octa address);
extern void write_data_cache(octa address, int size);
extern void clear_data_cache(octa address, int size);
extern void clear_instruction_cache(octa address, int size);
extern void prego_instruction_cache(octa address, int size);
extern void preload_data_cache(octa address, int size);
extern void load_cached_instruction(tetra *instruction, octa address);
extern void write_all_data_cache(void);
extern void clear_all_data_cache(void);
extern void clear_all_instruction_cache(void);
extern void vmb_atexit(void);
extern void init_mmix_bus(char *host, int port, char *name);
extern char localhost[];
extern int port; /* on which port to connect to the bus */
extern char *host; /* on which host to connect to the bus */
