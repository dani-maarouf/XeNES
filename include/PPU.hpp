#pragma once
#include <cstdint>

typedef uint8_t u8;
typedef uint16_t u16;

const int NES_SCREEN_WIDTH = 256;
const int NES_SCREEN_HEIGHT = 240;

/* nametable mirroring configuration */
enum Mirroring {
    VERTICAL,
    HORIZONTAL,
    FOUR_SCREEN,
};

class PPU {

private:

    //render variables
    u8 m_SpriteOld1;
    u8 m_SpriteOld2;
    u8 m_PaletteOld;
    u8 m_SpriteNew1;
    u8 m_SpriteNew2;
    u8 m_PaletteNew;    

    u8 m_x;                        //fine x scroll
    u16 m_v;                       //current vram address
    u16 m_t;                       //temporary vram address
    bool addressLatch;                  //w register
    u8 readBuffer;                 //$2007 buffer

    //hardware
    u8 palette[0x20];              //palette indicies
    u8 VRAM[0x1000];               //4kB PPU internal VRAM
    u8 lineOAM[6 * 8];             //secondary oam, not faithful to hardware

    void setPpuByte(u16, u8); //set byte in PPU address space
    u8 getPpuByte(u16);       //get byte from PPU address space
    void loadNewTile();                 //load shift registers with rendering bytes
    void ppuFlagUpdate(bool *);         //update ppu state based on register access
    void drawPixel(int, int);           //draw background and sprite pixels
    void updateSecondaryOAM(int);       //prepare secondary oam for next line

public:

    //0-7 = $2000 - $2007
    int writeFlag;
    int readFlag;

    //registers
    u8 oamAddress;                 //current OAM address
    bool suppressVBL;                   //dont throw up vbl flag
    bool suppressCpuTickSkip;

    //info/configuration
    bool draw;                          //draw frame?
    uintmax_t ppuClock;                  
    int numRomBanks;                    //number of 4kb rom banks
    int ppuMapper;                      //cartridge mapper
    bool usesRAM;                       //true if CHR_RAM is used rather than CHR_ROM
    enum Mirroring mirroring;           //nametable arrangement

    //hardware
    u8 OAM[0x100];                 //256 byte PPU OAM
    u8 ppuRegisters[0x8];          //PPU registers
    u8 * CHR_ROM;                  //cartridge video ROM (pattern tables)
    u8 pixels[256 * 240];

    PPU();
    void tick(bool *, uintmax_t *);      //one PPU tick is executed
    void freePointers();                //free memory
    u8 return2007();               //return ppudata for 'cpu get byte'

};
