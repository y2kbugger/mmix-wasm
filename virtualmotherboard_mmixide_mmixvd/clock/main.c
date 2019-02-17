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

/* the files maintemplate and devicetemplate
   is a template for a simple device, containing just what is
   necessary. Everyting else was omited.
   Its a good starting pont for complex devices and a complete
   reference to the functions needed and provided by the
   vmb library.
*/
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include "vmb.h"
#include "option.h"
#include "param.h"
#include "bus-arith.h"

#define RAMSIZE 8

static unsigned char ram[RAMSIZE]= {0};

unsigned char *vmb_get_payload(unsigned int offset,int size)
{
  time_t t;
  t = time(NULL);
  inttochar(t,ram+4);
  return ram+offset;
}



int main(int argc, char *argv[])
{
 param_init(argc, argv);
 vmb_size = 8;
 vmb_debugs(0, "%s ",vmb_program_name);
 vmb_debugs(0, "%s ", version);
 vmb_debugs(0, "host: %s ",host);
 vmb_debugi(0, "port: %d ",port);
 close(0);
 vmb_debugi(0, "address hi: %x",vmb_address_hi);
 vmb_debugi(0, "address lo: %x",vmb_address_lo);
 vmb_debugi(0, "size: %x ",vmb_size);
 vmb_connect(host,port); 
 vmb_register(vmb_address_hi,vmb_address_lo,vmb_size,
               0, 0, vmb_program_name,major_version,minor_version);
 vmb_wait_for_disconnect();
 return 0;
}
