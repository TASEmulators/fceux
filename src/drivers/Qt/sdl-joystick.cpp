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

#include <QDir>
#include "Qt/sdl.h"
#include "Qt/sdl-joystick.h"
#include "Qt/config.h"

#include <cstdlib>
//#include <unistd.h>
//#include <fcntl.h>
#include <cerrno>

//#define MAX_JOYSTICKS	32

// Public Variables
GamePad_t GamePad[4];

// Static Functions
static int sdlButton2NesGpIdx(const char *id);

// Static Variables
static int s_jinited = 0;

static const char *buttonNames[GAMEPAD_NUM_BUTTONS] =
	{
		"a", "b", "back", "start",
		"dpup", "dpdown", "dpleft", "dpright",
		"turboA", "turboB"};

//********************************************************************************
// Joystick Device
jsDev_t::jsDev_t(void)
{
	js = NULL;
	gc = NULL;
	devIdx = 0;
	portBindMask = 0;
};

//********************************************************************************
int jsDev_t::close(void)
{
	if (gc)
	{
		SDL_GameControllerClose(gc);
		gc = NULL;
		js = NULL;
	}
	else
	{
		if (js)
		{
			SDL_JoystickClose(js);
			js = NULL;
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
	return (name.c_str());
}

//********************************************************************************
const char *jsDev_t::getGUID(void)
{
	return (guidStr.c_str());
}

//********************************************************************************
int jsDev_t::bindPort(int idx)
{
	portBindMask |= (0x00000001 << idx);

	return portBindMask;
}
//********************************************************************************
int jsDev_t::unbindPort(int idx)
{
	portBindMask &= ~(0x00000001 << idx);

	return portBindMask;
}
//********************************************************************************
int jsDev_t::getBindPorts(void)
{
	return portBindMask;
}
//********************************************************************************
bool jsDev_t::isGameController(void)
{
	return (gc != NULL);
}

//********************************************************************************
bool jsDev_t::isConnected(void)
{
	return ((js != NULL) || (gc != NULL));
}

//********************************************************************************
void jsDev_t::init(int idx)
{
	SDL_JoystickGUID guid;
	char stmp[64];

	devIdx = idx;

	if (gc)
	{
		js = SDL_GameControllerGetJoystick(gc);

		guid = SDL_JoystickGetGUID(js);

		name.assign(SDL_GameControllerName(gc));
	}
	else
	{
		guid = SDL_JoystickGetGUID(js);

		name.assign(SDL_JoystickName(js));
	}
	SDL_JoystickGetGUIDString(guid, stmp, sizeof(stmp));

	guidStr.assign(stmp);

	// If game controller, save default mapping if it does not already exist.
	if (gc)
	{
		QDir dir;
		QFile defaultMapFile;
		std::string path;
		const char *baseDir = FCEUI_GetBaseDirectory();

		path = std::string(baseDir) + "/input/" + guidStr;

		dir.mkpath(QString::fromStdString(path));

		path += "/default.txt";

		defaultMapFile.setFileName(QString::fromStdString(path));

		if (!defaultMapFile.exists())
		{
			FILE *fp;

			fp = ::fopen(path.c_str(), "w");

			if (fp != NULL)
			{
				const char *defaultMap;

				defaultMap = SDL_GameControllerMapping(gc);

				if (defaultMap)
				{
					//printf("GameController Mapping: %s\n", defaultMap );
					fprintf(fp, "%s", defaultMap);
				}
				::fclose(fp);
			}
		}
		//else
		//{
		//	printf("GameController Mapping Exists: '%s'\n", path.c_str() );
		//}
	}
}

void jsDev_t::print(void)
{
	char guidStr[64];

	SDL_JoystickGUID guid = SDL_JoystickGetGUID(js);

	SDL_JoystickGetGUIDString(guid, guidStr, sizeof(guidStr));

	printf("JoyStickID: %i: '%s'  \n",
		   SDL_JoystickInstanceID(js), SDL_JoystickName(js));
	printf("GUID: %s \n", guidStr);
	printf("NumAxes: %i \n", SDL_JoystickNumAxes(js));
	printf("NumButtons: %i \n", SDL_JoystickNumButtons(js));
	printf("NumHats: %i \n", SDL_JoystickNumHats(js));

	if (gc)
	{
		printf("GameController Name: '%s'\n", SDL_GameControllerName(gc));
		printf("GameController Mapping: %s\n", SDL_GameControllerMapping(gc));
	}
}

static jsDev_t jsDev[MAX_JOYSTICKS];

//********************************************************************************
nesGamePadMap_t::nesGamePadMap_t(void)
{
	clearMapping();
}
//********************************************************************************
nesGamePadMap_t::~nesGamePadMap_t(void)
{
}
//********************************************************************************
void nesGamePadMap_t::clearMapping(void)
{
	guid[0] = 0;
	name[0] = 0;
	os[0] = 0;

	for (int c = 0; c < 4; c++)
	{
		for (int i = 0; i < GAMEPAD_NUM_BUTTONS; i++)
		{
			conf[c].btn[i][0] = 0;
		}
	}
}
//********************************************************************************
int nesGamePadMap_t::parseMapping(const char *map)
{
	int i, j, k, c, bIdx;
	char id[32][64];
	char val[32][64];

	//clearMapping();

	i = 0;
	j = 0;
	k = 0;
	c = 0;

	while (map[i])
	{
		while (isspace(map[i]))
			i++;

		j = 0;
		while ((map[i] != 0) && (map[i] != ',') && (map[i] != ':'))
		{
			id[k][j] = map[i];
			i++;
			j++;
		}
		id[k][j] = 0;
		val[k][0] = 0;

		if (map[i] == ':')
		{
			i++;
			j = 0;

			while ((map[i] != 0) && (map[i] != ','))
			{
				val[k][j] = map[i];
				i++;
				j++;
			}
			val[k][j] = 0;
		}

		if (map[i] == ',')
		{
			k++;
			i++;
		}
		else
		{
			break;
		}
	}

	strcpy(guid, id[0]); // GUID is always 1st field
	strcpy(name, id[1]); // Name is always 2nd field

	for (i = 0; i < k; i++)
	{
		bIdx = sdlButton2NesGpIdx(id[i]);

		//printf(" '%s' = '%s'  %i \n", id[i], val[i], bIdx );
		if (bIdx >= 0)
		{
			strcpy( conf[c].btn[bIdx], val[i]);
		}
		else if (strcmp(id[i], "config") == 0)
		{
			c = atoi(val[i]);
		}
		else if (strcmp(id[i], "platform") == 0)
		{
			strcpy(os, val[i]);
		}
	}
	return 0;
}
//********************************************************************************
GamePad_t::GamePad_t(void)
{
	devIdx = -1;
	portNum = 0;

	for (int c = 0; c < NUM_CONFIG; c++)
	{
		for (int i = 0; i < GAMEPAD_NUM_BUTTONS; i++)
		{
			bmap[c][i].ButtType = BUTTC_KEYBOARD;
			bmap[c][i].DeviceNum = -1;
			bmap[c][i].ButtonNum = -1;
			bmap[c][i].state = 0;
		}
	}
}
//********************************************************************************
GamePad_t::~GamePad_t(void)
{
	deleteHotKeyMappings();
}
//********************************************************************************
int GamePad_t::init(int port, const char *guid, const char *profile)
{
	int i, mask;

	portNum = port;

	//printf("Init: %i   %s   %s \n", port, guid, profile );

	// First look for a controller that matches the specific GUID
	// that is not already in use by another port.
	if (devIdx < 0)
	{
		for (i = 0; i < MAX_JOYSTICKS; i++)
		{
			mask = jsDev[i].getBindPorts();

			if (mask != 0)
			{
				continue;
			}
			if (!jsDev[i].isConnected())
			{
				continue;
			}

			if (strcmp(jsDev[i].getGUID(), guid) == 0)
			{
				setDeviceIndex(i);
				if (loadProfile(profile, guid))
				{
					loadDefaults();
				}
				break;
			}
		}
	}

	// If a specific controller was not matched,
	// then look for any game controller that is plugged in
	// and not already bound.
	if ( (devIdx < 0) && (strcmp(guid, "keyboard") != 0) )
	{
		for (i = 0; i < MAX_JOYSTICKS; i++)
		{
			mask = jsDev[i].getBindPorts();

			if (mask != 0)
			{
				continue;
			}

			if (jsDev[i].isGameController())
			{
				setDeviceIndex(i);
				if (loadProfile(profile))
				{
					loadDefaults();
				}
				break;
			}
		}
	}

	// If we get to this point and still have not found a
	// game controller, then load default keyboard.
	if ((portNum == 0) && (devIdx < 0))
	{
		if (loadProfile(profile))
		{
			loadDefaults();
		}
	}

	return 0;
}
//********************************************************************************
int GamePad_t::setDeviceIndex(int in)
{
	if (devIdx >= 0)
	{
		jsDev[devIdx].unbindPort(portNum);
	}

	devIdx = in;

	if (devIdx >= 0)
	{
		jsDev[devIdx].bindPort(portNum);
	}
	return 0;
}
//********************************************************************************
const char *GamePad_t::getGUID(void)
{
	if (devIdx < 0)
	{
		return "keyboard";
	}
	if (jsDev[devIdx].isConnected())
	{
		return jsDev[devIdx].getGUID();
	}
	return NULL;
}
//********************************************************************************
static int sdlButton2NesGpIdx(const char *id)
{
	int i, idx = -1;

	for (i = 0; i < GAMEPAD_NUM_BUTTONS; i++)
	{
		if (strcmp(id, buttonNames[i]) == 0)
		{
			idx = i;
			break;
		}
	}

	return idx;
}
//********************************************************************************
int GamePad_t::convText2ButtConfig( const char *txt, ButtConfig *bmap )
{
	bmap->ButtType  = -1;
	bmap->DeviceNum = -1;
	bmap->ButtonNum = -1;

	if (txt[0] == 0 )
	{
		return 0;
	}

	if (txt[0] == 'k')
	{
		SDL_Keycode key;

		bmap->ButtType = BUTTC_KEYBOARD;
		bmap->DeviceNum = -1;

		key = SDL_GetKeyFromName(&txt[1]);

		if (key != SDLK_UNKNOWN)
		{
			bmap->ButtonNum = key;
		}
		else
		{
			bmap->ButtonNum = -1;
		}
	}
	else if ((txt[0] == 'b') && isdigit(txt[1]))
	{
		bmap->ButtType = BUTTC_JOYSTICK;
		bmap->DeviceNum = devIdx;
		bmap->ButtonNum = atoi(&txt[1]);
	}
	else if ((txt[0] == 'h') && isdigit(txt[1]) &&
			 (txt[2] == '.') && isdigit(txt[3]))
	{
		int hatIdx, hatVal;

		hatIdx = txt[1] - '0';
		hatVal = atoi(&txt[3]);

		bmap->ButtType = BUTTC_JOYSTICK;
		bmap->DeviceNum = devIdx;
		bmap->ButtonNum = 0x2000 | ((hatIdx & 0x1F) << 8) | (hatVal & 0xFF);
	}
	else if ((txt[0] == 'a') || (txt[1] == 'a'))
	{
		int l = 0, axisIdx = 0, axisSign = 0;

		l = 0;
		if (txt[l] == '-')
		{
			axisSign = 1;
			l++;
		}
		else if (txt[l] == '+')
		{
			axisSign = 0;
			l++;
		}

		if (txt[l] == 'a')
		{
			l++;
		}
		if (isdigit(txt[l]))
		{
			axisIdx = atoi(&txt[l]);

			while (isdigit(txt[l]))
				l++;
		}
		if (txt[l] == '-')
		{
			axisSign = 1;
			l++;
		}
		else if (txt[l] == '+')
		{
			axisSign = 0;
			l++;
		}
		bmap->ButtType = BUTTC_JOYSTICK;
		bmap->DeviceNum = devIdx;
		bmap->ButtonNum = 0x8000 | (axisSign ? 0x4000 : 0) | (axisIdx & 0xFF);
	}

	return 0;
}
//********************************************************************************
int GamePad_t::setMapping(nesGamePadMap_t *gpm)
{
	for (int c = 0; c < NUM_CONFIG; c++)
	{
		for (int i = 0; i < GAMEPAD_NUM_BUTTONS; i++)
		{
			bmap[c][i].ButtType = BUTTC_KEYBOARD;
			bmap[c][i].DeviceNum = -1;
			bmap[c][i].ButtonNum = -1;

			if (gpm->conf[c].btn[i][0] == 0)
			{
				continue;
			}
			convText2ButtConfig( gpm->conf[c].btn[i], &bmap[c][i] );
		}
	}
	return 0;
}
//********************************************************************************
int GamePad_t::setMapping(const char *map)
{
	nesGamePadMap_t gpm;

	gpm.parseMapping(map);
	setMapping(&gpm);

	return 0;
}
//********************************************************************************
int GamePad_t::getMapFromFile(const char *filename, nesGamePadMap_t *gpm)
{
	int i = 0;
	FILE *fp;
	char line[256];

	fp = ::fopen(filename, "r");

	if (fp == NULL)
	{
		return -1;
	}
	while (fgets(line, sizeof(line), fp) != 0)
	{
		i = 0;
		while (line[i] != 0)
		{
			if (line[i] == '#')
			{
				line[i] = 0;
				break;
			}
			i++;
		}

		if (i < 32)
			continue; // need at least 32 chars for a valid line entry

		i = 0;
		while (isspace(line[i]))
			i++;

		gpm->parseMapping( &line[i] );
	}

	::fclose(fp);

	return 0;
}
//********************************************************************************
int GamePad_t::deleteHotKeyMappings(void)
{
	while ( !gpKeySeqList.empty() )
	{
		delete gpKeySeqList.back();

		gpKeySeqList.pop_back();
	}
	return 0;
}
//********************************************************************************
int GamePad_t::loadHotkeyMapFromFile(const char *filename)
{
	int i = 0, j = 0;
	FILE *fp;
	char line[256];
	char id[128];
	char val[128];
	bool lineIsHotKey = false;
	char modBtn[32], priBtn[32];
	char onPressAct[64], onReleaseAct[64];

	//printf("Loading HotKey Map From File: %s\n", filename );

	fp = ::fopen(filename, "r");

	if (fp == NULL)
	{
		return -1;
	}
	deleteHotKeyMappings();

	while (fgets(line, sizeof(line), fp) != 0)
	{
		i = 0; j = 0;
		while (line[i] != 0)
		{
			if (line[i] == '#')
			{
				line[i] = 0;
				break;
			}
			i++;
		}
		lineIsHotKey = false;
		modBtn[0] = 0; priBtn[0] = 0;
		onPressAct[0] = 0; onReleaseAct[0] = 0;

		i=0; j=0;
		while (line[i])
		{
			j=0;

			while ((line[i] != 0) && (line[i] != ',') && (line[i] != ':'))
			{
				id[j] = line[i];
				i++;
				j++;
			}
			id[j] = 0;
			val[0] = 0;

			if (line[i] == ':')
			{
				i++;
				j = 0;

				while ((line[i] != 0) && (line[i] != ','))
				{
					val[j] = line[i];
					i++;
					j++;
				}
				val[j] = 0;
			}

			if ( strcmp( id, "hotkey" ) == 0 )
			{
				lineIsHotKey = true;
			}
			else if ( strcmp( id, "modifier" ) == 0 )
			{
				strcpy( modBtn, val );
			}
			else if ( strcmp( id, "button" ) == 0 )
			{
				strcpy( priBtn, val );
			}
			else if ( strcmp( id, "press" ) == 0 )
			{
				strcpy( onPressAct, val );
			}
			else if ( strcmp( id, "release" ) == 0 )
			{
				strcpy( onReleaseAct, val );
			}


			if (line[i] == ',')
			{
				i++;
			}
			else
			{
				break;
			}
		}

		if ( lineIsHotKey && (priBtn[0] != 0) && 
			( (onPressAct[0] != 0) || (onReleaseAct[0] != 0) ) )
		{
			gamepad_function_key_t *fk = new gamepad_function_key_t();

			convText2ButtConfig( modBtn, &fk->bmap[0] );
			convText2ButtConfig( priBtn, &fk->bmap[1] );

			fk->hk[0] = getHotKeyIndexByName( onPressAct   );
			fk->hk[1] = getHotKeyIndexByName( onReleaseAct );

			if ( fk->hk[0] >= 0 )
			{
				fk->keySeq[0].name.assign( onPressAct );
			}

			if ( fk->hk[1] >= 0 )
			{
				fk->keySeq[1].name.assign( onReleaseAct );
			}
			gpKeySeqList.push_back( fk );
		}

	}
	::fclose(fp);

	return 0;
}
//********************************************************************************
int GamePad_t::getDefaultMap(const char *guid)
{
	const char *baseDir = FCEUI_GetBaseDirectory();
	std::string path;
	nesGamePadMap_t gpm;

	if (devIdx < 0)
	{
		guid = "keyboard";
	}
	if (guid == NULL)
	{
		if (jsDev[devIdx].isConnected())
		{
			guid = jsDev[devIdx].getGUID();
		}
	}
	if (guid == NULL)
	{
		return -1;
	}

	path = std::string(baseDir) + "/input/" + std::string(guid) + "/default.txt";

	if (getMapFromFile(path.c_str(), &gpm) == 0)
	{
		//printf("Using Mapping From File: %s\n", path.c_str());
		setMapping(&gpm);
		loadHotkeyMapFromFile( path.c_str() );
		return 0;
	}

	if (devIdx >= 0)
	{
		if (jsDev[devIdx].gc)
		{
			char *sdlMapping;

			sdlMapping = SDL_GameControllerMapping(jsDev[devIdx].gc);

			if (sdlMapping == NULL)
				return -1;

			gpm.parseMapping(sdlMapping);

			setMapping(&gpm);

			SDL_free(sdlMapping);

			return 0;
		}
	}
	else
	{
		if (strcmp(guid, "keyboard") == 0)
		{
			for (int x = 0; x < GAMEPAD_NUM_BUTTONS; x++)
			{
				bmap[0][x].ButtType = BUTTC_KEYBOARD;
				bmap[0][x].DeviceNum = 0;
				bmap[0][x].ButtonNum = DefaultGamePad[portNum][x];
			}
		}
	}
	return -1;
}
//********************************************************************************
int GamePad_t::loadDefaults(void)
{
	getDefaultMap();

	return 0;
}
//********************************************************************************
int GamePad_t::loadProfile(const char *name, const char *guid)
{
	nesGamePadMap_t gpm;
	const char *baseDir = FCEUI_GetBaseDirectory();
	std::string path;

	if (devIdx < 0)
	{
		guid = "keyboard";
	}
	if (guid == NULL)
	{
		if (jsDev[devIdx].isConnected())
		{
			guid = jsDev[devIdx].getGUID();
		}
	}
	if (guid == NULL)
	{
		return -1;
	}

	path = std::string(baseDir) + "/input/" + std::string(guid) +
		   "/" + std::string(name) + ".txt";

	//printf("Using File: %s\n", path.c_str() );

	if (getMapFromFile(path.c_str(), &gpm) == 0)
	{
		setMapping( &gpm );
		loadHotkeyMapFromFile( path.c_str() );
		return 0;
	}

	return -1;
}
//********************************************************************************
int GamePad_t::saveCurrentMapToFile(const char *name)
{
	int i,c;
	char stmp[256];
	const char *guid = NULL;
	const char *baseDir = FCEUI_GetBaseDirectory();
	std::string path, output;
	std::list <gamepad_function_key_t*>::iterator it;
	QDir dir;

	if (devIdx >= 0)
	{
		if (!jsDev[devIdx].isConnected())
		{
			printf("Error: JS%i Not Connected\n", devIdx);
			return -1;
		}
		guid = jsDev[devIdx].getGUID();
	}
	else
	{
		guid = "keyboard";
	}
	path = std::string(baseDir) + "/input/" + std::string(guid);

	dir.mkpath(QString::fromStdString(path));

	path += "/" + std::string(name) + ".txt";

	for (c = 0; c < NUM_CONFIG; c++)
	{
		output.append(guid);
		output.append(",");
		output.append(name);
		output.append(",");
		output.append("config:");
		sprintf( stmp, "%i,", c );
		output.append(stmp);

		for (i = 0; i < GAMEPAD_NUM_BUTTONS; i++)
		{
			if (bmap[c][i].ButtType == BUTTC_KEYBOARD)
			{
				sprintf(stmp, "k%s", SDL_GetKeyName(bmap[c][i].ButtonNum));
			}
			else
			{
				if (bmap[c][i].ButtonNum & 0x2000)
				{
					/* Hat "button" */
					sprintf(stmp, "h%i.%i",
							(bmap[c][i].ButtonNum >> 8) & 0x1F, bmap[c][i].ButtonNum & 0xFF);
				}
				else if (bmap[c][i].ButtonNum & 0x8000)
				{
					/* Axis "button" */
					sprintf(stmp, "%ca%i",
							(bmap[c][i].ButtonNum & 0x4000) ? '-' : '+', bmap[c][i].ButtonNum & 0x3FFF);
				}
				else
				{
					/* Button */
					sprintf(stmp, "b%i", bmap[c][i].ButtonNum);
				}
			}
			output.append(buttonNames[i]);
			output.append(":");
			output.append(stmp);
			output.append(",");
		}
		output.append("\n");
	}

	for (it=gpKeySeqList.begin(); it!=gpKeySeqList.end(); it++)
	{
		gamepad_function_key_t *fk = *it;

		//printf("hk[0]=%i   hk[1]=%i   keySeq[0]=%s   keySeq[1]=%s  bmap[0].buttType=%i  bmap[1].buttType=%i\n", 
		//		fk->hk[0], fk->hk[1], fk->keySeq[0].name.c_str(), fk->keySeq[1].name.c_str(),
		//     			fk->bmap[0].ButtType, fk->bmap[1].ButtType );

		if ( fk->bmap[1].ButtType >= 0 )
		{
			output.append("\nhotkey,");

			for (i = 0; i < 2; i++)
			{
				if ( fk->bmap[i].ButtType >= 0 )
				{
					if (fk->bmap[i].ButtType == BUTTC_KEYBOARD)
					{
						sprintf(stmp, "k%s", SDL_GetKeyName(fk->bmap[i].ButtonNum));
					}
					else
					{
						if (fk->bmap[i].ButtonNum & 0x2000)
						{
							/* Hat "button" */
							sprintf(stmp, "h%i.%i",
									(fk->bmap[i].ButtonNum >> 8) & 0x1F, fk->bmap[i].ButtonNum & 0xFF);
						}
						else if (fk->bmap[i].ButtonNum & 0x8000)
						{
							/* Axis "button" */
							sprintf(stmp, "%ca%i",
									(fk->bmap[i].ButtonNum & 0x4000) ? '-' : '+', fk->bmap[i].ButtonNum & 0x3FFF);
						}
						else
						{
							/* Button */
							sprintf(stmp, "b%i", fk->bmap[i].ButtonNum);
						}
					}
					if ( i == 0 )
					{
						output.append("modifier:");
						output.append(stmp);
						output.append(",");
					}
					else
					{
						output.append("button:");
						output.append(stmp);
						output.append(",");
					}
				}
			}
			for (i = 0; i < 2; i++)
			{
				const char *nameStr, *keySeqStr;

				if ( fk->hk[i] >= 0 )
				{
					getHotKeyConfig( fk->hk[i], &nameStr, &keySeqStr );

					output.append( i ? "release" : "press");
					output.append(":");
					output.append(nameStr);
					output.append(",");
				}
			}
		}
	}

	return saveMappingToFile(path.c_str(), output.c_str());
}
//********************************************************************************
int GamePad_t::saveMappingToFile(const char *filename, const char *txtMap)
{
	FILE *fp;

	fp = ::fopen(filename, "w");

	if (fp == NULL)
	{
		return -1;
	}
	fprintf(fp, "%s\n", txtMap);

	::fclose(fp);

	return 0;
}
//********************************************************************************
int GamePad_t::createProfile(const char *name)
{
	const char *guid = NULL;
	const char *baseDir = FCEUI_GetBaseDirectory();
	std::string path;
	QDir dir;

	if (baseDir[0] == 0)
	{
		printf("Error: Invalid base directory\n");
		return -1;
	}
	if (devIdx >= 0)
	{
		if (!jsDev[devIdx].isConnected())
		{
			printf("Error: JS%i Not Connected\n", devIdx);
			return -1;
		}
		guid = jsDev[devIdx].getGUID();
	}
	else
	{
		guid = "keyboard";
	}
	path = std::string(baseDir) + "/input/" + std::string(guid);

	dir.mkpath(QString::fromStdString(path));
	//printf("DIR: '%s'\n", path.c_str() );

	//path += "/" + std::string(name) + ".txt";

	//printf("File: '%s'\n", path.c_str() );

	saveCurrentMapToFile(name);

	return 0;
}
//********************************************************************************
int GamePad_t::deleteMapping(const char *name)
{
	const char *guid = NULL;
	const char *baseDir = FCEUI_GetBaseDirectory();
	std::string path;

	if (baseDir[0] == 0)
	{
		printf("Error: Invalid base directory\n");
		return -1;
	}
	if (devIdx >= 0)
	{
		if (!jsDev[devIdx].isConnected())
		{
			printf("Error: JS%i Not Connected\n", devIdx);
			return -1;
		}
		guid = jsDev[devIdx].getGUID();
	}
	else
	{
		guid = "keyboard";
	}
	path = std::string(baseDir) + "/input/" + std::string(guid) +
		   "/" + std::string(name) + ".txt";

	//printf("File: '%s'\n", path.c_str() );

	return remove(path.c_str());
}
//********************************************************************************
jsDev_t *getJoystickDevice(int devNum)
{
	if ((devNum >= 0) && (devNum < MAX_JOYSTICKS))
	{
		return &jsDev[devNum];
	}
	return NULL;
}

//********************************************************************************
/**
 * Tests if the given button is active on the joystick.
 */
int DTestButtonJoy(ButtConfig *bc)
{
	SDL_Joystick *js;

	if (bc->ButtonNum == -1)
	{
		return 0;
	}
	if (bc->DeviceNum < 0)
	{
		return 0;
	}
	js = jsDev[bc->DeviceNum].getJS();

	if (bc->ButtonNum & 0x2000)
	{
		/* Hat "button" */
		if (SDL_JoystickGetHat(js,
							   ((bc->ButtonNum >> 8) & 0x1F)) &
			(bc->ButtonNum & 0xFF))
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
		pos = SDL_JoystickGetAxis(js,
								  bc->ButtonNum & 0x3FFF);
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
	else if (SDL_JoystickGetButton(js,
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
int KillJoysticks(void)
{
	int n; /* joystick index */

	if (!s_jinited)
	{
		return -1;
	}

	for (n = 0; n < MAX_JOYSTICKS; n++)
	{
		jsDev[n].close();
	}
	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);

	s_jinited = 0;

	return 0;
}

//********************************************************************************
int AddJoystick(int which)
{
	//printf("Add Joystick: %i \n", which );
	if (jsDev[which].isConnected())
	{
		//printf("Error: Joystick already exists at device index %i \n", which );
		return -1;
	}
	else
	{
		if (SDL_IsGameController(which))
		{
			jsDev[which].gc = SDL_GameControllerOpen(which);

			if (jsDev[which].gc == NULL)
			{
				printf("Could not open game controller %d: %s.\n",
					   which, SDL_GetError());
			}
			else
			{
				//printf("Added Joystick: %i \n", which );
				jsDev[which].init(which);
				//jsDev[which].print();
				//printJoystick( s_Joysticks[which] );
			}
		}
		else
		{
			jsDev[which].js = SDL_JoystickOpen(which);

			if (jsDev[which].js == NULL)
			{
				printf("Could not open joystick %d: %s.\n",
					   which, SDL_GetError());
			}
			else
			{
				//printf("Added Joystick: %i \n", which );
				jsDev[which].init(which);
				//jsDev[which].print();
				//printJoystick( s_Joysticks[which] );
			}
		}
	}
	return 0;
}

//********************************************************************************
int RemoveJoystick(int which)
{
	//printf("Remove Joystick: %i \n", which );

	for (int i = 0; i < MAX_JOYSTICKS; i++)
	{
		if (jsDev[i].isConnected())
		{
			if (SDL_JoystickInstanceID(jsDev[i].getJS()) == which)
			{
				printf("Remove Joystick: %i \n", which);
				jsDev[i].close();
				return 0;
			}
		}
	}
	return -1;
}

//********************************************************************************
int FindJoystickByInstanceID( int which )
{
	for (int i = 0; i < MAX_JOYSTICKS; i++)
	{
		if (jsDev[i].isConnected())
		{
			if (SDL_JoystickInstanceID(jsDev[i].getJS()) == which)
			{
				return i;
			}
		}
	}
	return -1;
}
//********************************************************************************
/**
 * Initialize the SDL joystick subsystem.
 */
int InitJoysticks(void)
{
	int n; /* joystick index */
	int total;

	if (s_jinited)
	{
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
