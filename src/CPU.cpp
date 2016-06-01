#include <iostream>
#include <bitset>

#include "CPU.hpp"

static bool getBit(uint8_t num, int bitNum) {

	if (bitNum == 0) {
		return (num & 0x1) ? true : false;
	} else if (bitNum == 1) {
		return (num & 0x2) ? true : false;
	} else if (bitNum == 2) {
		return (num & 0x4) ? true : false;
	} else if (bitNum == 3) {
		return (num & 0x8) ? true : false;
	} else if (bitNum == 4) {
		return (num & 0x10) ? true : false;
	} else if (bitNum == 5) {
		return (num & 0x20) ? true : false;
	} else if (bitNum == 6) {
		return (num & 0x40) ? true : false;
	} else if (bitNum == 7) {
		return (num & 0x80) ? true : false;
	} else {
		std::cerr << "Error, called getBit() with invalid number" << std::endl;
		return false;
	}
}

static int binToBcd(uint8_t hex) {
	int decimal;
	decimal = (hex & 0xF) + 10 * ((hex & 0xF0) >> 4);
	return decimal;
}

static uint8_t bcdToBin(int num) {
	int higher = num / 10;
	int lower = num % 10;
	uint8_t bin;
	bin = (higher << 4) | lower;
	return bin;
}

void CPU::init() {

	for (int x = 0; x < 0x10000; x++) {
		memory[x] = 0x0;
	}
	for (int x = 0; x < 8; x++) {
		PS[x] = false;
	}
	for (int x = 0; x < 0x100; x++) {
		stack[x] = 0x0;
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

			PS[V] = (getBit(A, 7) != getBit(total, 7)) ? true : false;
			PS[N] = getBit(A, 7);
			PS[Z] = (total == 0) ? true : false;

			if (PS[D]) {
				total = binToBcd(A) + binToBcd(memByte) + PS[C];
				PS[C] = (total > 99) ? true : false;
				A = bcdToBin(total) & 0xFF;
			} else {
				PS[C] = (total > 255) ? true : false;
				A = total & 0xFF;
			}
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
				address = ((iByte2 | (iByte3 << 8)) + X) & 0xFFFF;
				memByte = memory[address];
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

			PS[N] = getBit(A, 7);
			PS[Z] = (A == 0x0) ? true : false;

			break;
		}

		case 0x0A:		//ASL
		case 0x06:
		case 0x16:
		case 0x0E:
		case 0x1E: {

			if (opcode == 0x0A) {

				PS[C] = getBit(A, 7);
				A = ((A << 1) & 0xFE);
				PS[N] = getBit(A, 7);
				PS[Z] = (A == 0) ? true : false;

				PC++;
				tCnt = 2;

			} else if (opcode == 0x06) {

				uint8_t address;
				address = iByte2;

				PS[C] = getBit(memory[address], 7);
				memory[address] = ((memory[address] << 1) & 0xFE);
				PS[N] = getBit(memory[address], 7);
				PS[Z] = (memory[address] == 0) ? true : false;

				PC += 2;
				tCnt = 5;

			} else if (opcode == 0x16) {

				uint8_t address;
				address = (iByte2 + X) & 0xFF;

				PS[C] = getBit(memory[address], 7);
				memory[address] = ((memory[address] << 1) & 0xFE);
				PS[N] = getBit(memory[address], 7);
				PS[Z] = (memory[address] == 0) ? true : false;

				PC += 2;
				tCnt = 6;

			} else if (opcode == 0x0E) {

				uint16_t address;
				address = (iByte2 | (iByte3 << 8));

				PS[C] = getBit(memory[address], 7);
				memory[address] = ((memory[address] << 1) & 0xFE);
				PS[N] = getBit(memory[address], 7);
				PS[Z] = (memory[address] == 0) ? true : false;

				PC += 3;
				tCnt = 6;

			} else if (opcode == 0x1E) {

				uint16_t address;
				address = ((iByte2 | (iByte3 << 8)) + X) & 0xFFFF;

				PS[C] = getBit(memory[address], 7);
				memory[address] = ((memory[address] << 1) & 0xFE);
				PS[N] = getBit(memory[address], 7);
				PS[Z] = (memory[address] == 0) ? true : false;

				PC += 3;
				tCnt = 7;

			} else {
				return false;
			}

			break;
		}

		case 0x90:			//BCC
		if (PS[C] == 0) {
			PC += iByte2;
		} else {
			PC += 2;
		}
		tCnt = 2;
		break;

		case 0xB0:			//BCS
		if (PS[C]) {
			PC += iByte2;
		} else {
			PC += 2;
		}
		tCnt = 2;
		break;

		case 0xF0:			//BEQ
		if (PS[Z]) {
			PC += iByte2;
		} else {
			PC += 2;
		}
		tCnt = 2;
		break;

		case 0x24:			//BIT
		case 0x2C: {

			uint8_t memByte;

			if (opcode == 0x24) {
				memByte = memory[iByte2];
				PC += 2;
				tCnt = 3;
			} else if (opcode == 0x2C) {
				memByte = memory[(iByte2 | (iByte3 << 8))];
				PC += 3;
				tCnt = 4;
			} else {
				return false;
			}

			uint8_t num;
			num = A & memByte;

			PS[N] = getBit(num, 7);
			PS[V] = getBit(num, 6);

			PS[Z] = (num == 0) ? true : false;

			break;
		}

		case 0x30:			//BMI
		if (PS[N]) {
			PC += iByte2;
		} else {
			PC += 2;
		}
		tCnt = 2;
		break;

		case 0xD0:			//BNE
		if (PS[Z] == 0) {
			PC += iByte2;
		} else {
			PC += 2;
		}
		tCnt = 2;
		break;

		case 0x10:			//BPL
		if (PS[N] == 0) {
			PC += iByte2;
		} else {
			PC += 2;
		}
		tCnt = 2;
		break;

		case 0x00:			//BRK: todo
		break;

		case 0x50:			//BVC
		if (PS[V] == 0) {
			PC += iByte2;
		} else {
			PC += 2;
		}
		tCnt = 2;
		break;

		case 0x70:			//BVS
		if (PS[V]) {
			PC += iByte2;
		} else {
			PC += 2;
		}
		tCnt = 2;
		break;

		case 0x18:			//CLC
		PS[C] = false;
		tCnt = 2;
		PC++;
		break;

		case 0xD8:			//CLD
		PS[D] = false;
		tCnt = 2;
		PC++;
		break;

		case 0x58:			//CLI
		PS[I] = false;
		tCnt = 2;
		PC++;
		break;

		case 0xB8:			//CLV
		PS[V] = false;
		tCnt = 2;
		PC++;
		break;

		case 0xC9:			//CMP
		case 0xC5:
		case 0xD5:
		case 0xCD:
		case 0xDD:
		case 0xD9:
		case 0xC1:
		case 0xD1: {

			uint8_t memByte;

			if (opcode == 0xC9) {
				memByte = iByte2;
				tCnt = 2;
				PC += 2;
			} else if (opcode == 0xC5) {
				memByte = memory[iByte2];
				tCnt = 3;
				PC += 2;
			} else if (opcode == 0xD5) {
				memByte = memory[(iByte2 + X) & 0xFF];
				tCnt = 4;
				PC += 2;
			} else if (opcode == 0xCD) {
				uint16_t address;
				address = (iByte2 | (iByte3 << 8));
				memByte = memory[address];
				tCnt = 4;
				PC += 3;
			} else if (opcode == 0xDD) {
				uint16_t address;
				address = (iByte2 | (iByte3 << 8));
				memByte = memory[(address + X) & 0xFFFF];
				tCnt = 4;
				PC += 3;
			} else if (opcode == 0xD9) {
				uint16_t address;
				address = (iByte2 | (iByte3 << 8));
				memByte = memory[(address + Y) & 0xFFFF];
				tCnt = 4;
				PC += 3;
			} else if (opcode == 0xC1) {
				uint16_t address;
				address = (memory[(iByte2 + X) & 0xFF]) | ((memory[iByte3 + X] & 0xFF) << 8);
				memByte = memory[address];
				tCnt = 6;
				PC += 2;
			} else if (opcode == 0xD1) {
				uint16_t address;
				address = (memory[iByte2]) | (memory[iByte3] << 8);
				memByte = memory[(address + Y) & 0xFFFF];
				tCnt = 5;
				PC += 2;
			} else {
				return false;
			}

			int num;
			num = A - memByte;
			PS[N] = getBit(num, 7);

			PS[C] = (A >= memByte) ? true : false;
			PS[Z] = (num == 0) ? true : false;

			break;
		}

		case 0xE0:			//CPX
		case 0xE4:
		case 0xEC: {

			uint8_t memByte;

			if (opcode == 0xE0) {
				memByte = iByte2;
				PC += 2;
				tCnt = 2;
			} else if (opcode == 0xE4) {
				memByte = memory[iByte2];
				PC += 2;
				tCnt = 3;
			} else if (opcode == 0xEC) {
				memByte = memory[(iByte2 | (iByte3 << 8))];
				PC += 3;
				tCnt = 4;
			} else {
				return false;
			}

			int total;
			total = X - memByte;
			PS[N] = (total & 0x80) ? true : false;
			PS[C] = (X >= memByte) ? true : false;
			PS[Z] = (total == 0) ? 1 : 0;

			break;
		}

		case 0xC0:			//CPY
		case 0xC4:
		case 0xCC: {

			uint8_t memByte;

			if (opcode == 0xC0) {
				memByte = iByte2;
				PC += 2;
				tCnt = 2;
			} else if (opcode == 0xC4) {
				memByte = memory[iByte2];
				PC += 2;
				tCnt = 3;
			} else if (opcode == 0xCC) {
				memByte = memory[(iByte2 | (iByte3 << 8))];
				PC += 3;
				tCnt = 4;
			} else {
				return false;
			}

			int total;
			total = Y - memByte;
			PS[N] = (total & 0x80) ? true : false;
			PS[C] = (Y >= memByte) ? true : false;
			PS[Z] = (total == 0) ? 1 : 0;

			break;
		}

		case 0xC6:			//DEC
		case 0xD6:
		case 0xCE:
		case 0xDE: {

			uint16_t address;

			if (opcode == 0xC6) {
				address = iByte2;
				PC += 2;
				tCnt = 5;
			} else if (opcode == 0xD6) {
				address = ((iByte2 + X) & (0xFF));
				PC += 2;
				tCnt = 6;
			} else if (opcode == 0xCE) {
				address = (iByte2 | (iByte3 << 8));
				PC += 3;
				tCnt = 6;
			} else if (opcode == 0xDE) {
				address = (((iByte2 | (iByte3 << 8)) + X) & (0xFFFF));
				PC += 3;
				tCnt = 7;
			} else {
				return false;
			}

			memory[address] = ((memory[address] - 1) & 0xFF);
			PS[N] = getBit(memory[address], 7);
			PS[Z] = (memory[address] == 0) ? true : false;

			break;
		}

		case 0xCA:			//DEX
		PC++;
		tCnt = 2;
		X = X - 1;
		PS[Z] = (X == 0) ? true : false;
		PS[N] = getBit(X, 7);
		break;

		case 0x88:			//DEY
		PC++;
		tCnt = 2;
		Y = Y - 1;
		PS[Z] = (Y == 0) ? true : false;
		PS[N] = getBit(Y, 7);
		break;

		case 0x49:			//EOR
		case 0x45:
		case 0x55:
		case 0x4D:
		case 0x5D:
		case 0x59:
		case 0x41:
		case 0x51: {


			uint8_t memByte;

			if (opcode == 0x49) {
				memByte = iByte2;
				tCnt = 2;
				PC += 2;
			} else if (opcode == 0x45) {
				memByte = memory[iByte2];
				tCnt = 3;
				PC += 2;
			} else if (opcode == 0x55) {
				memByte = memory[(iByte2 + X) & 0xFF];
				tCnt = 4;
				PC += 2;
			} else if (opcode == 0x4D) {
				uint16_t address;
				address = (iByte2 | (iByte3 << 8));
				memByte = memory[address];
				tCnt = 4;
				PC += 3;
			} else if (opcode == 0x5D) {
				uint16_t address;
				address = ((iByte2 | (iByte3 << 8)) + X) & 0xFFFF;
				memByte = memory[address];
				tCnt = 4;
				PC += 3;
			} else if (opcode == 0x59) {
				uint16_t address;
				address = (iByte2 | (iByte3 << 8));
				memByte = memory[(address + Y) & 0xFFFF];
				tCnt = 4;
				PC += 3;
			} else if (opcode == 0x41) {
				uint16_t address;
				address = (memory[(iByte2 + X) & 0xFF]) | ((memory[iByte3 + X] & 0xFF) << 8);
				memByte = memory[address];
				tCnt = 6;
				PC += 2;
			} else if (opcode == 0x51) {
				uint16_t address;
				address = (memory[iByte2]) | (memory[iByte3] << 8);
				memByte = memory[(address + Y) & 0xFFFF];
				tCnt = 5;
				PC += 2;
			} else {
				return false;
			}

			A = A ^ memByte;
			PS[N] = getBit(A, 7);
			PS[Z] = (A == 0) ? true : false;


			break;
		}

		case 0xE6:				//INC
		case 0xF6:
		case 0xEE:
		case 0xFE: {


			uint16_t address;

			if (opcode == 0xE6) {
				address = iByte2;
				PC += 2;
				tCnt = 5;
			} else if (opcode == 0xF6) {
				address = ((iByte2 + X) & (0xFF));
				PC += 3;
				tCnt = 6;
			} else if (opcode == 0xEE) {
				address = (iByte2 | (iByte3 << 8));
				PC += 3;
				tCnt = 6;
			} else if (opcode == 0xFE) {
				address = (((iByte2 | (iByte3 << 8)) + X) & (0xFFFF));
				PC += 3;
				tCnt = 7;
			} else {
				return false;
			}

			memory[address] = ((memory[address] + 1) & 0xFF);
			PS[N] = getBit(memory[address], 7);
			PS[Z] = (memory[address] == 0) ? true : false;

			break;
		}

		case 0xE8:				//INX
		PC++;
		tCnt = 2;
		X = X + 1;
		PS[Z] = (X == 0) ? true : false;
		PS[N] = getBit(X, 7);
		break;

		case 0xC8:				//INY
		PC++;
		tCnt = 2;
		Y = Y + 1;
		PS[Z] = (Y == 0) ? true : false;
		PS[N] = getBit(Y, 7);
		break;

		case 0x4C:				//JMP
		case 0x6C: {

			uint16_t memByte;

			if (opcode == 0x4C) {
				tCnt = 3;
				memByte = (iByte2 | (iByte3 << 8));
			} else if (opcode == 0x6C) {
				tCnt = 5;
				memByte = memory[(iByte2 | (iByte3 << 8))];
			} else {
				return false;
			}

			PC = memByte;
			break;
		}

		case 0x20: {			//JSR
			uint16_t store;
			store = PC - 1;

			uint8_t low, high;

			low = store & 0xFF;
			high = (store & 0xFF00) >> 8;

			stack[SP] = high;
			SP--;
			stack[SP] = low;
			SP--;

			PC = (iByte2 | (iByte3 << 8));
			break;
		}

		case 0xA9:			//LDA
		case 0xA5:
		case 0xB5:
		case 0xAD:
		case 0xBD:
		case 0xB9:
		case 0xA1:
		case 0xB1: {

			uint8_t memByte;

			if (opcode == 0xA9) {
				memByte = iByte2;
				tCnt = 2;
				PC += 2;
			} else if (opcode == 0xA5) {
				memByte = memory[iByte2];
				tCnt = 3;
				PC += 2;
			} else if (opcode == 0xB5) {
				memByte = memory[(iByte2 + X) & 0xFF];
				tCnt = 4;
				PC += 2;
			} else if (opcode == 0xAD) {
				uint16_t address;
				address = (iByte2 | (iByte3 << 8));
				memByte = memory[address];
				tCnt = 4;
				PC += 3;
			} else if (opcode == 0xBD) {
				uint16_t address;
				address = ((iByte2 | (iByte3 << 8)) + X) & 0xFFFF;
				memByte = memory[address];
				tCnt = 4;
				PC += 3;
			} else if (opcode == 0xB9) {
				uint16_t address;
				address = (iByte2 | (iByte3 << 8));
				memByte = memory[(address + Y) & 0xFFFF];
				tCnt = 4;
				PC += 3;
			} else if (opcode == 0xA1) {
				uint16_t address;
				address = (memory[(iByte2 + X) & 0xFF]) | ((memory[iByte3 + X] & 0xFF) << 8);
				memByte = memory[address];
				tCnt = 6;
				PC += 2;
			} else if (opcode == 0xB1) {
				uint16_t address;
				address = (((memory[iByte2]) | (memory[iByte3] << 8) + Y) & 0xFFFF);
				memByte = memory[address];
				tCnt = 5;
				PC += 2;
			} else {
				return false;
			}

			A = memByte;
			PS[N] = getBit(A, 7);
			PS[Z] = (A == 0) ? true : false;

			break;
		}


		case 0xA2:				//LDX
		case 0xA6:
		case 0xB6:
		case 0xAE:
		case 0xBE: {

			uint8_t memByte;

			if (opcode == 0xA2) {
				memByte = iByte2;
				PC+=2;
				tCnt = 2;
			} else if (opcode == 0xA6) {
				memByte = memory[iByte2];
				PC+=2;
				tCnt = 3;
			} else if (opcode == 0xB6) {
				memByte = memory[(iByte2 + Y) & 0xFF];
				PC +=2;
				tCnt = 4;
			} else if (opcode == 0xAE) {
				memByte = memory[(iByte2 | (iByte3 << 8))];
				PC += 3;
				tCnt = 4;
			} else if (opcode == 0xBE) {
				memByte = memory[(((iByte2 | (iByte3 << 8)) + Y) & 0xFFFF)];
				PC += 3;
				tCnt = 4;
			} else {
				return false;
			}

			X = memByte;
			PS[N] = getBit(X, 7);
			PS[Z] = (X == 0) ? true : false;

			break;
		}

		case 0xA0:				//LDY
		case 0xA4:
		case 0xB4:
		case 0xAC:
		case 0xBC: {

			uint8_t memByte;

			if (opcode == 0xA0) {
				memByte = iByte2;
				PC+=2;
				tCnt = 2;
			} else if (opcode == 0xA4) {
				memByte = memory[iByte2];
				PC+=2;
				tCnt = 3;
			} else if (opcode == 0xB4) {
				memByte = memory[(iByte2 + X) & 0xFF];
				PC +=2;
				tCnt = 4;
			} else if (opcode == 0xAC) {
				memByte = memory[(iByte2 | (iByte3 << 8))];
				PC += 3;
				tCnt = 4;
			} else if (opcode == 0xBC) {
				memByte = memory[(((iByte2 | (iByte3 << 8)) + X) & 0xFFFF)];
				PC += 3;
				tCnt = 4;
			} else {
				return false;
			}

			Y = memByte;
			PS[N] = getBit(Y, 7);
			PS[Z] = (Y == 0) ? true : false;

			break;
		}

		case 0x4A:				//LSR
		case 0x46:
		case 0x56:
		case 0x4E:
		case 0x5E: {

			if (opcode == 0x4A) {
				PC++;
				tCnt = 2;

				PS[N] = 0;
				PS[C] = getBit(A, 0);

				A = (A >> 1) & 0x7F;
				PS[Z] = (A == 0) ? true : false;

			} else {

				uint16_t address;

				if (opcode == 0x46) {
					PC+=2;
					tCnt = 5;
					address = iByte2;
				} else if (opcode == 0x56) {
					PC += 2;
					tCnt = 6;
					address = (iByte2 + X) & 0xFF;
				} else if (opcode == 0x4E) {
					PC += 3;
					tCnt = 6;
					address = (iByte2 | (iByte3 << 8));
				} else if (opcode == 0x5E) {
					PC += 3;
					tCnt = 7;
					address = ((iByte2 | (iByte3 << 8)) + X) & 0xFFFF;
				} else {
					return false;
				}

				PS[N] = 0;
				PS[C] = getBit(memory[address], 0);

				memory[address] = (memory[address] >> 1) & 0x7F;
				PS[Z] = (memory[address] == 0) ? true : false;
			}

			break;
		}

		case 0xEA:				//NOP
		PC++;
		tCnt = 2;
		break;

		case 0x09:				//ORA: todo
		case 0x05:
		case 0x15:
		case 0x0D:
		case 0x1D:
		case 0x19:
		case 0x01:
		case 0x11: {




			break;
		}

		case 0x48:
		PC++;
		tCnt = 3;
		stack[SP] = A;
		SP--;
		break;

		case 0x08: {				//PHP
			PC++;
			tCnt = 3;
			uint8_t memByte;
			memByte = 0;

			if (PS[C]) memByte |= 0x1;
			if (PS[Z]) memByte |= 0x2;
			if (PS[I]) memByte |= 0x4;
			if (PS[D]) memByte |= 0x8;
			if (PS[B]) memByte |= 0x10;
			if (PS[V]) memByte |= 0x40;
			if (PS[N]) memByte |= 0x80;

			stack[SP] = memByte;
			SP--;

			break;
		}

		case 0x68: {				//PLA
			PC++;
			tCnt = 4;
			SP++;
			A = memory[SP];
			PS[N] = getBit(A, 7);
			PS[Z] = (A == 0) ? true : false;
			break;
		}

		case 0x28: {				//PLP
			PC++;
			tCnt = 4;
			SP++;
			uint8_t memByte = memory[SP];

			PS[N] = getBit(memByte, 7);
			PS[V] = getBit(memByte, 6);
			PS[B] = getBit(memByte, 4);
			PS[D] = getBit(memByte, 3);
			PS[I] = getBit(memByte, 2);
			PS[Z] = getBit(memByte, 1);
			PS[C] = getBit(memByte, 0);
			break;
		}


		case 0x2A:				//ROL: todo
		case 0x26:
		case 0x36:
		case 0x2E:
		case 0x3E: {



			break;
		}

		case 0x6A:				//ROR: todo
		case 0x66:
		case 0x76:
		case 0x6E:
		case 0x7E: {




			break;
		}

		case 0x40: {				//RTI
			SP--;
			tCnt = 6;
			uint8_t memByte;
			memByte = memory[SP];

			PS[N] = getBit(memByte, 7);
			PS[V] = getBit(memByte, 6);
			PS[B] = getBit(memByte, 4);
			PS[D] = getBit(memByte, 3);
			PS[I] = getBit(memByte, 2);
			PS[Z] = getBit(memByte, 1);
			PS[C] = getBit(memByte, 0);

			SP--;
			uint16_t low = memory[SP];
			SP--;
			uint16_t high = memory[SP] << 8;
			PC = high | low;
			break;
		}

		case 0x60: {				//RTS
			SP++;
			tCnt = 6;
			uint16_t low = memory[SP];
			SP++;
			uint16_t high = memory[SP] << 8;
			PC = (high | low) + 1;
			break;
		}

		case 0xE9:				//SBC
		case 0xE5:
		case 0xF5:
		case 0xED:
		case 0xFD:
		case 0xF9:
		case 0xE1:
		case 0xF1: {

			uint8_t memByte;

			if (opcode == 0xE9) {
				memByte = iByte2;
				tCnt = 2;
				PC += 2;
			} else if (opcode == 0xE5) {
				memByte = memory[iByte2];
				tCnt = 3;
				PC += 2;
			} else if (opcode == 0xF5) {
				memByte = memory[(iByte2 + X) & 0xFF];
				tCnt = 4;
				PC += 2;
			} else if (opcode == 0xED) {
				uint16_t address;
				address = (iByte2 | (iByte3 << 8));
				memByte = memory[address];
				tCnt = 4;
				PC += 3;
			} else if (opcode == 0xFD) {
				uint16_t address;
				address = ((iByte2 | (iByte3 << 8)) + X) & 0xFFFF;
				memByte = memory[address];
				tCnt = 4;
				PC += 3;
			} else if (opcode == 0xF9) {
				uint16_t address;
				address = (iByte2 | (iByte3 << 8));
				memByte = memory[(address + Y) & 0xFFFF];
				tCnt = 4;
				PC += 3;
			} else if (opcode == 0xE1) {
				uint16_t address;
				address = (memory[(iByte2 + X) & 0xFF]) | ((memory[iByte3 + X] & 0xFF) << 8);
				memByte = memory[address];
				tCnt = 6;
				PC += 2;
			} else if (opcode == 0xF1) {
				uint16_t address;
				address = (((memory[iByte2]) | (memory[iByte3] << 8) + Y) & 0xFFFF);
				memByte = memory[address];
				tCnt = 5;
				PC += 2;
			} else {
				return false;
			}

			int total;

			if (PS[D]) {

				total = binToBcd(A) - binToBcd(memByte) - (!PS[C]);
				PS[V] = (total > 99 || total < 0) ? true : false;
				total %= 100;
				A = bcdToBin(total);
				std::cout << "Binary subtraction detected, this needs to be fixed" << std::endl;

			} else {

				total = A - memByte - (!PS[C]);
				PS[V] = (total > 127 || total < -128) ? true : false;
				A = total & 0xFF;

			}

			PS[C] = (total == 0) ? true : false;
			PS[N] = getBit(total, 7);
			PS[Z] = (total == 0) ? 1 : 0;

			break;
		}

		case 0x38:				//SEC
		PC++;
		tCnt = 2;
		PS[C] = true;
		break;

		case 0xF8:				//SED
		PC++;
		tCnt = 2;
		PS[D] = true;
		break;

		case 0x78:				//SEI
		PC++;
		tCnt = 2;
		PS[I] = true;
		break;

		case 0x85:				//STA
		case 0x95:
		case 0x8D:
		case 0x9D:
		case 0x99:
		case 0x81:
		case 0x91: {

			uint16_t address;

			if (opcode == 0x85) {
				address = iByte2;
				PC += 2;
				tCnt = 3;
			} else if (opcode == 0x95) {
				address = ((iByte2 + X) & 0xFF);
				PC += 2;
				tCnt = 4;
			} else if (opcode == 0x8D) {
				address = (iByte2 | (iByte3 << 8));
				PC += 3;
				tCnt = 4;
			} else if (opcode == 0x9D) {
				address = (((iByte2 | (iByte3 << 8)) + X) & 0xFFFF);
				PC += 3;
				tCnt = 5;
			} else if (opcode == 0x99) {
				address = (((iByte2 | (iByte3 << 8)) + Y) & 0xFFFF);
				PC += 3;
				tCnt = 5;
			} else if (opcode == 0x81) {
				address = (memory[(iByte2 + X) & 0xFF]) | ((memory[iByte3 + X] & 0xFF) << 8);
				PC += 2;
				tCnt = 6;
			} else if (opcode == 0x91) {
				address = (((memory[iByte2]) | (memory[iByte3] << 8) + Y) & 0xFFFF);
				PC += 2;
				tCnt = 6;
			} else {
				return false;
			}

			memory[address] = A;
			break;
		}

		case 0x86:				//STX
		case 0x96:
		case 0x8E: {

			uint16_t address;

			if (opcode == 0x86) {
				PC += 2;
				tCnt = 3;
				address = iByte2;
			} else if (opcode == 0x96) {
				PC += 2;
				tCnt = 4;
				address = ((iByte2 + X) & 0xFF);
			} else if (opcode == 0x8E) {
				PC += 3;
				tCnt = 4;
				address = (iByte2 | (iByte3 << 8));
			} else {
				return false;
			}

			memory[address] = X;
			break;

			break;
		}

		case 0x84:				//STY
		case 0x94:
		case 0x8C: {

			uint16_t address;

			if (opcode == 0x84) {
				PC += 2;
				tCnt = 3;
				address = iByte2;
			} else if (opcode == 0x94) {
				PC += 2;
				tCnt = 4;
				address = ((iByte2 + X) & 0xFF);
			} else if (opcode == 0x8C) {
				PC += 3;
				tCnt = 4;
				address = (iByte2 | (iByte3 << 8));
			} else {
				return false;
			}

			memory[address] = Y;
			break;
		}

		case 0xAA:				//TAX
		PC++;
		tCnt = 2;
		X = A;
		PS[N] = getBit(X, 7);
		PS[Z] = (X == 0) ? true : false;
		break;

		case 0xA8:				//TAY
		PC++;
		tCnt = 2;
		Y = A;
		PS[N] = getBit(Y, 7);
		PS[Z] = (Y == 0) ? true : false;
		break;

		case 0xBA:				//TSX
		PC++;
		tCnt = 2;
		X = SP;
		PS[N] = getBit(X, 7);
		PS[Z] = (X == 0) ? true : false;
		break;

		case 0x8A:				//TXA
		PC++;
		tCnt = 2;
		A = X;
		PS[N] = getBit(A, 7);
		PS[Z] = (A == 0) ? true : false;
		break;

		case 0x9A:				//TXS
		PC++;
		tCnt = 2;
		SP = X;
		break;

		case 0x98:				//TYA
		PC++;
		tCnt = 2;
		A = Y;
		PS[N] = getBit(A, 7);
		PS[Z] = (A == 0) ? true : false;
		break;

		default:
		std::cerr << "Unrecognized opcode " << opcode << std::endl;
		return false;
	}

	return true;

}
