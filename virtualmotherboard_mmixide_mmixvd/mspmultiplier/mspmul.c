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


	mspmul.c

	Implementation of MSP430 multiplier
*/

#include <string.h>
#include "mspmulshared.h"
#include "mspmul.h"
#include "mspshared.h"

char version[] = "1.0";
char howto[] = "Hardware multiplier for MSP430\n";

unsigned int mul_mode = -1;
int isByteOp1 = FALSE;
int isByteOp2 = FALSE;
#define setB1(b) isByteOp1 = b;
#define setB2(b) isByteOp2 = b;

static msp_word registers[REGISTERS_COUNT];
int memsize = MEMSIZE;

void invokeMul() {
	INT64 iResult;
	INT32 iOP1, iOP2;		// for sign extension
	INT32 compNegative = 0x80000000;

	if (isByteOp1)
		iOP1 = (signed char)registers[mul_mode].asBytes.lo;
	else
		iOP1 = (INT16)registers[mul_mode].asWord;
	if (isByteOp2)
		iOP2 = (signed char)registers[OP2].asBytes.lo;
	else
		iOP2 = (INT16)registers[OP2].asWord;

	iResult = iOP1 * iOP2;

	switch (mul_mode) {
	case MPY:
		registers[RESLO].asWord = iResult & WORD_MASK;
		registers[RESHI].asWord = (iResult & 0xFFFF0000)>>16;
		registers[SUMEXT].asWord = 0;
		break;
	case MPYS:
		registers[RESLO].asWord = iResult & WORD_MASK;
		registers[RESHI].asWord = (iResult & 0xFFFF0000)>>16;
		if (iResult & compNegative)		// Sign
			registers[SUMEXT].asWord = -1;
		else
			registers[SUMEXT].asWord = 0;
		break;
	case MAC:
		iResult += (registers[RESHI].asWord<<16)|registers[RESLO].asWord;
		registers[RESLO].asWord = iResult & WORD_MASK;
		registers[RESHI].asWord = (iResult & 0xFFFF0000)>>16;
		if (iResult & ~0xFFFFFFFF)			// Carry
			registers[SUMEXT].asWord = -1;		
		else
			registers[SUMEXT].asWord = 0;
		break;
	case MACS:
		iResult += (registers[RESHI].asWord<<16)|registers[RESLO].asWord;
		registers[RESLO].asWord = iResult & WORD_MASK;
		registers[RESHI].asWord = (iResult & 0xFFFF0000)>>16;
		if (iResult & compNegative)			// Sign
			registers[SUMEXT].asWord = -1;		
		else
			registers[SUMEXT].asWord = 0;
		break;
	default:
		// Error
		return;
	}
}

static void mem_clean(void)
{ 
	memset(&registers,0,MEMSIZE);
}

void device_poweron(void)
{  mem_clean();
}


void device_reset(void)
{ mem_clean();
}


unsigned char *device_get_payload(unsigned int offset,int size) { 
	if (size > 0 && (size+offset <= MEMSIZE))
		return (unsigned char*)&registers+offset;
	else
		return NULL;
}

__inline void copyToRegister(unsigned char *payload, int regNumber, int regOffset, int size) {
	if (regOffset) {
		if (size == 1)
			registers[regNumber].asBytes.hi = *payload;
	} else {
		if (size == 1) {
			registers[regNumber].asBytes.lo = *payload;
			registers[regNumber].asBytes.hi = 0;
		} else {
			registers[regNumber].asWord = *(UINT16*)payload;
		}
	}
}

void device_put_payload(unsigned int offset,int size, unsigned char *payload) {  
	int regNum, regOffset;
	
	if (offset + size > MEMSIZE)
		return;

	regNum = offset/sizeof(msp_word);
	regOffset = offset % sizeof(msp_word);

	switch (regNum) {
	case MPY:
		mul_mode = regNum;
		copyToRegister(payload, regNum, regOffset, size);
		setB1(size == 1);
		break;
	case MPYS:
		mul_mode = regNum;
		copyToRegister(payload, regNum, regOffset, size);
		setB1(size == 1);
		break;
	case MAC:
		mul_mode = regNum;
		copyToRegister(payload, regNum, regOffset, size);
		setB1(size == 1);
		break;
	case MACS:
		mul_mode = regNum;
		copyToRegister(payload, regNum, regOffset, size);
		setB1(size == 1);
		break;
	case OP2:
		copyToRegister(payload, regNum, regOffset, size);
		setB2(size == 1);
		invokeMul();
		break;
	case RESLO:
		copyToRegister(payload, regNum, regOffset, size);
		break;
	case RESHI:
		copyToRegister(payload, regNum, regOffset, size);
		break;
	case SUMEXT:
		// Read-only!
		break;
	}
}


void init_device(device_info *vmb)
{	vmb->poweron=device_poweron;
	vmb->poweroff=vmb_poweroff; /* use default */
	vmb->disconnected=vmb_disconnected;  /* use default */
	vmb->reset=device_reset;
	vmb->terminate=vmb_terminate; /* use default */
	vmb->get_payload=device_get_payload;
	vmb->put_payload=device_put_payload;
}

device_info vmb = {0};

static void vmb_atexit(void)
{ vmb_disconnect(&vmb);
  vmb_end();
}

int main(int argc, char *argv[])
{
	param_init();
	vmb_begin();
	vmb_debug_flag = 0;
	vmb_program_name = "Multiplier for MSP430";
	init_device(&vmb);
	vmb_connect(&vmb,host,port); 
	vmb_register(&vmb,HI32(vmb_address),LO32(vmb_address),memsize,0,0,vmb_program_name,major_version,minor_version);
	atexit(vmb_atexit);
	vmb_debug_on();
	
	vmb_wait_for_disconnect(&vmb);
	return 0;
}
