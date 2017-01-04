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

    bool crashImminent;

    bool disassemble(u16, int);
    bool memDemp(u16, int);

public:

    bool log;

    int instrsToLog;

    Debugger(NES *);
    bool check_breaks();
    u16 print_next_instr(u16, bool, bool *);
    int cmd();
    int perform_events();

};

