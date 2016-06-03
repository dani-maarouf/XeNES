#include <iostream>
#include <bitset>

#include "CPU.hpp"

static bool getBit(uint8_t, int);
static int binToBcd(uint8_t);
static uint8_t bcdToBin(int);

uint8_t CPU::getByte(uint16_t address) {
	return cpuMem[address];
}

bool CPU::setByte(uint16_t address, uint8_t byte) {
	cpuMem[address] = byte;
}

void CPU::init() {

	for (int x = 0; x < 0x10000; x++) {
		setByte(x, 0x0);
	}
	for (int x = 0; x < 8; x++) {
		PS[x] = false;
	}
	for (int x = 0; x < 0x100; x++) {
		stack[x] = 0x0;
	}

	PC = 0x0;
	SP = 0xFF;
	A  = 0x0;
	X  = 0x0;
	Y  = 0x0;

	
	PS[I] = true;

	nesAPU.init();

	return;
}

//http://homepage.ntlworld.com/cyborgsystems/CS_Main/6502/6502.htm
bool CPU::executeNextOpcode(bool debug, bool verbose) {

	int tCnt;

	uint8_t opcode;
	uint8_t iByte2;
	uint8_t iByte3;

	opcode = getByte(PC);
	iByte2 = getByte(PC + 1);
	iByte3 = getByte(PC + 2);

	std::cout << std::hex << std::uppercase;

	uint8_t P;
	P = 0;

	if (PS[C]) P |= 0x1;
	if (PS[Z]) P |= 0x2;
	if (PS[I]) P |= 0x4;
	if (PS[D]) P |= 0x8;
	if (PS[V]) P |= 0x40;
	if (PS[N]) P |= 0x80;

	P |= 0x20;

	if (verbose) {
		std::cout << std::hex << std::uppercase << "PC:" << (int) PC << " SP:" << (int) SP << " X:" << (int) X;
		std::cout << std::hex << std::uppercase <<" Y:" << (int) Y << " A:" << (int) A << " N:" << PS[N] << " V:" << PS[V];
		std::cout << std::hex << std::uppercase <<" D:" << PS[D] << " I:" << PS[I] << " Z:" << PS[Z] << " C:" << PS[C] << " P:" << (int) P << "\t\t\t"; 
	}

	switch (opcode) {

		case 0x69:		//ADC
		case 0x65:
		case 0x75:
		case 0x6D:
		case 0x7D:
		case 0x79:
		case 0x61:
		case 0x71: {

			if (debug) std::cout << "ADC ";

			uint8_t memByte;

			if (opcode == 0x69) {							//Immediate
				memByte = iByte2;
				tCnt = 2;
				PC += 2;

				if (debug) std::cout << "#$" << std::hex << std::uppercase << (unsigned int) iByte2;
				
			} else if (opcode == 0x65) {					//Zero Page
				memByte = getByte(iByte2);
				tCnt = 3;
				PC += 2;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0x75) {					//Zero Page,X 
				memByte = getByte((iByte2 + X) & 0xFF );
				tCnt = 4;
				PC += 2;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0x6D) {					//Absolute
				uint16_t address;
				address = (iByte2 | (iByte3 << 8));
				memByte = getByte(address);
				tCnt = 4;
				PC += 3;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3;

			} else if (opcode == 0x7D) {					//Absolute,X
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + X) & 0xFFFF);
				memByte = getByte(address);
				tCnt = 4;
				PC += 3;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3 << ",X";

			} else if (opcode == 0x79) {					//Absolute,Y
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + Y) & 0xFFFF);
				memByte = getByte(address);
				tCnt = 4;
				PC += 3;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3 << ",Y";

			} else if (opcode == 0x61) {					//(Indirect,X)
				uint16_t address;

				uint16_t low;
				uint16_t high;
				low = getByte((iByte2 + X) & 0xFF);
				high = getByte(iByte2 + 1 + X & 0xFF) << 8;
				address = low | high;
				memByte = getByte(address);
				tCnt = 6;
				PC += 2;

				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X)";

			} else if (opcode == 0x71) {					//(Indirect),Y

				uint16_t address;
				address = ((((getByte(iByte2)) | (getByte(iByte2 + 1 & 0xFF)) << 8) + Y) & 0xFFFF);
				memByte = getByte(address);
				tCnt = 5;
				PC += 2;

				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << "),Y";


			} else {
				return false;
			}

			uint16_t total;
			total = A + memByte + PS[C];


			int8_t num1, num2;
			num1 = (int8_t) A;
			num2 = (int8_t) memByte;

			if (num1 + num2 + PS[C] < -128 || num1 + num2 + PS[C] > 127) {
				PS[V] = true;
			} else {
				PS[V] = false;
			}


			PS[C] = (total > 255) ? true : false;
			A = total & 0xFF;
			PS[Z] = (A == 0) ? true : false;


			PS[N] = getBit(A, 7);

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

			if (debug) std::cout << "AND ";

			uint8_t memByte;

			if (opcode == 0x29) {
				memByte = iByte2;
				tCnt = 2;
				PC += 2;
				if (debug) std::cout << "#$" << std::hex << std::uppercase << (unsigned int) iByte2;
			} else if (opcode == 0x25) {
				memByte = getByte(iByte2);
				tCnt = 2;
				PC += 2;
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;
			} else if (opcode == 0x35) {
				memByte = getByte( (iByte2 + X) & 0xFF );
				tCnt = 3;
				PC += 2;
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";
			} else if (opcode == 0x2D) {
				uint16_t address;
				address = (iByte2 | (iByte3 << 8));
				memByte = getByte(address);
				tCnt = 4;
				PC += 3;
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3;
			} else if (opcode == 0x3D) {
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + X) & 0xFFFF);
				memByte = getByte(address);
				tCnt = 4;
				PC += 3;
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3 << ",X";
			} else if (opcode == 0x39) {
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + Y) & 0xFFFF);
				memByte = getByte(address);
				tCnt = 4;
				PC += 3;
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3 << ",Y";
			} else if (opcode == 0x21) {
				uint16_t address;

				uint16_t low;
				uint16_t high;
				low = getByte((iByte2 + X) & 0xFF);
				high = (getByte(iByte2 + 1 + X & 0xFF)) << 8;
				address = low | high;
				memByte = getByte(address);
				tCnt = 6;
				PC += 2;
				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X)";
			} else if (opcode == 0x31) {

				uint16_t address;
				address = ((((getByte(iByte2)) | (getByte(iByte2 + 1 & 0xFF)) << 8) + Y) & 0xFFFF);
				memByte = getByte(address);

				tCnt = 5;
				PC += 2;

				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << "),Y";
			} else {
				return false;
			}

			A = A & memByte;

			PS[N] = getBit(A, 7);
			PS[Z] = (A == 0) ? true : false;

            break;
		}

		case 0x0A:		//ASL
		case 0x06:
		case 0x16:
		case 0x0E:
		case 0x1E: {

			if (debug) std::cout << "ASL ";

			if (opcode == 0x0A) {

				PS[C] = getBit(A, 7);
				A = ((A << 1) & 0xFE);
				PS[N] = getBit(A, 7);
				PS[Z] = (A == 0) ? true : false;

				PC++;
				tCnt = 2;

				if (debug) std::cout << "A";

			} else if (opcode == 0x06) {

				uint8_t address;
				address = iByte2;

				PS[C] = getBit(getByte(address), 7);

				setByte(address, ((getByte(address) << 1) & 0xFE));

				PS[N] = getBit(getByte(address), 7);
				PS[Z] = (getByte(address) == 0) ? true : false;

				PC += 2;
				tCnt = 5;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0x16) {

				uint8_t address;
				address = (iByte2 + X) & 0xFF;

				PS[C] = getBit(getByte(address), 7);

				setByte(address, ((getByte(address) << 1) & 0xFE));

				PS[N] = getBit(getByte(address), 7);
				PS[Z] = (getByte(address) == 0) ? true : false;

				PC += 2;
				tCnt = 6;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0x0E) {

				uint16_t address;
				address = (iByte2 | (iByte3 << 8));

				PS[C] = getBit(getByte(address), 7);

				setByte(address, ((getByte(address) << 1) & 0xFE));

				PS[N] = getBit(getByte(address), 7);
				PS[Z] = (getByte(address) == 0) ? true : false;

				PC += 3;
				tCnt = 6;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3;

			} else if (opcode == 0x1E) {

				uint16_t address;
				address = ((iByte2 | (iByte3 << 8)) + X) & 0xFFFF;

				PS[C] = getBit(getByte(address), 7);

				setByte(address, ((getByte(address) << 1) & 0xFE));

				PS[N] = getBit(getByte(address), 7);
				PS[Z] = (getByte(address) == 0) ? true : false;

				PC += 3;
				tCnt = 7;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3 << ",X";

			} else {
				return false;
			}

            break;
		}

		case 0x90: {			//BCC
			if (debug) std::cout << "BCC $" << std::hex << std::uppercase << (unsigned int) iByte2;
			if (PS[C] == 0) {
				PC += (int8_t) iByte2 + 2;
			} else {
				PC += 2;
			}
			tCnt = 2;
            break;
		}

		case 0xB0: {			//BCS
			if (debug) std::cout << "BCS $" << std::hex << std::uppercase << (unsigned int) iByte2;
			if (PS[C]) {
				PC += (int8_t) iByte2 + 2;
			} else {
				PC += 2;
			}
			tCnt = 2;
            break;
		}

		case 0xF0: {			//BEQ
			if (debug) std::cout << "BEQ $" << std::hex << std::uppercase << (unsigned int) iByte2;
			if (PS[Z]) {
				PC += (int8_t) iByte2 + 2;
			} else {
				PC += 2;
			}
			tCnt = 2;
            break;
		}

		case 0x24:			//BIT
		case 0x2C: {

			if (debug) std::cout << "BIT $";

			uint8_t memByte;

			if (opcode == 0x24) {
				memByte = getByte(iByte2);
				PC += 2;
				tCnt = 3;
				if (debug) std::cout << std::hex << std::uppercase << (unsigned int) iByte2;
			} else if (opcode == 0x2C) {
				memByte = getByte((iByte2 | (iByte3 << 8)));
				PC += 3;
				tCnt = 4;
				if (debug) std::cout << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3;
			} else {
				return false;
			}

			uint8_t num;
			num = A & memByte;

			PS[N] = getBit(memByte, 7);
			PS[V] = getBit(memByte, 6);

			PS[Z] = (num == 0) ? true : false;

            break;
		}

		case 0x30: {			//BMI
			if (debug) std::cout << "BMI $" << std::hex << std::uppercase << (unsigned int) iByte2;
			if (PS[N]) {
				PC += (int8_t) iByte2 + 2;
			} else {
				PC += 2;
			}
			tCnt = 2;
            break;
		}

		case 0xD0: {			//BNE
			if (debug) std::cout << "BNE $" << std::hex << std::uppercase << (unsigned int) iByte2;
			if (PS[Z] == 0) {
				PC += (int8_t) iByte2 + 2;
			} else {
				PC += 2;
			}
			tCnt = 2;
            break;
		}

		case 0x10: {			//BPL
			if (debug) std::cout << "BPL $" << std::hex << std::uppercase << (unsigned int) iByte2;
			if (PS[N] == 0) {
				PC += (int8_t) iByte2 + 2;
			} else {
				PC += 2;
			}
			tCnt = 2;
            break;
		}

		case 0x00: {			//BRK

			if (debug) std::cout << "BRK";

			tCnt = 7;
			PC += 2;

			/*

			PC += 2;

			uint8_t low, high;
			low = PC & 0xFF;
			high = (PC & 0xFF00) >> 8;

			stack[SP] = high;
			SP--;
			stack[SP] = low;
			SP--;

			uint8_t memByte;

			if (PS[C]) memByte |= 0x1;
			if (PS[Z]) memByte |= 0x2;
			if (PS[I]) memByte |= 0x4;
			if (PS[D]) memByte |= 0x8;
			if (PS[V]) memByte |= 0x40;
			if (PS[N]) memByte |= 0x80;

			stack[SP] = memByte | 0x10;
			SP--;

			low = getByte(0xFFFE);
			high = getByte(0xFFFF);

			PC = (high << 8) | low;

			*/

			if (debug) std::cout << std::endl;

			break;
		}

		case 0x50: {			//BVC
			if (debug) std::cout << "BVC $" << std::hex << std::uppercase << (unsigned int) iByte2;
			if (PS[V] == 0) {
				PC += (int8_t) iByte2 + 2;
			} else {
				PC += 2;
			}
			tCnt = 2;
            break;
		}

		case 0x70: {			//BVS
			if (debug) std::cout << "BVS $" << std::hex << std::uppercase << (unsigned int) iByte2;
			if (PS[V]) {
				PC += (int8_t) iByte2 + 2;
			} else {
				PC += 2;
			}
			tCnt = 2;
            break;
		}

		case 0x18: {			//CLC
			if (debug) std::cout << "CLC";
			PS[C] = false;
			tCnt = 2;
			PC++;
            break;
		}

		case 0xD8: {			//CLD
			if (debug) std::cout << "CLD";
			PS[D] = false;
			tCnt = 2;
			PC++;
            break;
		}

		case 0x58: {			//CLI
			if (debug) std::cout << "CLI";
			PS[I] = false;
			tCnt = 2;
			PC++;
            break;
		}

		case 0xB8: {			//CLV
			if (debug) std::cout << "CLV";
			PS[V] = false;
			tCnt = 2;
			PC++;
            break;
		}

		case 0xC9:			//CMP
		case 0xC5:
		case 0xD5:
		case 0xCD:
		case 0xDD:
		case 0xD9:
		case 0xC1:
		case 0xD1: {

			if (debug) std::cout << "CMP ";

			uint8_t memByte;

			if (opcode == 0xC9) {
				memByte = iByte2;
				tCnt = 2;
				PC += 2;

				if (debug) std::cout << "#$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xC5) {
				memByte = getByte(iByte2);
				tCnt = 3;
				PC += 2;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xD5) {
				memByte = getByte( (iByte2 + X) & 0xFF );
				tCnt = 4;
				PC += 2;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0xCD) {
				uint16_t address;
				address = (iByte2 | (iByte3 << 8));
				memByte = getByte(address);
				tCnt = 4;
				PC += 3;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3;

			} else if (opcode == 0xDD) {
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + X) & 0xFFFF);
				memByte = getByte(address);
				tCnt = 4;
				PC += 3;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3 << ",X";

			} else if (opcode == 0xD9) {
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + Y) & 0xFFFF);
				memByte = getByte(address);
				tCnt = 4;
				PC += 3;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3 << ",Y";

			} else if (opcode == 0xC1) {
				uint16_t address;

				uint16_t low;
				uint16_t high;
				low = getByte((iByte2 + X) & 0xFF);
				high = (getByte(iByte2 + 1 + X & 0xFF)) << 8;
				address = low | high;
				memByte = getByte(address);
				tCnt = 6;
				PC += 2;

				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X)";

			} else if (opcode == 0xD1) {
				uint16_t address;
				address = ((((getByte(iByte2)) | (getByte(iByte2 + 1 & 0xFF)) << 8) + Y) & 0xFFFF);
				memByte = getByte(address);
				tCnt = 5;
				PC += 2;

				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << "),Y";

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

			if (debug) std::cout << "CPX ";

			uint8_t memByte;

			if (opcode == 0xE0) {
				memByte = iByte2;
				PC += 2;
				tCnt = 2;

				if (debug) std::cout << "#$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xE4) {
				memByte = getByte(iByte2);
				PC += 2;
				tCnt = 3;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xEC) {
				memByte = getByte((iByte2 | (iByte3 << 8)));
				PC += 3;
				tCnt = 4;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3;

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

			if (debug) std::cout << "CPY ";

			uint8_t memByte;

			if (opcode == 0xC0) {
				memByte = iByte2;
				PC += 2;
				tCnt = 2;

				if (debug) std::cout << "#$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xC4) {
				memByte = getByte(iByte2);
				PC += 2;
				tCnt = 3;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xCC) {
				memByte = getByte((iByte2 | (iByte3 << 8)));
				PC += 3;
				tCnt = 4;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3;

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

			if (debug) std::cout << "DEC ";

			uint16_t address;

			if (opcode == 0xC6) {
				address = iByte2;
				PC += 2;
				tCnt = 5;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xD6) {
				address = ((iByte2 + X) & 0xFF);
				PC += 2;
				tCnt = 6;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0xCE) {
				address = (iByte2 | (iByte3 << 8));
				PC += 3;
				tCnt = 6;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3;

			} else if (opcode == 0xDE) {
				address = (((iByte2 | (iByte3 << 8)) + X) & (0xFFFF));
				PC += 3;
				tCnt = 7;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3 << ",X";

			} else {
				return false;
			}

			setByte(address, ((getByte(address) - 1) & 0xFF));
			PS[N] = getBit(getByte(address), 7);
			PS[Z] = (getByte(address) == 0) ? true : false;

            break;
		}

		case 0xCA: {			//DEX

			if (debug) std::cout << "DEX";

			PC++;
			tCnt = 2;
			X = X - 1;
			PS[Z] = (X == 0) ? true : false;
			PS[N] = getBit(X, 7);
            break;
		}

		case 0x88: {			//DEY

			if (debug) std::cout << "DEY";

			PC++;
			tCnt = 2;
			Y--;
			PS[Z] = (Y == 0) ? true : false;
			PS[N] = getBit(Y, 7);
            break;
		}

		case 0x49:			//EOR
		case 0x45:
		case 0x55:
		case 0x4D:
		case 0x5D:
		case 0x59:
		case 0x41:
		case 0x51: {

			if (debug) std::cout << "EOR ";

			uint8_t memByte;

			if (opcode == 0x49) {
				memByte = iByte2;
				tCnt = 2;
				PC += 2;

				if (debug) std::cout << "#$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0x45) {
				memByte = getByte(iByte2);
				tCnt = 3;
				PC += 2;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0x55) {
				memByte = getByte( (iByte2 + X) & 0xFF );
				tCnt = 4;
				PC += 2;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0x4D) {
				uint16_t address;
				address = (iByte2 | (iByte3 << 8));
				memByte = getByte(address);
				tCnt = 4;
				PC += 3;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3;

			} else if (opcode == 0x5D) {
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + X) & 0xFFFF);
				memByte = getByte(address);
				tCnt = 4;
				PC += 3;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3 << ",X";

			} else if (opcode == 0x59) {
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + Y) & 0xFFFF);
				memByte = getByte(address);
				tCnt = 4;
				PC += 3;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3 << ",Y";

			} else if (opcode == 0x41) {
				uint16_t address;

				uint16_t low;
				uint16_t high;
				low = getByte((iByte2 + X) & 0xFF);
				high = (getByte(iByte2 + 1 + X & 0xFF)) << 8;
				address = low | high;
				memByte = getByte(address);
				tCnt = 6;
				PC += 2;

				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X)";

			} else if (opcode == 0x51) {
				uint16_t address;
				address = ((((getByte(iByte2)) | (getByte(iByte2 + 1 & 0xFF)) << 8) + Y) & 0xFFFF);
				memByte = getByte(address);
				tCnt = 5;
				PC += 2;

				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << "),Y";

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

			if (debug) std::cout << "INC ";

			uint16_t address;

			if (opcode == 0xE6) {
				address = iByte2;
				PC += 2;
				tCnt = 5;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xF6) {
				address = ((iByte2 + X) & 0xFF);
				PC += 3;
				tCnt = 6;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0xEE) {
				address = (iByte2 | (iByte3 << 8));
				PC += 3;
				tCnt = 6;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3;

			} else if (opcode == 0xFE) {
				address = (((iByte2 | (iByte3 << 8)) + X) & 0xFFFF);
				PC += 3;
				tCnt = 7;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3 << ",X";

			} else {
				return false;
			}

			setByte(address, ((getByte(address) + 1) & 0xFF));
			PS[N] = getBit(getByte(address), 7);
			PS[Z] = (getByte(address) == 0) ? true : false;

            break;
		}

		case 0xE8: {				//INX
			if (debug) std::cout << "INX";
			PC++;
			tCnt = 2;
			X = X + 1;
			PS[Z] = (X == 0) ? true : false;
			PS[N] = getBit(X, 7);
            break;
		}

		case 0xC8: {				//INY
			if (debug) std::cout << "INY";
			PC++;
			tCnt = 2;
			Y = Y + 1;
			PS[Z] = (Y == 0) ? true : false;
			PS[N] = getBit(Y, 7);
            break;
		}

		case 0x4C:				//JMP
		case 0x6C: {

			if (debug) std::cout << "JMP ";

			uint16_t address;

			if (opcode == 0x4C) {
				tCnt = 3;
				address = (iByte2 | (iByte3 << 8));
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3;
			} else if (opcode == 0x6C) {
				tCnt = 5;
				address = getByte((iByte2 | (iByte3 << 8)));
				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3 << ")";
			} else {
				return false;
			}

			PC = address;
            break;
		}

		case 0x20: {			//JSR

			if (debug) std::cout << "JSR $" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3;
			uint16_t store;
			PC += 3;
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

			if (debug) std::cout << "LDA ";

			uint8_t memByte;

			if (opcode == 0xA9) {
				memByte = iByte2;
				tCnt = 2;
				PC += 2;

				if (debug) std::cout << "#$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xA5) {
				memByte = getByte(iByte2);
				tCnt = 3;
				PC += 2;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xB5) {
				memByte = getByte((iByte2 + X) & 0xFF);
				tCnt = 4;
				PC += 2;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0xAD) {
				uint16_t address;
				address = (iByte2 | (iByte3 << 8));
				memByte = getByte(address);
				tCnt = 4;
				PC += 3;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3;


			} else if (opcode == 0xBD) {
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + X) & 0xFFFF);
				memByte = getByte(address);
				tCnt = 4;
				PC += 3;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3 << ",X";

			} else if (opcode == 0xB9) {
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + Y) & 0xFFFF);
				memByte = getByte(address);
				tCnt = 4;
				PC += 3;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3 << ",Y";

			} else if (opcode == 0xA1) {
				uint16_t address;

				uint16_t low;
				uint16_t high;
				low = getByte((iByte2 + X) & 0xFF);
				high = (getByte(iByte2 + 1 + X & 0xFF)) << 8;
				address = low | high;
				memByte = getByte(address);
				tCnt = 6;
				PC += 2;

				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X)";

			} else if (opcode == 0xB1) {
				uint16_t address;
				address = ((((getByte(iByte2)) | (getByte(iByte2 + 1 & 0xFF)) << 8) + Y) & 0xFFFF);
				memByte = getByte(address);
				tCnt = 5;
				PC += 2;

				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << "),Y";

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

			if (debug) std::cout << "LDX ";

			uint8_t memByte;

			if (opcode == 0xA2) {
				memByte = iByte2;
				PC+=2;
				tCnt = 2;

				if (debug) std::cout << "#$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xA6) {
				memByte = getByte(iByte2);
				PC+=2;
				tCnt = 3;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xB6) {
				memByte = getByte((iByte2 + Y) & 0xFF);
				PC +=2;
				tCnt = 4;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",Y";

			} else if (opcode == 0xAE) {
				memByte = getByte((iByte2 | (iByte3 << 8)));
				PC += 3;
				tCnt = 4;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3;

			} else if (opcode == 0xBE) {
				memByte = getByte((((iByte2 | (iByte3 << 8)) + Y) & 0xFFFF));
				PC += 3;
				tCnt = 4;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3 << ",X";

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

			if (debug) std::cout << "LDY ";

			uint8_t memByte;

			if (opcode == 0xA0) {
				memByte = iByte2;
				PC+=2;
				tCnt = 2;

				if (debug) std::cout << "#$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xA4) {
				memByte = getByte(iByte2);
				PC+=2;
				tCnt = 3;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;


			} else if (opcode == 0xB4) {
				memByte = getByte((iByte2 + X) & 0xFF);
				PC +=2;
				tCnt = 4;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0xAC) {
				memByte = getByte((iByte2 | (iByte3 << 8)));
				PC += 3;
				tCnt = 4;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3;

			} else if (opcode == 0xBC) {
				memByte = getByte((((iByte2 | (iByte3 << 8)) + X) & 0xFFFF));
				PC += 3;
				tCnt = 4;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3 << ",X";

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

			if (debug) std::cout << "LSR ";

			if (opcode == 0x4A) {
				PC++;
				tCnt = 2;

				PS[N] = 0;
				PS[C] = getBit(A, 0);

				A = (A >> 1) & 0x7F;
				PS[Z] = (A == 0) ? true : false;

				if (debug) std::cout << "A";

			} else {

				uint16_t address;

				if (opcode == 0x46) {
					PC+=2;
					tCnt = 5;
					address = iByte2;

					if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

				} else if (opcode == 0x56) {
					PC += 2;
					tCnt = 6;
					address = (iByte2 + X) & 0xFF;

					if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

				} else if (opcode == 0x4E) {
					PC += 3;
					tCnt = 6;
					address = (iByte2 | (iByte3 << 8));

					if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3;

				} else if (opcode == 0x5E) {
					PC += 3;
					tCnt = 7;
					address = ((iByte2 | (iByte3 << 8)) + X) & 0xFFFF;

					if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3 << ",X";

				} else {
					return false;
				}

				PS[N] = 0;
				PS[C] = getBit(getByte(address), 0);

				setByte(address, (getByte(address) >> 1) & 0x7F);
				PS[Z] = (getByte(address) == 0) ? true : false;
			}

            break;
		}

		case 0xEA: {				//NOP
			if (debug) std::cout << "NOP";
			PC++;
			tCnt = 2;
            break;
		}

		case 0x09:				//ORA
		case 0x05:
		case 0x15:
		case 0x0D:
		case 0x1D:
		case 0x19:
		case 0x01:
		case 0x11: {

			if (debug) std::cout << "ORA ";

			uint8_t memByte;

			if (opcode == 0x09) {
				memByte = iByte2;
				tCnt = 2;
				PC += 2;

				if (debug) std::cout << "#$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0x05) {
				memByte = getByte(iByte2);
				tCnt = 2;
				PC += 2;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0x15) {
				memByte = getByte((iByte2 + X) & 0xFF);
				tCnt = 3;
				PC += 2;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0x0D) {
				uint16_t address;
				address = (iByte2 | (iByte3 << 8));
				memByte = getByte(address);
				tCnt = 4;
				PC += 3;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3;

			} else if (opcode == 0x1D) {
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + X) & 0xFFFF);
				memByte = getByte(address);
				tCnt = 4;
				PC += 3;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3 << ",X";

			} else if (opcode == 0x19) {
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + Y) & 0xFFFF);
				memByte = getByte(address);
				tCnt = 4;
				PC += 3;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3 << ",Y";

			} else if (opcode == 0x01) {
				uint16_t address;

				uint16_t low;
				uint16_t high;
				low = getByte((iByte2 + X) & 0xFF);
				high = (getByte(iByte2 + 1 + X & 0xFF)) << 8;
				address = low | high;
				memByte = getByte(address);
				tCnt = 6;
				PC += 2;

				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X)";

			} else if (opcode == 0x11) {
				uint16_t address;
				address = ((((getByte(iByte2)) | (getByte(iByte2 + 1 & 0xFF)) << 8) + Y) & 0xFFFF);
				memByte = getByte(address);
				tCnt = 5;
				PC += 2;

				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << "),Y";

			} else {
				return false;
			}

			A = (A | memByte);
			PS[N] = getBit(A, 7);
			PS[Z] = (A == 0) ? true : false;

            break;
		}

		case 0x48: {				//PHA

			if (debug) std::cout << "PHA";

			PC++;
			tCnt = 3;
			stack[SP] = A;
			SP--;
            break;
		}

		case 0x08: {				//PHP

			if (debug) std::cout << "PHP";

			PC++;
			tCnt = 3;
			uint8_t memByte;
			memByte = 0;

			if (PS[C]) memByte |= 0x1;
			if (PS[Z]) memByte |= 0x2;
			if (PS[I]) memByte |= 0x4;
			if (PS[D]) memByte |= 0x8;
			if (PS[V]) memByte |= 0x40;
			if (PS[N]) memByte |= 0x80;

			memByte |= 0x20;

			stack[SP] = memByte;
			SP--;

            break;
		}

		case 0x68: {				//PLA

			if (debug) std::cout << "PLA";

			PC++;
			tCnt = 4;
			SP++;
			A = stack[SP];
			PS[N] = getBit(A, 7);
			PS[Z] = (A == 0) ? true : false;
            break;
		}

		case 0x28: {				//PLP

			if (debug) std::cout << "PLP";

			PC++;
			tCnt = 4;
			SP++;
			uint8_t memByte = stack[SP];

			PS[N] = getBit(memByte, 7);
			PS[V] = getBit(memByte, 6);
			PS[D] = getBit(memByte, 3);
			PS[I] = getBit(memByte, 2);
			PS[Z] = getBit(memByte, 1);
			PS[C] = getBit(memByte, 0);
            break;
		}


		case 0x2A:				//ROL
		case 0x26:
		case 0x36:
		case 0x2E:
		case 0x3E: {

			if (debug) std::cout << "ROL ";


			if (opcode == 0x2A) {
				PC++;
				tCnt = 2;

				bool store;
				store = getBit(A, 7);

				A = (A << 1) & 0xFE;
				A |= PS[C];
				PS[C] = store;
				PS[Z] = (A == 0) ? true : false;
				PS[N] = getBit(A, 7);

				if (debug) std::cout << "A" << std::endl;

			} else {

				uint16_t address;

				if (opcode == 0x26) {
					PC+=2;
					tCnt = 5;
					address = iByte2;

					if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

				} else if (opcode == 0x36) {
					PC += 2;
					tCnt = 6;
					address = (iByte2 + X) & 0xFF;

					if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

				} else if (opcode == 0x2E) {
					PC += 3;
					tCnt = 6;
					address = (iByte2 | (iByte3 << 8));

					if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3;

				} else if (opcode == 0x3E) {
					PC += 3;
					tCnt = 7;
					address = ((iByte2 | (iByte3 << 8)) + X) & 0xFFFF;

					if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3 << ",X";

				} else {
					return false;
				}

				bool store;
				store = getBit(getByte(address), 7);

				setByte(address, (getByte(address) << 1) & 0xFE);

				setByte(address, getByte(address) | PS[C]);

				PS[C] = store;
				PS[Z] = (getByte(address) == 0) ? true : false;
				PS[N] = getBit(getByte(address), 7);
			}

            break;

		}

		case 0x6A:				//ROR
		case 0x66:
		case 0x76:
		case 0x6E:
		case 0x7E: {

			if (debug) std::cout << "ROR ";


			if (opcode == 0x6A) {
				PC++;
				tCnt = 2;

				bool store;
				store = getBit(A, 0);

				A = (A >> 1) & 0x7F;
				A |= (PS[C] ? 0x80 : 0x0);
				PS[C] = store;
				PS[Z] = (A == 0) ? true : false;
				PS[N] = getBit(A, 7);

				if (debug) std::cout << "A" << std::endl;

			} else {

				uint16_t address;

				if (opcode == 0x66) {
					PC+=2;
					tCnt = 5;
					address = iByte2;

					if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

				} else if (opcode == 0x76) {
					PC += 2;
					tCnt = 6;
					address = (iByte2 + X) & 0xFF;

					if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

				} else if (opcode == 0x6E) {
					PC += 3;
					tCnt = 6;
					address = (iByte2 | (iByte3 << 8));

					if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3;

				} else if (opcode == 0x7E) {
					PC += 3;
					tCnt = 7;
					address = ((iByte2 | (iByte3 << 8)) + X) & 0xFFFF;

					if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3 << ",X";

				} else {
					return false;
				}

				bool store;
				store = getBit(getByte(address), 0);

				setByte(address, (getByte(address) >> 1) & 0x7F);

				setByte(address, getByte(address) | (PS[C] ? 0x80 : 0x0));


				PS[C] = store;
				PS[Z] = (getByte(address) == 0) ? true : false;
				PS[N] = getBit(getByte(address), 7);

			}

            break;

		}

		case 0x40: {				//RTI

			if (debug) std::cout << "RTI";

			SP--;
			tCnt = 6;
			uint8_t memByte;
			memByte = stack[SP];

			PS[N] = getBit(memByte, 7);
			PS[V] = getBit(memByte, 6);
			PS[D] = getBit(memByte, 3);
			PS[I] = getBit(memByte, 2);
			PS[Z] = getBit(memByte, 1);
			PS[C] = getBit(memByte, 0);

			SP--;
			uint16_t low = stack[SP];
			SP--;
			uint16_t high = stack[SP] << 8;
			PC = high | low;
            break;
		}

		case 0x60: {				//RTS

			if (debug) std::cout << "RTS";

			SP++;
			tCnt = 6;
			uint16_t low = stack[SP];
			SP++;
			uint16_t high = stack[SP] << 8;
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

			if (debug) std::cout << "SBC ";

			uint8_t memByte;

			if (opcode == 0xE9) {
				memByte = iByte2;
				tCnt = 2;
				PC += 2;

				if (debug) std::cout << "#$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xE5) {
				memByte = getByte(iByte2);
				tCnt = 3;
				PC += 2;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xF5) {
				memByte = getByte((iByte2 + X) & 0xFF);
				tCnt = 4;
				PC += 2;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0xED) {
				uint16_t address;
				address = (iByte2 | (iByte3 << 8));
				memByte = getByte(address);
				tCnt = 4;
				PC += 3;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3;

			} else if (opcode == 0xFD) {
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + X) & 0xFFFF);
				memByte = getByte(address);
				tCnt = 4;
				PC += 3;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3 << ",X";

			} else if (opcode == 0xF9) {
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + Y) & 0xFFFF);
				memByte = getByte(address);
				tCnt = 4;
				PC += 3;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3 << ",Y";

			} else if (opcode == 0xE1) {
				uint16_t address;

				uint16_t low;
				uint16_t high;
				low = getByte((iByte2 + X) & 0xFF);
				high = (getByte(iByte2 + 1 + X & 0xFF)) << 8;
				address = low | high;
				memByte = getByte(address);
				tCnt = 6;
				PC += 2;

				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X)";

			} else if (opcode == 0xF1) {
				uint16_t address;
				address = ((((getByte(iByte2)) | (getByte(iByte2 + 1 & 0xFF)) << 8) + Y) & 0xFFFF);
				memByte = getByte(address);
				tCnt = 5;
				PC += 2;

				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << "),Y";

			} else {
				return false;
			}

			int total;

			total = A - memByte - (!PS[C]);


			int8_t num1, num2;
			num1 = (int8_t) A;
			num2 = (int8_t) memByte;

			if (num1 - num2 - (!PS[C]) < -128 || num1 - num2 - (!PS[C]) > 127) {
				PS[V] = true;
			} else {
				PS[V] = false;
			}



			A = total & 0xFF;

			PS[C] = (total >= 0) ? true : false;
			PS[N] = getBit(total, 7);
			PS[Z] = (A == 0) ? 1 : 0;

            break;
		}

		case 0x38: {				//SEC

			if (debug) std::cout << "SEC";

			PC++;
			tCnt = 2;
			PS[C] = true;
            break;
		}

		case 0xF8: {				//SED

			if (debug) std::cout << "SED";

			PC++;
			tCnt = 2;
			PS[D] = true;
            break;
		}

		case 0x78: {				//SEI

			if (debug) std::cout << "SEI";

			PC++;
			tCnt = 2;
			PS[I] = true;
            break;
		}

		case 0x85:				//STA
		case 0x95:
		case 0x8D:
		case 0x9D:
		case 0x99:
		case 0x81:
		case 0x91: {

			if (debug) std::cout << "STA ";

			uint16_t address;

			if (opcode == 0x85) {
				address = iByte2;
				PC += 2;
				tCnt = 3;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0x95) {
				address = ((iByte2 + X) & 0xFF);
				PC += 2;
				tCnt = 4;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0x8D) {
				address = (iByte2 | (iByte3 << 8));
				PC += 3;
				tCnt = 4;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3;

			} else if (opcode == 0x9D) {
				address = (((iByte2 | (iByte3 << 8)) + X) & 0xFFFF);
				PC += 3;
				tCnt = 5;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3 << ",X";

			} else if (opcode == 0x99) {
				address = (((iByte2 | (iByte3 << 8)) + Y) & 0xFFFF);
				PC += 3;
				tCnt = 5;

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3 << ",Y";

			} else if (opcode == 0x81) {
				uint16_t low;
				uint16_t high;
				low = getByte((iByte2 + X) & 0xFF);
				high = (getByte(iByte2 + 1 + X & 0xFF)) << 8;
				address = low | high;
				PC += 2;
				tCnt = 6;

				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X)";

			} else if (opcode == 0x91) {
				address = ((((getByte(iByte2)) | (getByte(iByte2 + 1 & 0xFF)) << 8) + Y) & 0xFFFF);
				PC += 2;
				tCnt = 6;

				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << "),Y";

			} else {
				return false;
			}

			setByte(address, A);
            break;
		}

		case 0x86:				//STX
		case 0x96:
		case 0x8E: {

			if (debug) std::cout << "STX ";

			uint16_t address;

			if (opcode == 0x86) {
				PC += 2;
				tCnt = 3;
				address = iByte2;
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;
			} else if (opcode == 0x96) {
				PC += 2;
				tCnt = 4;
				address = ((iByte2 + Y) & 0xFF);
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";
			} else if (opcode == 0x8E) {
				PC += 3;
				tCnt = 4;
				address = (iByte2 | (iByte3 << 8));
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3;
			} else {
				return false;
			}

			setByte(address, X);
            break;

		}

		case 0x84:				//STY
		case 0x94:
		case 0x8C: {

			if (debug) std::cout << "STY ";

			uint16_t address;

			if (opcode == 0x84) {
				PC += 2;
				tCnt = 3;
				address = iByte2;
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;
			} else if (opcode == 0x94) {
				PC += 2;
				tCnt = 4;
				address = ((iByte2 + X) & 0xFF);
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";
			} else if (opcode == 0x8C) {
				PC += 3;
				tCnt = 4;
				address = (iByte2 | (iByte3 << 8));
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << (unsigned int) iByte3;
			} else {
				return false;
			}

			setByte(address, Y);
            break;
		}

		case 0xAA: {				//TAX

			if (debug) std::cout << "TAX";

			PC++;
			tCnt = 2;
			X = A;
			PS[N] = getBit(X, 7);
			PS[Z] = (X == 0) ? true : false;
            break;
		}

		case 0xA8: {				//TAY

			if (debug) std::cout << "TAY";

			PC++;
			tCnt = 2;
			Y = A;
			PS[N] = getBit(Y, 7);
			PS[Z] = (Y == 0) ? true : false;
            break;

		}
		case 0xBA: {				//TSX

			if (debug) std::cout << "TSX";

			PC++;
			tCnt = 2;
			X = SP;
			PS[N] = getBit(X, 7);
			PS[Z] = (X == 0) ? true : false;
            break;
		}

		case 0x8A: {				//TXA

			if (debug) std::cout << "TXA";

			PC++;
			tCnt = 2;
			A = X;
			PS[N] = getBit(A, 7);
			PS[Z] = (A == 0) ? true : false;
            break;
		}

		case 0x9A: {				//TXS

			if (debug) std::cout << "TXS";

			PC++;
			tCnt = 2;
			SP = X;
            break;

		}
		case 0x98: {				//TYA

			if (debug) std::cout << "TYA";

			PC++;
			tCnt = 2;
			A = Y;
			PS[N] = getBit(A, 7);
			PS[Z] = (A == 0) ? true : false;
            break;
		}

		default: {
			std:: cout << "Unrecognized opcode : " << std::hex << (int) opcode << std::endl;
			return false;
		}
	}

	if (debug) std::cout << std::endl;

	return true;

}

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
