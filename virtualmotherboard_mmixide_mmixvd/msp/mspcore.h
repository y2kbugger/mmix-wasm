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

	mspcore.h

	Header file for MSP430 simulator core functions.
*/

#ifndef MSP_CORE
#define MSP_CORE
#include <stdlib.h>
#include <stdio.h>

#ifdef WIN32
#include <windows.h>
//HWND hMainWnd = NULL; /* there is no Window */
#endif
#include "mspbus.h"
#include "mspshared.h"

// Registers
enum msp_register {
	PC,SP,SR,CG1=2,CG2,
	R0=0,R1,R2,R3,R4,R5,R6,R7,R8,R9,R10,R11,R12,R13,R14,R15
};

// SR-Bits
enum status_bit {
	C=0,Z,N,GIE,CPU_OFF,OSC_OFF,SCG0,SCG1,V
};

// Address modes
enum addr_mode {
	ADDR_MODE_REGISTER,						// Register operands
	ADDR_MODE_INDEXED_SYMBOLIC_ABSOLUTE,	// Indexed: Register+X points to the operand (X is stored in next word)
											// Symbolic: register is PC
											// Absolute: register is SR/CG1, X is the absolute address
	ADDR_MODE_INDIRECT_REGISTER,			// Register points to the operand
	ADDR_MODE_INDIRECT_AUTOINC_IMMEDIATE	// Register points to the operand, register content is autoincremented
											// by 1 for .B-instructions, by 2 - for .W-instructions
											// Immediate: The register is PC (the following word contains the pointer)
};

// Condition constants for the jump instruction
enum jump_conditions {
	JNE = 0,
	JEQ,
	JNC,
	JC,
	JN,
	JGE,
	JL,
	JMP
};

// Instruction executor function definition
typedef int (*executorPtr)();

extern int ADD_executor();
extern int ADDC_executor();
extern int AND_executor();
extern int BIC_executor();
extern int BIS_executor();
extern int BIT_executor();
extern int CALL_executor();
extern int CMP_executor();
extern int DADD_executor();
extern int JUMP_executor();
extern int MOV_executor();
extern int PUSH_executor();
extern int RETI_executor();
extern int RRA_executor();
extern int RRC_executor();
extern int SUB_executor();
extern int SUBC_executor();
extern int SWPB_executor();
extern int SXT_executor();
extern int XOR_executor();

// Instruction codes
static const executorPtr INSTRUCTIONS[39] = 
{
	NULL,					// 0
	&JUMP_executor,		// 1
	NULL,					// 2
	NULL,					// 3
	&MOV_executor,		// 4
	&ADD_executor,		// 5
	&ADDC_executor,		// 6
	&SUBC_executor,		// 7
	&SUB_executor,		// 8
	&CMP_executor,		// 9
	&DADD_executor,		// 10
	&BIT_executor,		// 11
	&BIC_executor,		// 12
	&BIS_executor,		// 13
	&XOR_executor,		// 14
	&AND_executor,		// 15
	NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL, // 16-31
	&RRC_executor,		// 32
	&SWPB_executor,		// 33
	&RRA_executor,		// 34
	&SXT_executor,		// 35
	&PUSH_executor,		// 36
	&CALL_executor,		// 37
	&RETI_executor		// 38
};

static const char* INSTRUCTION_NAMES[39] = 
{
	"n.a.",					// 0
	"JMP",		// 1
	"n.a.",					// 2
	"n.a.",					// 3
	"MOV",		// 4
	"ADD",		// 5
	"ADDC",		// 6
	"SUBC",		// 7
	"SUB",		// 8
	"CMP",		// 9
	"DADD",		// 10
	"BIT",		// 11
	"BIC",		// 12
	"BIS",		// 13
	"XOR",		// 14
	"AND",		// 15
	"n.a.","n.a.","n.a.","n.a.","n.a.","n.a.","n.a.","n.a.","n.a.",
	"n.a.","n.a.","n.a.","n.a.","n.a.","n.a.","n.a.", // 16-31
	"RRC",		// 32
	"SWPB",		// 33
	"RRA",		// 34
	"SXT",		// 35
	"PUSH",		// 36
	"CALL",		// 37
	"RETI"		// 38
};

#define REGISTERS_COUNT 16
// MSP registers
static msp_word registers[REGISTERS_COUNT];

// Access status bits
#define cBit registers[SR].asBits.C
#define zBit registers[SR].asBits.Z
#define nBit registers[SR].asBits.N
#define vBit registers[SR].asBits.V

static UINT64 clocks = 0;

#define MEMORY_WRITEBACK_NO (-1)

// TBD: There is a possibillity to mount the bootloader ROM
// Execution start adress must be configurable

#define DEFAULT_EXECUTION_START_LOCATION 0xFFFE
// TBD: RAM configuration per config file

#define RAM_START_AT 0x200

#define MAX_BREAKPOINTS 16


#define IntToWordByPtr(from,dest) *(UINT16*)dest = (UINT16)(from & WORD_MASK);
#define IntToByteByPtr(from,dest) *(UINT8*)dest = (UINT8)(from & BYTE_MASK);

// Increases the programm counter by 2
extern void increasePC();

extern int decodeInstructionFormat(msp_word instruction, executorPtr *executor);
extern executorPtr findExecutor(UINT16 instructionCode);
extern msp_word fetchInstruction(msp_word msp_address);
extern int decodeF1Instruction(msp_word instruction, void **source, void **destination, int *isByte);
extern int decodeF2Instruction(msp_word instruction, void **operand, int *isByte);
int decodeF3Instruction(msp_word instruction, unsigned int *condition, int *offset);
extern void executeInstruction(unsigned int instructionCode);

extern void executionLoop();
extern void initCore(void);
extern void initRegisters();
extern int writeBack(UINT32 write_back_address, void *data, unsigned char size);
extern void checkCBit(UINT32 check, int asByte);

#endif

