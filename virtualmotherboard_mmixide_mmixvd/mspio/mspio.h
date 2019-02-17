/*
    Copyright 2012 Wladimir Danilov
    
    w.danilov@googlemail.com

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


	mspio.h

	Header file for digital I/O
	Use INTERRUPTS directive to compile the I/O port as an
	interrupt port (P1 or P2).
*/

#ifndef MSPIO
#define MSPIO

#include "mspioshared.h"

#ifdef WIN32
#include <windows.h>
#endif

// device registers
enum reg_names {
	REG_IN,		// input buffer
	REG_OUT,	// output buffer
	REG_DIR,	// direction bits (0: input, 1: output)
#ifdef INTERRUPTS
	REG_INT_FLAG,			// interrupt flags
	REG_INT_EDGE_SELECT,	//  edge selection for interrupt generation (0: interrupt on low->high transition,
							// 1: interrupt on high->low transition)
	REG_INT_ENABLE,			// interrupt enable reg (1: enable interrupts)
#endif
	REG_PORT_SELECT			// port select register: (0: I/O used for pin, 1: peripheral used for pin)
};


// The memory mapped registers allocate 4/7 bytes
#ifdef INTERRUPTS
#define MEMSIZE 7
#else
#define MEMSIZE 4
#endif

// vmb reference
extern device_info vmb;

// The size of mapped memory (4 or 7 bytes)
extern int memsize;

// VMB interface functions
extern void initVMBInterface();
extern void wait_for_disconnect();
//extern void init_device(device_info *vmb);
extern void initVMBInterface();
extern void init_device(device_info *vmb);
extern void mio_poweron(void);
extern void mio_reset(void);
extern unsigned char *mio_get_payload(unsigned int offset,int size);
extern void mio_put_payload(unsigned int offset,int size, unsigned char *payload);
extern void sendChanges(unsigned char payload);
extern int wait_for_power(void);
extern void vmb_atexit(void);
#endif