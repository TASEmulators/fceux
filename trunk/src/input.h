#ifndef _INPUT_H_
#define _INPUT_H_


#include "git.h"

#include <ostream>

// macro def used to isolate transformer board specific code
#define KEYBOARDTRANSFORMER_SPECIFIC


void LagCounterToggle(void);

class MovieRecord;

//MBG TODO - COMBINE THESE INPUTC AND INPUTCFC

//The interface for standard joystick port device drivers
struct INPUTC
{
	//these methods call the function pointers (or not, if they are null)
	uint8 Read(int w) { if(_Read) return _Read(w); else return 0; }
	void Write(uint8 w) { if(_Write) _Write(w); }
	void Strobe(int w) { if(_Strobe) _Strobe(w); }
	void Update(int w, void *data, int arg) { if(_Update) _Update(w,data,arg); }
	void SLHook(int w, uint8 *bg, uint8 *spr, uint32 linets, int final) { if(_SLHook) _SLHook(w,bg,spr,linets,final); }
	void Draw(int w, uint8 *buf, int arg) { if(_Draw) _Draw(w,buf,arg); }
	void Log(int w, MovieRecord* mr) { if(_Log) _Log(w,mr); }
	void Load(int w, MovieRecord* mr) { if(_Load) _Load(w,mr); }

	uint8 (*_Read)(int w);
	void (*_Write)(uint8 v);
	void (*_Strobe)(int w);
	//update will be called if input is coming from the user. refresh your logical state from user input devices
	void (*_Update)(int w, void *data, int arg);
	void (*_SLHook)(int w, uint8 *bg, uint8 *spr, uint32 linets, int final);
	void (*_Draw)(int w, uint8 *buf, int arg);

	//log is called when you need to put your logical state into a movie record for recording
	void (*_Log)(int w, MovieRecord* mr);
	//load will be called if input is coming from a movie. refresh your logical state from a movie record
	void (*_Load)(int w, MovieRecord* mr);
};

//The interface for the expansion port device drivers
struct INPUTCFC
{
	//these methods call the function pointers (or not, if they are null)
	uint8 Read(int w, uint8 ret) { if(_Read) return _Read(w,ret); else return ret; }
	void Write(uint8 v) { if(_Write) _Write(v); }
	void Strobe() { if(_Strobe) _Strobe(); }
	void Update(void *data, int arg) { if(_Update) _Update(data,arg); }
	void SLHook(uint8 *bg, uint8 *spr, uint32 linets, int final) { if(_SLHook) _SLHook(bg,spr,linets,final); }
	void Draw(uint8 *buf, int arg) { if(_Draw) _Draw(buf,arg); }
	void Log(MovieRecord* mr) { if(_Log) _Log(mr); }
	void Load(MovieRecord* mr) { if(_Load) _Load(mr); }

	uint8 (*_Read)(int w, uint8 ret);
	void (*_Write)(uint8 v);
	void (*_Strobe)();
	//update will be called if input is coming from the user. refresh your logical state from user input devices
	void (*_Update)(void *data, int arg);
	void (*_SLHook)(uint8 *bg, uint8 *spr, uint32 linets, int final);
	void (*_Draw)(uint8 *buf, int arg);

	//log is called when you need to put your logical state into a movie record for recording
	void (*_Log)(MovieRecord* mr);

	//load will be called if input is coming from a movie. refresh your logical state from a movie record
	void (*_Load)(MovieRecord* mr);
};

extern struct JOYPORT
{
	JOYPORT(int _w)
		: w(_w)
	{}

	int w;
	int attrib;
	ESI type;
	void* ptr;
	INPUTC* driver;

	void log(MovieRecord* mr) { driver->Log(w,mr); }
	void load(MovieRecord* mr) { driver->Load(w,mr); }
} joyports[2];

extern struct FCPORT
{
	int attrib;
	ESIFC type;
	void* ptr;
	INPUTCFC* driver;
} portFC;


void FCEU_DrawInput(uint8 *buf);
void FCEU_UpdateInput(void);
void InitializeInput(void);
void FCEU_UpdateBot(void);
extern void (*PStrobe[2])(void);

//called from PPU on scanline events.
extern void InputScanlineHook(uint8 *bg, uint8 *spr, uint32 linets, int final);

void FCEU_DoSimpleCommand(int cmd);

enum EMUCMD
{
	EMUCMD_NONE=-1,
	EMUCMD_POWER=0,
	EMUCMD_RESET,
	EMUCMD_PAUSE,
	EMUCMD_FRAME_ADVANCE,
	EMUCMD_SCREENSHOT,
	EMUCMD_HIDE_MENU_TOGGLE,
	EMUCMD_EXIT,

	EMUCMD_SPEED_SLOWEST,
	EMUCMD_SPEED_SLOWER,
	EMUCMD_SPEED_NORMAL,
	EMUCMD_SPEED_FASTER,
	EMUCMD_SPEED_FASTEST,
	EMUCMD_SPEED_TURBO,
	EMUCMD_SPEED_TURBO_TOGGLE,

	EMUCMD_SAVE_SLOT_0,
	EMUCMD_SAVE_SLOT_1,
	EMUCMD_SAVE_SLOT_2,
	EMUCMD_SAVE_SLOT_3,
	EMUCMD_SAVE_SLOT_4,
	EMUCMD_SAVE_SLOT_5,
	EMUCMD_SAVE_SLOT_6,
	EMUCMD_SAVE_SLOT_7,
	EMUCMD_SAVE_SLOT_8,
	EMUCMD_SAVE_SLOT_9,
	EMUCMD_SAVE_SLOT_NEXT,
	EMUCMD_SAVE_SLOT_PREV,
	EMUCMD_SAVE_STATE,
	EMUCMD_SAVE_STATE_AS,
	EMUCMD_SAVE_STATE_SLOT_0,
	EMUCMD_SAVE_STATE_SLOT_1,
	EMUCMD_SAVE_STATE_SLOT_2,
	EMUCMD_SAVE_STATE_SLOT_3,
	EMUCMD_SAVE_STATE_SLOT_4,
	EMUCMD_SAVE_STATE_SLOT_5,
	EMUCMD_SAVE_STATE_SLOT_6,
	EMUCMD_SAVE_STATE_SLOT_7,
	EMUCMD_SAVE_STATE_SLOT_8,
	EMUCMD_SAVE_STATE_SLOT_9,
	EMUCMD_LOAD_STATE,
	EMUCMD_LOAD_STATE_FROM,
	EMUCMD_LOAD_STATE_SLOT_0,
	EMUCMD_LOAD_STATE_SLOT_1,
	EMUCMD_LOAD_STATE_SLOT_2,
	EMUCMD_LOAD_STATE_SLOT_3,
	EMUCMD_LOAD_STATE_SLOT_4,
	EMUCMD_LOAD_STATE_SLOT_5,
	EMUCMD_LOAD_STATE_SLOT_6,
	EMUCMD_LOAD_STATE_SLOT_7,
	EMUCMD_LOAD_STATE_SLOT_8,
	EMUCMD_LOAD_STATE_SLOT_9,

	EMUCMD_MOVIE_RECORD_TO,
	EMUCMD_MOVIE_REPLAY_FROM,
	EMUCMD_MOVIE_PLAY_FROM_BEGINNING,
	EMUCMD_MOVIE_STOP,
	EMUCMD_MOVIE_READONLY_TOGGLE,
	EMUCMD_MOVIE_FRAME_DISPLAY_TOGGLE,
	EMUCMD_MOVIE_INPUT_DISPLAY_TOGGLE,
	EMUCMD_MOVIE_ICON_DISPLAY_TOGGLE,

#ifdef _S9XLUA_H
	EMUCMD_SCRIPT_RELOAD,
#endif
	EMUCMD_SOUND_TOGGLE,
	EMUCMD_SOUND_VOLUME_UP,
	EMUCMD_SOUND_VOLUME_DOWN,
	EMUCMD_SOUND_VOLUME_NORMAL,

	EMUCMD_AVI_RECORD_AS,
	EMUCMD_AVI_STOP,

	EMUCMD_FDS_EJECT_INSERT,
	EMUCMD_FDS_SIDE_SELECT,

	EMUCMD_VSUNI_COIN,
	EMUCMD_VSUNI_TOGGLE_DIP_0,
	EMUCMD_VSUNI_TOGGLE_DIP_1,
	EMUCMD_VSUNI_TOGGLE_DIP_2,
	EMUCMD_VSUNI_TOGGLE_DIP_3,
	EMUCMD_VSUNI_TOGGLE_DIP_4,
	EMUCMD_VSUNI_TOGGLE_DIP_5,
	EMUCMD_VSUNI_TOGGLE_DIP_6,
	EMUCMD_VSUNI_TOGGLE_DIP_7,
	EMUCMD_VSUNI_TOGGLE_DIP_8,
	EMUCMD_VSUNI_TOGGLE_DIP_9,
	EMUCMD_MISC_AUTOSAVE,
	EMUCMD_MISC_SHOWSTATES,
	EMUCMD_MISC_USE_INPUT_PRESET_1,
	EMUCMD_MISC_USE_INPUT_PRESET_2,
	EMUCMD_MISC_USE_INPUT_PRESET_3,
	EMUCMD_MISC_DISPLAY_BG_TOGGLE,
	EMUCMD_MISC_DISPLAY_OBJ_TOGGLE,
	EMUCMD_MISC_DISPLAY_LAGCOUNTER_TOGGLE,

	//Currently Windows only------
	EMUCMD_MISC_OPENTASEDITOR,
	EMUCMD_TOOL_OPENMEMORYWATCH,
	EMUCMD_TOOL_OPENCHEATS,
	EMUCMD_TOOL_OPENDEBUGGER,
	EMUCMD_TOOL_OPENHEX,
	EMUCMD_TOOL_OPENPPU,
	EMUCMD_TOOL_OPENTRACELOGGER,
	EMUCMD_TOOL_OPENCDLOGGER,
	//----------------------------
	EMUCMD_FRAMEADV_SKIPLAG,
	//Currently only windows (but sdl could easily add onto these)
	EMUCMD_OPENROM,
	EMUCMD_CLOSEROM,
	EMUCMD_RELOAD,
	//-----------------------------
	EMUCMD_MISC_DISPLAY_MOVIESUBTITLES,
	EMUCMD_MISC_UNDOREDOSAVESTATE,
	EMUCMD_MISC_TOGGLEFULLSCREEN,
	EMUCMD_TOOL_OPENRAMWATCH,
	EMUCMD_TOOL_OPENRAMSEARCH,
	EMUCMD_TOOL_RAMSEARCHLT,
	EMUCMD_TOOL_RAMSEARCHGT,
	EMUCMD_TOOL_RAMSEARCHLTE,
	EMUCMD_TOOL_RAMSEARCHGTE,
	EMUCMD_TOOL_RAMSEARCHEQ,
	EMUCMD_TOOL_RAMSEARCHNE,
	EMUCMD_TOOL_OPENNTVIEW,
	EMUCMD_TASEDITOR_REWIND,
	EMUCMD_RERECORD_DISPLAY_TOGGLE,
	EMUCMD_TASEDITOR_RESTORE_PLAYBACK,
	EMUCMD_TASEDITOR_CANCEL_SEEKING,
	EMUCMD_TASEDITOR_SWITCH_AUTORESTORING,
	EMUCMD_TASEDITOR_SWITCH_MULTITRACKING,
	EMUCMD_TASEDITOR_RUN_MANUAL_LUA,
	//-----------------------------
	//keep adding these in order of newness or else the hotkey binding configs will get messed up...
	EMUCMD_FPS_DISPLAY_TOGGLE,
	EMUCMD_NUM,
};

enum EMUCMDTYPE
{
	EMUCMDTYPE_MISC = 0,
	EMUCMDTYPE_SPEED,
	EMUCMDTYPE_STATE,
	EMUCMDTYPE_MOVIE,
	EMUCMDTYPE_SOUND,
	EMUCMDTYPE_AVI,
	EMUCMDTYPE_FDS,
	EMUCMDTYPE_VSUNI,
	EMUCMDTYPE_TOOL,  //All Tools type are currenty windows only programs
	EMUCMDTYPE_TASEDITOR,	// Windows-only as well

	EMUCMDTYPE_MAX
};

extern const char* FCEUI_CommandTypeNames[];

typedef void EMUCMDFN(void);

enum EMUCMDFLAG
{
	EMUCMDFLAG_NONE = 0,
	EMUCMDFLAG_TASEDITOR = 1,
};

struct EMUCMDTABLE
{
	EMUCMD cmd;
	EMUCMDTYPE type;
	EMUCMDFN* fn_on;
	EMUCMDFN* fn_off;
	int state;
	char* name;
	int flags; //EMUCMDFLAG
};

extern struct EMUCMDTABLE FCEUI_CommandTable[];

// emulator command key combo (key scan + optional modifiers)
typedef struct keyCombo_ {
public:
	keyCombo_(uint32 combo);

public:
	uint32 get(void) const;
	void assign(uint32 combo);

	uint32 getKey(void) const; // get key scan code
	void setKey(int keyCode);

	uint32 getMeta() const; // get combo WITHOUT key scan code
	void setMeta(int meta);

	uint32 getModifiers(void) const; // get modifier flags
	void setModifiers(uint32 mask);
	void clearModifiers(uint32 mask);

	bool isEmpty(void) const;

public:
	static const uint32 KEY_MASK = 0xff;
	static const uint32 META_MASK = (~KEY_MASK);
	static const uint32 MOD_MASK = 0xffff0000;
	static const uint32 LSHIFT_BIT = (1<<16);
	static const uint32 RSHIFT_BIT = (1<<17);
	static const uint32 SHIFT_BIT = LSHIFT_BIT|RSHIFT_BIT;
	static const uint32 LCTRL_BIT = (1<<18);
	static const uint32 RCTRL_BIT = (1<<19);
	static const uint32 CTRL_BIT = LCTRL_BIT|RCTRL_BIT;
	static const uint32 LALT_BIT = (1<<20);
	static const uint32 RALT_BIT = (1<<21);
	static const uint32 ALT_BIT = LALT_BIT|RALT_BIT;

private:
	uint32 mCombo;
} KeyCombo;

KeyCombo GetCommandKeyCombo(EMUCMD command);
void SetCommandKeyCombo(EMUCMD command, KeyCombo combo);

///A callback that the emu core uses to poll the state of a given emulator command key
typedef int TestCommandState(EMUCMD cmd);
///Assign callback function to test command keycombo state
void FCEUI_SetTestCommandHotkeyCallback(TestCommandState* cbf);
///Signals the emu core to poll for emulator commands and take actions
void FCEUI_HandleEmuCommands(void);

extern unsigned int lagCounter;
extern bool lagCounterDisplay;
extern char lagFlag;
extern bool turbo;
void LagCounterReset();


bool& _FIXME_GetReplaceP2StartWithMicrophoneVar(void);
int* _FIXME_GetCommandMappingVar(void);
int _FIXME_GetCommandMappingVarSize(void);

#endif //_INPUT_H_
