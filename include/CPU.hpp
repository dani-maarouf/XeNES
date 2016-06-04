#ifndef __NES_CPU__
#define __NES_CPU__

#include <cstdint>

#include "APU.hpp"

enum ProcessorStatusIndex {
    N,      //is number negative
    V,      //overflow
    D,      //BCD mode
    I,      //disable interrupts
    Z,      //zero flag
    C       //carry flag
};

enum AddressMode {
    ABS,
    ABSX,
    ABSY,
    ACC,
    IMM,
    IMP,
    IND,
    INDX,
    INDY,
    REL,
    ZRP,
    ZRPX,
    ZRPY,
    NONE
};

class CPU {

public:
    uint16_t PC;        //program counter
    uint8_t SP;         //stack pointer

    uint8_t A;          //accumulator
    uint8_t X;          //register X
    uint8_t Y;          //register Y

    bool PS[8];         //processor status word

    CPU();

    //uint8_t stack[0x100];
    uint8_t cpuMem[0x10000];

    APU nesAPU;

    uint16_t retrieveAddress(enum AddressMode);
    uint8_t getByte(uint16_t);
    bool setByte(uint16_t, uint8_t);
    bool executeNextOpcode(bool, bool);

};

#endif
/* DEFINED __NES_CPU__ */
