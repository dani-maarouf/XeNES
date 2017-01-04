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

static void print_hex(u16, int);
static int debug_print_val(enum AddressMode, int, int);
static int print_instruction(u8, u8, u8, u8, u8, u8, u16, u16, enum AddressMode, bool);
static void print_registers(u8 A, u8 X, u8 Y, u8 PS, u8 SP);
static bool valid_digits(std::string string, bool hex, int start, int end);
static bool valid_digits(std::string string, bool hex, int start);
static std::vector<std::string> split(std::string s);
static void list_hex(std::vector<uint16_t> v);
static bool remove_value(std::vector<uint16_t> * nums, uint16_t toRemove);
static bool add_value(std::vector<uint16_t> * nums, uint16_t toAdd);
static void print_debugger_help();

Debugger::Debugger(NES * nes) {
    log = false;
    instrsToLog = 0;
    ignoreNextBreaks = false;
    this->nesSystem = nes;
}

enum DebuggerEventStatus Debugger::perform_events() {

    bool temp = true;

    if (check_breaks()) {
        instrsToLog = 0;
        return BREAK_HIT;
    }

    if (instrsToLog > 0) {

        print_next_instr(nesSystem->m_nesCPU.m_PC, true, &temp);

        instrsToLog--;
        if (instrsToLog == 0) {
            return DONE_LOGGED_EXECUTION;
        }

    } else if (log) {
        print_next_instr(nesSystem->m_nesCPU.m_PC, true, &temp);
    }

    if (!temp) {
        return CRASH_IMMINENT;
    }

    return NO_EVENT;
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

bool Debugger::mem_dump(uint16_t address, int rows) {

    int test = (address & ~0xF) + rows * 0x10;
    if (test > 0x10000) {
        return false;
    }

    address &= ~0xF;

    for (int x = 0; x < rows; x++) {

        print_hex(address + x * 0x10, 4);
        std::cout << ":  ";

        for (int y = 0; y < 16; y++) {
            print_hex(nesSystem->m_nesCPU.get_cpu_byte(address + x * 0x10 + y, true), 2);
            std::cout << " ";
        }

        std::cout << std::endl;

    }

    return true;
}

bool Debugger::disassemble(u16 PC, int numIntrs) {

    bool valid = true;

    for (int x = 0; x < numIntrs; x++) {
        PC = print_next_instr(PC, false, &valid);
        if (!valid) {
            return false;
        }
    }

    return true;
}

enum DebuggerCommandStatus Debugger::cmd() {

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

    sieve


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
                return RUN_RETURN_FOCUS;
            } else if (tokens[0].compare("quit") == 0 || tokens[0].compare("q") == 0) {
                return QUIT;
            } else if (tokens[0].compare("help") == 0 || tokens[0].compare("h") == 0) {
                print_debugger_help();
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

            bool argIsNumber = valid_digits(tokens[1], true, 0);
            uint16_t num = (argIsNumber) ? std::stoi (tokens[1], nullptr, 16) : 0;

            if ((tokens[0].compare("log") == 0 || tokens[0].compare("l") == 0) && !argIsNumber) {
                if (tokens[1].compare("true") == 0 || tokens[1].compare("on") == 0) {
                    log = true;
                } else if (tokens[1].compare("false") == 0 || tokens[1].compare("off") == 0) {
                    log = false;
                } else {
                    std::cout << "Command not recognized" << std::endl;
                }

            } else if ((tokens[0].compare("log") == 0 || tokens[0].compare("l") == 0) && argIsNumber) {
                instrsToLog = std::stoi (tokens[1], nullptr, 16);
                return RUN_NO_FOCUS;
            } else if ((tokens[0].compare("j") == 0 || tokens[0].compare("jump") == 0 || tokens[0].compare("jmp") == 0) && argIsNumber) {
                nesSystem->m_nesCPU.m_PC = num;
            } else if ((tokens[0].compare("d") == 0 || tokens[0].compare("dis") == 0 || tokens[0].compare("disassemble") == 0) && argIsNumber) {

                if (!disassemble(nesSystem->m_nesCPU.m_PC, num)) {
                    std::cout << "Disassembling these bytes leads to crash" << std::endl;
                }

            } else if (tokens[0].compare("ba") == 0 && argIsNumber) {

                if (!add_value(&breakpoints, num)) {
                    std::cout << "Breakpoint already exists" << std::endl;
                } else {
                    ignoreNextBreaks = true;
                }

            } else if (tokens[0].compare("wa") == 0 && argIsNumber) {

                if (!add_value(&watchpoints, num)) {
                    std::cout << "Watchpoint already exists" << std::endl;
                } else {
                    ignoreNextBreaks = true;
                }
            } else if (tokens[0].compare("br") == 0 && argIsNumber) {

                if (!remove_value(&breakpoints, num)) {
                    std::cout << "Breakpoint not found" << std::endl;
                }

            } else if (tokens[0].compare("wr") == 0 && argIsNumber) {

                if (!remove_value(&watchpoints, num)) {
                    std::cout << "Watchpoint not found" << std::endl;
                }

            } else {
                std::cout << "Command not recognized" << std::endl;
            }

            break;   
        }

        case 3: {

            bool firstArgNumber = valid_digits(tokens[1], true, 0);
            bool secondArgNumber = valid_digits(tokens[2], true, 0);

            uint16_t firstNumber = (firstArgNumber) ? std::stoi(tokens[1], nullptr, 16) : 0;
            uint16_t secondNumber = (secondArgNumber) ? std::stoi(tokens[2], nullptr, 16) : 0;

            if ((tokens[0].compare("memory") == 0 || tokens[0].compare("mem") == 0 || tokens[0].compare("m") == 0) && firstArgNumber && secondArgNumber) {

                if (!mem_dump(firstNumber, (((secondNumber - 1) / 16) + 1))) {
                    std::cout << "Memory dump out of bounds" << std::endl;
                }

            } else if ((tokens[0].compare("d") == 0 || tokens[0].compare("dis") == 0 || tokens[0].compare("disassemble") == 0) && firstArgNumber && secondArgNumber) {

                if (!disassemble(firstNumber, secondNumber)) {
                    std::cout << "Disassembling these bytes leads to crash" << std::endl;
                }

            } else if ((tokens[0].compare("set") == 0 || tokens[0].compare("s") == 0) && !firstArgNumber && secondArgNumber)  {

                if (secondNumber > 0xFF) {
                    std::cout << "Second argument must fit in 8 bits" << std::endl;
                    break;
                }

                if (tokens[1].compare("X") == 0) {
                    nesSystem->m_nesCPU.m_X = secondNumber;
                } else if (tokens[1].compare("Y") == 0) {
                    nesSystem->m_nesCPU.m_Y = secondNumber;
                } else if (tokens[1].compare("ACCU") == 0) {
                    nesSystem->m_nesCPU.m_A = secondNumber;
                } else if (tokens[1].compare("P") == 0) {
                    get_psw_from_byte(nesSystem->m_nesCPU.m_PS, secondNumber);
                } else if (tokens[1].compare("SP") == 0) {
                    nesSystem->m_nesCPU.m_SP = secondNumber;
                } else {
                    std::cout << "Command not recognized" << std::endl;
                }

            } else if ((tokens[0].compare("set") == 0 || tokens[0].compare("s") == 0) && firstArgNumber && secondArgNumber) {

                if (secondNumber > 0xFF) {
                    std::cout << "Second argument must fit in 8 bits" << std::endl;
                    break;
                }

                nesSystem->m_nesCPU.set_cpu_byte(firstNumber, secondNumber);
                
            } else if ((tokens[0].compare("zero") == 0 || tokens[0].compare("z") == 0) && firstArgNumber && secondArgNumber) {

                for (int i = 0; i < secondNumber; i++) {
                    nesSystem->m_nesCPU.set_cpu_byte((firstNumber + i) & 0xFFFF, 0);
                }
            }

            else {
                std::cout << "Command not recognized" << std::endl;
            }

            break;
        }

        default:
        std::cout << "Command not recognized" << std::endl;
        break;

    }

    return CONTINUE_DEBUG;

}

static void print_hex(u16 num, int width) {
    std::cout << std::hex << std::uppercase << std::setfill('0') << std::setw(width) << (int) num;
    return;
}

static int debug_print_val(enum AddressMode mode, int firstByte, int secondByte) {

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

static int print_instruction(u8 opcode, u8 instruction2, u8 instruction3, u8 memByte, u8 X, 
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

static void print_registers(u8 A, u8 X, u8 Y, u8 PS, u8 SP) {
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

static bool valid_digits(std::string string, bool hex, int start, int end) {

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

static bool valid_digits(std::string string, bool hex, int start) {
    return valid_digits(string, hex, start, string.length());
}

static std::vector<std::string> split(std::string s) {
    std::stringstream ss(s);

    std::string temp;

    std::vector<std::string> tokens;

    while (ss >> temp) {
        tokens.push_back(temp);
    }

    return tokens;

}

static void list_hex(std::vector<uint16_t> v) {

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

static bool remove_value(std::vector<uint16_t> * nums, uint16_t toRemove) {

    for (unsigned int i = 0; i < (*nums).size(); i++) {
        if ((*nums)[i] == toRemove) {
            (*nums).erase((*nums).begin() + i);
            return true;
        }
    }

    return false;

}

static bool add_value(std::vector<uint16_t> * nums, uint16_t toAdd) {

    for (unsigned int i = 0; i < (*nums).size(); i++) {
        if ((*nums)[i] == toAdd) {
            return false;
        }
    }

    (*nums).push_back(toAdd);
    return true;
}

static void print_debugger_help() {

    std::cout << "~~[Debugger commands]~~" << std::endl << std::endl;

    std::cout << "Press esc while playing to enter debugger console" << std::endl;

    std::cout << "Command     : 'run', 'r'" << std::endl;
    std::cout << "Arguments   : 0" << std::endl;
    std::cout << "Description : Leave debugger console, continue executing and return focus to emulator window" << std::endl << std::endl;

    std::cout << "Command     : 'quit', 'q'" << std::endl;
    std::cout << "Arguments   : 0" << std::endl;
    std::cout << "Description : Terminate program" << std::endl << std::endl;

    std::cout << "Command     : 'help', 'h'" << std::endl;
    std::cout << "Arguments   : 0" << std::endl;
    std::cout << "Description : Print information about debugger commands" << std::endl << std::endl;

    std::cout << "Command     : 'reg', 'registers'" << std::endl;
    std::cout << "Arguments   : 0" << std::endl;
    std::cout << "Description : Print the state of all registers" << std::endl << std::endl;

    std::cout << "Command     : 'log', 'l'" << std::endl;
    std::cout << "Arguments   : 1" << std::endl;
    std::cout << "'on','true'   - enable logging" << std::endl;
    std::cout << "'off','false' - disable logging" << std::endl;
    std::cout << "{number}      - log next {number} instructions and return to debugger console" << std::endl;
    std::cout << "Description : Detailed instruction information to stdout" << std::endl << std::endl;

    std::cout << "Command     : 'jump', 'jmp', 'j'" << std::endl;
    std::cout << "Arguments   : 1:" << std::endl;
    std::cout << "{number} - change program counter to {number}" << std::endl;
    std::cout << "Description : Change execution location" << std::endl << std::endl;

    std::cout << "Command     : 'disassemble', 'dis', 'd'" << std::endl;
    std::cout << "Arguments   : 1 or 2:" << std::endl;
    std::cout << "If one argument:" << std::endl;
    std::cout << "{number} - disassemble {number} instructions starting from current PC" << std::endl;
    std::cout << "If two arguments:" << std::endl;
    std::cout << "{number 1} and {number 2} - disassemble {number 2} instructions starting from {number 1}" << std::endl;
    std::cout << "Description : Interpret bytes as instructions (view only, not execute)" << std::endl << std::endl;

    std::cout << "Command     : 'memory','mem', 'm'" << std::endl;
    std::cout << "Arguments   : 2:" << std::endl;
    std::cout << "{number 1} and {number 2} - dump {number 2} bytes starting at {number 1}" << std::endl;
    std::cout << "Description : Dump memory to stdout" << std::endl << std::endl;

    std::cout << "Command     : 'ba'" << std::endl;
    std::cout << "Arguments   : 1:" << std::endl;
    std::cout << "{number} - set breakpoint at {number}" << std::endl;
    std::cout << "Description : Stop execution before PC reaches these memory locations" << std::endl << std::endl;

    std::cout << "Command     : 'br'" << std::endl;
    std::cout << "Arguments   : 1:" << std::endl;
    std::cout << "{number} - remove breakpoint at {number}" << std::endl;
    std::cout << "Description : Stop execution before PC reaches these memory locations" << std::endl << std::endl;

    std::cout << "Command     : 'bv'" << std::endl;
    std::cout << "Arguments   : 0" << std::endl;
    std::cout << "Description : Print all active breakpoints" << std::endl << std::endl;

    std::cout << "Command     : 'wa'" << std::endl;
    std::cout << "Arguments   : 1:" << std::endl;
    std::cout << "{number} - set watchpoint at {number}" << std::endl;
    std::cout << "Description : Stop execution before these memory locations are accessed" << std::endl << std::endl;

    std::cout << "Command     : 'wr'" << std::endl;
    std::cout << "Arguments   : 1:" << std::endl;
    std::cout << "{number} - remove watchpoint at {number}" << std::endl;
    std::cout << "Description : Stop execution before these memory locations are accessed" << std::endl << std::endl;

    std::cout << "Command     : 'wv'" << std::endl;
    std::cout << "Arguments   : 0" << std::endl;
    std::cout << "Description : Print all active watchpoints" << std::endl << std::endl;

    std::cout << "Command     : 'set', 's'" << std::endl;

}
