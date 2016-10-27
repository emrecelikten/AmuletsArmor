#include <iostream>
#include <SDL.h>
#include "sdl_engine.h"

int
main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
    {
        std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    SDL_ShowCursor(SDL_DISABLE);
    SDL_SetRelativeMouseMode(SDL_TRUE);

    InitSDLGraphics();

    SDL_Quit();
    return 0;
}
