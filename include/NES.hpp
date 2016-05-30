#ifndef __NES_NES__
#define __NES_NES__

#include "CPU.hpp"
#include "PPU.hpp"

class NES {

public:

	CPU nesCPU;
	PPU nesPPU;

	bool openROM(const char *);

};

#endif
/* DEFINED __NES_NES__ */