#ifndef _SDL_EXTERN_H_
#define _SDL_EXTERN_H_

extern "C" {
#include "MOUSEMOD.H"
#include "GENERAL.H"

void SDLEngineUpdate(char *p_screen, unsigned char * palette) ;
void OutsideMouseDriverGet(T_word16 *xPos, T_word16 *yPos);
T_void MouseSet(T_word16 newX, T_word16 newY) ;
T_buttonClick MouseGetButton(T_void) ;
T_void MouseSetButton(T_buttonClick click) ;
T_void MouseClearButton(T_buttonClick click) ;

}

#endif // _SDL_EXTERN_H_
