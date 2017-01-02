#include <iostream>
#include <chrono>
#include <thread>
#include <SDL2/SDL.h>

#include "NES.hpp"
#include "debugger.hpp"

//obtained from blargg's Full Palette demo
const uint32_t paletteTable [] = {
    ( 84<<16)|( 84<<8)|( 84),(  0<<16)|( 30<<8)|(116),(  8<<16)|( 16<<8)|(144),( 48<<16)|(  0<<8)|(136),
    ( 68<<16)|(  0<<8)|(100),( 92<<16)|(  0<<8)|( 48),( 84<<16)|(  4<<8)|(  0),( 60<<16)|( 24<<8)|(  0),
    ( 32<<16)|( 42<<8)|(  0),(  8<<16)|( 58<<8)|(  0),(  0<<16)|( 64<<8)|(  0),(  0<<16)|( 60<<8)|(  0),
    (  0<<16)|( 50<<8)|( 60),(  0<<16)|(  0<<8)|(  0),(  0<<16)|(  0<<8)|(  0),(  0<<16)|(  0<<8)|(  0),
    (152<<16)|(150<<8)|(152),(  8<<16)|( 76<<8)|(196),( 48<<16)|( 50<<8)|(236),( 92<<16)|( 30<<8)|(228),
    (136<<16)|( 20<<8)|(176),(160<<16)|( 20<<8)|(100),(152<<16)|( 34<<8)|( 32),(120<<16)|( 60<<8)|(  0),
    ( 84<<16)|( 90<<8)|(  0),( 40<<16)|(114<<8)|(  0),(  8<<16)|(124<<8)|(  0),(  0<<16)|(118<<8)|( 40), 
    (  0<<16)|(102<<8)|(120),(  0<<16)|(  0<<8)|(  0),(  0<<16)|(  0<<8)|(  0),(  0<<16)|(  0<<8)|(  0),
    (236<<16)|(238<<8)|(236),( 76<<16)|(154<<8)|(236),(120<<16)|(124<<8)|(236),(176<<16)|( 98<<8)|(236), 
    (228<<16)|( 84<<8)|(236),(236<<16)|( 88<<8)|(180),(236<<16)|(106<<8)|(100),(212<<16)|(136<<8)|( 32),
    (160<<16)|(170<<8)|(  0),(116<<16)|(196<<8)|(  0),( 76<<16)|(208<<8)|( 32),( 56<<16)|(204<<8)|(108),
    ( 56<<16)|(180<<8)|(204),( 60<<16)|( 60<<8)|( 60),(  0<<16)|(  0<<8)|(  0),(  0<<16)|(  0<<8)|(  0),
    (236<<16)|(238<<8)|(236),(168<<16)|(204<<8)|(236),(188<<16)|(188<<8)|(236),(212<<16)|(178<<8)|(236),
    (236<<16)|(174<<8)|(236),(236<<16)|(174<<8)|(212),(236<<16)|(180<<8)|(176),(228<<16)|(196<<8)|(144),
    (204<<16)|(210<<8)|(120),(180<<16)|(222<<8)|(120),(168<<16)|(226<<8)|(144),(152<<16)|(226<<8)|(180),
    (160<<16)|(214<<8)|(228),(160<<16)|(162<<8)|(160),(  0<<16)|(  0<<8)|(  0),(  0<<16)|(  0<<8)|(  0),
};

/* SDL video */
SDL_Window * window = NULL;     //SDL window
SDL_Renderer * renderer = NULL; //SDL renderer
SDL_Texture * texture = NULL;   //SDL texture
uint32_t localPixels[256 * 240];

/* consts */
const double MILLISECONDS_PER_FRAME = 1000.0/60.0; //60FPS
const int SCALE_FACTOR = 3;                  //size of each NES display pixel in real pixels
const bool REMOVE_OVERSCAN = true;

/* SDL audio */
const int samplingFrequency = 48000;
const int sampleBytes = sizeof(int16_t) * 2;
const int channels = 2;
const SDL_AudioFormat format = AUDIO_S16SYS;
SDL_AudioDeviceID sdlAudioDevice = 0;

/* local function prototypes */
static int get_refresh_rate(SDL_Window * win);
static bool init_sdl(const char *);
static void close_sdl();
static void draw(uint8_t *);
static bool process_events(SDL_Event *, uint8_t *, bool *);

void loop(NES nesSystem, const char * fileLoc) {

    if (!init_sdl(fileLoc)) {
        std::cerr << "SDL did not initialize, quitting" << std::endl;
        return;
    }

    Debugger debugger(&nesSystem);

    //game loop variables
    double frequency = SDL_GetPerformanceFrequency();
    uintmax_t startTime = SDL_GetPerformanceCounter();
    bool paused = false;
    SDL_Event event;

    for (int x = 0; x < 256 * 240; x++) localPixels[x] = 0;
    draw(nesSystem.m_nesCPU.m_nesPPU.m_pixels);       //draw screen black
    SDL_PauseAudioDevice(sdlAudioDevice, 0);    //unpause audio

    for (;;) {

        //1 process events
        if (!process_events(&event, &nesSystem.m_nesCPU.m_controllerByte, &paused)) {
            break;
        }

        if (debugger.toDisassemble != 0) {
            paused = false;
        }

        //2 logic
        if (!paused) {
            do {

                bool breakPointHit = false;

                if (debugger.perform_events(&breakPointHit)) {
                    paused = true;
                }

                if (breakPointHit) {
                    paused = true;
                    break;
                }

                //execute one cpu opcode
                nesSystem.m_nesCPU.execute_next_opcode();
                //ppu catches up
                nesSystem.m_nesCPU.m_nesPPU.tick(&nesSystem.m_nesCPU.m_NMI, &nesSystem.m_nesCPU.m_cpuClock);
            } while (!nesSystem.m_nesCPU.m_nesPPU.m_draw && !paused);

            
            //3.1 audio
            nesSystem.m_nesCPU.m_nesAPU.fill_buffer(&nesSystem, &nesSystem.m_nesCPU.m_IRQ);
            if (SDL_GetQueuedAudioSize(sdlAudioDevice) > (unsigned int) nesSystem.m_nesCPU.m_nesAPU.m_audioBufferSize * 10) {
                //prevents audio from becoming too out of sync
                SDL_ClearQueuedAudio(sdlAudioDevice);
            }
            SDL_QueueAudio(sdlAudioDevice, (void *) nesSystem.m_nesCPU.m_nesAPU.m_audioBuffer, nesSystem.m_nesCPU.m_nesAPU.m_audioBufferSize);
            
        } else {

            bool quit = false;
            bool doDraw = false;
            bool bringFocus = false;

            while (!debugger.shell(&quit, &doDraw, &bringFocus)) {
                if (doDraw) {
                    draw(nesSystem.m_nesCPU.m_nesPPU.m_pixels);
                }
            }

            if (quit) {
                break;
            }

            paused = false;

            if (bringFocus) {
                SDL_RaiseWindow(window);
            
            }
            

        }

        //3.2 video
        draw(nesSystem.m_nesCPU.m_nesPPU.m_pixels);
        
        //4 sync framerate
        double delay = MILLISECONDS_PER_FRAME - (((SDL_GetPerformanceCounter() - startTime) / frequency) * 1000) - 0.5;
        if (delay > 0) {
            std::this_thread::sleep_for(std::chrono::microseconds((int) (delay * 1000)));
        }
        while ((((SDL_GetPerformanceCounter() - startTime) / frequency) * 1000)  < MILLISECONDS_PER_FRAME) {

        }
        startTime = SDL_GetPerformanceCounter();
    }

    SDL_PauseAudioDevice(sdlAudioDevice, 1);    //pause
    SDL_ClearQueuedAudio(sdlAudioDevice);       //clear audio queue
    close_sdl();

    return;
}

static int get_refresh_rate(SDL_Window * win) {

	SDL_DisplayMode mode;
	int displayIndex = SDL_GetWindowDisplayIndex(win);

	if (SDL_GetDesktopDisplayMode(displayIndex, &mode) != 0 ) {
        return 0;
    } else {
    	return mode.refresh_rate;
    }
}

static bool init_sdl(const char * fileLoc) {

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "Could not initialize SDL : " << SDL_GetError() << std::endl;
        return false;
    }

    /*  START VIDEO */
    //set window title
    char windowTitle[100];
    strcpy(windowTitle, "XeNES: ");
    strcat(windowTitle, fileLoc);

    int outputHeight;

    if (REMOVE_OVERSCAN) {
        outputHeight = (NES_SCREEN_HEIGHT - 16);
    } else {
        outputHeight = NES_SCREEN_HEIGHT;
    }

    window = SDL_CreateWindow(windowTitle, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        NES_SCREEN_WIDTH * SCALE_FACTOR, outputHeight * SCALE_FACTOR, SDL_WINDOW_RESIZABLE);

    if (window == NULL) {
        std::cerr << "Could not create SDL window : " << SDL_GetError() << std::endl;
        SDL_Quit();
        return false;
    }

    //SDL_RENDERER_PRESENTVSYNC
    if (get_refresh_rate(window) == 60) {
    	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
    } else {	
    	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0");
    }
    

    if (renderer == NULL) {
        std::cerr << "Could not create SDL renderer : " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, NES_SCREEN_WIDTH, outputHeight);

    if (texture == NULL) {
        std::cerr << "Could not create SDL texture : " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, 0); 
    /* END VIDEO */

    /* START AUDIO */
    SDL_AudioSpec want;
    SDL_memset(&want, 0, sizeof(want));

    SDL_AudioSpec have;
    want.freq = samplingFrequency;
    want.format = format;        //32-bit floating point samples in little-endian byte order
    want.channels = channels;
    want.samples = samplingFrequency * sampleBytes / 60;

    sdlAudioDevice = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);

    if (sdlAudioDevice == 0) {
        std::cerr << "Failed to open audio : " << SDL_GetError() << std::endl;
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    /* END AUDIO */
    
    return true;
}

static void close_sdl() {
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_CloseAudioDevice(sdlAudioDevice);
    SDL_Quit();
    return;
}

static void draw(uint8_t * pixels) {

    for (int x = 0; x < 256 * 240; x++) {
        localPixels[x] = paletteTable[pixels[x]];
    }

    uint32_t * sdlPixels;

    if (REMOVE_OVERSCAN) {
        sdlPixels = localPixels + NES_SCREEN_WIDTH * 8;
    } else {
        sdlPixels = localPixels;
    }

    SDL_UpdateTexture(texture, NULL, sdlPixels, NES_SCREEN_WIDTH * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    return;
}

static bool process_events(SDL_Event * event, uint8_t * controller, bool * paused) {

    while (SDL_PollEvent(event)) {
        switch(event->type) {

            case SDL_QUIT:
            return false;

            
            case SDL_WINDOWEVENT: {
                
                switch(event->window.event) {

                    case SDL_WINDOWEVENT_FOCUS_LOST:
                    *paused = true;
                    break;

                    case SDL_WINDOWEVENT_FOCUS_GAINED:
                    *paused = false;
                    break;

                    default:
                    break;

                }

                break;
            }
            

            case SDL_KEYDOWN: {
                switch(event->key.keysym.sym) {

                    case SDLK_SPACE:    //A
                    (*controller) |= 0x1;
                    break;

                    case SDLK_LCTRL:    //B
                    (*controller) |= 0x2;
                    break;

                    case SDLK_c:    //select
                    (*controller) |= 0x4;
                    break;

                    case SDLK_x:    //start
                    (*controller) |= 0x8;
                    break;

                    case SDLK_w:    //up
                    (*controller) |= 0x10;
                    break;

                    case SDLK_s:    //down
                    (*controller) |= 0x20;
                    break;

                    case SDLK_a:    //left
                    (*controller) |= 0x40;
                    break;

                    case SDLK_d:    //right
                    (*controller) |= 0x80;
                    break;

                    case SDLK_ESCAPE:
                    *paused = true;
                    break;

                    default:
                    break;

                }
                break;
            }

            case SDL_KEYUP: {
                switch(event->key.keysym.sym) {

                    case SDLK_SPACE:    //A
                    (*controller) &= ~0x1;
                    break;

                    case SDLK_LCTRL:    //B
                    (*controller) &= ~0x2;
                    break;

                    case SDLK_c:    //select
                    (*controller) &= ~0x4;
                    break;

                    case SDLK_x:    //start
                    (*controller) &= ~0x8;
                    break;

                    case SDLK_w:    //up
                    (*controller) &= ~0x10;
                    break;

                    case SDLK_s:    //down
                    (*controller) &= ~0x20;
                    break;

                    case SDLK_a:    //left
                    (*controller) &= ~0x40;
                    break;

                    case SDLK_d:    //right
                    (*controller) &= ~0x80;
                    break;

                    default:
                    break;

                }
                break;
            }

            default:
            break;
        }
    }
    return true;
}
