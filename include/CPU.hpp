#ifndef __NES_CPU__
#define __NES_CPU__

#include <cstdint>  //TODO: make this multiplatorm

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

class NES;      //forward decleration of outer NES class to allow pointer to be passed to executeNextOpcode()

class CPU {

private:
    
    uint8_t SP;         //stack pointer
    uint8_t A;          //accumulator
    bool PS[8];         //processor status word
    int cpuCycle;       //current cpu cycle in PPU ticks

public:

    bool NMI;           //non maskable interupt occuring?
    uint16_t PC;        //program counter
    uint8_t X;          //register X
    uint8_t Y;          //register Y

    
    uint8_t RAM[0x800];         //2kB CPU RAM
    uint8_t * PRG_ROM;          //cartridge program ROM
    uint8_t * PRG_RAM;          //cartridge program RAM
    int numRomBanks;            //number of rom banks
    int numRamBanks;            //number of RAM banks
    uint8_t cpuMem[0x10000];    //fallback memory addresses for testing
    
    /*
    executeNextOpcode:
    execute instruction located at PC in CPU address space
    */
    int executeNextOpcode(NES *, bool);

    CPU();                  //stage 1 initialize, stage 2 when ROM loading occurs
    void freePointers();    //deinitialize
    

};

#endif
/* DEFINED __NES_CPU__ */
