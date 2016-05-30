#include <iostream>
#include <fstream>

#include "CPU.hpp"

bool openROM(const char * fileLoc) {

	if (fileLoc == NULL) {
		return false;
	}

	std::ifstream romFile;
	romFile.open(fileLoc, std::ios::binary);

	if (!romFile.is_open()) {
		return false;
	}



	while (romFile) {
		char c;
		romFile.get(c);
		unsigned char u = (unsigned char) c;
		int binaryValue = (int) u;

		std::cout << std::hex << binaryValue << ' ';
	}
	std::cout << std::endl;

	romFile.close();
	return true;
}

int main(int argc, char ** argv) {

	if (argc != 2) {
		std::cerr << "Invalid number of arguments. Expected one. Received " << argc - 1 << std::endl;
		std::cout << "Please call program with format: '" << argv[0] << " ROM.nes'" << std::endl;
		return 1;
	}

	if (!openROM(argv[1])) {
		std::cerr << "Could not read file" << std::endl;
		return 1;
	}

	return 0;
}
