#include <iostream>
#include <cstring>
#include <iomanip>

#include "CPU.hpp"
#include "mappers.hpp"

const int lengthTable[] = {
    10,254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30,
};


#define getBit(num, bit)    (bit == 0) ? (num & 0x1) : (bit == 1) ? (num & 0x2) :   \
                            (bit == 2) ? (num & 0x4) : (bit == 3) ? (num & 0x8) :   \
                            (bit == 4) ? (num & 0x10) : (bit == 5) ? (num & 0x20) : \
                            (bit == 6) ? (num & 0x40) : (num & 0x80)

//used to calculate number of cpu ticks for each instruction
enum InstructionType {
    READ   = 0,     //read from mem address
    WRITE  = 1,     //write to mem address
    R_M_W  = 2,     //read from mem address, modify byte, write byte (read modify write)
    WRITE2 = 3,     //illegal opcodes
    OTHER  = 4,     //note: above 4 are not applicable to every opcode
};

//memory addressing mode for each opcode
static const enum AddressMode addressModes[] = {
  //0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
    IMP, INDX,NONE,INDX,ZRP ,ZRP ,ZRP ,ZRP ,IMP, IMM, ACC, NONE,ABS, ABS ,ABS, ABS,   //0
    REL, INDY,NONE,INDY,ZRPX,ZRPX,ZRPX,ZRPX,IMP, ABSY,IMP, ABSY,ABSX,ABSX,ABSX,ABSX,  //1
    ABS, INDX,NONE,INDX,ZRP, ZRP, ZRP, ZRP, IMP, IMM, ACC, NONE,ABS, ABS, ABS, ABS,   //2
    REL, INDY,NONE,INDY,ZRPX,ZRPX,ZRPX,ZRPX,IMP, ABSY,IMP, ABSY,ABSX,ABSX,ABSX,ABSX,  //3
    IMP, INDX,NONE,INDX,ZRP, ZRP, ZRP, ZRP, IMP, IMM, ACC, NONE,ABS, ABS, ABS, ABS,   //4
    REL, INDY,NONE,INDY,ZRPX,ZRPX,ZRPX,ZRPX,IMP, ABSY,IMP, ABSY,ABSX,ABSX,ABSX,ABSX,  //5
    IMP, INDX,NONE,INDX,ZRP, ZRP, ZRP, ZRP, IMP, IMM, ACC, NONE,IND, ABS, ABS, ABS,   //6
    REL, INDY,NONE,INDY,ZRPX,ZRPX,ZRPX,ZRPX,IMP, ABSY,IMP, ABSY,ABSX,ABSX,ABSX,ABSX,  //7
    IMM, INDX,NONE,INDX,ZRP, ZRP, ZRP, ZRP, IMP, NONE,IMP, NONE,ABS, ABS, ABS, ABS,   //8
    REL, INDY,NONE,INDY,ZRPX,ZRPX,ZRPY,ZRPY,IMP, ABSY,IMP, NONE,NONE,ABSX,NONE,NONE,  //9
    IMM, INDX,IMM, INDX,ZRP, ZRP, ZRP, ZRP, IMP, IMM, IMP, NONE,ABS, ABS, ABS, ABS,   //A
    REL, INDY,NONE,INDY,ZRPX,ZRPX,ZRPY,ZRPY,IMP, ABSY,IMP, NONE,ABSX,ABSX,ABSY,ABSY,  //B
    IMM, INDX,NONE,INDX,ZRP, ZRP, ZRP, ZRP, IMP, IMM, IMP, NONE,ABS, ABS, ABS, ABS,   //C
    REL, INDY,NONE,INDY,ZRPX,ZRPX,ZRPX,ZRPX,IMP, ABSY,IMP, ABSY,ABSX,ABSX,ABSX,ABSX,  //D
    IMM, INDX,NONE,INDX,ZRP, ZRP, ZRP, ZRP, IMP, IMM, IMP, IMM, ABS, ABS, ABS, ABS,   //E
    REL, INDY,NONE,INDY,ZRPX,ZRPX,ZRPX,ZRPX,IMP, ABSY,IMP, ABSY,ABSX,ABSX,ABSX,ABSX,  //F
}; 

//opcode mapping to assembly mnemonics and instruction type
static const int opnameMap[] = {
   //0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
    11,39, 0,56,38,39, 3,56,41,39, 3, 0,38,39, 3,56,  //0
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
     6,52, 0,29,38,52,26,29,54,52,38,29,38,52,26,29,  //F
};

//opcode mnemonics for debugging
static const char * opnames[] = {
    //0     1      2      3      4      5      6      7      8      9
    "$$$", "ADC", "AND", "ASL", "BCC", "BCS", "BEQ", "BIT", "BMI", "BNE", //0
    "BPL", "BRK", "BVC", "BVS", "CLC", "CLD", "CLI", "CLV", "CMP", "CPX", //1
    "CPY","*DCP", "DEC", "DEX", "DEY", "EOR", "INC", "INX", "INY","*ISB", //2
    "JMP", "JSR","*LAX", "LDA", "LDX", "LDY", "LSR", "NOP","*NOP", "ORA", //3
    "PHA", "PHP", "PLA", "PLP","*RLA", "ROL", "ROR","*RRA", "RTI", "RTS", //4
   "*SAX","*SBC", "SBC", "SEC", "SED", "SEI","*SLO","*SRE", "STA", "STX", //5
    "STY", "TAX", "TAY", "TSX", "TXA", "TXS", "TYA"                       //6
};      

//opcode types for tick calculation
static const enum InstructionType opInstrTypes[] = {
    //0    1      2      3      4      5      6      7      8      9   
    OTHER, READ,  READ,  R_M_W, OTHER, OTHER, OTHER, READ,  OTHER, OTHER, //0
    OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, READ,  READ,  //1
    READ,  WRITE2,R_M_W, OTHER, OTHER, READ,  R_M_W, OTHER, OTHER, WRITE2,//2
    OTHER, OTHER, READ,  READ,  READ,  READ,  R_M_W, READ,  READ,  READ,  //3
    OTHER, OTHER, OTHER, OTHER, WRITE2,R_M_W, R_M_W, WRITE2,OTHER, OTHER, //4
    WRITE, READ,  READ,  OTHER, OTHER, OTHER, WRITE2,WRITE2,WRITE, WRITE, //5
    WRITE, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER                       //6
};         

//cpu cycles taken for different address modes
static const int cycles[] = {
    2,2,2,0,0,0,0,4,3,0,1,2,2,  //read
    2,3,3,0,0,0,0,4,4,0,1,2,2,  //write
    4,5,5,0,0,0,0,0,0,0,3,4,4,  //read modify write
    4,5,5,2,2,2,2,6,6,2,3,4,4,  //write2
    0,0,0,0,0,0,0,0,0,0,0,0,0,  //other
};

//table of opcode lengths for advancing PC
                                   //0,1,2,3,4,5,6,7,8,9,A,B,C,D,E,F
static const int opcodeLens[0x20] = {2,2,0,2,2,2,2,2,1,2,1,2,3,3,3,3,  //0 2 4 6 8 A C E
                                     2,2,0,2,2,2,2,2,1,3,1,3,3,3,3,3}; //1 3 5 7 9 B D F

static inline uint8_t getPswByte(bool *);
static inline void getPswFromByte(bool * PS, uint8_t byte);
static void printByte(uint8_t);
static int debugPrintVal(enum AddressMode, int, int);
static void printDebugLine(uint16_t, uint8_t, uint8_t, uint8_t, enum AddressMode, 
    uint16_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, bool *, int);

CPU::CPU() {

    nesPPU = PPU();
    nesAPU = APU();

    for (int x = 0; x < 0x2000; x++) cpuMem[x] = 0x0;
    for (int x = 0; x < 0x800; x++) RAM[x] = 0x0;
    for (int x = 0; x < 8; x++) PS[x] = false;

    PRG_ROM = PRG_RAM = NULL;
    SP = 0xFD;
    A = X = Y = 0x0;
    NMI = false;
    PS[I] = true;
    cpuClock = 0;
    controllerByte = storedControllerByte = currentControllerBit = 0;
    readController = false;

    return;
}

void CPU::freePointers() {
    if (PRG_ROM != NULL) {
        delete [] PRG_ROM;
    }
    if (PRG_RAM != NULL) {
        delete [] PRG_RAM;
    }
    return;
}

inline bool CPU::returnControllerBit() {

    if (currentControllerBit < 8) {
        currentControllerBit++;
        bool bit;
        bit = getBit(storedControllerByte, currentControllerBit - 1);
        return bit;
    } else {
        return true;
    }

}

inline uint8_t CPU::getCpuByte(uint16_t memAddress, bool silent) {

    if (memAddress < 0x2000) {
        return RAM[memAddress % 0x800];
    } else if (memAddress < 0x4000) {
        uint16_t address;
        address = (memAddress - 0x2000) % 8;

        if (!silent) {
            nesPPU.readFlag = address;

            if (address == 0x2) {
        
                
                /*
                //ppu/cpu synchronization hack
                if (  (nesPPU.ppuClock % (262 * 341)) < (341 * 241 + 1) && (cpuClock % (262 * 341)) >= (341 * 241 + 1) ) {
                    nesPPU.ppuRegisters[2] |= 0x80;
                    nesPPU.suppressVBL = true;
                    NMI = false;
                } else if (  (nesPPU.ppuClock % (262 * 341)) < (341 * 241 + 0 ) && (cpuClock % (262 * 341)) >= (341 * 241 + 0) ) {
                    nesPPU.suppressVBL = true;
                    NMI = false;
                } else if (  (nesPPU.ppuClock % (262 * 341)) < (341 * 261 + 1) && (cpuClock % (262 * 341)) >= (341 * 261 + 1) ) {
                    NMI = false;
                    nesPPU.ppuRegisters[2] &= 0x1F;
                    NMI = false;
                }
                */
                
                
                

            } else if (address == 0x4) {

                if ((nesPPU.ppuRegisters[2] & 0x80) || ((nesPPU.ppuRegisters[1] & 0x18) == 0)) {
                    return nesPPU.OAM[nesPPU.oamAddress];
                } else {
                    nesPPU.oamAddress++;
                    return nesPPU.OAM[nesPPU.oamAddress - 1];
                }

            } else if (address == 0x7) {
                return nesPPU.return2007();
            }

        }

        return nesPPU.ppuRegisters[address];
    } else if (memAddress < 0x4020) {

        if (memAddress == 0x4016) {
            if (readController) {
                return returnControllerBit();
            }
        }
        
        return nesAPU.registers[ memAddress - 0x4000 ];
    } else if (memAddress < 0x6000) {
        return cpuMem[memAddress - 0x4000];
    } else if (memAddress < 0x8000) {
        /* Family Basic only: PRG RAM, mirrored as necessary to fill entire 8 KiB window, write protectable with an external switch */
        return PRG_RAM[memAddress - 0x6000];
    } else {
        if (cpuMapper == 0) {
            return getCpuMapper0(memAddress, numRomBanks, PRG_ROM);
        } else {
            std::cerr << "Fatal error, mapper not recognized in getCpuByte()" << std::endl;
            exit(EXIT_FAILURE);
            return 0;
        }
    }
}

inline void CPU::setCpuByte(uint16_t memAddress, uint8_t byte) {

    if (memAddress < 0x2000) {

        RAM[memAddress % 0x800] = byte;

    } else if (memAddress < 0x4000) {

        uint16_t address;
        address = (memAddress - 0x2000) % 8;
        nesPPU.writeFlag = address;
        nesPPU.ppuRegisters[address] = byte;

    } else if (memAddress < 0x4020) { 

        if (memAddress == 0x4014) {

            uint8_t OAMDMA;
            OAMDMA = byte;
            for (unsigned int x = 0; x < 0x100; x++) {
                nesPPU.OAM[nesPPU.oamAddress] = getCpuByte( (OAMDMA << 8) + x , false);
                nesPPU.oamAddress = (nesPPU.oamAddress + 1) & 0xFF;
            }

            if (cpuClock % 2 == 1) {
                //odd cycle
                cpuClock += 514 * 3; 
            } else {
                //even cycle
                cpuClock += 513 * 3; 
            }

        } else if (memAddress == 0x4015) {

            
        } else if (memAddress == 0x4016) {
            if ((byte & 0x1)) {
                //controller state is read into shift registers
                storedControllerByte = controllerByte;
                readController = false;
            } else {
                //serial controller data can start being read starting from bit 0
                currentControllerBit = 0;
                readController = true;
            }
        }
        nesAPU.registers[memAddress - 0x4000] = byte;

    } else if (memAddress < 0x6000) {

        cpuMem[memAddress - 0x4000] = byte;

    } else if (memAddress < 0x8000) {

        PRG_RAM[memAddress - 0x6000] = byte;

    } else {
        /* memAddress >= 0x8000 && memAddress <= 0xFFFF */
        std::cerr << "Segmentation fault! Can't write to 0x" << std::hex << memAddress << std::endl;
        exit(EXIT_FAILURE);
        return;
    }

    return;
}

inline uint16_t CPU::retrieveCpuAddress(enum AddressMode mode, bool * pagePass,
    uint8_t firstByte, uint8_t secondByte) {

    *pagePass = false;

    switch (mode) {

        case ZRP:
        return firstByte;

        case ZRPX:
        return ((firstByte + X) & 0xFF);

        case ZRPY:
        return ((firstByte + Y) & 0xFF);

        case ABS:
        return (firstByte | (secondByte << 8));

        case ABSX: {
            uint16_t before = (firstByte | (secondByte << 8));
            uint16_t after = ((before + X) & 0xFFFF);
            if ((before / 256) != (after/256)) *pagePass = true;
            return after;
        }

        case ABSY: {
            uint16_t before = (firstByte | (secondByte << 8));
            uint16_t after = ((before + Y) & 0xFFFF);
            if ((before / 256) != (after/256)) *pagePass = true;
            return after;
        }

        case IND: {
            uint8_t low = getCpuByte((firstByte | (secondByte << 8)), false);
            uint8_t high = getCpuByte(((firstByte + 1) & 0xFF) | (secondByte << 8), false);
            return ((high << 8) | low);
        }

        case INDX: {
            uint8_t low = getCpuByte((firstByte + X) & 0xFF, false);
            uint8_t high = getCpuByte((firstByte + 1 + X) & 0xFF, false);
            return ((high << 8) | low);
        }
        
        case INDY: {
            uint8_t low = (getCpuByte(firstByte, false));
            uint8_t high = (getCpuByte((firstByte + 1) & 0xFF, false));
            uint16_t before = (low | (high << 8));
            uint16_t after = ((before + Y) & 0xFFFF);
            if (( before/ 256) != (after/256)) *pagePass = true;
            return after;
        }

        default:
        std::cerr << "Fatal error. Addressing type not recognized" << std::endl;
        exit(EXIT_FAILURE);
        return 0;
    }
}

void CPU::executeNextOpcode(bool debug) {

    if (NMI) {
        uint16_t store;
        store = PC;

        uint8_t high = (store & 0xFF00) >> 8;
        setCpuByte(SP + 0x100, high);
        SP--;

        uint8_t low = store & 0xFF;;
        setCpuByte(SP + 0x100, low);
        SP--;

        setCpuByte(0x100 + SP, getPswByte(PS) | 0x10);
        SP--;

        PC = getCpuByte(0xFFFA, false) | (getCpuByte(0xFFFB, false) << 8);    //nmi handler

        NMI = false;
        cpuClock += 21;

        return;
    }

    /* PREPARE TO EXECUTE OPCODE */

    cpuClock += 6;        //always spend 2 cycles fetching opcode and next byte
    uint8_t opcode = getCpuByte(PC, false);
    uint8_t iByte2 = getCpuByte(PC + 1, false);
    uint8_t iByte3 = getCpuByte(PC + 2, false);
    enum AddressMode opAddressMode = addressModes[opcode];
    bool pass = false;    //page boundy cross?

    uint16_t address = 0;
    if (opAddressMode != ACC && opAddressMode != IMM && opAddressMode != REL && opAddressMode != IMP) {
        address = retrieveCpuAddress(opAddressMode, &pass, iByte2, iByte3);
    }

    enum InstructionType instrType = opInstrTypes[opnameMap[opcode]];
    cpuClock += cycles[instrType * 13 + opAddressMode] * 3;

    if (instrType == READ) {
        if (pass) {
            cpuClock += 3;
        }
    }

    /*
    if (nesPPU.ppuRegisters[1] & 0x8) {
        if (nesPPU.ppuClock % (262 * 341 * 2) > cpuClock % (262 * 341 * 2)) {
            cpuClock++;
            nesPPU.suppressCpuTickSkip = true;
        }
    }
    */
    


    uint8_t memoryByte;
    if (opAddressMode == IMM) {
        memoryByte = iByte2;
    } else if (opAddressMode == NONE || opAddressMode == ACC
        || opAddressMode == IMP || opAddressMode == REL) {
        memoryByte = 0;
    } else {

        switch(opcode) {
            case 0x85: case 0x95: case 0x8D: case 0x9D: case 0x99:
            case 0x81: case 0x91: case 0x86: case 0x96: case 0x8E:
            case 0x84: case 0x94: case 0x8C: case 0x83: case 0x87:
            case 0x97: case 0x8F:
            memoryByte = 0;
            break;

            default:
            /* this selectiveness is needed to prevent spurious reads to $2007, $4014 and
            othe registers which may incorrectly affect the state of the cpu */
            memoryByte = getCpuByte(address, false);
            break;
        }
    }

    if (debug) {
        printDebugLine(address, opcode, iByte2, iByte3, opAddressMode, 
            PC, memoryByte, A, X, Y, SP, PS, nesPPU.ppuClock);
        std::cout << std::endl;
    }
    
    PC += opcodeLens[opcode % 0x20];    //increment program counter
    if (opcode == 0xA2) PC += 2;        //irregular opcode

    /* END PREPERATION */
    
    switch (opcode) {

        //ADC
        case 0x69: case 0x65: case 0x75: case 0x6D: case 0x7D: case 0x79: case 0x61: case 0x71: {
            uint16_t total;
            total = A + memoryByte + PS[C];
            PS[V] = (((int8_t) A) + ((int8_t) memoryByte) + PS[C] < -128
                || ((int8_t) A) + ((int8_t) memoryByte) + PS[C] > 127);  
            PS[C] = (total > 255) ? true : false;
            A = total & 0xFF;
            PS[Z] = (A == 0) ? true : false;
            PS[N] = getBit(A, 7);
            break;
        }

        //AND
        case 0x29: case 0x25: case 0x35: case 0x2D: case 0x3D: case 0x39: case 0x21: case 0x31: 
        A = A & memoryByte;
        PS[N] = getBit(A, 7);
        PS[Z] = (A == 0) ? true : false;
        break;

        //ASL
        case 0x0A: case 0x06: case 0x16: case 0x0E: case 0x1E: {
            if (opcode == 0x0A) {
                PS[C] = getBit(A, 7);
                A = ((A << 1) & 0xFE);
                PS[N] = getBit(A, 7);
                PS[Z] = (A == 0) ? true : false;
            } else {
                PS[C] = getBit(getCpuByte(address, true), 7);
                setCpuByte(address, ((getCpuByte(address, true) << 1) & 0xFE));
                PS[N] = getBit(getCpuByte(address, true), 7);
                PS[Z] = (getCpuByte(address, true) == 0) ? true : false;
            }
            break;
        }

        case 0x90:             //BCC
        if (PS[C] == 0) {
            if ( ((PC + (int8_t) iByte2) / 256) != (PC / 256)) cpuClock += 3;
            PC += (int8_t) iByte2;
            cpuClock += 3;
        }
        break;
        
        case 0xB0:            //BCS
        if (PS[C]) {
            if ( ((PC + (int8_t) iByte2) / 256) != (PC / 256)) cpuClock += 3;
            PC += (int8_t) iByte2;
            cpuClock += 3;
        }
        break;
        
        case 0xF0:             //BEQ
        if (PS[Z]) {
            if ( ((PC + (int8_t) iByte2) / 256) != (PC / 256)) cpuClock += 3;
            PC += (int8_t) iByte2;
            cpuClock += 3;
        }
        break;
        
        //BIT
        case 0x24: case 0x2C: {
            uint8_t num;
            num = A & memoryByte;
            PS[N] = getBit(memoryByte, 7);
            PS[V] = getBit(memoryByte, 6);
            PS[Z] = (num == 0) ? true : false;
            break;
        }

        case 0x30:             //BMI
        if (PS[N]) {
            if ( ((PC + (int8_t) iByte2) / 256) != (PC / 256)) cpuClock += 3;
            PC += (int8_t) iByte2;
            cpuClock += 3;
        }
        break;
        
        case 0xD0:             //BNE
        if (PS[Z] == 0) {
            if ( ((PC + (int8_t) iByte2) / 256) != (PC / 256)) cpuClock += 3;
            PC += (int8_t) iByte2;
            cpuClock += 3;
        }
        break;
        
        case 0x10:             //BPL
        if (PS[N] == 0) {
            if ( ((PC + (int8_t) iByte2) / 256) != (PC / 256)) cpuClock += 3;
            PC += (int8_t) iByte2;
            cpuClock += 3;
        }
        break;
        
        case 0x00: {            //BRK
            uint8_t high = (PC & 0xFF00) >> 8;
            setCpuByte(0x100 + SP, high);
            SP--;
            cpuClock += 3;

            uint8_t low = PC & 0xFF;
            setCpuByte(0x100 + SP, low);
            SP--;
            cpuClock += 3;

            uint8_t memByte = getPswByte(PS);
            setCpuByte(0x100 + SP, memByte | 0x10);
            SP--;
            cpuClock += 3;

            low = getCpuByte(0xFFFE, false);
            cpuClock += 3;

            high = getCpuByte(0xFFFF, false);
            cpuClock += 3;

            PC = (high << 8) | low;
            break;
        }

        case 0x50:          //BVC
        if (PS[V] == 0) {
            if ( ((PC + (int8_t) iByte2) / 256) != (PC / 256)) cpuClock += 3;
            PC += (int8_t) iByte2;
            cpuClock += 3;
        }
        break;
        
        case 0x70:          //BVS
        if (PS[V]) {
            if ( ((PC + (int8_t) iByte2) / 256) != (PC / 256)) cpuClock += 3;
            PC += (int8_t) iByte2;
            cpuClock += 3;
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
            setCpuByte(address, (getCpuByte(address, true) - 1) & 0xFF);
            memoryByte = getCpuByte(address, true);
            int num;
            num = A - memoryByte;
            PS[N] = getBit(num, 7);
            PS[C] = (A >= memoryByte) ? true : false;
            PS[Z] = (num == 0) ? true : false;
            break;
        }

        //DEC
        case 0xC6: case 0xD6: case 0xCE: case 0xDE: 
        setCpuByte(address, ((getCpuByte(address, true) - 1) & 0xFF));
        PS[N] = getBit(getCpuByte(address, true), 7);
        PS[Z] = (getCpuByte(address, true) == 0) ? true : false;
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
        A = A ^ memoryByte;
        PS[N] = getBit(A, 7);
        PS[Z] = (A == 0) ? true : false;
        break;
        
        //INC
        case 0xE6: case 0xF6: case 0xEE: case 0xFE: 
        setCpuByte(address, ((getCpuByte(address, true) + 1) & 0xFF));
        PS[N] = getBit(getCpuByte(address, true), 7);
        PS[Z] = (getCpuByte(address, true) == 0) ? true : false;
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
            setCpuByte(address, (getCpuByte(address, true) + 1) & 0xFF);
            memoryByte = getCpuByte(address, true);
            int total;
            total = A - memoryByte - (!PS[C]);
            PS[V] = (((int8_t) A) - ((int8_t) memoryByte) - (!PS[C]) < -128 
                || ((int8_t) A) - ((int8_t) memoryByte) - (!PS[C]) > 127);
            A = total & 0xFF;
            PS[C] = (total >= 0) ? true : false;
            PS[N] = getBit(total, 7);
            PS[Z] = (A == 0) ? 1 : 0;
            break;
        }

        //JMP
        case 0x4C: case 0x6C: 
        PC = address;
        cpuClock += (opAddressMode == ABS) ? 3 : 9;
        break;

        case 0x20: {            //JSR
            cpuClock += 3;

            uint16_t store;
            store = PC;
            
            uint8_t high = (store & 0xFF00) >> 8;
            setCpuByte(SP + 0x100, high);
            SP--;
            cpuClock += 3;

            uint8_t low = store & 0xFF;;
            setCpuByte(SP + 0x100, low);
            SP--;
            cpuClock += 3;

            PC = (iByte2 | (iByte3 << 8));
            cpuClock += 3;
            break;
        }

        //LAX
        case 0xA3: case 0xA7: case 0xAF: case 0xB3: case 0xB7: case 0xBF:  
        A = memoryByte;
        X = memoryByte;
        PS[N] = getBit(A, 7);
        PS[Z] = (A == 0) ? true : false;
        break;
        
        //LDA
        case 0xA9: case 0xA5: case 0xB5: case 0xAD: case 0xBD: case 0xB9: case 0xA1: case 0xB1: 
        A = memoryByte;
        PS[N] = getBit(A, 7);
        PS[Z] = (A == 0) ? true : false;
        break;
        
        //LDX
        case 0xA2: case 0xA6: case 0xB6: case 0xAE: case 0xBE: 
        X = memoryByte;
        PS[N] = getBit(X, 7);
        PS[Z] = (X == 0) ? true : false;
        break;
        
        //LDY
        case 0xA0: case 0xA4: case 0xB4: case 0xAC: case 0xBC: 
        Y = memoryByte;
        PS[N] = getBit(Y, 7);
        PS[Z] = (Y == 0) ? true : false;
        break;

        //LSR
        case 0x4A: case 0x46: case 0x56: case 0x4E: case 0x5E: {
            PS[N] = 0;
            if (opcode == 0x4A) {
                PS[C] = getBit(A, 0);
                A = (A >> 1) & 0x7F;
                PS[Z] = (A == 0) ? true : false;
            } else {
                PS[C] = getBit(getCpuByte(address, true), 0);
                setCpuByte(address, (getCpuByte(address, true) >> 1) & 0x7F);
                PS[Z] = (getCpuByte(address, true) == 0) ? true : false;
            }
            break;
        }

        //NOP
        case 0x04: case 0x44: case 0x64: case 0x0C: case 0x14: case 0x34: case 0x54: case 0x74:
        case 0xD4: case 0xF4: case 0x1A: case 0x3A: case 0x5A: case 0x7A: case 0xDA: case 0xFA:
        case 0x80: case 0x1C: case 0x3C: case 0x5C: case 0x7C: case 0xDC: case 0xFC: case 0xEA:  
        break;
        
        //ORA
        case 0x09: case 0x05: case 0x15: case 0x0D: case 0x1D: case 0x19: case 0x01: case 0x11: 
        A = (A | memoryByte);
        PS[N] = getBit(A, 7);
        PS[Z] = (A == 0) ? true : false;
        break;
        
        case 0x48:              //PHA
        setCpuByte(SP + 0x100, A);
        SP--;
        cpuClock += 3;
        break;

        case 0x08:                 //PHP
        setCpuByte(SP + 0x100, (getPswByte(PS) | 0x10));
        SP--;
        cpuClock += 3;
        break;
        
        case 0x68:              //PLA
        SP++;
        cpuClock += 3;
        A = getCpuByte(SP + 0x100, true);
        cpuClock += 3;
        PS[N] = getBit(A, 7);
        PS[Z] = (A == 0) ? true : false;
        break;
        
        case 0x28:                 //PLP
        SP++;
        cpuClock += 3;
        getPswFromByte(PS, getCpuByte(SP + 0x100, true));
        cpuClock += 3;
        break;
        
        //RLA
        case 0x23: case 0x27: case 0x2F: case 0x33: case 0x37: case 0x3B: case 0x3F: {
            bool store;
            store = getBit(getCpuByte(address, true), 7);
            setCpuByte(address, (getCpuByte(address, true) << 1) & 0xFE);
            setCpuByte(address, getCpuByte(address, true) | PS[C]);
            PS[C] = store;
            A &= getCpuByte(address, true);
            PS[Z] = (A == 0) ? true : false;
            PS[N] = getBit(A, 7);
            break;
        }

        //ROL
        case 0x2A: case 0x26: case 0x36: case 0x2E: case 0x3E: {
            bool store;
            if (opcode == 0x2A) {
                store = getBit(A, 7);
                A = (A << 1) & 0xFE;
                A |= PS[C];
                PS[Z] = (A == 0) ? true : false;
                PS[N] = getBit(A, 7);
            } else {
                store = getBit(getCpuByte(address, true), 7);
                setCpuByte(address, (getCpuByte(address, true) << 1) & 0xFE);
                setCpuByte(address, getCpuByte(address, true) | PS[C]);
                PS[Z] = (getCpuByte(address, true) == 0) ? true : false;
                PS[N] = getBit(getCpuByte(address, true), 7);
            }
            PS[C] = store;
            break;
        }

        //ROR
        case 0x6A: case 0x66: case 0x76: case 0x6E: case 0x7E: {
            bool store;
            if (opcode == 0x6A) {
                store = getBit(A, 0);
                A = (A >> 1) & 0x7F;
                A |= (PS[C] ? 0x80 : 0x0);
                PS[Z] = (A == 0) ? true : false;
                PS[N] = getBit(A, 7);
            } else {
                store = getBit(getCpuByte(address, true), 0);
                setCpuByte(address, (getCpuByte(address, true) >> 1) & 0x7F);
                setCpuByte(address, getCpuByte(address, true) | (PS[C] ? 0x80 : 0x0));
                PS[Z] = (getCpuByte(address, true) == 0) ? true : false;
                PS[N] = getBit(getCpuByte(address, true), 7);
            }
            PS[C] = store;
            break;
        }

        //RRA
        case 0x63: case 0x67: case 0x6F: case 0x73: case 0x77: case 0x7B: case 0x7F: {
            bool store;
            store = getBit(getCpuByte(address, true), 0);
            setCpuByte(address, (getCpuByte(address, true) >> 1) & 0x7F);
            setCpuByte(address, getCpuByte(address, true) | (PS[C] ? 0x80 : 0x0));
            PS[C] = store;
            uint8_t memByte;
            memByte = getCpuByte(address, true);
            uint16_t total;
            total = A + memByte + PS[C];
            PS[V] = (((int8_t) A) + ((int8_t) memByte) + PS[C] < -128 
                || ((int8_t) A) + ((int8_t) memByte) + PS[C] > 127);
            PS[C] = (total > 255) ? true : false;
            A = total & 0xFF;
            PS[Z] = (A == 0) ? true : false;
            PS[N] = getBit(A, 7);
            break;
        }

        case 0x40: {                //RTI
            SP++;
            cpuClock += 3;

            uint8_t memByte;
            memByte = getCpuByte(SP + 0x100, true);
            getPswFromByte(PS, memByte);

            SP++;
            cpuClock += 3;
            uint16_t low = getCpuByte(SP + 0x100, true);

            SP++;
            cpuClock += 3;
            uint16_t high = getCpuByte(SP + 0x100, true) << 8;
            cpuClock += 3;

            PC = high | low;
            break;
        }

        case 0x60: {                //RTS
            SP++;
            cpuClock += 3;

            uint16_t low = getCpuByte(SP + 0x100, true);
            SP++;
            cpuClock += 3;

            uint16_t high = getCpuByte(SP + 0x100, true) << 8;
            cpuClock += 3;
            PC = (high | low);

            PC++;
            cpuClock += 3;
            break;
        }

        //SBC
        case 0xE9: case 0xE5: case 0xF5: case 0xED: case 0xFD: case 0xF9: case 0xE1: case 0xF1: case 0xEB: {
            int total;
            total = A - memoryByte - (!PS[C]);
            PS[V] = (((int8_t) A) - ((int8_t) memoryByte) - (!PS[C]) < -128 
                || ((int8_t) A) - ((int8_t) memoryByte) - (!PS[C]) > 127);
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
        setCpuByte(address, A & X);
        break;
        
        //*SLO
        case 0x03: case 0x07: case 0x0F: case 0x13: case 0x17: case 0x1B: case 0x1F: 
        PS[C] = getBit(getCpuByte(address, true), 7);
        setCpuByte(address, ((getCpuByte(address, true) << 1) & 0xFE));
        A |= getCpuByte(address, true);
        PS[N] = getBit(A, 7);
        PS[Z] = (A == 0) ? true : false;
        break;

        //SRE
        case 0x43: case 0x47: case 0x4F: case 0x53: case 0x57: case 0x5B: case 0x5F: 
        PS[C] = getBit(getCpuByte(address, true), 0);
        setCpuByte(address, (getCpuByte(address, true) >> 1) & 0x7F);
        A ^= getCpuByte(address, true);
        PS[N] = getBit(A, 7);
        PS[Z] = (A == 0) ? true : false;
        break;

        //STA
        case 0x85: case 0x95: case 0x8D: case 0x9D: case 0x99: case 0x81: case 0x91: 
        setCpuByte(address, A);
        break;
        
        //STX
        case 0x86: case 0x96: case 0x8E: 
        setCpuByte(address, X);
        break;

        //STY
        case 0x84: case 0x94: case 0x8C: 
        setCpuByte(address, Y);
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
        std:: cout << " Unrecognized opcode : " << std::hex << (int) opcode << std::endl;
        exit(EXIT_FAILURE);
        break;
    }

    return;
}

static inline uint8_t getPswByte(bool * PS) {
    uint8_t P;
    P = 0x20;
    if (PS[C]) P |= 0x1;
    if (PS[Z]) P |= 0x2;
    if (PS[I]) P |= 0x4;
    if (PS[D]) P |= 0x8;
    if (PS[V]) P |= 0x40;
    if (PS[N]) P |= 0x80;
    return P;
}

static inline void getPswFromByte(bool * PS, uint8_t byte) {
    PS[N] = getBit(byte, 7);
    PS[V] = getBit(byte, 6);
    PS[D] = getBit(byte, 3);
    PS[I] = getBit(byte, 2);
    PS[Z] = getBit(byte, 1);
    PS[C] = getBit(byte, 0);
    return;
}

//debugging
static void printByte(uint8_t byte) {
    std::cout << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int) byte;
    return;
}

//note:ugly, for debugging
static int debugPrintVal(enum AddressMode mode, int firstByte, int secondByte) {

    switch (mode) {
        case ABS:
        std::cout << '$' << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << secondByte << std::setfill('0') << std::setw(2) << firstByte;
        return 5;

        case ABSX:
        std::cout << '$' << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << secondByte << std::setfill('0') << std::setw(2) << firstByte << ",X";
        return 7;

        case ABSY:
        std::cout << '$' << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << secondByte << std::setfill('0') << std::setw(2) << firstByte << ",Y";
        return 7;

        case ACC:
        std::cout << 'A';
        return 1;

        case IMM:
        std::cout << "#$" << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << firstByte;
        return 4;

        case IMP:
        return 0;

        case IND:
        std::cout << "($" << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << secondByte << std::setfill('0') << std::setw(2) << firstByte << ')';
        return 7;

        case INDX:
        std::cout << "($" << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << firstByte << ",X)";
        return 7;

        case INDY:
        std::cout << "($" << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << firstByte << "),Y";
        return 7;

        case REL:
        return 0;

        case ZRP:
        std::cout << '$' << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << firstByte;
        return 3;

        case ZRPX:
        std::cout << '$' << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << firstByte << ",X";
        return 5;
        
        case ZRPY:
        std::cout << '$' << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << firstByte << ",Y";
        return 5;
        
        default:
        std::cerr << "Unrecognized address mode" << std::endl;
        return 0;
    }
}

//note:ugly, for debugging, matches nintendulator log
static void printDebugLine(uint16_t address, uint8_t opcode, uint8_t iByte2,
 uint8_t iByte3, enum AddressMode opAddressMode, uint16_t PC, uint8_t memByte, 
 uint8_t A, uint8_t X, uint8_t Y, uint8_t SP, bool * PS, int ppuClock) {

    std::cout << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << (int) PC << "  ";

    if (opAddressMode == IMP || opAddressMode == ACC) {
        printByte(opcode);
        std::cout << "       ";
    } else if (opAddressMode == ZRP || opAddressMode == ZRPX || opAddressMode == ZRPY
        || opAddressMode == REL || opAddressMode == IMM
        || opAddressMode == INDX || opAddressMode == INDY) {
        printByte(opcode);
        std::cout << ' ';
        printByte(iByte2);
        std::cout << "    ";
    } else if (opAddressMode == ABS || opAddressMode == ABSX
        || opAddressMode == ABSY || opAddressMode == IND) {
        printByte(opcode);
        std::cout << ' ';
        printByte(iByte2);
        std::cout << ' ';
        printByte(iByte3);
        std::cout << ' ';
    } else {
        std::cout << "         ";
    }

    if (strlen(opnames[opnameMap[opcode]]) == 3) {
        std::cout << ' ';
    }

    int whiteSpace;
    whiteSpace = 28;

    std::cout << opnames[opnameMap[opcode]] << ' ';

    

    if (opAddressMode == REL) {
        std::cout << '$';
        std::cout << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << (int) PC + (int8_t) iByte2 + 2;
        whiteSpace -= 5;
    } else {
        int addressLen;

        addressLen = debugPrintVal(addressModes[opcode], iByte2, iByte3);
        whiteSpace -= addressLen;
    }

    if (opnameMap[opcode] == 58 || opnameMap[opcode] == 59 || opnameMap[opcode] == 50 || 
        opnameMap[opcode] == 60 || opnameMap[opcode] == 7  || opnameMap[opcode] == 34 ||
        opnameMap[opcode] == 35 || opnameMap[opcode] == 33 || opnameMap[opcode] == 32 ||
        opnameMap[opcode] == 39 || opnameMap[opcode] == 2  || opnameMap[opcode] == 25 ||
        opnameMap[opcode] == 1  || opnameMap[opcode] == 18 || opnameMap[opcode] == 51 ||
        opnameMap[opcode] == 52 || opnameMap[opcode] == 19 || opnameMap[opcode] == 20 ||
        opnameMap[opcode] == 36 || opnameMap[opcode] == 3  || opnameMap[opcode] == 45 ||
        opnameMap[opcode] == 46 || opnameMap[opcode] == 26 || opnameMap[opcode] == 22 ||
        opnameMap[opcode] == 30 || opnameMap[opcode] == 38 || opnameMap[opcode] == 21 ||
        opnameMap[opcode] == 29 || opnameMap[opcode] == 56 || opnameMap[opcode] == 47 ||
        opnameMap[opcode] == 57 || opnameMap[opcode] == 44) {

        if ((opAddressMode == ZRP || opAddressMode == ABS) && (opnameMap[opcode] != 30)) {
            whiteSpace-=5;
            std::cout << " = " << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int) memByte;
        } else if (opAddressMode == INDX) {
            whiteSpace -= 17;

            std::cout << " @ " << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int) ((iByte2 + X) & 0xFF);
            std::cout << " = " << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << address;
            std::cout << " = " << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int) memByte;
        } else if (opAddressMode == INDY) {

            whiteSpace -= 19;
            std::cout << " = " << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << ((address - Y) & 0xFFFF);
            std::cout << " @ " << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << address;
            std::cout << " = " << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int) memByte;
        } else if (opAddressMode == IND) {

            whiteSpace-=7;
            std::cout << " = " << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << address;


        } else if (opAddressMode == ABSX || opAddressMode == ABSY) {
            whiteSpace -= 12;

            std::cout << " @ " << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << address;
            std::cout << " = " << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int) memByte;

        } else if (opAddressMode == ZRPX || opAddressMode == ZRPY) {

            whiteSpace -= 10;

            std::cout << " @ " << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int) address;
            std::cout << " = " << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int) memByte;


        }
    }

    for (int x = 0; x < whiteSpace; x++) {
        std::cout << ' ';
    }

    std::cout << "A:" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int) A << ' ';
    std::cout << "X:" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int) X << ' ';
    std::cout << "Y:" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int) Y << ' ';
    std::cout << "P:" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int) getPswByte(PS) << ' ';
    std::cout << "SP:" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int) SP << ' ';

    std::cout << "CYC:";
    
    int count = ppuClock % 341;
    int scanLines = (ppuClock % (341 * 262)) / 341; 

    if (count < 10) {
        std::cout << "  ";
    } else if (count < 100) {
        std::cout << " ";
    }
    std::cout << std::dec << count;

    
    std::cout << " SL:";
    std::cout << std::dec << scanLines;
    

    return;

}
