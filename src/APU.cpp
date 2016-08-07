#include <iostream>     //temp
#include <cstdlib>

#include "APU.hpp"

const int lengthTable[] = {
    10,254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30,
};

APU::APU() {

    for (int x = 0; x < 0x20; x++) {
        registers[x] = 0;
    }

    audioBuffer = NULL;
    audioBufferSize = 0;
    sampleClock = 0;

    lengthCounterPulse1 = lengthCounterPulse2 = lengthCounterTriangle = lengthCounterNoise = 0;

    return;
}


void APU::fillBuffer() {

    int volume = 800;
    

    for (int x = 0; x < 800 * 2; x++) {
        audioBuffer[x] = 0;
    }

    
    
    if (registers[0x15] & 0x1) {

        if (lengthCounterPulse1 > 0) {

            int t = ((registers[3] & 0x7) << 8) | (registers[2]);

            int period = 48000 / (1789773 / (16 * (t + 1)));
            period *= 2;


            if (period != 0) {
                for (int i = 0; i < 800; i++) {

                    int16_t sampleVal;

                    switch((registers[0] & 0xC0) >> 6) {

                        case 1:
                        sampleVal = (((sampleClock + i * 4) / (period)) % 8) ? -volume : volume;
                        break;

                        case 2:
                        sampleVal = (((sampleClock + i * 4) / (period)) % 4) ? -volume : volume;
                        break;

                        case 3:
                        sampleVal = (((sampleClock + i * 4) / (period)) % 2) ? volume : -volume;
                        break;

                        case 4:
                        sampleVal = (((sampleClock + i * 4) / (period)) % 4) ? volume : -volume;
                        break;

                        default:
                        sampleVal = 0;
                        break;

                    }

                    audioBuffer[i * 2] += sampleVal;
                    audioBuffer[i * 2 + 1] += sampleVal;
                }
            }

            if (!(registers[0] & 0x20)) {
                lengthCounterPulse1 -= 4;
            }

        }
    }
    
    if (registers[0x15] & 0x2) {

        if (lengthCounterPulse2 > 0) {

            int t = ((registers[7] & 0x7) << 8) | (registers[6]);

            int period = 48000 / (1789773 / (16 * (t + 1)));
            period *= 2;



            if (period != 0) {
                for (int i = 0; i < 800; i++) {

                    int16_t sampleVal;



                    switch((registers[4] & 0xC0) >> 6) {

                        case 1:
                        sampleVal = (((sampleClock + i * 4) / (period)) % 8) ? -volume : volume;
                        break;

                        case 2:
                        sampleVal = (((sampleClock + i * 4) / (period)) % 4) ? -volume : volume;
                        break;

                        case 3:
                        sampleVal = (((sampleClock + i * 4) / (period)) % 2) ? volume : -volume;
                        break;

                        case 4:
                        sampleVal = (((sampleClock + i * 4) / (period)) % 4) ? volume : -volume;
                        break;

                        default:
                        sampleVal = 0;
                        break;

                    }



                    audioBuffer[i * 2] += sampleVal;
                    audioBuffer[i * 2 + 1] += sampleVal;

                }
            }

            if (!(registers[4] & 0x20)) {
                lengthCounterPulse2 -= 4;
            }


        }


    }


    
    if (registers[0x15] & 0x4) {

        if (lengthCounterTriangle > 0) {

            int t = ((registers[0xB] & 0x7) << 8) | (registers[0xA]);
            int period = 48000 / (1789773 / (16 * (t + 1)));
            period *= 2;

            if (period != 0) {

                for (int i = 0; i < 800; i++) {

                    int16_t sampleVal;
                    if (((sampleClock + i * 4) / period) % 2) {
                        sampleVal = (((sampleClock + i * 4) % (period)) - (period / 2)) * (volume * 6 / period);
                    } else {
                        sampleVal = ( (period / 2) -((sampleClock + i * 4) % (period))) * (volume * 6 / period);
                    }

                    audioBuffer[i * 2] += sampleVal;
                    audioBuffer[i * 2 + 1] += sampleVal;


                }

            }

            if (!(registers[0x8] & 0x80)) {
                lengthCounterTriangle -= 4;
            }

        }

    }

    
    if (registers[0x15] & 0x8) {

        if (lengthCounterNoise > 0) {


            for (int i = 0; i < 800; i++) {

                int16_t val = rand() % volume;

                audioBuffer[i * 2] += val;
                audioBuffer[i * 2 + 1] += val;

            }

            if (!(registers[0xC] & 0x20)) {
                lengthCounterNoise -= 4;
            }

        }

        

    }
    
    
    

    


    
    
    
    sampleClock += 3200;

    return;
}