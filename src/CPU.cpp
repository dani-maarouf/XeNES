#include <iostream>
#include "CPU.hpp"
#include "mappers.hpp"

#include "debugger.hpp"

//used for setting length on APU
const int lengthTable[] = {
    10,254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30,
};

//DMC sample playback rate
const int rateTable[] = {
    428,380,340,320,286,254,226,214,190,160,142,128,106, 84, 72, 54
};


#define get_bit(num, bit)    (bit == 0) ? (num & 0x1) : (bit == 1) ? (num & 0x2) :   \
                            (bit == 2) ? (num & 0x4) : (bit == 3) ? (num & 0x8) :   \
                            (bit == 4) ? (num & 0x10) : (bit == 5) ? (num & 0x20) : \
                            (bit == 6) ? (num & 0x40) : (num & 0x80)

//used to calculate number of cpu ticks for each instruction
enum InstructionType {
    READ   = 0,     //read from mem address
    WRITE  = 1,     //write to mem address
    R_M_W  = 2,     //read from mem address, modify byte, write byte (read modify write)
    WRITE2 = 3,     //illegal opcodes
    OTHER  = 4,     //note: above 4 are not applicable to every opcode
}; 

//opcode types for tick calculation
static const enum InstructionType opInstrTypes[] = {
    //0    1      2      3      4      5      6      7      8      9   
    OTHER, READ,  READ,  R_M_W, OTHER, OTHER, OTHER, READ,  OTHER, OTHER, //0
    OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, READ,  READ,  //1
    READ,  WRITE2,R_M_W, OTHER, OTHER, READ,  R_M_W, OTHER, OTHER, WRITE2,//2
    OTHER, OTHER, READ,  READ,  READ,  READ,  R_M_W, READ,  READ,  READ,  //3
    OTHER, OTHER, OTHER, OTHER, WRITE2,R_M_W, R_M_W, WRITE2,OTHER, OTHER, //4
    WRITE, READ,  READ,  OTHER, OTHER, OTHER, WRITE2,WRITE2,WRITE, WRITE, //5
    WRITE, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER                       //6
};         

//cpu cycles taken for different address modes
static const int cycles[] = {
    2,2,2,0,0,0,0,4,3,0,1,2,2,  //read
    2,3,3,0,0,0,0,4,4,0,1,2,2,  //write
    4,5,5,0,0,0,0,0,0,0,3,4,4,  //read modify write
    4,5,5,2,2,2,2,6,6,2,3,4,4,  //write2
    0,0,0,0,0,0,0,0,0,0,0,0,0,  //other
};

//table of opcode lengths for advancing m_PC
                                   //0,1,2,3,4,5,6,7,8,9,m_A,B,C,D,E,F
static const int opcodeLens[0x20] = {2,2,0,2,2,2,2,2,1,2,1,2,3,3,3,3,  //0 2 4 6 8 m_A C E
                                     2,2,0,2,2,2,2,2,1,3,1,3,3,3,3,3}; //1 3 5 7 9 B D F

CPU::CPU() {

    m_nesPPU = PPU();
    m_nesAPU = APU();

    for (int x = 0; x < 0x2000; x++) m_cpuMem[x] = 0x0;
    for (int x = 0; x < 0x800; x++) m_RAM[x] = 0x0;
    for (int x = 0; x < 8; x++) m_PS[x] = false;

    m_PRG_ROM = m_PRG_RAM = NULL;
    m_SP = 0xFD;
    m_A = m_X = m_Y = 0x0;
    m_NMI = false;
    m_IRQ = false;
    m_PS[I] = true;
    m_cpuClock = 0;
    m_controllerByte = m_storedControllerByte = m_currentControllerBit = 0;
    m_readController = false;

    return;
}

void CPU::free_pointers() {
    if (m_PRG_ROM != NULL) {
        delete [] m_PRG_ROM;
    }
    if (m_PRG_RAM != NULL) {
        delete [] m_PRG_RAM;
    }
    return;
}

bool CPU::return_controller_bit() {

    if (m_currentControllerBit < 8) {
        m_currentControllerBit++;
        bool bit;
        bit = get_bit(m_storedControllerByte, m_currentControllerBit - 1);
        return bit;
    } else {
        return true;
    }

}

u8 CPU::get_cpu_byte(u16 memAddress, bool silent) {

    if (memAddress < 0x2000) {
        return m_RAM[memAddress % 0x800];
    } else if (memAddress < 0x4000) {
        u16 address = (memAddress - 0x2000) % 8;

        if (!silent) {
            m_nesPPU.m_readFlag = address;

            if (address == 0x2) {
        
                
                /*
                //ppu/cpu synchronization hack
                if (  (m_nesPPU.m_ppuClock % (262 * 341)) < (341 * 241 + 1) && (m_cpuClock % (262 * 341)) >= (341 * 241 + 1) ) {
                    m_nesPPU.m_ppuRegisters[2] |= 0x80;
                    m_nesPPU.suppressVBL = true;
                    m_NMI = false;
                } else if (  (m_nesPPU.m_ppuClock % (262 * 341)) < (341 * 241 + 0 ) && (m_cpuClock % (262 * 341)) >= (341 * 241 + 0) ) {
                    m_nesPPU.suppressVBL = true;
                    m_NMI = false;
                } else if (  (m_nesPPU.m_ppuClock % (262 * 341)) < (341 * 261 + 1) && (m_cpuClock % (262 * 341)) >= (341 * 261 + 1) ) {
                    m_NMI = false;
                    m_nesPPU.m_ppuRegisters[2] &= 0x1F;
                    m_NMI = false;
                }
                */
                
                
                

            } else if (address == 0x4) {

                if ((m_nesPPU.m_ppuRegisters[2] & 0x80) || ((m_nesPPU.m_ppuRegisters[1] & 0x18) == 0)) {
                    return m_nesPPU.m_OAM[m_nesPPU.m_oamAddress];
                } else {
                    m_nesPPU.m_oamAddress++;
                    return m_nesPPU.m_OAM[m_nesPPU.m_oamAddress - 1];
                }

            } else if (address == 0x7) {

                //add a silent mode for this!!!!
                return m_nesPPU.return_2007(silent);
            }

        }

        return m_nesPPU.m_ppuRegisters[address];
    } else if (memAddress < 0x4020) {

        if (memAddress == 0x4015) {

            //status read conditions

        } else if (memAddress == 0x4016) {
            if (m_readController) {
                if (!silent) {
                    return return_controller_bit();
                } else {
                    if (m_currentControllerBit < 8) {
                        return get_bit(m_storedControllerByte, m_currentControllerBit);
                    } else {
                        return 1;
                    }

                }
            }
        }
        
        return m_nesAPU.m_registers[ memAddress - 0x4000 ];
    } else if (memAddress < 0x6000) {
        return m_cpuMem[memAddress - 0x4000];
    } else if (memAddress < 0x8000) {
        /* Family Basic only: PRG m_RAM, mirrored as necessary to fill entire 8 KiB window, write protectable with an external switch */
        return m_PRG_RAM[memAddress - 0x6000];
    } else {
        if (m_cpuMapper == 0) {
            return get_cpu_mapper_0(memAddress, m_numRomBanks, m_PRG_ROM);
        } else {
            std::cerr << "Fatal error, mapper not recognized in get_cpu_byte()" << std::endl;
            exit(EXIT_FAILURE);
            return 0;
        }
    }
}

void CPU::set_cpu_byte(u16 memAddress, u8 byte) {

    if (memAddress < 0x2000) {

        m_RAM[memAddress % 0x800] = byte;

    } else if (memAddress < 0x4000) {

        u16 address;
        address = (memAddress - 0x2000) % 8;
        m_nesPPU.m_writeFlag = address;
        m_nesPPU.m_ppuRegisters[address] = byte;

    } else if (memAddress < 0x4020) { 

        //APU and IO mem write behaviour
        if (memAddress == 0x4003) {
            m_nesAPU.m_lengthCounterPulse1 = lengthTable[(byte >> 3)];
        } else if (memAddress == 0x4007) {
            m_nesAPU.m_lengthCounterPulse2 = lengthTable[(byte >> 3)];
        } else if (memAddress == 0x4008) {
            m_nesAPU.m_linearCounterTriangle = byte & 0x7F;
        } else if (memAddress == 0x400B) {
            m_nesAPU.m_linearReloading = true;
            m_nesAPU.m_lengthCounterTriangle = lengthTable[(byte >> 3)];
        } else if (memAddress == 0x400F) {
            m_nesAPU.m_lengthCounterNoise = lengthTable[(byte >> 3)];
        } else if (memAddress == 0x4010) {

            //set sample frequency
            m_nesAPU.m_sampleFrequency = 1789773.0/rateTable[byte & 0xF];


        } else if (memAddress == 0x4011) {

            //direct load
            m_nesAPU.m_dmcOut = (byte & 0x7F);
        } else if (memAddress == 0x4012) {

            m_nesAPU.m_sampleAddress = 0xC000 | (byte << 6);

        } else if (memAddress == 0x4013) {

            m_nesAPU.m_sampleByteLength = (byte << 4) | 0x1;

        } else if (memAddress == 0x4014) {

            u8 OAMDMA;
            OAMDMA = byte;
            for (unsigned int x = 0; x < 0x100; x++) {
                m_nesPPU.m_OAM[m_nesPPU.m_oamAddress] = get_cpu_byte( (OAMDMA << 8) + x , false);
                m_nesPPU.m_oamAddress = (m_nesPPU.m_oamAddress + 1) & 0xFF;
            }

            if (m_cpuClock % 2 == 1) {
                //odd cycle
                m_cpuClock += 514 * 3; 
            } else {
                //even cycle
                m_cpuClock += 513 * 3; 
            }

        } else if (memAddress == 0x4015) {
            
        } else if (memAddress == 0x4016) {
            if ((byte & 0x1)) {
                //controller state is read into shift register
                m_storedControllerByte = m_controllerByte;
                m_readController = false;
            } else {
                //serial controller data can start being read starting from bit 0
                m_currentControllerBit = 0;
                m_readController = true;
            }
        }

        m_nesAPU.m_registers[memAddress - 0x4000] = byte;

    } else if (memAddress < 0x6000) {

        m_cpuMem[memAddress - 0x4000] = byte;

    } else if (memAddress < 0x8000) {

        m_PRG_RAM[memAddress - 0x6000] = byte;

    } else {
        /* memAddress >= 0x8000 && memAddress <= 0xFFFF */
        std::cerr << "Segmentation fault! Can't write to 0x" << std::hex << memAddress << std::endl;
        exit(EXIT_FAILURE);
        return;
    }

    return;
}

u16 CPU::retrieve_cpu_address(enum AddressMode mode, bool * pagePass, bool * valid,
    u8 firstByte, u8 secondByte, bool silent) {

    *pagePass = false;
    *valid = true;

    switch (mode) {

        case ZRP:
        return firstByte;

        case ZRPX:
        return ((firstByte + m_X) & 0xFF);

        case ZRPY:
        return ((firstByte + m_Y) & 0xFF);

        case ABS:
        return (firstByte | (secondByte << 8));

        case ABSX: {
            u16 before = (firstByte | (secondByte << 8));
            u16 after = ((before + m_X) & 0xFFFF);
            if ((before / 256) != (after/256)) *pagePass = true;
            return after;
        }

        case ABSY: {
            u16 before = (firstByte | (secondByte << 8));
            u16 after = ((before + m_Y) & 0xFFFF);
            if ((before / 256) != (after/256)) *pagePass = true;
            return after;
        }

        case IND: {
            u8 low = get_cpu_byte((firstByte | (secondByte << 8)), silent);
            u8 high = get_cpu_byte(((firstByte + 1) & 0xFF) | (secondByte << 8), silent);
            return ((high << 8) | low);
        }

        case INDX: {
            u8 low = get_cpu_byte((firstByte + m_X) & 0xFF, silent);
            u8 high = get_cpu_byte((firstByte + 1 + m_X) & 0xFF, silent);
            return ((high << 8) | low);
        }
        
        case INDY: {
            u8 low = (get_cpu_byte(firstByte, silent));
            u8 high = (get_cpu_byte((firstByte + 1) & 0xFF, silent));
            u16 before = (low | (high << 8));
            u16 after = ((before + m_Y) & 0xFFFF);
            if (( before/ 256) != (after/256)) *pagePass = true;
            return after;
        }

        default:
        *valid = false;
        return 0;
    }
}

void CPU::execute_next_opcode() {

    if (m_NMI) {

        m_NMI = false;
        
        u16 store;
        store = m_PC;

        u8 high = (store & 0xFF00) >> 8;
        set_cpu_byte(m_SP + 0x100, high);
        m_SP--;

        u8 low = store & 0xFF;;
        set_cpu_byte(m_SP + 0x100, low);
        m_SP--;

        set_cpu_byte(0x100 + m_SP, get_psw_byte(m_PS) | 0x10);
        m_SP--;

        m_PC = get_cpu_byte(0xFFFA, false) | (get_cpu_byte(0xFFFB, false) << 8);    //nmi handler

        
        m_cpuClock += 21;

        return;
    }

    if (m_IRQ) {

        m_IRQ = false;

        if (m_PS[I] == false) {

            u16 store;
            store = m_PC + 1;

            u8 high = (store & 0xFF00) >> 8;
            set_cpu_byte(m_SP + 0x100, high);
            m_SP--;

            u8 low = store & 0xFF;;
            set_cpu_byte(m_SP + 0x100, low);
            m_SP--;

            set_cpu_byte(0x100 + m_SP, get_psw_byte(m_PS) | 0x10);
            m_SP--;

            m_PC = get_cpu_byte(0xFFFE, false) | (get_cpu_byte(0xFFFF, false) << 8);    //m_IRQ/BRK handler

            
            m_cpuClock += 21;

            return;
        }
    }

    /* PREPARE TO EXECUTE OPCODE */

    m_cpuClock += 6;        //always spend 2 cycles fetching opcode and next byte
    u8 opcode = get_cpu_byte(m_PC, false);
    u8 iByte2 = get_cpu_byte(m_PC + 1, false);
    u8 iByte3 = get_cpu_byte(m_PC + 2, false);
    enum AddressMode opAddressMode = addressModes[opcode];
    bool pass = false;    //page boundy cross?

    u16 address = 0;
    if (opAddressMode != ACC && opAddressMode != IMM && opAddressMode != REL && opAddressMode != IMP) {
        bool valid = true;
        address = retrieve_cpu_address(opAddressMode, &pass, &valid, iByte2, iByte3, false);
        if (!valid) {
            std::cout << "Invalid address!" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    enum InstructionType instrType = opInstrTypes[opnameMap[opcode]];
    m_cpuClock += cycles[instrType * 13 + opAddressMode] * 3;

    if (instrType == READ) {
        if (pass) {
            m_cpuClock += 3;
        }
    }

    /*
    if (m_nesPPU.m_ppuRegisters[1] & 0x8) {
        if (m_nesPPU.m_ppuClock % (262 * 341 * 2) > m_cpuClock % (262 * 341 * 2)) {
            m_cpuClock++;
            m_nesPPU.suppressCpuTickSkip = true;
        }
    }
    */
    


    u8 memoryByte;
    if (opAddressMode == IMM) {
        memoryByte = iByte2;
    } else if (opAddressMode == NONE || opAddressMode == ACC
        || opAddressMode == IMP || opAddressMode == REL) {
        memoryByte = 0;
    } else {

        switch(opcode) {
            case 0x85: case 0x95: case 0x8D: case 0x9D: case 0x99:
            case 0x81: case 0x91: case 0x86: case 0x96: case 0x8E:
            case 0x84: case 0x94: case 0x8C: case 0x83: case 0x87:
            case 0x97: case 0x8F:
            memoryByte = 0;
            break;

            default:
            /* this is needed to prevent spurious reads to $2007, $4014 and
            other registers which trigger a flag that tells the PPU
            to process the read which changes the state of the ppu */
            memoryByte = get_cpu_byte(address, false);
            break;
        }
    }

    /*
    if (debug) {
        print_debug_line(address, opcode, iByte2, iByte3, opAddressMode, 
            m_PC, memoryByte, m_A, m_X, m_Y, m_SP, m_PS, m_nesPPU.m_ppuClock);
        std::cout << std::endl;
    }
    */
    
    m_PC += opcodeLens[opcode % 0x20];    //increment program counter
    if (opcode == 0xA2) m_PC += 2;        //irregular opcode

    /* END PREPERATION */
    
    switch (opcode) {

        //ADC
        case 0x69: case 0x65: case 0x75: case 0x6D: case 0x7D: case 0x79: case 0x61: case 0x71: {
            u16 total;
            total = m_A + memoryByte + m_PS[C];
            m_PS[V] = (((int8_t) m_A) + ((int8_t) memoryByte) + m_PS[C] < -128
                || ((int8_t) m_A) + ((int8_t) memoryByte) + m_PS[C] > 127);  
            m_PS[C] = (total > 255) ? true : false;
            m_A = total & 0xFF;
            m_PS[Z] = (m_A == 0) ? true : false;
            m_PS[N] = get_bit(m_A, 7);
            break;
        }

        //AND
        case 0x29: case 0x25: case 0x35: case 0x2D: case 0x3D: case 0x39: case 0x21: case 0x31: 
        m_A = m_A & memoryByte;
        m_PS[N] = get_bit(m_A, 7);
        m_PS[Z] = (m_A == 0) ? true : false;
        break;

        //ASL
        case 0x0A: case 0x06: case 0x16: case 0x0E: case 0x1E: {
            if (opcode == 0x0A) {
                m_PS[C] = get_bit(m_A, 7);
                m_A = ((m_A << 1) & 0xFE);
                m_PS[N] = get_bit(m_A, 7);
                m_PS[Z] = (m_A == 0) ? true : false;
            } else {
                m_PS[C] = get_bit(get_cpu_byte(address, true), 7);
                set_cpu_byte(address, ((get_cpu_byte(address, true) << 1) & 0xFE));
                m_PS[N] = get_bit(get_cpu_byte(address, true), 7);
                m_PS[Z] = (get_cpu_byte(address, true) == 0) ? true : false;
            }
            break;
        }

        case 0x90:             //BCC
        if (m_PS[C] == 0) {
            if ( ((m_PC + (int8_t) iByte2) / 256) != (m_PC / 256)) m_cpuClock += 3;
            m_PC += (int8_t) iByte2;
            m_cpuClock += 3;
        }
        break;
        
        case 0xB0:            //BCS
        if (m_PS[C]) {
            if ( ((m_PC + (int8_t) iByte2) / 256) != (m_PC / 256)) m_cpuClock += 3;
            m_PC += (int8_t) iByte2;
            m_cpuClock += 3;
        }
        break;
        
        case 0xF0:             //BEQ
        if (m_PS[Z]) {
            if ( ((m_PC + (int8_t) iByte2) / 256) != (m_PC / 256)) m_cpuClock += 3;
            m_PC += (int8_t) iByte2;
            m_cpuClock += 3;
        }
        break;
        
        //BIT
        case 0x24: case 0x2C: {
            u8 num;
            num = m_A & memoryByte;
            m_PS[N] = get_bit(memoryByte, 7);
            m_PS[V] = get_bit(memoryByte, 6);
            m_PS[Z] = (num == 0) ? true : false;
            break;
        }

        case 0x30:             //BMI
        if (m_PS[N]) {
            if ( ((m_PC + (int8_t) iByte2) / 256) != (m_PC / 256)) m_cpuClock += 3;
            m_PC += (int8_t) iByte2;
            m_cpuClock += 3;
        }
        break;
        
        case 0xD0:             //BNE
        if (m_PS[Z] == 0) {
            if ( ((m_PC + (int8_t) iByte2) / 256) != (m_PC / 256)) m_cpuClock += 3;
            m_PC += (int8_t) iByte2;
            m_cpuClock += 3;
        }
        break;
        
        case 0x10:             //BPL
        if (m_PS[N] == 0) {
            if ( ((m_PC + (int8_t) iByte2) / 256) != (m_PC / 256)) m_cpuClock += 3;
            m_PC += (int8_t) iByte2;
            m_cpuClock += 3;
        }
        break;
        
        case 0x00: {            //BRK
            u8 high = (m_PC & 0xFF00) >> 8;
            set_cpu_byte(0x100 + m_SP, high);
            m_SP--;
            m_cpuClock += 3;

            u8 low = m_PC & 0xFF;
            set_cpu_byte(0x100 + m_SP, low);
            m_SP--;
            m_cpuClock += 3;

            u8 memByte = get_psw_byte(m_PS);
            set_cpu_byte(0x100 + m_SP, memByte | 0x10);
            m_SP--;
            m_cpuClock += 3;

            low = get_cpu_byte(0xFFFE, false);
            m_cpuClock += 3;

            high = get_cpu_byte(0xFFFF, false);
            m_cpuClock += 3;

            m_PC = (high << 8) | low;
            break;
        }

        case 0x50:          //BVC
        if (m_PS[V] == 0) {
            if ( ((m_PC + (int8_t) iByte2) / 256) != (m_PC / 256)) m_cpuClock += 3;
            m_PC += (int8_t) iByte2;
            m_cpuClock += 3;
        }
        break;
        
        case 0x70:          //BVS
        if (m_PS[V]) {
            if ( ((m_PC + (int8_t) iByte2) / 256) != (m_PC / 256)) m_cpuClock += 3;
            m_PC += (int8_t) iByte2;
            m_cpuClock += 3;
        }
        break;
        
        case 0x18:          //CLC           
        m_PS[C] = false;
        break;
        
        case 0xD8:          //CLD           
        m_PS[D] = false;
        break;
        
        case 0x58:          //CLI           
        m_PS[I] = false;
        break;
        
        case 0xB8:          //CLV           
        m_PS[V] = false;
        break;

        //CMP
        case 0xC9: case 0xC5: case 0xD5: case 0xCD: case 0xDD: case 0xD9: case 0xC1: case 0xD1: {
            int num;
            num = m_A - memoryByte;
            m_PS[N] = get_bit(num, 7);
            m_PS[C] = (m_A >= memoryByte) ? true : false;
            m_PS[Z] = (num == 0) ? true : false;
            break;
        }

        //CPX
        case 0xE0: case 0xE4: case 0xEC: {
            int total;
            total = m_X - memoryByte;
            m_PS[N] = (total & 0x80) ? true : false;
            m_PS[C] = (m_X >= memoryByte) ? true : false;
            m_PS[Z] = (total == 0) ? 1 : 0;
            break;
        }

        //CPY
        case 0xC0: case 0xC4: case 0xCC: {
            int total;
            total = m_Y - memoryByte;
            m_PS[N] = (total & 0x80) ? true : false;
            m_PS[C] = (m_Y >= memoryByte) ? true : false;
            m_PS[Z] = (total == 0) ? 1 : 0;
            break;
        }

        //DCP
        case 0xC3: case 0xD3: case 0xC7: case 0xD7: case 0xCF: case 0xDF: case 0xDB: {
            set_cpu_byte(address, (get_cpu_byte(address, true) - 1) & 0xFF);
            memoryByte = get_cpu_byte(address, true);
            int num;
            num = m_A - memoryByte;
            m_PS[N] = get_bit(num, 7);
            m_PS[C] = (m_A >= memoryByte) ? true : false;
            m_PS[Z] = (num == 0) ? true : false;
            break;
        }

        //DEC
        case 0xC6: case 0xD6: case 0xCE: case 0xDE: 
        set_cpu_byte(address, ((get_cpu_byte(address, true) - 1) & 0xFF));
        m_PS[N] = get_bit(get_cpu_byte(address, true), 7);
        m_PS[Z] = (get_cpu_byte(address, true) == 0) ? true : false;
        break;
        
        case 0xCA:          //DEX
        m_X = (m_X - 1) & 0xFF;
        m_PS[Z] = (m_X == 0) ? true : false;
        m_PS[N] = get_bit(m_X, 7);
        break;
        
        case 0x88:          //DEY           
        m_Y = (m_Y - 1) & 0xFF;
        m_PS[Z] = (m_Y == 0) ? true : false;
        m_PS[N] = get_bit(m_Y, 7);
        break;
        
        //EOR
        case 0x49: case 0x45: case 0x55: case 0x4D: case 0x5D: case 0x59: case 0x41: case 0x51: 
        m_A = m_A ^ memoryByte;
        m_PS[N] = get_bit(m_A, 7);
        m_PS[Z] = (m_A == 0) ? true : false;
        break;
        
        //INC
        case 0xE6: case 0xF6: case 0xEE: case 0xFE: 
        set_cpu_byte(address, ((get_cpu_byte(address, true) + 1) & 0xFF));
        m_PS[N] = get_bit(get_cpu_byte(address, true), 7);
        m_PS[Z] = (get_cpu_byte(address, true) == 0) ? true : false;
        break;
        
        case 0xE8:              //INX           
        m_X = (m_X + 1) & 0xFF;
        m_PS[Z] = (m_X == 0) ? true : false;
        m_PS[N] = get_bit(m_X, 7);
        break;
        
        case 0xC8:              //INY           
        m_Y = (m_Y + 1) & 0xFF;
        m_PS[Z] = (m_Y == 0) ? true : false;
        m_PS[N] = get_bit(m_Y, 7);
        break;

        //ISB
        case 0xE3: case 0xE7: case 0xF7: case 0xFB: case 0xEF: case 0xFF: case 0xF3: {     
            set_cpu_byte(address, (get_cpu_byte(address, true) + 1) & 0xFF);
            memoryByte = get_cpu_byte(address, true);
            int total;
            total = m_A - memoryByte - (!m_PS[C]);
            m_PS[V] = (((int8_t) m_A) - ((int8_t) memoryByte) - (!m_PS[C]) < -128 
                || ((int8_t) m_A) - ((int8_t) memoryByte) - (!m_PS[C]) > 127);
            m_A = total & 0xFF;
            m_PS[C] = (total >= 0) ? true : false;
            m_PS[N] = get_bit(total, 7);
            m_PS[Z] = (m_A == 0) ? 1 : 0;
            break;
        }

        //JMP
        case 0x4C: case 0x6C: 
        m_PC = address;
        m_cpuClock += (opAddressMode == ABS) ? 3 : 9;
        break;

        case 0x20: {            //JSR
            m_cpuClock += 3;

            u16 store;
            store = m_PC;
            
            u8 high = (store & 0xFF00) >> 8;
            set_cpu_byte(m_SP + 0x100, high);
            m_SP--;
            m_cpuClock += 3;

            u8 low = store & 0xFF;;
            set_cpu_byte(m_SP + 0x100, low);
            m_SP--;
            m_cpuClock += 3;

            m_PC = (iByte2 | (iByte3 << 8));
            m_cpuClock += 3;
            break;
        }

        //LAX
        case 0xA3: case 0xA7: case 0xAF: case 0xB3: case 0xB7: case 0xBF:  
        m_A = memoryByte;
        m_X = memoryByte;
        m_PS[N] = get_bit(m_A, 7);
        m_PS[Z] = (m_A == 0) ? true : false;
        break;
        
        //LDA
        case 0xA9: case 0xA5: case 0xB5: case 0xAD: case 0xBD: case 0xB9: case 0xA1: case 0xB1: 
        m_A = memoryByte;
        m_PS[N] = get_bit(m_A, 7);
        m_PS[Z] = (m_A == 0) ? true : false;
        break;
        
        //LDX
        case 0xA2: case 0xA6: case 0xB6: case 0xAE: case 0xBE: 
        m_X = memoryByte;
        m_PS[N] = get_bit(m_X, 7);
        m_PS[Z] = (m_X == 0) ? true : false;
        break;
        
        //LDY
        case 0xA0: case 0xA4: case 0xB4: case 0xAC: case 0xBC: 
        m_Y = memoryByte;
        m_PS[N] = get_bit(m_Y, 7);
        m_PS[Z] = (m_Y == 0) ? true : false;
        break;

        //LSR
        case 0x4A: case 0x46: case 0x56: case 0x4E: case 0x5E: {
            m_PS[N] = 0;
            if (opcode == 0x4A) {
                m_PS[C] = get_bit(m_A, 0);
                m_A = (m_A >> 1) & 0x7F;
                m_PS[Z] = (m_A == 0) ? true : false;
            } else {
                m_PS[C] = get_bit(get_cpu_byte(address, true), 0);
                set_cpu_byte(address, (get_cpu_byte(address, true) >> 1) & 0x7F);
                m_PS[Z] = (get_cpu_byte(address, true) == 0) ? true : false;
            }
            break;
        }

        //NOP
        case 0x04: case 0x44: case 0x64: case 0x0C: case 0x14: case 0x34: case 0x54: case 0x74:
        case 0xD4: case 0xF4: case 0x1A: case 0x3A: case 0x5A: case 0x7A: case 0xDA: case 0xFA:
        case 0x80: case 0x1C: case 0x3C: case 0x5C: case 0x7C: case 0xDC: case 0xFC: case 0xEA:  
        break;
        
        //ORA
        case 0x09: case 0x05: case 0x15: case 0x0D: case 0x1D: case 0x19: case 0x01: case 0x11: 
        m_A = (m_A | memoryByte);
        m_PS[N] = get_bit(m_A, 7);
        m_PS[Z] = (m_A == 0) ? true : false;
        break;
        
        case 0x48:              //PHA
        set_cpu_byte(m_SP + 0x100, m_A);
        m_SP--;
        m_cpuClock += 3;
        break;

        case 0x08:                 //PHP
        set_cpu_byte(m_SP + 0x100, (get_psw_byte(m_PS) | 0x10));
        m_SP--;
        m_cpuClock += 3;
        break;
        
        case 0x68:              //PLA
        m_SP++;
        m_cpuClock += 3;
        m_A = get_cpu_byte(m_SP + 0x100, true);
        m_cpuClock += 3;
        m_PS[N] = get_bit(m_A, 7);
        m_PS[Z] = (m_A == 0) ? true : false;
        break;
        
        case 0x28:                 //PLP
        m_SP++;
        m_cpuClock += 3;
        get_psw_from_byte(m_PS, get_cpu_byte(m_SP + 0x100, true));
        m_cpuClock += 3;
        break;
        
        //RLA
        case 0x23: case 0x27: case 0x2F: case 0x33: case 0x37: case 0x3B: case 0x3F: {
            bool store;
            store = get_bit(get_cpu_byte(address, true), 7);
            set_cpu_byte(address, (get_cpu_byte(address, true) << 1) & 0xFE);
            set_cpu_byte(address, get_cpu_byte(address, true) | m_PS[C]);
            m_PS[C] = store;
            m_A &= get_cpu_byte(address, true);
            m_PS[Z] = (m_A == 0) ? true : false;
            m_PS[N] = get_bit(m_A, 7);
            break;
        }

        //ROL
        case 0x2A: case 0x26: case 0x36: case 0x2E: case 0x3E: {
            bool store;
            if (opcode == 0x2A) {
                store = get_bit(m_A, 7);
                m_A = (m_A << 1) & 0xFE;
                m_A |= m_PS[C];
                m_PS[Z] = (m_A == 0) ? true : false;
                m_PS[N] = get_bit(m_A, 7);
            } else {
                store = get_bit(get_cpu_byte(address, true), 7);
                set_cpu_byte(address, (get_cpu_byte(address, true) << 1) & 0xFE);
                set_cpu_byte(address, get_cpu_byte(address, true) | m_PS[C]);
                m_PS[Z] = (get_cpu_byte(address, true) == 0) ? true : false;
                m_PS[N] = get_bit(get_cpu_byte(address, true), 7);
            }
            m_PS[C] = store;
            break;
        }

        //ROR
        case 0x6A: case 0x66: case 0x76: case 0x6E: case 0x7E: {
            bool store;
            if (opcode == 0x6A) {
                store = get_bit(m_A, 0);
                m_A = (m_A >> 1) & 0x7F;
                m_A |= (m_PS[C] ? 0x80 : 0x0);
                m_PS[Z] = (m_A == 0) ? true : false;
                m_PS[N] = get_bit(m_A, 7);
            } else {
                store = get_bit(get_cpu_byte(address, true), 0);
                set_cpu_byte(address, (get_cpu_byte(address, true) >> 1) & 0x7F);
                set_cpu_byte(address, get_cpu_byte(address, true) | (m_PS[C] ? 0x80 : 0x0));
                m_PS[Z] = (get_cpu_byte(address, true) == 0) ? true : false;
                m_PS[N] = get_bit(get_cpu_byte(address, true), 7);
            }
            m_PS[C] = store;
            break;
        }

        //RRA
        case 0x63: case 0x67: case 0x6F: case 0x73: case 0x77: case 0x7B: case 0x7F: {
            bool store;
            store = get_bit(get_cpu_byte(address, true), 0);
            set_cpu_byte(address, (get_cpu_byte(address, true) >> 1) & 0x7F);
            set_cpu_byte(address, get_cpu_byte(address, true) | (m_PS[C] ? 0x80 : 0x0));
            m_PS[C] = store;
            u8 memByte;
            memByte = get_cpu_byte(address, true);
            u16 total;
            total = m_A + memByte + m_PS[C];
            m_PS[V] = (((int8_t) m_A) + ((int8_t) memByte) + m_PS[C] < -128 
                || ((int8_t) m_A) + ((int8_t) memByte) + m_PS[C] > 127);
            m_PS[C] = (total > 255) ? true : false;
            m_A = total & 0xFF;
            m_PS[Z] = (m_A == 0) ? true : false;
            m_PS[N] = get_bit(m_A, 7);
            break;
        }

        case 0x40: {                //RTI
            m_SP++;
            m_cpuClock += 3;

            u8 memByte;
            memByte = get_cpu_byte(m_SP + 0x100, true);
            get_psw_from_byte(m_PS, memByte);

            m_SP++;
            m_cpuClock += 3;
            u16 low = get_cpu_byte(m_SP + 0x100, true);

            m_SP++;
            m_cpuClock += 3;
            u16 high = get_cpu_byte(m_SP + 0x100, true) << 8;
            m_cpuClock += 3;

            m_PC = high | low;
            break;
        }

        case 0x60: {                //RTS
            m_SP++;
            m_cpuClock += 3;

            u16 low = get_cpu_byte(m_SP + 0x100, true);
            m_SP++;
            m_cpuClock += 3;

            u16 high = get_cpu_byte(m_SP + 0x100, true) << 8;
            m_cpuClock += 3;
            m_PC = (high | low);

            m_PC++;
            m_cpuClock += 3;
            break;
        }

        //SBC
        case 0xE9: case 0xE5: case 0xF5: case 0xED: case 0xFD: case 0xF9: case 0xE1: case 0xF1: case 0xEB: {
            int total;
            total = m_A - memoryByte - (!m_PS[C]);
            m_PS[V] = (((int8_t) m_A) - ((int8_t) memoryByte) - (!m_PS[C]) < -128 
                || ((int8_t) m_A) - ((int8_t) memoryByte) - (!m_PS[C]) > 127);
            m_A = total & 0xFF;
            m_PS[C] = (total >= 0) ? true : false;
            m_PS[N] = get_bit(total, 7);
            m_PS[Z] = (m_A == 0) ? 1 : 0;
            break;
        }

        case 0x38:              //SEC
        m_PS[C] = true;
        break;
        
        case 0xF8:              //SED
        m_PS[D] = true;
        break;
        
        case 0x78:              //SEI
        m_PS[I] = true;
        break;
        
        //SAX
        case 0x83: case 0x87: case 0x97: case 0x8F:
        set_cpu_byte(address, m_A & m_X);
        break;
        
        //*SLO
        case 0x03: case 0x07: case 0x0F: case 0x13: case 0x17: case 0x1B: case 0x1F: 
        m_PS[C] = get_bit(get_cpu_byte(address, true), 7);
        set_cpu_byte(address, ((get_cpu_byte(address, true) << 1) & 0xFE));
        m_A |= get_cpu_byte(address, true);
        m_PS[N] = get_bit(m_A, 7);
        m_PS[Z] = (m_A == 0) ? true : false;
        break;

        //SRE
        case 0x43: case 0x47: case 0x4F: case 0x53: case 0x57: case 0x5B: case 0x5F: 
        m_PS[C] = get_bit(get_cpu_byte(address, true), 0);
        set_cpu_byte(address, (get_cpu_byte(address, true) >> 1) & 0x7F);
        m_A ^= get_cpu_byte(address, true);
        m_PS[N] = get_bit(m_A, 7);
        m_PS[Z] = (m_A == 0) ? true : false;
        break;

        //STA
        case 0x85: case 0x95: case 0x8D: case 0x9D: case 0x99: case 0x81: case 0x91: 
        set_cpu_byte(address, m_A);
        break;
        
        //STX
        case 0x86: case 0x96: case 0x8E: 
        set_cpu_byte(address, m_X);
        break;

        //STY
        case 0x84: case 0x94: case 0x8C: 
        set_cpu_byte(address, m_Y);
        break;

        case 0xAA:              //TAX           
        m_X = m_A;
        m_PS[N] = get_bit(m_X, 7);
        m_PS[Z] = (m_X == 0) ? true : false;
        break;
        
        case 0xA8:              //TAY           
        m_Y = m_A;
        m_PS[N] = get_bit(m_Y, 7);
        m_PS[Z] = (m_Y == 0) ? true : false;
        break;

        case 0xBA:              //TSX           
        m_X = m_SP;
        m_PS[N] = get_bit(m_X, 7);
        m_PS[Z] = (m_X == 0) ? true : false;
        break;

        case 0x8A:              //TXA           
        m_A = m_X;
        m_PS[N] = get_bit(m_A, 7);
        m_PS[Z] = (m_A == 0) ? true : false;
        break;
        
        case 0x9A:              //TXS           
        m_SP = m_X;
        break;
        
        case 0x98:              //TYA           
        m_A = m_Y;
        m_PS[N] = get_bit(m_A, 7);
        m_PS[Z] = (m_A == 0) ? true : false;
        break;
        
        default: 
        std:: cout << " Unrecognized opcode : " << std::hex << (int) opcode << std::endl;
        exit(EXIT_FAILURE);
        break;
    }

    return;
}

u8 get_psw_byte(bool * m_PS) {
    u8 P;
    P = 0x20;
    if (m_PS[C]) P |= 0x1;
    if (m_PS[Z]) P |= 0x2;
    if (m_PS[I]) P |= 0x4;
    if (m_PS[D]) P |= 0x8;
    if (m_PS[V]) P |= 0x40;
    if (m_PS[N]) P |= 0x80;
    return P;
}

void get_psw_from_byte(bool * m_PS, u8 byte) {
    m_PS[N] = get_bit(byte, 7);
    m_PS[V] = get_bit(byte, 6);
    m_PS[D] = get_bit(byte, 3);
    m_PS[I] = get_bit(byte, 2);
    m_PS[Z] = get_bit(byte, 1);
    m_PS[C] = get_bit(byte, 0);
    return;
}

