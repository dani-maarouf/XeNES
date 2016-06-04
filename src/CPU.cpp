#include <iostream>

#include "CPU.hpp"
                            //0,1,2,3,4,5,6,7,8,9,A,B,C,D,E,F
const int opcodeLens[0x20] = {2,2,0,2,2,2,2,2,1,2,1,2,3,3,3,3, //0 2 4 6 8 A C E
	                          2,2,0,2,2,2,2,2,1,3,1,3,3,3,3,3};//1 3 5 7 9 B D F

                         //0     1      2      3      4      5      6      7      8      9
const char * opnames[] = {"XXX", "ADC", "AND", "ASL", "BCC", "BCS", "BEQ", "BMI", "BIT", "BNE", //0
                          "BPL", "BRK", "BVC", "BVS", "CLC", "CLD", "CLI", "CLV", "CMP", "CPX", //1
                          "CPY","*DCP", "DEC", "DEX", "DEY", "EOR", "INC", "INX", "INY","*ISB", //2
                          "JMP", "JSR","*LAX", "LDA", "LDX", "LDY", "LSR", "NOP","*NOP", "ORA", //3
                          "PHA", "PHP", "PLA", "PLP","*RLA", "ROL", "ROR","*RRA", "RTI", "RTS", //4
                         "*SBC", "SBC", "SEC", "SED", "SEI","*SAX","*SLO","*SRE", "STA", "STX", //5
                          "STY", "TAX", "TAY", "TSX", "TXA", "TXS", "TYA"};                     //6

                       //0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
const int opnameMap[] = {11,39,00,56,38,39,03,56,41,39,03,00,38,39,03,56,  //0
                         10,39,00,56,38,39,03,56,14,39,38,56,38,39,03,56,  //1
                         31,02,00,44, 8,02,45,44,43,02,45,00, 8,02,45,44,  //2
                         07,02,00,44,38,02,45,44,52,02,38,44,38,02,45,44,  //3
                         48,25,00,57,38,25,36,57,40,25,36,00,30,25,36,57,  //4
                         12,25,00,57,38,25,36,57,16,25,38,57,38,25,36,57,  //5
                         49,01,00,47,38,01,46,47,42,01,46,00,30,01,46,47,  //6
                         13,01,00,47,38,01,46,47,54,01,38,47,38,01,46,36,  //7
                         38,58,00,55,60,58,59,55,24,00,64,00,60,58,59,55,  //8
                         04,58,00,00,60,58,59,55,66,58,65,00,00,58,00,00,  //9
                         35,33,34,32,35,33,34,32,62,33,61,00,35,33,34,32,  //A
                         05,33,00,32,35,33,34,32,17,33,63,00,35,33,34,32,  //B
                         20,18,00,21,20,18,22,21,28,18,23,00,20,18,22,21,  //C
                          9,18,00,21,38,18,22,21,15,18,38,21,38,18,22,21,  //D
                         19,50,00,29,19,51,26,29,27,51,38,51,19,51,26,29,  //E
                         06,51,00,29,38,51,26,29,53,51,38,29,38,51,26,29}; //F

const enum AddressMode opAddressModes[] = {};




static bool getBit(uint8_t, int);

uint8_t CPU::getByte(uint16_t address) {
	return cpuMem[address];
	
}

bool CPU::setByte(uint16_t address, uint8_t byte) {
	cpuMem[address] = byte;
	return true;
}

uint16_t CPU::retrieveAddress(enum AddressMode mode) {

	uint8_t firstByte, secondByte;
	firstByte = getByte(PC + 1);
	secondByte = getByte(PC + 2);

	switch (mode) {
		case ZERO_PAGE:
		return firstByte;

		case ZERO_PAGE_X:
		return ((firstByte + X) & 0xFF);

		case ZERO_PAGE_Y:
		return ((firstByte + Y) & 0xFF);

		case ABSOLUTE:
		return (firstByte | (secondByte << 8));

		case ABSOLUTE_X:
		return (((firstByte | (secondByte << 8)) + X) & 0xFFFF);

		case ABSOLUTE_Y:
		return (((firstByte | (secondByte << 8)) + Y) & 0xFFFF);

		case INDEXED_INDIRECT: {
			uint8_t low, high;
			low = getByte((firstByte + X) & 0xFF);
			high = getByte(firstByte + 1 + X & 0xFF);
			return ((high << 8) | low);
		}
		case INDIRECT_INDEXED: {
			return ((((getByte(firstByte)) | (getByte(firstByte + 1 & 0xFF)) << 8) + Y) & 0xFFFF);
		}

		default:
		return 0;
	}

}

void CPU::init() {

	for (int x = 0; x < 0x10000; x++) {
		setByte(x, 0x0);
	}
	for (int x = 0; x < 8; x++) {
		PS[x] = false;
	}

	PC = 0x0;
	SP = 0xFD;
	A  = 0x0;
	X  = 0x0;
	Y  = 0x0;

	PS[I] = true;

	nesAPU.init();

	return;
}

bool CPU::executeNextOpcode(bool debug, bool verbose) {

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

	PC += opcodeLens[opcode % 0x20];

	if (debug) std::cout << opnames[opnameMap[opcode]] << ' ';

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

			if (opcode == 0x69) {							//Immediate
				memByte = iByte2;
				if (debug) std::cout << "#$" << std::hex << std::uppercase << (unsigned int) iByte2;
			} else if (opcode == 0x65) {					//Zero Page
				memByte = getByte(iByte2);
				
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0x75) {					//Zero Page,X 
				memByte = getByte((iByte2 + X) & 0xFF );
				
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0x6D) {					//Absolute
				uint16_t address;
				address = (iByte2 | (iByte3 << 8));
				memByte = getByte(address);
				
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2;

			} else if (opcode == 0x7D) {					//Absolute,X
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + X) & 0xFFFF);
				memByte = getByte(address);
				
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0x79) {					//Absolute,Y
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + Y) & 0xFFFF);
				memByte = getByte(address);
				
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",Y";

			} else if (opcode == 0x61) {					//(Indirect,X)
				uint16_t address;

				uint16_t low;
				uint16_t high;
				low = getByte((iByte2 + X) & 0xFF);
				high = getByte(iByte2 + 1 + X & 0xFF) << 8;
				address = low | high;
				memByte = getByte(address);
				
				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X)";

			} else if (opcode == 0x71) {					//(Indirect),Y

				uint16_t address;
				address = ((((getByte(iByte2)) | (getByte(iByte2 + 1 & 0xFF)) << 8) + Y) & 0xFFFF);
				memByte = getByte(address);
				
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

			uint8_t memByte;

			if (opcode == 0x29) {
				memByte = iByte2;
				
				if (debug) std::cout << "#$" << std::hex << std::uppercase << (unsigned int) iByte2;
			} else if (opcode == 0x25) {
				memByte = getByte(iByte2);
				
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;
			} else if (opcode == 0x35) {
				memByte = getByte( (iByte2 + X) & 0xFF );
				
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";
			} else if (opcode == 0x2D) {
				uint16_t address;
				address = (iByte2 | (iByte3 << 8));
				memByte = getByte(address);
				
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2;
			} else if (opcode == 0x3D) {
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + X) & 0xFFFF);
				memByte = getByte(address);
				
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",X";
			} else if (opcode == 0x39) {
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + Y) & 0xFFFF);
				memByte = getByte(address);
				
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",Y";
			} else if (opcode == 0x21) {
				uint16_t address;

				uint16_t low;
				uint16_t high;
				low = getByte((iByte2 + X) & 0xFF);
				high = (getByte(iByte2 + 1 + X & 0xFF)) << 8;
				address = low | high;
				memByte = getByte(address);
				
				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X)";
			} else if (opcode == 0x31) {

				uint16_t address;
				address = ((((getByte(iByte2)) | (getByte(iByte2 + 1 & 0xFF)) << 8) + Y) & 0xFFFF);
				memByte = getByte(address);

				

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

			if (opcode == 0x0A) {

				PS[C] = getBit(A, 7);
				A = ((A << 1) & 0xFE);
				PS[N] = getBit(A, 7);
				PS[Z] = (A == 0) ? true : false;

				if (debug) std::cout << "A";

			} else {
				
				uint16_t address;

				if (opcode == 0x06) {

					address = iByte2;
					

					if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

				} else if (opcode == 0x16) {

					address = (iByte2 + X) & 0xFF;
					

					if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

				} else if (opcode == 0x0E) {

					address = (iByte2 | (iByte3 << 8));
					

					if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2;

				} else if (opcode == 0x1E) {
					address = ((iByte2 | (iByte3 << 8)) + X) & 0xFFFF;
					
					if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",X";

				} else {
					return false;
				}

				PS[C] = getBit(getByte(address), 7);

				setByte(address, ((getByte(address) << 1) & 0xFE));

				PS[N] = getBit(getByte(address), 7);
				PS[Z] = (getByte(address) == 0) ? true : false;
			}

            break;
		}

		case 0x90: {			//BCC
			if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;
			
			if (PS[C] == 0) PC += (int8_t) iByte2;
			
            break;
		}

		case 0xB0: {			//BCS
			if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;
			
			if (PS[C]) PC += (int8_t) iByte2;
			
			
            break;
		}

		case 0xF0: {			//BEQ
			if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;
			
			if (PS[Z]) PC += (int8_t) iByte2;
			
            break;
		}

		case 0x24:			//BIT
		case 0x2C: {

			if (debug) std::cout << "$";

			uint8_t memByte;

			if (opcode == 0x24) {
				memByte = getByte(iByte2);
				
				if (debug) std::cout << std::hex << std::uppercase << (unsigned int) iByte2;
			} else if (opcode == 0x2C) {
				memByte = getByte((iByte2 | (iByte3 << 8)));
				
				if (debug) std::cout << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2;
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
			if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			

			if (PS[N]) PC += (int8_t) iByte2;
			
            break;
		}

		case 0xD0: {			//BNE
			if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			

			if (PS[Z] == 0) PC += (int8_t) iByte2;
			
            break;
		}

		case 0x10: {			//BPL
			if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			

			if (PS[N] == 0) PC += (int8_t) iByte2;
			
            break;
		}

		case 0x00: {			//BRK

			uint8_t low, high;
			low = PC & 0xFF;
			high = (PC & 0xFF00) >> 8;

			setByte(0x100 + SP, high);
			SP--;
			setByte(0x100 + SP, low);
			SP--;

			uint8_t memByte;

			if (PS[C]) memByte |= 0x1;
			if (PS[Z]) memByte |= 0x2;
			if (PS[I]) memByte |= 0x4;
			if (PS[D]) memByte |= 0x8;
			if (PS[V]) memByte |= 0x40;
			if (PS[N]) memByte |= 0x80;

			setByte(0x100 + SP, memByte | 0x10);
			SP--;

			low = getByte(0xFFFE);
			high = getByte(0xFFFF);

			PC = (high << 8) | low;

			break;
		}

		case 0x50: {			//BVC
			if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			

			if (PS[V] == 0) PC += (int8_t) iByte2;
			
            break;
		}

		case 0x70: {			//BVS
			if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			

			if (PS[V]) PC += (int8_t) iByte2;
			
            break;
		}

		case 0x18: {			//CLC
			
			PS[C] = false;
			
            break;
		}

		case 0xD8: {			//CLD
			
			PS[D] = false;
			
            break;
		}

		case 0x58: {			//CLI
			
			PS[I] = false;
			
            break;
		}

		case 0xB8: {			//CLV
			
			PS[V] = false;
			
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

			uint8_t memByte;

			

			if (opcode == 0xC9) {
				memByte = iByte2;
				

				if (debug) std::cout << "#$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xC5) {
				memByte = getByte(iByte2);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xD5) {
				memByte = getByte( (iByte2 + X) & 0xFF );
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0xCD) {
				uint16_t address;
				address = (iByte2 | (iByte3 << 8));
				memByte = getByte(address);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2;

			} else if (opcode == 0xDD) {
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + X) & 0xFFFF);
				memByte = getByte(address);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0xD9) {
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + Y) & 0xFFFF);
				memByte = getByte(address);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",Y";

			} else if (opcode == 0xC1) {
				uint16_t address;

				uint16_t low;
				uint16_t high;
				low = getByte((iByte2 + X) & 0xFF);
				high = (getByte(iByte2 + 1 + X & 0xFF)) << 8;
				address = low | high;
				memByte = getByte(address);
				

				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X)";

			} else if (opcode == 0xD1) {
				uint16_t address;
				address = ((((getByte(iByte2)) | (getByte(iByte2 + 1 & 0xFF)) << 8) + Y) & 0xFFFF);
				memByte = getByte(address);
				

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

			uint8_t memByte;
		
			if (opcode == 0xE0) {
				memByte = iByte2;
				if (debug) std::cout << "#$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xE4) {
				memByte = getByte(iByte2);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xEC) {
				memByte = getByte((iByte2 | (iByte3 << 8)));
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2;

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
				

				if (debug) std::cout << "#$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xC4) {
				memByte = getByte(iByte2);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xCC) {
				memByte = getByte((iByte2 | (iByte3 << 8)));
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2;

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


		case 0xC3:			//DCP
		case 0xD3:
		case 0xC7:
		case 0xD7:
		case 0xCF:
		case 0xDF:
		case 0xDB: {

			uint8_t memByte;

			

			if (opcode == 0xC9) {
				memByte = (iByte2 - 1) & 0xFF;

				

				if (debug) std::cout << "#$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xC7) {
				setByte(iByte2, ( (getByte(iByte2) - 1) & 0xFF ));
				memByte = getByte(iByte2);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xD7) {

				uint8_t address;
				address = (iByte2 + X) & 0xFF;

				setByte(address, ( (getByte(address) - 1) & 0xFF ));

				memByte = getByte(address);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0xCF) {
				uint16_t address;
				address = (iByte2 | (iByte3 << 8));

				setByte(address, ( (getByte(address) - 1) & 0xFF ));

				memByte = getByte(address);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2;

			} else if (opcode == 0xDF) {
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + X) & 0xFFFF);

				setByte(address, ( (getByte(address) - 1) & 0xFF ));

				memByte = getByte(address);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",X";

			}  else if (opcode == 0xDB) {
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + Y) & 0xFFFF);

				setByte(address, ( (getByte(address) - 1) & 0xFF ));

				memByte = getByte(address);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",Y";

			} else if (opcode == 0xD9) {
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + Y) & 0xFFFF);

				setByte(address, ( (getByte(address) - 1) & 0xFF ));

				memByte = getByte(address);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",Y";

			} else if (opcode == 0xC3) {
				uint16_t address;

				uint16_t low;
				uint16_t high;
				low = getByte((iByte2 + X) & 0xFF);
				high = (getByte(iByte2 + 1 + X & 0xFF)) << 8;
				address = low | high;

				setByte(address, ( (getByte(address) - 1) & 0xFF ));

				memByte = getByte(address);
				

				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X)";

			} else if (opcode == 0xD3) {
				uint16_t address;
				address = ((((getByte(iByte2)) | (getByte(iByte2 + 1 & 0xFF)) << 8) + Y) & 0xFFFF);

				setByte(address, ( (getByte(address) - 1) & 0xFF ));

				memByte = getByte(address);
				

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



		case 0xC6:			//DEC
		case 0xD6:
		case 0xCE:
		case 0xDE: {

			uint16_t address;

			

			if (opcode == 0xC6) {
				address = iByte2;
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xD6) {
				address = ((iByte2 + X) & 0xFF);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0xCE) {
				address = (iByte2 | (iByte3 << 8));
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2;

			} else if (opcode == 0xDE) {
				address = (((iByte2 | (iByte3 << 8)) + X) & (0xFFFF));
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",X";

			} else {
				return false;
			}

			setByte(address, ((getByte(address) - 1) & 0xFF));
			PS[N] = getBit(getByte(address), 7);
			PS[Z] = (getByte(address) == 0) ? true : false;

            break;
		}




		case 0xCA: {			//DEX

			X = X - 1;
			PS[Z] = (X == 0) ? true : false;
			PS[N] = getBit(X, 7);
            break;
		}

		case 0x88: {			//DEY
			
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

			uint8_t memByte;

			if (opcode == 0x49) {
				memByte = iByte2;
				

				if (debug) std::cout << "#$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0x45) {
				memByte = getByte(iByte2);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0x55) {
				memByte = getByte( (iByte2 + X) & 0xFF );
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0x4D) {
				uint16_t address;
				address = (iByte2 | (iByte3 << 8));
				memByte = getByte(address);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2;

			} else if (opcode == 0x5D) {
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + X) & 0xFFFF);
				memByte = getByte(address);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0x59) {
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + Y) & 0xFFFF);
				memByte = getByte(address);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",Y";

			} else if (opcode == 0x41) {
				uint16_t address;

				uint16_t low;
				uint16_t high;
				low = getByte((iByte2 + X) & 0xFF);
				high = (getByte(iByte2 + 1 + X & 0xFF)) << 8;
				address = low | high;
				memByte = getByte(address);
				

				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X)";

			} else if (opcode == 0x51) {
				uint16_t address;
				address = ((((getByte(iByte2)) | (getByte(iByte2 + 1 & 0xFF)) << 8) + Y) & 0xFFFF);
				memByte = getByte(address);
				

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

			uint16_t address;

			

			if (opcode == 0xE6) {
				address = iByte2;
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xF6) {
				address = ((iByte2 + X) & 0xFF);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0xEE) {
				address = (iByte2 | (iByte3 << 8));
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2;

			} else if (opcode == 0xFE) {
				address = (((iByte2 | (iByte3 << 8)) + X) & 0xFFFF);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",X";

			} else {
				return false;
			}

			setByte(address, ((getByte(address) + 1) & 0xFF));
			PS[N] = getBit(getByte(address), 7);
			PS[Z] = (getByte(address) == 0) ? true : false;

            break;
		}

		case 0xE8: {				//INX			
			
			X = X + 1;
			PS[Z] = (X == 0) ? true : false;
			PS[N] = getBit(X, 7);
            break;
		}

		case 0xC8: {				//INY			
			
			Y = Y + 1;
			PS[Z] = (Y == 0) ? true : false;
			PS[N] = getBit(Y, 7);
            break;
		}

		case 0xE3:				//ISB
		case 0xE7:
		case 0xF7:
		case 0xFB:
		case 0xEF:
		case 0xFF:
		case 0xF3: {

			uint8_t memByte;

			

			if (opcode == 0xE9 || opcode == 0xEB) {
				memByte = iByte2;
				

				if (debug) std::cout << "#$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xE7) {

				setByte(iByte2, ((getByte(iByte2) + 1) & 0xFF));

				memByte = getByte(iByte2);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xF7) {

				uint8_t address;
				address = (iByte2 + X) & 0xFF;

				setByte(address, ((getByte(address) + 1) & 0xFF));

				memByte = getByte(address);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0xEF) {
				uint16_t address;
				address = (iByte2 | (iByte3 << 8));

				setByte(address, ((getByte(address) + 1) & 0xFF));

				memByte = getByte(address);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2;

			} else if (opcode == 0xFF) {
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + X) & 0xFFFF);

				setByte(address, ((getByte(address) + 1) & 0xFF));

				memByte = getByte(address);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0xFB) {
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + Y) & 0xFFFF);

				setByte(address, ((getByte(address) + 1) & 0xFF));

				memByte = getByte(address);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",Y";

			} else if (opcode == 0xE3) {
				uint16_t address;

				uint16_t low;
				uint16_t high;
				low = getByte((iByte2 + X) & 0xFF);
				high = (getByte(iByte2 + 1 + X & 0xFF)) << 8;
				address = low | high;

				setByte(address, ((getByte(address) + 1) & 0xFF));

				memByte = getByte(address);
				

				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X)";

			} else if (opcode == 0xF3) {
				uint16_t address;
				address = ((((getByte(iByte2)) | (getByte(iByte2 + 1 & 0xFF)) << 8) + Y) & 0xFFFF);

				setByte(address, ((getByte(address) + 1) & 0xFF));

				memByte = getByte(address);
				

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

		case 0x4C:				//JMP
		case 0x6C: {

			uint16_t address;

			if (opcode == 0x4C) {
				
				address = (iByte2 | (iByte3 << 8));
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2;
			} else if (opcode == 0x6C) {

				uint16_t low, high;

				low = getByte((iByte2 | (iByte3 << 8)));
				high = getByte(((iByte2 + 1) & 0xFF) | (iByte3 << 8));
				address = (high << 8) | low;

				
				//address = getByte((iByte2 | (iByte3 << 8)));
				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ")";


			} else {
				return false;
			}

			PC = address;
            break;
		}

		case 0x20: {			//JSR

			if (debug) std::cout << "JSR $" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2;
			uint16_t store;
		
			store = PC;

			uint8_t low, high;

			low = store & 0xFF;
			high = (store & 0xFF00) >> 8;

			setByte(SP + 0x100, high);
			SP--;
			setByte(SP + 0x100, low);
			SP--;

			PC = (iByte2 | (iByte3 << 8));

            break;
		}

		case 0xA3: 			//LAX
		case 0xA7: 
		case 0xAF: 
		case 0xB3: 
		case 0xB7: 
		case 0xBF: {			

			uint8_t memByte;

			

			if (opcode == 0xA9) {
				memByte = iByte2;
				

				if (debug) std::cout << "#$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xA7) {
				memByte = getByte(iByte2);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xB5) {
				memByte = getByte((iByte2 + X) & 0xFF);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0xB7) {
				memByte = getByte((iByte2 + Y) & 0xFF);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",Y";

			} else if (opcode == 0xAF) {
				uint16_t address;
				address = (iByte2 | (iByte3 << 8));
				memByte = getByte(address);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2;


			} else if (opcode == 0xBD) {
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + X) & 0xFFFF);
				memByte = getByte(address);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0xBF) {
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + Y) & 0xFFFF);
				memByte = getByte(address);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",Y";

			} else if (opcode == 0xA3) {
				uint16_t address;

				uint16_t low;
				uint16_t high;
				low = getByte((iByte2 + X) & 0xFF);
				high = (getByte(iByte2 + 1 + X & 0xFF)) << 8;
				address = low | high;
				memByte = getByte(address);
				

				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X)";

			} else if (opcode == 0xB3) {
				uint16_t address;
				address = ((((getByte(iByte2)) | (getByte(iByte2 + 1 & 0xFF)) << 8) + Y) & 0xFFFF);
				memByte = getByte(address);
				

				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << "),Y";

			} else {
				return false;
			}


			A = memByte;
			X = memByte;

			PS[N] = getBit(A, 7);
			PS[Z] = (A == 0) ? true : false;
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
				

				if (debug) std::cout << "#$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xA5) {
				memByte = getByte(iByte2);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xB5) {
				memByte = getByte((iByte2 + X) & 0xFF);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0xAD) {
				uint16_t address;
				address = (iByte2 | (iByte3 << 8));
				memByte = getByte(address);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2;


			} else if (opcode == 0xBD) {
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + X) & 0xFFFF);
				memByte = getByte(address);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0xB9) {
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + Y) & 0xFFFF);
				memByte = getByte(address);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",Y";

			} else if (opcode == 0xA1) {
				uint16_t address;

				uint16_t low;
				uint16_t high;
				low = getByte((iByte2 + X) & 0xFF);
				high = (getByte(iByte2 + 1 + X & 0xFF)) << 8;
				address = low | high;
				memByte = getByte(address);
				

				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X)";

			} else if (opcode == 0xB1) {
				uint16_t address;
				address = ((((getByte(iByte2)) | (getByte(iByte2 + 1 & 0xFF)) << 8) + Y) & 0xFFFF);
				memByte = getByte(address);
				

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

			uint8_t memByte;

			

			if (opcode == 0xA2) {
				PC += 2;
				memByte = iByte2;
				

				if (debug) std::cout << "#$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xA6) {
				memByte = getByte(iByte2);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xB6) {
				memByte = getByte((iByte2 + Y) & 0xFF);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",Y";

			} else if (opcode == 0xAE) {
				memByte = getByte((iByte2 | (iByte3 << 8)));
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2;

			} else if (opcode == 0xBE) {
				memByte = getByte((((iByte2 | (iByte3 << 8)) + Y) & 0xFFFF));
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",X";

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
				

				if (debug) std::cout << "#$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xA4) {
				memByte = getByte(iByte2);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;


			} else if (opcode == 0xB4) {
				memByte = getByte((iByte2 + X) & 0xFF);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0xAC) {
				memByte = getByte((iByte2 | (iByte3 << 8)));
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2;

			} else if (opcode == 0xBC) {
				memByte = getByte((((iByte2 | (iByte3 << 8)) + X) & 0xFFFF));
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",X";

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
				

				PS[N] = 0;
				PS[C] = getBit(A, 0);

				A = (A >> 1) & 0x7F;
				PS[Z] = (A == 0) ? true : false;

				if (debug) std::cout << "A";

			} else {

				uint16_t address;

				if (opcode == 0x46) {
					
					address = iByte2;

					if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

				} else if (opcode == 0x56) {
					
					address = (iByte2 + X) & 0xFF;

					if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

				} else if (opcode == 0x4E) {
					
					address = (iByte2 | (iByte3 << 8));

					if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2;

				} else if (opcode == 0x5E) {
					
					address = ((iByte2 | (iByte3 << 8)) + X) & 0xFFFF;

					if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",X";

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

		case 0x04:
		case 0x44:
		case 0x64:
		case 0x0C:
		case 0x14:
		case 0x34:
		case 0x54:
		case 0x74:
		case 0xD4:
		case 0xF4:
		case 0x1A:
		case 0x3A:
		case 0x5A:
		case 0x7A:
		case 0xDA:
		case 0xFA:
		case 0x80:
		case 0x1C:
		case 0x3C:
		case 0x5C:
		case 0x7C:
		case 0xDC:
		case 0xFC:
		case 0xEA: {				//NOP

			if (opcode == 0xEA || opcode == 0x1A || opcode == 0x3A || opcode == 0x5A || opcode == 0x7A || opcode == 0xDA || opcode == 0xFA) {
			} else if (opcode == 0x04 || opcode == 0x44 || opcode == 0x64) {
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;
			} else if (opcode == 0x0C) {
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2;
			} else if (opcode == 0x14 || opcode == 0x34 || opcode == 0x54 || opcode == 0x74 || opcode == 0xD4 || opcode == 0xF4) {
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";
			} else if (opcode == 0x80) {
				if (debug) std::cout << "#$" << std::hex << std::uppercase << (unsigned int) iByte2;
			} else if (opcode == 0x1C || opcode == 0x3C || opcode == 0x5C || opcode == 0x7C || opcode == 0xDC || opcode == 0xFC) {
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",X";
			} else {
				return false;
			}

						//todo: fix for illegal opcodes
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

			uint8_t memByte;

			if (opcode == 0x09) {
				memByte = iByte2;
				
				if (debug) std::cout << "#$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0x05) {
				memByte = getByte(iByte2);
				
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0x15) {
				memByte = getByte((iByte2 + X) & 0xFF);
				
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0x0D) {
				uint16_t address;
				address = (iByte2 | (iByte3 << 8));
				memByte = getByte(address);
				
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2;

			} else if (opcode == 0x1D) {
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + X) & 0xFFFF);
				memByte = getByte(address);
				
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0x19) {
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + Y) & 0xFFFF);
				memByte = getByte(address);
				
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",Y";

			} else if (opcode == 0x01) {
				uint16_t address;

				uint16_t low;
				uint16_t high;
				low = getByte((iByte2 + X) & 0xFF);
				high = (getByte(iByte2 + 1 + X & 0xFF)) << 8;
				address = low | high;
				memByte = getByte(address);
				
				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X)";

			} else if (opcode == 0x11) {
				uint16_t address;
				address = ((((getByte(iByte2)) | (getByte(iByte2 + 1 & 0xFF)) << 8) + Y) & 0xFFFF);
				memByte = getByte(address);
				
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
			setByte(SP + 0x100, A);
			SP--;
            break;
		}

		case 0x08: {				//PHP

			uint8_t memByte;
			memByte = 0;

			if (PS[C]) memByte |= 0x1;
			if (PS[Z]) memByte |= 0x2;
			if (PS[I]) memByte |= 0x4;
			if (PS[D]) memByte |= 0x8;
			if (PS[V]) memByte |= 0x40;
			if (PS[N]) memByte |= 0x80;

			memByte |= 0x20;

			setByte(SP + 0x100, memByte);
			SP--;

            break;
		}

		case 0x68: {				//PLA
			SP++;
			A = getByte(SP + 0x100);
			PS[N] = getBit(A, 7);
			PS[Z] = (A == 0) ? true : false;
            break;
		}

		case 0x28: {				//PLP

			SP++;
			uint8_t memByte = getByte(SP + 0x100);

			PS[N] = getBit(memByte, 7);
			PS[V] = getBit(memByte, 6);
			PS[D] = getBit(memByte, 3);
			PS[I] = getBit(memByte, 2);
			PS[Z] = getBit(memByte, 1);
			PS[C] = getBit(memByte, 0);
            break;
		}

		case 0x23:				//RLA
		case 0x27:
		case 0x2F:
		case 0x33:
		case 0x37:
		case 0x3B:
		case 0x3F: {


			uint16_t address;

			if (opcode == 0x27) {
				address = iByte2;
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0x37) {
				address = ((iByte2 + X) & 0xFF);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0x2F) {
				address = (iByte2 | (iByte3 << 8));
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2;

			} else if (opcode == 0x3F) {
				address = (((iByte2 | (iByte3 << 8)) + X) & 0xFFFF);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0x3B) {
				address = (((iByte2 | (iByte3 << 8)) + Y) & 0xFFFF);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",Y";

			} else if (opcode == 0x23) {
				uint16_t low;
				uint16_t high;
				low = getByte((iByte2 + X) & 0xFF);
				high = (getByte(iByte2 + 1 + X & 0xFF)) << 8;
				address = low | high;
				

				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X)";

			} else if (opcode == 0x33) {
				address = ((((getByte(iByte2)) | (getByte(iByte2 + 1 & 0xFF)) << 8) + Y) & 0xFFFF);
				

				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << "),Y";

			} else {
				return false;
			}

			bool store;
			store = getBit(getByte(address), 7);
			setByte(address, (getByte(address) << 1) & 0xFE);
			setByte(address, getByte(address) | PS[C]);
			PS[C] = store;

			A &= getByte(address);

			PS[Z] = (A == 0) ? true : false;
			PS[N] = getBit(A, 7);



			break;
		}


		case 0x2A:				//ROL
		case 0x26:
		case 0x36:
		case 0x2E:
		case 0x3E: {

			if (opcode == 0x2A) {
				

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
					
					address = iByte2;

					if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

				} else if (opcode == 0x36) {
					
					address = (iByte2 + X) & 0xFF;

					if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

				} else if (opcode == 0x2E) {
					
					address = (iByte2 | (iByte3 << 8));

					if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2;

				} else if (opcode == 0x3E) {
					
					address = ((iByte2 | (iByte3 << 8)) + X) & 0xFFFF;

					if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",X";

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

			if (opcode == 0x6A) {
				

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
					
					address = iByte2;

					if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

				} else if (opcode == 0x76) {
					
					address = (iByte2 + X) & 0xFF;

					if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

				} else if (opcode == 0x6E) {
					
					address = (iByte2 | (iByte3 << 8));

					if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2;

				} else if (opcode == 0x7E) {
					
					address = ((iByte2 | (iByte3 << 8)) + X) & 0xFFFF;

					if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",X";

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


		case 0x63:				//RRA
		case 0x67:
		case 0x6F:
		case 0x73:
		case 0x77:
		case 0x7B:
		case 0x7F: {

			uint16_t address;

			if (opcode == 0x67) {
				address = iByte2;
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0x77) {
				address = ((iByte2 + X) & 0xFF);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0x6F) {
				address = (iByte2 | (iByte3 << 8));
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2;

			} else if (opcode == 0x7F) {
				address = (((iByte2 | (iByte3 << 8)) + X) & 0xFFFF);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0x7B) {
				address = (((iByte2 | (iByte3 << 8)) + Y) & 0xFFFF);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",Y";

			} else if (opcode == 0x63) {
				uint16_t low;
				uint16_t high;
				low = getByte((iByte2 + X) & 0xFF);
				high = (getByte(iByte2 + 1 + X & 0xFF)) << 8;
				address = low | high;
				

				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X)";

			} else if (opcode == 0x73) {
				address = ((((getByte(iByte2)) | (getByte(iByte2 + 1 & 0xFF)) << 8) + Y) & 0xFFFF);
				

				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << "),Y";

			} else {
				return false;
			}

			bool store;
			store = getBit(getByte(address), 0);

			setByte(address, (getByte(address) >> 1) & 0x7F);
			setByte(address, getByte(address) | (PS[C] ? 0x80 : 0x0));


			PS[C] = store;

			uint8_t memByte;
			memByte = getByte(address);

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

		

		case 0x40: {				//RTI

			SP++;
			
			uint8_t memByte;
			memByte = getByte(SP + 0x100);

			PS[N] = getBit(memByte, 7);
			PS[V] = getBit(memByte, 6);
			PS[D] = getBit(memByte, 3);
			PS[I] = getBit(memByte, 2);
			PS[Z] = getBit(memByte, 1);
			PS[C] = getBit(memByte, 0);

			SP++;
			uint16_t low = getByte(SP + 0x100);
			SP++;
			uint16_t high = getByte(SP + 0x100) << 8;
			PC = high | low;
            break;
		}

		case 0x60: {				//RTS

			SP++;
			
			uint16_t low = getByte(SP + 0x100);
			SP++;
			uint16_t high = getByte(SP + 0x100) << 8;
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
		case 0xF1: 
		case 0xEB: {

			uint8_t memByte;

			if (opcode == 0xE9 || opcode == 0xEB) {
				memByte = iByte2;
				

				if (debug) std::cout << "#$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xE5) {
				memByte = getByte(iByte2);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0xF5) {
				memByte = getByte((iByte2 + X) & 0xFF);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0xED) {
				uint16_t address;
				address = (iByte2 | (iByte3 << 8));
				memByte = getByte(address);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2;

			} else if (opcode == 0xFD) {
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + X) & 0xFFFF);
				memByte = getByte(address);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0xF9) {
				uint16_t address;
				address = (((iByte2 | (iByte3 << 8)) + Y) & 0xFFFF);
				memByte = getByte(address);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",Y";

			} else if (opcode == 0xE1) {
				uint16_t address;

				uint16_t low;
				uint16_t high;
				low = getByte((iByte2 + X) & 0xFF);
				high = (getByte(iByte2 + 1 + X & 0xFF)) << 8;
				address = low | high;
				memByte = getByte(address);
				

				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X)";

			} else if (opcode == 0xF1) {
				uint16_t address;
				address = ((((getByte(iByte2)) | (getByte(iByte2 + 1 & 0xFF)) << 8) + Y) & 0xFFFF);
				memByte = getByte(address);
				

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
			PS[C] = true;
            break;
		}

		case 0xF8: {				//SED
			
			PS[D] = true;
            break;
		}

		case 0x78: {				//SEI

			PS[I] = true;
            break;
		}

		case 0x83: 					//SAX
		case 0x87:
		case 0x97:
		case 0x8F: {				

			uint16_t address;

			if (opcode == 0x87) {
				address = iByte2;
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0x97) {
				address = ((iByte2 + Y) & 0xFF);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",Y";

			} else if (opcode == 0x8F) {
				address = (iByte2 | (iByte3 << 8));
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2;

			} else if (opcode == 0x9D) {
				address = (((iByte2 | (iByte3 << 8)) + X) & 0xFFFF);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0x99) {
				address = (((iByte2 | (iByte3 << 8)) + Y) & 0xFFFF);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",Y";

			} else if (opcode == 0x83) {
				uint16_t low;
				uint16_t high;
				low = getByte((iByte2 + X) & 0xFF);
				high = (getByte(iByte2 + 1 + X & 0xFF)) << 8;
				address = low | high;
				

				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X)";

			} else if (opcode == 0x93) {
				address = ((((getByte(iByte2)) | (getByte(iByte2 + 1 & 0xFF)) << 8) + Y) & 0xFFFF);
				

				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << "),Y";

			} else {
				return false;
			}

			setByte(address, A & X);
            break;
		}

		case 0x03:		//*SLO
		case 0x07:
		case 0x0F:
		case 0x13:
		case 0x17:
		case 0x1B:
		case 0x1F: {


			uint16_t address;
			

			if (opcode == 0x07) {
				address = iByte2;
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0x17) {
				address = ((iByte2 + X) & 0xFF);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0x0F) {
				address = (iByte2 | (iByte3 << 8));
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2;

			} else if (opcode == 0x1F) {
				address = (((iByte2 | (iByte3 << 8)) + X) & 0xFFFF);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0x1B) {
				address = (((iByte2 | (iByte3 << 8)) + Y) & 0xFFFF);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",Y";

			} else if (opcode == 0x03) {
				uint16_t low;
				uint16_t high;
				low = getByte((iByte2 + X) & 0xFF);
				high = (getByte(iByte2 + 1 + X & 0xFF)) << 8;
				address = low | high;
				

				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X)";

			} else if (opcode == 0x13) {
				address = ((((getByte(iByte2)) | (getByte(iByte2 + 1 & 0xFF)) << 8) + Y) & 0xFFFF);
				

				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << "),Y";

			} else {
				return false;
			}



			PS[C] = getBit(getByte(address), 7);

			setByte(address, ((getByte(address) << 1) & 0xFE));

			A |= getByte(address);

			PS[N] = getBit(A, 7);
			PS[Z] = (A == 0) ? true : false;


			break;
		}

		case 0x43:				//SRE
		case 0x47:
		case 0x4F:
		case 0x53:
		case 0x57:
		case 0x5B:
		case 0x5F: {


			uint16_t address;
			

			if (opcode == 0x47) {
				address = iByte2;
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0x57) {
				address = ((iByte2 + X) & 0xFF);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0x4F) {
				address = (iByte2 | (iByte3 << 8));
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2;

			} else if (opcode == 0x5F) {
				address = (((iByte2 | (iByte3 << 8)) + X) & 0xFFFF);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0x5B) {
				address = (((iByte2 | (iByte3 << 8)) + Y) & 0xFFFF);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",Y";

			} else if (opcode == 0x43) {
				uint16_t low;
				uint16_t high;
				low = getByte((iByte2 + X) & 0xFF);
				high = (getByte(iByte2 + 1 + X & 0xFF)) << 8;
				address = low | high;
				

				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X)";

			} else if (opcode == 0x53) {
				address = ((((getByte(iByte2)) | (getByte(iByte2 + 1 & 0xFF)) << 8) + Y) & 0xFFFF);
				

				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << "),Y";

			} else {
				return false;
			}

			PS[N] = 0;
			PS[C] = getBit(getByte(address), 0);

			setByte(address, (getByte(address) >> 1) & 0x7F);

			A ^= getByte(address);

			PS[N] = getBit(A, 7);
			PS[Z] = (A == 0) ? true : false;


			break;
		}



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
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;

			} else if (opcode == 0x95) {
				address = ((iByte2 + X) & 0xFF);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0x8D) {
				address = (iByte2 | (iByte3 << 8));
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2;

			} else if (opcode == 0x9D) {
				address = (((iByte2 | (iByte3 << 8)) + X) & 0xFFFF);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",X";

			} else if (opcode == 0x99) {
				address = (((iByte2 | (iByte3 << 8)) + Y) & 0xFFFF);
				

				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2 << ",Y";

			} else if (opcode == 0x81) {
				uint16_t low;
				uint16_t high;
				low = getByte((iByte2 + X) & 0xFF);
				high = (getByte(iByte2 + 1 + X & 0xFF)) << 8;
				address = low | high;
				

				if (debug) std::cout << "($" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X)";

			} else if (opcode == 0x91) {
				address = ((((getByte(iByte2)) | (getByte(iByte2 + 1 & 0xFF)) << 8) + Y) & 0xFFFF);
				

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

			uint16_t address;

			if (opcode == 0x86) {
				
				address = iByte2;
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;
			} else if (opcode == 0x96) {
				
				address = ((iByte2 + Y) & 0xFF);
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";
			} else if (opcode == 0x8E) {
				
				address = (iByte2 | (iByte3 << 8));
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2;
			} else {
				return false;
			}

			setByte(address, X);
            break;

		}

		case 0x84:				//STY
		case 0x94:
		case 0x8C: {

			uint16_t address;


			if (opcode == 0x84) {
				
				address = iByte2;
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2;
			} else if (opcode == 0x94) {
				
				address = ((iByte2 + X) & 0xFF);
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte2 << ",X";
			} else if (opcode == 0x8C) {
				
				address = (iByte2 | (iByte3 << 8));
				if (debug) std::cout << "$" << std::hex << std::uppercase << (unsigned int) iByte3 << (unsigned int) iByte2;
			} else {
				return false;
			}

			setByte(address, Y);
            break;
		}

		case 0xAA: {				//TAX			
			X = A;
			PS[N] = getBit(X, 7);
			PS[Z] = (X == 0) ? true : false;
            break;
		}

		case 0xA8: {				//TAY			
			Y = A;
			PS[N] = getBit(Y, 7);
			PS[Z] = (Y == 0) ? true : false;
            break;

		}
		case 0xBA: {				//TSX			
			X = SP;
			PS[N] = getBit(X, 7);
			PS[Z] = (X == 0) ? true : false;
            break;
		}

		case 0x8A: {				//TXA			
			A = X;
			PS[N] = getBit(A, 7);
			PS[Z] = (A == 0) ? true : false;
            break;
		}

		case 0x9A: {				//TXS			
			SP = X;
            break;
		}
		case 0x98: {				//TYA			
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

	//fix indirect overflow bug behaviour?

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
