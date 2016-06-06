#include <iostream>
#include <fstream>
#include <bitset>
#include <iomanip>

#include "NES.hpp"

enum InstructionType {
	READ,
	WRITE,
	READ_MODIFY_WRITE,
};
                         //0     1      2      3      4      5      6      7      8      9
const char * opnames[] = {"$$$", "ADC", "AND", "ASL", "BCC", "BCS", "BEQ", "BIT", "BMI", "BNE", //0
                          "BPL", "BRK", "BVC", "BVS", "CLC", "CLD", "CLI", "CLV", "CMP", "CPX", //1
                          "CPY","*DCP", "DEC", "DEX", "DEY", "EOR", "INC", "INX", "INY","*ISB", //2
                          "JMP", "JSR","*LAX", "LDA", "LDX", "LDY", "LSR", "NOP","*NOP", "ORA", //3
                          "PHA", "PHP", "PLA", "PLP","*RLA", "ROL", "ROR","*RRA", "RTI", "RTS", //4
                         "*SAX","*SBC", "SBC", "SEC", "SED", "SEI","*SLO","*SRE", "STA", "STX", //5
                          "STY", "TAX", "TAY", "TSX", "TXA", "TXS", "TYA"};                     //6

                       //0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
const int opnameMap[] = {11,39, 0,56,38,39, 3,56,41,39, 3, 0,38,39, 3,56,  //0
                         10,39, 0,56,38,39, 3,56,14,39,38,56,38,39, 3,56,  //1
                         31, 2, 0,44, 7, 2,45,44,43, 2,45, 0, 7, 2,45,44,  //2
                          8, 2, 0,44,38, 2,45,44,53, 2,38,44,38, 2,45,44,  //3
                         48,25, 0,57,38,25,36,57,40,25,36, 0,30,25,36,57,  //4
                         12,25, 0,57,38,25,36,57,16,25,38,57,38,25,36,57,  //5
                         49, 1, 0,47,38, 1,46,47,42, 1,46, 0,30, 1,46,47,  //6
                         13, 1, 0,47,38, 1,46,47,55, 1,38,47,38, 1,46,47,  //7
                         38,58, 0,50,60,58,59,50,24, 0,64, 0,60,58,59,50,  //8
                          4,58, 0, 0,60,58,59,50,66,58,65, 0, 0,58, 0, 0,  //9
                         35,33,34,32,35,33,34,32,62,33,61, 0,35,33,34,32,  //A
                          5,33, 0,32,35,33,34,32,17,33,63, 0,35,33,34,32,  //B
                         20,18, 0,21,20,18,22,21,28,18,23, 0,20,18,22,21,  //C
                          9,18, 0,21,38,18,22,21,15,18,38,21,38,18,22,21,  //D
                         19,52, 0,29,19,52,26,29,27,52,37,51,19,52,26,29,  //E
                          6,52, 0,29,38,52,26,29,54,52,38,29,38,52,26,29}; //F

static bool getBit(uint8_t, int);
static uint8_t getPswByte(bool *);
static void getPswFromByte(bool * PS, uint8_t byte);
static int addressCycles(enum AddressMode, enum InstructionType);

NES::NES() {

    for (int x = 0; x < 0x10000; x++) cpuMem[x] = 0x0;
    for (int x = 0; x < 0x800; x++) cpuRAM[x] = 0x0;
    for (int x = 0; x < 8; x++) PS[x] = false;

    PC = 0x8000;
    SP = 0xFD;
    A  = 0x0;
    X  = 0x0;
    Y  = 0x0;

    count = 0;

    PS[I] = true;

    return;
}

bool NES::openROM(const char * fileLoc) {

    if (fileLoc == NULL) {
        return false;
    }

    std::ifstream romFile;
    romFile.open(fileLoc, std::ios::binary);

    if (!romFile.is_open()) {
        return false;
    }

    uint8_t flags[16];

    uint8_t prg_rom_size = 0;   //number of 16KB ROM blocks
    uint8_t chr_rom_size = 0;   //number of 8KB VROM blocks
    uint8_t prg_ram_size = 0;   //number of 8KB RAM blocks

    uint8_t mapperNumber;

    bool batteryRAM = false;
    bool trainer = false;

    bool NTSC;      //false = PAL

    int prgRomBytes, chrRomBytes, prgRamBytes;

    int index;
    index = 0;

    while (romFile) {
        char c;
        romFile.get(c);
        unsigned char u = (unsigned char) c;
        int binaryValue = (int) u;

        if (index < 16) {
            flags[index] = binaryValue;
            index++;
        } else {
            if (index == 16) {
                if (flags[0] != 0x4E || flags[1] != 0x45 || flags[2] != 0x53 || flags[3] != 0x1A) {
                    std::cerr << "iNes file format not recognized!" << std::endl;
                    romFile.close();
                    return false;
                }
                prg_rom_size = flags[4];
                chr_rom_size = flags[5];

                if (prg_rom_size == 1) {
                	PC = 0xC000;
                }

                prgRomBytes = prg_rom_size * 0x4000;
                chrRomBytes = chr_rom_size * 0x2000;

                if (prg_ram_size == 0) {
                    prgRamBytes = 0x2000;
                } else {
                    prgRamBytes = (prg_ram_size * 0x2000);
                }

                PRG_ROM = new uint8_t[prgRomBytes];

                if (PRG_ROM == NULL) {
                    return false;
                }

                CHR_ROM = new uint8_t[chrRomBytes];

                if (CHR_ROM == NULL) {
                    delete [] PRG_ROM;
                    return false;
                }

                PRG_RAM = new uint8_t[prgRamBytes];

                if (PRG_RAM == NULL) {
                    delete [] CHR_ROM;
                    delete [] PRG_ROM;
                    return false;
                }

                for (int a = 0; a < prgRamBytes; a++) {
                    PRG_RAM[a] = 0x0;
                }

                //upper nybble of flag 6 is lower nybble of mapper number 
                mapperNumber = (flags[6] >> 4);

                //flags 6
                if ((flags[6] & 0x9) == 0x0) {
                    std::cout << "vertical arrangement, horizontal mirroring, CIRAM A10 = PPU A11" << std::endl;
                } else if ((flags[6] & 0x9) == 0x1) {
                    std::cout << "horizontal arrangement, vertical mirroring, CIRAM A10 = PPU A10" << std::endl;
                } else if ((flags[6] & 0x8) == 0x8) {
                    std::cout << "four screen VRAM" << std::endl;
                } else {
                    std::cerr << "Could not identify mirroring for " << (std::bitset<8>) flags[6] << std::endl;
                    romFile.close();
                    return false;
                }
                if ((flags[6] & 0x2) == 0x2) {
                    batteryRAM = true;
                }
                if ((flags[6] & 0x4) == 0x4) {
                    std::cout << "Trainer present" << std::endl;
                    trainer = true;
                }

                mapperNumber |= (flags[7] & 0xF0);

                if ((flags[7] & 0xC) == 0x8) {
                    std::cout << "NES 2.0 format" << std::endl;
                }
                if ((flags[7] & 0x1) == 0x1) {
                    std::cout << "VS Unisystem" << std::endl;
                }
                if ((flags[7] & 0x2) == 0x2) {
                    std::cout << "Playchoice-10" << std::endl;
                }

                prg_ram_size = flags[8];

                if ((flags[9] & 0x1) == 0x1) {
                    NTSC = false;
                } else {
                    NTSC = true;
                }
            }

            if (trainer) {
                romFile.close();
                return false;
            }

            if (index < 16 + prgRomBytes) {
                PRG_ROM[index - 16] = binaryValue;
            } else if (index < 16 + prgRomBytes + chrRomBytes) {
                CHR_ROM[index - (16 + prgRomBytes)] = binaryValue;
            }
            
            index++;
        }
    }

    numRomBanks = prg_rom_size;

    romFile.close();
    return true;
}

uint8_t NES::getByte(uint16_t memAddress) {

    if (memAddress >= 0x0000 && memAddress < 0x2000) {
        return cpuRAM[memAddress % 0x800];
    } else if (memAddress >= 0x8000 && memAddress < 0x10000) {
        
        if (numRomBanks == 1) {
            return PRG_ROM[ (memAddress - 0x8000) % 0x4000];
        } else if (numRomBanks == 2) {
            return PRG_ROM[memAddress - 0x8000];
        } else {
            std::cerr << "Unsupported ROM bank configuration" << std::endl;
            return 0;
        }
        
    } else if (memAddress >= 0x2000 && memAddress < 0x4000) {
        return ppuRegisters[ (memAddress - 0x2000) % 8 ];
    } else if (memAddress >= 0x4000 && memAddress < 0x4020) {
        return ioRegisters[ memAddress - 0x4000 ];
    } else if (memAddress >= 0x6000 && memAddress < 0x8000) {
        return PRG_RAM[memAddress - 0x6000];
    } else {
        return cpuMem[memAddress];
    }
}

bool NES::setByte(uint16_t memAddress, uint8_t byte) {

    if (memAddress >= 0x0000 && memAddress < 0x2000) {
        cpuRAM[memAddress] = byte;
        return true;
    } else if (memAddress >= 0x8000 && memAddress < 0x1000) {

        std::cerr << "Segmentation fault! Can't write to 0x" << std::hex << memAddress << std::endl;
        return false;
        
    } else if (memAddress >= 0x2000 && memAddress < 0x4000) {
        ppuRegisters[(memAddress - 0x2000) % 8] = byte;
        return true;
    } else if (memAddress >= 0x4000 && memAddress < 0x4018) { 
        ioRegisters[memAddress - 0x4000] = byte;
        return true;
    } else if (memAddress >= 0x6000 && memAddress < 0x8000) {
        PRG_RAM[memAddress - 0x6000] = byte;
        return true;
    } else {
        cpuMem[memAddress] = byte;
        return true;
    }
}

uint16_t NES::retrieveAddress(enum AddressMode mode) {

    uint8_t firstByte, secondByte;
    firstByte = getByte(PC + 1);
    secondByte = getByte(PC + 2);

    switch (mode) {

        case ZRP:
        return firstByte;

        case ZRPX:
        return ((firstByte + X) & 0xFF);

        case ZRPY:
        return ((firstByte + Y) & 0xFF);

        case ABS:
        return (firstByte | (secondByte << 8));

        case ABSX:
        return (((firstByte | (secondByte << 8)) + X) & 0xFFFF);

        case ABSY:
        return (((firstByte | (secondByte << 8)) + Y) & 0xFFFF);

        case IND: {
            uint16_t low, high;
            low = getByte((firstByte | (secondByte << 8)));
            high = getByte(((firstByte + 1) & 0xFF) | (secondByte << 8));
            return ((high << 8) | low);
        }

        case INDX: {
            uint8_t low, high;
            low = getByte((firstByte + X) & 0xFF);
            high = getByte((firstByte + 1 + X) & 0xFF);
            return ((high << 8) | low);
        }
        
        case INDY: 
        return ((((getByte(firstByte)) | (getByte((firstByte + 1) & 0xFF)) << 8) + Y) & 0xFFFF);
        
        default:
        return 0;
    }
}

void NES::debugPrintVal(enum AddressMode mode) {

    int firstByte, secondByte;
    firstByte = (int) getByte(PC + 1);
    secondByte = (int) getByte(PC + 2);

    switch (mode) {
        case ABS:
        std::cout << '$' << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << secondByte << std::setfill('0') << std::setw(2) << firstByte;
        return;

        case ABSX:
        std::cout << '$' << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << secondByte << std::setfill('0') << std::setw(2) << firstByte << ",X";
        return;

        case ABSY:
        std::cout << '$' << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << secondByte << std::setfill('0') << std::setw(2) << firstByte << ",Y";
        return;

        case ACC:
        std::cout << 'A';
        return;

        case IMM:
        std::cout << "#$" << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << firstByte;
        return;

        case IMP:
        return;

        case IND:
        std::cout << "$(" << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << secondByte << std::setfill('0') << std::setw(2) << firstByte << ')';
        return;

        case INDX:
        std::cout << "$(" << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << firstByte << ",X)";
        return;

        case INDY:
        std::cout << "$(" << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << firstByte << "),Y";
        return;

        case REL:
        return;

        case ZRP:
        std::cout << '$' << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << firstByte;
        return;

        case ZRPX:
        std::cout << '$' << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << firstByte << ",X";
        return;
        
        case ZRPY:
        std::cout << '$' << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << firstByte << ",Y";
        return;
        
        default:
        std::cerr << "Unrecognized addrees mode" << std::endl;
        return;
    }
}

int NES::executeNextOpcode(bool debug, bool verbose) {

	int cyc;
	cyc = 0;

	enum AddressMode opAddressMode;
	opAddressMode = addressModes[getByte(PC)];

	//get address if applicable to instruction
	uint16_t address;
    address = retrieveAddress(opAddressMode);

    //get byte from memory if applicable
    uint8_t memoryByte;
    if (addressModes[getByte(PC)] == IMM) {
        memoryByte = getByte(PC + 1);
    } else {
        memoryByte = getByte(address);
    }

    //fetch instruction bytes
    uint8_t opcode, iByte2, iByte3;
    opcode = getByte(PC);
    iByte2 = getByte(PC + 1);
    iByte3 = getByte(PC + 2);

    if (debug) {

    	std::cout << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << (int) PC << "  ";

    	if (opcodeLens[opcode % 0x20] == 1) {
    		std::cout << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int) opcode << "        ";
    	} else if (opcodeLens[opcode % 0x20] == 2 || opcode == 0xA2) {
    		std::cout << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int) opcode << " ";
    		std::cout << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int) iByte2 << "     ";
    	} else if (opcodeLens[opcode % 0x20] == 3) {
    		std::cout << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int) opcode << " ";
    		std::cout << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int) iByte2 << " ";
    		std::cout << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int) iByte3 << "  ";
    	} else {
    		std::cout << "          ";
    	}

        std::cout << opnames[opnameMap[opcode]] << ' ';
        debugPrintVal(addressModes[opcode]);

        std::cout << " " << std::dec << count;


    }

        //increment program counter
    PC += opcodeLens[opcode % 0x20];
    if (opcode == 0xA2) PC += 2;        //irregular opcode

    //always spend 2 cycles fetching opcode and next byte
    cyc += 2;
    
    switch (opcode) {

        //ADC
        case 0x69: case 0x65: case 0x75: case 0x6D: case 0x7D: case 0x79: case 0x61: case 0x71: {

        	cyc += addressCycles(opAddressMode, READ);

            uint16_t total;
            total = A + memoryByte + PS[C];
            int8_t num1, num2;
            num1 = (int8_t) A;
            num2 = (int8_t) memoryByte;
            PS[V] = (num1 + num2 + PS[C] < -128 || num1 + num2 + PS[C] > 127);  
            PS[C] = (total > 255) ? true : false;
            A = total & 0xFF;
            PS[Z] = (A == 0) ? true : false;
            PS[N] = getBit(A, 7);
            break;
        }

        //AND
        case 0x29: case 0x25: case 0x35: case 0x2D: case 0x3D: case 0x39: case 0x21: case 0x31: 

        cyc += addressCycles(opAddressMode, READ);

        A = A & memoryByte;
        PS[N] = getBit(A, 7);
        PS[Z] = (A == 0) ? true : false;
        break;

        //ASL
        case 0x0A: case 0x06: case 0x16: case 0x0E: case 0x1E: {

        	cyc += addressCycles(opAddressMode, READ_MODIFY_WRITE);

            if (opcode == 0x0A) {
                PS[C] = getBit(A, 7);
                A = ((A << 1) & 0xFE);
                PS[N] = getBit(A, 7);
                PS[Z] = (A == 0) ? true : false;
            } else {
                PS[C] = getBit(getByte(address), 7);
                setByte(address, ((getByte(address) << 1) & 0xFE));
                PS[N] = getBit(getByte(address), 7);
                PS[Z] = (getByte(address) == 0) ? true : false;
            }
            break;
        }

        case 0x90:             //BCC
        if (PS[C] == 0) {
        	if ( ((PC + (int8_t) iByte2) / 256) != (PC / 256)) {
        		cyc++;
        	}
        	PC += (int8_t) iByte2;
        	cyc++;

        }
        break;
        
        case 0xB0:            //BCS
        if (PS[C]) {
        	if ( ((PC + (int8_t) iByte2) / 256) != (PC / 256)) {
        		cyc++;
        	}
        	PC += (int8_t) iByte2;
        	cyc++;
        }
        break;
        
        case 0xF0:             //BEQ
        if (PS[Z]) {
        	if ( ((PC + (int8_t) iByte2) / 256) != (PC / 256)) {
        		cyc++;
        	}
        	PC += (int8_t) iByte2;
        	cyc++;
        }
        break;
        
        //BIT
        case 0x24: case 0x2C: {
        	cyc += addressCycles(opAddressMode, READ);

            uint8_t num;
            num = A & memoryByte;
            PS[N] = getBit(memoryByte, 7);
            PS[V] = getBit(memoryByte, 6);
            PS[Z] = (num == 0) ? true : false;
            break;
        }

        case 0x30:             //BMI
        if (PS[N]) {
        	if ( ((PC + (int8_t) iByte2) / 256) != (PC / 256)) {
        		cyc++;
        	}
        	PC += (int8_t) iByte2;
        	cyc++;
        }
        break;
        
        case 0xD0:             //BNE
        if (PS[Z] == 0) {
        	if ( ((PC + (int8_t) iByte2) / 256) != (PC / 256)) {
        		cyc++;
        	}
        	PC += (int8_t) iByte2;
        	cyc++;
        }
        break;
        
        case 0x10:             //BPL
        if (PS[N] == 0) {
        	if ( ((PC + (int8_t) iByte2) / 256) != (PC / 256)) {
        		cyc++;
        	}
        	PC += (int8_t) iByte2;
        	cyc++;
        }
        break;
        
        case 0x00: {            //BRK
            
            uint8_t high = (PC & 0xFF00) >> 8;
            setByte(0x100 + SP, high);
            SP--;
            cyc++;

            uint8_t low = PC & 0xFF;
            setByte(0x100 + SP, low);
            SP--;
            cyc++;

            uint8_t memByte = getPswByte(PS);
            setByte(0x100 + SP, memByte | 0x10);
            SP--;
            cyc++;

            low = getByte(0xFFFE);
            cyc++;

            high = getByte(0xFFFF);
            cyc++;

            PC = (high << 8) | low;
            break;
        }

        case 0x50:          //BVC
        if (PS[V] == 0) {
        	if ( ((PC + (int8_t) iByte2) / 256) != (PC / 256)) {
        		cyc++;
        	}
        	PC += (int8_t) iByte2;
        	cyc++;
        }
        break;
        
        case 0x70:          //BVS
        if (PS[V]) {
        	if ( ((PC + (int8_t) iByte2) / 256) != (PC / 256)) {
        		cyc++;
        	}
        	PC += (int8_t) iByte2;
        	cyc++;
        }
        break;
        
        case 0x18:          //CLC           
        PS[C] = false;
        break;
        
        case 0xD8:          //CLD           
        PS[D] = false;
        break;
        
        case 0x58:          //CLI           
        PS[I] = false;
        break;
        
        case 0xB8:          //CLV           
        PS[V] = false;
        break;

        //CMP
        case 0xC9: case 0xC5: case 0xD5: case 0xCD: case 0xDD: case 0xD9: case 0xC1: case 0xD1: {

        	cyc += addressCycles(opAddressMode, READ);

            int num;
            num = A - memoryByte;
            PS[N] = getBit(num, 7);
            PS[C] = (A >= memoryByte) ? true : false;
            PS[Z] = (num == 0) ? true : false;
            break;
        }
        //CPX
        case 0xE0: case 0xE4: case 0xEC: {
            int total;
            total = X - memoryByte;
            PS[N] = (total & 0x80) ? true : false;
            PS[C] = (X >= memoryByte) ? true : false;
            PS[Z] = (total == 0) ? 1 : 0;
            break;
        }

        //CPY
        case 0xC0: case 0xC4: case 0xCC: {
            int total;
            total = Y - memoryByte;
            PS[N] = (total & 0x80) ? true : false;
            PS[C] = (Y >= memoryByte) ? true : false;
            PS[Z] = (total == 0) ? 1 : 0;
            break;
        }

        //DCP
        case 0xC3: case 0xD3: case 0xC7: case 0xD7: case 0xCF: case 0xDF: case 0xDB: {
        	cyc += addressCycles(opAddressMode, READ_MODIFY_WRITE);
            setByte(address, (getByte(address) - 1) & 0xFF);
            memoryByte = getByte(address);
            int num;
            num = A - memoryByte;
            PS[N] = getBit(num, 7);
            PS[C] = (A >= memoryByte) ? true : false;
            PS[Z] = (num == 0) ? true : false;
            break;
        }

        //DEC
        case 0xC6: case 0xD6: case 0xCE: case 0xDE: 
        cyc += addressCycles(opAddressMode, READ_MODIFY_WRITE);
        setByte(address, ((getByte(address) - 1) & 0xFF));
        PS[N] = getBit(getByte(address), 7);
        PS[Z] = (getByte(address) == 0) ? true : false;
        break;
        
        case 0xCA:          //DEX
        X = (X - 1) & 0xFF;
        PS[Z] = (X == 0) ? true : false;
        PS[N] = getBit(X, 7);
        break;
        
        case 0x88:          //DEY           
        Y = (Y - 1) & 0xFF;
        PS[Z] = (Y == 0) ? true : false;
        PS[N] = getBit(Y, 7);
        break;
        
        //EOR
        case 0x49: case 0x45: case 0x55: case 0x4D: case 0x5D: case 0x59: case 0x41: case 0x51: 
        cyc += addressCycles(opAddressMode, READ);
        A = A ^ memoryByte;
        PS[N] = getBit(A, 7);
        PS[Z] = (A == 0) ? true : false;
        break;
        
        //INC
        case 0xE6: case 0xF6: case 0xEE: case 0xFE: 
        cyc += addressCycles(opAddressMode, READ_MODIFY_WRITE);
        setByte(address, ((getByte(address) + 1) & 0xFF));
        PS[N] = getBit(getByte(address), 7);
        PS[Z] = (getByte(address) == 0) ? true : false;
        break;
        
        case 0xE8:              //INX           
        X = (X + 1) & 0xFF;
        PS[Z] = (X == 0) ? true : false;
        PS[N] = getBit(X, 7);
        break;
        
        case 0xC8:              //INY           
        Y = (Y + 1) & 0xFF;
        PS[Z] = (Y == 0) ? true : false;
        PS[N] = getBit(Y, 7);
        break;

        //ISB
        case 0xE3: case 0xE7: case 0xF7: case 0xFB: case 0xEF: case 0xFF: case 0xF3: {     
        	cyc += addressCycles(opAddressMode, READ_MODIFY_WRITE);   
            setByte(address, (getByte(address) + 1) & 0xFF);
            memoryByte = getByte(address);
            int total;
            total = A - memoryByte - (!PS[C]);
            int8_t num1, num2;
            num1 = (int8_t) A;
            num2 = (int8_t) memoryByte;
            PS[V] = (num1 - num2 - (!PS[C]) < -128 || num1 - num2 - (!PS[C]) > 127);
            A = total & 0xFF;
            PS[C] = (total >= 0) ? true : false;
            PS[N] = getBit(total, 7);
            PS[Z] = (A == 0) ? 1 : 0;
            break;
        }

        //JMP
        case 0x4C: case 0x6C: 
        PC = address;
        cyc += (opAddressMode == ABS) ? 1 : 3;
        break;

        case 0x20: {            //JSR

        	cyc++;

            uint16_t store;
            store = PC;
            
            uint8_t high = (store & 0xFF00) >> 8;
            setByte(SP + 0x100, high);
            SP--;
            cyc++;

            uint8_t low = store & 0xFF;;
            setByte(SP + 0x100, low);
            SP--;
            cyc++;

            PC = (iByte2 | (iByte3 << 8));
            cyc++;
            break;
        }

        //LAX
        case 0xA3: case 0xA7: case 0xAF: case 0xB3: case 0xB7: case 0xBF:  
        cyc += addressCycles(opAddressMode, READ);    
        A = memoryByte;
        X = memoryByte;
        PS[N] = getBit(A, 7);
        PS[Z] = (A == 0) ? true : false;
        break;
        
        //LDA
        case 0xA9: case 0xA5: case 0xB5: case 0xAD: case 0xBD: case 0xB9: case 0xA1: case 0xB1: 
        cyc += addressCycles(opAddressMode, READ);
        A = memoryByte;
        PS[N] = getBit(A, 7);
        PS[Z] = (A == 0) ? true : false;
        break;
        
        //LDX
        case 0xA2: case 0xA6: case 0xB6: case 0xAE: case 0xBE: 
        cyc += addressCycles(opAddressMode, READ);
        X = memoryByte;
        PS[N] = getBit(X, 7);
        PS[Z] = (X == 0) ? true : false;
        break;
        
        //LDY
        case 0xA0: case 0xA4: case 0xB4: case 0xAC: case 0xBC: 
        cyc += addressCycles(opAddressMode, READ);
        Y = memoryByte;
        PS[N] = getBit(Y, 7);
        PS[Z] = (Y == 0) ? true : false;
        break;

        //LSR
        case 0x4A: case 0x46: case 0x56: case 0x4E: case 0x5E: {

        	cyc += addressCycles(opAddressMode, READ_MODIFY_WRITE);

            PS[N] = 0;
            if (opcode == 0x4A) {
                PS[C] = getBit(A, 0);
                A = (A >> 1) & 0x7F;
                PS[Z] = (A == 0) ? true : false;
            } else {
                PS[C] = getBit(getByte(address), 0);
                setByte(address, (getByte(address) >> 1) & 0x7F);
                PS[Z] = (getByte(address) == 0) ? true : false;
            }
            break;
        }

        //NOP
        case 0x04: case 0x44: case 0x64: case 0x0C: case 0x14: case 0x34: case 0x54: case 0x74:
        case 0xD4: case 0xF4: case 0x1A: case 0x3A: case 0x5A: case 0x7A: case 0xDA: case 0xFA:
        case 0x80: case 0x1C: case 0x3C: case 0x5C: case 0x7C: case 0xDC: case 0xFC: case 0xEA:  
        cyc += addressCycles(opAddressMode, READ);               
        break;
        
        //ORA
        case 0x09: case 0x05: case 0x15: case 0x0D: case 0x1D: case 0x19: case 0x01: case 0x11: 
        cyc += addressCycles(opAddressMode, READ);
        A = (A | memoryByte);
        PS[N] = getBit(A, 7);
        PS[Z] = (A == 0) ? true : false;
        break;
        
        case 0x48:              //PHA
        setByte(SP + 0x100, A);
        SP--;
        cyc++;
        break;

        case 0x08:                 //PHP
        setByte(SP + 0x100, getPswByte(PS));
        SP--;
        cyc++;
        break;
        
        case 0x68:              //PLA
        SP++;
        cyc++;

        A = getByte(SP + 0x100);
        cyc++;

        PS[N] = getBit(A, 7);
        PS[Z] = (A == 0) ? true : false;
        break;
        
        case 0x28:                 //PLP
        SP++;
        cyc++;

        getPswFromByte(PS, getByte(SP + 0x100));
        cyc++;

        break;
        
        //RLA
        case 0x23: case 0x27: case 0x2F: case 0x33: case 0x37: case 0x3B: case 0x3F: {
        	cyc += addressCycles(opAddressMode, READ_MODIFY_WRITE);
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

        //ROL
        case 0x2A: case 0x26: case 0x36: case 0x2E: case 0x3E: {
        	cyc += addressCycles(opAddressMode, READ_MODIFY_WRITE);
            bool store;
            if (opcode == 0x2A) {
                store = getBit(A, 7);
                A = (A << 1) & 0xFE;
                A |= PS[C];
                PS[Z] = (A == 0) ? true : false;
                PS[N] = getBit(A, 7);
            } else {
                store = getBit(getByte(address), 7);
                setByte(address, (getByte(address) << 1) & 0xFE);
                setByte(address, getByte(address) | PS[C]);
                PS[Z] = (getByte(address) == 0) ? true : false;
                PS[N] = getBit(getByte(address), 7);
            }
            PS[C] = store;
            break;
        }

        //ROR
        case 0x6A: case 0x66: case 0x76: case 0x6E: case 0x7E: {
        	cyc += addressCycles(opAddressMode, READ_MODIFY_WRITE);
            bool store;
            if (opcode == 0x6A) {
                store = getBit(A, 0);
                A = (A >> 1) & 0x7F;
                A |= (PS[C] ? 0x80 : 0x0);
                PS[Z] = (A == 0) ? true : false;
                PS[N] = getBit(A, 7);
            } else {
                store = getBit(getByte(address), 0);
                setByte(address, (getByte(address) >> 1) & 0x7F);
                setByte(address, getByte(address) | (PS[C] ? 0x80 : 0x0));
                PS[Z] = (getByte(address) == 0) ? true : false;
                PS[N] = getBit(getByte(address), 7);
            }
            PS[C] = store;
            break;
        }

        //RRA
        case 0x63: case 0x67: case 0x6F: case 0x73: case 0x77: case 0x7B: case 0x7F: {
        	cyc += addressCycles(opAddressMode, READ_MODIFY_WRITE);
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
            PS[V] = (num1 + num2 + PS[C] < -128 || num1 + num2 + PS[C] > 127);
            PS[C] = (total > 255) ? true : false;
            A = total & 0xFF;
            PS[Z] = (A == 0) ? true : false;
            PS[N] = getBit(A, 7);
            break;
        }

        case 0x40: {                //RTI
            SP++;
            cyc++;

            uint8_t memByte;
            memByte = getByte(SP + 0x100);
            getPswFromByte(PS, memByte);
            SP++;
            cyc++;

            uint16_t low = getByte(SP + 0x100);
            SP++;
            cyc++;

            uint16_t high = getByte(SP + 0x100) << 8;
            cyc++;

            PC = high | low;
            break;
        }

        case 0x60: {                //RTS
            SP++;
            cyc++;

            uint16_t low = getByte(SP + 0x100);
            SP++;
            cyc++;

            uint16_t high = getByte(SP + 0x100) << 8;
            cyc++;
            PC = (high | low);

            PC++;
            cyc++;
            break;
        }

        //SBC
        case 0xE9: case 0xE5: case 0xF5: case 0xED: case 0xFD: case 0xF9: case 0xE1: case 0xF1: case 0xEB: {
            cyc += addressCycles(opAddressMode, READ);
            int total;
            total = A - memoryByte - (!PS[C]);
            int8_t num1, num2;
            num1 = (int8_t) A;
            num2 = (int8_t) memoryByte;
            PS[V] = (num1 - num2 - (!PS[C]) < -128 || num1 - num2 - (!PS[C]) > 127);
            A = total & 0xFF;
            PS[C] = (total >= 0) ? true : false;
            PS[N] = getBit(total, 7);
            PS[Z] = (A == 0) ? 1 : 0;
            break;
        }

        case 0x38:              //SEC
        PS[C] = true;
        break;
        
        case 0xF8:              //SED
        PS[D] = true;
        break;
        
        case 0x78:              //SEI
        PS[I] = true;
        break;
        
        //SAX
        case 0x83: case 0x87: case 0x97: case 0x8F:
        cyc += addressCycles(opAddressMode, WRITE);          
        setByte(address, A & X);
        break;
        
        //*SLO
        case 0x03: case 0x07: case 0x0F: case 0x13: case 0x17: case 0x1B: case 0x1F: 
        cyc += addressCycles(opAddressMode, READ_MODIFY_WRITE);
        PS[C] = getBit(getByte(address), 7);
        setByte(address, ((getByte(address) << 1) & 0xFE));
        A |= getByte(address);
        PS[N] = getBit(A, 7);
        PS[Z] = (A == 0) ? true : false;
        break;

        //SRE
        case 0x43: case 0x47: case 0x4F: case 0x53: case 0x57: case 0x5B: case 0x5F: 
        cyc += addressCycles(opAddressMode, READ_MODIFY_WRITE);
        PS[N] = 0;
        PS[C] = getBit(getByte(address), 0);
        setByte(address, (getByte(address) >> 1) & 0x7F);
        A ^= getByte(address);
        PS[N] = getBit(A, 7);
        PS[Z] = (A == 0) ? true : false;
        break;

        //STA
        case 0x85: case 0x95: case 0x8D: case 0x9D: case 0x99: case 0x81: case 0x91: 
        cyc += addressCycles(opAddressMode, WRITE);
        setByte(address, A);
        break;
        
        //STX
        case 0x86: case 0x96: case 0x8E: 
        cyc += addressCycles(opAddressMode, WRITE);
        setByte(address, X);
        break;

        //STY
        case 0x84: case 0x94: case 0x8C: 
        cyc += addressCycles(opAddressMode, WRITE);
        setByte(address, Y);
        break;

        case 0xAA:              //TAX           
        X = A;
        PS[N] = getBit(X, 7);
        PS[Z] = (X == 0) ? true : false;
        break;
        
        case 0xA8:              //TAY           
        Y = A;
        PS[N] = getBit(Y, 7);
        PS[Z] = (Y == 0) ? true : false;
        break;

        case 0xBA:              //TSX           
        X = SP;
        PS[N] = getBit(X, 7);
        PS[Z] = (X == 0) ? true : false;
        break;

        case 0x8A:              //TXA           
        A = X;
        PS[N] = getBit(A, 7);
        PS[Z] = (A == 0) ? true : false;
        break;
        
        case 0x9A:              //TXS           
        SP = X;
        break;
        
        case 0x98:              //TYA           
        A = Y;
        PS[N] = getBit(A, 7);
        PS[Z] = (A == 0) ? true : false;
        break;
        
        default: 
        std:: cout << "Unrecognized opcode : " << std::hex << (int) opcode << std::endl;
        return 0;
    }

    if (debug) std::cout << std::endl;

    return cyc;
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

static uint8_t getPswByte(bool * PS) {
    uint8_t P;
    P = 0;
    if (PS[C]) P |= 0x1;
    if (PS[Z]) P |= 0x2;
    if (PS[I]) P |= 0x4;
    if (PS[D]) P |= 0x8;
    if (PS[V]) P |= 0x40;
    if (PS[N]) P |= 0x80;
    P |= 0x20;
    return P;
}

static void getPswFromByte(bool * PS, uint8_t byte) {
    PS[N] = getBit(byte, 7);
    PS[V] = getBit(byte, 6);
    PS[D] = getBit(byte, 3);
    PS[I] = getBit(byte, 2);
    PS[Z] = getBit(byte, 1);
    PS[C] = getBit(byte, 0);
    return;
}

static int addressCycles(enum AddressMode mode, enum InstructionType type) {

	switch (type) {
		case READ: {

			switch (mode) {

				case IMM:
				return 0;

				case ACC:
				return 0;

				case ABS:
				return 2;

				case ABSX:
				return 2;

				case ABSY:
				return 2;

				case INDX:
				return 4;

				case INDY:
				return 3;

				case ZRP:
				return 1;

				case ZRPX:
				return 2;

				case ZRPY:
				return 2;

				default:
				return 0;
			}

		}

		case READ_MODIFY_WRITE: {

			switch (mode) {

				case IMM:
				return 0;

				case ACC:
				return 0;

				case ABS:
				return 4;

				case ABSX:
				return 5;

				return 3;

				case ZRP:
				return 3;

				case ZRPX:
				return 4;

				case ZRPY:
				return 4;

				default:
				return 0;
			}

		}

		case WRITE: {

			switch (mode) {

				case ZRP:
				return 1;

				case ZRPX:
				return 2;

				case ZRPY:
				return 2;

				case ABS:
				return 2;

				case ABSX:
				return 3;

				case ABSY:
				return 3;

				case INDX:
				return 4;

				case INDY:
				return 4;

				default:
				return 0;
			}

		}
		

		default:
		return 0;
	}
}