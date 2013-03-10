#ifndef __FCEU_SDL_H
#define __FCEU_SDL_H

#if _SDL2
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif
#include "main.h"
#include "dface.h"
#include "input.h"

static void DoFun(int frameskip);
static int isloaded = 0;
extern int noGui;

int LoadGame(const char *path);
int CloseGame(void);

#endif
