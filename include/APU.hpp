#pragma once

#include <cstdint>

class NES;

class APU {

public:

    int m_sampleFrequency;
    uint8_t m_dmcOut;
    uint16_t m_sampleAddress;    

    uint16_t m_sampleByteLength;
    int m_currentSampleByte;
    uint8_t m_sampleBuffer;
    int m_bufferIndex;

    uint8_t m_registers[0x20];          //joystick and apu registers
    int16_t * m_audioBuffer;
    int m_audioBufferSize;
    uintmax_t m_sampleClock;

    int m_lengthCounterPulse1;
    int m_lengthCounterPulse2;
    int m_lengthCounterTriangle;
    int m_linearCounterTriangle;
    int m_lengthCounterNoise;

    bool m_linearReloading;

    APU();
    void fill_buffer(NES *, bool *);
};
