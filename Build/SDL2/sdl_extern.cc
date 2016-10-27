#include "sdl_extern.h"

extern "C" {

static T_buttonClick G_mouseButton = 0;
static T_word16 G_mouseX = 0;
static T_word16 G_mouseY = 0;
extern SDL_Window *window;

T_void
MouseSet(int newX, int newY)
{
    newX >>= 1;
    newY >>= 1; // scale for large screen

    if (newX > 319)
        newX = 319;
    if (newY > 199)
        newY = 199;
    G_mouseX = (T_word16) newX;
    G_mouseY = (T_word16) newY;
}

void
OutsideMouseDriverGet(T_word16 *xPos, T_word16 *yPos)
{
    *xPos = G_mouseX;
    *yPos = G_mouseY;
}

void
OutsideMouseDriverSet(T_word16 xPos, T_word16 yPos)
{
    SDL_WarpMouseInWindow(window, xPos * 2, yPos * 2);
    MouseSet(xPos * 2, yPos * 2);
}

T_void
MouseSetButton(T_buttonClick click)
{
    G_mouseButton = click;
}

T_buttonClick
MouseGetButton(T_void)
{
    return G_mouseButton;
}

}

