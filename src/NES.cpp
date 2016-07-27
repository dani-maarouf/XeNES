#include <iostream>
#include <cstring>
#include <fstream>
#include <bitset>

#include "NES.hpp"

#define getBit(num, bit)    (bit == 0) ? (num & 0x1) : (bit == 1) ? (num & 0x2) : \
    (bit == 2) ? (num & 0x4) : (bit == 3) ? (num & 0x8) :   \
    (bit == 4) ? (num & 0x10) : (bit == 5) ? (num & 0x20) : \
    (bit == 6) ? (num & 0x40) : (num & 0x80)

NES::NES() {

    for (int x = 0; x < 0x20; x++) ioRegisters[x] = 0;

    controllerByte = 0;
    storedControllerByte = 0;
    currentControllerBit = 0;
    readController = false;

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
    return nesCPU.executeNextOpcode(this, debug);
}

void NES::tickPPU(int numTicks) {
    nesPPU.tick(this, numTicks);
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

                //upper nybble of flag 6 is lower nybble of mapper number 
                mapperNumber = (flags[6] >> 4);
                mapperNumber |= (flags[7] & 0xF0);


                if (prg_rom_size != 1 && prg_rom_size != 2) {
                    std::cerr << "Unrecognized rom size " << (int) prg_rom_size << std::endl;
                    std::cout << "Mapper " << (int) mapperNumber << std::endl;
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

    for (int x = 0; x < 0x20; x++) ioRegisters[x] = 0x0;

    romFile.close();
    return true;
}

uint8_t NES::getCpuByte(uint16_t memAddress) {

    if (memAddress >= 0x0000 && memAddress < 0x2000) {
        return nesCPU.RAM[memAddress % 0x800];
    } else if (memAddress >= 0x8000 && memAddress <= 0xFFFF && nesCPU.numRomBanks == 1) {
        return nesCPU.PRG_ROM[ (memAddress - 0x8000) % 0x4000];
    } else if (memAddress >= 0x8000 && memAddress <= 0xFFFF && nesCPU.numRomBanks == 2) {
        return nesCPU.PRG_ROM[memAddress - 0x8000];
    } else if (memAddress >= 0x2000 && memAddress < 0x4000) {

        uint16_t address;
        address = (memAddress - 0x2000) % 8;


        if (address == 0x2) {

            nesPPU.addressLatch = false;
            //nesPPU.ppuRegisters[0x2] &= 0x7F;

        } else if (address == 0x7) {

            
            //if screen off
            if ((nesPPU.ppuRegisters[0x2] & 0x80) || ((nesPPU.ppuRegisters[0x1] & 0x18) == 0)) {
                if (nesPPU.m_t % 0x4000 < 0x3F00) {

                    uint8_t ppuByte;
                    ppuByte = nesPPU.readBuffer;
                    nesPPU.readBuffer = nesPPU.getPpuByte( nesPPU.m_t );


                    nesPPU.m_t += (nesPPU.ppuRegisters[0] & 0x04) ? 32 : 1;

                    return ppuByte;
                } else {

                    uint8_t ppuByte;

                    ppuByte = nesPPU.getPpuByte( nesPPU.m_t );

                    nesPPU.readBuffer = nesPPU.getPpuByte( nesPPU.m_t - 0x1000);

                    nesPPU.m_t += (nesPPU.ppuRegisters[0] & 0x04) ? 32 : 1;

                    return ppuByte;

                }
            }
            
        }

        nesPPU.registerReadFlags[address] = true;
        nesPPU.flagSet = true;
        return nesPPU.ppuRegisters[address];

    } else if (memAddress >= 0x4000 && memAddress < 0x4020) {

        if (memAddress == 0x4016) {
            if (readController) {
                if (currentControllerBit < 8) {
                    currentControllerBit++;
                    bool bit;
                    bit = getBit(storedControllerByte, currentControllerBit - 1);
                    return bit;
                } else {
                    return 1;
                }
            }
        }

        return ioRegisters[ memAddress - 0x4000 ];

    } else if (memAddress >= 0x6000 && memAddress < 0x8000) {
        return nesCPU.PRG_RAM[memAddress - 0x6000];
    } else {
        return nesCPU.cpuMem[memAddress];
    }
}

void NES::setCpuByte(uint16_t memAddress, uint8_t byte) {

    if (memAddress >= 0x0000 && memAddress < 0x2000) {
        nesCPU.RAM[memAddress] = byte;
    } else if (memAddress >= 0x8000 && memAddress <= 0xFFFF) {
        std::cerr << "Segmentation fault! Can't write to 0x" << std::hex << memAddress << std::endl;
    } else if (memAddress >= 0x2000 && memAddress < 0x4000) {
        uint16_t address;
        address = (memAddress - 0x2000) % 8;
        nesPPU.registerWriteFlags[address] = true;
        nesPPU.flagSet = true;
        nesPPU.ppuRegisters[address] = byte;
    } else if (memAddress >= 0x4000 && memAddress < 0x4018) { 
        if (memAddress == 0x4014) {
            nesPPU.registerWriteFlags[8] = true;
            nesPPU.flagSet = true;
        } else if (memAddress == 0x4016) {
            if ((byte & 0x1) == 0x1) {
                storedControllerByte = controllerByte;
                readController = false;
            } else {
                currentControllerBit = 0;
                readController = true;
            }
        }
        ioRegisters[memAddress - 0x4000] = byte;
    } else if (memAddress >= 0x6000 && memAddress < 0x8000) {
        nesCPU.PRG_RAM[memAddress - 0x6000] = byte;
    } else {
        nesCPU.cpuMem[memAddress] = byte;
    }

    return;
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
