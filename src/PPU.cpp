#include "PPU.hpp"

PPU::PPU() {

    for (int x = 0; x < 0x4000; x++) {
        memory[x] = 0x0;
    }

    return;
}
