#include <iostream>
#include <fstream>

#include "NES.hpp"
#include "gameLoop.hpp"

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

    loop(nesSystem);

    delete [] nesSystem.PRG_ROM;
    delete [] nesSystem.PRG_RAM;
    delete [] nesSystem.CHR_ROM;

    return 0;
}
