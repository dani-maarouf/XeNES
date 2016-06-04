#include <iostream>

#include "CPU.hpp"
                            //0,1,2,3,4,5,6,7,8,9,A,B,C,D,E,F
const int opcodeLens[0x20] = {2,2,0,2,2,2,2,2,1,2,1,2,3,3,3,3, //0 2 4 6 8 A C E
                              2,2,0,2,2,2,2,2,1,3,1,3,3,3,3,3};//1 3 5 7 9 B D F

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

const enum AddressMode addressModes[] = {
  //0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
    IMP, INDX,NONE,INDX,ZRP ,ZRP ,ZRP ,ZRP ,IMP, IMM, IMP, NONE,ABS, ABS ,ABS, ABS,   //0
    IMP, INDY,NONE,INDY,ZRPX,ZRPX,ZRPX,ZRPX,IMP, ABSY,IMP, ABSY,ABSX,ABSX,ABSX,ABSX,  //1
    ABS, INDX,NONE,INDX,ZRP, ZRP, ZRP, ZRP, IMP, IMM, ACC, NONE,ABS, ABS, ABS, ABS,   //2
    IMP, INDY,NONE,INDY,ZRPX,ZRPX,ZRPX,ZRPX,IMP, ABSY,IMP, ABSY,ABSX,ABSX,ABSX,ABSX,  //3
    IMP, INDX,NONE,INDX,ZRP, ZRP, ZRP, ZRP, IMP, IMM, ACC, NONE,ABS, ABS, ABS, ABS,   //4
    IMP, INDY,NONE,INDY,ZRPX,ZRPX,ZRPX,ZRPX,IMP, ABSY,IMP, ABSY,ABSX,ABSX,ABSX,ABSX,  //5
    IMP, INDX,NONE,INDX,ZRP, ZRP, ZRP, ZRP, IMP, IMM, ACC, NONE,IND, ABS, ABS, ABS,   //6
    IMP, INDY,NONE,INDY,ZRPX,ZRPX,ZRPX,ZRPX,IMP, ABSY,IMP, ABSY,ABSX,ABSX,ABSX,ABSX,  //7
    IMM, INDX,NONE,INDX,ZRP, ZRP, ZRP, ZRP, IMP, NONE,IMP, NONE,ABS, ABS, ABS, ABS,   //8
    IMP, INDY,NONE,INDY,ZRPX,ZRPX,ZRPY,ZRPY,IMP, ABSY,IMP, NONE,NONE,ABSX,NONE,NONE,  //9
    IMM, INDX,IMM, INDX,ZRP, ZRP, ZRP, ZRP, IMP, IMM, IMP, NONE,ABS, ABS, ABS, ABS,   //A
    IMP, INDY,NONE,INDY,ZRPX,ZRPX,ZRPY,ZRPY,IMP, ABSY,IMP, NONE,ABSX,ABSX,ABSY,ABSY,  //B
    IMM, INDX,NONE,INDX,ZRP, ZRP, ZRP, ZRP, IMP, IMM, IMP, NONE,ABS, ABS, ABS, ABS,   //C
    IMP, INDY,NONE,INDY,ZRPX,ZRPX,ZRPX,ZRPX,IMP, ABSY,IMP, ABSY,ABSX,ABSX,ABSX,ABSX,  //D
    IMM, INDX,NONE,INDX,ZRP, ZRP, ZRP, ZRP, IMP, IMM, IMP, IMM, ABS, ABS, ABS, ABS,   //E
    IMP, INDY,NONE,INDY,ZRPX,ZRPX,ZRPX,ZRPX,IMP, ABSY,IMP, ABSY,ABSX,ABSX,ABSX,ABSX}; //F

static bool getBit(uint8_t, int);
static uint8_t getPswByte(bool *);
static void getPswFromByte(bool * PS, uint8_t byte);

CPU::CPU() {

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

    return;
}

uint8_t CPU::getByte(uint16_t memAddress) {
    return cpuMem[memAddress];
}

bool CPU::setByte(uint16_t memAddress, uint8_t byte) {
    cpuMem[memAddress] = byte;
    return true;
}

uint16_t CPU::retrieveAddress(enum AddressMode mode) {

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
            high = getByte(firstByte + 1 + X & 0xFF);
            return ((high << 8) | low);
        }
        
        case INDY: 
        return ((((getByte(firstByte)) | (getByte(firstByte + 1 & 0xFF)) << 8) + Y) & 0xFFFF);
        
        default:
        return 0;
    }
}

bool CPU::executeNextOpcode(bool debug, bool verbose) {

    uint8_t opcode, iByte2, iByte3;
    opcode = getByte(PC);
    iByte2 = getByte(PC + 1);
    iByte3 = getByte(PC + 2);

    if (verbose) {
        std::cout << std::hex << std::uppercase << "PC:" << (int) PC << " SP:" << (int) SP << " X:" << (int) X;
        std::cout << std::hex << std::uppercase <<" Y:" << (int) Y << " A:" << (int) A << " N:" << PS[N] << " V:" << PS[V];
        std::cout << std::hex << std::uppercase <<" D:" << PS[D] << " I:" << PS[I] << " Z:" << PS[Z] << " C:" << PS[C] << " P:" << (int) getPswByte(PS) << "\t\t\t"; 
    }

    uint16_t address;
    address = retrieveAddress(addressModes[opcode]);

    PC += opcodeLens[opcode % 0x20];
    if (opcode == 0xA2) PC += 2;        //irregular opcode

    if (debug) std::cout << opnames[opnameMap[opcode]] << ' ';

    uint8_t memoryByte;
    if (addressModes[opcode] == IMM) {
        memoryByte = iByte2;
    } else {
        memoryByte = getByte(address);
    }
    

    switch (opcode) {

        case 0x69:      //ADC
        case 0x65:
        case 0x75:
        case 0x6D:
        case 0x7D:
        case 0x79:
        case 0x61:
        case 0x71: {
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

        case 0x29:      //AND
        case 0x25:
        case 0x35:
        case 0x2D:
        case 0x3D:
        case 0x39:
        case 0x21:
        case 0x31: 
        A = A & memoryByte;
        PS[N] = getBit(A, 7);
        PS[Z] = (A == 0) ? true : false;
        break;

        case 0x0A:      //ASL
        case 0x06:
        case 0x16:
        case 0x0E:
        case 0x1E: {
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
        if (PS[C] == 0) PC += (int8_t) iByte2;
        break;
        
        case 0xB0:            //BCS
        if (PS[C]) PC += (int8_t) iByte2;
        break;
        
        case 0xF0:             //BEQ
        if (PS[Z]) PC += (int8_t) iByte2;
        break;
        
        case 0x24:          //BIT
        case 0x2C: {
            uint8_t num;
            num = A & memoryByte;
            PS[N] = getBit(memoryByte, 7);
            PS[V] = getBit(memoryByte, 6);
            PS[Z] = (num == 0) ? true : false;
            break;
        }

        case 0x30:             //BMI
        if (PS[N]) PC += (int8_t) iByte2;
        break;
        
        case 0xD0:             //BNE
        if (PS[Z] == 0) PC += (int8_t) iByte2;
        break;
        
        case 0x10:             //BPL
        if (PS[N] == 0) PC += (int8_t) iByte2;
        break;
        
        case 0x00: {            //BRK

            uint8_t low, high;
            low = PC & 0xFF;
            high = (PC & 0xFF00) >> 8;

            setByte(0x100 + SP, high);
            SP--;
            setByte(0x100 + SP, low);
            SP--;

            uint8_t memByte = getPswByte(PS);

            setByte(0x100 + SP, memByte | 0x10);
            SP--;

            low = getByte(0xFFFE);
            high = getByte(0xFFFF);

            PC = (high << 8) | low;

            break;
        }

        case 0x50:          //BVC
        if (PS[V] == 0) PC += (int8_t) iByte2;
        break;
        
        case 0x70:          //BVS
        if (PS[V]) PC += (int8_t) iByte2;
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

        case 0xC9:          //CMP
        case 0xC5:
        case 0xD5:
        case 0xCD:
        case 0xDD:
        case 0xD9:
        case 0xC1:
        case 0xD1: {
            int num;
            num = A - memoryByte;
            PS[N] = getBit(num, 7);

            PS[C] = (A >= memoryByte) ? true : false;
            PS[Z] = (num == 0) ? true : false;

            break;
        }

        case 0xE0:          //CPX
        case 0xE4:
        case 0xEC: {
            int total;
            total = X - memoryByte;
            PS[N] = (total & 0x80) ? true : false;
            PS[C] = (X >= memoryByte) ? true : false;
            PS[Z] = (total == 0) ? 1 : 0;
            break;
        }

        case 0xC0:          //CPY
        case 0xC4:
        case 0xCC: {
            int total;
            total = Y - memoryByte;
            PS[N] = (total & 0x80) ? true : false;
            PS[C] = (Y >= memoryByte) ? true : false;
            PS[Z] = (total == 0) ? 1 : 0;
            break;
        }


        case 0xC3:          //DCP
        case 0xD3:
        case 0xC7:
        case 0xD7:
        case 0xCF:
        case 0xDF:
        case 0xDB: {

            uint8_t memByte;

            if (opcode == 0xC9) {
                memByte = (iByte2 - 1) & 0xFF;
            } else {
                setByte(address, (getByte(address) - 1) & 0xFF);
                memByte = getByte(address);
            }

            int num;
            num = A - memByte;
            PS[N] = getBit(num, 7);

            PS[C] = (A >= memByte) ? true : false;
            PS[Z] = (num == 0) ? true : false;

            break;
        }

        case 0xC6:          //DEC
        case 0xD6:
        case 0xCE:
        case 0xDE: 
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
        
        case 0x49:          //EOR
        case 0x45:
        case 0x55:
        case 0x4D:
        case 0x5D:
        case 0x59:
        case 0x41:
        case 0x51: 
        A = A ^ memoryByte;
        PS[N] = getBit(A, 7);
        PS[Z] = (A == 0) ? true : false;
        break;
        
        case 0xE6:              //INC
        case 0xF6:
        case 0xEE:
        case 0xFE: 
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

        case 0xE3:              //ISB
        case 0xE7:
        case 0xF7:
        case 0xFB:
        case 0xEF:
        case 0xFF:
        case 0xF3: {        

            uint8_t memByte;
            if (opcode == 0xE9 || opcode == 0xEB) {
                memByte = iByte2;
            } else {
                setByte(address, (getByte(address) + 1) & 0xFF);
                memByte = getByte(address);
            }

            int total;

            total = A - memByte - (!PS[C]);

            int8_t num1, num2;
            num1 = (int8_t) A;
            num2 = (int8_t) memByte;

            PS[V] = (num1 - num2 - (!PS[C]) < -128 || num1 - num2 - (!PS[C]) > 127);

            A = total & 0xFF;

            PS[C] = (total >= 0) ? true : false;
            PS[N] = getBit(total, 7);
            PS[Z] = (A == 0) ? 1 : 0;

            break;
        }

        case 0x4C:              //JMP
        case 0x6C: 
        PC = address;
        break;

        case 0x20: {            //JSR

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

        case 0xA3:          //LAX
        case 0xA7: 
        case 0xAF: 
        case 0xB3: 
        case 0xB7: 
        case 0xBF:      
        A = memoryByte;
        X = memoryByte;
        PS[N] = getBit(A, 7);
        PS[Z] = (A == 0) ? true : false;
        break;
        
        case 0xA9:          //LDA
        case 0xA5:
        case 0xB5:
        case 0xAD:
        case 0xBD:
        case 0xB9:
        case 0xA1:
        case 0xB1: 
        A = memoryByte;
        PS[N] = getBit(A, 7);
        PS[Z] = (A == 0) ? true : false;
        break;
        
        case 0xA2:              //LDX
        case 0xA6:
        case 0xB6:
        case 0xAE:
        case 0xBE: 
        X = memoryByte;
        PS[N] = getBit(X, 7);
        PS[Z] = (X == 0) ? true : false;
        break;
        
        case 0xA0:              //LDY
        case 0xA4:
        case 0xB4:
        case 0xAC:
        case 0xBC: 
        Y = memoryByte;
        PS[N] = getBit(Y, 7);
        PS[Z] = (Y == 0) ? true : false;
        break;

        case 0x4A:              //LSR
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
                PS[N] = 0;
                PS[C] = getBit(getByte(address), 0);
                setByte(address, (getByte(address) >> 1) & 0x7F);
                PS[Z] = (getByte(address) == 0) ? true : false;
            }

            break;
        }

        case 0x04:              //NOP
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
        case 0xEA:                 
        break;
        
        case 0x09:              //ORA
        case 0x05:
        case 0x15:
        case 0x0D:
        case 0x1D:
        case 0x19:
        case 0x01:
        case 0x11: 
        A = (A | memoryByte);
        PS[N] = getBit(A, 7);
        PS[Z] = (A == 0) ? true : false;
        break;
        
        case 0x48:              //PHA
        setByte(SP + 0x100, A);
        SP--;
        break;

        case 0x08:                 //PHP
        setByte(SP + 0x100, getPswByte(PS));
        SP--;
        break;
        
        case 0x68:              //PLA
        SP++;
        A = getByte(SP + 0x100);
        PS[N] = getBit(A, 7);
        PS[Z] = (A == 0) ? true : false;
        break;
        
        case 0x28:                 //PLP
        SP++;
        getPswFromByte(PS, getByte(SP + 0x100));
        break;
        
        case 0x23:              //RLA
        case 0x27:
        case 0x2F:
        case 0x33:
        case 0x37:
        case 0x3B:
        case 0x3F: {

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

        case 0x2A:              //ROL
        case 0x26:
        case 0x36:
        case 0x2E:
        case 0x3E: {

            bool store;

            if (opcode == 0x2A) {

                store = getBit(A, 7);

                A = (A << 1) & 0xFE;
                A |= PS[C];
                PS[C] = store;
                PS[Z] = (A == 0) ? true : false;
                PS[N] = getBit(A, 7);

                if (debug) std::cout << "A" << std::endl;

            } else {

                store = getBit(getByte(address), 7);

                setByte(address, (getByte(address) << 1) & 0xFE);
                setByte(address, getByte(address) | PS[C]);
                PS[C] = store;
                PS[Z] = (getByte(address) == 0) ? true : false;
                PS[N] = getBit(getByte(address), 7);
            }

            break;

        }

        case 0x6A:              //ROR
        case 0x66:
        case 0x76:
        case 0x6E:
        case 0x7E: {

            bool store;

            if (opcode == 0x6A) {
                
                store = getBit(A, 0);

                A = (A >> 1) & 0x7F;
                A |= (PS[C] ? 0x80 : 0x0);
                PS[C] = store;
                PS[Z] = (A == 0) ? true : false;
                PS[N] = getBit(A, 7);

            } else {

                store = getBit(getByte(address), 0);

                setByte(address, (getByte(address) >> 1) & 0x7F);
                setByte(address, getByte(address) | (PS[C] ? 0x80 : 0x0));
                PS[C] = store;
                PS[Z] = (getByte(address) == 0) ? true : false;
                PS[N] = getBit(getByte(address), 7);
            }

            break;

        }

        case 0x63:              //RRA
        case 0x67:
        case 0x6F:
        case 0x73:
        case 0x77:
        case 0x7B:
        case 0x7F: {

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
            
            uint8_t memByte;
            memByte = getByte(SP + 0x100);

            getPswFromByte(PS, memByte);

            SP++;
            uint16_t low = getByte(SP + 0x100);
            SP++;
            uint16_t high = getByte(SP + 0x100) << 8;
            PC = high | low;
            break;
        }

        case 0x60: {                //RTS
            SP++;
            uint16_t low = getByte(SP + 0x100);
            SP++;
            uint16_t high = getByte(SP + 0x100) << 8;
            PC = (high | low) + 1;
            break;
        }

        case 0xE9:              //SBC
        case 0xE5:
        case 0xF5:
        case 0xED:
        case 0xFD:
        case 0xF9:
        case 0xE1:
        case 0xF1: 
        case 0xEB: {

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
        
        case 0x83:                  //SAX
        case 0x87:
        case 0x97:
        case 0x8F:          
        setByte(address, A & X);
        break;
        
        case 0x03:      //*SLO
        case 0x07:
        case 0x0F:
        case 0x13:
        case 0x17:
        case 0x1B:
        case 0x1F: 
        PS[C] = getBit(getByte(address), 7);
        setByte(address, ((getByte(address) << 1) & 0xFE));
        A |= getByte(address);
        PS[N] = getBit(A, 7);
        PS[Z] = (A == 0) ? true : false;
        break;

        case 0x43:              //SRE
        case 0x47:
        case 0x4F:
        case 0x53:
        case 0x57:
        case 0x5B:
        case 0x5F: 
        PS[N] = 0;
        PS[C] = getBit(getByte(address), 0);
        setByte(address, (getByte(address) >> 1) & 0x7F);
        A ^= getByte(address);
        PS[N] = getBit(A, 7);
        PS[Z] = (A == 0) ? true : false;
        break;

        case 0x85:              //STA
        case 0x95:
        case 0x8D:
        case 0x9D:
        case 0x99:
        case 0x81:
        case 0x91: 
        setByte(address, A);
        break;
        
        case 0x86:              //STX
        case 0x96:
        case 0x8E: 
        setByte(address, X);
        break;

        case 0x84:              //STY
        case 0x94:
        case 0x8C: 
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
        return false;
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
