/////////////////////////////////////////////////////////////////
// getSDLKey.cpp
// Lukas Sabota - October, 2011
//
// Prints the corresponding SDL keysym for a key pressed
//  into a SDL window.  Useful for remapping hotkeys in they
//  ~/.fceux/fceux.cfg config file, but may be useful for other
//  applications as well.
//
/////////////////////////////////////////////////////////////////

#include<SDL/SDL.h>

void DisplayState(SDL_KeyboardEvent *key)
{
    if (key->type == SDL_KEYUP)
        printf("RELEASED: ");
    else
        printf("PRESSED: ");
 
}

void DisplayModifiers(SDL_KeyboardEvent *key)
{
    SDLMod modifier = key->keysym.mod;
    if( modifier & KMOD_NUM ) printf( "NUMLOCK " );
    if( modifier & KMOD_CAPS ) printf( "CAPSLOCK " );
    if( modifier & KMOD_MODE ) printf( "MODE " );
    if( modifier & KMOD_LCTRL ) printf( "LCTRL " );
    if( modifier & KMOD_RCTRL ) printf( "RCTRL " );
    if( modifier & KMOD_LSHIFT ) printf( "LSHIFT " );
    if( modifier & KMOD_RSHIFT ) printf( "RSHIFT " );
    if( modifier & KMOD_LALT ) printf( "LALT " );
    if( modifier & KMOD_RALT ) printf( "RALT " );
    if( modifier & KMOD_LMETA ) printf( "LMETA " );
    if( modifier & KMOD_RMETA ) printf( "RMETA " );
}

void DisplayKey(SDL_KeyboardEvent *key)
{
    printf( "%s\n", SDL_GetKeyName(key->keysym.sym));
}

void DisplayKeysym(SDL_KeyboardEvent *key)
{
    printf ("%d\n", key->keysym.sym);
}


int main(int argc, char** argv)
{
    SDL_Surface *screen;
 
    if (SDL_Init(SDL_INIT_VIDEO) != 0) 
    {
        printf("Unable to initialize SDL: %s\n", SDL_GetError());
        return 1;
    }
 
    atexit(SDL_Quit);
 
    screen = SDL_SetVideoMode(320, 240, 0, SDL_ANYFORMAT);
    SDL_WM_SetCaption("Get SDL Keysyms! Now 50\% off!", NULL);
    if (screen == NULL)
    {
        printf("Unable to set video mode: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Event event;
    int running = 1;
 
    while(running) {
        while(SDL_PollEvent(&event)) {
            switch(event.type){
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    DisplayState(&event.key);
                    DisplayModifiers(&event.key);
                    DisplayKey(&event.key);
                    DisplayKeysym(&event.key);
                    break;
                case SDL_QUIT:
                    running = 0;
                    break;
            }
        }
    }
    
    return 0;
}
