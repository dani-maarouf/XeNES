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

    bool suppressVBL;

    //temp
    uint8_t m_SpriteOld1;
    uint8_t m_SpriteOld2;
    uint8_t m_PaletteOld;
    uint8_t m_SpriteNew1;
    uint8_t m_SpriteNew2;
    uint8_t m_PaletteNew;

    //info
    int ppuCycle;                           //0-341 per scanline
    bool evenFrame;                         //tracks even and odd frames

    //registers
    uint8_t secondaryOamAddress;        
    bool spriteZeroFlag;                    //set PPUSTATUS at end of line

    //hardware
    uint8_t secondaryOAM[8];                //OAM for sprites on line
    uint8_t palette[0x20];
    uint8_t VRAM[0x1000];                   //4kB PPU internal VRAM

    inline void loadNewTile();

    void setPpuByte(uint16_t, uint8_t);     //set byte in PPU address space
    void ppuFlagUpdate(NES *);
    void drawPixel(int, int);
    void updateSecondaryOAM();
    void incrementCycle();

public:

    uint64_t ppuClock;

    int numRomBanks;

    int ppuMapper;

    //temp
    uint16_t vramAddress;

    //0-7 = $2000 - $2007, 8 = $4014
    bool registerWriteFlags[9];
    bool registerReadFlags[8];
    bool flagSet;

    //registers
    uint16_t m_v;           //current vram address
    uint16_t m_t;           //temporary vram address
    uint8_t m_x;            //fine x scroll
    bool addressLatch;      //w register
    uint8_t readBuffer;
    uint8_t oamAddress;                     //current OAM address


    //info
    int scanline;               //current scanline
    bool draw;                  //draw frame?

    //configuration
    bool usesRAM;               //true if CHR_RAM is used rather than CHR_ROM
    enum Mirroring mirroring;   //nametable arrangement

    //hardware
    uint8_t OAM[0x100];                     //256 byte PPU OAM
    uint8_t ppuRegisters[0x8];              //PPU registers
    uint8_t * CHR_ROM;                      //cartridge video ROM (pattern tables)
    uint8_t pixels[256 * 240];

    PPU();
    void tick(NES *, int);            //one PPU tick is executed
    void freePointers();              //free memory
    uint8_t getPpuByte(uint16_t);     //get byte from PPU address space

};

#endif
/* DEFINED __NES_PPU__ */
