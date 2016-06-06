#include <iostream>
#include <SDL2/SDL.h>

#include "NES.hpp"

const int screenWidth = 256 * 2;
const int screenHeight = 240 * 2;

//SDL window
SDL_Window * window = NULL;

//Surface contained by window
SDL_Surface * windowSurface = NULL;

static bool initSDL();
static void closeSDL();

void loop(NES nesSystem) {

	/*
	if (!initSDL()) {
		std::cerr << "SDL did not initialize, quitting" << std::endl;
		return;
	}
	*/

	for (int x = 0; x < 8991; x++) {

		int executeResult;
		executeResult = nesSystem.executeNextOpcode(true, false);

        if (executeResult == 0) {
            std::cerr << "Error executing opcode" << std::endl;
            break;
        } else {
        	nesSystem.count = (nesSystem.count + 3 * executeResult) % 341;
        }
    }

    //closeSDL();

	return;
}

bool initSDL() {

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cerr << "Could not initialize SDL : " << SDL_GetError() << std::endl;
		return false;
	}

	window = SDL_CreateWindow("NES", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		screenWidth, screenHeight, SDL_WINDOW_SHOWN);

	if (window == NULL) {
		std::cerr << "Could not create SDL window : " << SDL_GetError() << std::endl;
		SDL_Quit();
		return false;
	}

	windowSurface = SDL_GetWindowSurface(window);

	return true;

}

void closeSDL() {
	if (window != NULL) {
		SDL_DestroyWindow(window);
		window = NULL;
	}
	SDL_Quit();
}
