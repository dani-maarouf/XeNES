#ifndef __NES_PPU__
#define __NES_PPU__

#include <cstdint>

const int NES_SCREEN_WIDTH = 256;
const int NES_SCREEN_HEIGHT = 240;

enum Mirroring {
    VERTICAL, HORIZONTAL, FOUR_SCREEN,
};

class PPU {

private:

    uint8_t palette[0x20];
    uint8_t ppuRAM[0x800];      //2kB PPU RAM
    uint8_t ppuOAM[0x100];      //256 byte PPU OAM
    
    int ppuCycle;               //0-341
    bool evenFrame;

    uint16_t ppuReadAddress;

    uint8_t getPpuByte(uint16_t);
    bool setPpuByte(uint16_t, uint8_t);
    void printSprites();
    int getPpuCycle();
    void drawSprites();

public:

    PPU();

    bool ppuGetAddr;
    bool readLower;
    bool ppuReadByte;

    bool usesRAM;               //true if CHR_RAM is used rather than CHR_ROM

    uint8_t ppuRegisters[0x8];  //PPU registers
    enum Mirroring mirroring;   //nametable arrangement
    uint8_t * CHR_ROM;          //cartridge video ROM
    int scanline;

    bool draw;
    uint32_t * pixels;

    int tick(bool *);
    void freePointers();
};

#endif
/* DEFINED __NES_PPU__ */