#include <iostream>
#include <cstring>
#include <SDL2/SDL.h>

#include "NES.hpp"

const int FPS = 60;
const int TICKS_PER_FRAME = 1000/FPS;

SDL_Window * window = NULL;
SDL_Renderer * renderer = NULL;
SDL_Texture * texture = NULL;

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

    memset(nesSystem.pixels, 0, screenWidth * screenHeight * sizeof(uint32_t));

    nesSystem.drawSprites();

    SDL_UpdateTexture(texture, NULL, nesSystem.pixels, screenWidth * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

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
        executeResult = nesSystem.executeNextOpcode(false);

        if (executeResult == 0) {
            std::cerr << "Error executing opcode" << std::endl;
            break;
        } else {
            nesSystem.setCpuCycle((nesSystem.getCpuCycle() + 3 * executeResult) % 341);

            int currentCpuCycle;
            currentCpuCycle = nesSystem.getCpuCycle();

            while (nesSystem.getPpuCycle() != currentCpuCycle) {

                nesSystem.ppuTick();

                if (nesSystem.draw) {

                    SDL_Delay(50);

                    /*
                    nesSystem.drawSprites();

                    SDL_UpdateTexture(texture, NULL, nesSystem.pixels, screenWidth * sizeof(uint32_t));
                    SDL_RenderClear(renderer);
                    SDL_RenderCopy(renderer, texture, NULL, NULL);
                    SDL_RenderPresent(renderer);

                    
                    SDL_Delay(1000);
                    */

                }
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
        screenWidth, screenHeight, 0);

    if (window == NULL) {
        std::cerr << "Could not create SDL window : " << SDL_GetError() << std::endl;
        SDL_Quit();
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, 0);

    if (renderer == NULL) {
        std::cerr << "Could not create SDL renderer : " << SDL_GetError() << std::endl;
        SDL_Quit();
        return false;
    }

    texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, screenWidth, screenHeight);

    if (texture == NULL) {
        std::cerr << "Could not create SDL texture : " << SDL_GetError() << std::endl;
        SDL_Quit();
        return false;
    }

    return true;

}

void closeSDL() {
    if (window != NULL) {
        SDL_DestroyWindow(window);
        window = NULL;
    }
    SDL_Quit();
}
