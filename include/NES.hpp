#pragma once
#include "CPU.hpp"

class NES {

public:

    CPU nesCPU;                         //CPU

    NES();
    bool openCartridge(const char *);   //load ROM
    void closeCartridge();              //free memory associated with cartridge
};
