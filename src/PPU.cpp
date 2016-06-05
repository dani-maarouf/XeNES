#include "PPU.hpp"

PPU::PPU(uint8_t * chrROM, uint8_t * regs) {

    for (int x = 0; x < 0x4000; x++) {
        memory[x] = 0x0;
    }

    registers = regs;
    CHR_ROM = chrROM;

    return;
}
