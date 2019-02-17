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

	mspshared.h

	Header file for MSP430 simulator configuration, shared variables and definitions.
*/


#ifndef MSP_SHARED
#define MSP_SHARED

#include "vmb.h"		// Datatypes & interfaces

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

#define WORD_MASK 0xFFFF
#define BYTE_MASK 0xFF

typedef union {
	// MSP is little endian!
	UINT16 asWord;
	struct {
#ifdef BIGENDIAN
		unsigned int RESERVED : 7;
		unsigned int V : 1;
		unsigned int SCG1 : 1;
		unsigned int SCG0 : 1;
		unsigned int OSCOFF : 1;
		unsigned int CPUOFF : 1;
		unsigned int GIE : 1;
		unsigned int N : 1;
		unsigned int Z : 1;
		unsigned int C : 1;
#else
		unsigned int C : 1;
		unsigned int Z : 1;
		unsigned int N : 1;
		unsigned int GIE : 1;
		unsigned int CPUOFF : 1;
		unsigned int OSCOFF : 1;
		unsigned int SCG0 : 1;
		unsigned int SCG1 : 1;
		unsigned int V : 1;
		unsigned int RESERVED : 7;
#endif
	} asBits;

	// Byte access
	struct {
#ifdef BIGENDIAN
		unsigned int hi : 8;
		unsigned int lo : 8;
#else
		unsigned int lo : 8;
		unsigned int hi : 8;
#endif
	} asBytes;

	// Opcode masks
	struct F1Mask {
#ifdef BIGENDIAN
		unsigned int opcode : 4;
		unsigned int sreg : 4;
		unsigned int ad : 1;
		unsigned int bflag : 1;
		unsigned int as : 2;
		unsigned int dreg : 4;
#else
		unsigned int dreg : 4;
		unsigned int as : 2;
		unsigned int bflag : 1;
		unsigned int ad : 1;
		unsigned int sreg : 4;
		unsigned int opcode : 4;
#endif
	} asF1Mask;

	struct F2Mask {
#ifdef BIGENDIAN
		unsigned int opcode : 9;
		unsigned int bflag : 1;
		unsigned int ad : 2;
		unsigned int dsreg : 4;
#else
		unsigned int dsreg : 4;
		unsigned int ad : 2;
		unsigned int bflag : 1;
		unsigned int opcode : 9;
#endif
	} asF2Mask;

	struct F3Mask {
#ifdef BIGENDIAN
		unsigned int opcode : 3;
		unsigned int condition : 3;
		signed int offset : 10;
#else
		signed int offset : 10;
		unsigned int condition : 3;
		unsigned int opcode : 3;
#endif
	} asF3Mask;
} msp_word;

// Configuration parameters and options
#include "option.h"		// configuration
#include "param.h"
extern char version[];
extern char howto[];
static int executionStartAddress = 0;
static int translate_ram_to_upper = 0;
static int interactiveMode = 1;

#endif