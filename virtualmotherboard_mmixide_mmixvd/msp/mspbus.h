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

	mspbus.h

	Header file for bus functions, providing the connectivity to virtual motherboard and
	msp430 peripherals.


	msp430 address space is 16 bit, ranging from 0h
	to 0FFFFh (20 bit implementations are not yet supported).
	
	Memory layout

	0x0000 - 0x0007: interrupt control registers
	0x0008 - 0x00FF: 8-bit peripheral (accessed by byte instructions)
	0x0100 - 0x01FF: 16-bit peripherals (accessed by wyde operations)
	0x0200 - 0x09FF: Up to 2K RAM are mapped here
	0x0C00 - 0x0FFF: bootstrap loader flash ROM (1K)
	0x1000 - 0x10FF: data flash ROM (256 bytes)
	0x1100 - 0x38FF: extended RAM (or a copy of lower RAM address space, 2K, upper border varies)
	0x1100 - 0xFFFF: Up to 60KB programm ROM, smaller ROMs start at higher addresses
	0xFFE0 - 0xFFFF: Interrupt vector table (16 words, might be 32 words)

	Depending on the simulated msp430 device and the peripherals must be configured
	accordingly. The read and write requests will be issued over the vmb bus.
	Offset address decoding must be provided by the peripheral devices.
*/

#ifndef MSP_BUS
#define MSP_BUS
#include "mspshared.h"

extern device_info vmb;

// Read and write access functions provide access to RAM, ROM and device memory
// The function block the execution till the byte/word was delivered by bus
extern int vmbReadByteAt(msp_word msp_address, UINT8* readInto);
extern int vmbReadWordAt(msp_word msp_address, UINT16* readInto);
extern int vmbWriteByteAt(msp_word msp_address, UINT8* writeFrom);
extern int vmbWriteWordAt(msp_word msp_address, UINT16* writeFrom);

extern void initVMBInterface();
extern void wait_for_disconnect();
extern void init_device(device_info *vmb);
extern void device_poweron(void);
extern void device_reset(void);
extern unsigned char *device_get_payload(unsigned int offset,int size);
extern void device_put_payload(unsigned int offset,int size, unsigned char *payload);
extern void vmb_atexit(void);
extern int wait_for_power(void);
#endif
