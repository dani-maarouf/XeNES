#include <iostream>
#include <fstream>
#include <bitset>

#include "NES.hpp"


int main(int argc, char ** argv) {

	if (argc != 2) {
		std::cerr << "Invalid number of arguments. Expected one. Received " << argc - 1 << std::endl;
		std::cout << "Please call program with format: '" << argv[0] << " ROM.nes'" << std::endl;
		return 1;
	}

	NES nesSystem;
	nesSystem.init();

	if (!nesSystem.openROM(argv[1])) {
		std::cerr << "Could not read file" << std::endl;
		return 1;
	}

	nesSystem.nesCPU.PC = 0x8000;

	for (int x = 0; x < 100; x++) {
		nesSystem.nesCPU.executeNextOpcode();
	}

	


	return 0;
}
