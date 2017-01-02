#include <iostream>
#include <cstring>
#include <iomanip>
#include <string>

#include "debugger.hpp"

//opcode mnemonics for debugging
static const char * opnames[] = {
    //0     1      2      3      4      5      6      7      8      9
    "$$$", "ADC", "AND", "ASL", "BCC", "BCS", "BEQ", "BIT", "BMI", "BNE", //0
    "BPL", "BRK", "BVC", "BVS", "CLC", "CLD", "CLI", "CLV", "CMP", "CPX", //1
    "CPY","*DCP", "DEC", "DEX", "DEY", "EOR", "INC", "INX", "INY","*ISB", //2
    "JMP", "JSR","*LAX", "LDA", "LDX", "LDY", "LSR", "NOP","*NOP", "ORA", //3
    "PHA", "PHP", "PLA", "PLP","*RLA", "ROL", "ROR","*RRA", "RTI", "RTS", //4
   "*SAX","*SBC", "SBC", "SEC", "SED", "SEI","*SLO","*SRE", "STA", "STX", //5
    "STY", "TAX", "TAY", "TSX", "TXA", "TXS", "TYA"                       //6
};   

Debugger::Debugger(NES * nes) {
    log = false;
    toDisassemble = 0;
    ignoreNextBreaks = false;
    this->nesSystem = nes;
}

bool Debugger::perform_events(bool * breakpointHit) {

    if (log) {
        peek_next_instruction(true, breakpointHit);
    } else if (toDisassemble > 0) {

        toDisassemble--;
        peek_next_instruction(true, breakpointHit);
        if (toDisassemble == 0) {
            return true;
        }

    } else {

        peek_next_instruction(false, breakpointHit);

    }

    if (*breakpointHit) {
        toDisassemble = 0;
    }

    return false;
}

void Debugger::print_hex(u16 num, int width) {
    std::cout << std::hex << std::uppercase << std::setfill('0') << std::setw(width) << (int) num;
    return;
}

int Debugger::debug_print_val(enum AddressMode mode, int firstByte, int secondByte) {

    switch (mode) {
        case ABS:
        std::cout << '$' << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << secondByte << std::setfill('0') << std::setw(2) << firstByte;
        return 5;

        case ABSX:
        std::cout << '$' << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << secondByte << std::setfill('0') << std::setw(2) << firstByte << ",X";
        return 7;

        case ABSY:
        std::cout << '$' << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << secondByte << std::setfill('0') << std::setw(2) << firstByte << ",Y";
        return 7;

        case ACC:
        std::cout << "A";
        return 1;

        case IMM:
        std::cout << "#$" << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << firstByte;
        return 4;

        case IMP:
        return 0;

        case IND:
        std::cout << "($" << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << secondByte << std::setfill('0') << std::setw(2) << firstByte << ')';
        return 7;

        case INDX:
        std::cout << "($" << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << firstByte << ",X)";
        return 7;

        case INDY:
        std::cout << "($" << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << firstByte << "),Y";
        return 7;

        case REL:
        return 0;

        case ZRP:
        std::cout << '$' << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << firstByte;
        return 3;

        case ZRPX:
        std::cout << '$' << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << firstByte << ",X";
        return 5;
        
        case ZRPY:
        std::cout << '$' << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << firstByte << ",Y";
        return 5;
        
        default:
        std::cerr << "Unrecognized address mode" << std::endl;
        return 0;
    }
}

void Debugger::peek_next_instruction(bool print, bool * pauseExecution) {

    bool pass = false;

    u16 m_PC = nesSystem->m_nesCPU.m_PC;

    u8 opcode = nesSystem->m_nesCPU.get_cpu_byte(m_PC, true);
    u8 iByte2 = nesSystem->m_nesCPU.get_cpu_byte(m_PC + 1, true);
    u8 iByte3 = nesSystem->m_nesCPU.get_cpu_byte(m_PC + 2, true);
    enum AddressMode opAddressMode = addressModes[opcode];


    u16 address = 0;
    if (opAddressMode != ACC && opAddressMode != IMM && opAddressMode != REL && opAddressMode != IMP) {
        address = nesSystem->m_nesCPU.retrieve_cpu_address(opAddressMode, &pass, iByte2, iByte3, true);
    }

    u8 m_A = nesSystem->m_nesCPU.m_A;
    u8 m_X = nesSystem->m_nesCPU.m_X;
    u8 m_Y = nesSystem->m_nesCPU.m_Y;
    u8 m_SP = nesSystem->m_nesCPU.m_SP;
    bool * m_PS = nesSystem->m_nesCPU.m_PS;
    int m_ppuClock = nesSystem->m_nesCPU.m_nesPPU.m_ppuClock;

    u8 memByte;
    if (opAddressMode == IMM) {
        memByte = iByte2;
    } else if (opAddressMode == NONE || opAddressMode == ACC
        || opAddressMode == IMP || opAddressMode == REL) {
        memByte = 0;
    } else {

        switch(opcode) {
            case 0x85: case 0x95: case 0x8D: case 0x9D: case 0x99:
            case 0x81: case 0x91: case 0x86: case 0x96: case 0x8E:
            case 0x84: case 0x94: case 0x8C: case 0x83: case 0x87:
            case 0x97: case 0x8F:
            memByte = 0;
            break;

            default:
            /* this is needed to prevent spurious reads to $2007, $4014 and
            other registers which trigger a flag that tells the PPU
            to process the read which changes the state of the ppu */
            memByte = nesSystem->m_nesCPU.get_cpu_byte(address, true);
            break;
        }
    }

    if (ignoreNextBreaks) {
        ignoreNextBreaks = false;
    } else {
        for (unsigned int i = 0; i < breakpoints.size(); i++) {
            if (breakpoints[i] == m_PC) {
                *pauseExecution = true;
                ignoreNextBreaks = true;
                return;
            }
        }

        for (unsigned int i = 0; i < watchpoints.size(); i++) {
            if (address == watchpoints[i]) {
                *pauseExecution = true;
                ignoreNextBreaks = true;
                return;
            }
        }

    }


    if (!print) {
        return;
    }

    std::cout << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << (int) m_PC << "  ";

    if (opAddressMode == IMP || opAddressMode == ACC) {
        print_hex(opcode, 2);
        std::cout << "       ";
    } else if (opAddressMode == ZRP || opAddressMode == ZRPX || opAddressMode == ZRPY
        || opAddressMode == REL || opAddressMode == IMM
        || opAddressMode == INDX || opAddressMode == INDY) {
        print_hex(opcode, 2);
        std::cout << ' ';
        print_hex(iByte2, 2);
        std::cout << "    ";
    } else if (opAddressMode == ABS || opAddressMode == ABSX
        || opAddressMode == ABSY || opAddressMode == IND) {
        print_hex(opcode, 2);
        std::cout << ' ';
        print_hex(iByte2, 2);
        std::cout << ' ';
        print_hex(iByte3, 2);
        std::cout << ' ';
    } else {
        std::cout << "         ";
    }

    if (strlen(opnames[opnameMap[opcode]]) == 3) {
        std::cout << ' ';
    }

    int whiteSpace;
    whiteSpace = 28;

    std::cout << opnames[opnameMap[opcode]] << ' ';

    if (opAddressMode == REL) {
        std::cout << '$';
        std::cout << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << (int) m_PC + (int8_t) iByte2 + 2;
        whiteSpace -= 5;
    } else {
        int addressLen;

        addressLen = debug_print_val(addressModes[opcode], iByte2, iByte3);
        whiteSpace -= addressLen;
    }

    if (opnameMap[opcode] == 58 || opnameMap[opcode] == 59 || opnameMap[opcode] == 50 || 
        opnameMap[opcode] == 60 || opnameMap[opcode] == 7  || opnameMap[opcode] == 34 ||
        opnameMap[opcode] == 35 || opnameMap[opcode] == 33 || opnameMap[opcode] == 32 ||
        opnameMap[opcode] == 39 || opnameMap[opcode] == 2  || opnameMap[opcode] == 25 ||
        opnameMap[opcode] == 1  || opnameMap[opcode] == 18 || opnameMap[opcode] == 51 ||
        opnameMap[opcode] == 52 || opnameMap[opcode] == 19 || opnameMap[opcode] == 20 ||
        opnameMap[opcode] == 36 || opnameMap[opcode] == 3  || opnameMap[opcode] == 45 ||
        opnameMap[opcode] == 46 || opnameMap[opcode] == 26 || opnameMap[opcode] == 22 ||
        opnameMap[opcode] == 30 || opnameMap[opcode] == 38 || opnameMap[opcode] == 21 ||
        opnameMap[opcode] == 29 || opnameMap[opcode] == 56 || opnameMap[opcode] == 47 ||
        opnameMap[opcode] == 57 || opnameMap[opcode] == 44) {

        if ((opAddressMode == ZRP || opAddressMode == ABS) && (opnameMap[opcode] != 30)) {
            whiteSpace-=5;
            std::cout << " = " << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int) memByte;
        } else if (opAddressMode == INDX) {
            whiteSpace -= 17;

            std::cout << " @ " << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int) ((iByte2 + m_X) & 0xFF);
            std::cout << " = " << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << address;
            std::cout << " = " << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int) memByte;
        } else if (opAddressMode == INDY) {

            whiteSpace -= 19;
            std::cout << " = " << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << ((address - m_Y) & 0xFFFF);
            std::cout << " @ " << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << address;
            std::cout << " = " << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int) memByte;
        } else if (opAddressMode == IND) {

            whiteSpace-=7;
            std::cout << " = " << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << address;


        } else if (opAddressMode == ABSX || opAddressMode == ABSY) {
            whiteSpace -= 12;

            std::cout << " @ " << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << address;
            std::cout << " = " << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int) memByte;

        } else if (opAddressMode == ZRPX || opAddressMode == ZRPY) {

            whiteSpace -= 10;

            std::cout << " @ " << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int) address;
            std::cout << " = " << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int) memByte;


        }
    }

    for (int x = 0; x < whiteSpace; x++) {
        std::cout << ' ';
    }

    std::cout << "A:" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int) m_A << ' ';
    std::cout << "X:" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int) m_X << ' ';
    std::cout << "Y:" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int) m_Y << ' ';
    std::cout << "P:" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int) get_psw_byte(m_PS) << ' ';
    std::cout << "SP:" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int) m_SP << ' ';

    std::cout << "CYC:";
    
    int count = m_ppuClock % 341;
    int scanLines = (m_ppuClock % (341 * 262)) / 341; 

    if (count < 10) {
        std::cout << "  ";
    } else if (count < 100) {
        std::cout << " ";
    }
    std::cout << std::dec << count;

    
    std::cout << " SL:";
    std::cout << std::dec << scanLines;
    std::cout << std::endl;
    
    return;

}

bool valid_digits(std::string string, bool hex, int start, int end) {

    if (hex) {
        for (int i = start; i < end; i++) {

            if (!(string[i] >= '0' && string[i] <= '9') && !(string[i] >= 'A' && string[i] <= 'F')) {
                return false;
            }
        }
    } else {

        for (int i = start; i < end; i++) {
            if (string[i] < '0' || string[i] > '9') {
                return false;
            }
        }
    }

    return true;
}

bool valid_digits(std::string string, bool hex, int start) {
    return valid_digits(string, hex, start, string.length());
}

bool Debugger::shell(bool * quit, bool * draw, bool * focus) {

    /*
    add features:
    set controller input until 'x'

    view code

    waveform visualizer
    change speed
    breakpoints (mem reads, writes, until next frame, NMI, IRQ, etc)
    memory view
    disassembler
    nametables and pattern tables view

    stack related breakpoints?


    */

    std::cout << "XeNES (0x";
    std::cout << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << nesSystem->m_nesCPU.m_PC;
    std::cout << ") > ";

    std::string input = "";
    getline(std::cin, input);

    if (input.compare("l 1") == 0 || input.compare("l1") == 0 || input.compare("l true") == 0 ) {
        log = true;
    } else if (input.compare("l 0") == 0 || input.compare("l0") == 0 || input.compare("l false") == 0) {
        log = false;
    } else if (input.compare("run") == 0 || input.compare("r") == 0) {
        *focus = true;
        return true;
    } else if (input.compare("q") == 0 || input.compare("quit") == 0) {
        *quit = true;
        return true;
    } else if ((input[0] == 'd' || input[0] == 'j' || input[0] == 'm') && input[1] == ' ') {


        if (input.length() == 1 || input.length() == 2) {
            std::cout << "Invalid command" << std::endl;
            return false;
        } else {

            if (input[0] == 'd') {

                if (!valid_digits(input, false, 2)) {
                    std::cout << "Invalid number" << std::endl;
                    return false;
                }

                toDisassemble = std::stoi (input.substr(2, std::string::npos), nullptr, 10);
                return true;
            } else if (input[0] == 'j') {

                if (!valid_digits(input, true, 2)) {
                    std::cout << "Invalid number" << std::endl;
                    return false;
                }

                nesSystem->m_nesCPU.m_PC = std::stoi (input.substr(2, std::string::npos), nullptr, 16);
                return false;
            } else {

                if (!valid_digits(input, true, 2, 6)) {
                    std::cout << "Invalid first number" << std::endl;
                    return false;
                }

                uint16_t memLoc = std::stoi (input.substr(2, 6), nullptr, 16);

                if (input[6] != ' ') {
                    std::cout << "Invalid command" << std::endl;
                    return false;
                }

                if (!valid_digits(input, false, 7)) {
                    std::cout << "Invalid second number" << std::endl;
                    return false;
                }

                int numBytes = std::stoi(input.substr(7, std::string::npos), nullptr, 10);

                int numRows = ((numBytes / 16) + 1);

                memLoc &= ~0xF;

                for (int x = 0; x < numRows; x++) {

                    print_hex(memLoc + x * 0x10, 4);
                    std::cout << "   ";

                    for (int y = 0; y < 16; y++) {
                        print_hex(nesSystem->m_nesCPU.get_cpu_byte(memLoc + x * 0x10 + y, true), 2);
                        std::cout << " ";
                    }

                    std::cout << std::endl;


                }
            }
        }

    } else if ((input[0] == 'b' || input[0] == 'w') && (input[1] == 'a' || input[1] == 'r')) {

        if (input.length() != 7) {
            std::cout << "Invalid command" << std::endl;
            return false;
        }

        if (!valid_digits(input, true, 3)) {
            std::cout << "Invalid number" << std::endl;
            return false;
        }

        uint16_t num = std::stoi (input.substr(3, std::string::npos), nullptr, 16);

        if (input[1] == 'a') {

            if (input[0] == 'b') {
                breakpoints.push_back(num);
            } else {
                watchpoints.push_back(num);
            }

        } else {

            if (input[0] == 'b') {

                bool found = false;

                for (unsigned int i = 0; i < breakpoints.size(); i++) {
                    if (breakpoints[i] == num) {
                        breakpoints.erase(breakpoints.begin() + i);
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    std::cout << "Breakpoint not found" << std::endl;
                }

            } else {

                bool found = false;

                for (unsigned int i = 0; i < watchpoints.size(); i++) {
                    if (watchpoints[i] == num) {
                        watchpoints.erase(watchpoints.begin() + i);
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    std::cout << "Watchpoint not found" << std::endl;
                }
            }

            return false;
        }

    } else if ((input[0] == 'b' || input[0] == 'w') && (input[1] == 'v')) {

        std::vector<uint16_t> temp = (input[0] == 'b') ? breakpoints : watchpoints;

        if (temp.size() != 0) {
            for (int i = 0; i < (int) temp.size(); i++) {

                std::cout << "0x";
                print_hex(temp[i], 4);

                if (i != (int) temp.size() - 1) {
                    std::cout << ", ";  
                }
                

            }
            std::cout << std::endl;
        } else {
            std::cout << "None" << std::endl;
        }

    } else if (input.compare("h") == 0 || input.compare("help") == 0) {

        std::cout << "print help prompt" << std::endl;

    } else {

        if (input.compare("") != 0) {
            std::cout << "Command not recognized" << std::endl;
        }

    }

    return false;


}
