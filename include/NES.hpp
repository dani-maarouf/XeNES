#pragma once
#include "CPU.hpp"

class NES {

public:

    CPU m_nesCPU;                         //CPU

    NES();
    bool open_cartridge(const char *);   //load ROM
    void close_cartridge();              //free memory associated with cartridge
};
