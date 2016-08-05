#ifndef __NES_CPU__
#define __NES_CPU__

#include <cstdint>
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
    uint8_t storedControllerByte;
    int currentControllerBit;

    //registers
    uint8_t SP;         //stack pointer
    uint8_t A;          //accumulator
    bool PS[8];         //processor status word
    uint8_t X;          //register X
    uint8_t Y;          //register Y

    //hardware
    uint8_t RAM[0x800];         //2kB CPU RAM
    uint8_t cpuMem[0x2000];    //fallback memory addresses for (0x4020 - 0x6000)

    bool returnControllerBit();
    void setCpuByte(uint16_t, uint8_t); //set byte in CPU address space
    uint16_t retrieveCpuAddress(enum AddressMode, bool *, uint8_t, uint8_t);  //get address basedon address mode

public:

    PPU nesPPU;
    APU nesAPU;
    
    uint64_t cpuClock;          //time in ppu ticks

    //info
    int cpuMapper;
    int numRomBanks;            //number of rom banks
    int numRamBanks;            //number of RAM banks

    //registers
    bool NMI;                   //non maskable interupt
    uint16_t PC;                //program counter
    uint8_t controllerByte;     //controller 1

    //hardware
    uint8_t * PRG_ROM;          //cartridge program ROM
    uint8_t * PRG_RAM;          //cartridge program RAM

    CPU();                  //stage 1 initialize, stage 2 when ROM loading occurs
    void freePointers();    //deinitialize
    void executeNextOpcode(bool);
    uint8_t getCpuByte(uint16_t, bool);       //get byte from CPU address space

};

#endif
/* DEFINED __NES_CPU__ */
