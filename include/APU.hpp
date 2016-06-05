#ifndef __NES_APU__
#define __NES_APU__

#include <cstdint>

class APU {

public:

    uint8_t * registers;        //owned by APU

    APU(uint8_t *);
    ~APU();
};

#endif
/* DEFINED __NES_APU__ */
