#ifndef __NES_APU__
#define __NES_APU__

#include <cstdint>

class APU {

public:
    uint8_t registers[0x20];          //joystick and apu registers

    APU();
};

#endif
/* DEFINED __NES_APU__ */
