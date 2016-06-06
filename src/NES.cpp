#include <iostream>
#include <fstream>
#include <bitset>

#include "NES.hpp"

NES::NES() {

    return;
}

NES::~NES() {

    delete nesAPU;
    delete nesPPU;
    delete nesCPU;

    return;
}

bool NES::openROM(const char * fileLoc) {

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

    bool batteryRAM = false;        //($6000-7FFF)
    bool trainer = false;           //trainer at $7000-$71FF

    bool NTSC;      //false = PAL

    int prgRomBytes, chrRomBytes, prgRamBytes;

    int index;
    index = 0;

    uint8_t * prgROM;
    uint8_t * chrROM;
    uint8_t * prgRAM;

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

                prgRomBytes = prg_rom_size * 0x4000;
                chrRomBytes = chr_rom_size * 0x2000;

                if (prg_ram_size == 0) {
                    prgRamBytes = 0x2000;
                } else {
                    prgRamBytes = (prg_ram_size * 0x2000);
                }

                prgROM = new uint8_t[prgRomBytes];

                if (prgROM == NULL) {
                    return false;
                }

                chrROM = new uint8_t[chrRomBytes];
                if (chrROM == NULL) {
                    delete [] prgROM;
                    return false;
                }

                prgRAM = new uint8_t[prgRamBytes];
                if (prgRAM == NULL) {
                    delete [] chrROM;
                    delete [] prgROM;
                    return false;
                }

                for (int a = 0; a < prgRamBytes; a++) {
                    prgRAM[a] = 0x0;
                }

                //upper nybble of flag 6 is lower nybble of mapper number 
                mapperNumber = (flags[6] >> 4);

                //flags 6
                if ((flags[6] & 0x9) == 0x0) {
                    std::cout << "vertical arrangement, horizontal mirroring, CIRAM A10 = PPU A11" << std::endl;
                } else if ((flags[6] & 0x9) == 0x1) {
                    std::cout << "horizontal arrangement, vertical mirroring, CIRAM A10 = PPU A10" << std::endl;
                } else if ((flags[6] & 0x8) == 0x8) {
                    std::cout << "four screen VRAM" << std::endl;
                } else {
                    std::cerr << "Could not identify mirroring for " << (std::bitset<8>) flags[6] << std::endl;
                    romFile.close();
                    return false;
                }
                if ((flags[6] & 0x2) == 0x2) {
                    batteryRAM = true;
                }
                if ((flags[6] & 0x4) == 0x4) {
                    std::cout << "Trainer present" << std::endl;
                    trainer = true;
                }

                mapperNumber |= (flags[7] & 0xF0);

                if ((flags[7] & 0xC) == 0x8) {
                    std::cout << "NES 2.0 format" << std::endl;
                }
                if ((flags[7] & 0x1) == 0x1) {
                    std::cout << "VS Unisystem" << std::endl;
                }
                if ((flags[7] & 0x2) == 0x2) {
                    std::cout << "Playchoice-10" << std::endl;
                }

                prg_ram_size = flags[8];

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
                prgROM[index - 16] = binaryValue;
            } else if (index < 16 + prgRomBytes + chrRomBytes) {
                chrROM[index - (16 + prgRomBytes)] = binaryValue;
            }
            
            index++;
        }
    }

    uint8_t * ppuRegs, * ioRegs;
    ppuRegs = new uint8_t[8];

    if (ppuRegs == NULL) {
        delete [] prgROM;
        delete [] chrROM;
        delete [] prgRAM;
        return false;
    }

    ioRegs = new uint8_t[0x20];

    if (ioRegs == NULL) {
        delete [] prgROM;
        delete [] chrROM;
        delete [] ppuRegs;
        delete [] prgRAM;
        return false;
    }

    nesCPU = new CPU(prgROM, prgRAM, (int) prg_rom_size, (int) prg_ram_size, ppuRegs, ioRegs);

    if (nesCPU == NULL) {
        delete [] prgROM;
        delete [] chrROM;
        delete [] ppuRegs;
        delete [] ioRegs;
        delete [] prgRAM;
        return false;
    }

    nesPPU = new PPU(chrROM, ppuRegs);

    if (nesPPU == NULL) {
        delete [] prgROM;
        delete [] chrROM;
        delete [] ppuRegs;
        delete [] ioRegs;
        delete [] prgRAM;
        delete nesCPU;
        return false;
    }

    nesAPU = new APU(ioRegs);

    if (nesAPU == NULL) {
        delete [] prgROM;
        delete [] chrROM;
        delete [] ppuRegs;
        delete [] ioRegs;
        delete [] prgRAM;
        delete nesCPU;
        delete nesAPU;
        return false;
    }

    romFile.close();
    return true;
}
