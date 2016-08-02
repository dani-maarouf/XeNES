#include <iostream>
#include <fstream>

#include "NES.hpp"

NES::NES() {


    return;
}

void NES::closeCartridge() {
    nesCPU.freePointers();
    nesCPU.nesPPU.freePointers();
    return;
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

    //bool batteryRAM = false;
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

                //upper nybble of flag 6 is lower nybble of mapper number 
                mapperNumber = (flags[6] >> 4);
                mapperNumber |= (flags[7] & 0xF0);

                nesCPU.cpuMapper = mapperNumber;
                nesCPU.nesPPU.ppuMapper = mapperNumber;

                if (mapperNumber != 0 && mapperNumber != 1) {
                    std::cout << "Unrecognized mapper : " << (int) mapperNumber << std::endl;
                    std::cerr << "Rom size : " << (int) prg_rom_size << std::endl;
                    romFile.close();
                    return false;
                }

                prgRomBytes = prg_rom_size * 0x4000;

                if (chr_rom_size == 0) {
                    chrRomBytes = 0x2000;
                    nesCPU.nesPPU.usesRAM = true;
                } else {
                    chrRomBytes = chr_rom_size * 0x2000;    
                    nesCPU.nesPPU.usesRAM = false;
                }

                nesCPU.PRG_ROM = new uint8_t[prgRomBytes];

                if (nesCPU.PRG_ROM == NULL) {
                    romFile.close();
                    return false;
                }

                nesCPU.nesPPU.CHR_ROM = new uint8_t[chrRomBytes];

                if (nesCPU.nesPPU.CHR_ROM == NULL) {
                    delete [] nesCPU.PRG_ROM;
                    romFile.close();
                    return false;
                }

                //flags 6
                if ((flags[6] & 0x9) == 0x0) {
                    nesCPU.nesPPU.mirroring = HORIZONTAL;
                    //std::cout << "vertical arrangement, horizontal mirroring, CIRAM A10 = PPU A11" << std::endl;
                } else if ((flags[6] & 0x9) == 0x1) {
                    nesCPU.nesPPU.mirroring = VERTICAL;
                    //std::cout << "horizontal arrangement, vertical mirroring, CIRAM A10 = PPU A10" << std::endl;
                } else if ((flags[6] & 0x8) == 0x8) {
                    nesCPU.nesPPU.mirroring = FOUR_SCREEN;
                    std::cout << "four screen VRAM" << std::endl;
                    romFile.close();
                    return false;
                } else {
                    //std::cerr << "Could not identify mirroring for " << (std::bitset<8>) flags[6] << std::endl;
                    romFile.close();
                    return false;
                }
                if ((flags[6] & 0x2) == 0x2) {
                    //batteryRAM = true;
                }
                if ((flags[6] & 0x4) == 0x4) {
                    std::cout << "Trainer present" << std::endl;
                    trainer = true;
                    romFile.close();
                    return false;
                }

                

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
                    delete [] nesCPU.nesPPU.CHR_ROM;
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
                nesCPU.nesPPU.CHR_ROM[index - (16 + prgRomBytes)] = binaryValue;
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
    nesCPU.nesPPU.numRomBanks = chr_rom_size;

    nesCPU.PC = nesCPU.getCpuByte(0xFFFC, false) | (nesCPU.getCpuByte(0xFFFD, false) << 8);

    romFile.close();
    return true;
}
