// Filename : myCPU.h
// Author   : Lauren Rush
// Ref.     : [3] Chip-8 Instruction set: http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#3.0

#pragma once

#include "Stack.h"
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
using namespace std;

class myCPU
{
public:
	myCPU();
	~myCPU();

	void dissassemble();
	
	///////////////////////////////////////////////////////////////////////////
	// MEMORY MAP, per [3]
	///////////////////////////////////////////////////////////////////////////
	char rsvd[0x0200];     // 0x0000 - 0x01FF: Reserved for interpretter
	char chip8ram[0x0DFF]; // 0x0200 - 0x0FFF: Program/Data RAM

	///////////////////////////////////////////////////////////////////////////
	// REGISTERS, per [3]
	///////////////////////////////////////////////////////////////////////////
	// - 16 general purpose 8-bit registers, Vx where x = 0 to F
	char regVx[0xF];
	// - I, 16-bit register stores memory addresses
	short regI;
	// - VF register used by instructions
	char regVF;
	// - DT (delay timer)
	char regDT;
	// - ST (sound timer)
	char regST;
	// - PC (program counter)
	short regPC;
	// - SP (stack pointer)
	char regSP;
	// - Stack: array of 16, 16-bit values
	Stack* regStack;

	///////////////////////////////////////////////////////////////////////////
	// KEYBOARD, per [3]
	///////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////
	// DISPLAY, per [3]
	///////////////////////////////////////////////////////////////////////////
};

