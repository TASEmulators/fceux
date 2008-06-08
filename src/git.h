#ifndef __FCEU_GIT
#define __FCEU_GIT

enum EGIT
{
	GIT_CART	= 0,  //Cart
	GIT_VSUNI	= 1,  //VS Unisystem
	GIT_FDS		= 2,  // Famicom Disk System
	GIT_NSF		= 3,  //NES Sound Format
};

enum EGIV
{
	GIV_NTSC	= 0,  //NTSC emulation.
	GIV_PAL		= 1,  //PAL emulation.
	GIV_USER	= 2,  //What was set by FCEUI_SetVidSys().
};

enum ESIS
{
	SIS_NONE		= 0,
	SIS_DATACH		= 1,
	SIS_NWC			= 2,
	SIS_VSUNISYSTEM	= 3,
	SIS_NSF			= 4,
};

//input device types for the standard joystick port
enum ESI
{
	SI_UNSET		= -1,
	SI_NONE			= 0,
	SI_GAMEPAD		= 1,
	SI_ZAPPER		= 2,
	SI_POWERPADA	= 3,
	SI_POWERPADB	= 4,
	SI_ARKANOID		= 5,
	SI_MOUSE		= 6 //mbg merge 7/17/06 added
};

//input device types for the expansion port
enum ESIFC
{
	SIFC_UNSET		= -1,
	SIFC_NONE		= 0,
	SIFC_ARKANOID	= 1,
	SIFC_SHADOW		= 2,
	SIFC_4PLAYER	= 3,
	SIFC_FKB		= 4,
	SIFC_SUBORKB	= 5,
	SIFC_HYPERSHOT	= 6,
	SIFC_MAHJONG	= 7,
	SIFC_QUIZKING	= 8,
	SIFC_FTRAINERA	= 9,
	SIFC_FTRAINERB	= 10,
	SIFC_OEKAKIDS	= 11,
	SIFC_BWORLD		= 12,
	SIFC_TOPRIDER	= 13,
};

#include "utils/md5.h"

typedef struct {
	uint8 *name;	//Game name, UTF8 encoding

	EGIT type;      
	EGIV vidsys;     //Current emulated video system;
	ESI input[2];   //Desired input for emulated input ports 1 and 2; -1 for unknown desired input.
	ESIFC inputfc;  //Desired Famicom expansion port device. -1 for unknown desired input.
	ESIS cspecial;  //Special cart expansion: DIP switches, barcode reader, etc.
	
	MD5DATA MD5;

	//mbg 6/8/08 - ???
	int soundrate;  //For Ogg Vorbis expansion sound wacky support.  0 for default.
	int soundchan;  //Number of sound channels.
} FCEUGI;
#endif
