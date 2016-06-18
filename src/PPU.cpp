#include <iostream>
#include <bitset>

#include "NES.hpp"

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
    
    ppuReadAddress = 0x0;

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
                    pixels[pixelStart + (7 - k) + (j * NES_SCREEN_WIDTH)] = 0xFFFFFFFF;
                } else if (spriteRow2[k] == 2) {
                    pixels[pixelStart + (7 - k) + (j * NES_SCREEN_WIDTH)] = 0xFFF00000;
                } else if (spriteRow2[k] == 1) {
                    pixels[pixelStart + (7 - k) + (j * NES_SCREEN_WIDTH)] = 0x000FFF00;
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
            ppuReadAddress |= ppuRegisters[6];
            //std::cout << "PPU VRAM addr : " << std::hex << ppuReadAddress << std::endl;
            readLower = false;
        } else {
            ppuReadAddress = (ppuRegisters[6] << 8);
            readLower = true;
        }
        ppuGetAddr = false;
    }

    if (readToRAM) {
        setPpuByte(ppuReadAddress, ppuRegisters[7]);

        ppuReadAddress += vramInc;

        //std::cout << "PPU VRAM addr : " << std::hex << ppuReadAddress << std::endl;

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

            int x;
            int y;

            x = (i % 32);
            y = (i / 32);

            int pixelStart;
            pixelStart = x * 8 + y * NES_SCREEN_WIDTH * 8;

            int spriteStart;
            spriteStart = getPpuByte(0x2000 + i + nametableOffset);

            for (int j = 0; j < 8; j++) {

                int spriteRow2[8];

                std::bitset<8> spriteLayer1 = (std::bitset<8>) getPpuByte(spriteStart * 16 + j + 0x1000);
                std::bitset<8> spriteLayer2 = (std::bitset<8>) getPpuByte(spriteStart * 16 + j + 8 + 0x1000);

                for (int a = 0; a < 8; a++) {
                    spriteRow2[a] = 0;
                    spriteRow2[a] += spriteLayer1[a];
                    spriteRow2[a] += spriteLayer2[a];
                }

                for (int k = 7; k >= 0; k--) {

                    if (spriteRow2[k] == 3) {
                        pixels[pixelStart + (7 - k) + (j * NES_SCREEN_WIDTH)] = 0xFFFFFFFF;
                    } else if (spriteRow2[k] == 2) {
                        pixels[pixelStart + (7 - k) + (j * NES_SCREEN_WIDTH)] = 0xFFF00000;
                    } else if (spriteRow2[k] == 1) {
                        pixels[pixelStart + (7 - k) + (j * NES_SCREEN_WIDTH)] = 0x000FFF00;
                    } else {
                        pixels[pixelStart + (7 - k) + (j * NES_SCREEN_WIDTH)] = 0x00000000;
                    }
                    

                }
            }
        }
    }


    return 1;
}
