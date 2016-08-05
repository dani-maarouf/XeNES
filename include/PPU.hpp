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

class PPU {

private:

    //render
    uint8_t m_SpriteOld1;
    uint8_t m_SpriteOld2;
    uint8_t m_PaletteOld;
    uint8_t m_SpriteNew1;
    uint8_t m_SpriteNew2;
    uint8_t m_PaletteNew;    

    uint8_t m_x;                        //fine x scroll
    uint16_t m_v;                       //current vram address
    uint16_t m_t;                       //temporary vram address
    bool addressLatch;                  //w register
    uint8_t readBuffer;                 //$2007 buffer

    //hardware
    uint8_t palette[0x20];
    uint8_t VRAM[0x1000];               //4kB PPU internal VRAM
    uint8_t lineOAM[6 * 8];             //secondary oam, not faithful to hardware

    void loadNewTile();
    void setPpuByte(uint16_t, uint8_t); //set byte in PPU address space
    uint8_t getPpuByte(uint16_t);       //get byte from PPU address space
    void ppuFlagUpdate(bool *);
    void drawPixel(int, int);
    void updateSecondaryOAM(int);

public:

    //0-7 = $2000 - $2007
    int writeFlag;
    int readFlag;

    //registers
    uint8_t oamAddress;                 //current OAM address
    bool suppressVBL;
    bool suppressCpuTickSkip;


    //info/configuration
    bool draw;                          //draw frame?
    uint64_t ppuClock;
    int numRomBanks;
    int ppuMapper;
    bool usesRAM;                       //true if CHR_RAM is used rather than CHR_ROM
    enum Mirroring mirroring;           //nametable arrangement

    //hardware
    uint8_t OAM[0x100];                 //256 byte PPU OAM
    uint8_t ppuRegisters[0x8];          //PPU registers
    uint8_t * CHR_ROM;                  //cartridge video ROM (pattern tables)
    uint8_t pixels[256 * 240];

    PPU();
    void setVblank(bool);
    void tick(bool *, uint64_t *);             //one PPU tick is executed
    void freePointers();                //free memory
    uint8_t return2007();               //return $2007

};

#endif
/* DEFINED __NES_PPU__ */
