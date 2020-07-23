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
/// \brief Handles joystick input using the SDL.

#include "Qt/sdl.h"

#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>

#define MAX_JOYSTICKS	32
struct jsDev_t
{
   SDL_Joystick *js;
	SDL_GameController *gc;

	jsDev_t(void)
	{
   	js = NULL;
		gc = NULL;
	};	

	//~jsDev_t(void)
	//{
	//	if ( js )
	//	{
	//		SDL_JoystickClose( js ); js = NULL;
	//	}
	//	if ( gc )
	//	{
	//		SDL_GameControllerClose( gc ); gc = NULL;
	//	}
	//}

	int close(void)
	{
		if ( js )
		{
			SDL_JoystickClose( js ); js = NULL;
		}
		if ( gc )
		{
			SDL_GameControllerClose( gc ); gc = NULL;
		}
		return 0;
	}

	SDL_Joystick *getJS(void)
	{
		if ( gc != NULL )
		{
			return SDL_GameControllerGetJoystick( gc );
		}
		return js;
	}

	bool isGameController(void)
	{
		return ( gc != NULL );
	}

	bool inUse(void)
	{
		return ( (js != NULL) || (gc != NULL) );
	}

	void print(void)
	{
		char guidStr[64];

		SDL_JoystickGUID guid = SDL_JoystickGetGUID( getJS() );

		SDL_JoystickGetGUIDString( guid, guidStr, sizeof(guidStr) );

		printf("JoyStickID: %i: '%s'  \n", 
			SDL_JoystickInstanceID( getJS() ),	SDL_JoystickName( getJS() ) );
		printf("GUID: %s \n", guidStr );
		printf("NumAxes: %i \n", SDL_JoystickNumAxes(getJS()) );
		printf("NumButtons: %i \n", SDL_JoystickNumButtons(getJS()) );
		printf("NumHats: %i \n", SDL_JoystickNumHats(getJS()) );

		if ( gc )
		{
			printf("GameController Name: '%s'\n", SDL_GameControllerName(gc) );
			printf("GameController Mapping: %s\n", SDL_GameControllerMapping(gc) );
		}
	}
};

static jsDev_t  jsDev[ MAX_JOYSTICKS ];
//static SDL_Joystick *s_Joysticks[MAX_JOYSTICKS] = {NULL};

static int s_jinited = 0;


/**
 * Tests if the given button is active on the joystick.
 */
int
DTestButtonJoy(ButtConfig *bc)
{
	int x;
   SDL_Joystick *js;

	for(x = 0; x < bc->NumC; x++)
	{
		if (bc->ButtonNum[x] == -1)
		{
			continue;
		}
		js = jsDev[bc->DeviceNum[x]].getJS();

		if (bc->ButtonNum[x] & 0x2000)
		{
			/* Hat "button" */
			if(SDL_JoystickGetHat( js,
								((bc->ButtonNum[x] >> 8) & 0x1F)) & 
								(bc->ButtonNum[x]&0xFF))
				return 1; 
		}
		else if (bc->ButtonNum[x] & 0x8000) 
		{
			/* Axis "button" */
			int pos;
			pos = SDL_JoystickGetAxis( js,
									bc->ButtonNum[x] & 16383);
			if ((bc->ButtonNum[x] & 0x4000) && pos <= -16383) {
				return 1;
			} else if (!(bc->ButtonNum[x] & 0x4000) && pos >= 16363) {
				return 1;
			}
		} 
		else if(SDL_JoystickGetButton( js,
									bc->ButtonNum[x]))
		return 1;
	}
	return 0;
}


//static void printJoystick( SDL_Joystick *js )
//{
//	char guidStr[64];
//   SDL_Joystick *js;
//
//	js = jsDev[i].getJS();
//
//	SDL_JoystickGUID guid = SDL_JoystickGetGUID( js );
//
//	SDL_JoystickGetGUIDString( guid, guidStr, sizeof(guidStr) );
//
//	printf("JoyStickID: %i: %s  \n", 
//		SDL_JoystickInstanceID( js ),	SDL_JoystickName( js ) );
//	printf("GUID: %s \n", guidStr );
//	printf("NumAxes: %i \n", SDL_JoystickNumAxes(js) );
//	printf("NumButtons: %i \n", SDL_JoystickNumButtons(js) );
//	printf("NumHats: %i \n", SDL_JoystickNumHats(js) );
//
//}

/**
 * Shutdown the SDL joystick subsystem.
 */
int
KillJoysticks(void)
{
	int n;  /* joystick index */

	if (!s_jinited) {
		return -1;
	}

	for (n = 0; n < MAX_JOYSTICKS; n++) 
	{
		jsDev[n].close();
	}
	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
	return 0;
}

int AddJoystick( int which )
{
	if ( jsDev[ which ].inUse() )
	{
		//printf("Error: Joystick already exists at device index %i \n", which );
		return -1;
	}
	else
	{
		if ( SDL_IsGameController(which) )
		{
			jsDev[which].gc = SDL_GameControllerOpen(which);

			if ( jsDev[which].gc == NULL )
			{
				printf("Could not open game controller %d: %s.\n",
					which, SDL_GetError());
			}
			else
			{
				printf("Added Joystick: %i \n", which );
				jsDev[which].print();
				//printJoystick( s_Joysticks[which] );
			}
		}
		else
		{
			jsDev[which].js = SDL_JoystickOpen(which);

			if ( jsDev[which].js == NULL )
			{
				printf("Could not open joystick %d: %s.\n",
					which, SDL_GetError());
			}
			else
			{
				printf("Added Joystick: %i \n", which );
				jsDev[which].print();
				//printJoystick( s_Joysticks[which] );
			}
		}
	}
	return 0;
}

int RemoveJoystick( int which )
{
	//printf("Remove Joystick: %i \n", which );

	for (int i=0; i<MAX_JOYSTICKS; i++)
	{
		if ( jsDev[i].inUse() )
		{
			if ( SDL_JoystickInstanceID( jsDev[i].getJS() ) == which )
			{
				printf("Remove Joystick: %i \n", which );
				jsDev[i].close();
				return 0;
			}
		}
	}
	return -1;
}

/**
 * Initialize the SDL joystick subsystem.
 */
int
InitJoysticks(void)
{
	int n; /* joystick index */
	int total;

	if (s_jinited) {
		return 1;
	}
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);

	total = SDL_NumJoysticks();
	if (total > MAX_JOYSTICKS) 
	{
		total = MAX_JOYSTICKS;
	}

	for (n = 0; n < total; n++) 
	{
		/* Open the joystick under SDL. */
		AddJoystick(n);
		//if ( SDL_IsGameController(n) )
		//{
		//	SDL_GameController *gc = SDL_GameControllerOpen(n);
		//	printf("Is Game Controller: %i \n", n);

		//	printf("Mapping: %s \n", SDL_GameControllerMapping(gc) );
		//}
		//	s_Joysticks[n] = SDL_JoystickOpen(n);

		//if ( s_Joysticks[n] == NULL )
		//{
		//	printf("Could not open joystick %d: %s.\n",
		//		n, SDL_GetError());
		//}
		//else
		//{
		//	printf("Opened JS %i: \n", SDL_JoystickInstanceID( s_Joysticks[n] ) );
		//	jsDev[which].print();
		//	//printJoystick( s_Joysticks[n] );
		//}
	}

	s_jinited = 1;
	return 1;
}
