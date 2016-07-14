#ifndef __NES_PPU__
#define __NES_PPU__

#include <cstdint>

const int NES_SCREEN_WIDTH = 256;
const int NES_SCREEN_HEIGHT = 240;

/* nametable mirroring configuration */
enum Mirroring {
    VERTICAL,
    HORIZONTAL,
    FOUR_SCREEN,
};


class NES;

class PPU {

private:

    //render
    int paletteIndex;
    uint8_t spriteLayer1;
    uint8_t spriteLayer2;
    int spriteStart;

    //temp
    bool spriteZeroFlag;

    //registers
    uint16_t vramAddress;                   //current VRAM address
    uint8_t oamAddress;                     //current OAM address

    uint8_t palette[0x20];      
    uint8_t VRAM[0x1000];                   //4kB PPU internal RAM
    
    uint8_t secondaryOAM[8];
    uint8_t secondaryOamAddress;        
    
    int ppuCycle;                           //0-341 per scanline
    bool evenFrame;                         //tracks even and odd frames

    uint8_t OAM[0x100];                     //256 byte PPU OAM

    void setPpuByte(uint16_t, uint8_t);     //set byte in PPU address space

public:

    //0-7 = $2000 - $2007, 8 = $4014
    bool registerWriteFlags[9];
    bool registerReadFlags[8];

    //temp
    uint8_t readBuffer;
    int scanline;               //current scanline
    bool draw;                  //draw frame?

    //registers
    uint16_t m_v;       //current vram address
    uint16_t m_t;       //temporary vram address
    uint8_t m_x;        //fine x scroll
    bool addressLatch;                      //this is the w register

    //settings
    bool usesRAM;               //true if CHR_RAM is used rather than CHR_ROM
    enum Mirroring mirroring;   //nametable arrangement

    //hardware
    uint8_t ppuRegisters[0x8];              //PPU registers
    uint8_t * CHR_ROM;                      //cartridge video ROM (pattern tables)

    uint32_t pixels[256 * 240]; //pixel buffer

    PPU();

    void tick(NES *, int);            //one PPU tick is executed
    void freePointers();        //free memory

    uint8_t getPpuByte(uint16_t);           //get byte from PPU address space

};

#endif
/* DEFINED __NES_PPU__ */
