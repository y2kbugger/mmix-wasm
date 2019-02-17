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

	mspbus.c

	Provides the implementation for VMB bus connectivity
*/

#include "mspbus.h"
#include "mspcore.h"
#ifdef _DEBUG
#include "memcheck.h"
#endif

device_info vmb = {0};

static void vmb_atexit(void)
{ vmb_disconnect(&vmb);
  vmb_end();
}

// Translates the address in lower ram to extended ram,
// if translate_ram_to_upper is set
msp_word translate_address(msp_word address) {
	msp_word result;
	if (translate_ram_to_upper && (address.asWord >= 0x200) && (address.asWord <= 0x9FF))
		result.asWord = address.asWord + 0xF00;
	else
		result.asWord = address.asWord;
	return result;
}

void initVMBInterface() {
  vmb_begin();
  vmb_debug_flag = 0;
  vmb_program_name = "MSP430";
  vmb.reset=initCore;
  vmb_connect(&vmb,host,port); 
  vmb_register(&vmb,HI32(vmb_address),LO32(vmb_address),0,-1,-1,vmb_program_name,major_version,minor_version);
  atexit(vmb_atexit);
  vmb_debug_on();
}

int wait_for_power(void)
{
	fprintf(stderr,"Power...");
	while(!vmb.power) {
		vmb_wait_for_power(&vmb);
		if (!vmb.connected) return FALSE;
	}
	fprintf(stderr," ON\n");
	return TRUE;
}



int vmbReadByteAt(msp_word msp_address, UINT8* readInto) {
	data_address da = {0};
	vmb_init_data_address(&da, 1);
	da.address_hi = 0x0;
	da.address_lo = translate_address(msp_address).asWord;
	vmb_load(&vmb, &da);
	while (da.status != STATUS_VALID)
		vmb_wait_for_valid(&vmb, &da);
	*readInto = *(UINT8*)da.data;
	return TRUE;
}

int vmbReadWordAt(msp_word msp_address, UINT16* readInto) {
	data_address da = {0};
	vmb_init_data_address(&da, 2);
	da.address_hi = 0x0;
	da.address_lo = translate_address(msp_address).asWord;
	vmb_load(&vmb, &da);
	while (da.status != STATUS_VALID)
		vmb_wait_for_valid(&vmb, &da);
	*readInto = *(UINT16*)da.data;
	return TRUE;
}

int vmbWriteByteAt(msp_word msp_address, UINT8* writeFrom) {
	data_address da = {0};
	vmb_init_data_address(&da, 1);
	da.address_hi = 0x0;
	da.address_lo = translate_address(msp_address).asWord;
	da.data = (unsigned char*)writeFrom;
	vmb_store(&vmb, &da);
	return TRUE;
}

int vmbWriteWordAt(msp_word msp_address, UINT16* writeFrom) {
	data_address da = {0};
	vmb_init_data_address(&da, 2);
	da.address_hi = 0x0;
	da.address_lo = translate_address(msp_address).asWord;
	da.data = (unsigned char*)writeFrom;
	vmb_store(&vmb, &da);
	return TRUE;
}


