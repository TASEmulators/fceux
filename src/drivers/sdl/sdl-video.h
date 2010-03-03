#ifndef __FCEU_SDL_VIDEO_H
#define __FCEU_SDL_VIDEO_H
uint32 PtoV(uint16 x, uint16 y);
bool FCEUD_ShouldDrawInputAids();
bool FCEUI_AviDisableMovieMessages();
static SDL_Surface *s_screen;

#endif
