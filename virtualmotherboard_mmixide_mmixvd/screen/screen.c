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
#include <string.h>
#ifdef WIN32
#include <windows.h>
#include "resource.h"
#include "winopt.h"
#include "inspect.h"
extern HWND hMainWnd;
extern void setfont(void);
#else
#include <unistd.h>
/* make mem_update a no-op */
#define  mem_update(offset,size)
#endif

#include "bus-arith.h"
#include "option.h"
#include "param.h"
#include "vmb.h"

static void display_char(char c);
extern device_info vmb;
char title[] ="VMB Screen";

int major_version=2, minor_version=1;
char version[]="$Revision: 664 $ $Date: 2018-12-10 18:10:14 +0100 (Mon, 10 Dec 2018) $";

char howto[] =
"The program will contact the motherboard at [host:]port\r\n"
"and register itself with the given address and interrupt.\r\n"
"Then, the program will be ready to receive bytes and display them\r\n"
"by sending them to standard output.\r\n"
"Whenever ready to receive a byte, it will signal an interrupt with the\r\n"
"given interrupt number.\r\n"
"At the given address it will provide a read/write octabyte\r\n"
"in the following format:\r\n"
"\r\n"
"   XX00 00YY 0000 00ZZ\r\n"
"\r\n"
"The XX byte signals errors it will be 0x80 if an error occured\r\n"
"and 0x00 otherwise.\r\n"
"\r\n"
"The YY byte contains the number of characters that were written\r\n"
"to this address since the last output. This should be 0\r\n"
"if the address is ready to receive a character or 1 if the hardware\r\n"
"is bussy with writing a byte. Any other value will indicate that characters were\r\n"
"lost since the last write operation.\r\n"
"\r\n"
"The ZZ byte contains the last character received. It is valid only\r\n"
"if YY is not zero.\r\n"
"\r\n"
"The complete ocatbyte will be reset to zero after a byte is output.\r\n"
"\r\n"
;

static unsigned char data[8];
#define ERROR 0
#define COUNT 3
#define DATA  7



static void display_char(char c)
{ 
#ifdef WIN32	
   SendMessage(hMainWnd,WM_VMB_OTHER+2,(WPARAM)c,0);
#else
    printf("%c",c);
#endif
} 

/* Interface to the virtual motherboard */


unsigned char *screen_get_payload(unsigned int offset,int size)
{ return data+offset;
}

void screen_put_payload(unsigned int offset,int size, unsigned char *payload)
{   
    unsigned char cpData[8]; //!< to fix payload smaller then 8
    int i;

    if( offset + size < 8 )
    {
	 for(i=0;i<7;i++) //!< cleanout memory (just in case)
	   cpData[i] = 0;
	 cpData[DATA] = payload[size-1-offset]; //!< copy the char on the right place
	 payload = cpData;
    }
    if (data[COUNT]<0xFF) data[COUNT]++;
    else data[ERROR]=0x80;
    data[DATA] = payload[7-offset];
    mem_update(0,8);
    vmb_debugi(VMB_DEBUG_INFO, "(%02X)",data[DATA]);
    display_char(data[DATA]);
    memset(data,0,8);
    if (! disable_interrupt)
      vmb_raise_interrupt(&vmb,interrupt_no);
    mem_update(0,8);
}
#ifdef WIN32
struct register_def screen_regs[] = {
	/* name no offset size chunk format */
	{"Error" ,ERROR,1,byte_chunk,hex_format},
	{"Count" ,COUNT,1,byte_chunk,unsigned_format},
    {"Char"  ,DATA,1,byte_chunk,ascii_format},
    {"Char"  ,DATA,1,byte_chunk,hex_format},
	{0}};

int screen_reg_read(unsigned int offset, int size, unsigned char *buf)
{ if (offset>8) return 0;
  if (offset+size>8) size =8-offset;
  memmove(buf,data+offset,size);
  return size;
}
struct inspector_def inspector[2] = {
    /* name size get_mem address num_regs regs */
	{"Registers",5*8,screen_reg_read,screen_get_payload,screen_put_payload,0,0,8,4,screen_regs},
	{0}
};
#endif

void init_device(device_info *vmb)
{ vmb_debugi(VMB_DEBUG_INFO, "address hi: %x",HI32(vmb_address));
  vmb_debugi(VMB_DEBUG_INFO, "address lo: %x",LO32(vmb_address));
  vmb_debugi(VMB_DEBUG_INFO, "interrupt: %d",interrupt_no);
  vmb_size = 8;
#ifndef WIN32
   setvbuf(stdout,NULL,_IONBF,0); /* make ouput unbuffered */
#else
   setfont();
#endif
  vmb->poweron=vmb_poweron;
  vmb->poweroff=vmb_poweroff;
  vmb->reset = vmb_reset;
  vmb->disconnected=vmb_disconnected;
  vmb->terminate=vmb_terminate;
  vmb->put_payload=screen_put_payload;
  vmb->get_payload=screen_get_payload;
#ifdef WIN32
  inspector[0].address=vmb_address;
#endif
}
#ifdef WIN32
#else
device_info vmb = {0};
int main(int argc, char *argv[])
{
  param_init(argc, argv);
  vmb_debugs(VMB_DEBUG_INFO, "%s ",vmb_program_name);
  vmb_debugs(VMB_DEBUG_INFO, "%s ", version);
  vmb_debugs(VMB_DEBUG_INFO, "host: %s ",host);
  vmb_debugi(VMB_DEBUG_INFO, "port: %d ",port);
  close(0); /* stdin */  init_device(&vmb);
  init_device(&vmb);
  vmb_debugi(VMB_DEBUG_INFO, "address hi: %x",HI32(vmb_address));
  vmb_debugi(VMB_DEBUG_INFO, "address lo: %x",LO32(vmb_address));
  vmb_debugi(VMB_DEBUG_INFO, "size: %x ",vmb_size);
  
  vmb_connect(&vmb,host,port); 

  vmb_register(&vmb,HI32(vmb_address),LO32(vmb_address),vmb_size,
               0, 0, vmb_program_name,major_version,minor_version);

  vmb_wait_for_disconnect(&vmb);
  return 0;
}
#endif


