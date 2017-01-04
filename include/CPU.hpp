#pragma once
#include "PPU.hpp"
#include "APU.hpp"

enum ProcessorStatusIndex {
    C = 0,       //carry flag
    Z = 1,      //zero flag
    I = 2,      //disable interrupts
    D = 3,      //BCD mode
    V = 6,      //overflow
    N = 7,      //is number negative
};

enum AddressMode {
    ABS  = 0,    //absolute
    ABSX = 1,   //absolute indexed x
    ABSY = 2,   //absolute indexed y
    ACC  = 3,    //accumulator
    IMM  = 4,    //immediate
    IMP  = 5,    //implied
    IND  = 6,    //indirect
    INDX = 7,   //indexed indirect
    INDY = 8,   //indirect indexed
    REL  = 9,    //relative
    ZRP  =10,    //zero-page
    ZRPX =11,   //zero-page indexed x
    ZRPY =12,   //zero-page indexed y
    NONE,
};

u8 get_psw_byte(bool *);
void get_psw_from_byte(bool * m_PS, u8 byte);

//memory addressing mode for each opcode
static const enum AddressMode addressModes[] = {
  //0    1    2    3    4    5    6    7    8    9    m_A    B    C    D    E    F
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
   //0  1  2  3  4  5  6  7  8  9  m_A  B  C  D  E  F
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

//table of opcode lengths for advancing program counter
                                   //0,1,2,3,4,5,6,7,8,9,m_A,B,C,D,E,F
static const int opcodeLens[0x20] = {2,2,0,2,2,2,2,2,1,2,1,2,3,3,3,3,  //0 2 4 6 8 A C E
                                     2,2,0,2,2,2,2,2,1,3,1,3,3,3,3,3}; //1 3 5 7 9 B D F

class CPU {

friend class Debugger;

private:
    
    //controller 1
    bool m_readController;
    u8 m_storedControllerByte;
    int m_currentControllerBit;

    //registers
    u8 m_SP;         //stack pointer
    u8 m_A;          //accumulator
    bool m_PS[8];         //processor status word
    u8 m_X;          //register X
    u8 m_Y;          //register Y

    //hardware
    u8 m_RAM[0x800];         //2kB CPU RAM
    u8 m_cpuMem[0x2000];    //fallback memory addresses for (0x4020 - 0x6000)

    bool return_controller_bit();
    void set_cpu_byte(u16, u8); //set byte in CPU address space
    

public:

    PPU m_nesPPU;
    APU m_nesAPU;
    
    uintmax_t m_cpuClock;          //time in ppu ticks

    //info
    int m_cpuMapper;
    int m_numRomBanks;            //number of rom banks
    int m_numRamBanks;            //number of RAM banks

    //registers
    bool m_NMI;                   //non maskable interupt
    bool m_IRQ;
    u16 m_PC;                //program counter
    u8 m_controllerByte;     //controller 1

    //hardware
    u8 * m_PRG_ROM;          //cartridge program ROM
    u8 * m_PRG_RAM;          //cartridge program RAM

    CPU();                  //stage 1 initialize, stage 2 when ROM loading occurs
    void free_pointers();    //deinitialize
    void execute_next_opcode();
    u8 get_cpu_byte(u16, bool);       //get byte from CPU address space
    u16 retrieve_cpu_address(enum AddressMode, bool *, bool *, u8, u8, bool);  //get address based on address mode

};
