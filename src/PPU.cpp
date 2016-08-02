#include <iostream>

#include "mappers.hpp"
#include "PPU.hpp"

const bool DEBUG = false;


/* Begin macro functions */
#define getBit(num, bit)    (bit == 0) ? (num & 0x1) : (bit == 1) ? (num & 0x2) : \
    (bit == 2) ? (num & 0x4) : (bit == 3) ? (num & 0x8) :   \
    (bit == 4) ? (num & 0x10) : (bit == 5) ? (num & 0x20) : \
    (bit == 6) ? (num & 0x40) : (num & 0x80)

#define horizontalIncrement(loopyv) if (ppuRegisters[1] & 0x18) {\
    if ((loopyv & 0x001F) == 31) {      \
        loopyv &= ~0x001F;              \
        loopyv ^= 0x400;                \
    } else {                            \
        loopyv += 1;                    \
    }                                   \
}


#define verticalIncrement(loopyv) if (ppuRegisters[1] & 0x18) {\
    if ((loopyv & 0x7000) != 0x7000) {  \
        loopyv += 0x1000;                               \
    } else {                                            \
        loopyv &= ~0x7000;                              \
        int yVal;                                       \
        yVal = (loopyv & 0x03E0) >> 5;                  \
        if (yVal == 29) {                               \
            yVal = 0;                                   \
            loopyv ^= 0x800;                            \
        } else if (yVal == 31) {                        \
            yVal = 0;                                   \
        } else {                                        \
            yVal += 1;                                  \
        }                                               \
        loopyv = ((loopyv & ~0x03E0) | (yVal << 5));    \
    }                                                   \
}

#define copyHorizontalBits(loopyv, loopyt) if (ppuRegisters[1] & 0x18) {\
loopyv &= ~0x041F;  \
loopyv |= (loopyt & 0x041F);   \
}


#define copyVerticalBits(loopyv, loopyt) if (ppuRegisters[1] & 0x18) {\
loopyv &= 0x041F;  \
                loopyv |= (loopyt & (~0x041F)); \
            }
/* End macro functions */

/* Begin PPU class functions */
PPU::PPU() {

    for (int x = 0; x < 0x20; x++) palette[x] = 0;
    for (int x = 0; x < 0x8; x++) ppuRegisters[x] = 0x0;
    for (int x = 0; x < 0x800; x++) VRAM[x] = 0x0;
    for (int x = 0; x < 0x100; x++) OAM[x] = 0x0;
    for (int x = 0; x < (NES_SCREEN_WIDTH * NES_SCREEN_HEIGHT); x++) pixels[x] = 63;    //black in palette
    for (int x = 0; x < 6 * 8; x++) lineOAM[x] = 0x0;
    
    evenFrame = true;
    draw = false;
    spriteZeroFlag = false;
    addressLatch = false;
    m_v = 0;
    m_t = 0;
    m_x = 0;
    oamAddress = 0x0;
    CHR_ROM = NULL;
    readBuffer = 0;

    //render
    m_SpriteOld1 = 0;
    m_SpriteOld2 = 0;
    m_PaletteOld = 0;
    m_SpriteNew1 = 0;
    m_SpriteNew2 = 0;
    m_PaletteNew = 0;


    suppressVBL = false;

    //ppuClock = 241 * 341;   //start at scanline 241
    ppuClock = 0;

    writeFlag = -1;
    readFlag = -1;

    return;
}

void PPU::freePointers() {
    if (CHR_ROM != NULL) {
        delete [] CHR_ROM;
    }
    return;
}

inline void PPU::loadNewTile() {

    m_SpriteOld1 = m_SpriteNew1;
    m_SpriteOld2 = m_SpriteNew2;
    m_PaletteOld = m_PaletteNew;

    m_PaletteNew = ((getPpuByte(0x23C0 | (m_v & 0x0C00) | ((m_v >> 4) & 0x38) | ((m_v >> 2) & 0x07))) >> (2 * ((((m_v & 0x1F) % 4) / 2) + ((((m_v & 0x370) >> 5) % 4) / 2) * 2))) & 0x3;

    int spriteStart = getPpuByte(0x2000 | (m_v & 0x0FFF));
    int spriteAddress = spriteStart * 16 + ((m_v & 0x7000) >> 12) + ((ppuRegisters[0] & 0x10) ? 0x1000 : 0x0);

    //load sprite data
    m_SpriteNew1 = getPpuByte(spriteAddress);
    m_SpriteNew2 = getPpuByte(spriteAddress + 8);

    return;
}

inline uint8_t PPU::getPpuByte(uint16_t address) {
    address %= 0x4000;

	if (address < 0x2000) {
		
		if (ppuMapper == 0) {
			return getPpuMapper0(address, CHR_ROM);
		} else {
			std::cerr << "Fatal error, mapper not recognized in getPpuByte()" << std::endl;
			exit(1);
			return 0;
		}


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

inline void PPU::setPpuByte(uint16_t address, uint8_t byte) {
    address %= 0x4000;

    if (address < 0x1000) {
        CHR_ROM[address] = byte;    //needed for vbl_nmi_timing test roms
        if (DEBUG) {
            std::cout << "Illegal write attempt to PPU: $" << std::hex << address << std::endl;
        }
    } else if (address < 0x2000) {
        CHR_ROM[address] = byte;    //needed for vbl_nmi_timing test roms
        if (DEBUG) {
            std::cout << "Illegal write attempt to PPU: $" << std::hex << address << std::endl;
        }
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

inline void PPU::ppuFlagUpdate(bool * NMI) {

    if (writeFlag == 0) {
        m_t &= ~0x0C00;
        m_t |= ((ppuRegisters[0] & 0x03) << 10);

        //extendedSprites = (ppuRegisters[0] & 0x20) ? true : false;
        //ppuMaster = (ppuRegisters[0] & 0x40) ? true : false;

        //this breaks spelunker
        
        /*
        if (suppressVBL) {
            ppuRegisters[0] &= 0x7F;
            suppressVBL = false;
        } else {
            if (ppuRegisters[0] & 0x80) {
                if (ppuRegisters[2] & 0x80) {
                    *NMI = true;
                }
            }
        }
        */
        


	} else if (writeFlag == 1) {

    } else if (writeFlag == 2) {


    } else if (writeFlag == 3) {

        oamAddress = ppuRegisters[0x3];

    } else if (writeFlag == 4) {

        OAM[oamAddress] = ppuRegisters[0x4];
        oamAddress = (oamAddress + 1) & 0xFF;

    } else if (writeFlag == 5) {
        if (addressLatch == true) {
            m_t &= 0x0C1F;
            m_t |= (ppuRegisters[0x5] & 0x7) << 12;
            m_t |= (ppuRegisters[0x5] & 0xF8) << 2;
        } else {
            m_x = (ppuRegisters[0x5] & 0x7);
            m_t &= ~0x001F;
            m_t |= (ppuRegisters[0x5] & 0xF8) >> 3;
        }
        addressLatch = !addressLatch;

    } else if (writeFlag == 6) {
        if (addressLatch) {
            m_t &= 0xFF00;
            m_t |= ppuRegisters[6];
            m_v = m_t;
        } else {
            m_t &= 0x00FF;

            m_t |= (ppuRegisters[6] & 0x3F) << 8;
            m_t &= 0x3FFF;
        }
        addressLatch = !addressLatch;

    } else if (writeFlag == 7) {
        //read from 2007
        m_v &= 0x3FFF;
        setPpuByte(m_v , ppuRegisters[7]);
        //increment vram address
        m_v += (ppuRegisters[0] & 0x04) ? 32 : 1;
    }

    //process state changes due to register read
    if (readFlag == 2) {
        addressLatch = false;

        ppuRegisters[2] &= 0x7F;
        *NMI = false;

        //does this need to be catched within some specific "tick window"?
        if (ppuClock % (262 * 341) == (241 * 341) + 1) {
            //scanline 241 ppu 1

            suppressVBL = true;
            //ppuRegisters[0] &= 0x7F;	//breaks spelunker
            //*NMI = false;


        }



    } else if (readFlag == 4) {

    } else if (readFlag == 7) {
        //todo: fix this
    }

    if (writeFlag != -1) {
        ppuRegisters[0x2] &= ~0x1F;
        ppuRegisters[0x2] |= (ppuRegisters[writeFlag] & 0x1F);
    }


    return;

}

uint8_t PPU::return2007() {

    //if screen off
    //if ((nesPPU.ppuRegisters[0x2] & 0x80) || ((nesPPU.ppuRegisters[0x1] & 0x18) == 0) || true) {

    uint8_t ppuByte;

    m_v &= 0x3FFF;


    if (m_v % 0x4000 < 0x3F00) {
        ppuByte = readBuffer;
        readBuffer = getPpuByte( m_v );
    } else {
        ppuByte = getPpuByte( m_v );
        readBuffer = getPpuByte( m_v - 0x1000);
    }

    m_v += (ppuRegisters[0] & 0x04) ? 32 : 1;

    m_v &= 0x7FFF;



    return ppuByte;
}

inline void PPU::drawPixel(int cycle, int line) {

    //think about how chunks of similar operations can be grouped together for caching
    //decode palette to rgb all at once at end of frame?

    int pixelLocation = (cycle - 1) + NES_SCREEN_WIDTH * line;  //current pixel being rendered
    uint8_t backgroundColour = 0;

    //render background pixel
    if (((ppuRegisters[1] & 0x2) && cycle - 1 < 8 && (ppuRegisters[1] & 0x8)) || ((ppuRegisters[1] & 0x8) && cycle - 1 >= 8)) {    //is background rendering enabled?

        bool passBound;
        if (((cycle - 1) % 8) + m_x > 7) {
            passBound = true;
        } else {
            passBound = false;
        }

        int interTilePixelLoc = (((cycle - 1) % 8 + (m_x)) % 8 );

        int pal;
        if (passBound) {
            //mux select bit
            if (getBit(m_SpriteNew1, 7 - interTilePixelLoc)) {
                backgroundColour |= 0x1;   
            }
            if (getBit(m_SpriteNew2, 7 - interTilePixelLoc)) {
                backgroundColour |= 0x2;
            }
            pal = m_PaletteNew;
        } else {
                    //mux select bit
            if (getBit(m_SpriteOld1, 7 - interTilePixelLoc)) {
                backgroundColour |= 0x1;   
            }
            if (getBit(m_SpriteOld2, 7 - interTilePixelLoc)) {
                backgroundColour |= 0x2;
            }
            pal = m_PaletteOld;
        }

        if (backgroundColour == 0) {
            pixels[pixelLocation] = getPpuByte(0x3F00);
        } else { 
            pixels[pixelLocation] = getPpuByte(0x3F00 + backgroundColour + pal * 4);
        }
    }

    //render sprite pixel if applicable
    if (((ppuRegisters[1] & 0x4) && cycle - 1 < 8 && (ppuRegisters[1] & 0x10)) || ((ppuRegisters[1] & 0x10) && cycle - 1 >= 8)) {    
        for (int i = 0; i < 8; i++) {

            uint8_t spriteLayer1 = lineOAM[i * 6 + 4];
            uint8_t spriteLayer2 = lineOAM[i * 6 + 5];

            if (spriteLayer1 == 0 && spriteLayer2 == 0) {
                continue;
            }

            int xPosition;
            xPosition = lineOAM[i * 6 + 2];

            if (!( (cycle - 1) - xPosition < 8 && (cycle - 1) - xPosition >= 0 )) {
                //current sprite i, is out of range for current pixel location 
                continue;
            }

            uint8_t spriteAttributes = lineOAM[i * 6 + 3];

            //column location of pixel in sprite data
            int spriteColumnNumber;
            if (spriteAttributes & 0x40) {
                spriteColumnNumber = ((cycle - 1) - xPosition);
            } else {
                spriteColumnNumber = 7 - ((cycle - 1) - xPosition);
            }

            //palette info for pixel
            int spriteColour = 0; 
            if (getBit(spriteLayer1, spriteColumnNumber)) {
                spriteColour |= 0x1;
            }
            if (getBit(spriteLayer2, spriteColumnNumber)) {
                spriteColour |= 0x2;
            }

            //sprite zero hit detection
            if ((spriteColour != 0)) {
                if (lineOAM[i * 6] == 1) {
                    if (((ppuRegisters[1] & 0x8)) && ((ppuRegisters[1] & 0x10))) {

                        if (backgroundColour != 0) {
                            spriteZeroFlag = true;	//this alone makes NEStress flicker
                        }
                        spriteZeroFlag = true;

                        
                    }
                }
                //priority
                if (((spriteAttributes & 0x20) == 0) || backgroundColour == 0) {
                    pixels[pixelLocation] = getPpuByte(0x3F10 + spriteColour + (spriteAttributes & 0x3) * 4);
                    break;
                }
            } else {
                //pixel is transparent
                continue;
            }
        }
    }

    return;
}

inline void PPU::updateSecondaryOAM(int line) {

    int secondaryOamAddress = 0;

    for (int x = 0; x < 8 * 6; x++) {
        lineOAM[x] = 0;
    }

    for (int x = 0; x < 64; x++) {
        int yPos;
        yPos = OAM[x * 4];

        //determine whether sprite will appear on next scanline
        if ((((line + 1) % 262) - yPos < 8 && ((line + 1) % 262) - yPos >= 0 && (yPos < 240) && (OAM[x * 4 + 3] < 255) && ((ppuRegisters[0x0] & 0x20) == 0)) ||     //8x8
            (((line + 1) % 262) - yPos < 16 && ((line + 1) % 262) - yPos >= 0 && (yPos < 240) && (OAM[x * 4 + 3] < 255) && ((ppuRegisters[0x0] & 0x20))))            //16x8
            {

            //load sprite to sOAM if in range of next scanline
            if (secondaryOamAddress < 8) {
                //sprite 0?
                if (x == 0) {
                    lineOAM[secondaryOamAddress * 6] = 1;       
                } else {
                    lineOAM[secondaryOamAddress * 6] = 0;
                }

                lineOAM[secondaryOamAddress * 6 + 1] = yPos;
                lineOAM[secondaryOamAddress * 6 + 2] = OAM[x * 4 + 3];  //xpos
                lineOAM[secondaryOamAddress * 6 + 3] = OAM[x * 4 + 2];  //attributes

                int sIndex = OAM[x * 4 + 1];

                //row location of pixel in sprite data
                int spriteRowNumber;
                if (OAM[x * 4 + 2] & 0x80) {
                    spriteRowNumber = 7 -(line + 1 - yPos);
                } else {
                    spriteRowNumber = (line + 1- yPos);
                }

                //pattern data for sprite
                lineOAM[secondaryOamAddress * 6 + 4] = getPpuByte(sIndex * 16 + (spriteRowNumber + ((ppuRegisters[0] & 0x8) ? 0x1000 : 0x0)));
                lineOAM[secondaryOamAddress * 6 + 5] = getPpuByte(sIndex * 16 + (spriteRowNumber + ((ppuRegisters[0] & 0x8) ? 0x1000 : 0x0) + 8));
            }

            secondaryOamAddress++;

            //sprite overflow detection
            if (secondaryOamAddress == 9) {

                if (((ppuRegisters[1] & 0x10)) || ((ppuRegisters[1] & 0x8))) {
                    ppuRegisters[2] |= 0x20;
                    break;
                }   
            }
        }
    }

    return;
}

void PPU::tick(bool * NMI, uint64_t * cpuClock) {

    draw = false; 
    uint64_t ppuTime = *cpuClock;

    //process state changes due to register write
    if (readFlag != -1 || writeFlag != -1) {
        ppuFlagUpdate(NMI);
        readFlag = -1;
        writeFlag = -1;
    }  

    for ( ; ppuClock < ppuTime; ppuClock++) {

        if (*NMI) {
            //return;
        }

        int cyc = ppuClock % 341;
        int line = (ppuClock % (341 * 262)) / 341;

        if (line < 240) {

            //not in vblank
            ppuRegisters[2] &= 0x7F;

            //first frame tick skipped on odd frame when screen on
            if (line == 0 && cyc == 0 && !evenFrame && (ppuRegisters[1] & 0x18)) {
                ppuClock++;
                ppuTime++;
                *cpuClock += 1;
            }

            if (cyc == 0) {
                continue;
            } else if (cyc > 0 && cyc < 256) {


                drawPixel(cyc, line);

                if (((cyc - 1) % 8 == 7)) {
                    loadNewTile();
                    horizontalIncrement(m_v);
                }
            } else if (cyc == 256) {
                drawPixel(cyc, line);
                horizontalIncrement(m_v);
                verticalIncrement(m_v);
            } else if (cyc == 257) {
                if (ppuRegisters[1] & 0x18) {
                    copyHorizontalBits(m_v, m_t);
                }
                
            } else if (cyc == 328) {
                loadNewTile();
                horizontalIncrement(m_v);
            } else if (cyc == 336) {
                loadNewTile();
                horizontalIncrement(m_v);
            } else if (cyc == 340) {
                updateSecondaryOAM(line);

                if (spriteZeroFlag) {
                    ppuRegisters[2] |= 0x40;
                    spriteZeroFlag = false;
                }

            } 
        } else if (line == 240) {
            continue;
            //idle scanline
        } else if (line == 241) {
            if (cyc == 1) {
                //set vblank in PPUSTATUS

                if (!suppressVBL) {
                    ppuRegisters[2] |= 0x80;
                } else {
                    ppuRegisters[2] &= 0x7F;
                    suppressVBL = false;
                }
                
                //throw NMI
                if ((ppuRegisters[0] & 0x80) && (ppuRegisters[2] & 0x80)) {
                    *NMI = true;
                }
            }


        } else if (line == 261) {
            if (cyc > 0 && cyc < 256) {
                if (cyc == 1) {
                    //clear vblank, sprite 0 and overflow
                    ppuRegisters[2] &= 0x1F;
                    //*NMI = false;
                    //ppuRegisters[0] &= 0x7F;	//breaks spelunker
                } else if (((cyc - 1) % 8 == 7)) {
                    loadNewTile();
                    horizontalIncrement(m_v);
                }
            } else if (cyc == 256) {
                horizontalIncrement(m_v);
                verticalIncrement(m_v);
            } else if (cyc == 257) {
                if (ppuRegisters[1] & 0x18) {
                    copyHorizontalBits(m_v, m_t);
                }
                
            } else if (cyc == 304) {

                if (ppuRegisters[1] & 0x18) {
                    copyVerticalBits(m_v, m_t);
                }
                

            } else if (cyc == 328) {
                loadNewTile();
                horizontalIncrement(m_v);
            } else if (cyc == 336) {
                loadNewTile();
                horizontalIncrement(m_v);
            } else if (cyc == 340) {
                updateSecondaryOAM(line);

                if (spriteZeroFlag) {
                    ppuRegisters[2] |= 0x40;
                    spriteZeroFlag = false;
                }

                draw = true;
                evenFrame ^= true;
            } 
        }
    }

    return;
}
/* End PPU class functions */
