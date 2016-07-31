#ifndef __NES_NES__
#define __NES_NES__

#include <cstdint>

#include "CPU.hpp"

class NES {

public:

    CPU nesCPU;                         //CPU

    NES();
    bool openCartridge(const char *);   //load ROM
    void closeCartridge();              //free memory associated with cartridge
};

#endif
/* DEFINED __NES_NES__ */
