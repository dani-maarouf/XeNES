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

    int nametableOffset;
    int vramInc;
    int spriteTableOffset;
    int backgroundTableOffset;
    bool extendedSprites;
    bool ppuMaster;
    bool generateNMI;


    uint8_t palette[0x20];      
    uint8_t VRAM[0x800];                    //2kB PPU internal RAM
    uint8_t OAM[0x100];                     //256 byte PPU OAM
    uint8_t secondaryOAM[32];
    uint8_t secondaryOamAddress;        
    
    int ppuCycle;                           //0-341 per scanline
    bool evenFrame;                         //tracks even and odd frames

    uint16_t vramAddress;                   //current VRAM address


    uint8_t getPpuByte(uint16_t);           //get byte from PPU address space
    bool setPpuByte(uint16_t, uint8_t);     //set byte in PPU address space
    void drawSprites();                     //draw sprites to pixel display

public:

    uint8_t oamAddress;                     //current OAM address

    PPU();

    bool getVramAddress;        //CPU has written half of address to 0x2006 in CPU address space
    bool readLower;             //lower part of write to 2006 is occuring

    bool readToRAM;             //CPU has written byte to 0x2007 in CPU address space
    bool readToOAM;             //CPU has written byte to 0x4014 in CPU address space

    bool setCtrl;

    bool usesRAM;               //true if CHR_RAM is used rather than CHR_ROM



    uint8_t ppuRegisters[0x8];  //PPU registers
    enum Mirroring mirroring;   //nametable arrangement
    uint8_t * CHR_ROM;          //cartridge video ROM
    int scanline;               //current scanline

    bool draw;                  //draw frame?
    uint32_t pixels[256 * 240]; //pixel display

    void tick(NES *);            //one PPU tick is executed
    void freePointers();        //free memory
};

#endif
/* DEFINED __NES_PPU__ */
