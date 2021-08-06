#ifndef _aosdfjk02fmasf
#define _aosdfjk02fmasf

#include <stdint.h>
#include <list>

#include <QWidget>
#include <QKeySequence>
#include <QShortcut>
#include <QAction>

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

class hotkey_t
{
	public:
		// Methods
		hotkey_t(void);

		int init( QWidget *parent );

		int readConfig(void);

		int getState(void);

		int getRisingEdge(void);

		int getString( char *s );

		void setModifierFromString( const char *s );

		void setConfigName(const char *cName);
		void setAction( QAction *act );

		const char *getConfigName(void);
		QShortcut *getShortcut(void);

		QKeySequence getKeySeq(void){ return keySeq; };

		// Member variables
		struct 
		{
			int value;
			int modifier;
		} sdl;

		struct
		{
			int value;
			int modifier;
		} qkey;

		char prevState;

	private:
		void conv2SDL(void);

		const char *configName;
		QKeySequence keySeq;
		QShortcut *shortcut;
		QAction   *act;
		QString    actText;
};
extern class hotkey_t Hotkeys[];

struct gamepad_function_key_t
{
	struct {
		int  key;
		unsigned int modifier;
		std::string  name;

	} keySeq[2];

	int  hk[2];
	char  keyRelReq[2];

	struct ButtConfig  bmap[2];

	gamepad_function_key_t(void);
	~gamepad_function_key_t(void);

	void sendKeyPressEvent(int idx);
	void sendKeyReleaseEvent(int idx);

	void updateStatus(void);
};

//extern std::list <gamepad_function_key_t*> gpKeySeqList;

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

const char* ButtonName(const ButtConfig* bc);

int getInputSelection( int port, int *cur, int *usr );
int saveInputSettingsToFile( const char *fileBase = NULL );
int loadInputSettingsFromFile( const char *filename = NULL );
void toggleFamilyKeyboardFunc(void);
bool isFamilyKeyboardActv(void);

#endif

