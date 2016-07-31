#include <iostream>

#include "NES.hpp"
#include "gameLoop.hpp"

int main(int argc, char ** argv) {

    if (argc != 2) {
        std::cerr << "Invalid number of arguments. Expected one. Received " << argc - 1 << std::endl;
        std::cout << "\nPlease call program with format: '" << argv[0] << " ROM.nes'\n" << std::endl;

        std::cout << "Flags:\n-t\t\t\tAutomated testing" << std::endl;
        return 1;
    }

    /*
    todo:

    automated testing with pixel buffer hashing
    8x16 sprites
    pass NEStress ppu
    nes_instr_test 2 and 6

    */

    NES nesSystem;

    if (!nesSystem.openCartridge(argv[1])) {
        std::cerr << "Could not load ROM : " << argv[1] << std::endl;
        nesSystem.closeCartridge();
        return 1;
    }

    loop(nesSystem, argv[1]);

    nesSystem.closeCartridge();
    return 0;
}
