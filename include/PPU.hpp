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
    uint8_t attributeByte;
    int paletteIndex;
    uint8_t spriteLayer1;
    uint8_t spriteLayer2;
    int spriteStart;
    int tileX;
    int tileY;
    int nametableIndex;
    int tableOverflow;
    int internalAttributeIndex;
    int attributeTableIndex;
    int nametableOffset;
    int spriteTableOffset;
    int backgroundTableOffset;
    bool extendedSprites;

    //temp
    bool spriteZeroOnScanline;
    uint8_t xScrolling;
    uint8_t yScrolling;

    bool ppuMaster;
    bool generateNMI;


    uint8_t palette[0x20];      
    uint8_t VRAM[0x1000];                   //4kB PPU internal RAM
    
    uint8_t secondaryOAM[8];
    uint8_t secondaryOamAddress;        
    
    int ppuCycle;                           //0-341 per scanline
    bool evenFrame;                         //tracks even and odd frames


    void setPpuByte(uint16_t, uint8_t);     //set byte in PPU address space

public:

    //temp
    int vramInc;
    uint8_t readBuffer;
    int scanline;               //current scanline
    bool draw;                  //draw frame?
    uint16_t seperateVram;      //temp

    //flags
    bool setCtrl;
    bool getVramAddress;        //CPU has written half of address to 0x2006 in CPU address space
    bool readToRAM;             //CPU has written byte to 0x2007 in CPU address space
    bool readToOAM;             //CPU has written byte to 0x4014 in CPU address space
    bool readScroll;

    //registers
    uint16_t vramAddress;                   //current VRAM address
    uint8_t oamAddress;                     //current OAM address
    bool addressLatch;             //lower part of write to 2006 is occuring

    //settings
    bool usesRAM;               //true if CHR_RAM is used rather than CHR_ROM
    enum Mirroring mirroring;   //nametable arrangement

    //hardware
    uint8_t OAM[0x100];                     //256 byte PPU OAM
    uint8_t ppuRegisters[0x8];  //PPU registers
    uint8_t * CHR_ROM;          //cartridge video ROM
    uint32_t pixels[256 * 240]; //pixel display

    PPU();

    void tick(NES *, int);            //one PPU tick is executed
    void freePointers();        //free memory

    uint8_t getPpuByte(uint16_t);           //get byte from PPU address space

};

#endif
/* DEFINED __NES_PPU__ */
