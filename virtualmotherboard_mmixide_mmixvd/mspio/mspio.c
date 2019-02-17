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



	mspio.c

	Provides the implementation for digital I/O for MSP430

*/
#include <string.h>
#include "mspio.h"
#include <Windows.h>

// Memory-mapped registers	
static unsigned char regs[MEMSIZE];
static int memsize = MEMSIZE;

char version[] = "1.0";
char howto[] = "Simulator for MSP430 digital i/o port\n";

// The address of the output device
uint64_t output_address = 0x0;

// vmb interface
static device_info vmb = {0};

static void mem_clean(void)
/* set the initial conditions */
{ int i;
  for (i=0;i<MEMSIZE;i++)
    regs[i] = 0;
}

void mio_poweron(void)
{  mem_clean();
}


void mio_reset(void)
{ 
	// Send on to all leds
	regs[REG_DIR] = -1;
	regs[REG_OUT] = 0;
	sendChanges((-1) & 0xFF);
	// Send off to all leds
	regs[REG_OUT] = (-1);
	sendChanges(0);
	// Set to reset conditions
	mem_clean();
}


unsigned char *mio_get_payload(unsigned int offset,int size)
{ 
	return (unsigned char*)&regs[offset];
}

/*
	Sends signals on state change
*/
void sendChanges(unsigned char payload) {
	unsigned char sendTo, signal;
	data_address da = {0};

	// If the state (level) of any output ports is changing, 
	// a signal will be sent to the corresponding device
	sendTo = (payload^regs[REG_OUT]) & regs[REG_DIR];
	if (sendTo) {
		regs[REG_OUT] = payload;
		//fprintf(stderr, "Sending signal to %4x\n",output_address);
		signal = (regs[REG_OUT] & sendTo);
		vmb_init_data_address(&da, 1);
		da.address_hi = HI32(output_address);
		da.address_lo = LO32(output_address);
		da.data = &signal;
		vmb_store(&vmb, &da);
	}
}

void mio_put_payload(unsigned int offset,int size, unsigned char *payload)
{  
	// Check if the write access is bytewise
	// and the read-only register is not accessed
	//fprintf(stderr, "Received packet for reg %d: 0x%4x\n",offset,*payload);
	if (size == 1 && offset != 0) {
		if (offset == REG_OUT) {
			sendChanges(*payload);
		} else {
			memmove(regs+offset,payload,size);
		}
	}
}

void initVMBInterface() {
	vmb_begin();
	param_init();
	vmb_debug_flag = 0;
	vmb_program_name = "Digital I/O for MSP430";
	vmb.poweron=mio_reset;
	vmb.reset=mio_reset;
	vmb.get_payload=mio_get_payload;
	vmb.put_payload=mio_put_payload;

	vmb_connect(&vmb,host,port); 
	vmb_register(&vmb,HI32(vmb_address),LO32(vmb_address),memsize,0,0,vmb_program_name,major_version,minor_version);
	atexit(vmb_atexit);
	vmb_debug_on();
}

static void vmb_atexit(void)
{ 
	vmb_disconnect(&vmb);
	vmb_end();
}

int wait_for_power(void)
{
	fprintf(stderr,"Power...");
	while(!vmb.power)
	{
		vmb_wait_for_power(&vmb);
		if (!vmb.connected) return FALSE;
	}
	fprintf(stderr,"ON\n");
	return TRUE;
}

int main(int argc, char *argv[])
{	
	initVMBInterface();
 boot:
	if (!wait_for_power())
		goto end_simulation;
	mio_reset();

	
	while (vmb.connected) {
		if (!vmb.power || vmb.reset_flag)
		{
		  vmb.reset_flag = 0;
		  goto boot;
		}
		fprintf(stderr, "VMB address: 0x%4x\n",vmb_address);
		fprintf(stderr, "Allocated registers: %d\n",memsize);
		fprintf(stderr, "Output to 0x%4x\n",output_address);
		
		vmb_wait_for_disconnect(&vmb);
	}
end_simulation:
	vmb_end();
	return 0;
}
