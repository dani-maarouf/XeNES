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

        int rawPeriod = ((registers[3] & 0x7) << 8) | (registers[2]);
        int period = ((rawPeriod + 1) / 111860.8) * 48000.0;

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



    }
    
    
    if (registers[0x15] & 0x2) {


        int rawPeriod = ((registers[7] & 0x7) << 8) | (registers[6]);
        int period = ((rawPeriod + 1) / 111860.8) * 48000.0;

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



    }


    
    if (registers[0x15] & 0x4) {




        int rawPeriod = ((registers[0xB] & 0x7) << 8) | (registers[0xA]);
        int period = ((rawPeriod + 1) / 55930.4) * 48000.0;

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


    }

    
    /*
    if (registers[0x15] & 0x8) {



        for (int i = 0; i < 800; i++) {

            int16_t val = rand() % volume;

            audioBuffer[i * 2] += val;
            audioBuffer[i * 2 + 1] += val;

        }

    

    }
    */
    
    sampleClock += 3200;
    

    return;
}