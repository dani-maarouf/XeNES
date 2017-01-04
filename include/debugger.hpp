#pragma once

#include <cstdint>
#include <cstdbool>
#include "NES.hpp"
#include <vector>

enum DebuggerCommandStatus {
    RUN_RETURN_FOCUS,
    RUN_NO_FOCUS,
    QUIT,
    CONTINUE_DEBUG
};

enum DebuggerEventStatus {
    BREAK_HIT,
    DONE_LOGGED_EXECUTION,
    CRASH_IMMINENT,
    NO_EVENT
};

class Debugger {

private:
    
    std::vector<uint16_t> breakpoints;
    std::vector<uint16_t> watchpoints;

    bool ignoreNextBreaks;
    NES * nesSystem;
    bool log;
    int instrsToLog;

    bool disassemble(u16, int);
    bool mem_dump(u16, int);
    u16 print_next_instr(u16, bool, bool *);
    bool check_breaks();

public:

    Debugger(NES *);
    enum DebuggerCommandStatus cmd();
    enum DebuggerEventStatus perform_events();

};

