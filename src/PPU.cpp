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

    for (int x = 0; x < 32; x++) {
        secondaryOAM[x] = 0x0;
    }

    scanline = 241;
    ppuCycle = 0;

    evenFrame = true;
    draw = false;
    
    vramAddress = 0x0;
    oamAddress = 0x0;
    secondaryOamAddress = 0x0;

    getVramAddress = false;
    readLower = false;

    readToRAM = false;
    readToOAM = false;

    nametableOffset = 0;
    vramInc = 1;
    spriteTableOffset = 0;
    backgroundTableOffset = 0;
    extendedSprites = false;
    ppuMaster = false;
    generateNMI = false;

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

    if (address >= 0x0 && address < 0x2000) {
        return CHR_ROM[address];
    } else if (address >= 0x3F00 && address < 0x4000) {
        uint16_t newAddress;
        newAddress = (address - 0x3F00) % 0x20;

        if (newAddress == 0x10 || newAddress == 0x14 || newAddress == 0x18 || newAddress == 0x1C) {
            newAddress -= 0x10;
        }

        return palette[newAddress];
    } else {

        if (address >= 0x3000 && address < 0x3F00) {
            address -= 0x1000;      //0x3000-0x3EFF map to 0x2000-0x2EFF
        }

        if (mirroring == HORIZONTAL) {
            if (address >= 0x2000 && address < 0x2400) {
                return VRAM[address - 0x2000];
            } else if (address >= 0x2400 && address < 0x2800) {
                return VRAM[address - 0x2400];
            } else if (address >= 0x2800 && address < 0x2C00) {
                return VRAM[address - 0x2400];
            } else if (address >= 0x2C00 && address < 0x3000) {
                return VRAM[address - 0x2800];
            } else {
                return 0;
            }
        } else if (mirroring == VERTICAL) {

            if (address >= 0x2000 && address < 0x2400) {
                return VRAM[address - 0x2000];
            } else if (address >= 0x2400 && address < 0x2800) {
                return VRAM[address - 0x2000];
            } else if (address >= 0x2800 && address < 0x2C00) {
                return VRAM[address - 0x2800];
            } else if (address >= 0x2C00 && address < 0x3000) {
                return VRAM[address - 0x2800];
            } else {
                return 0;
            }
        } else {
            std::cerr << "Mirroring not recognized in getPpuByte()" << std::endl;
            return 0;
        }
    }
}

bool PPU::setPpuByte(uint16_t address, uint8_t byte) {
    address %= 0x4000;

    if (address >= 0x3000 && address < 0x3F00) {
        address -= 0x1000;      //0x3000-0x3EFF map to 0x2000-0x2EFF
    }

    if (address >= 0x0 && address < 0x2000) {
        CHR_ROM[address] = byte;
        if (usesRAM == false) {
            //std::cerr << "Warning, modified ROM" << std::endl;
        }
        return true;
    } else {

        if (mirroring == HORIZONTAL) {

            if (address >= 0x2000 && address < 0x2400) {
                VRAM[address - 0x2000] = byte;
                return true;
            } else if (address >= 0x2400 && address < 0x2800) {
                VRAM[address - 0x2400] = byte;
                return true;
            } else if (address >= 0x2800 && address < 0x2C00) {
                VRAM[address - 0x2400] = byte;
                return true;
            } else if (address >= 0x2C00 && address < 0x3000) {
                VRAM[address - 0x2800] = byte;
                return true;
            }

        } else if (mirroring == VERTICAL) {

            if (address >= 0x2000 && address < 0x2400) {
                VRAM[address - 0x2000] = byte;
                return true;
            } else if (address >= 0x2400 && address < 0x2800) {
                VRAM[address - 0x2000] = byte;
                return true;
            } else if (address >= 0x2800 && address < 0x2C00) {
                VRAM[address - 0x2800] = byte;
                return true;
            } else if (address >= 0x2C00 && address < 0x3000) {
                VRAM[address - 0x2800] = byte;
                return true;
            }
        } else {
            std::cerr << "Mirroring not recognized in getPpuByte()" << std::endl;
            return false;
        }

    }

    if (address >= 0x3F00 && address < 0x4000) {

        uint16_t newAddress;
        newAddress = (address - 0x3F00) % 0x20;

                if (newAddress == 0x10 || newAddress == 0x14 || newAddress == 0x18 || newAddress == 0x1C) {
            newAddress -= 0x10;
        }

        palette[newAddress] = byte;
        return true;
    }

    return false;
}

void PPU::drawSprites() {

    for (int i = 0; i < 32 * 30; i++) {

        int x;
        int y;

        x = (i % 32);
        y = (i / 32);

        int pixelStart;
        pixelStart = x * 8 + y * NES_SCREEN_WIDTH * 8;

        for (int j = 0; j < 8; j++) {

            int spriteRow2[8];

            uint8_t spriteLayer1 = getPpuByte(i * 16 + j);
            uint8_t spriteLayer2 = getPpuByte(i * 16 + j + 8);

            for (int a = 0; a < 8; a++) {
                spriteRow2[a] = 0;
                spriteRow2[a] += getBit(spriteLayer1,a);
                spriteRow2[a] += getBit(spriteLayer2,a);
            }

            for (int k = 7; k >= 0; k--) {
                if (spriteRow2[k] == 3) {
                    pixels[pixelStart + (7 - k) + (j * NES_SCREEN_WIDTH)] = 0x00FFFFFF;
                } else if (spriteRow2[k] == 2) {
                    pixels[pixelStart + (7 - k) + (j * NES_SCREEN_WIDTH)] = 0x00FFF000;
                } else if (spriteRow2[k] == 1) {
                    pixels[pixelStart + (7 - k) + (j * NES_SCREEN_WIDTH)] = 0x00000FFF;
                } else {
                    pixels[pixelStart + (7 - k) + (j * NES_SCREEN_WIDTH)] = 0x00000000;
               }
           }
       }

   }

   return;
}

void PPU::tick(NES * nes) {

    if (setCtrl) {
    /* PPUCTRL */   //this can be set in setCpuByte
        nametableOffset = (ppuRegisters[0] & 0x03) * 0x400;
        vramInc = (ppuRegisters[0] & 0x04) ? 32 : 1;
        spriteTableOffset = (ppuRegisters[0] & 0x8) ? 0x1000 : 0x0;
        backgroundTableOffset = (ppuRegisters[0] & 0x10) ? 0x1000 : 0x0;
        extendedSprites = (ppuRegisters[0] & 0x20) ? true : false;
        ppuMaster = (ppuRegisters[0] & 0x40) ? true : false;
        generateNMI = (ppuRegisters[0] & 0x80) ? true : false;

        setCtrl = false;
    }

    if (getVramAddress) {
        if (readLower) {
            vramAddress |= ppuRegisters[6];
            readLower = false;
        } else {
            vramAddress = (ppuRegisters[6] << 8);
            readLower = true;
        }
        getVramAddress = false;
    }

    if (readToRAM) {
        setPpuByte(vramAddress, ppuRegisters[7]);
        vramAddress += vramInc;
        readToRAM = false;
    }

    if (readToOAM) {
        uint8_t OAMDMA;
        OAMDMA = nes->getCpuByte(0x4014);

        for (unsigned int x = 0; x < 256; x++) {
            OAM[oamAddress] = nes->getCpuByte( (OAMDMA << 8) + x );
            oamAddress++;
        }
    
        readToOAM = false;
    }





    ppuRegisters[2] &= 0xBF;

    if (scanline > 240 && scanline <= 261) {
        ppuRegisters[2] |= 0x80;

    } else if (scanline >= 0 && scanline < 240) {
        ppuRegisters[2] &= 0x7F;

        //crude pixel by pixel render of background
        if (ppuCycle > 0 && ppuCycle < 257) {

            int tileX;
            int tileY;

            int nametableIndex;


            tileX = ( (ppuCycle - 1) / 8);
            tileY = ( scanline / 8);

            nametableIndex = (tileY * 32 + tileX);

            int internalAttributeIndex;
            internalAttributeIndex = ((tileX % 4) / 2) + ((tileY % 4) / 2) * 2;

            int attributeTableIndex;
            attributeTableIndex = (tileX / 4) + (tileY / 4) * 8;
            uint8_t attributeByte;
            attributeByte = getPpuByte(0x23C0 + nametableOffset + attributeTableIndex);

            int paletteIndex;

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

            int pixelStart;
            pixelStart = (ppuCycle - 1) + NES_SCREEN_WIDTH * scanline;

            int spriteStart;
            spriteStart = getPpuByte(0x2000 + nametableIndex + nametableOffset);

            uint8_t spriteLayer1 = getPpuByte(spriteStart * 16 + (scanline % 8) + backgroundTableOffset);
            uint8_t spriteLayer2 = getPpuByte(spriteStart * 16 + (scanline % 8) + 8 + backgroundTableOffset);

            uint32_t colour;
            uint8_t num = 0;

            if (getBit(spriteLayer1, 7 - ((ppuCycle - 1) % 8))) {
                num |= 0x1;
            }
            if (getBit(spriteLayer2, 7 - ((ppuCycle - 1) % 8))) {
                num |= 0x2;
            }

            if (num == 0) {
                colour = paletteTable[getPpuByte(0x3F00)];
            } else { 
                colour = paletteTable[getPpuByte(0x3F00 + num + paletteIndex * 4)];
            }

            pixels[pixelStart] = colour;

            for (int i = 0; i < secondaryOamAddress; i++) {

                int xPos;
                xPos = secondaryOAM[i * 4 + 3];

                if (!( (ppuCycle - 1) - xPos < 8 && (ppuCycle - 1) - xPos >= 0 )) {
                    continue;
                }

                uint8_t byte0, byte1, byte2, byte3;

                byte0 = secondaryOAM[i * 4];
                byte1 = secondaryOAM[(i * 4) + 1];
                byte2 = secondaryOAM[(i * 4) + 2];
                byte3 = secondaryOAM[xPos];

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


                uint8_t spriteLayer3 = getPpuByte(byte1 * 16 + spriteRowNumber + spriteTableOffset);
                uint8_t spriteLayer4 = getPpuByte(byte1 * 16 + spriteRowNumber + 8 + spriteTableOffset);

                uint8_t number = 0; 

                if (getBit(spriteLayer3, spriteColumnNumber)) {
                    number |= 0x1;
                }
                if (getBit(spriteLayer4, spriteColumnNumber)) {
                    number |= 0x2;
                }

                if (number == 0) {

                    if (num == 0) {
                        //sprite zero hit

                        ppuRegisters[2] |= 0x40;

                    }

                    continue;
                }

                uint32_t newColour;
                newColour = paletteTable[getPpuByte(0x3F10 + number + paletteIndex2 * 4)];
                pixels[pixelStart] = newColour;



            }


        }

    } else if (scanline == 240) {

        if (ppuCycle == 340) {
            if (generateNMI) {
                nes->nesCPU.NMI = true;
            }
        }

    } else {
        std::cout << "Error, invalid scanline " << scanline << std::endl;
    }

    if (ppuCycle == 340) {

        //prepare secondary OAM for next scanline
        for (int x = 0; x < 32; x++) {
            secondaryOAM[x] = 0;
        }

        secondaryOamAddress = 0;

        for (int x = 0; x < 64; x++) {
            int yPos;
            yPos = OAM[x * 4];
            if (((scanline + 1) % 262) - yPos < 8 && ((scanline + 1) % 262) - yPos >= 0) {
                secondaryOAM[secondaryOamAddress * 4] = yPos;
                secondaryOAM[secondaryOamAddress * 4 + 1] = OAM[x * 4 + 1];
                secondaryOAM[secondaryOamAddress * 4 + 2] = OAM[x * 4 + 2];
                secondaryOAM[secondaryOamAddress * 4 + 3] = OAM[x * 4 + 3];

                secondaryOamAddress++;

                if (secondaryOamAddress > 7) {
                    break;
                }

            }
        }
    }



    draw = false;
    ppuCycle = (ppuCycle + 1) % 341;

    if (ppuCycle == 0) {
        scanline = (scanline + 1) % 262;
        if (scanline == 0) {
            draw = true;
            evenFrame ^= true;
        }
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
