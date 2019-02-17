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

	mspcore.c

	Provides the implementation of MSP430 simulator core
*/

#include "mspcore.h"
#ifdef _DEBUG
#include "memcheck.h"
#endif

// Version parameters
char version[] = "1.0";
char howto[] = "MSP430 Simulator for VMB\n";
static msp_word breakpoints[MAX_BREAKPOINTS] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
//static int interactiveMode = TRUE;
static msp_word currentInstruction = {0};
static INT32 memoryWriteBack = MEMORY_WRITEBACK_NO;
unsigned int compNegative = 0x80;
unsigned int compCarryByte = ~BYTE_MASK;
unsigned int compCarryWord = ~WORD_MASK;
int x = FALSE;		// execution flag

__inline void freeConditional(void * mem) {
	if ((mem < (void*)&registers[0]) || (mem >= (void*)&registers[REGISTERS_COUNT+1])) {
		free(mem);
	}
}

int ADD_executor () {
	int isByteInstruction;
	void *source;
	void *destination;
	
	UINT32 result = 0;
	UINT16 uiSource, uiDestination;

	if (!decodeF1Instruction(currentInstruction, &source, &destination, &isByteInstruction))
		return FALSE;
	// Execute
	if (isByteInstruction) {
		uiSource = *(UINT8*)source;
		uiDestination = *(UINT8*)destination;
		result = uiSource + uiDestination;
		IntToByteByPtr(result,destination);
	} else {
		uiSource = *(UINT16*)source;
		uiDestination = *(UINT16*)destination;
		result = uiSource + uiDestination;
		IntToWordByPtr(result,destination);
	}
	// (N-bit)
	if (!isByteInstruction) compNegative <<= 8;
	nBit = ((result & compNegative) != 0);

	// (Z-bit)
	zBit = (!(result & WORD_MASK));
	
	// C-Bit
	checkCBit(result, isByteInstruction);
	
	// (V-bit)
	vBit = (((!(uiSource & compNegative)) && (!(uiDestination & compNegative)) && (nBit)) 
		|| ((uiSource & compNegative) && (uiDestination & compNegative) && (!nBit)));

	if (!writeBack(memoryWriteBack, destination, (!isByteInstruction)+1))
		return FALSE;
	freeConditional(source);
	freeConditional(destination);

	// PC to next word
	increasePC();
	clocks++;
	return TRUE;
}

int ADDC_executor () {
	int isByteInstruction;
	void *source, *destination;
	UINT32 result;
	UINT16 uiSource, uiDestination;
	
	if (!decodeF1Instruction(currentInstruction, &source, &destination, &isByteInstruction))
		return FALSE;

	// Execute
	if (isByteInstruction) {
		uiSource = (*(UINT8*)source);
		uiDestination = (*(UINT8*)destination);
		result = uiSource + uiDestination + cBit;
		IntToByteByPtr(result,destination);
	} else {
		uiSource = (*(UINT16*)source);
		uiDestination = (*(UINT16*)destination);
		result = uiSource + uiDestination + cBit;
		IntToWordByPtr(result,destination);
	}
	// (N-bit)
	if (!isByteInstruction) compNegative <<= 8;
	nBit = (result & compNegative);
	// (Z-bit)
	zBit= (!(result & WORD_MASK));
	// (C-bit)
	checkCBit(result, isByteInstruction);
	
	// (V-bit)
	vBit= ((!(uiSource & compNegative) && !(uiDestination & compNegative) && (nBit)) 
	     || ((uiSource & compNegative) && (uiDestination & compNegative) && (!nBit)));

	if (!writeBack(memoryWriteBack, destination, (!isByteInstruction)+1))
		return FALSE;
	freeConditional(source);
	freeConditional(destination);

	// PC to next word
	increasePC();
	clocks++;
	return TRUE;
}

int AND_executor () {
	int isByteInstruction = FALSE;
	void *source;
	void *destination;
	UINT16 result;
	UINT16 uiSource, uiDestination;

	if (!decodeF1Instruction(currentInstruction, &source, &destination, &isByteInstruction))
		return FALSE;

	// Execute
	if (isByteInstruction) {
		uiSource = (*(UINT8*)source);
		uiDestination = (*(UINT8*)destination);
		result = uiSource & uiDestination;
		IntToByteByPtr(result,destination);
	} else {
		uiSource = (*(UINT16*)source);
		uiDestination = (*(UINT16*)destination);
		result = uiSource & uiDestination;
		IntToWordByPtr(result,destination);
	}
	// (N-bit)
	if (!isByteInstruction) compNegative <<= 8;
	nBit = (result & compNegative);

	// (Z-bit)
	zBit = (!(result & WORD_MASK));

	// Carry bit is set, if result != ZERO
	cBit = !zBit;
	
	// V-bit: always reset
	vBit = 0;

	if (!writeBack(memoryWriteBack, destination, (!isByteInstruction)+1))
		return FALSE;
	freeConditional(source);
	freeConditional(destination);

	// PC to next word
	increasePC();
	clocks++;
	return TRUE;
}

int BIC_executor () {
	int isByteInstruction = FALSE;
	void *source;
	void *destination;
	UINT16 result;
	UINT16 uiSource, uiDestination;

	if (!decodeF1Instruction(currentInstruction, &source, &destination, &isByteInstruction))
		return FALSE;

	// Execute
	if (isByteInstruction) {
		uiSource = (*(UINT8*)source);
		uiDestination = (*(UINT8*)destination);
		result = (~uiSource) & uiDestination;
		IntToByteByPtr(result,destination);
	} else {
		uiSource = (*(UINT16*)source);
		uiDestination = (*(UINT16*)destination);
		result = (~uiSource) & uiDestination;
		IntToWordByPtr(result,destination);
	}
	
	// Status bits are not affected
	if (!writeBack(memoryWriteBack, destination, (!isByteInstruction)+1))
		return FALSE;
	freeConditional(source);
	freeConditional(destination);

	// PC to next word
	increasePC();
	clocks++;
	return TRUE;
}



int BIS_executor () {
	int isByteInstruction = FALSE;
	void *source;
	void *destination;
	UINT16 result;
	UINT16 uiSource, uiDestination;

	if (!decodeF1Instruction(currentInstruction, &source, &destination, &isByteInstruction))
		return FALSE;

	// Execute
	if (isByteInstruction) {
		uiSource = (*(UINT8*)source);
		uiDestination = (*(UINT8*)destination);
		result = uiSource | uiDestination;
		IntToByteByPtr(result,destination);
	} else {
		uiSource = (*(UINT16*)source);
		uiDestination = (*(UINT16*)destination);
		result = uiSource | uiDestination;
		IntToWordByPtr(result,destination);
	}
	
	// Status bits are not affected

	if (!writeBack(memoryWriteBack, destination, (!isByteInstruction)+1))
		return FALSE;
	freeConditional(source);
	freeConditional(destination);

	// PC to next word
	increasePC();
	clocks++;
	return TRUE;
}

int BIT_executor () {
	int isByteInstruction = FALSE;
	void *source;
	void *destination;
	UINT16 result;
	UINT16 uiSource, uiDestination;

	if (!decodeF1Instruction(currentInstruction, &source, &destination, &isByteInstruction))
		return FALSE;

	// Execute
	if (isByteInstruction) {
		uiSource = (*(UINT8*)source);
		uiDestination = (*(UINT8*)destination);
		result = uiSource & uiDestination;
		IntToByteByPtr(result,destination);
	} else {
		uiSource = (*(UINT16*)source);
		uiDestination = (*(UINT16*)destination);
		result = uiSource & uiDestination;
		IntToWordByPtr(result,destination);
	}
	
	// Only status bits affected
	// (N-bit)
	if (!isByteInstruction) compNegative <<= 8;
	nBit = (result & compNegative);

	// (Z-bit)
	zBit = (!(result & WORD_MASK));

	// Carry bit is set, if result != ZERO
	cBit = !zBit;
	
	// V-bit: always reset
	vBit = FALSE;

	if (!writeBack(memoryWriteBack, destination, (!isByteInstruction)+1))
		return FALSE;
	freeConditional(source);
	freeConditional(destination);

	// PC to next word
	increasePC();
	clocks++;
	return TRUE;
}

int CALL_executor () {
	/*
		Decreases the stack pointer, pushes the PC (or rather the address of the next
		instruction) to the stack and overwrites the programm counter with the
		operand.
	*/
	void *operand;
	int isByteInstruction;
	UINT16 operandValue;
	UINT16 pcForStack;

	if (!decodeF2Instruction(currentInstruction, &operand, &isByteInstruction))
		return FALSE;

	operandValue = *(UINT16*)operand;
	registers[SP].asWord -= 2;
	pcForStack = registers[PC].asWord + 2;
	clocks++;
	if (!vmbWriteWordAt(registers[SP], &pcForStack))
		return FALSE;
	else
		clocks++;
	registers[PC].asWord = operandValue;
	clocks++;
	freeConditional(operand);
	clocks++;
	return TRUE;
}

int CMP_executor () {
	int isByteInstruction = FALSE;
	void *source;
	void *destination;
	unsigned int result;
	UINT16 uiSource, uiDestination;

	if (!decodeF1Instruction(currentInstruction, &source, &destination, &isByteInstruction))
		return FALSE;

	// Execute
	if (isByteInstruction) {
		uiSource = (*(UINT8*)source);
		uiDestination = (*(UINT8*)destination);
		result = uiDestination - uiSource;
		IntToByteByPtr(result,destination);
	} else {
		uiSource = (*(UINT16*)source);
		uiDestination = (*(UINT16*)destination);
		result = uiDestination - uiSource;
		IntToWordByPtr(result,destination);
	}
	
	// Only status bits affected
	// (N-bit)
	if (!isByteInstruction) compNegative <<= 8;
	nBit = (result & compNegative);

	// (Z-bit)
	zBit = (!(result & WORD_MASK));

	// C-bit
	checkCBit(result, isByteInstruction);

	
	// (V-bit)
	vBit = ((!(uiSource & compNegative) && !(uiDestination & compNegative) && (nBit)) 
		|| ((uiSource & compNegative) && (uiDestination & compNegative) && (!nBit)));

	if (!writeBack(memoryWriteBack, destination, (!isByteInstruction)+1))
		return FALSE;
	freeConditional(source);
	freeConditional(destination);

	// PC to next word
	increasePC();
	clocks++;
	return TRUE;
}

int DADD_executor () {
	int isByteInstruction = FALSE;
	void *source;
	void *destination;
	unsigned int result;
	UINT16 uiSource, uiDestination;
	UINT16 uiDigit, uiDigitMask = 0xF;
	int carry, i, maxI;

	if (!decodeF1Instruction(currentInstruction, &source, &destination, &isByteInstruction))
		return FALSE;

	// Execute
	if (isByteInstruction) {
		maxI = 2;
		uiSource = (*(UINT8*)source);
		uiDestination = (*(UINT8*)destination);
	}
	else {
		maxI = 4;
		uiSource = (*(UINT16*)source);
		uiDestination = (*(UINT16*)destination);
	}

	i = 0;
	result = 0;
	carry = cBit;
	while (i < maxI) {
		uiDigit = uiSource & uiDigitMask + uiDestination & uiDigitMask;
		if (carry) {
			uiDigit += 1;
			carry = FALSE;
		}
		if (uiDigit > 9) {
			carry = TRUE;
			uiDigit = uiDigit % 10;
		}
		else
			carry = FALSE;
		result |= (uiDigit << (i*4));
		uiDigitMask <<= 4;
		i++;
	}
		
	if (isByteInstruction) {
		IntToByteByPtr(result,destination);
	} else {
		IntToWordByPtr(result,destination);
	}

	// (N-bit)
	if (!isByteInstruction) compNegative <<= 8;
	nBit = (result & compNegative) != 0;

	// (Z-bit)
	zBit = (!(result & WORD_MASK));

	// (C-bit)
	cBit = carry;
	
	// (V-bit undefined)
	vBit = 0;

	if (!writeBack(memoryWriteBack, destination, (!isByteInstruction)+1))
		return FALSE;
	freeConditional(source);
	freeConditional(destination);

	// PC to next word
	increasePC();
	clocks++;
	return TRUE;
}

int JUMP_executor () {
	// Jumps to address PC+2*offset (offset is signed!)
	//void *condition;
	unsigned int uiCondition;
	//void *offset;
	int iOffset;
	int jump = FALSE;

	if (!decodeF3Instruction(currentInstruction, &uiCondition, &iOffset))
		return FALSE;

	//uiCondition = *(unsigned int*)condition;
	//iOffset = *(INT16*)offset;

	switch (uiCondition) {
	case JNE:
		if (!zBit)
			jump = TRUE;
		break;
	case JEQ:
		if (zBit)
			jump = TRUE;
		break;
	case JNC:
		if (!cBit)
			jump = TRUE;
		break;
	case JC:
		if (cBit)
			jump = TRUE;
		break;
	case JN:
		if (nBit)
			jump = TRUE;
		break;
	case JGE:
		if (!(nBit ^ vBit))
			jump = TRUE;
		break;
	case JL:
		if (nBit ^ vBit)
			jump = TRUE;
		break;
	case JMP:
		jump = TRUE;
		break;
	default:
		break;
	}

	increasePC();
	if (jump) {		
		registers[PC].asWord += 2*iOffset;
		clocks++;
	}

	clocks++;
	return TRUE;
}

int MOV_executor () {
	int isByteInstruction = FALSE;
	void *source;
	void *destination;

	if (!decodeF1Instruction(currentInstruction, &source, &destination, &isByteInstruction))
		return FALSE;

	// Execute
	if (isByteInstruction)
		*(UINT8*)destination = *(UINT8*)source;
	else
		*(UINT16*)destination = *(UINT16*)source;

	// Status bits are not affected

	if (!writeBack(memoryWriteBack, destination, (!isByteInstruction)+1))
		return FALSE;
	freeConditional(source);
	freeConditional(destination);
	// PC to next word
	increasePC();
	clocks++;
	return TRUE;
}

int PUSH_executor () {
	/*
		Decreases stack pointer by 2 and pushes a byte or word to the stack.
	*/
	void *operand = NULL;
	UINT16 operandValue = 0;
	int isByteInstruction = FALSE;

	if (!decodeF2Instruction(currentInstruction, &operand, &isByteInstruction))
		return FALSE;
	
	// Update stack pointer
	registers[SP].asWord -= 2;
	clocks++;

	if (isByteInstruction) {
		operandValue = *(UINT8*)operand;
	} else {
		operandValue = *(UINT16*)operand;
	}

	// Status bits are not affected

	if (!vmbWriteWordAt(registers[SP], &operandValue))
		return FALSE;
	else
		clocks++;
	freeConditional(operand);

	increasePC();
	clocks++;
	return TRUE;
}

int RETI_executor () {
	/*
		Return from interrupt:
		Word @TOS is moved to SR, stack pointer decreased by 2, @TOS is moved to PC,
		stack pointer decreased by 2.
	*/
	void *operand;
	int isByteInstruction = FALSE;

	if (!decodeF2Instruction(currentInstruction, &operand, &isByteInstruction))
		return FALSE;

	// Restore status register
	if (!vmbReadWordAt(registers[SP], &registers[SR].asWord))
		return FALSE;
	else
		clocks++;

	registers[SP].asWord -= 2;
	clocks++;
	// Restore programm counter
	if (!vmbReadWordAt(registers[SP], &registers[PC].asWord))
		return FALSE;
	else
		clocks++;
	registers[SP].asWord -= 2;
	clocks++;
	clocks++;
	return TRUE;
}

int RRA_executor () {
	/*
		Rotates the operand to the right (arithmetically):
		MSB->MSB, MSB->MSB-1,...,LSB+1->LSB,LSB->C
	*/
	int bC = FALSE;
	int msb = FALSE;
	int isByteInstruction = FALSE;
	void *operand;

	if (!decodeF2Instruction(currentInstruction, &operand, &isByteInstruction))
		return FALSE;
	
	if (isByteInstruction) {
		UINT8 operandValue;
		operandValue = *(UINT8*)operand;
		
		nBit = (operandValue & 0x80);
		msb = nBit;
		zBit = (!operandValue);
		
		cBit = ((operandValue & 0x1) > 0);

		operandValue >>= 1;
		if (msb)
			operandValue |= 0x80;
		*(UINT8*)operand = operandValue;
	} else {
		UINT16 operandValue;
		operandValue = *(UINT16*)operand;
		nBit = (operandValue & 0x8000);
		msb = nBit;
		cBit = (operandValue & 0x1);
		zBit = (!operandValue);

		operandValue >>= 1;
		if (msb)
			operandValue |= 0x8000;
		*(UINT16*)operand = operandValue;
	}

	vBit = 0;

	if (!writeBack(memoryWriteBack, operand, (!isByteInstruction)+1))
		return FALSE;
	freeConditional(operand);

	increasePC();
	clocks++;
	return TRUE;
}

int RRC_executor () {
	/*
		Rotates the operand right with carry:
		C->MSB, ..., LSB->C
	*/
	int lsb = FALSE;
	int isByteInstruction = FALSE;
	void *operand;

	if (!decodeF2Instruction(currentInstruction, &operand, &isByteInstruction))
		return FALSE;

	if (isByteInstruction) {
		UINT8 operandValue;
		operandValue = *(UINT8*)operand;
		if ((operandValue & 0x1) > 0)
			lsb = TRUE;

		operandValue >>= 1;
		if (cBit)
			operandValue |= 0x80;
		nBit = cBit;
		cBit = lsb;
		zBit = (!operandValue);
		*(UINT8*)operand = operandValue;
	} else {
		UINT16 operandValue;
		operandValue = *(UINT16*)operand;
		lsb = ((operandValue & 0x1) > 0);

		operandValue >>= 1;
		if (cBit)
			operandValue |= 0x8000;
		nBit = cBit;
		cBit = lsb;
		zBit = (!operandValue);
		*(UINT16*)operand = operandValue;
	}

	vBit = 0;

	if (!writeBack(memoryWriteBack, operand, (!isByteInstruction)+1))
		return FALSE;
	freeConditional(operand);

	increasePC();
	clocks++;
	return TRUE;
}

int SUB_executor () {
	int isByteInstruction = FALSE;
	void *source;
	void *destination;
	UINT16 result = 0;
	UINT16 uiSource, uiDestination;

	if (!decodeF1Instruction(currentInstruction, &source, &destination, &isByteInstruction))
		return FALSE;

	// Execute
	if (isByteInstruction) {
		uiSource = (*(UINT8*)source);
		uiDestination = (*(UINT8*)destination);
		result =  uiDestination - uiSource;
		IntToByteByPtr(result,destination);
	} else {
		uiSource = (*(UINT16*)source);
		uiDestination = (*(UINT16*)destination);
		result =  uiDestination - uiSource;
		IntToWordByPtr(result,destination);
	}

	// (N-bit)
	if (!isByteInstruction) compNegative <<= 8;
	nBit = ((result & compNegative) > 0);

	// (Z-bit)
	zBit = ((result & WORD_MASK) == 0);

	// (C-bit)
	checkCBit(result, isByteInstruction);

	
	// (V-bit)
	vBit = (((uiSource & compNegative) && !(uiDestination & compNegative) && (nBit)) // pos - neg = neg?
		|| (!(uiSource & compNegative) && (uiDestination & compNegative) && (!nBit)));		// neg - pos = pos?

	if (!writeBack(memoryWriteBack, destination, (!isByteInstruction)+1))
		return FALSE;
	freeConditional(source);
	freeConditional(destination);
	// PC to next word
	increasePC();
	clocks++;
	return TRUE;
}

int SUBC_executor () {
	int isByteInstruction = FALSE;
	void *source;
	void *destination;
	UINT16 result;
	UINT16 uiSource, uiDestination;

	if (!decodeF1Instruction(currentInstruction, &source, &destination, &isByteInstruction))
		return FALSE;

	// Execute
	if (isByteInstruction) {
		uiSource = (*(UINT8*)source);
		uiDestination = (*(UINT8*)destination);
		result =  uiDestination - uiSource - 1 + cBit;
		IntToByteByPtr(result,destination);
	} else {
		uiSource = (*(UINT16*)source);
		uiDestination = (*(UINT16*)destination);
		result =  uiDestination - uiSource - 1 + cBit;
		*(UINT16*)destination = (UINT16)(result & BYTE_MASK);
	}

	// (N-bit)
	if (!isByteInstruction) compNegative <<= 8;
	nBit = (result & compNegative);

	// (Z-bit)
	zBit = (!(result & WORD_MASK));

	// (C-bit)
	checkCBit(result, isByteInstruction);
	
	// (V-bit)
	vBit = (((uiSource & compNegative) && !(uiDestination & compNegative) && (nBit))	// pos - neg = neg
		|| (!(uiSource & compNegative) && (uiDestination & compNegative) && (!nBit)));	// neg - pos = pos

	if (!writeBack(memoryWriteBack, destination, (!isByteInstruction)+1))
		return FALSE;
	freeConditional(source);
	freeConditional(destination);
	// PC to next word
	increasePC();
	clocks++;
	return TRUE;
}

int SWPB_executor () {
	/*
		Swaps the both bytes of the operand
	*/
	UINT8 tmp;
	int isByteInstruction = FALSE;
	void *operand;
	UINT16 operandValue;


	if (!decodeF2Instruction(currentInstruction, &operand, &isByteInstruction))
		return FALSE;

	operandValue = *(UINT16*)operand;
	tmp = (operandValue & 0xFF00)>>8;
	operandValue <<= 8;
	operandValue |= tmp;
	*(UINT16*)operand = operandValue;

	if (!writeBack(memoryWriteBack, operand, 2))
		return FALSE;
	freeConditional(operand);

	increasePC();
	clocks++;
	return TRUE;
}

int SXT_executor () {
	/*
		Extends the sign of the lower byte to the higher byte
	*/
	int isByteInstruction = FALSE;
	void *operand;
	UINT16 operandValue;

	if (!decodeF2Instruction(currentInstruction, &operand, &isByteInstruction))
		return FALSE;

	operandValue = *(UINT16*)operand;

	if (operandValue & 0x80) {
		operandValue |= 0xFF00;
		nBit = TRUE;
	}
	else {
		operandValue &= 0x00FF;
		nBit = FALSE;
	}

	zBit = (!operandValue);

	cBit = !zBit;

	vBit = 0;

	*(UINT16*)operand = operandValue;

	if (!writeBack(memoryWriteBack, operand, 2))
		return FALSE;
	freeConditional(operand);

	clocks++;
	increasePC();
	return TRUE;
}

int XOR_executor () {
	int isByteInstruction = FALSE;
	void *source;
	void *destination;
	UINT16 result;
	UINT16 uiSource, uiDestination;

	if (!decodeF1Instruction(currentInstruction, &source, &destination, &isByteInstruction))
		return FALSE;

	// Execute
	if (isByteInstruction) {
		uiSource = (*(UINT8*)source);
		uiDestination = (*(UINT8*)destination);
		result = uiSource ^ uiDestination;
		IntToByteByPtr(result,destination);
	} else {
		uiSource = (*(UINT16*)source);
		uiDestination = (*(UINT16*)destination);
		result = uiSource ^ uiDestination;
		IntToWordByPtr(result,destination);
	}

	// (N-bit)
	if (!isByteInstruction) compNegative <<= 8;
	nBit = (result & compNegative);

	// (Z-bit)
	zBit = (!(result & WORD_MASK));

	// Carry bit is set, if result != ZERO
	cBit = !zBit;
	
	// V-bit: set if both operands are negative
	vBit = ((uiSource & compNegative) && (uiDestination & compNegative));

	if (!writeBack(memoryWriteBack, destination, (!isByteInstruction)+1))
		return FALSE;
	freeConditional(source);
	freeConditional(destination);
	
	// PC to next word
	increasePC();
	clocks++;
	return TRUE;
}


/*
	Decodes the instruction format and finds executor.
	Executor is returned by reference (*executor).
	Returns format value.
*/
int decodeInstructionFormat(msp_word instruction, executorPtr *executor) {
	UINT16 instructionCode;
	int format = 0;

	instructionCode = instruction.asF1Mask.opcode;
	
	// Decode instruction code
	switch (instructionCode) {
	case 1:		// Format 2
		instructionCode = instruction.asF2Mask.opcode;
		format = 2;
		break;
	case 2:	// Format 3
		instructionCode = instruction.asF3Mask.opcode;
		format = 3;
		break;
	case 3: // Format 3
		instructionCode = instruction.asF3Mask.opcode;
		format = 3;
		break;
	default:	// Format 1
		//instructionCode = instruction.asF1Mask.opcode;
		format = 1;
		break;
	}

	*executor = findExecutor(instructionCode);
	if (executor != NULL)
		return format;
	else
		return 0;
}

/*
	Decodes format 1 instructions.
	The decoded results will be stored at the locations indicated by 
	- instruction_code-
	- source-
	- destination-
	and isByte- pointers.

	The function requires a little endian system, so 
	byte instructions can use (only) the lower byte of 
	source/destination addresses, otherwise the appropriate
	cases (e.g. if isByteInstruction -> address of lower register byte 
	is returned for source/destination) should be implemented. The same measures
	should be applied to the constant generator.
*/
int decodeF1Instruction(msp_word instruction, void **source, 
	void **destination, int *isByte) {

	unsigned int As, Ad, sReg, dReg;
	//void *constant;
	char cConstant = 0;
	int createConstant = FALSE;
	int isByteInstruction = FALSE;
	msp_word baseAddress = {0};
	msp_word t_address = {0};
	UINT16 offset = 0;

	// Byte or word?
	isByteInstruction = instruction.asF1Mask.bflag;
	*isByte = isByteInstruction;
	memoryWriteBack = MEMORY_WRITEBACK_NO;
	// Decode source operand
	As = instruction.asF1Mask.as;
	sReg = instruction.asF1Mask.sreg;

	switch (As) {	// Addressing mode
	case ADDR_MODE_REGISTER:
		switch (sReg) {
		case CG2:	// const generator
			createConstant = TRUE;
			cConstant = 0;
			break;
		default:
			*source = &registers[sReg];
			break;
		}
		break;
	case ADDR_MODE_INDEXED_SYMBOLIC_ABSOLUTE:
		switch (sReg) {
		case CG1:		// = SR
			// absolute addressing, offset (to 0) in next word
			increasePC();
			if (!vmbReadWordAt(registers[PC], &offset))
				return FALSE;
			else
				clocks++;
			t_address.asWord = offset;
			if (isByteInstruction) {
				*source = malloc(sizeof(UINT8));
				if (!vmbReadByteAt(t_address,(UINT8*)*source))
					return FALSE;
				else
					clocks++;
			} else {
				*source = malloc(sizeof(UINT16));
				if (!vmbReadWordAt(t_address,(UINT16*)*source))
					return FALSE;
				else
					clocks++;
			}
			break;
		case CG2:		// const generator
			createConstant = TRUE;
			cConstant = 1;
			break;
		default:
			// indexed addressing: reg+offset, offset in next word
			baseAddress = registers[sReg];
			increasePC();
			if (!vmbReadWordAt(registers[PC], &offset))
				return FALSE;
			else
				clocks++;
			t_address.asWord = baseAddress.asWord + offset;
			if (isByteInstruction) {
				*source = malloc(sizeof(UINT8));
				if (!vmbReadByteAt(t_address,(UINT8*)*source))
					return FALSE;
				else
					clocks++;
			} else {
				*source = malloc(sizeof(UINT16));
				if (!vmbReadWordAt(t_address,(UINT16*)*source))
					return FALSE;
				else
					clocks++;
			}
			break;
		}
		break;
	case ADDR_MODE_INDIRECT_REGISTER:
		switch (sReg) {
		case CG1:		// const generator
			createConstant = TRUE;
			cConstant = 4;
			break;
		case CG2:		// const generator
			createConstant = TRUE;
			cConstant = 2;
			break;
		default:
			// indirect register addressing: address in reg
			t_address = registers[sReg];
			if (isByteInstruction) {
				*source = malloc(sizeof(UINT8));
				if (!vmbReadByteAt(t_address,(UINT8*)*source))
					return FALSE;
				else
					clocks++;
			} else {
				*source = malloc(sizeof(UINT16));
				if (!vmbReadWordAt(t_address,(UINT16*)*source))
					return FALSE;
				else
					clocks++;
			}
			break;
		}
		break;
	case ADDR_MODE_INDIRECT_AUTOINC_IMMEDIATE:
		switch (sReg) {
		case CG1:		// const generator
			createConstant = TRUE;
			cConstant = 8;
			break;
		case CG2:		// const generator
			createConstant = TRUE;
			cConstant = -1;
			break;
		default:
			// indirect register auto-increment or immediate address mode
			if (sReg == PC) {		// immediate
				increasePC();
			}
			t_address = registers[sReg];
			if (isByteInstruction) {
				*source = malloc(sizeof(UINT8));
				if (!vmbReadByteAt(t_address,(UINT8*)*source))
					return FALSE;
				else
					clocks++;
			} else {
				*source = malloc(sizeof(UINT16));
				if (!vmbReadWordAt(t_address,(UINT16*)*source))
					return FALSE;
				else
					clocks++;
			}
			if (sReg != PC) {
				registers[sReg].asWord += (!isByteInstruction)+1;
			}
			break;
		}
		break;
	default:
		*source = NULL;
	}
	if (createConstant) {
		if (isByteInstruction) {
			*source = malloc(sizeof(UINT8));
			*(UINT8*)*source = cConstant;
		}
		else {
			*source = malloc(sizeof(UINT16));
			*(UINT16*)*source = cConstant;
		}
	}
	
	// Decode destination operand
	Ad = instruction.asF1Mask.ad;
	dReg = instruction.asF1Mask.dreg;

	switch (Ad) {
	case ADDR_MODE_REGISTER:
		// Register is the operand
		*destination = &registers[dReg];
		break;
	case ADDR_MODE_INDEXED_SYMBOLIC_ABSOLUTE:
		switch (dReg) {
		case CG1:		// = SR
			// absolute addressing, offset (to 0) in next word
			increasePC();
			if (!vmbReadWordAt(registers[PC], &offset))
				return FALSE;
			else
				clocks++;
			t_address.asWord = offset;
			memoryWriteBack = t_address.asWord;
			if (isByteInstruction) {
				*destination = malloc(sizeof(UINT8));
				if (!vmbReadByteAt(t_address,(UINT8*)*destination))
					return FALSE;
				else if (instruction.asF1Mask.opcode != 4)		// No clock for MOV
					clocks++;
			} else {
				*destination = malloc(sizeof(UINT16));
				if (!vmbReadWordAt(t_address,(UINT16*)*destination))
					return FALSE;
				else if (instruction.asF1Mask.opcode != 4)		// No clock for MOV
					clocks++;
			}
			break;
		default:
			// indexed addressing: reg+offset, offset in next word
			baseAddress = registers[dReg];
			increasePC();
			if (!vmbReadWordAt(registers[PC], &offset))
				return FALSE;
			else
				clocks++;
			t_address.asWord = baseAddress.asWord + offset;
			memoryWriteBack = t_address.asWord;
			if (isByteInstruction) {
				*destination = malloc(sizeof(UINT8));
				if (!vmbReadByteAt(t_address,(UINT8*)*destination))
					return FALSE;
				else if (instruction.asF1Mask.opcode != 4)		// No clock for MOV
					clocks++;
			} else {
				*destination = malloc(sizeof(UINT16));
				if (!vmbReadWordAt(t_address,(UINT16*)*destination))
					return FALSE;
				else if (instruction.asF1Mask.opcode != 4)		// No clock for MOV
					clocks++;
			}
			break;
		}
		break;
	default:
		*destination = NULL;
	}
	
	if (*source == NULL || *destination == NULL)
		return FALSE;
	else
		return TRUE;
}


int decodeF2Instruction(msp_word instruction, void **operand, int *isByte) {
	unsigned int Ad, DSReg;
	int isByteInstruction = FALSE;
	msp_word baseAddress;
	msp_word t_address;
	UINT16 offset;
	memoryWriteBack = MEMORY_WRITEBACK_NO;
	Ad = instruction.asF2Mask.ad;
	DSReg = instruction.asF2Mask.dsreg;
	isByteInstruction = instruction.asF2Mask.bflag;

	*isByte = isByteInstruction;

	switch (Ad) {
	case ADDR_MODE_REGISTER:		// Register is the operand
		*operand = &registers[DSReg];
		break;
	case ADDR_MODE_INDEXED_SYMBOLIC_ABSOLUTE:
		// Register contains the base address, next word contains the offset
		baseAddress = registers[DSReg];
		increasePC();
		if (!vmbReadWordAt(registers[PC], &offset))
			return FALSE;
		else
			clocks++;
		t_address.asWord = baseAddress.asWord + offset;
		memoryWriteBack = t_address.asWord;
		if (isByteInstruction) {
			*operand = malloc(sizeof(UINT8));
			if (!vmbReadByteAt(t_address, (UINT8*)*operand))
				return FALSE;
			else
				clocks++;
		}
		else {
			*operand = malloc(sizeof(UINT16));
			if (!vmbReadWordAt(t_address, (UINT16*)*operand))
				return FALSE;
			else
				clocks++;
		}
		break;
	case ADDR_MODE_INDIRECT_REGISTER:
		// Register contains the address of the operand
		t_address = registers[DSReg];
		memoryWriteBack = t_address.asWord;
		if (isByteInstruction) {
			*operand = malloc(sizeof(UINT8));
			if (!vmbReadByteAt(t_address, (UINT8*)*operand))
				return FALSE;
		} else {
			*operand = malloc(sizeof(UINT16));
			if (!vmbReadWordAt(t_address, (UINT16*)*operand))
				return FALSE;
		}
		break;
	case ADDR_MODE_INDIRECT_AUTOINC_IMMEDIATE:
		// Register contains the address. Register value will be incremented (byte/wyde)
		if (DSReg == PC)
			increasePC();
		else
			memoryWriteBack = registers[DSReg].asWord;
		if (isByteInstruction) {
			*operand = malloc(sizeof(UINT8));
			if (!vmbReadByteAt(registers[DSReg], (UINT8*)*operand))
				return FALSE;
		} else {
			*operand = malloc(sizeof(UINT16));
			if (!vmbReadWordAt(registers[DSReg], (UINT16*)*operand))
				return FALSE;
		}
		// Execute autoincrement (PC is already incremented)
		if (DSReg != PC) {
			registers[DSReg].asWord += (!isByteInstruction)+1;
		}
		break;
	default:
		*operand = NULL;
	}

	if (*operand != NULL)
		return TRUE;
	else
		return FALSE;
}

int decodeF3Instruction(msp_word instruction, unsigned int *condition, int *offset) {
	unsigned int /*uiInstruction, */uiCondition;
	INT16 iOffset;

	uiCondition = instruction.asF3Mask.condition;
	iOffset = instruction.asF3Mask.offset;
	// Check for sign (offset has 10 bit)
	if (iOffset & 512)
		iOffset &= (-1);

	*condition = uiCondition;
	*offset = iOffset;

	return TRUE;
}

executorPtr findExecutor(UINT16 instructionCode) {
     return INSTRUCTIONS[instructionCode];
}
	
void increasePC() {
	registers[PC].asWord += 2;
	return;
}


/*
	WD: Initializes core components
	and starts the execution at the address pointed by
	value @ 0xFFFE (interrupt vector table).
*/
void initCore(void) {
	msp_word t_address;
	// Initialize local storage
	initRegisters();

	if (!executionStartAddress) {
		t_address.asWord = DEFAULT_EXECUTION_START_LOCATION;
		vmbReadWordAt(t_address, &registers[PC].asWord);		/* Initialize the programm 
														counter with the start address */
	} else
		registers[PC].asWord = executionStartAddress;
	fprintf(stderr,"MSP430 initialized.\n");
}


/*
	Initializes the registers
*/
void initRegisters() {
	memset(&registers, 0, sizeof(msp_word) * REGISTERS_COUNT);
}

msp_word fetchInstruction(msp_word msp_address) {
	msp_word result;
	
	if (vmbReadWordAt(msp_address, &result.asWord))
		return result;
	else {
		result.asWord = 0;
		return result;
	}
}

__inline int writeBack (UINT32 write_back_address, void *data, unsigned char size) {
	msp_word t_address;
	t_address.asWord = write_back_address & WORD_MASK;
	if (write_back_address >= 0) {
		if (size == 1) {
			if (!vmbWriteByteAt(t_address, (UINT8*)data))
				return FALSE;
		} else if (size == 2) {
			if (!vmbWriteWordAt(t_address, (UINT16*)data))
				return FALSE;
		} else 
			return FALSE;
		clocks++;
	}
	return TRUE;
}

__inline void checkCBit(UINT32 check, int asByte) {
	if (asByte)
		cBit = ((check & (compCarryByte)) != 0);
	else
		cBit = ((check & (compCarryWord)) != 0);
}

void UI() {
	char input_buffer[256];
	char t_buffer[10];
	int t_int;
	int inErrFlag = FALSE;
	msp_word t_address;
	int i,j;
	x = FALSE;

	//Debug output
	
	if (interactiveMode) {
		fprintf(stdout,"Programm stats:\tPC @ 0x%4X\tSP @ 0x%4X\tCycles: %d\n",
			registers[PC].asWord,registers[SP].asWord,clocks);
retry_input:
		fprintf(stdout,"Choose x to execute, q to quit, c to continue,\nbHex to set or unset a breakpoint @ 0xHex, rN to view register N (0-15),\nmHex to view memory @ 0xHex: ");
		inErrFlag = FALSE;

		memset(&input_buffer,0,256);
		fscanf(stdin,"%s",&input_buffer[0]);
		if (strlen(&input_buffer[0])) {
			memset(&t_buffer[0],0,10);
			sscanf(&input_buffer[0],"%[c]",t_buffer);	// Test c
			if (t_buffer[0]) {
				interactiveMode = !interactiveMode;
				x = !interactiveMode;
				goto skip_i_checks;
			}

			memset(&t_buffer[0],0,10);
			sscanf(&input_buffer[0],"%[q]",&t_buffer[0]);	// Test q
			if (t_buffer[0])
				return;

			memset(&t_buffer[0],0,10);
			sscanf(&input_buffer[0],"%[x]",&t_buffer[0]);	// Test x
			if (t_buffer[0]) {
				x = TRUE;
				goto skip_i_checks;
			}

			t_int = -1;
			sscanf(&input_buffer[0],"r%2d",&t_int);	// Test r
			if ((t_int >= 0)) {
				if (t_int < REGISTERS_COUNT) {
					fprintf(stdout,"\nRegister %d: 0x%4X\n",t_int,
						registers[t_int].asWord);
					goto skip_i_checks;
				} else {
					inErrFlag = TRUE;
					goto skip_i_checks;
				}
			}

			t_int = -1;
			sscanf(&input_buffer[0],"b%4x",&t_int); // Test b
			if (t_int >= 0) {
				t_address.asWord = t_int;
				// (un)set breakpoint
				for (i=0;i<MAX_BREAKPOINTS;i++) {
					if (breakpoints[i].asWord == t_address.asWord) {
						breakpoints[i].asWord = 0;
						for (j=i;j<MAX_BREAKPOINTS-1;j++) {
							breakpoints[i] = breakpoints[i+1];
						}
						breakpoints[MAX_BREAKPOINTS-1].asWord = 0;
						fprintf(stdout,"\nBreakpoint @0x%X%X unset.\n",t_address.asBytes.hi,t_address.asBytes.lo);
						break;
					}
					if (breakpoints[i].asWord == 0) {
						breakpoints[i] = t_address;
						fprintf(stdout,"\nBreakpoint @0x%X%X set.\n",t_address.asBytes.hi,t_address.asBytes.lo);
						break;
					}
				}
				goto skip_i_checks;
			}

			// Test m
			t_int = -1;
			memset(&t_buffer[0],0,10);
			sscanf(&input_buffer[0],"m%4x",&t_int); 
			if (t_int >= 0) {
				t_address.asWord = t_int;
				vmbReadWordAt(t_address, (UINT16*)&t_buffer[0]);
				fprintf(stdout,"\nMemory @0x%4X: 0x%4X\n",t_address.asWord, 
					((msp_word*)&t_buffer[0])->asWord);
				goto skip_i_checks;
			}

				
skip_i_checks:
			fprintf(stdout,"\n");
		} else {
			inErrFlag = TRUE;
		}

		if (inErrFlag) {
			fprintf(stdout,"Invalid input \"%s\".\n",&input_buffer[0]);
			goto retry_input;
		}
		if (!x) {
			goto retry_input;
		}
	} else {
		if (clocks % 10000 < 5) {
			fprintf(stdout,"Programm stats:\tPC @ 0x%4X\tSP @ 0x%4X\tCycles: %d\n",
				registers[PC].asWord,registers[SP].asWord,clocks);
		}
		// Check breakpoints
		for(i=0;i<MAX_BREAKPOINTS;i++) {
			if (breakpoints[i].asWord == registers[PC].asWord) {
				interactiveMode = TRUE;
				goto retry_input;
			}
		}
		// Continous mode
		x = TRUE;
	}
}

int main(int argc, char *argv[])
{	
	executorPtr executor;
	unsigned int format = 0;
	unsigned int opcode = 0;

	// Init core and start execution
	initVMBInterface();
	fprintf(stderr,"VMB connected.\n");
 boot:
	if (!wait_for_power())
		goto end_simulation;
	initCore();
	

	// main loop
	while (TRUE) {
		if (!vmb.connected) {
			fprintf(stderr,"VMB disconnected during execution.\n");
			break;
		}
		if (!vmb.power || vmb.reset_flag)
		{ 
		  vmb.reset_flag= 0;
		  goto boot;
		}
		// fetch
		currentInstruction = fetchInstruction(registers[PC]);
		// Call UI
		UI();
		if (!x)
			goto end_simulation;
		
		if (!currentInstruction.asWord) {
			fprintf(stderr,"MSP430-Error: Invalid instruction format @0x%4X in instruction 0x%4X\n",
				registers[PC].asWord,currentInstruction.asWord);
			break;
		}
		format = decodeInstructionFormat(currentInstruction, &executor);
		
		// Opcode for debugging output
		switch (format) {
		case 1:
			opcode = currentInstruction.asF1Mask.opcode;
			break;
		case 2:
			opcode = currentInstruction.asF2Mask.opcode;
			break;
		case 3:
			opcode = currentInstruction.asF3Mask.opcode;
			break;
		default:
			opcode = 0;
			break;
		}
		if (!format) {
			fprintf(stderr,"MSP430-Error: Invalid instruction format @0x%4X in instruction 0x%4X\n",
				registers[PC].asWord,currentInstruction.asWord);
			break;
		}
		if (executor == NULL) {
			fprintf(stderr,"MSP430-Error: Invalid opcode %d @0x%4X in instruction 0x%4X\n",
				opcode,registers[PC].asWord,currentInstruction.asWord);
			break;
		}
		if (!executor()) {
			fprintf(stderr,"MSP430-Error: Could not execute %s @0x%4X in instruction 0x%4X\n",
				INSTRUCTION_NAMES[opcode],
				registers[PC].asWord,currentInstruction.asWord);
			break;
		}
	}
end_simulation:
	
	fprintf(stderr,"Exiting...\n");
	vmb_disconnect(&vmb);
	vmb_end();
#ifdef MEM_CHECK_H
	EXIT_SUCC;
#else
	return 0;
#endif
}

