#include <cstdint>

uint8_t get_cpu_mapper_0(uint16_t memAddress, int romBanks, uint8_t * ROM) {


    if (memAddress < 0xC000) {
        return ROM[memAddress - 0x8000];
    } else {
        /* memAddress >= 0xC000 && < 0x10000 */
        if (romBanks == 1) {
            return ROM[ (memAddress - 0x8000) % 0x4000];
        } else {
            /* 2 rom banks */
            return ROM[memAddress - 0x8000];
        }
    }

}

uint8_t get_ppu_mapper_0(uint16_t memAddress, uint8_t * VROM) {
    return VROM[memAddress];
}
