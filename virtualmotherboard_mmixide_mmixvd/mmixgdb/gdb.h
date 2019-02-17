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

#ifndef GDB_H
#define GDB_H

#ifdef WIN32
#include <windows.h>
#endif


#include "signals.h"
#include "breaks.h"

/* this is taken from tm-mmix.h in gdb/config/mmix/ it must agree with the values given there
 */

#define NUM_REGS (32 + 1 + 256)
/*   Number of machine registers */

#define REGISTER_SIZE 8
/* the number of bytes per register */


#define	PBUFSIZ ((REGISTER_SIZE * NUM_REGS * 2) + 32)

/* special register numbers see regcache */
#define RET_REGNUM 4
#define PC_REGNUM 32
/* the framepointer is in $254 and the memory stackpointer is in $rO */
#define FP_REGNUM 34  
#define SP_REGNUM 10   
#define BB_REGNUM 7

/* This should be a 64 bit integer type */

#ifdef WIN32

typedef LONGLONG CORE_ADDR;

#else
typedef long long CORE_ADDR;
#endif

/*Some constants for register access*/
#define BYTE_PER_REGISTER 8
#define SPECIAL_REGISTERS 32

/* Functions from remote-utils.c */

extern int gdb_init(int port);
extern int putpkt (char *buf);
extern int getack(void);

extern char *get_free_buffer(void);
extern void put_free_buffer(char *buffer);
extern char *get_gdb_command(void);
extern void put_cmd_buffer(char *buffer);

extern int wait_for_continue(void);
extern void signal_continue(int r);
/* from mmix-sim.c */

extern int breakpoint;
extern bool stepping; /* should we pause after the next instruction? */

extern int gdb_signal; /* signal to send to GDB */
extern int gdbport; /* port for the remot gdb to connect, with default */
extern int mmgetchars(unsigned char *buf, int size, octa addr, int stop);

/* from gdb.c */
/* called from the simulator */
extern int interact_with_gdb(void);
/* called from remote utils */
extern void handle_gdb_command(char *buffer);
extern int async_gdb_command(char *gdb_command);
/* extensions to the GDB remote Commands for special cases */
#define STOPED    'T'
#define BAD       '!'
#define ACK       '+'
#define REJECT    '-'  
#define BREAK     ((char)3) 
#define IS_ASYNC(cmd) ((cmd)[0]==BREAK)

#endif
