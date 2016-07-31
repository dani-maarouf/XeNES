#ifndef __NES_NES__
#define __NES_NES__

#include <cstdint>

#include "CPU.hpp"
#include "PPU.hpp"

class NES {

    /* allows child class CPU to access parent NES member functions getCpuByte(), setCpuByte()
       and retrieveCpuAddress(), ugly but works */
    //friend class CPU;
    //friend class PPU;

public:


    CPU nesCPU;                         //CPU


    NES();
    bool openCartridge(const char *);   //load ROM
    void closeCartridge();              //free memory associated with cartridge

    bool drawFlag();                    //draw frame?
    void tickPPU(int);                  //1 ppu tick

};

#endif
/* DEFINED __NES_NES__ */
