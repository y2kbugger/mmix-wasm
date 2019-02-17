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


	mspmul.h

	Header file for MSP430 multiplier
*/

#ifndef MSPMUL
#define MSPMUL

#include "mspmulshared.h"

#ifdef WIN32
#include <windows.h>
HWND hMainWnd = NULL; /* there is no Window */
#endif

enum register_names {
	MPY, 
	MPYS, 
	MAC, 
	MACS, 
	OP2, 
	RESLO, 
	RESHI, 
	SUMEXT
};

extern unsigned int mul_mode;
#define REGISTERS_COUNT 8
#define MEMSIZE REGISTERS_COUNT * sizeof(msp_word)
#endif