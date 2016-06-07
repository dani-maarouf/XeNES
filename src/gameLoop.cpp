#include <iostream>
#include <cstring>
#include <SDL2/SDL.h>

#include "NES.hpp"

const int FPS = 60;
const int screenWidth = 256 * 2;
const int screenHeight = 240 * 2;

//SDL window
SDL_Window * window = NULL;

//Surface contained by window
SDL_Surface * windowSurface = NULL;

static bool initSDL(const char *);
static void closeSDL();

void loop(NES nesSystem, const char * fileLoc) {

	if (!initSDL(fileLoc)) {
		std::cerr << "SDL did not initialize, quitting" << std::endl;
		return;
	}

	bool running;
	SDL_Event event;
	running = true;

	while (running) {

		SDL_PollEvent(&event);

		switch(event.type) {
			case SDL_QUIT:
				running = false;
				break;

			default:
			break;
		}

		int executeResult;
		executeResult = nesSystem.executeNextOpcode(true);

        if (executeResult == 0) {
            std::cerr << "Error executing opcode" << std::endl;
            break;
        } else {
        	nesSystem.setCpuCycle((nesSystem.getCpuCycle() + 3 * executeResult) % 341);

        	int currentCpuCycle;
        	currentCpuCycle = nesSystem.getCpuCycle();

        	while (nesSystem.getPpuCycle() != currentCpuCycle) {
        		nesSystem.ppuTick();
        	}
        }
	}

    closeSDL();

	return;
}

bool initSDL(const char * fileLoc) {

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cerr << "Could not initialize SDL : " << SDL_GetError() << std::endl;
		return false;
	}

	char windowTitle[100];
	strcpy(windowTitle, "NES: ");
	strcat(windowTitle, fileLoc);

	window = SDL_CreateWindow(windowTitle, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
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
