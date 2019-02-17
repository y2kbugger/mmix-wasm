/*
    Copyright 2008  Martin Ruckert
    
    ruckertm@acm.org

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

*/

/*  the files maintemplate and devicetemplate
   is a template for a simple device, containing just what is
   necessary. Everyting else was omited.
   Its a good starting pont for complex devices and a complete
   reference to the functions needed and provided by the
   vmb library.
*/

#include <string.h>

#ifdef WIN32
#include <windows.h>
extern HWND hMainWnd;
#endif

/* This device simulates RAMSIZE bytes of RAM */
#define RAMSIZE 1024
static unsigned char ram[RAMSIZE];
int ramsize = RAMSIZE;

static void ram_clean(void)
/* clean the ram, done after power on and reset */
{ int i;
  for (i=0;i<RAMSIZE;i++)
    ram[i] = 0;
}


/* the include file vmb.h contains all the prototypes for
   functions and variables needed to use the vmb library. */

#include "vmb.h"


/* the next funtions are required callback functions
   for the vmb interface. They are called from threads
   distinct from the main thread. If these callbacks
   share resources with the main thread, it might be necessary
   to use a mutex to synchronize access to the resources.
   In this template, the main thread does nothing with
   the ram. Hence no synchronization is needed.
*/

void vmb_poweron(void)
/* this function is called when the virtual power is turned on */
{  ram_clean();
#ifdef WIN32
   SendMessage(hMainWnd,WM_USER+1,0,0);
#endif
}

void vmb_poweroff(void)
/* this function is called when the virtual power is turned off */
{ /* do nothing */
#ifdef WIN32
   SendMessage(hMainWnd,WM_USER+2,0,0);
#endif
}

void vmb_disconnected(void)
/* this function is called when the reading thread disconnects from the virtual bus. */
{ /* do nothing */
#ifdef WIN32
   SendMessage(hMainWnd,WM_USER+4,0,0);
#endif
}
void vmb_reset(void)
/* this function is called when the virtual reset button is pressed */
{ ram_clean();
}

unsigned char *vmb_get_payload(unsigned int offset,int size)
/* this function is called if some other device on the virtual bus
   wants to read size byte from this device at the given offset.
   offset and size are checked to fall completely within the
   address space ocupied by this device
   The function must return a pointer to the requested bytes.
*/
{ return ram+offset;
}

void vmb_put_payload(unsigned int offset,int size, unsigned char *payload)
/* this function is called if some other device on the virtual bus
   wants to write size byte to this device at the given offset.
   The new byte are contained in the payload.
   offset and size are checked to fall completely within the
   address space ocupied by this device.
*/
{  memmove(ram+offset,payload,size);
}

