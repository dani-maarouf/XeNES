#ifndef __NES_CPU__
#define __NES_CPU__

#include <cstdint>

enum ProcessorStatusIndex {
	N,		//is number negative
	V,		//overflow
	B,		//set for BRK instruction
	D,		//BCD mode
	I,		//disable interrupts
	Z,		//zero flag
	C 		//carry flag
};

class CPU {

public:
	uint16_t PC;		//program counter
	uint8_t SP;			//stack pointer

	uint8_t A;			//accumulator
	uint8_t X;			//register X
	uint8_t Y;			//register Y

	bool PS[8];			//processor status word

	uint8_t memory[0x10000];

};

#endif
/* DEFINED __NES_CPU__ */
