// Filename : myCPU.cpp
// Author   : Lauren Rush
// Refs     : [1] file i/o: http://www.cplusplus.com/doc/tutorial/files/
//            [2] two bytes at a time: http://www.cplusplus.com/forum/beginner/48160/
//            [3] Chip-8 Instruction set: http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#3.0
//            [4] iomanip: http://en.cppreference.com/w/cpp/io/manip/

#include "myCPU.h"

myCPU::~myCPU()
{
}

myCPU::myCPU()
{
	memset(rsvd, 0, 0x0200);     // 0x0000 - 0x01FF: Reserved for interpretter
	memset(chip8ram, 0, 0x0DFF); // 0x0200 - 0x0FFF: Program/Data RAM

	///////////////////////////////////////////////////////////////////////////
	// REGISTERS, per [3]
	///////////////////////////////////////////////////////////////////////////
	// - 16 general purpose 8-bit registers, Vx where x = 0 to F
	memset(regVx, 0, 0xF);
	// - I, 16-bit register stores memory addresses
	regI = 0x00;
	// - VF register used by instructions
	regVF = 0x0;
	// - DT (delay timer)
	regDT = 0x0;
	// - ST (sound timer)
	regST = 0x0;
	// - PC (program counter)
	regPC = 0x00;
	// - SP (stack pointer)
	regSP = 0x0;
	// - Stack: array of 16, 16-bit values
	regStack = new Stack(0x10);
}

void myCPU::dissassemble()
{
	unsigned int i = 0x0200;
	char msb, lsb;
	short inst = 0;
	ifstream myROM;
	ofstream myDIS;

	// (1) Open binary file [1]
	myROM.open(("..\\Debug\\INVADERS"), ios::binary);

	// Create output file for source assembly listing...
	myDIS.open("output.s");

	// File is valid, verify it has been successfully opened...
	if (myROM.is_open())
	{
		myDIS << "ADDRESS    OPCODE     MNEMONIC\n";
		
		// Until the end of the file, grab bytes...
		do {
			// Read two bytes at a time to [2]
			msb = myROM.get();
			lsb = myROM.get();

			inst = ((msb << 8) | lsb); // Reconstitute instruction, 2 bytes [2]

			// Parse address, opcode, mnemonic... [4]
			myDIS << hex << setw(4) << setfill('0') << i    << "       " 
				         << setw(4) << setfill('0') << inst << "       ";
			i += 2;
			//*****************************************************************
			// Parse Instructions [3]
			//*****************************************************************
			// (1) 0nnn - SYS addr
			//	Jump to a machine code routine at nnn.
			//	This instruction is only used on the old computers on which 
			//	Chip - 8 was originally implemented.  
			//  It is ignored by modern interpreters.

			//*****************************************************************
			// (2) 00E0 - CLS
			//	Clear the display.
			//*****************************************************************
			if (inst == 0x00E0)				
				myDIS << "CLS\n";

			//*****************************************************************
			// (3) 00EE - RET
			//	Return from a subroutine.
			//*****************************************************************
			if (inst == 0x00EE)
			{
				myDIS << "RET\n";
				//	The interpreter sets the program counter to the address at
				//	the top of the stack, then subtracts 1 from the stack ptr.
				// regPC = 
				// regSP--;
			}

			//*****************************************************************
			// (4) 1nnn - JP addr
			//	Jump to location nnn.
			//	The interpreter sets the program counter to nnn.
			//*****************************************************************
			if ((inst & 0xF000) == 0x1000)
			{
				//*************************************************************
				// Dissassembler parsing
				//*************************************************************
				myDIS << "JP " << setw(3) << setfill('0') << (0x0FFF & inst);
				if (i == 0x0202) // First instruction
				{
					myROM.seekg((0x0FFF & inst) - 0x200);
					i = (0x0FFF & inst);
				}
				//*************************************************************

				//*************************************************************
				// Emulator
				//*************************************************************
				regPC = (0x0FFF & inst);
			}

			//*****************************************************************
			// (5) 2nnn - CALL addr
			//	Call subroutine at nnn.
			//	The interpreter increments the stack pointer, then puts the  
			//  current PC on the top of the stack. The PC is then set to nnn.
			//*****************************************************************
			if ((inst & 0xF000) == 0x2000)
			{
				//*************************************************************
				// Dissassembler parsing
				//*************************************************************
				myDIS << "CALL " << setw(3) << setfill('0') << (0x0FFF & inst);
				//*************************************************************

				//*************************************************************
				// Emulator
				//*************************************************************
				regSP++;
				regStack->push(regPC);
			}

			//*****************************************************************
			// (6) 3xkk - SE Vx, byte
			//	Skip next instruction if Vx = kk.
			//	The interpreter compares register Vx to kk, and if they are 
			//  equal, increments the program counter by 2.
			//*****************************************************************
			if ((inst & 0xF000) == 0x3000)
				myDIS << "SE V" << ((0x0F00 & inst) >> 8) << ", " << (0x00FF & inst);

			//*****************************************************************
			// (7) 4xkk - SNE Vx, byte
			//	Skip next instruction if Vx != kk.
			//	The interpreter compares register Vx to kk, and if they are not
			//  equal, increments the program counter by 2.
			//*****************************************************************
			if ((inst & 0xF000) == 0x4000)
				myDIS << "SNE V" << ((0x0F00 & inst) >> 8) << ", " << (0x00FF & inst);

			//*****************************************************************
			// (8) 5xy0 - SE Vx, Vy
			//	Skip next instruction if Vx = Vy.
			//	The interpreter compares register Vx to register Vy, and if 
			//  they are equal, increments the program counter by 2.
			//*****************************************************************
			if ((inst & 0xF000) == 0x5000)
				myDIS << "SE V" << ((0x0F00 & inst) >> 8) << ", V" << ((0x00F0 & inst) >> 4);

			//*****************************************************************
			// (9) 6xkk - LD Vx, byte
			//	Set Vx = kk.
			//	The interpreter puts the value kk into register Vx.
			//*****************************************************************
			if ((inst & 0xF000) == 0x6000)
				myDIS << "LD V" << ((0x0F00 & inst) >> 8) << ", " << (0x00FF & inst);

			//*****************************************************************
			// (10)	7xkk - ADD Vx, byte
			//	Set Vx = Vx + kk.
			//	Adds the value kk to the value of register Vx, then stores the 
			//  result in Vx.
			//*****************************************************************
			if ((inst & 0xF000) == 0x7000)
				myDIS << "ADD V" << ((0x0F00 & inst) >> 8) << ", " << (0x00FF & inst);

			//*****************************************************************
			// (11)	8xy0 - LD Vx, Vy
			//	Set Vx = Vy.
			//	Stores the value of register Vy in register Vx.
			//*****************************************************************
			if ((inst & 0xF00F) == 0x8000)
				myDIS << "LD V" << ((0x0F00 & inst) >> 8) << ", V" << ((0x00F0 & inst) >> 4);

			//*****************************************************************
			// (12)	8xy1 - OR Vx, Vy
			//	Set Vx = Vx OR Vy.
			//	Performs a bitwise OR on the values of Vx and Vy, then stores 
			//  the result in Vx.
			//  A bitwise OR compares the corrseponding bits from two values, 
			//  and if either bit is 1, then the same bit in the result is 
			//  also 1. 
			//  Otherwise, it is 0.
			//*****************************************************************
			if ((inst & 0xF00F) == 0x8001)
				myDIS << "OR V" << ((0x0F00 & inst) >> 8) << ", V" << ((0x00F0 & inst) >> 4);

			//*****************************************************************
			// (13)	8xy2 - AND Vx, Vy
			//	Set Vx = Vx AND Vy.
			//	Performs a bitwise AND on the values of Vx and Vy, then stores 
			//  the result in Vx.
			//  A bitwise AND compares the corrseponding bits from two values, 
			//  and if both bits are 1, then the same bit in the result is 
			//  also 1. 
			//  Otherwise, it is 0.
			//*****************************************************************
			if ((inst & 0xF00F) == 0x8002)
				myDIS << "AND V" << ((0x0F00 & inst) >> 8) << ", V" << ((0x00F0 & inst) >> 4);

			//*****************************************************************
			// (14)	8xy3 - XOR Vx, Vy
			//	Set Vx = Vx XOR Vy.
			//	Performs a bitwise exclusive OR on the values of Vx and Vy, then stores the result in Vx.
			//  An exclusive OR compares the corrseponding bits from two values, and if the bits are not 
			//  both the same, then the corresponding bit in the result is set to 1. Otherwise, it is 0.
			//*****************************************************************
			if ((inst & 0xF00F) == 0x8003)
				myDIS << "XOR V" << ((0x0F00 & inst) >> 8) << ", V" << ((0x00F0 & inst) >> 4);

			//*****************************************************************
			// (15)	8xy4 - ADD Vx, Vy
			//	Set Vx = Vx + Vy, set VF = carry.
			//	The values of Vx and Vy are added together.
			//   If the result is greater than 8 bits(i.e., > 255, ) 
			//   VF is set to 1, otherwise 0. Only the lowest 8 bits of the result are kept, and stored in Vx.
			//*****************************************************************
			if ((inst & 0xF00F) == 0x8004)
				myDIS << "ADD V" << ((0x0F00 & inst) >> 8) << ", V" << ((0x00F0 & inst) >> 4);

			//*****************************************************************
			// (16)	8xy5 - SUB Vx, Vy
			//	Set Vx = Vx - Vy, set VF = NOT borrow.
			//	If Vx > Vy, then VF is set to 1, otherwise 0. Then Vy is subtracted from Vx, and the results stored in Vx.
			//*****************************************************************
			if ((inst & 0xF00F) == 0x8005)
				myDIS << "SUB V" << ((0x0F00 & inst) >> 8) << ", V" << ((0x00F0 & inst) >> 4);

			//*****************************************************************
			// (17)	8xy6 - SHR Vx{ , Vy }
			//  Set Vx = Vx SHR 1.
			//	If the least - significant bit of Vx is 1, then VF is set to 1, otherwise 0. Then Vx is divided by 2.
			//*****************************************************************
			if ((inst & 0xF00F) == 0x8006)
				myDIS << "SHR V" << ((0x0F00 & inst) >> 8) << "{ , V" << ((0x00F0 & inst) >> 4) << " }";

			//*****************************************************************
			// (18)	8xy7 - SUBN Vx, Vy
			//	Set Vx = Vy - Vx, set VF = NOT borrow.
			//	If Vy > Vx, then VF is set to 1, otherwise 0. Then Vx is subtracted from Vy, and the results stored in Vx.
			//*****************************************************************
			if ((inst & 0xF00F) == 0x8007)
				myDIS << "SUBN V" << ((0x0F00 & inst) >> 8) << ", V" << ((0x00F0 & inst) >> 4);

			//*****************************************************************
			// (19)	8xyE - SHL Vx{ , Vy }
			//  Set Vx = Vx SHL 1.
			//	If the most - significant bit of Vx is 1, then VF is set to 1, otherwise to 0. Then Vx is multiplied by 2.
			//*****************************************************************
			if ((inst & 0xF00F) == 0x800E)
				myDIS << "SHL V" << ((0x0F00 & inst) >> 8) << "{ , V" << ((0x00F0 & inst) >> 4) << " }";

			//*****************************************************************
			// (20)	9xy0 - SNE Vx, Vy
			//	Skip next instruction if Vx != Vy.
			//	The values of Vx and Vy are compared, and if they are not equal, the program counter is increased by 2.
			//*****************************************************************
			if ((inst & 0xF000) == 0x9000)
				myDIS << "SNE V" << ((0x0F00 & inst) >> 8) << ", V" << ((0x00F0 & inst) >> 4);

			//*****************************************************************
			// (21)	Annn - LD I, addr
			//	Set I = nnn.
			//	The value of register I is set to nnn.
			//*****************************************************************
			if ((inst & 0xF000) == 0xA000)
				myDIS << "LD I, " << setw(3) << setfill('0') << (0x0FFF & inst);

			//*****************************************************************
			// (22)	Bnnn - JP V0, addr
			//	Jump to location nnn + V0.
			//	The program counter is set to nnn plus the value of V0.
			//*****************************************************************
			if ((inst & 0xF000) == 0xB000)
				myDIS << "JP V0, " << setw(3) << setfill('0') << (0x0FFF & inst);

			//*****************************************************************
			// (23)	Cxkk - RND Vx, byte
			//	Set Vx = random byte AND kk.
			//	The interpreter generates a random number from 0 to 255, which is then ANDed with the value kk.
			//  The results are stored in Vx.See instruction 8xy2 for more information on AND.
			//*****************************************************************
			if ((inst & 0xF000) == 0xC000)
				myDIS << "RND V" << ((0x0F00 & inst) >> 8) << ", " << (0x00FF & inst);

			//*****************************************************************
			// (24)	Dxyn - DRW Vx, Vy, nibble
			//	Display n - byte sprite starting at memory location I at(Vx, Vy), set VF = collision.
			//	The interpreter reads n bytes from memory, starting at the address stored in I.
			//  These bytes are then displayed as sprites on screen at coordinates(Vx, Vy).
			//  Sprites are XORed onto the existing screen.  If this causes any pixels to be erased, VF is set to 1, otherwise it is set to 0. 
			//  If the sprite is positioned so part of it is outside the coordinates of the display, it wraps around to the opposite side of the screen.
			//  See instruction 8xy3 for more information on XOR, and section 2.4, Display, for more information on the Chip - 8 screen and sprites.
			//*****************************************************************
			if ((inst & 0xF000) == 0xD000)
				myDIS << "DRW V" << ((0x0F00 & inst) >> 8) << ", V" << ((0x00F0 & inst) >> 4) << ", " << (0x000F & inst);

			//*****************************************************************
			// (25) Ex9E - SKP Vx
			//	Skip next instruction if key with the value of Vx is pressed.
			//	Checks the keyboard, and if the key corresponding to the value 
			//  of Vx is currently in the down position, PC is increased by 2.
			//*****************************************************************
			if ((inst & 0xF000) == 0xE09E)
				myDIS << "SKP V" << ((0x0F00 & inst) >> 8);

			//*****************************************************************
			// (26) ExA1 - SKNP Vx
			//	Skip next instruction if key with the value of Vx is not pressed.
			//	Checks the keyboard, and if the key corresponding to the value of Vx is currently in the up position, PC is increased by 2.
			//*****************************************************************
			if ((inst & 0xF0FF) == 0xE0A1)
				myDIS << "SKNP V" << ((0x0F00 & inst) >> 8);

			//*****************************************************************
			// (27)	Fx07 - LD Vx, DT
			//	Set Vx = delay timer value.
			//	The value of DT is placed into Vx.
			//*****************************************************************
			if ((inst & 0xF0FF) == 0xF007)
				myDIS << "LD V" << ((0x0F00 & inst) >> 8) << ", DT";

			//*****************************************************************
			// (28)	Fx0A - LD Vx, K
			//	Wait for a key press, store the value of the key in Vx.
			//	All execution stops until a key is pressed, then the value of that key is stored in Vx.
			//*****************************************************************
			if ((inst & 0xF0FF) == 0xF00A)
				myDIS << "LD V" << ((0x0F00 & inst) >> 8) << ", K";

			//*****************************************************************
			// (29)	Fx15 - LD DT, Vx
			//	Set delay timer = Vx.
			//	DT is set equal to the value of Vx.
			//*****************************************************************
			if ((inst & 0xF0FF) == 0xF015)
				myDIS << "LD DT, V" << ((0x0F00 & inst) >> 8);

			//*****************************************************************
			// (30)	Fx18 - LD ST, Vx
			//	Set sound timer = Vx.
			//	ST is set equal to the value of Vx.
			//*****************************************************************
			if ((inst & 0xF0FF) == 0xF018)
				myDIS << "LD ST, V" << ((0x0F00 & inst) >> 8);

			//*****************************************************************
			// (31)	Fx1E - ADD I, Vx
			//	Set I = I + Vx.
			//	The values of I and Vx are added, and the results are stored in I.
			//*****************************************************************
			if ((inst & 0xF0FF) == 0xF01E)
				myDIS << "ADD I, V" << ((0x0F00 & inst) >> 8);

			//*****************************************************************
			// (32)	Fx29 - LD F, Vx
			//	Set I = location of sprite for digit Vx.
			//	The value of I is set to the location for the hexadecimal sprite corresponding to the value of Vx.
			//   See section 2.4, Display, for more information on the Chip - 8 hexadecimal font.
			//*****************************************************************
			if ((inst & 0xF0FF) == 0xF029)
				myDIS << "LD F, V" << ((0x0F00 & inst) >> 8);

			//*****************************************************************
			// (33)	Fx33 - LD B, Vx
			//	Store BCD representation of Vx in memory locations I, I + 1, and I + 2.
			//	The interpreter takes the decimal value of Vx, and places the hundreds digit in memory at location 
			//   in I, the tens digit at location I + 1, and the ones digit at location I + 2.
			//*****************************************************************
			if ((inst & 0xF0FF) == 0xF033)
				myDIS << "LD B, V" << ((0x0F00 & inst) >> 8);

			//*****************************************************************
			// (34)	Fx55 - LD[I], Vx
			//	Store registers V0 through Vx in memory starting at location I.
			//	The interpreter copies the values of registers V0 through Vx into memory, starting at the address in I.
			//*****************************************************************
			if ((inst & 0xF0FF) == 0xF055)
				myDIS << "LD[I], V" << ((0x0F00 & inst) >> 8);

			//*****************************************************************
			// (35)	Fx65 - LD Vx, [I]
			//	Read registers V0 through Vx from memory starting at location I.
			//	The interpreter reads values from memory starting at location I into registers V0 through Vx.
			//*****************************************************************
			if ((inst & 0xF0FF) == 0xF065)
				myDIS << "LD V" << ((0x0F00 & inst) >> 8) << ", [I]";

			myDIS << endl;
		} while (myROM.peek() != EOF);

		// Close file, when done...
		myROM.close();
		myDIS.close();
	}
	else // Invalid file or filename...
	{
		cout << "Error opening file" << endl;
	}
}