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
    bool m_addressLatch;                  //w register
    u8 m_readBuffer;                 //$2007 buffer

    //hardware
    u8 m_palette[0x20];              //palette indicies
    u8 m_VRAM[0x1000];               //4kB PPU internal VRAM
    u8 m_lineOAM[6 * 8];             //secondary oam, not faithful to hardware

    void set_ppu_byte(u16, u8); //set byte in PPU address space
    u8 get_ppu_byte(u16);       //get byte from PPU address space
    void load_new_tile();                 //load shift registers with rendering bytes
    void ppu_flag_update(bool *);         //update ppu state based on register access
    void draw_pixel(int, int);           //draw background and sprite pixels
    void update_secondary_oam(int);       //prepare secondary oam for next line

public:

    //0-7 = $2000 - $2007
    int m_writeFlag;
    int m_readFlag;

    //registers
    u8 m_oamAddress;                 //current OAM address
    bool m_suppressVBL;                   //dont throw up vbl flag
    bool m_suppressCpuTickSkip;

    //info/configuration
    bool m_draw;                          //draw frame?
    uintmax_t m_ppuClock;                  
    int m_numRomBanks;                    //number of 4kb rom banks
    int m_ppuMapper;                      //cartridge mapper
    bool m_usesRAM;                       //true if CHR_RAM is used rather than CHR_ROM
    enum Mirroring m_mirroring;           //nametable arrangement

    //hardware
    u8 m_OAM[0x100];                 //256 byte PPU OAM
    u8 m_ppuRegisters[0x8];          //PPU registers
    u8 * m_CHR_ROM;                  //cartridge video ROM (pattern tables)
    u8 m_pixels[256 * 240];

    PPU();
    void tick(bool *, uintmax_t *);      //one PPU tick is executed
    void free_pointers();                //free memory
    u8 return_2007();               //return ppudata for 'cpu get byte'

};
