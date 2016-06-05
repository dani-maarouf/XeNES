#include "APU.hpp"

APU::APU(uint8_t * apuRegs) {

	registers = apuRegs;

	return;
}

APU::~APU() {

	delete [] registers;

	return;
}
