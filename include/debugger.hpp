#pragma once

#include <cstdint>
#include <cstdbool>
#include "NES.hpp"
#include <vector>


class Debugger {

private:
    
    std::vector<uint16_t> breakpoints;
    std::vector<uint16_t> watchpoints;

    bool ignoreNextBreaks;
    NES * nesSystem;

    int debug_print_val(enum AddressMode, int, int);
    bool disassemble(u16 *);

public:

    bool log;

    int toDisassemble;

    Debugger(NES *);
    void peek_next_instruction(bool, bool *);
    bool shell(bool *, bool *, bool *);
    bool perform_events(bool *);

};
