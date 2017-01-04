#include <iostream>
#include <cstring>
#include <iomanip>

#include <string>
#include <sstream>


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
    crashImminent = false;
    this->nesSystem = nes;
}

bool Debugger::perform_events() {

    bool temp = true;

    if (check_breaks()) {
        toDisassemble = 0;
        return true;
    }

    if (toDisassemble > 0) {

        print_next_instr(nesSystem->m_nesCPU.m_PC, true, &temp);

        toDisassemble--;
        if (toDisassemble == 0) {
            return true;
        }

    } else if (log) {
        print_next_instr(nesSystem->m_nesCPU.m_PC, true, &temp);
    }

    if (!temp) {
        crashImminent = true;
        return true;
    }

    return false;
}

void print_hex(u16 num, int width) {
    std::cout << std::hex << std::uppercase << std::setfill('0') << std::setw(width) << (int) num;
    return;
}

int debug_print_val(enum AddressMode mode, int firstByte, int secondByte) {

    switch (mode) {
        case ABS:
        std::cout << '$';
        print_hex(secondByte, 2);
        print_hex(firstByte, 2);
        return 5;

        case ABSX:
        std::cout << '$';
        print_hex(secondByte, 2);
        print_hex(firstByte, 2);
        std::cout <<  ",X";
        return 7;

        case ABSY:
        std::cout << '$';
        print_hex(secondByte, 2);
        print_hex(firstByte, 2);
        std::cout <<  ",Y";
        return 7;

        case ACC:
        std::cout << "A";
        return 1;

        case IMM:
        std::cout << "#$";
        print_hex(firstByte, 2);
        return 4;

        case IMP:
        return 0;

        case IND:
        std::cout << "($";
        print_hex(secondByte, 2);
        print_hex(firstByte, 2);
        std::cout << ')';
        return 7;

        case INDX:
        std::cout << "($";
        print_hex(firstByte, 2);
        std::cout << ",X)";

        return 7;

        case INDY:
        std::cout << "($";
        print_hex(firstByte, 2);
        std::cout << "),Y";
        return 7;

        case REL:
        return 0;

        case ZRP:
        std::cout << '$';
        print_hex(firstByte, 2);
        return 3;

        case ZRPX:
        std::cout << '$';
        print_hex(firstByte, 2);
        std::cout << ",X";
        return 5;
        
        case ZRPY:
        std::cout << '$';
        print_hex(firstByte, 2);
        std::cout << ",Y";
        return 5;
        
        default:
        std::cerr << "Unrecognized address mode" << std::endl;
        return 0;
    }
}

int print_instruction(u8 opcode, u8 instruction2, u8 instruction3, u8 memByte, u8 X, 
    u8 Y, u16 address, u16 PC, enum AddressMode opAddressMode, bool viewIndirect) {
    if (opAddressMode == IMP || opAddressMode == ACC) {
        print_hex(opcode, 2);
        std::cout << "       ";
    } else if (opAddressMode == ZRP || opAddressMode == ZRPX || opAddressMode == ZRPY
        || opAddressMode == REL || opAddressMode == IMM
        || opAddressMode == INDX || opAddressMode == INDY) {
        print_hex(opcode, 2);
        std::cout << ' ';
        print_hex(instruction2, 2);
        std::cout << "    ";
    } else if (opAddressMode == ABS || opAddressMode == ABSX
        || opAddressMode == ABSY || opAddressMode == IND) {
        print_hex(opcode, 2);
        std::cout << ' ';
        print_hex(instruction2, 2);
        std::cout << ' ';
        print_hex(instruction3, 2);
        std::cout << ' ';
    } else {
        std::cout << "         ";
    }

    if (strlen(opnames[opnameMap[opcode]]) == 3) {
        std::cout << ' ';
    }

    int whiteSpace = 28;

    std::cout << opnames[opnameMap[opcode]] << ' ';

    if (opAddressMode == REL) {
        std::cout << '$';
        print_hex((int) PC + (int8_t) instruction2 + 2, 4);
        whiteSpace -= 5;
    } else {
        int addressLen;
        addressLen = debug_print_val(addressModes[opcode], instruction2, instruction3);
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
            whiteSpace -= 5;
            std::cout << " = ";
            print_hex(memByte, 2);
        } else if (opAddressMode == INDX) {
            if (viewIndirect) {
                std::cout << " @ ";
                print_hex(((instruction2 + X) & 0xFF), 2);
                std::cout << " = ";
                print_hex(address, 4);
                std::cout << " = ";
                print_hex(memByte, 2);
                whiteSpace -= 17;
            }
        } else if (opAddressMode == INDY) {
            if (viewIndirect) {
                std::cout << " = ";
                print_hex(((address - Y) & 0xFFFF), 4);
                std::cout << " @ ";
                print_hex(address, 4);
                std::cout << " = ";
                print_hex(memByte, 2);
                whiteSpace -= 19;
            }
        } else if (opAddressMode == IND) {
            whiteSpace -= 7;
            std::cout << " = ";
            print_hex(address, 4);
        } else if (opAddressMode == ABSX || opAddressMode == ABSY) {
            whiteSpace -= 12;
            std::cout << " @ ";
            print_hex(address, 4);
            std::cout << " = ";
            print_hex(memByte, 2);
        } else if (opAddressMode == ZRPX || opAddressMode == ZRPY) {
            whiteSpace -= 10;
            std::cout << " @ ";
            print_hex(address, 2);
            std::cout << " = ";
            print_hex(memByte, 2);
        }
    }

    return whiteSpace;
}

void print_registers(u8 A, u8 X, u8 Y, u8 PS, u8 SP) {
    std::cout << "A:";
    print_hex(A, 2);
    std::cout << ' ';
    std::cout << "X:";
    print_hex(X, 2);
    std::cout << ' ';
    std::cout << "Y:";
    print_hex(Y, 2);
    std::cout << ' ';
    std::cout << "P:";
    print_hex(PS, 2);
    std::cout << ' ';
    std::cout << "SP:";
    print_hex(SP, 2);
    std::cout << ' ';
}

bool Debugger::check_breaks() {

    u16 PC = nesSystem->m_nesCPU.m_PC;
    u8 opcode = nesSystem->m_nesCPU.get_cpu_byte(PC, true);
    u8 instruction2 = nesSystem->m_nesCPU.get_cpu_byte(PC + 1, true);
    u8 instruction3 = nesSystem->m_nesCPU.get_cpu_byte(PC + 2, true);
    enum AddressMode opAddressMode = addressModes[opcode];

    u16 address = 0;
    if (opAddressMode != ACC && opAddressMode != IMM && opAddressMode != REL && opAddressMode != IMP) {
        bool valid = true;
        bool pass = false;
        address = nesSystem->m_nesCPU.retrieve_cpu_address(opAddressMode, &pass, &valid, instruction2, instruction3, true);
    }


    if (ignoreNextBreaks) {
        ignoreNextBreaks = false;
    } else {
        for (unsigned int i = 0; i < breakpoints.size(); i++) {
            if (breakpoints[i] == PC) {
                ignoreNextBreaks = true;
                return true;
            }
        }

        for (unsigned int i = 0; i < watchpoints.size(); i++) {
            if (address == watchpoints[i]) {
                ignoreNextBreaks = true;
                return true;
            }
        }
    }

    return false;
}

u16 Debugger::print_next_instr(u16 PC, bool printAllInfo, bool * valid) {

    u8 opcode = nesSystem->m_nesCPU.get_cpu_byte(PC, true);
    u8 instruction2 = nesSystem->m_nesCPU.get_cpu_byte(PC + 1, true);
    u8 instruction3 = nesSystem->m_nesCPU.get_cpu_byte(PC + 2, true);
    enum AddressMode opAddressMode = addressModes[opcode];

    u16 address = 0;
    if (opAddressMode != ACC && opAddressMode != IMM && opAddressMode != REL && opAddressMode != IMP) {
        bool pass = false;
        address = nesSystem->m_nesCPU.retrieve_cpu_address(opAddressMode, &pass, valid, instruction2, instruction3, true);
        if (*valid == false) {
            return 0;
        }
    }

    u8 memByte;
    if (opAddressMode == IMM) {
        memByte = instruction2;
    } else if (opAddressMode == NONE || opAddressMode == ACC || opAddressMode == IMP || opAddressMode == REL) {
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
            memByte = nesSystem->m_nesCPU.get_cpu_byte(address, true);
            break;
        }
    }

    print_hex(PC, 4);
    std::cout << "  ";

    int whitespace = print_instruction(opcode, instruction2, instruction3, memByte, 
        nesSystem->m_nesCPU.m_X, nesSystem->m_nesCPU.m_Y, address, PC, opAddressMode, printAllInfo);

    if (printAllInfo) {

        for (int x = 0; x < whitespace; x++) std::cout << ' ';

        print_registers(nesSystem->m_nesCPU.m_A, nesSystem->m_nesCPU.m_X, nesSystem->m_nesCPU.m_Y, 
            get_psw_byte(nesSystem->m_nesCPU.m_PS), nesSystem->m_nesCPU.m_SP);

        std::cout << "CYC:";
        int count = nesSystem->m_nesCPU.m_nesPPU.m_ppuClock % 341;
        int scanLines = (nesSystem->m_nesCPU.m_nesPPU.m_ppuClock % (341 * 262)) / 341; 
        if (count < 10) {
            std::cout << "  ";
        } else if (count < 100) {
            std::cout << " ";
        }
        std::cout << std::dec << count;
        std::cout << " SL:" << std::dec << scanLines <<std::endl;
    } else {
        std::cout << std::endl;
    }
    
    if (opcode == 0xA2) {
        return PC + 2;
    } else {
        return PC + opcodeLens[opcode % 0x20];
    }
}

bool valid_digits(std::string string, bool hex, int start, int end) {

    if (hex) {
        for (int i = start; i < end; i++) {

            if (!(string[i] >= '0' && string[i] <= '9') && !((string[i] >= 'A' && string[i] <= 'F') || (string[i] >= 'a' && string[i] <= 'f'))) {
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

std::vector<std::string> split(std::string s) {
    std::stringstream ss(s);

    std::string temp;

    std::vector<std::string> tokens;

    while (ss >> temp) {
        tokens.push_back(temp);
    }

    return tokens;

}

void list_hex(std::vector<uint16_t> v) {

    if (v.size() != 0) {
        for (int i = 0; i < (int) v.size(); i++) {

            std::cout << "0x";
            print_hex(v[i], 4);

            if (i != (int) v.size() - 1) {
                std::cout << ", ";  
            }
        }
        std::cout << std::endl;
    } else {
        std::cout << "None" << std::endl;
    }

}

bool remove_value(std::vector<uint16_t> * nums, uint16_t toRemove) {

    for (unsigned int i = 0; i < (*nums).size(); i++) {
        if ((*nums)[i] == toRemove) {
            (*nums).erase((*nums).begin() + i);
            return true;
        }
    }

    return false;

}

bool add_value(std::vector<uint16_t> * nums, uint16_t toAdd) {

    for (unsigned int i = 0; i < (*nums).size(); i++) {
        if ((*nums)[i] == toAdd) {
            return false;
        }
    }

    (*nums).push_back(toAdd);
    return true;
}

bool Debugger::memDemp(uint16_t address, int rows) {

    int test = (address & ~0xF) + rows * 0x10;
    if (test > 0xFFFF) {
        return false;
    }

    address &= ~0xF;

    for (int x = 0; x < rows; x++) {

        print_hex(address + x * 0x10, 4);
        std::cout << "   ";

        for (int y = 0; y < 16; y++) {
            print_hex(nesSystem->m_nesCPU.get_cpu_byte(address + x * 0x10 + y, true), 2);
            std::cout << " ";
        }

        std::cout << std::endl;

    }

    return true;
}

bool Debugger::cmd(bool * quit, bool * draw, bool * focus) {

    /*
    add features:
    set controller input until 'x'

    view code

    change memory and registers

    waveform visualizer
    change speed
    breakpoints (mem reads, writes, until next frame, NMI, IRQ, etc)
    memory view
    disassembler
    nametables and pattern tables view

    stack related breakpoints?
    register related breakpoints?


    */

    std::cout << "XeNES (0x";
    print_hex(nesSystem->m_nesCPU.m_PC, 4);
    std::cout << ") > ";

    std::string input = "";
    getline(std::cin, input);

    std::vector<std::string> tokens = split(input);

    switch(tokens.size()) {

        case 0:
        break;

        case 1: {

            if (tokens[0].compare("run") == 0 || tokens[0].compare("r") == 0) {
                *focus = true;
                return true;
            } else if (tokens[0].compare("quit") == 0 || tokens[0].compare("q") == 0) {
                *quit = true;
                return true;
            } else if (tokens[0].compare("help") == 0 || tokens[0].compare("h") == 0) {
                std::cout << "print help prompt" << std::endl;
            } else if (tokens[0].compare("bv") == 0) {
                list_hex(breakpoints);
            } else if (tokens[0].compare("wv") == 0) {
                list_hex(watchpoints);
            } else if (tokens[0].compare("registers") == 0 || tokens[0].compare("reg") == 0) {
                print_registers(nesSystem->m_nesCPU.m_A, nesSystem->m_nesCPU.m_X, nesSystem->m_nesCPU.m_Y, 
                    get_psw_byte(nesSystem->m_nesCPU.m_PS), nesSystem->m_nesCPU.m_SP);
                std::cout << std::endl;
            } else {
                std::cout << "Command not recognized" << std::endl;
            }

            break;
        }

        case 2: {

            if (tokens[0].compare("log") == 0 || tokens[0].compare("l") == 0) {

                if (tokens[1].compare("true") == 0 || tokens[1].compare("t") == 0 || tokens[1].compare("1") == 0) {
                    log = true;
                } else if (tokens[1].compare("false") == 0 || tokens[1].compare("f") == 0 || tokens[1].compare("0") == 0) {
                    log = false;
                } else {
                    std::cout << "Command not recognized" << std::endl;
                }

            } else {

                if (!valid_digits(tokens[1], true, 0)) {
                    std::cout << "Invalid second operand (must be a number)" << std::endl;
                    break;
                }

                uint16_t num = std::stoi (tokens[1], nullptr, 16);

                if (tokens[0].compare("d") == 0 || tokens[0].compare("dis") == 0 || tokens[0].compare("disassemble") == 0) {
                    toDisassemble = std::stoi (tokens[1], nullptr, 16);
                    return true;
                } else if (tokens[0].compare("j") == 0 || tokens[0].compare("jump") == 0 || tokens[0].compare("jmp") == 0) {
                    nesSystem->m_nesCPU.m_PC = num;
                } else if (tokens[0].compare("ba") == 0) {

                    if (!add_value(&breakpoints, num)) {
                        std::cout << "Breakpoint already exists" << std::endl;
                    } else {
                        ignoreNextBreaks = true;
                    }

                } else if (tokens[0].compare("wa") == 0) {

                    if (!add_value(&breakpoints, num)) {
                        std::cout << "Watchpoint already exists" << std::endl;
                    } else {
                        ignoreNextBreaks = true;
                    }
                } else if (tokens[0].compare("br") == 0) {

                    if (!remove_value(&breakpoints, num)) {
                        std::cout << "Breakpoint not found" << std::endl;
                    }

                } else if (tokens[0].compare("wr") == 0) {

                    if (!remove_value(&breakpoints, num)) {
                        std::cout << "Watchpoint not found" << std::endl;
                    }

                } else {
                    std::cout << "Command not recognized" << std::endl;
                }
            }

            break;   
        }

        case 3: {

            if (!valid_digits(tokens[1], true, 0)) {
                std::cout << "Invalid second operand (must be a number)" << std::endl;
                break;
            }

            if (!valid_digits(tokens[2], true, 0)) {
                std::cout << "Invalid third operand (must be a number)" << std::endl;
                break;
            }

            uint16_t firstNumber = std::stoi (tokens[1], nullptr, 16);
            uint16_t secondNumber = std::stoi (tokens[2], nullptr, 16);

            if (tokens[0].compare("memory") == 0 || tokens[0].compare("mem") == 0 || tokens[0].compare("m") == 0) {

                if (!valid_digits(tokens[1], true, 0)) {
                    std::cout << "Invalid second operand (must be a number)" << std::endl;
                    break;
                }

                if (!valid_digits(tokens[2], true, 0)) {
                    std::cout << "Invalid third operand (must be a number)" << std::endl;
                    break;
                }

                if (!memDemp(firstNumber, ((secondNumber / 16) + 1))) {
                    std::cout << "Memory dump out of bounds" << std::endl;
                }

            } else if (tokens[0].compare("d") == 0 || tokens[0].compare("dis") == 0 || tokens[0].compare("disassemble") == 0) {
                toDisassemble = secondNumber;
                nesSystem->m_nesCPU.m_PC = firstNumber;
                return true;
            } else if (tokens[0].compare("peek") == 0 || tokens[0].compare("p") == 0) {

                u16 temp_PC = firstNumber;
                bool valid = true;

                for (int x = 0; x < secondNumber; x++) {

                    temp_PC = print_next_instr(temp_PC, false, &valid);
                    if (!valid) {
                        std::cout << "Interpreting these bytes as instructions leads to crash" << std::endl;
                        break;
                    }

                }

            } else {
                std::cout << "Command not recognized" << std::endl;
            }

            break;
        }

        default:
        std::cout << "Command not recognized" << std::endl;
        break;

    }

    return false;

}
