#include <iostream>
#include <bitset>

#include "NES.hpp"

#define MAX_INT 4294967295;

uint8_t NES::getPpuByte(uint16_t address) {
    address %= 0x4000;

    if (address >= 0x3000 && address < 0x3F00) {
        address -= 0x1000;      //0x3000-0x3EFF map to 0x2000-0x2EFF
    }

    if (address >= 0x0 && address < 0x2000) {
        return CHR_ROM[address];
    } else {

        if (mirroring == HORIZONTAL) {

            if (address >= 0x2000 && address < 0x2400) {
                return ppuRAM[address - 0x2000];
            } else if (address >= 0x2400 && address < 0x2800) {
                return ppuRAM[address - 0x2400];
            } else if (address >= 0x2800 && address < 0x2C00) {
                return ppuRAM[address - 0x2400];
            } else if (address >= 0x2C00 && address < 0x3000) {
                return ppuRAM[address - 0x2800];
            }

        } else if (mirroring == VERTICAL) {

            if (address >= 0x2000 && address < 0x2400) {
                return ppuRAM[address - 0x2000];
            } else if (address >= 0x2400 && address < 0x2800) {
                return ppuRAM[address - 0x2000];
            } else if (address >= 0x2800 && address < 0x2C00) {
                return ppuRAM[address - 0x2800];
            } else if (address >= 0x2C00 && address < 0x3000) {
                return ppuRAM[address - 0x2800];
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

bool NES::setPpuByte(uint16_t address, uint8_t byte) {
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
                ppuRAM[address - 0x2000] = byte;
                return true;
            } else if (address >= 0x2400 && address < 0x2800) {
                ppuRAM[address - 0x2400] = byte;
                return true;
            } else if (address >= 0x2800 && address < 0x2C00) {
                ppuRAM[address - 0x2400] = byte;
                return true;
            } else if (address >= 0x2C00 && address < 0x3000) {
                ppuRAM[address - 0x2800] = byte;
                return true;
            }

        } else if (mirroring == VERTICAL) {

            if (address >= 0x2000 && address < 0x2400) {
                ppuRAM[address - 0x2000] = byte;
                return true;
            } else if (address >= 0x2400 && address < 0x2800) {
                ppuRAM[address - 0x2000] = byte;
                return true;
            } else if (address >= 0x2800 && address < 0x2C00) {
                ppuRAM[address - 0x2800] = byte;
                return true;
            } else if (address >= 0x2C00 && address < 0x3000) {
                ppuRAM[address - 0x2800] = byte;
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

void NES::printSprites() {

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

void NES::drawSprites() {

    int count;
    count = 0;

    for (int i = 0x0; i < 0x2000; i += 0x10) {

        for (int x = i; x < 0x8 + i; x++) {

            std::bitset<8> row;
            row = (std::bitset<8>) (getPpuByte(x) | getPpuByte(x + 8));

            for (int y = 7; y >= 0; y--) {
                if (row[y]) {
                    pixels[0 + (count % 32) * 8 + (count / 32) * 8 * NES_SCREEN_WIDTH + (7 - y) + (x - i) * NES_SCREEN_WIDTH] = MAX_INT;
                } else {
                    pixels[0 + (count % 32) * 8 + (count / 32) * 8 * NES_SCREEN_WIDTH + (7 - y) + (x - i) * NES_SCREEN_WIDTH] = 0;
                }
            }
        }
        count++;
    }
}

void NES::ppuTick() {


    if (scanline > 240 && scanline < 261) {
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

        for (int i = 0; i < 32 * 30; i++) {

            int x;
            int y;

            x = (i % 32);
            y = (i / 32);

            int pixelStart;
            pixelStart = x * 8 + y * NES_SCREEN_WIDTH * 8;

            int spriteStart;
            spriteStart = (int) getPpuByte(0x2000 + i);


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
                         pixels[pixelStart + (7 - k) + (j * NES_SCREEN_WIDTH)] = 0x0;
                    }
                    

                }



                /*

                std::bitset<8> spriteRow;
                spriteRow = (std::bitset<8>) (getPpuByte(spriteStart * 16 + j + 0x1000) | (getPpuByte(spriteStart * 16 + j + 8 + 0x1000)));


                for (int k = 7; k >= 0; k--) {


                    
                    if (spriteRow[k]) {
                        pixels[pixelStart + (7 - k) + (j * NES_SCREEN_WIDTH)] = MAX_INT;
                    } else {
                        pixels[pixelStart + (7 - k) + (j * NES_SCREEN_WIDTH)] = 0;

                    }
                    

                }

                */




            }


        }

    }


    return;
}

int NES::getPpuCycle() {
    return ppuCycle;
}
