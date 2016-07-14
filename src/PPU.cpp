#include <iostream>

#include "NES.hpp"

//obtained from blargg's Full Palette demo
const uint32_t paletteTable [] =
 {  
    ( 84 << 16) | ( 84 << 8) | ( 84),  (  0 << 16) | ( 30 << 8) | (116),  (  8 << 16) | ( 16 << 8) | (144),  ( 48 << 16) | (  0 << 8) | (136),
    ( 68 << 16) | (  0 << 8) | (100),  ( 92 << 16) | (  0 << 8) | ( 48),  ( 84 << 16) | (  4 << 8) | (  0),  ( 60 << 16) | ( 24 << 8) | (  0),
    ( 32 << 16) | ( 42 << 8) | (  0),  (  8 << 16) | ( 58 << 8) | (  0),  (  0 << 16) | ( 64 << 8) | (  0),  (  0 << 16) | ( 60 << 8) | (  0),
    (  0 << 16) | ( 50 << 8) | ( 60),  (  0 << 16) | (  0 << 8) | (  0),  (  0 << 16) | (  0 << 8) | (  0),  (  0 << 16) | (  0 << 8) | (  0),
    (152 << 16) | (150 << 8) | (152),  (  8 << 16) | ( 76 << 8) | (196),  ( 48 << 16) | ( 50 << 8) | (236),  ( 92 << 16) | ( 30 << 8) | (228),
    (136 << 16) | ( 20 << 8) | (176),  (160 << 16) | ( 20 << 8) | (100),  (152 << 16) | ( 34 << 8) | ( 32),  (120 << 16) | ( 60 << 8) | (  0),
    ( 84 << 16) | ( 90 << 8) | (  0),  ( 40 << 16) | (114 << 8) | (  0),  (  8 << 16) | (124 << 8) | (  0),  (  0 << 16) | (118 << 8) | ( 40), 
    (  0 << 16) | (102 << 8) | (120),  (  0 << 16) | (  0 << 8) | (  0),  (  0 << 16) | (  0 << 8) | (  0),  (  0 << 16) | (  0 << 8) | (  0),
    (236 << 16) | (238 << 8) | (236),  ( 76 << 16) | (154 << 8) | (236),  (120 << 16) | (124 << 8) | (236),  (176 << 16) | ( 98 << 8) | (236), 
    (228 << 16) | ( 84 << 8) | (236),  (236 << 16) | ( 88 << 8) | (180),  (236 << 16) | (106 << 8) | (100),  (212 << 16) | (136 << 8) | ( 32),
    (160 << 16) | (170 << 8) | (  0),  (116 << 16) | (196 << 8) | (  0),  ( 76 << 16) | (208 << 8) | ( 32),  ( 56 << 16) | (204 << 8) | (108),
    ( 56 << 16) | (180 << 8) | (204),  ( 60 << 16) | ( 60 << 8) | ( 60),  (  0 << 16) | (  0 << 8) | (  0),  (  0 << 16) | (  0 << 8) | (  0),
    (236 << 16) | (238 << 8) | (236),  (168 << 16) | (204 << 8) | (236),  (188 << 16) | (188 << 8) | (236),  (212 << 16) | (178 << 8) | (236),
    (236 << 16) | (174 << 8) | (236),  (236 << 16) | (174 << 8) | (212),  (236 << 16) | (180 << 8) | (176),  (228 << 16) | (196 << 8) | (144),
    (204 << 16) | (210 << 8) | (120),  (180 << 16) | (222 << 8) | (120),  (168 << 16) | (226 << 8) | (144),  (152 << 16) | (226 << 8) | (180),
    (160 << 16) | (214 << 8) | (228),  (160 << 16) | (162 << 8) | (160),  (  0 << 16) | (  0 << 8) | (  0),  (  0 << 16) | (  0 << 8) | (  0)
};

static inline bool getBit(uint8_t, int);

PPU::PPU() {

    for (int x = 0; x < 0x20; x++) palette[x] = 0;
    for (int x = 0; x < 0x8; x++) ppuRegisters[x] = 0x0;
    for (int x = 0; x < 0x800; x++) VRAM[x] = 0x0;
    for (int x = 0; x < 0x100; x++) OAM[x] = 0x0;
    for (int x = 0; x < (NES_SCREEN_WIDTH * NES_SCREEN_HEIGHT); x++) pixels[x] = 0;

    for (int x = 0; x < 8; x++) {
        secondaryOAM[x] = 0x0;
    }

    for (int x = 0; x < 0x9; x++) {
        registerWriteFlags[x] = false;
    }

    for (int x = 0; x < 8; x++) {
        registerReadFlags[x] = false;
    }

    scanline = 241;
    ppuCycle = 0;

    evenFrame = true;
    draw = false;
    
    vramAddress = 0x2000;
    oamAddress = 0x0;
    secondaryOamAddress = 0x0;

    addressLatch = false;

    CHR_ROM = NULL;

    spriteZeroFlag = false;

    //rendering
    spriteLayer1 = 0;
    spriteLayer2 = 0;

    readBuffer = 0;

    m_v = 0;
    m_t = 0;
    m_x = 0;


    return;
}

void PPU::freePointers() {
    if (CHR_ROM != NULL) {
        delete [] CHR_ROM;
    }
    return;
}

uint8_t PPU::getPpuByte(uint16_t address) {
    address %= 0x4000;

    if (address < 0x1000) {
        return CHR_ROM[address];
    } else if (address < 0x2000) {
        return CHR_ROM[address];
    } else if (address < 0x2400) {
        return VRAM[address - 0x2000];
    } else if (address < 0x2800) {
        if (mirroring == VERTICAL) {
            return VRAM[address - 0x2000];
        } else if (mirroring == HORIZONTAL) {
            return VRAM[address - 0x2400];
        } else {
            return VRAM[address - 0x2000];
        }
    } else if (address < 0x2C00) {
        if (mirroring == VERTICAL) {
            return VRAM[address - 0x2800];
        } else if (mirroring == HORIZONTAL) {
            return VRAM[address - 0x2400];
        } else {
            return VRAM[address - 0x2000];
        }
    } else if (address < 0x3000) {
        if (mirroring == VERTICAL) {
            return VRAM[address - 0x2800];
        } else if (mirroring == HORIZONTAL) {
            return VRAM[address - 0x2800];
        } else {
            return VRAM[address - 0x2000];
        }
    } else if (address < 0x3400) {
        return VRAM[address - 0x3000];
    } else if (address < 0x3800) {
        if (mirroring == VERTICAL) {
            return VRAM[address - 0x3000];
        } else if (mirroring == HORIZONTAL) {
            return VRAM[address - 0x3400];
        } else {
            return VRAM[address - 0x3000];
        }
    } else if (address < 0x3C00) {
        if (mirroring == VERTICAL) {
            return VRAM[address - 0x3800];
        } else if (mirroring == HORIZONTAL) {
            return VRAM[address - 0x3400];
        } else {
            return VRAM[address - 0x3000];
        }
    } else if (address < 0x3F00) {
        if (mirroring == VERTICAL) {
            return VRAM[address - 0x3800];
        } else if (mirroring == HORIZONTAL) {
            return VRAM[address - 0x3800];
        } else {
            return VRAM[address - 0x3000];
        }
    } else {
        uint8_t newAddress;
        newAddress = (address - 0x3F00) % 0x20;
        if (newAddress == 0x10 || newAddress == 0x14 || newAddress == 0x18 || newAddress == 0x1C) {
            newAddress -= 0x10;
        }
        return palette[newAddress];
    }
}

void PPU::setPpuByte(uint16_t address, uint8_t byte) {
    address %= 0x4000;

    if (address < 0x1000) {
        CHR_ROM[address] = byte;
    } else if (address < 0x2000) {
        CHR_ROM[address] = byte;
    } else if (address < 0x2400) {
        VRAM[address - 0x2000] = byte;
    } else if (address < 0x2800) {
        if (mirroring == VERTICAL) {
            VRAM[address - 0x2000] = byte;
        } else if (mirroring == HORIZONTAL) {
            VRAM[address - 0x2400] = byte;
        } else {
            VRAM[address - 0x2000] = byte;
        }
    } else if (address < 0x2C00) {
        if (mirroring == VERTICAL) {
            VRAM[address - 0x2800] = byte;
        } else if (mirroring == HORIZONTAL) {
            VRAM[address - 0x2400] = byte;
        } else {
            VRAM[address - 0x2000] = byte;
        }
    } else if (address < 0x3000) {
        if (mirroring == VERTICAL) {
            VRAM[address - 0x2800] = byte;
        } else if (mirroring == HORIZONTAL) {
            VRAM[address - 0x2800] = byte;
        } else {
            VRAM[address - 0x2000] = byte;
        }
    } else if (address < 0x3400) {
        VRAM[address - 0x3000] = byte;
    } else if (address < 0x3800) {
        if (mirroring == VERTICAL) {
            VRAM[address - 0x3000] = byte;
        } else if (mirroring == HORIZONTAL) {
            VRAM[address - 0x3400] = byte;
        } else {
            VRAM[address - 0x3000] = byte;
        }
    } else if (address < 0x3C00) {
        if (mirroring == VERTICAL) {
            VRAM[address - 0x3800] = byte;
        } else if (mirroring == HORIZONTAL) {
            VRAM[address - 0x3400] = byte;
        } else {
            VRAM[address - 0x3000] = byte;
        }
    } else if (address < 0x3F00) {
        if (mirroring == VERTICAL) {
            VRAM[address - 0x3800] = byte;
        } else if (mirroring == HORIZONTAL) {
            VRAM[address - 0x3800] = byte;
        } else {
            VRAM[address - 0x3000] = byte;
        }
    } else {
        uint8_t newAddress;
        newAddress = (address - 0x3F00) % 0x20;
        if (newAddress == 0x10 || newAddress == 0x14 || newAddress == 0x18 || newAddress == 0x1C) {
            newAddress -= 0x10;
        }
        palette[newAddress] = byte;
    }
    return;
}

void PPU::tick(NES * nes, int numTicks) {

    draw = false; 

    if (registerWriteFlags[0]) {

        m_t &= 0xF3FF;
        m_t |= ((ppuRegisters[0] & 0x03) << 10);

        //spriteTableOffset = (ppuRegisters[0] & 0x8) ? 0x1000 : 0x0;
        //backgroundTableOffset = (ppuRegisters[0] & 0x10) ? 0x1000 : 0x0;
        //extendedSprites = (ppuRegisters[0] & 0x20) ? true : false;
        //ppuMaster = (ppuRegisters[0] & 0x40) ? true : false;
        //generateNMI = (ppuRegisters[0] & 0x80) ? true : false;

        ppuRegisters[0x2] |= (ppuRegisters[0x0] & 0x1F);
    } else if (registerWriteFlags[1]) {


         ppuRegisters[0x2] |= (ppuRegisters[0x1] & 0x1F);
    } else if (registerWriteFlags[2]) {


         ppuRegisters[0x2] |= (ppuRegisters[0x2] & 0x1F);
    } else if (registerWriteFlags[3]) {

        oamAddress = ppuRegisters[0x3];

         ppuRegisters[0x2] |= (ppuRegisters[0x3] & 0x1F);
    } else if (registerWriteFlags[4]) {


         ppuRegisters[0x2] |= (ppuRegisters[0x4] & 0x1F);
    } else if (registerWriteFlags[5]) {


        if (addressLatch == true) {

            m_t &= 0x0C1F;
            m_t |= (ppuRegisters[0x5] & 0x7) << 12;
            m_t |= (ppuRegisters[0x5] & 0xF8) << 2;

            addressLatch = false;

        } else {

            m_x = (ppuRegisters[0x5] & 0x7);

            m_t &= 0xFFE0;
            m_t |= (ppuRegisters[0x5] & 0xF8) >> 3;

            addressLatch = true;

        }
        
         ppuRegisters[0x2] |= (ppuRegisters[0x5] & 0x1F);
    } else if (registerWriteFlags[6]) {


        if (addressLatch) {


            m_t &= 0xFF00;
            m_t |= ppuRegisters[6];
            m_v = m_t;


            vramAddress &= 0xFF00;
            vramAddress |= ppuRegisters[6];
            //address latch sets to false on ppustatus read
            addressLatch = false;
            
        } else {


            m_t &= 0x00FF;
            m_t |= (ppuRegisters[6] & 0x3F) << 8;



            vramAddress = (ppuRegisters[6] << 8);

            addressLatch = true;

        }
        
         ppuRegisters[0x2] |= (ppuRegisters[0x6] & 0x1F);
    } else if (registerWriteFlags[7]) {


        //read from 2007
        setPpuByte(vramAddress, ppuRegisters[7]);

        //increment vram address

        vramAddress += (ppuRegisters[0] & 0x04) ? 32 : 1;
        //v_t +=
            
         ppuRegisters[0x2] |= (ppuRegisters[0x7] & 0x1F);
    } else if (registerWriteFlags[8]) {

        uint8_t OAMDMA;
        OAMDMA = nes->getCpuByte(0x4014);

        for (unsigned int x = 0; x < 256; x++) {
            OAM[oamAddress] = nes->getCpuByte( (OAMDMA << 8) + x );
            oamAddress++;
        }

    }

    if (registerReadFlags[2]) {

        addressLatch = false;

    } else if (registerReadFlags[4]) {

    } else if (registerReadFlags[7]) {

        //todo: fix this

    }

    for (int x = 0; x < 0x9; x++) {
        registerWriteFlags[x] = false;
    }

    for (int x = 0; x < 8; x++) {
        registerReadFlags[x] = false;
    }



    /*
     * render
     */
    
    for (int loop = 0; loop < numTicks; loop++) {

        if (scanline == 240) {
            if (ppuCycle == 340) {

                if ((ppuRegisters[0] & 0x80)) {
                    //throw interupt
                    nes->nesCPU.NMI = true;
                }
                
            }
        } else if (scanline == 261) {



            //update ppustatus
            if (ppuCycle == 2) {
                ppuRegisters[2] &= 0xBF;
                ppuRegisters[2] &= 0xDF;
            }

            if (ppuCycle == 304) {
                m_v &= 0x041F;
                m_v |= (m_t & (~0x041F));
            }
            


        } else if (scanline > 240) {

            //update ppustatus
            ppuRegisters[2] |= 0x80;

        } else if (scanline < 240) {
            ppuRegisters[2] &= 0x7F;


            if (ppuCycle > 0 && ppuCycle < 257) {



                
                int pixelStart;
                pixelStart = (ppuCycle - 1) + NES_SCREEN_WIDTH * scanline;  //current pixel being rendered

                uint8_t num;
                num = 0;                

                //RENDER BACKGROUNDS
                {
                    
                    if (((ppuCycle - 1 + m_x) % 8 == 0 || ppuCycle == 1)) {

                        int tileX = (m_v & 0x1F);
                        int tileY = (m_v & 0x370) >> 5;

                        uint8_t attributeByte = getPpuByte(0x23C0 | (m_v & 0x0C00) | ((m_v >> 4) & 0x38) | ((m_v >> 2) & 0x07));

                        int internalAttributeIndex = ((tileX % 4) / 2) + ((tileY % 4) / 2) * 2;
                        if (internalAttributeIndex == 0) {
                            paletteIndex = (attributeByte & 0x3);
                        } else if (internalAttributeIndex == 1) {
                            paletteIndex = (attributeByte & 0xC) >> 2;
                        } else if (internalAttributeIndex == 2) {
                            paletteIndex = (attributeByte & 0x30) >> 4;
                        } else if (internalAttributeIndex == 3) {
                            paletteIndex = (attributeByte & 0xC0) >> 6;
                        } else {
                            paletteIndex = 0;
                            std::cout << "Error!" << std::endl;
                        }

                        spriteStart = getPpuByte(0x2000 | (m_v & 0x0FFF));

                        spriteLayer1 = getPpuByte(spriteStart * 16 + ((m_v & 0x7000) >> 12) + ((ppuRegisters[0] & 0x10) ? 0x1000 : 0x0));
                        spriteLayer2 = getPpuByte(spriteStart * 16 + ((m_v & 0x7000) >> 12) + ((ppuRegisters[0] & 0x10) ? 0x1000 : 0x0) + 8);

                    }



                    if (ppuRegisters[1] & 0x8) {

                        if (getBit(spriteLayer1, 7 - ((ppuCycle - 1 + (m_x)) % 8 ))) {
                            num |= 0x1;
                        }
                        if (getBit(spriteLayer2, 7 - ((ppuCycle - 1 + (m_x)) % 8 ))) {
                            num |= 0x2;
                        }

                        uint32_t colour;
                        if (num == 0) {
                            colour = paletteTable[getPpuByte(0x3F00)];
                        } else { 
                            colour = paletteTable[getPpuByte(0x3F00 + num + paletteIndex * 4)];
                        }

                        pixels[pixelStart] = colour;

                    }
                }




                //RENDER SPRITES
                if (ppuRegisters[1] & 0x10) {

                    for (int i = 0; i < secondaryOamAddress; i++) {

                        int xPos;
                        xPos = OAM[secondaryOAM[i] + 3];

                        if (!( (ppuCycle - 1) - xPos < 8 && (ppuCycle - 1) - xPos >= 0 )) {
                            continue;
                        }

                        uint8_t byte0, byte1, byte2;

                        byte0 = OAM[secondaryOAM[i]];
                        byte1 = OAM[secondaryOAM[i] + 1];
                        byte2 = OAM[secondaryOAM[i] + 2];

                        int paletteIndex2 = byte2 & 0x3;

                        bool flipHorizontal;
                        bool flipVertical;
                        flipHorizontal = (byte2 & 0x40) ? true : false;
                        flipVertical = (byte2 & 0x80) ? true : false;

                        int spriteRowNumber;

                        if (flipVertical) {
                            spriteRowNumber = 7 -(scanline - byte0);
                        } else {
                            spriteRowNumber = (scanline - byte0);
                        }

                        int spriteColumnNumber;

                        if (flipHorizontal) {
                            spriteColumnNumber = ((ppuCycle - 1) - xPos);
                        } else {
                            spriteColumnNumber = 7 - ((ppuCycle - 1) - xPos);
                        }


                        uint8_t spriteLayer3 = getPpuByte(byte1 * 16 + spriteRowNumber + ((ppuRegisters[0] & 0x8) ? 0x1000 : 0x0));
                        uint8_t spriteLayer4 = getPpuByte(byte1 * 16 + spriteRowNumber + 8 + ((ppuRegisters[0] & 0x8) ? 0x1000 : 0x0));

                        uint8_t number = 0; 

                        if (getBit(spriteLayer3, spriteColumnNumber)) {
                            number |= 0x1;
                        }
                        if (getBit(spriteLayer4, spriteColumnNumber)) {
                            number |= 0x2;
                        }

                        if ((number != 0x0)) {

                            if (secondaryOAM[i] == 0) {
                                if ((ppuRegisters[1] & 0x8) && (ppuRegisters[1] & 0x10)) {
                                    spriteZeroFlag = true;
                                }
                            }
                        }

                        if (number == 0x0) {
                            continue;
                        }    

                        uint32_t newColour;
                        newColour = paletteTable[getPpuByte(0x3F10 + number + paletteIndex2 * 4)];

                        if (((byte2 & 0x20) == 0) || num == 0) {
                            pixels[pixelStart] = newColour;
                        }

                    }
                }



                if (((ppuCycle - 1 + m_x) % 8 == 0 || ppuCycle == 1)) {

                    if ((m_v & 0x001F) == 31) {
                        //~ = bitwise not
                        m_v &= ~0x001F;
                        m_v ^= 0x400;
                    } else {
                        m_v++;
                    }
                }

                if (ppuCycle == 256) {

                    if ((m_v & 0x7000) != 0x7000) {
                        m_v += 0x1000;
                    } else {
                        m_v &= ~0x7000;

                        int yVal;
                        yVal = (m_v & 0x03E0) >> 5;

                        if (yVal == 29) {
                            yVal = 0;
                            m_v ^= 0x800;
                        } else if (yVal == 31) {
                            yVal = 0;
                        } else {
                            yVal += 1;
                        }

                        m_v = (m_v & ~0x03E0) | (yVal << 5);
                    }

                }
            } else if (ppuCycle == 257) {

                m_v &= ~0x041F;
                m_v |= (m_t & 0x041F);

            } 


        }

        //prepare sprites for next scaline
        if (ppuCycle == 340 && (scanline == 261 || scanline < 239)) {

            //prepare secondary OAM for next scanline
            for (int x = 0; x < 8; x++) {
                secondaryOAM[x] = 0;
            }

            secondaryOamAddress = 0;

            for (int x = 0; x < 64; x++) {
                int yPos;
                yPos = OAM[x * 4];
                if (((scanline + 1) % 262) - yPos < 8 && ((scanline + 1) % 262) - yPos >= 0) {

                    if (secondaryOamAddress < 8) {
                        secondaryOAM[secondaryOamAddress] = x * 4;
                    }

                    secondaryOamAddress++;

                    //sprite overflow detection
                    if (secondaryOamAddress == 9) {

                        if (((ppuRegisters[1] & 0x10)) || ((ppuRegisters[1] & 0x8))) {
                            ppuRegisters[2] |= 0x20;
                            secondaryOamAddress--;
                            break;
                        }

                        
                    }

                }
            }
        }


        //increment ppuCycle and scanline, and set draw flag
        ppuCycle = (ppuCycle + 1) % 341;
        if (ppuCycle == 0) {
            scanline = (scanline + 1) % 262;
            if (spriteZeroFlag) {
                ppuRegisters[2] |= 0x40;
                spriteZeroFlag = false;
            }
            if (scanline == 0) {

                //vramAddress = 0x2000;

                draw = true;
                evenFrame ^= true;
            }
        }

        //todo: odd/even frame timing
        

    }

    return;
}

static inline bool getBit(uint8_t num, int bitNum) {

    if (bitNum == 0) {
        return (num & 0x1);
    } else if (bitNum == 1) {
        return (num & 0x2);
    } else if (bitNum == 2) {
        return (num & 0x4);
    } else if (bitNum == 3) {
        return (num & 0x8);
    } else if (bitNum == 4) {
        return (num & 0x10);
    } else if (bitNum == 5) {
        return (num & 0x20);
    } else if (bitNum == 6) {
        return (num & 0x40);
    } else {
        return (num & 0x80);
    }
}
