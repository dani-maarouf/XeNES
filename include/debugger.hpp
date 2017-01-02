#pragma once

#include <cstdint>
#include "NES.hpp"


class Debugger {

private:

    void print_byte(uint8_t);
    int debug_print_val(enum AddressMode, int, int);
    

public:

    void print_debug_line(NES *);
    void shell();

};

