#ifndef __NES_NES__
#define __NES_NES__

#include <cstdint>

#include "CPU.hpp"
#include "PPU.hpp"

class NES {

    /* allows child class CPU to access parent NES member functions getCpuByte(), setCpuByte()
       and retrieveCpuAddress() */
    friend class CPU;

private:

    PPU nesPPU;
    CPU nesCPU;
    uint8_t ioRegisters[0x20];

    uint8_t getCpuByte(uint16_t);       //get byte from CPU address space
    bool setCpuByte(uint16_t, uint8_t); //set byte in CPU address space
    uint16_t retrieveCpuAddress(enum AddressMode, bool *);  //get address basedon address mode

public:

    NES();
    bool openCartridge(const char *);
    void closeCartridge();

    uint32_t * getDisplayPixels();
    bool drawFlag();
    int tickPPU();

    int executeOpcode(bool);
};

#endif
/* DEFINED __NES_NES__ */
