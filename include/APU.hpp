#pragma once
#include <cstdint>

class APU {

public:
    uint8_t registers[0x20];          //joystick and apu registers
    int16_t * audioBuffer;
    int audioBufferSize;
    uintmax_t sampleClock;

    int lengthCounterPulse1;
    int lengthCounterPulse2;
    int lengthCounterTriangle;
    int linearCounterTriangle;
    int lengthCounterNoise;

    bool linearReloading;

    APU();
    void fillBuffer(bool *);
};
