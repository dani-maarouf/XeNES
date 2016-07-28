#ifndef __NES_MAPPER__
#define __NES_MAPPER__

#include <cstdint>

uint8_t getCpuMapper0(uint16_t, int, uint8_t *);
uint8_t getPpuMapper0(uint16_t, uint8_t *);

#endif
/* DEFINED __NES_MAPPER__ */