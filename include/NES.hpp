#ifndef __NES_NES__
#define __NES_NES__

#include "CPU.hpp"
#include "PPU.hpp"
#include "APU.hpp"

class NES {

public:

    CPU * nesCPU;
    PPU * nesPPU;
    APU * nesAPU;

    bool openROM(const char *);
    NES();
    ~NES();

};

#endif
/* DEFINED __NES_NES__ */
