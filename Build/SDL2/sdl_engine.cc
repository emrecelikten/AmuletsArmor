#include <iostream>
#include <SDL.h>
#include "sdl_engine.h"
#include "sdl_extern.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 400

static SDL_Renderer *renderer;
static unsigned char *pixels;
static unsigned char *large_pixels;
static SDL_Texture *texture;

void
UpdateMouse(void)
{
    int flags = 0;
    int x, y;
    Uint8 state;

    state = SDL_GetMouseState(&x, &y);
    MouseSet(x, y);
    if (state & SDL_BUTTON_LMASK)
        flags |= MOUSE_BUTTON_LEFT;
    if (state & SDL_BUTTON_RMASK)
        flags |= MOUSE_BUTTON_RIGHT;
    if (state & SDL_BUTTON_MMASK)
        flags |= MOUSE_BUTTON_MIDDLE;
    MouseSetButton(flags);
}

void
HandleSDLEvents()
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_QUIT:break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                }
                else if ((event.key.keysym.sym == SDLK_RETURN) && (event.key.keysym.mod == KMOD_ALT))
                {
                    // ALT-Enter toggles full screen
//                    flags = renderer->flags; /* Save the current flags in case toggling fails */
//                    screen = SDL_SetVideoMode(0, 0, 0, screen->flags ^ SDL_FULLSCREEN); /*Toggles FullScreen Mode */
//                    if (screen == NULL)
//                        screen = SDL_SetVideoMode(0, 0, 0, flags); /* If toggle FullScreen failed, then switch back */
//                    if (screen == NULL)
//                        exit(1); /* If you can't switch back for some reason, then epic fail */
                }
                break;
        }
    }
}

void
SDLEngineUpdate(char *p_screen, unsigned char *palette)
{
    SDL_Color colors[256];
    int i;
    unsigned char *src = pixels;
    unsigned char *dst = large_pixels;
    unsigned char *line;
    static int lastFPS = 0;
    static int fps = 0;
    int x, y;
    T_word32 tick = clock();
    static T_word32 lastTick = 0xFFFFEEEE;
    static double movingAverage = 0;
    T_word32 v;
    T_word32 frac;

#if CAP_SPEED_TO_FPS
    if ((tick-lastTick)<(1000/CAP_SPEED_TO_FPS))
    {
        Sleep((1000/CAP_SPEED_TO_FPS) - (tick-lastTick));
        // 10 ms between frames (top out at 100 ms)
    }
    else
#endif
    {
        lastTick = tick;

        // Setup the color palette for this update
        for (i = 0; i < 256; i++)
        {
            colors[i].r = ((((unsigned int) *(palette++)) & 0x3F) << 2);
            colors[i].g = ((((unsigned int) *(palette++)) & 0x3F) << 2);
            colors[i].b = ((((unsigned int) *(palette++)) & 0x3F) << 2);
        }
        //SDL_SetColors(surface, colors, 0, 256);
        //SDL_SetColors(largesurface, colors, 0, 256);

        // Blit the current surface from 320x200 to 640x480
        line = src;
        for (y = 0, frac = 0; y < 200; y++, line += 320)
        {
#pragma unroll
            for (x = 0; x < 320; x++)
            {
                *(dst++) = *src;
                *(dst++) = *(src++);
            }
        }

        if (SDL_UpdateTexture(texture, NULL, pixels, 320 * sizeof(char))) {
            std::cout << "Failed update: " << SDL_GetError() << std::endl;
        }

        SDL_RenderClear(renderer);
        if (SDL_RenderCopy(renderer, texture, NULL, NULL) < 0)
        {
            std::cout << "Failed blit: " << SDL_GetError() << std::endl;
        }
        SDL_RenderPresent(renderer);

        fps++;

        if ((tick - lastFPS) >= 1000)
        {
            if (movingAverage < 1.0)
                movingAverage = fps;
            movingAverage = ((double) fps) * 0.05 + movingAverage * 0.95;
            lastFPS += 1000;
            //printf("%02d:%02d:%02d.%03d FPS: %d, %f\n", tick/3600000, (tick/60000) % 60, (tick/1000) % 60, tick%1000, fps, movingAverage);
            fps = 0;
        }

        // Handle IO updates
        HandleSDLEvents();
        UpdateMouse();
        KeyboardUpdate(TRUE);
#if CAP_SPEED_TO_100_FPS
        Sleep(1);
#endif
    }
}

int
InitSDLGraphics()
{

//    SDL_Window *win = SDL_CreateWindow("Amulets & Armor",
//                                       SDL_WINDOWPOS_UNDEFINED,
//                                       SDL_WINDOWPOS_UNDEFINED,
//                                       SCREEN_WIDTH,
//                                       SCREEN_HEIGHT, SDL_WINDOW_FULLSCREEN);
    SDL_Window *win = SDL_CreateWindow("Amulets & Armor",
                                       SDL_WINDOWPOS_UNDEFINED,
                                       SDL_WINDOWPOS_UNDEFINED,
                                       SCREEN_WIDTH,
                                       SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (win == nullptr)
    {
        std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr)
    {
        SDL_DestroyWindow(win);
        std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    texture = SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                640, 480);

    if (texture == nullptr)
    {
        std::cout << "SDL_CreateTexture Error for texture: " << SDL_GetError() << std::endl;
        return 1;
    }

    pixels = (unsigned char *) malloc(320 * 240 * sizeof(char));
    large_pixels = (unsigned char *) malloc(640 * 480 * sizeof(char));
    GRAPHICS_ACTUAL_SCREEN = (T_screen) pixels;

    game_main(NULL, NULL);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
}