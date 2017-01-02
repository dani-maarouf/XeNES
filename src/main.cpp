#include <iostream>
#include <ctime>

#include "NES.hpp"
#include "gameLoop.hpp"

int main(int argc, char ** argv) {


    if (argc != 2) {
        if (argc != 1) {
            std::cerr << "Invalid number of arguments. Expected one. Received " << argc - 1 << std::endl;
        }
        
        std::cout << "\nPlease call program with format: '" << argv[0] << " ROM.nes'\n" << std::endl;
        std::cout << "Flags:\n-t\t\t\tAutomated testing (not yet implemented)" << std::endl;
        return EXIT_FAILURE;
    }

    /*
    todo:

    state snapshot

    figure out better way to handle pausing and activating debugger

    add debugger features
    automated testing: hash pixel buffer and compare to known good result
    fix sprite rendering, output pixel alignment, sprite 0 hits and ppu even and odd frame timing
    */

    srand (time(NULL));     //for audio noise channel

    NES nesSystem;

    if (!nesSystem.open_cartridge(argv[1])) {
        std::cerr << "Could not load ROM : " << argv[1] << std::endl;
        nesSystem.close_cartridge();
        return EXIT_FAILURE;
    }

    loop(nesSystem, argv[1]);

    nesSystem.close_cartridge();
    return EXIT_SUCCESS;
}
