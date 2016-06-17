#include <iostream>
#include <cstring>
#include <SDL2/SDL.h>

#include "NES.hpp"

const int FPS = 60;
const int TICKS_PER_FRAME = 1000/FPS;
const int scaleFactor = 2;      //size of each NES display pixel in real pixels

SDL_Window * window = NULL;     //SDL window
SDL_Renderer * renderer = NULL; //SDL renderer
SDL_Texture * texture = NULL;   //SDL texture

static bool initSDL(const char *);
static void closeSDL();
static void draw(uint32_t *);

void loop(NES nesSystem, const char * fileLoc) {

    if (!initSDL(fileLoc)) {
        std::cerr << "SDL did not initialize, quitting" << std::endl;
        return;
    }

    bool running;
    int frameStartTime;
    SDL_Event event;

    running = true;
    frameStartTime = SDL_GetTicks();

    draw(nesSystem.getDisplayPixels());

    while (running) {

        //1. process events
        SDL_PollEvent(&event);
        switch(event.type) {
            case SDL_QUIT:
            running = false;
            break;

            default:
            break;
        }

        //2. logic
        int executeResult;
        executeResult = nesSystem.executeOpcode(false);

        if (executeResult == 0) {
            std::cerr << "Error executing opcode" << std::endl;
            break;
        } else {

            /* let PPU ticks catch up to CPU ticks */
            for (int x = 0; x < 3 * executeResult; x++) {

                //3. logic
                nesSystem.tickPPU();

                if (nesSystem.drawFlag()) {
                    //4. draw
                    draw(nesSystem.getDisplayPixels());

                    SDL_Delay(500);     //for testing
                    
                    /* sync framerate */
                    int frameEndTime;
                    frameEndTime = SDL_GetTicks();
                    if (frameEndTime - frameStartTime < TICKS_PER_FRAME) {
                        SDL_Delay(frameEndTime - frameStartTime);
                    }
                    frameStartTime = SDL_GetTicks();
                }
            }
        }
    }

    closeSDL();

    return;
}

static bool initSDL(const char * fileLoc) {

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Could not initialize SDL : " << SDL_GetError() << std::endl;
        return false;
    }

    //set window title
    char windowTitle[100];
    strcpy(windowTitle, "NES: ");
    strcat(windowTitle, fileLoc);

    window = SDL_CreateWindow(windowTitle, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        NES_SCREEN_WIDTH * scaleFactor, NES_SCREEN_HEIGHT * scaleFactor, 0);

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
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, NES_SCREEN_WIDTH, NES_SCREEN_HEIGHT);

    if (texture == NULL) {
        std::cerr << "Could not create SDL texture : " << SDL_GetError() << std::endl;
        SDL_Quit();
        return false;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, 0); 

    return true;

}

static void closeSDL() {
    if (texture != NULL) {
        SDL_DestroyTexture(texture);
        texture = NULL;
    }
    if (renderer != NULL) {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }
    if (window != NULL) {
        SDL_DestroyWindow(window);
        window = NULL;
    }

    SDL_Quit();
}

static void draw(uint32_t * pixels) {

    SDL_UpdateTexture(texture, NULL, pixels, NES_SCREEN_WIDTH * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

}
