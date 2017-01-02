#include <cstdlib>

#include "APU.hpp"
#include "NES.hpp"

#define get_bit(num, bit)    (bit == 0) ? (num & 0x1) : (bit == 1) ? (num & 0x2) : \
    (bit == 2) ? (num & 0x4) : (bit == 3) ? (num & 0x8) :   \
    (bit == 4) ? (num & 0x10) : (bit == 5) ? (num & 0x20) : \
    (bit == 6) ? (num & 0x40) : (num & 0x80)

//arbitrary number which results in reasonable volume
const int SDL_VOLUME = 1200;

APU::APU() {

    for (int x = 0; x < 0x20; x++) {
        m_registers[x] = 0;
    }

    m_audioBuffer = NULL;
    m_audioBufferSize = 0;
    m_sampleClock = 0;

    m_lengthCounterPulse1 = m_lengthCounterPulse2 = m_lengthCounterTriangle = m_linearCounterTriangle = m_lengthCounterNoise = 0;
    m_registers[0x15] = 0x1F; 

    m_linearReloading = false;

    m_sampleFrequency = 0;
    m_dmcOut = 0;
    m_sampleByteLength = 0;
    m_sampleAddress = 0;
    m_sampleBuffer = 0;
    m_bufferIndex = 0;
    m_currentSampleByte = 0;

    return;
}

void APU::fill_buffer(NES * nesSystem, bool * IRQ) {

    int lastValue = m_audioBuffer[1599];

    for (int x = 0; x < 800 * 2; x++) {
        m_audioBuffer[x] = 0;
    }

    if (m_registers[0x15] & 0x1) {

        if (m_lengthCounterPulse1 > 0) {

            int rawPeriod = ((m_registers[3] & 0x7) << 8) | (m_registers[2]);

            //period in sdl samples
            int period = ((rawPeriod + 1) / 111860.8) * 48000.0;

            int volume = (m_registers[0] & 0xF) * (SDL_VOLUME / 16);


            if (period != 0) {
                for (int i = 0; i < 800; i++) {

                    int16_t sampleVal;

                    switch((m_registers[0] & 0xC0) >> 6) {

                        case 1:
                        sampleVal = (((m_sampleClock + i * 4) / (period)) % 8) ? -volume : volume;
                        break;

                        case 2:
                        sampleVal = (((m_sampleClock + i * 4) / (period)) % 4) ? -volume : volume;
                        break;

                        case 3:
                        sampleVal = (((m_sampleClock + i * 4) / (period)) % 2) ? volume : -volume;
                        break;

                        case 4:
                        sampleVal = (((m_sampleClock + i * 4) / (period)) % 4) ? volume : -volume;
                        break;

                        default:
                        sampleVal = 0;
                        break;

                    }

                    m_audioBuffer[i * 2] += sampleVal;
                    m_audioBuffer[i * 2 + 1] += sampleVal;
                }
            }

            if (!(m_registers[0] & 0x20)) {
                m_lengthCounterPulse1 -= 2;
            }

        } else {
            //m_registers[0x15] &= ~0x1;
        }


    }
    
    
    if (m_registers[0x15] & 0x2) {

        if (m_lengthCounterPulse2 > 0) {

            int rawPeriod = ((m_registers[7] & 0x7) << 8) | (m_registers[6]);
            int period = ((rawPeriod + 1) / 111860.8) * 48000.0;

            int volume = (m_registers[4] & 0xF) * (SDL_VOLUME / 16);


            if (period != 0) {
                for (int i = 0; i < 800; i++) {

                    int16_t sampleVal;



                    switch((m_registers[4] & 0xC0) >> 6) {

                        case 1:
                        sampleVal = (((m_sampleClock + i * 4) / (period)) % 8) ? -volume : volume;
                        break;

                        case 2:
                        sampleVal = (((m_sampleClock + i * 4) / (period)) % 4) ? -volume : volume;
                        break;

                        case 3:
                        sampleVal = (((m_sampleClock + i * 4) / (period)) % 2) ? volume : -volume;
                        break;

                        case 4:
                        sampleVal = (((m_sampleClock + i * 4) / (period)) % 4) ? volume : -volume;
                        break;

                        default:
                        sampleVal = 0;
                        break;

                    }



                    m_audioBuffer[i * 2] += sampleVal;
                    m_audioBuffer[i * 2 + 1] += sampleVal;

                }
            }


            if (!(m_registers[3] & 0x20)) {
                m_lengthCounterPulse2 -= 2;
            }

        } else {
            //m_registers[0x15] &= ~0x2;
        }
    }

    
    if (m_registers[0x15] & 0x4) {


        if (m_lengthCounterTriangle > 0 && m_linearCounterTriangle > 0) {

            int rawPeriod = ((m_registers[0xB] & 0x7) << 8) | (m_registers[0xA]);
            int period = ((rawPeriod + 1) / 55930.4) * 48000.0;

            period *= 2;

            int volume = SDL_VOLUME;

            if (period != 0) {

                for (int i = 0; i < 800; i++) {

                    int16_t sampleVal;
                    if (((m_sampleClock + i * 4) / period) % 2) {
                        sampleVal = ((m_sampleClock + i * 4) % (period)) * (volume * 4 / period);
                    } else {
                        sampleVal = (period - ((m_sampleClock + i * 4) % (period))) * (volume * 4 / period);
                    }

                    m_audioBuffer[i * 2] += sampleVal;
                    m_audioBuffer[i * 2 + 1] += sampleVal;
                }
            }
        }

        if (m_linearReloading) {
            m_linearCounterTriangle = m_registers[0x8] & 0x7F;
        } else {
            m_linearCounterTriangle -= 2;
        }

        if (!(m_registers[0x8] & 0x80)) {
            m_lengthCounterTriangle -= 2;
            m_linearReloading = false;
        }


    }

    
    if (m_registers[0x15] & 0x8) {

        if (m_lengthCounterNoise > 0) {

            int volume = (m_registers[0xC] & 0xF) * (SDL_VOLUME / 8);

            if (volume != 0) {
                for (int i = 0; i < 800; i++) {

                    int16_t val = rand() % volume;

                    m_audioBuffer[i * 2] += val;
                    m_audioBuffer[i * 2 + 1] += val;

                }

            }


            if (!(m_registers[0xC] & 0x20)) {
                m_lengthCounterNoise -= 2;
            }
        

        } else {
            //m_registers[0x15] &= ~0x8;
        }
    }


    if (m_registers[0x15] & 0x10) {

        m_sampleBuffer = nesSystem->m_nesCPU.get_cpu_byte(m_sampleAddress +  m_currentSampleByte, true);

        int period = 48000.0 / (double) m_sampleFrequency;
        if (period == 0) {
            period = 1;
        }


        for (int i = 0; i < 800; i++) {

            if ((m_sampleClock + i * 4) % period == 0) {

                int sampleBit = get_bit(m_sampleBuffer, m_bufferIndex);

                if (sampleBit) {
                    if (m_dmcOut < 0x7) {
                        m_dmcOut++;
                    }
                } else {
                    if (m_dmcOut > 0) {
                        m_dmcOut--;
                    }
                }

                if (m_bufferIndex == 7) {


                    if (m_currentSampleByte == m_sampleByteLength - 1) {

                        if (m_registers[0x10] & 0x40) {
                            m_currentSampleByte = 0;
                        }

                        if (m_registers[0x10] & 0x80) {
                            *IRQ = true;
                        }


                    } else {
                        m_currentSampleByte++;
                        m_sampleBuffer = nesSystem->m_nesCPU.get_cpu_byte(m_sampleAddress +  m_currentSampleByte, true);


                    }


                }

                m_bufferIndex = (m_bufferIndex + 1) % 8;


            }

            m_audioBuffer[i * 2] += (64 - m_dmcOut) * 200;
            m_audioBuffer[i * 2 + 1] += (64 - m_dmcOut) * 200;

        } 


    } else {
        //PCM always outputs
        for (int i = 0; i < 800; i++) {

            m_audioBuffer[i * 2] += (64 - m_dmcOut) * 200;
            m_audioBuffer[i * 2 + 1] += (64 - m_dmcOut) * 200;

        } 
    }


    
    /*
    if ((m_registers[0x17] & 0xC0) == 0) {
        *IRQ = true;
    }
    */


    //anti clicking, smoother waveform
    int smoothLength = 25;
    for (int x = 0; x < smoothLength; x++) {
        m_audioBuffer[x] = (lastValue * (smoothLength - 1 - x) + m_audioBuffer[x] * x) / (smoothLength - 1);
    }
    
    m_sampleClock += 3200;

    return;
}
