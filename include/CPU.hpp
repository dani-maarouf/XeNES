#ifndef __NES_CPU__
#define __NES_CPU__

#include <cstdint>

#include "APU.hpp"

enum ProcessorStatusIndex {
	N,		//is number negative
	V,		//overflow
	D,		//BCD mode
	I,		//disable interrupts
	Z,		//zero flag
	C 		//carry flag
};

enum AddressMode {
	ABSOLUTE,
	ABSOLUTE_X,
	ABSOLUTE_Y,
	ACCUMULATOR,
	IMMEDIATE,
	IMPLIED,
	INDEXED_INDIRECT,
	INDIRECT,
	INDIRECT_INDEXED,
	RELATIVE,
	ZERO_PAGE,
	ZERO_PAGE_X,
	ZERO_PAGE_Y,
	NONE
};

class CPU {

public:
	uint16_t PC;		//program counter
	uint8_t SP;			//stack pointer

	uint8_t A;			//accumulator
	uint8_t X;			//register X
	uint8_t Y;			//register Y

	bool PS[8];			//processor status word

	//uint8_t stack[0x100];
	uint8_t cpuMem[0x10000];

	APU nesAPU;

	uint16_t retrieveAddress(enum AddressMode);
	uint8_t getByte(uint16_t);
	bool setByte(uint16_t, uint8_t);

	void init();
	bool executeNextOpcode(bool, bool);

};

#endif
/* DEFINED __NES_CPU__ */
