#ifndef __NES_APU__
#define __NES_APU__

#include <cstdint>

class APU {

public:
    uint8_t registers[0x20];          //joystick and apu registers
    int16_t * audioBuffer;
    int audioBufferSize;
    uint64_t sampleClock;

    int lengthCounterPulse1;
    int lengthCounterPulse2;
    int lengthCounterTriangle;
    int lengthCounterNoise;

    APU();
    void fillBuffer();
};

#endif
/* DEFINED __NES_APU__ */
