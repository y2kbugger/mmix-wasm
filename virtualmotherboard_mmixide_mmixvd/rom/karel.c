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
#pragma warning(disable : 4996)
extern HWND hMainWnd;
#include <io.h>
#else
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>

#include "bus-arith.h"
#include "option.h"
#include "param.h"
#include "vmb.h"
#include "inspect.h"



char version[]="$Revision: 1.1 $ $Date: 2014-01-23 14:48:45 $";

char howto[] =
"\n"
"The program will contact the motherboard at host:port\n"
"and register itself with the given satrt address.\n"
"Then, the program will simulate karel the robot.\n"
"\n"
;

#define MEMSIZE 8
static unsigned char mem[MEMSIZE];


/* Interface to the virtual motherboard */


unsigned char *karel_get_payload(unsigned int offset,int size)
{
    return mem+offset;
}


void karel_poweron(void)
{ memset(mem,0,MEMSIZE);
  mem_update(0,0,vmb_size);
#ifdef WIN32
   PostMessage(hMainWnd,WM_VMB_ON,0,0);
#endif
}


static int karel_read(unsigned int offset,int size,unsigned char *buf)
{ if (offset>vmb_size) return 0;
  if (offset+size>vmb_size) size =vmb_size-offset;
  memmove(buf,mem+offset,size);
  return size;
}

struct inspector_def inspector[2] = {
    /* name size get_mem address num_regs regs */
	{"Memory",0,karel_read,0,0,NULL},
	{0}
};


void init_device(device_info *vmb)
{  vmb_debugi(VMB_DEBUG_INFO, "address hi: %x",HI32(vmb_address));
   vmb_debugi(VMB_DEBUG_INFO, "address lo: %x",LO32(vmb_address));
   vmb->poweron=karel_poweron;
   vmb->poweroff=vmb_poweroff;
   vmb->disconnected=vmb_disconnected;
   vmb->reset=vmb_reset;
   vmb->terminate=vmb_terminate;
   vmb->get_payload=karel_get_payload;
   vmb_size=MEMSIZE;
   vmb_debugi(VMB_DEBUG_INFO, "size: %d",vmb_size);
   close(0);
}
