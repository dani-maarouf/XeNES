#include <iostream>
#include <cstring>
#include <fstream>
#include <bitset>

#include "NES.hpp"

NES::NES() {
    for (int x = 0; x < 0x20; x++) ioRegisters[x] = 0x0;
    return;
}

void NES::closeCartridge() {
    nesCPU.freePointers();
    nesPPU.freePointers();
    return;
}

uint32_t * NES::getDisplayPixels() {
    return nesPPU.pixels;
}

int NES::executeOpcode(bool debug) {
    return nesCPU.executeNextOpcode(this, false);
}

int NES::tickPPU() {
    nesPPU.tick(&nesCPU.NMI);
    return 1;
}

bool NES::drawFlag() {
    return nesPPU.draw;
}

//fix possible memory leaks
bool NES::openCartridge(const char * fileLoc) {

    if (fileLoc == NULL) {
        return false;
    }

    std::ifstream romFile;
    romFile.open(fileLoc, std::ios::binary);

    if (!romFile.is_open()) {
        return false;
    }

    uint8_t flags[16];

    uint8_t prg_rom_size = 0;   //number of 16KB ROM blocks
    uint8_t chr_rom_size = 0;   //number of 8KB VROM blocks
    uint8_t prg_ram_size = 0;   //number of 8KB RAM blocks

    uint8_t mapperNumber;

    bool batteryRAM = false;
    bool trainer = false;

    bool NTSC;      //false = PAL
    NTSC = false;

    int prgRomBytes = 0;
    int chrRomBytes = 0;
    int prgRamBytes = 0;

    int index;
    index = 0;

    while (romFile) {
        char c;
        romFile.get(c);
        unsigned char u = (unsigned char) c;
        int binaryValue = (int) u;

        if (index < 16) {
            flags[index] = binaryValue;
            index++;
        } else {
            if (index == 16) {
                if (flags[0] != 0x4E || flags[1] != 0x45 || flags[2] != 0x53 || flags[3] != 0x1A) {
                    std::cerr << "iNes file format not recognized!" << std::endl;
                    romFile.close();
                    return false;
                }
                prg_rom_size = flags[4];
                chr_rom_size = flags[5];

                if (prg_rom_size != 1 && prg_rom_size != 2) {
                    std::cerr << "Unrecognized rom size " << (int) prg_rom_size << std::endl;
                    romFile.close();
                    return false;
                }

                prgRomBytes = prg_rom_size * 0x4000;

                if (chr_rom_size == 0) {
                    chrRomBytes = 0x2000;
                    nesPPU.usesRAM = true;
                } else {
                    chrRomBytes = chr_rom_size * 0x2000;    
                    nesPPU.usesRAM = false;
                }

                nesCPU.PRG_ROM = new uint8_t[prgRomBytes];

                if (nesCPU.PRG_ROM == NULL) {
                    romFile.close();
                    return false;
                }

                nesPPU.CHR_ROM = new uint8_t[chrRomBytes];

                if (nesPPU.CHR_ROM == NULL) {
                    delete [] nesCPU.PRG_ROM;
                    romFile.close();
                    return false;
                }

                //upper nybble of flag 6 is lower nybble of mapper number 
                mapperNumber = (flags[6] >> 4);

                //flags 6
                if ((flags[6] & 0x9) == 0x0) {
                    nesPPU.mirroring = HORIZONTAL;
                    //std::cout << "vertical arrangement, horizontal mirroring, CIRAM A10 = PPU A11" << std::endl;
                } else if ((flags[6] & 0x9) == 0x1) {
                    nesPPU.mirroring = VERTICAL;
                    //std::cout << "horizontal arrangement, vertical mirroring, CIRAM A10 = PPU A10" << std::endl;
                } else if ((flags[6] & 0x8) == 0x8) {
                    nesPPU.mirroring = FOUR_SCREEN;
                    std::cout << "four screen VRAM" << std::endl;
                    romFile.close();
                    return false;
                } else {
                    //std::cerr << "Could not identify mirroring for " << (std::bitset<8>) flags[6] << std::endl;
                    romFile.close();
                    return false;
                }
                if ((flags[6] & 0x2) == 0x2) {
                    batteryRAM = true;
                }
                if ((flags[6] & 0x4) == 0x4) {
                    std::cout << "Trainer present" << std::endl;
                    trainer = true;
                    romFile.close();
                    return false;
                }

                mapperNumber |= (flags[7] & 0xF0);

                if ((flags[7] & 0xC) == 0x8) {
                    std::cout << "NES 2.0 format" << std::endl;
                    romFile.close();
                    return false;
                }
                if ((flags[7] & 0x1) == 0x1) {
                    std::cout << "VS Unisystem" << std::endl;
                    romFile.close();
                    return false;
                }
                if ((flags[7] & 0x2) == 0x2) {
                    std::cout << "Playchoice-10" << std::endl;
                    romFile.close();
                    return false;
                }

                prg_ram_size = flags[8];

                if (prg_ram_size == 0) {
                    prgRamBytes = 0x2000;
                } else {
                    prgRamBytes = (prg_ram_size * 0x2000);
                }

                nesCPU.PRG_RAM = new uint8_t[prgRamBytes];

                if (nesCPU.PRG_RAM == NULL) {
                    delete [] nesPPU.CHR_ROM;
                    delete [] nesCPU.PRG_ROM;
                    romFile.close();
                    return false;
                }

                for (int a = 0; a < prgRamBytes; a++) {
                    nesCPU.PRG_RAM[a] = 0x0;
                }


                if ((flags[9] & 0x1) == 0x1) {
                    NTSC = false;
                } else {
                    NTSC = true;
                }
            }

            if (trainer) {
                romFile.close();
                return false;
            }

            if (index < 16 + prgRomBytes) {
                nesCPU.PRG_ROM[index - 16] = binaryValue;
            } else if (index < 16 + prgRomBytes + chrRomBytes) {
                nesPPU.CHR_ROM[index - (16 + prgRomBytes)] = binaryValue;
            }
            
            index++;
        }
    }

    if (NTSC == false) {
        std::cerr << "PAL detected. Quitting" << std::endl;
        romFile.close();
        return false;
    }

    nesCPU.numRomBanks = prg_rom_size;

    nesCPU.PC = getCpuByte(0xFFFC) | (getCpuByte(0xFFFD) << 8);

    romFile.close();
    return true;
}

uint8_t NES::getCpuByte(uint16_t memAddress) {

    if (memAddress >= 0x0000 && memAddress < 0x2000) {
        return nesCPU.cpuRAM[memAddress % 0x800];
    } else if (memAddress >= 0x8000 && memAddress < 0x10000 && nesCPU.numRomBanks == 1) {
        return nesCPU.PRG_ROM[ (memAddress - 0x8000) % 0x4000];
    } else if (memAddress >= 0x8000 && memAddress < 0x10000 && nesCPU.numRomBanks == 2) {
        return nesCPU.PRG_ROM[memAddress - 0x8000];
    } else if (memAddress >= 0x2000 && memAddress < 0x4000) {
        return nesPPU.ppuRegisters[ (memAddress - 0x2000) % 8 ];
    } else if (memAddress >= 0x4000 && memAddress < 0x4020) {
        return ioRegisters[ memAddress - 0x4000 ];
    } else if (memAddress >= 0x6000 && memAddress < 0x8000) {
        return nesCPU.PRG_RAM[memAddress - 0x6000];
    } else {
        return nesCPU.cpuMem[memAddress];
    }
}

bool NES::setCpuByte(uint16_t memAddress, uint8_t byte) {

    if (memAddress >= 0x0000 && memAddress < 0x2000) {
        nesCPU.cpuRAM[memAddress] = byte;
        return true;
    } else if (memAddress >= 0x8000 && memAddress < 0x10000) {

        std::cerr << "Segmentation fault! Can't write to 0x" << std::hex << memAddress << std::endl;
        return false;
        
    } else if (memAddress >= 0x2000 && memAddress < 0x4000) {

        uint16_t address;
        address = (memAddress - 0x2000) % 8;

        if (address == 0x6) {
            nesPPU.ppuGetAddr = true;
        } else if (address == 0x7) {
            nesPPU.ppuReadByte = true;
        }

        nesPPU.ppuRegisters[address] = byte;
        return true;
    } 

    else if (memAddress >= 0x4000 && memAddress < 0x4018) { 
        ioRegisters[memAddress - 0x4000] = byte;
        return true;
    } else if (memAddress >= 0x6000 && memAddress < 0x8000) {
        nesCPU.PRG_RAM[memAddress - 0x6000] = byte;
        return true;
    } else {
        nesCPU.cpuMem[memAddress] = byte;
        return true;
    }
}

uint16_t NES::retrieveCpuAddress(enum AddressMode mode, bool * pagePass, uint8_t firstByte, uint8_t secondByte) {

    *pagePass = false;

    switch (mode) {

        case ZRP:
        return firstByte;

        case ZRPX:
        return ((firstByte + nesCPU.X) & 0xFF);

        case ZRPY:
        return ((firstByte + nesCPU.Y) & 0xFF);

        case ABS:
        return (firstByte | (secondByte << 8));

        case ABSX:

        if (((firstByte | (secondByte << 8)) / 256) != ((((firstByte | (secondByte << 8)) + nesCPU.X) & 0xFFFF)/256)) {
            *pagePass = true;
        }

        return (((firstByte | (secondByte << 8)) + nesCPU.X) & 0xFFFF);

        case ABSY:

        if (((firstByte | (secondByte << 8)) / 256) != ((((firstByte | (secondByte << 8)) + nesCPU.Y) & 0xFFFF)/256)) {
            *pagePass = true;
        }

        return (((firstByte | (secondByte << 8)) + nesCPU.Y) & 0xFFFF);

        case IND: {
            uint16_t low, high;
            low = getCpuByte((firstByte | (secondByte << 8)));
            high = getCpuByte(((firstByte + 1) & 0xFF) | (secondByte << 8));
            return ((high << 8) | low);
        }

        case INDX: {
            uint8_t low, high;
            low = getCpuByte((firstByte + nesCPU.X) & 0xFF);
            high = getCpuByte((firstByte + 1 + nesCPU.X) & 0xFF);
            return ((high << 8) | low);
        }
        
        case INDY: 

        if ((((getCpuByte(firstByte)) | (getCpuByte((firstByte + 1) & 0xFF)) << 8) / 256) != (((((getCpuByte(firstByte)) | (getCpuByte((firstByte + 1) & 0xFF)) << 8) + nesCPU.Y) & 0xFFFF)/256)) {
            *pagePass = true;
        }

        return ((((getCpuByte(firstByte)) | (getCpuByte((firstByte + 1) & 0xFF)) << 8) + nesCPU.Y) & 0xFFFF);
        
        default:
        return 0;
    }
}
