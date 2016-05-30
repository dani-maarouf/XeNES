#include "CPU.hpp"

void CPU::init() {

	for (int x = 0; x < 0x10000; x++) {
		memory[x] = 0x0;
	}
	for (int x = 0; x < 8; x++) {
		PS[x] = false;
	}

	PC = 0x0;
	SP = 0x0;
	A  = 0x0;
	X  = 0x0;
	Y  = 0x0;

	nesAPU.init();

	return;
}
