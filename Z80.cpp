#include "Z80.h"
#include <memory.h>
#include <stdio.h>

Z80::Z80(void) {
	memset(&m_DAATable, 0, sizeof(m_DAATable));
	memset(&m_ZSPTable, 0, sizeof(m_ZSPTable));
	InitDAATable();
}

Z80::~Z80(void)
{

}

WORD Z80::ReadWord() const
{
	WORD result = m_ContextZ80.m_FuncPtrRead(m_ContextZ80.m_ProgramCounter + 1);
	result = result << 8;
	result |= m_ContextZ80.m_FuncPtrRead(m_ContextZ80.m_ProgramCounter);
	return result;
}

void Z80::PushWordOntoStack(WORD word)
{
	BYTE hi = word >> 8;
	BYTE lo = word & 0xFF;
	m_ContextZ80.m_StackPointer.reg--;
	m_ContextZ80.m_FuncPtrWrite(m_ContextZ80.m_StackPointer.reg, hi);
	m_ContextZ80.m_StackPointer.reg--;
	m_ContextZ80.m_FuncPtrWrite(m_ContextZ80.m_StackPointer.reg, lo);
}

WORD Z80::PopWordOffStack()
{
	WORD word = m_ContextZ80.m_FuncPtrRead(m_ContextZ80.m_StackPointer.reg + 1) << 8;
	word |= m_ContextZ80.m_FuncPtrRead(m_ContextZ80.m_StackPointer.reg);
	m_ContextZ80.m_StackPointer.reg += 2;
	return word;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//												OPCODES
/////////////////////////////////////////////////////////////////////////////////////////////////////

// Load immediate byte into register
void Z80::CPU_8BIT_LI(BYTE& reg) {
	m_ContextZ80.m_OpcodeCycle = 7;
	BYTE toLoad = m_ContextZ80.m_FuncPtrRead(m_ContextZ80.m_ProgramCounter);
	m_ContextZ80.m_ProgramCounter++;
	reg = toLoad;
}

void Z80::CPU_REG_LOAD(BYTE& reg, BYTE load) {
	m_ContextZ80.m_OpcodeCycle = 4;
	reg = load;
}

void Z80::CPU_REG_LOAD_ROM(BYTE& reg, WORD address) {
	m_ContextZ80.m_OpcodeCycle = 7;
	reg = m_ContextZ80.m_FuncPtrRead(address);
}

// Add 8-bit
void Z80::CPU_8BIT_ADD(BYTE& reg, BYTE toAdd, int cycles, bool useImmediate, bool addCarry) {
	m_ContextZ80.m_OpcodeCycle = cycles;
	BYTE before = reg;
	unsigned int adding = 0; 
	BYTE nonMod = toAdd;
	int res = 0;

	// Immediate mode
	if (useImmediate) {
		adding = m_ContextZ80.m_FuncPtrRead(m_ContextZ80.m_ProgramCounter);
		nonMod = adding;
		m_ContextZ80.m_ProgramCounter++;
	}
	// Using second param
	else {
		adding = toAdd;
	}

	// Adding carry
	if (addCarry) {
		if (TestBit(m_ContextZ80.m_RegisterAF.lo, FLAG_C)) {
			adding++;
		}
	}

	res = reg + adding;
	reg += adding;

	// Set flags
	if (TestBit(reg, 7))
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_S);

	if(reg == 0)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_Z);

	if((nonMod ^ before ^ 0x80) & (nonMod ^ res) & 0x80)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);

	if((before ^ res ^ nonMod) & 0x10)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_H);

	if((before + adding) > 0xFF)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_C);

}

// Sub 8-bit --- Possibly wrong
void Z80::CPU_8BIT_SUB(BYTE& reg, BYTE toSub, int cycles, bool useImmediate, bool subCarry) {
	m_ContextZ80.m_OpcodeCycle = cycles;
	BYTE before = reg;
	unsigned int subtracting = 0;
	BYTE nonMod = toSub;
	int res = 0;

	if (useImmediate) {
		subtracting = m_ContextZ80.m_FuncPtrRead(m_ContextZ80.m_ProgramCounter);
		nonMod = subtracting;
		m_ContextZ80.m_ProgramCounter++;
	}
	else {
		subtracting = toSub;
	}

	if (subCarry) {
		if (TestBit(m_ContextZ80.m_RegisterAF.lo, FLAG_C)) {
			subtracting++;
		}
	}

	res = reg - subtracting;
	reg -= subtracting;

	m_ContextZ80.m_RegisterAF.lo = 0;

	if(reg == 0)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_Z);

	m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_N);

	if(before < subtracting)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_C);

	SIGNED_WORD htest = (before & 0xF);
	htest -= (subtracting & 0xF);

	if((before ^ res ^ nonMod) & 0x10)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_H);

	if (TestBit(reg, 7))
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_S);

	if((nonMod ^ before) & (before ^ res) & 0x80)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);
}

void Z80::CPU_8BIT_AND(BYTE& reg, BYTE toAnd, int cycles, bool useImmediate) {
	m_ContextZ80.m_OpcodeCycle = cycles;
	BYTE anding = 0;

	if (useImmediate) {
		anding = m_ContextZ80.m_FuncPtrRead(m_ContextZ80.m_ProgramCounter);
		m_ContextZ80.m_ProgramCounter++;
	}
	else {
		anding = toAnd;
	}

	reg &= anding;

	m_ContextZ80.m_RegisterAF.lo;

	if(reg == 0)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_Z);

	m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_H);

	if(TestBit(reg,7))
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_S);

	// Going off of someone else here who isn't sure so could be wrong
	int pcount = 0;
	for (int i = 0; i < 8; i++) {
		if (TestBit(reg, i)) {
			pcount++;
		}
	}

	if((pcount == 0) || ((pcount % 2) == 0))
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);

}

void Z80::CPU_8BIT_OR(BYTE& reg, BYTE toOr, int cycles, bool useImmediate) {
	m_ContextZ80.m_OpcodeCycle = cycles;
	BYTE oring = 0;

	if (useImmediate) {
		oring = m_ContextZ80.m_FuncPtrRead(m_ContextZ80.m_ProgramCounter);
		m_ContextZ80.m_ProgramCounter++;
	}
	else {
		oring = toOr;
	}

	reg |= oring;
	m_ContextZ80.m_RegisterAF.lo = 0;

	if(reg == 0)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_Z);
	
	if(TestBit(reg, 7))
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_S);

	// Going off of someone else here who isn't sure so could be wrong
	int pcount = 0;
	for (int i = 0; i < 8; i++)
	{
		if (TestBit(reg, i))
			pcount++;
	}

	if ((pcount == 0) || ((pcount % 2) == 0))
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);

}

void Z80::CPU_8BIT_XOR(BYTE& reg, BYTE toXOr, int cycles, bool useImmediate) {
	m_ContextZ80.m_OpcodeCycle = cycles;
	BYTE xoring = 0;
	if (useImmediate) {
		xoring = m_ContextZ80.m_FuncPtrRead(m_ContextZ80.m_ProgramCounter);
		m_ContextZ80.m_ProgramCounter++;
	}
	else {
		xoring = toXOr;
	}

	reg ^= xoring;
	m_ContextZ80.m_RegisterAF.lo = 0;

	if(reg == 0)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_Z);

	if(TestBit(reg, 7))
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_S);

	// Going off of someone else here who isn't sure so could be wrong
	int pcount = 0;
	for (int i = 0; i < 8; i++)
	{
		if (TestBit(reg, i))
			pcount++;
	}

	if ((pcount == 0) || ((pcount % 2) == 0))
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);
}

// Doesn't change any registers so don't pass by reference
void Z80::CPU_8BIT_CMPR(BYTE reg, BYTE toSubtract, int cycles, bool useImmediate) {
	// if there are any problems with this function it must be to do with the flags that CPI sets itself like
	// the carry flag and the PV flag.

	m_ContextZ80.m_OpcodeCycle = cycles;
	BYTE before = reg;
	BYTE subtracting = 0;
	BYTE nonMod = toSubtract;
	int res = 0;

	if (useImmediate) {
		subtracting = m_ContextZ80.m_FuncPtrRead(m_ContextZ80.m_ProgramCounter);
		nonMod = subtracting;
		m_ContextZ80.m_ProgramCounter++;
	}
	else {
		subtracting = toSubtract;
	}

	res = reg - subtracting;
	reg -= subtracting;

	m_ContextZ80.m_RegisterAF.lo = 0;

	if(reg == 0)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_Z);
	
	m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_N);

	if(before < subtracting)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_C);

	SIGNED_WORD htest = (before & 0xF);
	htest -= (subtracting & 0xF);

	if((before ^ res ^ nonMod) & 0x10)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_H);

	if(TestBit(reg, 7))
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_S);

	if((nonMod ^ before) & (before ^ res) & 0x80)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);


}
void Z80::CPU_8BIT_INC(BYTE& reg, int cycles) {
	// WHEN EDITING THIS FUNCTION DONT FORGET TO MAKE THE SAME CHANGES TO CPU_8BIT_MEM_INC
	m_ContextZ80.m_OpcodeCycle = cycles;

	BYTE before = reg;

	reg++;

	if (reg == 0)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_Z);
	else
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_Z);

	m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_N);

	if ((before & 0xF) == 0xF)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_H);
	else
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_H);

	if (TestBit(reg, 7))
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_S);
	else
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_S);
	
	if (before == 127)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);
	else
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);
}

void Z80::CPU_8BIT_DEC(BYTE& reg, int cycles) {
	// WHEN EDITING THIS FUNCTION DONT FORGET TO MAKE THE SAME CHANGES TO CPU_8BIT_MEM_DEC
	m_ContextZ80.m_OpcodeCycle = cycles;

	BYTE before = reg;

	reg--;

	if (reg == 0)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_Z);
	else
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_Z);

	if ((before & 0x0F) == 0)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_H);
	else
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_H);

	SIGNED_BYTE vtest = before;

	if (vtest == -128)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);
	else
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);

	m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_N);


	if (TestBit(reg, 7))
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_S);
	else
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_S);
}

void Z80::CPU_8BIT_MEM_INC(WORD address, int cycles) {
	// WHEN EDITING THIS FUNCTION DONT FORGET TO MAKE THE SAME CHANGES TO CPU_8BIT_INC
	m_ContextZ80.m_OpcodeCycle = cycles;

	BYTE before = m_ContextZ80.m_FuncPtrRead(address);
	m_ContextZ80.m_FuncPtrWrite(address, (before + 1));
	BYTE now = before + 1;

	if (now == 0)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_Z);
	else
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_Z);

	m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_N);

	if ((before & 0xF) == 0xF)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_H);
	else
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_H);

	if (TestBit(now, 7))
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_S);
	else
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_S);

	if (before == 127)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);
	else
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);
}

void Z80::CPU_8BIT_MEM_DEC(WORD address, int cycles) {
	// WHEN EDITING THIS FUNCTION DONT FORGET TO MAKE THE SAME CHANGES TO CPU_8BIT_DEC
	m_ContextZ80.m_OpcodeCycle = cycles;
	BYTE before = m_ContextZ80.m_FuncPtrRead(address);
	m_ContextZ80.m_FuncPtrWrite(address, (before - 1));
	BYTE now = before - 1;

	if (now == 0)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_Z);
	else
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_Z);

	m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_N);

	if ((before & 0x0F) == 0)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_H);
	else
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_H);

	if (TestBit(now, 7))
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_S);
	else
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_S);

	SIGNED_BYTE vtest = before;

	if (vtest == -128)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);
	else
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);
}

void Z80::CPU_8BIT_IXIY_LOAD(BYTE& store, const Z80REGISTER& reg) {
	CPU_REG_LOAD_ROM(store, GetIXIYAddress(reg.reg));
	m_ContextZ80.m_OpcodeCycle = 11;
}

void Z80::CPU_8BIT_MEM_IXIY_LOAD(BYTE store, const Z80REGISTER& reg) {
	m_ContextZ80.m_FuncPtrWrite(GetIXIYAddress(reg.reg), store);
	m_ContextZ80.m_OpcodeCycle = 19;
}

void Z80::CPU_16BIT_INC(WORD& word, int cycles) {
	m_ContextZ80.m_OpcodeCycle = cycles;
	word++;
}

void Z80::CPU_16BIT_DEC(WORD& word, int cycles) {
	m_ContextZ80.m_OpcodeCycle = cycles;
	word--;
}

void Z80::CPU_16BIT_ADD(WORD& reg, WORD toAdd, int cycles, bool addCarry) {
	m_ContextZ80.m_OpcodeCycle = cycles;
	WORD before = reg;
	unsigned int adding = toAdd;

	if (addCarry) {
		if (TestBit(m_ContextZ80.m_RegisterAF.lo, FLAG_C)) {
			adding++;
		}
	}

	unsigned long result = before + adding;

	m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_N);

	if(result & 0xFFFF0000)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_C);
	else
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_C);

	if(((before & 0x0FFF) + (adding & 0x0FFF)) > 0x0FFF)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_H);
	else 
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_H);

	if (addCarry) {
		if(!((before ^ adding) & 0x8000) && ((before ^ reg) & 0x8000))
			m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);
		else
			m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);

		if(reg == 0)
			m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_Z);
		else
			m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_Z);

		if(TestBit(reg, 15))
			m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_S);
		else
			m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_S);
	}
}

void Z80::CPU_16BIT_SUB(WORD& reg, WORD toSub, int cycles, bool subCarry) {
	m_ContextZ80.m_OpcodeCycle = cycles;
	WORD before = reg;
	int c = TestBit(m_ContextZ80.m_RegisterAF.lo, FLAG_C) ? 1 : 0;
	int res = reg - toSub - c;

	m_ContextZ80.m_RegisterAF.lo = 0;

	if(res & 0x10000)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_C);

	if(((before ^ res ^ toSub) >> 8) & 0x10)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_H);

	m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_N);

	if(!(res & 0xFFFF))
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_Z);

	if((toSub ^ before) & (before ^ res) & 0x8000)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);

	reg = res & 0xFFFF;

	if(TestBit(reg, 15))
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_S);
}

void Z80::CPU_16BIT_LOAD(WORD& reg) {
	m_ContextZ80.m_OpcodeCycle = 10;
	reg = ReadWord();
	m_ContextZ80.m_ProgramCounter += 2;
}

void Z80::CPU_JUMP(bool useCondition, int flag, bool condition) {
	m_ContextZ80.m_OpcodeCycle = 10;

	if (!useCondition) {
		WORD nn = ReadWord();
		m_ContextZ80.m_ProgramCounter += 2;
		m_ContextZ80.m_ProgramCounter = nn;
		return;
	}

	if (TestBit(m_ContextZ80.m_RegisterAF.lo, flag) == condition) {
		WORD nn = ReadWord();
		m_ContextZ80.m_ProgramCounter = nn;
	}
	else {
		m_ContextZ80.m_ProgramCounter += 2;
	}
}

void Z80::CPU_JUMP_IMMEDIATE(bool useCondition, int flag, bool condition) {
	m_ContextZ80.m_OpcodeCycle = 12;

	if (!useCondition) {
		SIGNED_BYTE n = (SIGNED_BYTE)m_ContextZ80.m_FuncPtrRead(m_ContextZ80.m_ProgramCounter);
		m_ContextZ80.m_ProgramCounter += n;
	}
	else if (TestBit(m_ContextZ80.m_RegisterAF.lo, flag) == condition) {
		SIGNED_BYTE n = m_ContextZ80.m_FuncPtrRead(m_ContextZ80.m_ProgramCounter);
		m_ContextZ80.m_ProgramCounter += n;
	}
	else {
		m_ContextZ80.m_OpcodeCycle = 7;
	}
	m_ContextZ80.m_ProgramCounter++;
}

void Z80::CPU_LOAD_NNN(WORD reg) {
	WORD nn = ReadWord();
	m_ContextZ80.m_ProgramCounter += 2;
	m_ContextZ80.m_OpcodeCycle = 16;
	m_ContextZ80.m_FuncPtrWrite(nn, reg & 0xFF);
	m_ContextZ80.m_FuncPtrWrite(nn + 1, reg >> 8);
}

void Z80::CPU_NEG() {
	BYTE before = m_ContextZ80.m_RegisterAF.hi;

	m_ContextZ80.m_RegisterAF.lo = 0;
	m_ContextZ80.m_RegisterAF.lo = 0 - m_ContextZ80.m_RegisterAF.hi;
	m_ContextZ80.m_OpcodeCycle = 8;

	BYTE a = m_ContextZ80.m_RegisterAF.hi;

	m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_N);

	if (a == 0) 
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_Z);

	if (a > 127) 
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_S);
	
	SIGNED_WORD htest = 0;
	htest -= (before & 0xF);

	if(htest < 0)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_H);

	if(before == 128)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);

	if(before != 0)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_C);
}

void Z80::CPU_CALL(bool useCondition, int flag, bool condition) {

}

void Z80::CPU_RETURN(bool useCondition, int flag, bool condition) {

}

void Z80::CPU_RESTARTS(BYTE n) {

}

void Z80::CPU_RLC(BYTE& reg, bool isAReg) {

}

void Z80::CPU_RLC_MEM(WORD address, bool isAReg) {

}

void Z80::CPU_RRC(BYTE& reg, bool isAReg) {

}

void Z80::CPU_RRC_MEM(WORD address, bool isAReg) {

}

void Z80::CPU_DDFD_RLC(BYTE& reg, WORD& ixiyreg, SIGNED_BYTE& displacement) {

}

void Z80::CPU_DDFD_RRC(BYTE& reg, WORD& ixiyreg, SIGNED_BYTE& displacement) {

}

void Z80::CPU_DAA() {

}

void Z80::CPU_RL(BYTE& reg, bool isAReg) {

}

void Z80::CPU_RL_MEM(WORD address, bool isAReg) {

}

void Z80::CPU_RR(BYTE& reg, bool isAReg) {

}

void Z80::CPU_RR_MEM(WORD address, bool isAReg) {

}

void Z80::CPU_DDFD_RL(BYTE& reg, WORD& ixiyreg, SIGNED_BYTE& displacement) {

}

void Z80::CPU_DDFD_RR(BYTE& reg, WORD& ixiyreg, SIGNED_BYTE& displacement) {

}

// Could have cycle count of 2 instead(?)
void Z80::CPU_RLD() {
	m_ContextZ80.m_OpcodeCycle = 5;
	BYTE hldata = m_ContextZ80.m_FuncPtrRead(m_ContextZ80.m_RegisterHL.reg);
	BYTE nibbleloA = m_ContextZ80.m_RegisterAF.hi & 0xF;
	BYTE nibbleloHL = hldata & 0xF;
	BYTE nibblehiHL = hldata >> 4;

	m_ContextZ80.m_RegisterAF.hi &= 0xF0;
	m_ContextZ80.m_RegisterAF.hi |= nibblehiHL;
	hldata = nibbleloHL << 4;
	hldata |= nibbleloA;

	m_ContextZ80.m_FuncPtrWrite(m_ContextZ80.m_RegisterHL.reg, hldata);

	BYTE a = m_ContextZ80.m_RegisterAF.hi;
	BYTE& f = m_ContextZ80.m_RegisterAF.lo;

	if (TestBit(a, 7))
		f = BitSet(f, FLAG_S);
	else
		f = BitReset(f, FLAG_S);

	if (a == 0)
		f = BitSet(f, FLAG_Z);
	else
		f = BitReset(f, FLAG_Z);

	f = BitReset(f, FLAG_H);
	f = BitReset(f, FLAG_N);

	int pcount = 0;
	for (int i = 0; i < 8; i++) {
		if (TestBit(a, i))
			pcount++;
	}

	if ((pcount == 0) || ((pcount % 2) == 0))
		f = BitSet(f, FLAG_PV);
	else
		f = BitReset(f, FLAG_PV);
}

// Could have cycle count of 2 instead(?)
void Z80::CPU_RRD() {
	m_ContextZ80.m_OpcodeCycle = 5;
	BYTE hldata = m_ContextZ80.m_FuncPtrRead(m_ContextZ80.m_RegisterHL.reg);
	BYTE nibbleloA = m_ContextZ80.m_RegisterAF.hi & 0xF;
	BYTE nibbleloHL = hldata & 0xF;
	BYTE nibblehiHL = hldata >> 4;

	m_ContextZ80.m_RegisterAF.hi &= 0xF0;
	m_ContextZ80.m_RegisterAF.hi |= nibbleloHL;
	hldata = nibbleloA << 4;
	hldata |= nibblehiHL;

	m_ContextZ80.m_FuncPtrWrite(m_ContextZ80.m_RegisterHL.reg, hldata);

	BYTE a = m_ContextZ80.m_RegisterAF.hi;
	BYTE& f = m_ContextZ80.m_RegisterAF.lo;

	if (TestBit(a, 7))
		f = BitSet(f, FLAG_S);
	else
		f = BitReset(f, FLAG_S);

	if (a == 0)
		f = BitSet(f, FLAG_Z);
	else
		f = BitReset(f, FLAG_Z);

	f = BitReset(f, FLAG_H);
	f = BitReset(f, FLAG_N);

	int pcount = 0;
	for (int i = 0; i < 8; i++)
	{
		if (TestBit(a, i))
			pcount++;
	}

	if ((pcount == 0) || ((pcount % 2) == 0))
		f = BitSet(f, FLAG_PV);
	else
		f = BitReset(f, FLAG_PV);
}

// Shift left arithmetic
// WHEN EDITING THIS ALSO EDIT CPU_SLA_MEM
void Z80::CPU_SLA(BYTE& reg) {
	m_ContextZ80.m_OpcodeCycle = 8;
	bool isMSBSet = TestBit(reg, 7);
	reg <<= 1;
	m_ContextZ80.m_RegisterAF.lo = 0;

	if (isMSBSet)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_C);

	if (reg == 0)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_Z);

	if (TestBit(reg, 7))
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_S);

	int pcount = 0;
	for (int i = 0; i < 8; i++) {
		if (TestBit(reg, i))
			pcount++;
	}

	if ((pcount == 0) || ((pcount % 2) == 0))
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);
}

// WHEN EDITING THIS ALSO EDIT CPU_SLA
void Z80::CPU_SLA_MEM(WORD address) {
	m_ContextZ80.m_OpcodeCycle = 15;
	BYTE reg = m_ContextZ80.m_FuncPtrRead(address);
	bool isMSBSet = TestBit(reg, 7);
	reg <<= 1;
	m_ContextZ80.m_RegisterAF.lo = 0;

	if (isMSBSet)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_C);

	if (reg == 0)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_Z);

	if (TestBit(reg, 7))
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_S);

	int pcount = 0;
	for (int i = 0; i < 8; i++) {
		if (TestBit(reg, i))
			pcount++;
	}

	if ((pcount == 0) || ((pcount % 2) == 0))
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);

	m_ContextZ80.m_FuncPtrWrite(address, reg);
}

// Shift right arithmetic
// WHEN EDITING THIS ALSO EDIT CPU_SRA_MEM
void Z80::CPU_SRA(BYTE& reg) {
	m_ContextZ80.m_OpcodeCycle = 8;
	bool isLSBSet = TestBit(reg, 0);
	bool isMSBSet = TestBit(reg, 7);
	m_ContextZ80.m_RegisterAF.lo = 0;
	reg >>= 1;

	if (isMSBSet)
		reg = BitSet(reg, 7);

	if (isLSBSet)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_C);

	if (reg == 0)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_Z);

	if (TestBit(reg, 7))
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_S);

	int pcount = 0;
	for (int i = 0; i < 8; i++) {
		if (TestBit(reg, i))
			pcount++;
	}

	if ((pcount == 0) || ((pcount % 2) == 0))
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);
}

// WHEN EDITING THIS ALSO EDIT CPU_SRA
void Z80::CPU_SRA_MEM(WORD address) {
	m_ContextZ80.m_OpcodeCycle = 15;
	BYTE reg = m_ContextZ80.m_FuncPtrRead(address);
	bool isLSBSet = TestBit(reg, 0);
	bool isMSBSet = TestBit(reg, 7);
	m_ContextZ80.m_RegisterAF.lo = 0;
	reg >>= 1;

	if (isMSBSet)
		reg = BitSet(reg, 7);
	
	if (isLSBSet)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_C);

	if (reg == 0)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_Z);

	if (TestBit(reg, 7))
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_S);

	int pcount = 0;
	for (int i = 0; i < 8; i++) {
		if (TestBit(reg, i))
			pcount++;
	}

	if ((pcount == 0) || ((pcount % 2) == 2))
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);

	m_ContextZ80.m_FuncPtrWrite(address, reg);
}

void Z80::CPU_SRL(BYTE& reg) {

}

void Z80::CPU_SRL_MEM(WORD address) {

}

void Z80::CPU_SLL(BYTE& reg) {

}

void Z80::CPU_SLL_MEM(WORD address) {

}

void Z80::CPU_DDFD_SLA(BYTE& reg, WORD& ixiyreg, SIGNED_BYTE& displacement) {

}

void Z80::CPU_DDFD_SRA(BYTE& reg, WORD& ixiyreg, SIGNED_BYTE& displacement) {

}

void Z80::CPU_DDFD_SRL(BYTE& reg, WORD& ixiyreg, SIGNED_BYTE& displacement) {

}

void Z80::CPU_DDFD_SLL(BYTE& reg, WORD& ixiyreg, SIGNED_BYTE& displacement) {

}

// WHEN EDITING THIS ALSO EDIT CPU_RESET_BIT_MEM
void Z80::CPU_RESET_BIT(BYTE& reg, int bit) {
	reg = BitReset(reg, bit);
	m_ContextZ80.m_OpcodeCycle = 8;
}

// WHEN EDITING THIS ALSO EDIT CPU_RESET_BIT
void Z80::CPU_RESET_BIT_MEM(WORD address, int bit) {
	BYTE mem = m_ContextZ80.m_FuncPtrRead(address);
	mem = BitReset(mem, bit);
	m_ContextZ80.m_FuncPtrWrite(address, mem);
	m_ContextZ80.m_OpcodeCycle = 15;
}

void Z80::CPU_DDFD_RESET_BIT(BYTE& reg, int bit, WORD& ixiyreg, SIGNED_BYTE& displacement) {
	WORD address = ixiyreg + displacement;
	reg = m_ContextZ80.m_FuncPtrRead(address);
	CPU_RESET_BIT(reg, bit);
	m_ContextZ80.m_FuncPtrWrite(address, reg);
	m_ContextZ80.m_OpcodeCycle = 23;
}

void Z80::CPU_TEST_BIT(BYTE reg, int bit, int cycles) {
	bool isSet = false;

	if (TestBit(reg, bit))
	{
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_Z);
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);
		isSet = true;
	}
	else
	{
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_Z);
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);
	}

	m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_N);
	m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_H);

	if ((bit == 7) && isSet)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_S);
	else
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_S);

	m_ContextZ80.m_OpcodeCycle = cycles;
}

void Z80::CPU_DDFD_TEST_BIT(BYTE& reg, int bit, WORD& ixiyreg, SIGNED_BYTE& displacement) {
	WORD address = ixiyreg + displacement;
	reg = m_ContextZ80.m_FuncPtrRead(address);
	CPU_TEST_BIT(reg, bit, 0);
	m_ContextZ80.m_FuncPtrWrite(address, reg);
	m_ContextZ80.m_OpcodeCycle = 20;
}

// WHEN EDITING THIS ALSO EDIT CPU_SET_BIT_MEM
void Z80::CPU_SET_BIT(BYTE& reg, int bit) {
	reg = BitSet(reg, bit);
	m_ContextZ80.m_OpcodeCycle = 8;
}

void Z80::CPU_DDFD_SET_BIT(BYTE& reg, int bit, WORD& ixiyreg, SIGNED_BYTE& displacement) {
	WORD address = ixiyreg + displacement;
	reg = m_ContextZ80.m_FuncPtrRead(address);
	CPU_SET_BIT(reg, bit);
	m_ContextZ80.m_FuncPtrWrite(address, reg);
	m_ContextZ80.m_OpcodeCycle = 23;
}

// WHEN EDITING THIS ALSO EDIT CPU_SET_BIT
void Z80::CPU_SET_BIT_MEM(WORD address, int bit) {
	BYTE mem = m_ContextZ80.m_FuncPtrRead(address);
	mem = BitSet(mem, bit);
	m_ContextZ80.m_FuncPtrWrite(address, mem);
	m_ContextZ80.m_OpcodeCycle = 15;
}

void Z80::CPU_IN(BYTE& data) {
	m_ContextZ80.m_OpcodeCycle = 12;
	data = m_ContextZ80.m_FuncPtrIORead(m_ContextZ80.m_RegisterBC.lo);

	if (TestBit(data, 7))
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_S);
	else 
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_S);

	if(data == 0)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_Z);
	else
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_Z);

	m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_H);
	m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_N);

	int pcount = 0;
	for (int i = 0; i < 8; i++) {
		if (TestBit(data, i))
			pcount++;
	}

	if ((pcount == 0) || ((pcount % 2) == 0))
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);
	else
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);
}

void Z80::CPU_IN_IMMEDIATE(BYTE& data) {
	m_ContextZ80.m_OpcodeCycle = 12;
	BYTE n = m_ContextZ80.m_FuncPtrRead(m_ContextZ80.m_ProgramCounter);
	m_ContextZ80.m_ProgramCounter++;
	data = m_ContextZ80.m_FuncPtrIORead(n);
}

void Z80::CPU_OUT(const BYTE& address, const BYTE& data) {
	m_ContextZ80.m_OpcodeCycle = 12;
	m_ContextZ80.m_FuncPtrIOWrite(address, data);
}

void Z80::CPU_OUT_IMMEDIATE(const BYTE& data) {
	m_ContextZ80.m_OpcodeCycle = 11;

	BYTE n = m_ContextZ80.m_FuncPtrRead(m_ContextZ80.m_ProgramCounter);
	m_ContextZ80.m_ProgramCounter++;
	m_ContextZ80.m_FuncPtrIOWrite(n, data);
}

void Z80::CPU_OUTI() {
	BYTE hldata = m_ContextZ80.m_FuncPtrRead(m_ContextZ80.m_RegisterHL.reg);
	m_ContextZ80.m_FuncPtrIOWrite(m_ContextZ80.m_RegisterBC.lo, hldata);

	CPU_16BIT_INC(m_ContextZ80.m_RegisterHL.reg, 0);
	CPU_8BIT_DEC(m_ContextZ80.m_RegisterBC.hi, 0);

	m_ContextZ80.m_OpcodeCycle = 16;
}

void Z80::CPU_OTIR() {
	CPU_OUTI();

	// Call until b == 0
	if (m_ContextZ80.m_RegisterBC.hi != 0)
	{
		m_ContextZ80.m_ProgramCounter -= 2;
		m_ContextZ80.m_OpcodeCycle = 21;
	}
	else
	{
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_S);
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_Z);
		m_ContextZ80.m_OpcodeCycle = 16;
	}
}

void Z80::CPU_OUTD() {
	BYTE hldata = m_ContextZ80.m_FuncPtrRead(m_ContextZ80.m_RegisterHL.reg);
	m_ContextZ80.m_FuncPtrIOWrite(m_ContextZ80.m_RegisterBC.lo, hldata);

	CPU_16BIT_DEC(m_ContextZ80.m_RegisterHL.reg, 0);
	CPU_8BIT_DEC(m_ContextZ80.m_RegisterBC.hi, 0);

	m_ContextZ80.m_OpcodeCycle = 16;
}

void Z80::CPU_OUTDR() {
	CPU_OUTD();

	if (m_ContextZ80.m_RegisterBC.hi != 0) {
		m_ContextZ80.m_OpcodeCycle = 21;
		m_ContextZ80.m_ProgramCounter -= 2;
	}
	else {
		m_ContextZ80.m_OpcodeCycle = 16;
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_S);
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_Z);
	}
}

// Compare and increment
BYTE Z80::CPU_CPI() {
	bool carry = TestBit(m_ContextZ80.m_RegisterAF.lo, FLAG_C);
	BYTE result = m_ContextZ80.m_FuncPtrRead(m_ContextZ80.m_RegisterHL.reg);
	CPU_8BIT_CMPR(m_ContextZ80.m_RegisterAF.hi, result, 0, false);
	CPU_16BIT_INC(m_ContextZ80.m_RegisterHL.reg, 0);
	CPU_16BIT_DEC(m_ContextZ80.m_RegisterBC.reg, 0);

	if (m_ContextZ80.m_RegisterBC.reg == 0)
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);
	else
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);

	if (!carry)
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_C);
	else
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_C);

	m_ContextZ80.m_OpcodeCycle = 16;
	return result;
}

void Z80::CPU_CPIR() {
	BYTE hladdress = CPU_CPI();

	// Call until b = 0
	if ((m_ContextZ80.m_RegisterBC.reg != 0) && (hladdress != m_ContextZ80.m_RegisterAF.hi)) {
		m_ContextZ80.m_OpcodeCycle = 21;
		m_ContextZ80.m_ProgramCounter -= 2;
	}
	else
		m_ContextZ80.m_OpcodeCycle = 16;
}

BYTE Z80::CPU_CPD() {
	bool carry = TestBit(m_ContextZ80.m_RegisterAF.lo, FLAG_C);
	BYTE result = m_ContextZ80.m_FuncPtrRead(m_ContextZ80.m_RegisterHL.reg);

	CPU_8BIT_CMPR(m_ContextZ80.m_RegisterAF.hi, result, 0, false);
	CPU_16BIT_DEC(m_ContextZ80.m_RegisterHL.reg, 0);
	CPU_16BIT_DEC(m_ContextZ80.m_RegisterBC.reg, 0);

	if (m_ContextZ80.m_RegisterBC.reg == 0)
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);
	else
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);

	if(!carry)
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_C);
	else
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_C);

	m_ContextZ80.m_OpcodeCycle = 16;
	return result;
}

void Z80::CPU_CPDR() {
	BYTE hladdress = CPU_CPD();
	
	// Call until bc = 0 or a = hladdress
	if ((m_ContextZ80.m_RegisterBC.reg != 0) && (hladdress != m_ContextZ80.m_RegisterAF.hi)) {
		m_ContextZ80.m_OpcodeCycle = 21;
		m_ContextZ80.m_ProgramCounter -= 2;
	}
	else
		m_ContextZ80.m_OpcodeCycle = 16;
}

void Z80::CPU_INI() {
	m_ContextZ80.m_FuncPtrWrite(m_ContextZ80.m_RegisterHL.reg, m_ContextZ80.m_FuncPtrIORead(m_ContextZ80.m_RegisterBC.lo));
	
	CPU_16BIT_INC(m_ContextZ80.m_RegisterHL.reg, 0);

	CPU_8BIT_DEC(m_ContextZ80.m_RegisterBC.hi, 0);

	m_ContextZ80.m_OpcodeCycle = 16;
}

void Z80::CPU_INIR() {
	CPU_INI();

	// Call until b = 0
	if (m_ContextZ80.m_RegisterBC.hi != 0) {
		m_ContextZ80.m_OpcodeCycle = 21;
		m_ContextZ80.m_ProgramCounter -= 2;
	}
	else {
		m_ContextZ80.m_OpcodeCycle = 16;
	}
}

void Z80::CPU_IND() {
	m_ContextZ80.m_FuncPtrWrite(m_ContextZ80.m_RegisterHL.reg, m_ContextZ80.m_FuncPtrIORead(m_ContextZ80.m_RegisterBC.lo));

	CPU_16BIT_DEC(m_ContextZ80.m_RegisterHL.reg, 0);
	CPU_8BIT_DEC(m_ContextZ80.m_RegisterBC.hi, 0);

	m_ContextZ80.m_OpcodeCycle = 16;
}

void Z80::CPU_INDR() {
	CPU_IND();

	// Call until b = 0
	if (m_ContextZ80.m_RegisterBC.hi != 0) {
		m_ContextZ80.m_OpcodeCycle = 21;
		m_ContextZ80.m_ProgramCounter -= 2;
	}
	else {
		m_ContextZ80.m_OpcodeCycle = 16;
	}
}

void Z80::CPU_LDI() {
	BYTE hldata = m_ContextZ80.m_FuncPtrRead(m_ContextZ80.m_RegisterHL.reg);
	m_ContextZ80.m_FuncPtrWrite(m_ContextZ80.m_RegisterDE.reg, hldata);

	CPU_16BIT_INC(m_ContextZ80.m_RegisterBC.reg, 0);
	CPU_16BIT_INC(m_ContextZ80.m_RegisterDE.reg, 0);
	CPU_16BIT_INC(m_ContextZ80.m_RegisterHL.reg, 0);

	m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_N);
	m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_H);

	if (m_ContextZ80.m_RegisterBC.reg == 0)
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);
	else
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);

	hldata += m_ContextZ80.m_RegisterAF.hi;

	m_ContextZ80.m_OpcodeCycle = 16;
}

void Z80::CPU_LDIR() {
	CPU_LDI();

	// Keep calling until reg BC = 0
	if (m_ContextZ80.m_RegisterBC.reg != 0) {
		m_ContextZ80.m_ProgramCounter -= 2;
		m_ContextZ80.m_OpcodeCycle = 21;
	}
	else {
		m_ContextZ80.m_OpcodeCycle = 16;
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);
	}
}

void Z80::CPU_DJNZ() {
	m_ContextZ80.m_RegisterBC.hi--;
	m_ContextZ80.m_OpcodeCycle = 8;

	if (m_ContextZ80.m_RegisterBC.hi != 0) {
		SIGNED_BYTE n = (SIGNED_BYTE)m_ContextZ80.m_FuncPtrRead(m_ContextZ80.m_ProgramCounter);
		m_ContextZ80.m_ProgramCounter += n;
		m_ContextZ80.m_OpcodeCycle = 13;
	}

	m_ContextZ80.m_ProgramCounter++;
}

void Z80::CPU_LDD() {
	BYTE hldata = m_ContextZ80.m_FuncPtrRead(m_ContextZ80.m_RegisterHL.reg);
	m_ContextZ80.m_FuncPtrWrite(m_ContextZ80.m_RegisterDE.reg, hldata);

	m_ContextZ80.m_RegisterBC.reg--;
	m_ContextZ80.m_RegisterDE.reg--;
	m_ContextZ80.m_RegisterHL.reg--;

	if (m_ContextZ80.m_RegisterBC.reg == 0)
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);
	else
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);

	m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_H);
	m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_N);

	m_ContextZ80.m_OpcodeCycle = 16;
}

void Z80::CPU_LDDR() {
	CPU_LDD();

	// until bc = 0
	if (m_ContextZ80.m_RegisterBC.reg != 0) {
		m_ContextZ80.m_ProgramCounter -= 2;
		m_ContextZ80.m_OpcodeCycle = 21;
	}
	else {
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);
		m_ContextZ80.m_OpcodeCycle = 16;
	}
}

void Z80::CPU_LDA_I() {
	m_ContextZ80.m_OpcodeCycle = 9;
	m_ContextZ80.m_RegisterAF.hi = m_ContextZ80.m_RegisterI;

	if (m_ContextZ80.m_RegisterAF.hi == 0)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_Z);
	else
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_Z);

	m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_H);
	m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_N);

	if (m_ContextZ80.m_IFF2)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);
	else
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);

	if (TestBit(m_ContextZ80.m_RegisterAF.hi, 7))
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_S);
	else
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_S);
}

void Z80::CPU_LDA_R() {
	m_ContextZ80.m_OpcodeCycle = 9;
	m_ContextZ80.m_RegisterAF.hi = m_ContextZ80.m_RegisterR;

	if (m_ContextZ80.m_RegisterAF.hi == 0)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_Z);
	else
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_Z);

	m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_H);
	m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_N);

	if (m_ContextZ80.m_IFF2)
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);
	else
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_PV);

	if (TestBit(m_ContextZ80.m_RegisterAF.hi, 7))
		m_ContextZ80.m_RegisterAF.lo = BitSet(m_ContextZ80.m_RegisterAF.lo, FLAG_S);
	else
		m_ContextZ80.m_RegisterAF.lo = BitReset(m_ContextZ80.m_RegisterAF.lo, FLAG_S);
}

void Z80::CPU_REG_LOAD_NNN(WORD& reg) {
	WORD nn = ReadWord();
	m_ContextZ80.m_ProgramCounter += 2;
	m_ContextZ80.m_OpcodeCycle = 16;
	reg = m_ContextZ80.m_FuncPtrRead(nn + 1) << 8;
	reg |= m_ContextZ80.m_FuncPtrRead(nn);
}

void Z80::CPU_EXCHANGE(WORD& reg1, WORD& reg2) {
	m_ContextZ80.m_OpcodeCycle = 4;
	WORD temp = reg1;
	reg1 = reg2;
	reg2 = temp;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//											END OPCODES
/////////////////////////////////////////////////////////////////////////////////////////////////////


void Z80::IncreaseRReg()
{
	if ((m_ContextZ80.m_RegisterR & 127) == 127)
		m_ContextZ80.m_RegisterR = m_ContextZ80.m_RegisterR & 128;
	else
		m_ContextZ80.m_RegisterR++;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//											JUMP TABLE
/////////////////////////////////////////////////////////////////////////////////////////////////////

void Z80::ExecuteOpcode(const BYTE& opcode)
{
	IncreaseRReg();
	switch (opcode) {

	// Nop
	case 0x00: m_ContextZ80.m_OpcodeCycle = 4; break;

	default:
		printf("Unimplemented opcode %X\n", opcode);
		break;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//											END JUMP TABLE
/////////////////////////////////////////////////////////////////////////////////////////////////////

void Z80::InitDAATable() {
	// Initialize z, s, v flags
	for (int i = 0; i < 256; ++i)
	{
		BYTE zFlag = (i == 0) ? 0x40 : 0;
		BYTE sFlag = i & 0x80;
		BYTE vFlag = 0x04;
		for (int v = 128; v != 0; v >>= 1)
		{
			if (i & v)
				vFlag ^= 0x04;
		}
		m_ZSPTable[i] = zFlag | sFlag | vFlag;
	}

}