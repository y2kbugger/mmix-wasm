/*
    Copyright 2008 Martin Ruckert
    
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


/* this file provides default do-nothing implementations
   of required functions, when linkung with vmblib
*/


#include <stdlib.h>
#include "vmb.h"


void vmb_buserror(unsigned char type, unsigned char address[8])
/* this function is called if there are error on the bus
*/
{ /* ignore the message */ 
}


