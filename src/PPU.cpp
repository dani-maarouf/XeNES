#include <iostream>
#include "mappers.hpp"
#include "PPU.hpp"

const bool DEBUG = false;

#define get_bit(num, bit)    (bit == 0) ? (num & 0x1) : (bit == 1) ? (num & 0x2) : \
    (bit == 2) ? (num & 0x4) : (bit == 3) ? (num & 0x8) :   \
    (bit == 4) ? (num & 0x10) : (bit == 5) ? (num & 0x20) : \
    (bit == 6) ? (num & 0x40) : (num & 0x80)

#define horizontal_increment(loopyv) if (m_ppuRegisters[1] & 0x18) {\
    if ((loopyv & 0x001F) == 31) {      \
        loopyv &= ~0x001F;              \
        loopyv ^= 0x400;                \
    } else {                            \
        loopyv++;                       \
    }                                   \
}

#define vertical_increment(loopyv) if (m_ppuRegisters[1] & 0x18) {\
    if ((loopyv & 0x7000) != 0x7000) {                  \
        loopyv += 0x1000;                               \
    } else {                                            \
        loopyv &= ~0x7000;                              \
        u16 yVal;                                       \
        yVal = (loopyv & 0x03E0) >> 5;                  \
        if (yVal == 29) {                               \
            yVal = 0;                                   \
            loopyv ^= 0x800;                            \
        } else if (yVal == 31) {                        \
            yVal = 0;                                   \
        } else {                                        \
            yVal++;                                     \
        }                                               \
        loopyv = ((loopyv & ~0x03E0) | (yVal << 5));    \
    }                                                   \
}

#define copy_horizontal_bits(loopyv, loopyt) if (m_ppuRegisters[1] & 0x18) {\
    loopyv &= ~0x041F;              \
    loopyv |= (loopyt & 0x041F);    \
}


#define copy_vertical_bits(loopyv, loopyt) if (m_ppuRegisters[1] & 0x18) {\
    loopyv &= 0x041F;               \
    loopyv |= (loopyt & (~0x041F)); \
}

/* Begin PPU class functions */
PPU::PPU() {

    for (int x = 0; x < 0x20; x++) m_palette[x] = 0;
    for (int x = 0; x < 0x8; x++) m_ppuRegisters[x] = 0x0;
    for (int x = 0; x < 0x800; x++) m_VRAM[x] = 0x0;
    for (int x = 0; x < 0x100; x++) m_OAM[x] = 0x0;
    for (int x = 0; x < (NES_SCREEN_WIDTH * NES_SCREEN_HEIGHT); x++) m_pixels[x] = 63;    //black in m_palette
    for (int x = 0; x < 6 * 8; x++) m_lineOAM[x] = 0x0;
    

    m_draw = m_addressLatch = m_suppressVBL = false;

    m_v = m_t = m_x = 0;

    m_oamAddress = 0;

    m_CHR_ROM = NULL;
    m_readBuffer = 0;

    m_SpriteOld1 = m_SpriteOld2 = m_PaletteOld = m_SpriteNew1 = m_SpriteNew2 = m_PaletteNew = 0;
    m_ppuClock = 0;
    m_writeFlag = m_readFlag = -1;

    return;
}

void PPU::free_pointers() {
    if (m_CHR_ROM != NULL) {
        delete [] m_CHR_ROM;
    }
    return;
}

inline void PPU::load_new_tile() {

    if (m_ppuRegisters[1] & 0x18) {
        m_SpriteOld1 = m_SpriteNew1;
        m_SpriteOld2 = m_SpriteNew2;
        m_PaletteOld = m_PaletteNew;

        m_PaletteNew = ((get_ppu_byte(0x23C0 | (m_v & 0x0C00) | ((m_v >> 4) & 0x38) | ((m_v >> 2) & 0x07))) >> (2 * ((((m_v & 0x1F) % 4) / 2) + ((((m_v & 0x370) >> 5) % 4) / 2) * 2))) & 0x3;

        int spriteStart = get_ppu_byte(0x2000 | (m_v & 0x0FFF));
        int spriteAddress = spriteStart * 16 + ((m_v & 0x7000) >> 12) + ((m_ppuRegisters[0] & 0x10) ? 0x1000 : 0x0);

        //load sprite data
        m_SpriteNew1 = get_ppu_byte(spriteAddress);
        m_SpriteNew2 = get_ppu_byte(spriteAddress + 8);
    }

    return;
}

inline u8 PPU::get_ppu_byte(u16 address) {
    address &= 0x3FFF;

	if (address < 0x2000) {
		
		if (m_ppuMapper == 0) {
			return get_ppu_mapper_0(address, m_CHR_ROM);
		} else {
			std::cerr << "Fatal error, mapper not recognized in get_ppu_byte()" << std::endl;
			exit(EXIT_FAILURE);
			return 0;
		}


    } else if (address < 0x2400) {
        return m_VRAM[address - 0x2000];
    } else if (address < 0x2800) {
        if (m_mirroring == VERTICAL) {
            return m_VRAM[address - 0x2000];
        } else if (m_mirroring == HORIZONTAL) {
            return m_VRAM[address - 0x2400];
        } else {
            return m_VRAM[address - 0x2000];
        }
    } else if (address < 0x2C00) {
        if (m_mirroring == VERTICAL) {
            return m_VRAM[address - 0x2800];
        } else if (m_mirroring == HORIZONTAL) {
            return m_VRAM[address - 0x2400];
        } else {
            return m_VRAM[address - 0x2000];
        }
    } else if (address < 0x3000) {
        if (m_mirroring == VERTICAL) {
            return m_VRAM[address - 0x2800];
        } else if (m_mirroring == HORIZONTAL) {
            return m_VRAM[address - 0x2800];
        } else {
            return m_VRAM[address - 0x2000];
        }
    } else if (address < 0x3400) {
        return m_VRAM[address - 0x3000];
    } else if (address < 0x3800) {
        if (m_mirroring == VERTICAL) {
            return m_VRAM[address - 0x3000];
        } else if (m_mirroring == HORIZONTAL) {
            return m_VRAM[address - 0x3400];
        } else {
            return m_VRAM[address - 0x3000];
        }
    } else if (address < 0x3C00) {
        if (m_mirroring == VERTICAL) {
            return m_VRAM[address - 0x3800];
        } else if (m_mirroring == HORIZONTAL) {
            return m_VRAM[address - 0x3400];
        } else {
            return m_VRAM[address - 0x3000];
        }
    } else if (address < 0x3F00) {
        if (m_mirroring == VERTICAL) {
            return m_VRAM[address - 0x3800];
        } else if (m_mirroring == HORIZONTAL) {
            return m_VRAM[address - 0x3800];
        } else {
            return m_VRAM[address - 0x3000];
        }
    } else {
        u8 newAddress;
        newAddress = (address - 0x3F00) % 0x20;
        if (newAddress == 0x10 || newAddress == 0x14 || newAddress == 0x18 || newAddress == 0x1C) {
            newAddress -= (u8) 0x10;
        }
        return m_palette[newAddress];
    }
}

inline void PPU::set_ppu_byte(u16 address, u8 byte) {
    address &= 0x3FFF;

    if (address < 0x1000) {
        m_CHR_ROM[address] = byte;    //needed for vbl_nmi_timing test roms
        if (DEBUG) {
            std::cout << "Illegal write attempt to PPU: $" << std::hex << address << std::endl;
        }
    } else if (address < 0x2000) {
        m_CHR_ROM[address] = byte;    //needed for vbl_nmi_timing test roms
        if (DEBUG) {
            std::cout << "Illegal write attempt to PPU: $" << std::hex << address << std::endl;
        }
    } else if (address < 0x2400) {
        m_VRAM[address - 0x2000] = byte;
    } else if (address < 0x2800) {
        if (m_mirroring == VERTICAL) {
            m_VRAM[address - 0x2000] = byte;
        } else if (m_mirroring == HORIZONTAL) {
            m_VRAM[address - 0x2400] = byte;
        } else {
            m_VRAM[address - 0x2000] = byte;
        }
    } else if (address < 0x2C00) {
        if (m_mirroring == VERTICAL) {
            m_VRAM[address - 0x2800] = byte;
        } else if (m_mirroring == HORIZONTAL) {
            m_VRAM[address - 0x2400] = byte;
        } else {
            m_VRAM[address - 0x2000] = byte;
        }
    } else if (address < 0x3000) {
        if (m_mirroring == VERTICAL) {
            m_VRAM[address - 0x2800] = byte;
        } else if (m_mirroring == HORIZONTAL) {
            m_VRAM[address - 0x2800] = byte;
        } else {
            m_VRAM[address - 0x2000] = byte;
        }
    } else if (address < 0x3400) {
        m_VRAM[address - 0x3000] = byte;
    } else if (address < 0x3800) {
        if (m_mirroring == VERTICAL) {
            m_VRAM[address - 0x3000] = byte;
        } else if (m_mirroring == HORIZONTAL) {
            m_VRAM[address - 0x3400] = byte;
        } else {
            m_VRAM[address - 0x3000] = byte;
        }
    } else if (address < 0x3C00) {
        if (m_mirroring == VERTICAL) {
            m_VRAM[address - 0x3800] = byte;
        } else if (m_mirroring == HORIZONTAL) {
            m_VRAM[address - 0x3400] = byte;
        } else {
            m_VRAM[address - 0x3000] = byte;
        }
    } else if (address < 0x3F00) {
        if (m_mirroring == VERTICAL) {
            m_VRAM[address - 0x3800] = byte;
        } else if (m_mirroring == HORIZONTAL) {
            m_VRAM[address - 0x3800] = byte;
        } else {
            m_VRAM[address - 0x3000] = byte;
        }
    } else {
        u8 newAddress;
        newAddress = (address - 0x3F00) % 0x20;
        if (newAddress == 0x10 || newAddress == 0x14 || newAddress == 0x18 || newAddress == 0x1C) {
            newAddress -= 0x10;
        }
        m_palette[newAddress] = byte;
    }
    return;
}

inline void PPU::ppu_flag_update(bool * NMI) {

    if (m_writeFlag == 0) {
        m_t &= ~0x0C00;
        m_t |= ((m_ppuRegisters[0] & 0x03) << 10);

        //extendedSprites = (m_ppuRegisters[0] & 0x20) ? true : false;
        //ppuMaster = (m_ppuRegisters[0] & 0x40) ? true : false;

        //this breaks spelunker
        
        /*
        if (m_suppressVBL) {
            m_ppuRegisters[0] &= 0x7F;
            m_suppressVBL = false;
        } else {
            if (m_ppuRegisters[0] & 0x80) {
                if (m_ppuRegisters[2] & 0x80) {
                    *NMI = true;
                }
            }
        }
        */
        


	} else if (m_writeFlag == 1) {

    } else if (m_writeFlag == 2) {


    } else if (m_writeFlag == 3) {

        m_oamAddress = m_ppuRegisters[0x3];

    } else if (m_writeFlag == 4) {

        m_OAM[m_oamAddress] = m_ppuRegisters[0x4];
        m_oamAddress = (m_oamAddress + 1) & 0xFF;

    } else if (m_writeFlag == 5) {
        if (m_addressLatch == true) {
            m_t &= 0x0C1F;
            m_t |= (m_ppuRegisters[0x5] & 0x7) << 12;
            m_t |= (m_ppuRegisters[0x5] & 0xF8) << 2;
        } else {
            m_x = (m_ppuRegisters[0x5] & 0x7);
            m_t &= ~0x001F;
            m_t |= (m_ppuRegisters[0x5] & 0xF8) >> 3;
        }
        m_addressLatch = !m_addressLatch;

    } else if (m_writeFlag == 6) {
        if (m_addressLatch) {
            m_t &= 0xFF00;
            m_t |= m_ppuRegisters[6];
            m_v = m_t;
        } else {
            m_t &= 0x00FF;

            m_t |= (m_ppuRegisters[6] & 0x3F) << 8;
            m_t &= 0x3FFF;
        }
        m_addressLatch = !m_addressLatch;

    } else if (m_writeFlag == 7) {
        //read from 2007
        m_v &= 0x3FFF;
        set_ppu_byte(m_v , m_ppuRegisters[7]);
        //increment vram address
        m_v += (m_ppuRegisters[0] & 0x04) ? 32 : 1;
    }

    //process state changes due to register read
    if (m_readFlag == 2) {
        m_addressLatch = false;
        m_ppuRegisters[2] &= 0x7F;
        *NMI = false;

    } else if (m_readFlag == 4) {

    } else if (m_readFlag == 7) {
        //todo: fix this
    }

    if (m_writeFlag != -1) {
        m_ppuRegisters[0x2] &= ~0x1F;
        m_ppuRegisters[0x2] |= (m_ppuRegisters[m_writeFlag] & 0x1F);
    }


    return;

}

u8 PPU::return_2007(bool silent) {

    //if screen off
    //if ((nesPPU.m_ppuRegisters[0x2] & 0x80) || ((nesPPU.m_ppuRegisters[0x1] & 0x18) == 0) || true) {

    u8 ppuByte;

    if (!silent) {
        m_v &= 0x3FFF;
    }
    
    if ( (m_v & 0x3FFF) % 0x4000 < 0x3F00) {
        ppuByte = m_readBuffer;
        m_readBuffer = get_ppu_byte( m_v );
    } else {
        ppuByte = get_ppu_byte( m_v );
        m_readBuffer = get_ppu_byte( m_v - 0x1000);
    }

    if (!silent) {
        m_v += (m_ppuRegisters[0] & 0x04) ? 32 : 1;
        m_v &= 0x7FFF;
    }

    return ppuByte;
}

inline void PPU::draw_pixel(int cycle, int line) {

    //think about how chunks of similar operations can be grouped together for caching
    //decode m_palette to rgb all at once at end of frame?

    int pixelLocation = (cycle - 1) + NES_SCREEN_WIDTH * line;  //current pixel being rendered
    u8 backgroundColour = 0;

    //render background pixel
    if (((m_ppuRegisters[1] & 0x2) && cycle  < 9 && (m_ppuRegisters[1] & 0x8)) || ((m_ppuRegisters[1] & 0x8) && cycle >= 9)) {    //is background rendering enabled?

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
            if (get_bit(m_SpriteNew1, 7 - interTilePixelLoc)) {
                backgroundColour |= 0x1;   
            }
            if (get_bit(m_SpriteNew2, 7 - interTilePixelLoc)) {
                backgroundColour |= 0x2;
            }
            pal = m_PaletteNew;
        } else {
                    //mux select bit
            if (get_bit(m_SpriteOld1, 7 - interTilePixelLoc)) {
                backgroundColour |= 0x1;   
            }
            if (get_bit(m_SpriteOld2, 7 - interTilePixelLoc)) {
                backgroundColour |= 0x2;
            }
            pal = m_PaletteOld;
        }

        if (backgroundColour == 0) {
            m_pixels[pixelLocation] = get_ppu_byte(0x3F00);
        } else { 
            m_pixels[pixelLocation] = get_ppu_byte(0x3F00 + backgroundColour + pal * 4);
        }
    }

    //render sprite pixel if applicable
    if (((m_ppuRegisters[1] & 0x4) && (cycle) < 9 && (m_ppuRegisters[1] & 0x10)) || ((m_ppuRegisters[1] & 0x10) && (cycle) >= 9)) {    
        for (int i = 0; i < 8; i++) {

            u8 spriteLayer1 = m_lineOAM[i * 6 + 4];
            u8 spriteLayer2 = m_lineOAM[i * 6 + 5];

            if (spriteLayer1 == 0 && spriteLayer2 == 0) {
                continue;
            }

            int xPosition;
            xPosition = m_lineOAM[i * 6 + 2];

            if (!( (cycle - 1) - xPosition < 8 && (cycle - 1) - xPosition >= 0 )) {
                //current sprite i, is out of range for current pixel location 
                continue;
            }

            u8 spriteAttributes = m_lineOAM[i * 6 + 3];

            //column location of pixel in sprite data
            int spriteColumnNumber;
            if (spriteAttributes & 0x40) {
                spriteColumnNumber = ((cycle - 1) - xPosition);
            } else {
                spriteColumnNumber = 7 - ((cycle - 1) - xPosition);
            }

            //m_palette info for pixel
            int spriteColour = 0; 
            if (get_bit(spriteLayer1, spriteColumnNumber)) {
                spriteColour |= 0x1;
            }
            if (get_bit(spriteLayer2, spriteColumnNumber)) {
                spriteColour |= 0x2;
            }

            //sprite zero hit detection
            if ((spriteColour != 0)) {
                if (m_lineOAM[i * 6] == 1) {
                    if ((m_ppuRegisters[1] & 0x18) == 0x18) {

                        if (backgroundColour != 0) {
                            if (cycle < 256) {
                                m_ppuRegisters[2] |= 0x40;    //this alone makes NEStress flicker
                            }
                        }
                        
                    }
                }
                //priority
                if (((spriteAttributes & 0x20) == 0) || backgroundColour == 0) {
                    m_pixels[pixelLocation] = get_ppu_byte(0x3F10 + spriteColour + (spriteAttributes & 0x3) * 4);
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

inline void PPU::update_secondary_oam(int line) {

    int secondaryOamAddress = 0;

    for (int x = 0; x < 8 * 6; x++) {
        m_lineOAM[x] = 0;
    }

    for (int x = 0; x < 64; x++) {
        u8 yPos;
        yPos = m_OAM[x * 4];

        //determine whether sprite will appear on next scanline
        if ((((line) % 262) - yPos < 8 && ((line) % 262) - yPos >= 0 && (yPos < 240) && (m_OAM[x * 4 + 3] < 255) && ((m_ppuRegisters[0x0] & 0x20) == 0)) ||     //8x8
            (((line) % 262) - yPos < 16 && ((line) % 262) - yPos >= 0 && (yPos < 240) && (m_OAM[x * 4 + 3] < 255) && ((m_ppuRegisters[0x0] & 0x20))))            //16x8
            {

            //load sprite to sOAM if in range of next scanline
            if (secondaryOamAddress < 8) {
                //sprite 0?
                if (x == 0) {
                    m_lineOAM[secondaryOamAddress * 6] = 1;       
                } else {
                    m_lineOAM[secondaryOamAddress * 6] = 0;
                }

                m_lineOAM[secondaryOamAddress * 6 + 1] = yPos;
                m_lineOAM[secondaryOamAddress * 6 + 2] = m_OAM[x * 4 + 3];  //xpos
                m_lineOAM[secondaryOamAddress * 6 + 3] = m_OAM[x * 4 + 2];  //attributes

                int sIndex = m_OAM[x * 4 + 1];

                //row location of pixel in sprite data
                int spriteRowNumber;
                if (m_OAM[x * 4 + 2] & 0x80) {
                    spriteRowNumber = 7 -(line - yPos);
                } else {
                    spriteRowNumber = (line- yPos);
                }

                //pattern data for sprite
                m_lineOAM[secondaryOamAddress * 6 + 4] = get_ppu_byte(sIndex * 16 + (spriteRowNumber + ((m_ppuRegisters[0] & 0x8) ? 0x1000 : 0x0)));
                m_lineOAM[secondaryOamAddress * 6 + 5] = get_ppu_byte(sIndex * 16 + (spriteRowNumber + ((m_ppuRegisters[0] & 0x8) ? 0x1000 : 0x0) + 8));
            }

            secondaryOamAddress++;

            //sprite overflow detection
            if (secondaryOamAddress == 9) {

                if (((m_ppuRegisters[1] & 0x10)) || ((m_ppuRegisters[1] & 0x8))) {
                    m_ppuRegisters[2] |= 0x20;    //sprite overflow
                    break;
                }   
            }
        }
    }

    return;
}

void PPU::tick(bool * NMI, uintmax_t * cpuClock) {

    m_draw = false; 
    uintmax_t ppuEnd = *cpuClock;

    //process state changes due to register write
    if (m_readFlag != -1 || m_writeFlag != -1) {
        ppu_flag_update(NMI);
        m_readFlag = -1;
        m_writeFlag = -1;
    }  

    for ( ; m_ppuClock < ppuEnd; m_ppuClock++) {

        int cyc = m_ppuClock % 341;
        int line = (m_ppuClock % (341 * 262)) / 341;

        
        //first frame tick skipped on odd frame when screen on
        if (m_ppuRegisters[1] & 0x8) {
            if (((m_ppuClock % (262 * 341 * 2)) == 0)) {
                m_ppuClock++;
                ppuEnd++;
                *cpuClock += 1;
                
            }
        }
        

        if (line < 240) {

            //not in vblank
            m_ppuRegisters[2] &= 0x7F;
            

            if (cyc == 0) {
                continue;
            } else if (cyc > 0 && cyc < 256) {

                if (m_ppuRegisters[1] & 0x18) {
                    draw_pixel(cyc, line);
                }

                if (((cyc - 1) % 8 == 7)) {
                    load_new_tile();
                    horizontal_increment(m_v);
                }
            } else if (cyc == 256) {
                draw_pixel(cyc, line);
                horizontal_increment(m_v);
                vertical_increment(m_v);
            } else if (cyc == 257) {
                if (m_ppuRegisters[1] & 0x18) {
                    copy_horizontal_bits(m_v, m_t);
                }
                
            } else if (cyc == 328) {
                load_new_tile();
                horizontal_increment(m_v);
            } else if (cyc == 336) {
                load_new_tile();
                horizontal_increment(m_v);
            } else if (cyc == 340) {
                if (m_ppuRegisters[1] & 0x18) {
                    update_secondary_oam(line);
                }

            } 
        } else if (line == 240) {
            continue;
            //idle scanline
        } else if (line == 241) {
            if (cyc == 1) {

                m_ppuRegisters[2] &= 0x7F;

                //set vblank in PPUSTATUS
                if (!m_suppressVBL) {
                    m_ppuRegisters[2] |= 0x80;
                }
                
            } else if (cyc == 2) {

                //throw NMI (should this be cyc == 1)? this matched nintendulator apparently
                if ((m_ppuRegisters[0] & 0x80) && (m_ppuRegisters[2] & 0x80)) {
                    *NMI = true;
                }

            }


        } else if (line == 261) {
            if (cyc > 0 && cyc < 256) {
                if (cyc == 1) {
                    //clear vblank, sprite 0 and overflow
                    m_ppuRegisters[2] &= 0x1F;
                    m_suppressVBL = false;
                    *NMI = false;
                    //m_ppuRegisters[0] &= 0x7F;	//breaks spelunker
                } else if (((cyc - 1) % 8 == 7)) {
                    load_new_tile();
                    horizontal_increment(m_v);
                }
            } else if (cyc == 256) {
                horizontal_increment(m_v);
                vertical_increment(m_v);
            } else if (cyc == 257) {
                copy_horizontal_bits(m_v, m_t);
            } else if (cyc == 304) {
                copy_vertical_bits(m_v, m_t);
            } else if (cyc == 328) {
                load_new_tile();
                horizontal_increment(m_v);
            } else if (cyc == 336) {
                load_new_tile();
                horizontal_increment(m_v);
            } else if (cyc == 340) {
                if (m_ppuRegisters[1] & 0x18) {
                    update_secondary_oam(line);
                }

                m_draw = true;
            } 
        }
    }

    return;
}
/* End PPU class functions */
