#include <iostream>

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
        std::cerr << "Could not load ROM" << argv[1] << std::endl;
        nesSystem.closeCartridge();
        return 1;
    }

    loop(nesSystem, argv[1]);

    nesSystem.closeCartridge();
    return 0;
}
