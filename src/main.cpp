#include <iostream>
#include <fstream>
#include <bitset>

#include "NES.hpp"

#include <unistd.h>


int main(int argc, char ** argv) {

	if (argc != 2) {
		std::cerr << "Invalid number of arguments. Expected one. Received " << argc - 1 << std::endl;
		std::cout << "Please call program with format: '" << argv[0] << " ROM.nes'" << std::endl;
		return 1;
	}

	NES nesSystem;

	if (!nesSystem.openROM(argv[1])) {
		std::cerr << "Could not read file" << std::endl;
		return 1;
	}

	nesSystem.nesCPU.PC = 0xC000;

	for (int x = 0; x < 8991; x++) {
		if (!nesSystem.nesCPU.executeNextOpcode(true, false)) {
			std::cerr << "Error executing opcode" << std::endl;
			break;
		}
	}

	return 0;
}
