#ifndef __NES_PPU__
#define __NES_PPU__

#include <cstdint>

enum Registers {
    PPUCTRL     = 0,
    PPUMASK     = 1,
    PPUSTATUS   = 2,
    OAMADDR     = 3,
    OAMDATA     = 4,
    PPUSCROLL   = 5,
    PPUADDR     = 6,
    PPUDATA     = 7,
    OAMDMA      = 8
};

class PPU {

public:

    uint8_t * registers;    //owned by PPU
    uint8_t * CHR_ROM;      //owned by PPU

    uint8_t memory[0x4000];

    PPU(uint8_t *, uint8_t *);

};

#endif
/* DEFINED __NES_PPU__ */
