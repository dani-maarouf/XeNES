#include <iostream>
#include <bitset>

#include "NES.hpp"


//obtained from blargg's Full Palette demo
//
const uint8_t paletteTable [] =
 {   84, 84, 84,    0, 30,116,    8, 16,144,   48,  0,136,   68,  0,100,   92,  0, 48,   84,  4,  0,   60, 24,  0,   32, 42,  0,    8, 58,  0,    0, 64,  0,    0, 60,  0,    0, 50, 60,    0,  0,  0,    0,0,0,    0,0,0,
    152,150,152,    8, 76,196,   48, 50,236,   92, 30,228,  136, 20,176,  160, 20,100,  152, 34, 32,  120, 60,  0,   84, 90,  0,   40,114,  0,    8,124,  0,    0,118, 40,    0,102,120,    0,  0,  0,    0,0,0,    0,0,0,
    236,238,236,   76,154,236,  120,124,236,  176, 98,236,  228, 84,236,  236, 88,180,  236,106,100,  212,136, 32,  160,170,  0,  116,196,  0,   76,208, 32,   56,204,108,   56,180,204,   60, 60, 60,    0,0,0,    0,0,0,
    236,238,236,  168,204,236,  188,188,236,  212,178,236,  236,174,236,  236,174,212,  236,180,176,  228,196,144,  204,210,120,  180,222,120,  168,226,144,  152,226,180,  160,214,228,  160,162,160,    0,0,0,    0,0,0
};

static uint32_t paletteTableToARGB(uint8_t index) {

    uint8_t byte1 = paletteTable[index * 3];
    uint8_t byte2 = paletteTable[index * 3 + 1];
    uint8_t byte3 = paletteTable[index * 3 + 2];

    return (byte1 << 16) | (byte2 << 8) | byte3;
}


PPU::PPU() {

    for (int x = 0; x < 0x20; x++) palette[x] = 0;
    for (int x = 0; x < 0x8; x++) ppuRegisters[x] = 0x0;
    for (int x = 0; x < 0x800; x++) VRAM[x] = 0x0;
    for (int x = 0; x < 0x100; x++) OAM[x] = 0x0;
    for (int x = 0; x < NES_SCREEN_WIDTH * NES_SCREEN_HEIGHT; x++) pixels[x] = 0;

    scanline = 241;
    ppuCycle = 0;

    evenFrame = true;
    draw = false;
    
    vramAddress = 0x0;

    ppuGetAddr = false;
    readLower = false;
    readToRAM = false;
    readToOAM = false;

}

void PPU::freePointers() {
    if (CHR_ROM != NULL) {
        delete [] CHR_ROM;
    }
    return;
}

uint8_t PPU::getPpuByte(uint16_t address) {
    address %= 0x4000;

    if (address >= 0x3000 && address < 0x3F00) {
        address -= 0x1000;      //0x3000-0x3EFF map to 0x2000-0x2EFF
    }

    if (address >= 0x0 && address < 0x2000) {
        return CHR_ROM[address];
    } else {

        if (mirroring == HORIZONTAL) {

            if (address >= 0x2000 && address < 0x2400) {
                return VRAM[address - 0x2000];
            } else if (address >= 0x2400 && address < 0x2800) {
                return VRAM[address - 0x2400];
            } else if (address >= 0x2800 && address < 0x2C00) {
                return VRAM[address - 0x2400];
            } else if (address >= 0x2C00 && address < 0x3000) {
                return VRAM[address - 0x2800];
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
            }

        } else {
            std::cerr << "Mirroring not recognized in getPpuByte()" << std::endl;
            return 0;
        }
    }

    if (address >= 0x3F00 && address < 0x4000) {
        return palette[(address - 0x3F00) % 0x20];
    }

    return 0;
}

bool PPU::setPpuByte(uint16_t address, uint8_t byte) {
    address %= 0x4000;

    if (address >= 0x3000 && address < 0x3F00) {
        address -= 0x1000;      //0x3000-0x3EFF map to 0x2000-0x2EFF
    }

    if (address >= 0x0 && address < 0x2000) {
        CHR_ROM[address] = byte;
        if (usesRAM == false) {
            std::cerr << "Warning, modified ROM" << std::endl;
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
        palette[(address - 0x3F00) % 0x20] = byte;
        return true;
    }

    return false;
}

void PPU::printSprites() {

    for (int i = 0x0; i < 0x2000; i += 0x10) {

        std::cout << "Sprite #" << std::dec << (i / 0x10) << " @ 0x" << std::hex << i << std::endl;

        for (int x = i; x < 0x8 + i; x++) {

            std::bitset<8> row;
            row = (std::bitset<8>) (getPpuByte(x) | getPpuByte(x + 8));

            for (int y = 7; y >= 0; y--) {
                if (row[y]) {
                    std::cout << '#';
                } else {
                    std::cout << ' ';
                }
            }
            std::cout << std::endl;
        }

        std::cout << std::endl << std::endl;
    }

    return;
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

            std::bitset<8> spriteLayer1 = (std::bitset<8>) getPpuByte(i * 16 + j);
            std::bitset<8> spriteLayer2 = (std::bitset<8>) getPpuByte(i * 16 + j + 8);

            for (int a = 0; a < 8; a++) {
                spriteRow2[a] = 0;
                spriteRow2[a] += spriteLayer1[a];
                spriteRow2[a] += spriteLayer2[a];
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

int PPU::tick(bool * NMI, NES * nes) {

    /* PPUCTRL */
    int nametableOffset;
    nametableOffset = (ppuRegisters[0] & 0x03) * 0x400;
    int vramInc;
    vramInc = (ppuRegisters[0] & 0x04) ? 32 : 1;
    int spriteTableOffset;
    spriteTableOffset = (ppuRegisters[0] & 0x8) ? 0x1000 : 0x0;
    int backgroundTableOffset;
    backgroundTableOffset = (ppuRegisters[0] & 0x10) ? 0x1000 : 0x0;
    bool extendedSprites;
    extendedSprites = (ppuRegisters[0] & 0x20) ? true : false;
    bool ppuMaster;
    ppuMaster = (ppuRegisters[0] & 0x40) ? true : false;
    bool generateNMI;
    generateNMI = (ppuRegisters[0] & 0x80) ? true : false;


    /* PPUMASK */
    //todo




    if (ppuGetAddr) {
        if (readLower) {
            vramAddress |= ppuRegisters[6];
            //std::cout << "PPU VRAM addr : " << std::hex << vramAddress << std::endl;
            readLower = false;
        } else {
            vramAddress = (ppuRegisters[6] << 8);
            readLower = true;
        }
        ppuGetAddr = false;
    }

    if (readToRAM) {
        setPpuByte(vramAddress, ppuRegisters[7]);

        vramAddress += vramInc;

        //std::cout << "PPU VRAM addr : " << std::hex << vramAddress << std::endl;

        readToRAM = false;
    }

    if (readToOAM) {

        
        uint8_t OAMDMA;
        OAMDMA = nes->getCpuByte(0x4014);

        
        for (int x = 0; x < 256; x++) {
            OAM[x] = nes->getCpuByte( (OAMDMA << 8) + x );
        }
    
        readToOAM = false;
    }

    if (scanline == 240) {
        ppuRegisters[2] &= 0x7F;
        if (ppuCycle == 340) {
            if (generateNMI) {
                *NMI = true;
            }
        }
    } else if (scanline > 240 && scanline < 261) {
        ppuRegisters[2] |= 0x80;
    } else {
        ppuRegisters[2] &= 0x7F;
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


    if (draw) {

        /* print everything */

        for (int i = 0; i < 32 * 30; i++) {

            int tileX;
            int tileY;

            tileX = (i % 32);
            tileY = (i / 32);

            int attributeX;
            int attributeY;

            attributeX = (tileX / 4);
            attributeY = (tileY / 4);

            int internalAttributeIndex;
            internalAttributeIndex = ((tileX % 4) / 2) + ((tileY % 4) / 2) * 2;



            int attributeTableIndex;
            attributeTableIndex = attributeX + attributeY * 8;
            uint8_t attributeByte;
            attributeByte = getPpuByte(0x2000 + nametableOffset + 0x3C0 + attributeTableIndex);

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
            pixelStart = tileX * 8 + tileY * NES_SCREEN_WIDTH * 8;

            int spriteStart;
            spriteStart = getPpuByte(0x2000 + i + nametableOffset);

            for (int j = 0; j < 8; j++) {

                uint32_t spriteRow2[8];

                std::bitset<8> spriteLayer1 = (std::bitset<8>) getPpuByte(spriteStart * 16 + j + 0x1000);
                std::bitset<8> spriteLayer2 = (std::bitset<8>) getPpuByte(spriteStart * 16 + j + 8 + 0x1000);

                for (int a = 0; a < 8; a++) {

                    uint8_t num = 0;

                    if (spriteLayer1[a]) {
                        num |= 0x1;
                    }
                    if (spriteLayer2[a]) {
                        num |= 0x2;
                    }

                    if (num == 0) {
                        spriteRow2[a] = paletteTableToARGB(getPpuByte(0x3F00));
                    } else if (num == 1) {

                        if (paletteIndex == 0) {
                            spriteRow2[a] = paletteTableToARGB(getPpuByte(0x3F01));
                        } else if (paletteIndex == 1) {
                            spriteRow2[a] = paletteTableToARGB(getPpuByte(0x3F05));
                        } else if (paletteIndex == 2) {
                            spriteRow2[a] = paletteTableToARGB(getPpuByte(0x3F09));
                        } else if (paletteIndex == 3) {
                            spriteRow2[a] = paletteTableToARGB(getPpuByte(0x3F0D));
                        }

                    } else if (num == 2) {

                        if (paletteIndex == 0) {
                            spriteRow2[a] = paletteTableToARGB(getPpuByte(0x3F02));
                        } else if (paletteIndex == 1) {
                            spriteRow2[a] = paletteTableToARGB(getPpuByte(0x3F06));
                        } else if (paletteIndex == 2) {
                            spriteRow2[a] = paletteTableToARGB(getPpuByte(0x3F0A));
                        } else if (paletteIndex == 3) {
                            spriteRow2[a] = paletteTableToARGB(getPpuByte(0x3F0E));
                        }

                    } else if (num == 3) {

                        if (paletteIndex == 0) {
                            spriteRow2[a] = paletteTableToARGB(getPpuByte(0x3F03));
                        } else if (paletteIndex == 1) {
                            spriteRow2[a] = paletteTableToARGB(getPpuByte(0x3F07));
                        } else if (paletteIndex == 2) {
                            spriteRow2[a] = paletteTableToARGB(getPpuByte(0x3F0B));
                        } else if (paletteIndex == 3) {
                            spriteRow2[a] = paletteTableToARGB(getPpuByte(0x3F00));
                        }

                    } else {
                        std::cout << "Error!" << std::endl;
                    }

                    /*

                    spriteRow2[a] = 0;
                    spriteRow2[a] += spriteLayer1[a];
                    spriteRow2[a] += spriteLayer2[a];

                    */

                }

                for (int k = 7; k >= 0; k--) {

                    pixels[pixelStart + (7 - k) + (j * NES_SCREEN_WIDTH)] = spriteRow2[k];

                    /*
                    if (spriteRow2[k] == 3) {
                        pixels[pixelStart + (7 - k) + (j * NES_SCREEN_WIDTH)] = 0xFFFFFFFF;
                    } else if (spriteRow2[k] == 2) {
                        pixels[pixelStart + (7 - k) + (j * NES_SCREEN_WIDTH)] = 0xFFF00000;
                    } else if (spriteRow2[k] == 1) {
                        pixels[pixelStart + (7 - k) + (j * NES_SCREEN_WIDTH)] = 0x000FFF00;
                    } else {
                        pixels[pixelStart + (7 - k) + (j * NES_SCREEN_WIDTH)] = 0x00000000;
                    }
                    */
                    

                }
            }
        }
    }


    return 1;
}
