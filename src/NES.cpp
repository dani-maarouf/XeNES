#include <iostream>
#include <fstream>
#include "NES.hpp"

NES::NES() {

    m_nesCPU = CPU();

    return;
}

void NES::close_cartridge() {
    m_nesCPU.free_pointers();
    m_nesCPU.m_nesPPU.free_pointers();
    return;
}

//fix possible memory leaks
bool NES::open_cartridge(const char * fileLoc) {

    if (fileLoc == NULL) {
        std::cerr << "Error: File location is NULL" << std::endl;
        return false;
    }

    std::ifstream romFile;
    romFile.open(fileLoc, std::ios::binary);

    if (!romFile.is_open()) {
        std::cerr << "Error: File " << fileLoc << " can not be accessed or does not exist" << std::endl;
        return false;
    }

    uint8_t flags[16];
    int prg_rom_size = 0;   //number of 16KB ROM blocks
    int chr_rom_size = 0;   //number of 8KB VROM blocks
    int prg_ram_size = 0;   //number of 8KB RAM blocks
    int prgRomBytes = 0;    //size of program rom in bytes
    int chrRomBytes = 0;    //size of vrom in bytes
    int prgRamBytes = 0;    //size of program ram in bytes
    int mapperNumber;       //mapper


    int index = 0;          //loop variable for reading file

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
                    std::cerr << "Error: iNes file format not recognized!" << std::endl;
                    romFile.close();
                    return false;
                }
                prg_rom_size = flags[4];
                chr_rom_size = flags[5];

                //upper nybble of flag 6 is lower nybble of mapper number 
                mapperNumber = (flags[6] >> 4);
                mapperNumber |= (flags[7] & 0xF0);

                m_nesCPU.m_cpuMapper = mapperNumber;
                m_nesCPU.m_nesPPU.m_ppuMapper = mapperNumber;

                if (mapperNumber != 0) {
                    std::cerr << "Unrecognized mapper : " << (int) mapperNumber << std::endl;
                    std::cerr << "Rom size : " << (int) prg_rom_size << std::endl;
                    romFile.close();
                    return false;
                }

                prgRomBytes = prg_rom_size * 0x4000;

                if (chr_rom_size == 0) {
                    chrRomBytes = 0x2000;
                    m_nesCPU.m_nesPPU.m_usesRAM = true;
                } else {
                    chrRomBytes = chr_rom_size * 0x2000;    
                    m_nesCPU.m_nesPPU.m_usesRAM = false;
                }

                m_nesCPU.m_PRG_ROM = new uint8_t[prgRomBytes];

                if (m_nesCPU.m_PRG_ROM == NULL) {
                    romFile.close();
                    return false;
                }

                m_nesCPU.m_nesPPU.m_CHR_ROM = new uint8_t[chrRomBytes];

                if (m_nesCPU.m_nesPPU.m_CHR_ROM == NULL) {
                    delete [] m_nesCPU.m_PRG_ROM;
                    romFile.close();
                    return false;
                }

                //flags 6
                if ((flags[6] & 0x9) == 0x0) {
                    m_nesCPU.m_nesPPU.m_mirroring = HORIZONTAL;
                    //std::cout << "vertical arrangement, horizontal m_mirroring, CIRAM A10 = PPU A11" << std::endl;
                } else if ((flags[6] & 0x9) == 0x1) {
                    m_nesCPU.m_nesPPU.m_mirroring = VERTICAL;
                    //std::cout << "horizontal arrangement, vertical m_mirroring, CIRAM A10 = PPU A10" << std::endl;
                } else if ((flags[6] & 0x8) == 0x8) {
                    m_nesCPU.m_nesPPU.m_mirroring = FOUR_SCREEN;
                    std::cout << "four screen VRAM" << std::endl;
                    romFile.close();
                    return false;
                } else {
                    //std::cerr << "Could not identify m_mirroring for " << (std::bitset<8>) flags[6] << std::endl;
                    romFile.close();
                    return false;
                }
                if ((flags[6] & 0x2) == 0x2) {
                    //batteryRAM = true;
                }
                if ((flags[6] & 0x4)) {
                    std::cout << "Trainer present" << std::endl;
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
                    std::cout << "Playchoice-10 detected. Quitting." << std::endl;
                    romFile.close();
                    return false;
                }

                prg_ram_size = flags[8];

                if (prg_ram_size == 0) {
                    prgRamBytes = 0x2000;
                } else {
                    prgRamBytes = (prg_ram_size * 0x2000);
                }

                m_nesCPU.m_PRG_RAM = new uint8_t[prgRamBytes];

                if (m_nesCPU.m_PRG_RAM == NULL) {
                    delete [] m_nesCPU.m_nesPPU.m_CHR_ROM;
                    delete [] m_nesCPU.m_PRG_ROM;
                    romFile.close();
                    return false;
                }

                for (int a = 0; a < prgRamBytes; a++) {
                    m_nesCPU.m_PRG_RAM[a] = 0x0;
                }


                if ((flags[9] & 0x1)) {
                    std::cerr << "PAL ROM detected. Only NTSC supported. Quitting" << std::endl;
                    romFile.close();
                    return false;

                }
            }

            if (index < 16 + prgRomBytes) {
                m_nesCPU.m_PRG_ROM[index - 16] = binaryValue;
            } else if (index < 16 + prgRomBytes + chrRomBytes) {
                m_nesCPU.m_nesPPU.m_CHR_ROM[index - (16 + prgRomBytes)] = binaryValue;
            }
            
            index++;
        }
    }

    m_nesCPU.m_nesAPU.m_audioBufferSize = sizeof(int16_t) * 2 * 800;
    m_nesCPU.m_nesAPU.m_audioBuffer = (int16_t *) malloc(sizeof(int16_t) * 2 * 800);


    m_nesCPU.m_numRomBanks = prg_rom_size;
    m_nesCPU.m_nesPPU.m_numRomBanks = chr_rom_size;

    m_nesCPU.m_PC = m_nesCPU.get_cpu_byte(0xFFFC, false) | (m_nesCPU.get_cpu_byte(0xFFFD, false) << 8);
    
    romFile.close();
    return true;
}
