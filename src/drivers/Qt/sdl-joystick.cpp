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
#include "Qt/sdl-joystick.h"

#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>

//#define MAX_JOYSTICKS	32

// Public Variables
GamePad_t GamePad[4];

// Static Functions
static int sdlButton2NesGpIdx( const char *id );

// Static Variables
static int s_jinited = 0;

//********************************************************************************
// Joystick Device 
jsDev_t::jsDev_t(void)
{
	js = NULL;
	gc = NULL;
};	

//********************************************************************************
int jsDev_t::close(void)
{
	if ( gc )
	{
		SDL_GameControllerClose( gc ); gc = NULL; js = NULL;
	}
	else
	{
		if ( js )
		{
			SDL_JoystickClose( js ); js = NULL;
		}
	}
	return 0;
}

//********************************************************************************
SDL_Joystick *jsDev_t::getJS(void)
{
	return js;
}

//********************************************************************************
const char *jsDev_t::getName(void)
{
	return ( name.c_str() );
}

//********************************************************************************
const char *jsDev_t::getGUID(void)
{
	return ( guidStr.c_str() );
}

//********************************************************************************
bool jsDev_t::isGameController(void)
{
	return ( gc != NULL );
}

//********************************************************************************
bool jsDev_t::inUse(void)
{
	return ( (js != NULL) || (gc != NULL) );
}

//********************************************************************************
void jsDev_t::init( int idx )
{
	SDL_JoystickGUID guid;
	char stmp[64];

	devIdx = idx;

	if ( gc ) 
	{
		js = SDL_GameControllerGetJoystick( gc );

		guid = SDL_JoystickGetGUID( js );

		name.assign( SDL_GameControllerName(gc) );
	}
	else
	{
		guid = SDL_JoystickGetGUID( js );

		name.assign( SDL_JoystickName(js) );
	}
	SDL_JoystickGetGUIDString( guid, stmp, sizeof(stmp) );

	guidStr.assign( stmp );

}

void jsDev_t::print(void)
{
	char guidStr[64];

	SDL_JoystickGUID guid = SDL_JoystickGetGUID( js );

	SDL_JoystickGetGUIDString( guid, guidStr, sizeof(guidStr) );

	printf("JoyStickID: %i: '%s'  \n", 
		SDL_JoystickInstanceID( js ),	SDL_JoystickName( js ) );
	printf("GUID: %s \n", guidStr );
	printf("NumAxes: %i \n", SDL_JoystickNumAxes(js) );
	printf("NumButtons: %i \n", SDL_JoystickNumButtons(js) );
	printf("NumHats: %i \n", SDL_JoystickNumHats(js) );

	if ( gc )
	{
		printf("GameController Name: '%s'\n", SDL_GameControllerName(gc) );
		printf("GameController Mapping: %s\n", SDL_GameControllerMapping(gc) );
	}
}

static jsDev_t  jsDev[ MAX_JOYSTICKS ];

//********************************************************************************
nesGamePadMap_t::nesGamePadMap_t(void)
{
   memset( guid, 0, sizeof(guid) );
   memset( name, 0, sizeof(name) );
   memset( os  , 0, sizeof(os) );
   memset( btn , 0, sizeof(btn) );
}
//********************************************************************************
nesGamePadMap_t::~nesGamePadMap_t(void)
{

}
//********************************************************************************
int nesGamePadMap_t::parseMapping( const char *map )
{
   int i,j,k,bIdx;
	char id[32][64];
	char val[32][64];

   i=0; j=0; k=0;

	while ( map[i] )
	{
		while ( isspace(map[i]) ) i++;

		j=0;
		while ( (map[i] != 0) && (map[i] != ',') && (map[i] != ':') )
		{
			id[k][j] = map[i]; i++; j++;
		}
		id[k][j] = 0;
		val[k][0] = 0;

		if ( map[i] == ':' )
		{
			i++; j=0;

			while ( (map[i] != 0) && (map[i] != ',') )
			{
				val[k][j] = map[i]; i++; j++;
			}
			val[k][j] = 0;
		}

		if ( map[i] == ',' )
		{
			k++; i++;
		}
		else
		{
			break;
		}
	}

   strcpy( guid, id[0] ); // GUID is always 1st field
   strcpy( name, id[1] ); // Name is always 2nd field

	for (i=0; i<k; i++)
	{
		bIdx = sdlButton2NesGpIdx( id[i] );

		//printf(" '%s' = '%s'  %i \n", id[i], val[i], bIdx );
		if ( bIdx >= 0 )
		{
         strcpy( btn[bIdx], val[i] );
      }
      else if ( strcmp( id[i], "platform" ) == 0 )
      {
         strcpy( os, val[i] );
      }
	}
}
//********************************************************************************
GamePad_t::GamePad_t(void)
{
	devIdx = 0;

	for (int i=0; i<GAMEPAD_NUM_BUTTONS; i++)
	{
		bmap[i].ButtType  =  BUTTC_KEYBOARD;
		bmap[i].DeviceNum = -1;
		bmap[i].ButtonNum = -1;
	}
}
//********************************************************************************
GamePad_t::~GamePad_t(void)
{

}
//********************************************************************************
static int sdlButton2NesGpIdx( const char *id )
{
	int idx = -1;

	if ( strcmp( id, "a" ) == 0 )
	{
		idx = 0;
	}
	else if ( strcmp( id, "b" ) == 0 )
	{
		idx = 1;
	}
	else if ( strcmp( id, "back" ) == 0 )
	{
		idx = 2;
	}
	else if ( strcmp( id, "start" ) == 0 )
	{
		idx = 3;
	}
	else if ( strcmp( id, "dpup" ) == 0 )
	{
		idx = 4;
	}
	else if ( strcmp( id, "dpdown" ) == 0 )
	{
		idx = 5;
	}
	else if ( strcmp( id, "dpleft" ) == 0 )
	{
		idx = 6;
	}
	else if ( strcmp( id, "dpright" ) == 0 )
	{
		idx = 7;
	}
	else if ( strcmp( id, "turboA" ) == 0 )
	{
		idx = 8;
	}
	else if ( strcmp( id, "turboB" ) == 0 )
	{
		idx = 9;
	}

	return idx;
}
//********************************************************************************
int GamePad_t::setMapping( nesGamePadMap_t *gpm )
{
   for (int i=0; i<GAMEPAD_NUM_BUTTONS; i++)
   {
		bmap[i].ButtType  = BUTTC_KEYBOARD;
		bmap[i].DeviceNum = -1;
		bmap[i].ButtonNum = -1;

      if (gpm->btn[i][0] == 'k')
      {
		   bmap[i].ButtType  = BUTTC_KEYBOARD;
		   bmap[i].DeviceNum = 0;
   		bmap[i].ButtonNum = 0; // FIXME
      }
      else if ( (gpm->btn[i][0] == 'b') && isdigit( gpm->btn[i][1] ) )
   	{
		   bmap[i].ButtType  = BUTTC_JOYSTICK;
		   bmap[i].DeviceNum = devIdx;
   		bmap[i].ButtonNum = atoi( &gpm->btn[i][1] );
   	}
   	else if ( (gpm->btn[i][0] == 'h') && isdigit( gpm->btn[i][1] ) &&
   					(gpm->btn[i][2] == '.') && isdigit( gpm->btn[i][3] ) )
   	{
   		int hatIdx, hatVal;
   
   		hatIdx = gpm->btn[i][1] - '0';
   		hatVal = atoi( &gpm->btn[i][3] );
   
		   bmap[i].ButtType  = BUTTC_JOYSTICK;
		   bmap[i].DeviceNum = devIdx;
   		bmap[i].ButtonNum = 0x2000 | ( (hatIdx & 0x1F) << 8) | (hatVal & 0xFF);
   	}
   	else if ( (gpm->btn[i][0] == 'a') || (gpm->btn[i][1] == 'a') )
   	{
   		int l=0, axisIdx=0, axisSign=0;
   
   		l=0;
   		if ( gpm->btn[i][l] == '-' )
   		{
   			axisSign = 1; l++;
   		}
   		else if ( gpm->btn[i][l] == '+' )
   		{
   			axisSign = 0; l++;
   		}
   
   		if ( gpm->btn[i][l] == 'a' )
   		{
   			l++;
   		}
   		if ( isdigit( gpm->btn[i][l] ) )
   		{
   			axisIdx = atoi( &gpm->btn[i][l] );

            while ( isdigit(gpm->btn[i][l]) ) l++;
   		}
         if ( gpm->btn[i][l] == '-' )
   		{
   			axisSign = 1; l++;
   		}
   		else if ( gpm->btn[i][l] == '+' )
   		{
   			axisSign = 0; l++;
   		}
		   bmap[i].ButtType  = BUTTC_JOYSTICK;
		   bmap[i].DeviceNum = devIdx;
   		bmap[i].ButtonNum = 0x8000 | (axisSign ? 0x4000 : 0) | (axisIdx & 0xFF);
   	}
	}
   return 0;
}
//********************************************************************************
int GamePad_t::setMapping( const char *map )
{
   nesGamePadMap_t gpm;

   gpm.parseMapping( map );
   setMapping( &gpm );

	return 0;
}
//********************************************************************************
int GamePad_t::loadDefaults(void)
{

	if ( jsDev[ devIdx ].gc )
	{	
		char *mapping;

		mapping = SDL_GameControllerMapping( jsDev[ devIdx ].gc );

		if ( mapping == NULL ) return -1;

		setMapping( mapping );

   	SDL_free(mapping);
	}
	return 0;
}
//********************************************************************************
//********************************************************************************
jsDev_t *getJoystickDevice( int devNum )
{
	if ( (devNum >= 0) && (devNum < MAX_JOYSTICKS) )
	{
		return &jsDev[ devNum ];
	}
	return NULL;
}

//********************************************************************************
/**
 * Tests if the given button is active on the joystick.
 */
int
DTestButtonJoy(ButtConfig *bc)
{
   SDL_Joystick *js;

	if (bc->ButtonNum == -1)
	{
		return 0;
	}
	js = jsDev[bc->DeviceNum].getJS();

	if (bc->ButtonNum & 0x2000)
	{
		/* Hat "button" */
		if (SDL_JoystickGetHat( js,
							((bc->ButtonNum >> 8) & 0x1F)) & 
							(bc->ButtonNum&0xFF))
      {
         bc->state = 1;
			return 1; 
      }
      else
      {
         bc->state = 0;
      }
	}
	else if (bc->ButtonNum & 0x8000) 
	{
		/* Axis "button" */
		int pos;
		pos = SDL_JoystickGetAxis( js,
								bc->ButtonNum & 16383);
		if ((bc->ButtonNum & 0x4000) && pos <= -16383) 
      {
         bc->state = 1;
			return 1;
		}
      else if (!(bc->ButtonNum & 0x4000) && pos >= 16363) 
      {
         bc->state = 1;
			return 1;
		}
      else
      {
         bc->state = 0;
      }
	} 
	else if (SDL_JoystickGetButton( js,
								bc->ButtonNum))
	{
      bc->state = 1;
		return 1;
   }
   else
   {
      bc->state = 0;
	}

	return 0;
}
//********************************************************************************


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

//********************************************************************************
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

//********************************************************************************
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
				jsDev[which].init(which);
				//jsDev[which].print();
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
				jsDev[which].init(which);
				//jsDev[which].print();
				//printJoystick( s_Joysticks[which] );
			}
		}
	}
	return 0;
}

//********************************************************************************
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

//********************************************************************************
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
	}

	s_jinited = 1;
	return 1;
}
//********************************************************************************
