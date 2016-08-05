#include "APU.hpp"

APU::APU() {


    for (int x = 0; x < 0x20; x++) {
        registers[x] = 0;
    }

    return;
}