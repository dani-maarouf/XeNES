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

class CPU {

private:
    
    //controller 1
    bool readController;
    u8 storedControllerByte;
    int currentControllerBit;

    //registers
    u8 SP;         //stack pointer
    u8 A;          //accumulator
    bool PS[8];         //processor status word
    u8 X;          //register X
    u8 Y;          //register Y

    //hardware
    u8 RAM[0x800];         //2kB CPU RAM
    u8 cpuMem[0x2000];    //fallback memory addresses for (0x4020 - 0x6000)

    bool returnControllerBit();
    void setCpuByte(u16, u8); //set byte in CPU address space
    u16 retrieveCpuAddress(enum AddressMode, bool *, u8, u8);  //get address basedon address mode

public:

    PPU nesPPU;
    APU nesAPU;
    
    uintmax_t cpuClock;          //time in ppu ticks

    //info
    int cpuMapper;
    int numRomBanks;            //number of rom banks
    int numRamBanks;            //number of RAM banks

    //registers
    bool NMI;                   //non maskable interupt
    bool IRQ;
    u16 PC;                //program counter
    u8 controllerByte;     //controller 1

    //hardware
    u8 * PRG_ROM;          //cartridge program ROM
    u8 * PRG_RAM;          //cartridge program RAM

    CPU();                  //stage 1 initialize, stage 2 when ROM loading occurs
    void freePointers();    //deinitialize
    void executeNextOpcode(bool);
    u8 getCpuByte(u16, bool);       //get byte from CPU address space

};
