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
				total = binToBcd(A) + binToBcd(memByte) + PS[C];
				if (total > 99) {
					PS[C] = true;
				} else {
					PS[C] = false;
				}
				A = bcdToBin(total) & 0xFF;
			} else {
				if (total > 255) {
					PS[C] = true;
				} else {
					PS[C] = false;
				}
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

			if (opcode == 0x0A) {

				if (A & 0x80) {
					PS[C] = true;
				} else {
					PS[C] = false;
				}

				A = (A << 1) & 0xFE;

				if (A & 0x80) {
					PS[N] = true;
				} else {
					PS[N] = false;
				}

				if (A == 0) {
					PS[Z] = true;
				} else {
					PS[Z] = false;
				}
				PC++;
				tCnt = 2;

			} else if (opcode == 0x06) {

				uint8_t address;
				address = iByte2;

				if (memory[address] & 0x80) {
					PS[C] = true;
				} else {
					PS[C] = false;
				}

				memory[address] = (memory[address] << 1) & 0xFE;

				if (memory[address] & 0x80) {
					PS[N] = true;
				} else {
					PS[N] = false;
				}

				if (memory[address] == 0) {
					PS[Z] = true;
				} else {
					PS[Z] = false;
				}
				PC += 2;
				tCnt = 5;

			} else if (opcode == 0x16) {

				uint8_t address;
				address = (iByte2 + X) & 0xFF;

				if (memory[address] & 0x80) {
					PS[C] = true;
				} else {
					PS[C] = false;
				}

				memory[address] = (memory[address] << 1) & 0xFE;

				if (memory[address] & 0x80) {
					PS[N] = true;
				} else {
					PS[N] = false;
				}

				if (memory[address] == 0) {
					PS[Z] = true;
				} else {
					PS[Z] = false;
				}
				PC += 2;
				tCnt = 6;

			} else if (opcode == 0x0E) {

				uint16_t address;
				address = (iByte2 | (iByte3 << 8));

				if (memory[address] & 0x80) {
					PS[C] = true;
				} else {
					PS[C] = false;
				}

				memory[address] = (memory[address] << 1) & 0xFE;

				if (memory[address] & 0x80) {
					PS[N] = true;
				} else {
					PS[N] = false;
				}

				if (memory[address] == 0) {
					PS[Z] = true;
				} else {
					PS[Z] = false;
				}
				PC += 3;
				tCnt = 6;

			} else if (opcode == 0x1E) {

				uint16_t address;
				address = ((iByte2 | (iByte3 << 8)) + X) & 0xFFFF;

				if (memory[address] & 0x80) {
					PS[C] = true;
				} else {
					PS[C] = false;
				}

				memory[address] = (memory[address] << 1) & 0xFE;

				if (memory[address] & 0x80) {
					PS[N] = true;
				} else {
					PS[N] = false;
				}

				if (memory[address] == 0) {
					PS[Z] = true;
				} else {
					PS[Z] = false;
				}
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

			if (num == 0) {
				PS[Z] = true;
			} else {
				PS[Z] = false;
			}
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

			if (A >= memByte) {
				PS[C] = true;
			} else {
				PS[C] = false;
			}

			if (num == 0) {
				PS[Z] = true;
			} else {
				PS[Z] = false;
			}

			break;
		}




		default:
		break;
	}

	return true;

}
