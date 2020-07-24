// sdl-joystick.h

#ifndef __SDL_JOYSTICK_H__
#define __SDL_JOYSTICK_H__

#include <string>

#include "Qt/main.h"
#include "Qt/input.h"
#include "Qt/sdl.h"

#define  MAX_JOYSTICKS  32

struct nesGamePadMap_t
{
   char guid[64];
   char name[128];
   char btn[GAMEPAD_NUM_BUTTONS][32];
   char os[64];

   nesGamePadMap_t(void);
  ~nesGamePadMap_t(void);

   void clearMapping(void);
   int parseMapping( const char *text );
};

struct jsDev_t
{
   SDL_Joystick *js;
	SDL_GameController *gc;

	jsDev_t(void);
	//~jsDev_t(void);

	void init( int idx );
	int close(void);
	SDL_Joystick *getJS(void);
	bool isGameController(void);
	bool isConnected(void);
	void print(void);
	const char *getName(void);
	const char *getGUID(void);

	private:
	int          devIdx;
	std::string  guidStr;
	std::string  name;
};

class GamePad_t
{
	public:

	//int  type;
	int  devIdx;

	ButtConfig bmap[GAMEPAD_NUM_BUTTONS];

	GamePad_t(void);
	~GamePad_t(void);

	const char *getGUID(void);

	int  loadDefaults(void);
   int  loadProfile( const char *name, const char *guid = NULL );

	int  setDeviceIndex( int devIdx );
	int  setMapping( const char *map );
	int  setMapping( nesGamePadMap_t *map );

   int  createProfile( const char *name );
   int  getMapFromFile( const char *filename, char *out );
   int  getDefaultMap( char *out, const char *guid = NULL );
   int  saveMappingToFile( const char *filename, const char *txtMap );
   int  saveCurrentMapToFile( const char *filename );
   int  deleteMapping( const char *name );
};

extern GamePad_t GamePad[4];

jsDev_t *getJoystickDevice( int devNum );

#endif
