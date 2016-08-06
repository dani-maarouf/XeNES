#include <iostream>

#include "NES.hpp"
#include "gameLoop.hpp"

int main(int argc, char ** argv) {

    if (argc != 2) {
        std::cerr << "Invalid number of arguments. Expected one. Received " << argc - 1 << std::endl;
        std::cout << "\nPlease call program with format: '" << argv[0] << " ROM.nes'\n" << std::endl;

        std::cout << "Flags:\n-t\t\t\tAutomated testing" << std::endl;
        return EXIT_FAILURE;
    }

    /*
    todo:

    fix sprite rendering, output pixel alignment, sprite 0 hits and ppu even and odd frame timing
    
    */

    NES nesSystem;

    if (!nesSystem.openCartridge(argv[1])) {
        std::cerr << "Could not load ROM : " << argv[1] << std::endl;
        nesSystem.closeCartridge();
        return EXIT_FAILURE;
    }

    loop(nesSystem, argv[1]);

    nesSystem.closeCartridge();
    return EXIT_SUCCESS;
}
