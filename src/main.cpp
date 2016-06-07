#include <iostream>
#include <bitset>

#include "NES.hpp"
#include "gameLoop.hpp"

int main(int argc, char ** argv) {

    if (argc != 2) {
        std::cerr << "Invalid number of arguments. Expected one. Received " << argc - 1 << std::endl;
        std::cout << "Please call program with format: '" << argv[0] << " ROM.nes'" << std::endl;
        return 1;
    }

    NES nesSystem;

    if (!nesSystem.openCartridge(argv[1])) {
        std::cerr << "Could not read file" << std::endl;
        return 1;
    }

    //loop(nesSystem);

    //nesSystem.ppuTick();

    for (int i = 0x0; i < 0x2000; i += 0x10) {

        for (int x = i; x < 0x8 + i; x++) {
            std::cout << (std::bitset<8>) (nesSystem.getPpuByte(x) | nesSystem.getPpuByte(x + 8)) << std::endl;
        }

        std::cout << std::endl << std::endl;
    }



    nesSystem.freeCartridgePointers();
    
    return 0;
}
