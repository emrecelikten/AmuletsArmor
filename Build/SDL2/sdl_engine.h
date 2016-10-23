#ifndef _SDL_ENGINE_H
#define _SDL_ENGINE_H

extern "C" {
#include "GENERAL.H"

T_void
game_main(T_word16 argc, char *argv[]);

T_void
KeyboardUpdate(E_Boolean updateBuffers);
};

int
InitSDLGraphics();

#endif //_SDL_ENGINE_H
