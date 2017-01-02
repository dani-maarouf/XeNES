#pragma once

#include <cstdint>
#include <cstdbool>
#include "NES.hpp"


class Debugger {

private:

    NES * nesSystem;

    bool log;

    void print_byte(uint8_t);
    int debug_print_val(enum AddressMode, int, int);
    

public:

    Debugger(NES *);
    void print_debug_line();
    bool shell(bool *, bool *);
    void perform_events();

};

