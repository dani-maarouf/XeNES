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
    u16 retrieve_cpu_address(enum AddressMode, bool *, u8, u8);  //get address basedon address mode

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
    void execute_next_opcode(bool);
    u8 get_cpu_byte(u16, bool);       //get byte from CPU address space

};
