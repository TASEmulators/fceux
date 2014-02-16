/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
 *  Copyright (C) 2002 Paul Kuliniewicz
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/// \file
/// \brief Handles GameController input using the SDL.  This is an SDL2-only feature

#include "sdl.h"

/* This entire file is SDL2 only. GameController API is new as of SDL2 */
#if SDL_VERSION_ATLEAST(2, 0, 0)

#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>

#define MAX_GAMECONTROLLERS   32
static SDL_GameController *s_GameCtrls[MAX_GAMECONTROLLERS] = {NULL};

static int s_gcinited = 0;


/**
 * Tests if the given button is active on the joystick.
 */
int
DTestButtonGC(ButtConfig *bc)
{
	int x;
	//printf("DTestButtonGC\n");
	SDL_GameController *gc;

	for(x = 0; x < bc->NumC; x++)
	{
		gc = s_GameCtrls[bc->DeviceNum[x]];
		if( SDL_GameControllerGetButton(gc, (SDL_GameControllerButton) bc->ButtonNum[x]) )
		{
			//printf("GC %d btn '%s'=down\n", bc->DeviceNum[x],
			//       SDL_GameControllerGetStringForButton( (SDL_GameControllerButton) bc->ButtonNum[x]));
			return 1;
		}
	}
	return 0;
}

/**
 * Shutdown the SDL joystick subsystem.
 */
int
KillGameControllers()
{
	int n;  /* joystick index */

	if(!s_gcinited) {
		return -1;
	}

	for(n = 0; n < MAX_GAMECONTROLLERS; n++) {
		if (s_GameCtrls[n]) {
			SDL_GameControllerClose(s_GameCtrls[n]);
		}
		s_GameCtrls[n]=0;
	}
	SDL_QuitSubSystem(MAX_GAMECONTROLLERS);
	return 0;
}

/**
 * Initialize the SDL Game-Controller subsystem.
 */
int
InitGameControllers()
{
	int n; /* joystick index */
	int total;


	SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);

	// Game Controller API wraps joystick API, so the number of joysticks is also
	// the number of game controllers.  For some reason SDL2 decided to leave
	// this as one API
	total = SDL_NumJoysticks();
	if(total>MAX_GAMECONTROLLERS) {
		total = MAX_GAMECONTROLLERS;
	}

	for(n = 0; n < total; n++) {
		// Detect if a JoyStick is a game-controller, and if so, open it.
		if( SDL_IsGameController(n) ) {
			s_GameCtrls[n] = SDL_GameControllerOpen(n);
			if( ! s_GameCtrls[n] ) {
				printf("Could not open gamecontroller %d: %s.\n",
				       n - 1, SDL_GetError());
			}
			else {
				printf("Opening Game Controller %d, name=%s\n", n, SDL_GameControllerName(s_GameCtrls[n]));
			}
		}
	}

	s_gcinited = 1;
	return 1;
}

#endif //#if SDL_VERSION_ATLEAST(2, 0, 0)

