#ifndef _aosdfjk02fmasf
#define _aosdfjk02fmasf

#include <stdint.h>

#include "common/configSys.h"

//#define MAXBUTTCONFIG   4

enum {
  BUTTC_KEYBOARD  = 0,
  BUTTC_JOYSTICK  = 1,
  BUTTC_MOUSE     = 2
};
struct ButtConfig
{
	int    ButtType; 
	int    DeviceNum; 
	int    ButtonNum; 
   int    state;
};

extern int NoWaiting;
extern CFGSTRUCT InputConfig[];
extern ARGPSTRUCT InputArgs[];
void ParseGIInput(FCEUGI *GI);
void setHotKeys(void);
int getKeyState( int k );
int ButtonConfigBegin();
void ButtonConfigEnd();
void ConfigButton(char *text, ButtConfig *bc);
int DWaitButton(const uint8_t *text, ButtConfig *bc, int *buttonConfigStatus = NULL);

struct hotkey_t
{
	int value;
	int modifier;
	char prevState;

	hotkey_t(void);

	int getState(void);

	int getRisingEdge(void);

	int getString( char *s );

	void setModifierFromString( const char *s );
};
extern struct hotkey_t Hotkeys[];

#define FCFGD_GAMEPAD   1
#define FCFGD_POWERPAD  2
#define FCFGD_HYPERSHOT 3
#define FCFGD_QUIZKING  4

#define SDL_FCEU_HOTKEY_EVENT	SDL_USEREVENT

void InitInputInterface(void);
void InputUserActiveFix(void);

extern bool replaceP2StartWithMicrophone;
//extern ButtConfig GamePadConfig[4][10];
//extern ButtConfig powerpadsc[2][12];
//extern ButtConfig QuizKingButtons[6];
//extern ButtConfig FTrainerButtons[12];

void IncreaseEmulationSpeed(void);
void DecreaseEmulationSpeed(void);
int CustomEmulationSpeed(int spdPercent);

int DTestButtonJoy(ButtConfig *bc);

void FCEUD_UpdateInput(void);

void UpdateInput(Config *config);

std::string GetUserText(const char* title);
const char* ButtonName(const ButtConfig* bc);

int getInputSelection( int port, int *cur, int *usr );
int saveInputSettingsToFile( const char *fileBase = NULL );
int loadInputSettingsFromFile( const char *filename = NULL );

#endif

