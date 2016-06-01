#include <bitset>
#include <iostream>

#include "CPU.hpp"

void CPU::init() {

	for (int x = 0; x < 0x10000; x++) {
		memory[x] = 0x0;
	}
	for (int x = 0; x < 8; x++) {
		PS[x] = false;
	}

	PC = 0x0;
	SP = 0x0;
	A  = 0x0;
	X  = 0x0;
	Y  = 0x0;

	nesAPU.init();

	return;
}

//http://homepage.ntlworld.com/cyborgsystems/CS_Main/6502/6502.htm
bool CPU::executeNextOpcode() {

	int tCnt;

	uint8_t opcode;
	uint8_t iByte2;
	uint8_t iByte3;

	opcode = memory[PC];
	iByte2 = memory[PC + 1];
	iByte3 = memory[PC + 2];

	switch (opcode) {

		case 0x69:		//ADC
		case 0x65:
		case 0x75:
		case 0x6D:
		case 0x7D:
		case 0x79:
		case 0x61:
		case 0x71: {

			uint8_t memByte;

			if (opcode == 0x69) {
				memByte = iByte2;
				tCnt = 2;
				PC += 2;
			} else if (opcode == 0x65) {
				memByte = memory[iByte2];
				tCnt = 3;
				PC += 2;
			} else if (opcode == 0x75) {
				memByte = memory[(iByte2 + X) & 0xFF];
				tCnt = 4;
				PC += 2;
			} else if (opcode == 0x6D) {
				uint16_t address;
				address = (iByte2 | (iByte3 << 8));
				memByte = memory[address];
				tCnt = 4;
				PC += 3;
			} else if (opcode == 0x7D) {
				uint16_t address;
				address = (iByte2 | (iByte3 << 8));
				memByte = memory[(address + X) & 0xFFFF];
				tCnt = 4;
				PC += 3;
			} else if (opcode == 0x79) {
				uint16_t address;
				address = (iByte2 | (iByte3 << 8));
				memByte = memory[(address + Y) & 0xFFFF];
				tCnt = 4;
				PC += 3;
			} else if (opcode == 0x61) {
				uint16_t address;
				address = (memory[(iByte2 + X) & 0xFF]) | ((memory[iByte3 + X] & 0xFF) << 8);
				memByte = memory[address];
				tCnt = 6;
				PC += 2;
			} else if (opcode == 0x71) {
				uint16_t address;
				address = (memory[iByte2]) | (memory[iByte3] << 8);
				memByte = memory[(address + Y) & 0xFFFF];
				tCnt = 5;
				PC += 2;
			} else {
				return false;
			}

			uint16_t total;
			total = A + memByte + PS[C];

			if ((A & 0x80) != (total & 0x80)) {
				PS[V] = true;
			} else {
				PS[V] = false;
			}

			if (A & 0x80) {
				PS[N] = true;
			} else {
				PS[N] = false;
			}

			if (total == 0) {
				PS[Z] = true;
			} else {
				PS[Z] = false;
			}

			if (PS[D]) {

				std::cout << "Binary coded decimal code detected!" << std::endl;

			} else {

				if (total > 255) {
					PS[C] = true;
				} else {
					PS[C] = false;
				}

			}

			A = total & 0xFF;

			break;
		}

		case 0x29:		//AND
		case 0x25:
		case 0x35:
		case 0x2D:
		case 0x3D:
		case 0x39:
		case 0x21:
		case 0x31: {

			uint8_t memByte;

			if (opcode == 0x29) {
				memByte = iByte2;
				tCnt = 2;
				PC += 2;
			} else if (opcode == 0x25) {
				memByte = memory[iByte2];
				tCnt = 2;
				PC += 2;
			} else if (opcode == 0x35) {
				memByte = memory[(iByte2 + X) & 0xFF];
				tCnt = 3;
				PC += 2;
			} else if (opcode == 0x2D) {
				uint16_t address;
				address = (iByte2 | (iByte3 << 8));
				memByte = memory[address];
				tCnt = 4;
				PC += 3;
			} else if (opcode == 0x3D) {
				uint16_t address;
				address = (iByte2 | (iByte3 << 8));
				memByte = memory[(address + X) & 0xFFFF];
				tCnt = 4;
				PC += 3;
			} else if (opcode == 0x39) {
				uint16_t address;
				address = (iByte2 | (iByte3 << 8));
				memByte = memory[(address + Y) & 0xFFFF];
				tCnt = 4;
				PC += 3;
			} else if (opcode == 0x21) {
				uint16_t address;
				address = (memory[(iByte2 + X) & 0xFF]) | ((memory[iByte3 + X] & 0xFF) << 8);
				memByte = memory[address];
				tCnt = 6;
				PC += 2;
			} else if (opcode == 0x31) {
				uint16_t address;
				address = (memory[iByte2]) | (memory[iByte3] << 8);
				memByte = memory[(address + Y) & 0xFFFF];
				tCnt = 5;
				PC += 2;
			} else {
				return false;
			}

			A = A & memByte;

			if (A & 0x80) {
				PS[N] = true;
			} else {
				PS[N] = false;
			}

			if (A == 0x0) {
				PS[Z] = true;
			} else {
				PS[Z] = false;
			}

			break;
		}

		case 0x0A:		//ASL
		case 0x06:
		case 0x16:
		case 0x0E:
		case 0x1E: {


			break;
		}


		default:
		break;
	}





	return true;

}