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
#ifdef WIN32
#include <windows.h>
extern HWND hMainWnd;
#endif
#include "vmb.h"


/* the next funtions are required callback functions
   for the vmb interface. They are called from threads
   distinct from the main thread. If these callbacks
   share resources with the main thread, it might be necessary
   to use a mutex to synchronize access to the resources.
   In this template, the main thread does nothing with
   the ram. Hence no synchronization is needed.
*/


void vmb_terminate(void)
/* this function is called when the motherboard politely asks the device to terminate.*/
{ 
#ifdef WIN32
   PostMessage(hMainWnd,WM_CLOSE,0,0);
#endif
}

