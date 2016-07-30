#ifndef __NES_NES__
#define __NES_NES__

#include <cstdint>

#include "CPU.hpp"
#include "PPU.hpp"

class NES {

    /* allows child class CPU to access parent NES member functions getCpuByte(), setCpuByte()
       and retrieveCpuAddress(), ugly but works */
    friend class CPU;
    friend class PPU;

private:

    int nesMapper;

    /* controller 1 */
    bool readController;
    uint8_t storedControllerByte;
    int currentControllerBit;

    uint8_t ioRegisters[0x20];          //joystick and apu registers

    CPU nesCPU;                         //CPU
    
    uint8_t getCpuByte(uint16_t);       //get byte from CPU address space
    void setCpuByte(uint16_t, uint8_t); //set byte in CPU address space
    uint16_t retrieveCpuAddress(enum AddressMode, bool *, uint8_t, uint8_t);  //get address basedon address mode

public:

    PPU nesPPU;                         //PPU
    uint8_t controllerByte;

    NES();
    bool openCartridge(const char *);   //load ROM
    void closeCartridge();              //free memory associated with cartridge

    bool drawFlag();                    //draw frame?
    void tickPPU(int);                      //1 ppu tick

    int executeOpcode(bool);            //CPU execute opcode interface
};

#endif
/* DEFINED __NES_NES__ */
