#include <iostream>

#include "NES.hpp"
#include "mappers.hpp"


/* Begin macro functions */
#define getBit(num, bit)    (bit == 0) ? (num & 0x1) : (bit == 1) ? (num & 0x2) : \
    (bit == 2) ? (num & 0x4) : (bit == 3) ? (num & 0x8) :   \
    (bit == 4) ? (num & 0x10) : (bit == 5) ? (num & 0x20) : \
    (bit == 6) ? (num & 0x40) : (num & 0x80)

#define horizontalIncrement(loopyv)  if ((loopyv & 0x001F) == 31) {     \
    loopyv &= ~0x001F;              \
    loopyv ^= 0x400;                \
} else {                            \
    loopyv += 1;                    \
}                                   

#define verticalIncrement(loopyv) if ((loopyv & 0x7000) != 0x7000) {  \
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
}

#define copyHorizontalBits(loopyv, loopyt) loopyv &= ~0x041F;  \
                loopyv |= (loopyt & 0x041F);


#define copyVerticalBits(loopyv, loopyt) loopyv &= 0x041F;  \
                loopyv |= (loopyt & (~0x041F));
/* End macro functions */

/* Begin PPU class functions */
PPU::PPU() {

    for (int x = 0; x < 0x20; x++) palette[x] = 0;
    for (int x = 0; x < 0x8; x++) ppuRegisters[x] = 0x0;
    for (int x = 0; x < 0x800; x++) VRAM[x] = 0x0;
    for (int x = 0; x < 0x100; x++) OAM[x] = 0x0;
    for (int x = 0; x < (NES_SCREEN_WIDTH * NES_SCREEN_HEIGHT); x++) pixels[x] = 63;    //black in palette
    for (int x = 0; x < 8; x++) secondaryOAM[x] = 0x0;
    for (int x = 0; x < 0x9; x++) registerWriteFlags[x] = false;
    for (int x = 0; x < 8; x++) registerReadFlags[x] = false;
    
    evenFrame = true;
    draw = false;
    spriteZeroFlag = false;
    addressLatch = false;
    m_v = 0;
    m_t = 0;
    m_x = 0;
    oamAddress = 0x0;
    secondaryOamAddress = 0x0;
    CHR_ROM = NULL;
    readBuffer = 0;
    flagSet = false;

    //temp
    m_SpriteOld1 = 0;
    m_SpriteOld2 = 0;
    m_PaletteOld = 0;
    m_SpriteNew1 = 0;
    m_SpriteNew2 = 0;
    m_PaletteNew = 0;
    vramAddress = 0;

    suppressVBL = false;

    ppuClock = 241 * 341;   //start at scanline 241

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

uint8_t PPU::getPpuByte(uint16_t address) {
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

void PPU::setPpuByte(uint16_t address, uint8_t byte) {
    address %= 0x4000;

    if (address < 0x1000) {
        CHR_ROM[address] = byte;
        //std::cout << "Illegal write to PPU: $" << std::hex << address << std::endl;
    } else if (address < 0x2000) {
        CHR_ROM[address] = byte;
        //std::cout << "Illegal write to PPU: $" << std::hex << address << std::endl;
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

void PPU::ppuFlagUpdate(NES * nes) {

    if (registerWriteFlags[0]) {
        m_t &= 0xF3FF;
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
                    nes->nesCPU.NMI = true;
                }
            }
        }
        */

		

        ppuRegisters[0x2] &= ~0x1F;
        ppuRegisters[0x2] |= (ppuRegisters[0x0] & 0x1F);
    } else if (registerWriteFlags[1]) {
        ppuRegisters[0x2] &= ~0x1F;
        ppuRegisters[0x2] |= (ppuRegisters[0x1] & 0x1F);

    } else if (registerWriteFlags[2]) {
        ppuRegisters[0x2] &= ~0x1F;
        ppuRegisters[0x2] |= (ppuRegisters[0x2] & 0x1F);

    } else if (registerWriteFlags[3]) {

        oamAddress = ppuRegisters[0x3];
        ppuRegisters[0x2] &= ~0x1F;
        ppuRegisters[0x2] |= (ppuRegisters[0x3] & 0x1F);

    } else if (registerWriteFlags[4]) {

        OAM[oamAddress] = ppuRegisters[0x4];
        oamAddress = (oamAddress + 1) & 0xFF;

        ppuRegisters[0x2] &= ~0x1F;
        ppuRegisters[0x2] |= (ppuRegisters[0x4] & 0x1F);


    } else if (registerWriteFlags[5]) {
        if (addressLatch == true) {
            m_t &= 0x0C1F;
            m_t |= (ppuRegisters[0x5] & 0x7) << 12;
            m_t |= (ppuRegisters[0x5] & 0xF8) << 2;
            addressLatch = false;
        } else {
            m_x = (ppuRegisters[0x5] & 0x7);
            m_t &= 0xFFE0;
            m_t |= (ppuRegisters[0x5] & 0xF8) >> 3;
            addressLatch = true;
        }
        ppuRegisters[0x2] &= ~0x1F;
        ppuRegisters[0x2] |= (ppuRegisters[0x5] & 0x1F);

    } else if (registerWriteFlags[6]) {
        if (addressLatch) {
            m_t &= 0xFF00;
            m_t |= ppuRegisters[6];
            m_t &= 0x7FFF;
            m_v = m_t;
            vramAddress = m_t;
            addressLatch = false;
        } else {
            m_t &= 0x00FF;
            m_t |= (ppuRegisters[6] & 0x3F) << 8;
            addressLatch = true;
        }
        ppuRegisters[0x2] &= ~0x1F;
        ppuRegisters[0x2] |= (ppuRegisters[0x6] & 0x1F);

    } else if (registerWriteFlags[7]) {
        //read from 2007

        setPpuByte(vramAddress , ppuRegisters[7]);

        //increment vram address
        vramAddress += (ppuRegisters[0] & 0x04) ? 32 : 1;
        vramAddress %= 0x4000;

        ppuRegisters[0x2] &= ~0x1F;
        ppuRegisters[0x2] |= (ppuRegisters[0x7] & 0x1F);

    } else if (registerWriteFlags[8]) {

        uint8_t OAMDMA;
        OAMDMA = nes->nesCPU.getCpuByte(0x4014);
        for (unsigned int x = 0; x < 256; x++) {
            OAM[oamAddress] = nes->nesCPU.getCpuByte( (OAMDMA << 8) + x );
            oamAddress = (oamAddress + 1) & 0xFF;
        }
    }

    //process state changes due to register read
    if (registerReadFlags[2]) {
        addressLatch = false;


        ppuRegisters[2] &= 0x7F;

        //does this need to be catched within some specific "tick window"?
        if (ppuClock % (262 * 341) == (241 * 341) + 1) {
            //scanline 241 ppu 1

            suppressVBL = true;
            //ppuRegisters[0] &= 0x7F;	breaks spelunker
            //nes->nesCPU.NMI = false;


        }



    } else if (registerReadFlags[4]) {

    } else if (registerReadFlags[7]) {
        //todo: fix this
    }

    //reset register access flags
    for (int x = 0; x < 0x9; x++) registerWriteFlags[x] = false;

    for (int x = 0; x < 8; x++) registerReadFlags[x] = false;

    return;

}

void PPU::drawPixel(int cycle, int line) {

    //think about how chunks of similar operations can be grouped together for caching
    //decode palette to rgb all at once at end of frame?

    int pixelLocation;
    pixelLocation = (cycle - 1) + NES_SCREEN_WIDTH * line;  //current pixel being rendered

    uint8_t backgroundColour;
    backgroundColour = 0;                

    if (((ppuRegisters[1] & 0x2) && cycle - 1 < 8 && (ppuRegisters[1] & 0x8)) || ((ppuRegisters[1] & 0x8) && cycle - 1 >= 8)) {    //is background rendering enabled?

        bool passBound;

        if (((cycle - 1) % 8) + m_x > 7) {
            passBound = true;
        } else {
            passBound = false;
        }

        int interTilePixelLoc;
        interTilePixelLoc = (((cycle - 1) % 8 + (m_x)) % 8 );

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



        //draw rgb pixel based on palette
        if (backgroundColour == 0) {
            pixels[pixelLocation] = getPpuByte(0x3F00);
        } else { 
            pixels[pixelLocation] = getPpuByte(0x3F00 + backgroundColour + pal * 4);
        }
    }

    if (((ppuRegisters[1] & 0x4) && cycle - 1 < 8 && (ppuRegisters[1] & 0x10)) || ((ppuRegisters[1] & 0x10) && cycle - 1 >= 8)) {    
        for (int i = 0; i < secondaryOamAddress; i++) {

            int xPosition;
            xPosition = OAM[secondaryOAM[i] + 3];

            if (!( (cycle - 1) - xPosition < 8 && (cycle - 1) - xPosition >= 0 )) {
                //current sprite i, is out of range for current pixel location 
                continue;
            }

            uint8_t yPosition = OAM[secondaryOAM[i]];
            uint8_t spriteIndex = OAM[secondaryOAM[i] + 1];
            uint8_t spriteAttributes = OAM[secondaryOAM[i] + 2];

            //row location of pixel in sprite data
            int spriteRowNumber;
            if (spriteAttributes & 0x80) {
                spriteRowNumber = 7 -(line - yPosition);
            } else {
                spriteRowNumber = (line - yPosition);
            }

            //column location of pixel in sprite data
            int spriteColumnNumber;
            if (spriteAttributes & 0x40) {
                spriteColumnNumber = ((cycle - 1) - xPosition);
            } else {
                spriteColumnNumber = 7 - ((cycle - 1) - xPosition);
            }

            //pattern data for sprite
            uint8_t spriteLayer3 = getPpuByte(spriteIndex * 16 + (spriteRowNumber + ((ppuRegisters[0] & 0x8) ? 0x1000 : 0x0)));
            uint8_t spriteLayer4 = getPpuByte(spriteIndex * 16 + (spriteRowNumber + ((ppuRegisters[0] & 0x8) ? 0x1000 : 0x0) + 8));

            //palette info for pixel
            int spriteColour = 0; 
            if (getBit(spriteLayer3, spriteColumnNumber)) {
                spriteColour |= 0x1;
            }
            if (getBit(spriteLayer4, spriteColumnNumber)) {
                spriteColour |= 0x2;
            }

            //sprite zero hit detection
            if ((spriteColour != 0x0)) {
                if (secondaryOAM[i] == 0) {
                    if (((ppuRegisters[1] & 0x8)) && ((ppuRegisters[1] & 0x10))) {

                        if (backgroundColour != 0) {
                            spriteZeroFlag = true;	//this alone makes NEStress flicker
                        }
                        spriteZeroFlag = true;

                        
                    }
                }
            }

            //pixel is transparent
            if (spriteColour == 0x0) {
                continue;
            }    

            //priority
            if (((spriteAttributes & 0x20) == 0) || backgroundColour == 0) {
                pixels[pixelLocation] = getPpuByte(0x3F10 + spriteColour + (spriteAttributes & 0x3) * 4);
            }
        }
    }

    return;
}

void PPU::updateSecondaryOAM(int line) {

    //prepare secondary OAM for next scanline
    for (int x = 0; x < 8; x++) {
        secondaryOAM[x] = 0;
    }

    secondaryOamAddress = 0;

    for (int x = 0; x < 64; x++) {
        int yPos;
        yPos = OAM[x * 4];


        //determine whether sprite will appear on next scanline
        if ((((line + 1) % 262) - yPos < 8 && ((line + 1) % 262) - yPos >= 0 && (yPos < 240) && (OAM[x * 4 + 3] < 255) && ((ppuRegisters[0x0] & 0x20) == 0)) ||     //8x8
            (((line + 1) % 262) - yPos < 16 && ((line + 1) % 262) - yPos >= 0 && (yPos < 240) && (OAM[x * 4 + 3] < 255) && ((ppuRegisters[0x0] & 0x20))))            //16x8
            {

            if (secondaryOamAddress < 8) {
                secondaryOAM[secondaryOamAddress] = x * 4;
            }

            secondaryOamAddress++;

            //sprite overflow detection
            if (secondaryOamAddress == 9) {

                if (((ppuRegisters[1] & 0x10)) || ((ppuRegisters[1] & 0x8))) {
                    ppuRegisters[2] |= 0x20;
                    secondaryOamAddress--;
                    break;
                }   
            }
        }
    }

    return;
}

void PPU::tick(NES * nes, int numTicks) {

    draw = false; 

    //process state changes due to register write
    if (flagSet) {
        ppuFlagUpdate(nes);
        flagSet = false;
    }  

    for (int loop = 0; loop < numTicks; loop++, ppuClock++) {

        int cyc = ppuClock % 341;
        int line = (ppuClock % (341 * 262)) / 341;


        if (line < 240) {

            //not in vblank
            ppuRegisters[2] &= 0x7F;

            //first frame tick skipped on odd frame when screen on
            if (line == 0 && cyc == 0 && !evenFrame && (ppuRegisters[1] & 0x18)) {
                ppuClock++;
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
                copyHorizontalBits(m_v, m_t);
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
                    nes->nesCPU.NMI = true;
                }
            }
        } else if (line == 261) {
            if (cyc > 0 && cyc < 256) {
                if (cyc == 1) {
                    //clear vblank, sprite 0 and overflow
                    ppuRegisters[2] &= 0x1F;
                    //nes->nesCPU.NMI = false;
                    //ppuRegisters[0] &= 0x7F;	//breaks spelunker
                } else if (((cyc - 1) % 8 == 7)) {
                    loadNewTile();
                    horizontalIncrement(m_v);
                }
            } else if (cyc == 256) {
                horizontalIncrement(m_v);
                verticalIncrement(m_v);
            } else if (cyc == 257) {
                copyHorizontalBits(m_v, m_t);
            } else if (cyc == 304) {
                copyVerticalBits(m_v, m_t);
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
