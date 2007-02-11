#ifndef WIN_INPUT_H
#define WIN_INPU_H

#include "dinput.h"

void ConfigInput(HWND hParent);
int InitDInput(void);
void CreateInputStuff(void);
void InitInputStuff(void);
void DestroyInput(void);
void InputScreenChanged(int fs);
void SetAutoFireDesynch(int DesynchOn);
int GetAutoFireDesynch();

extern LPDIRECTINPUT7 lpDI;

extern int InputType[3];
extern int UsrInputType[3];
extern int cidisabled;
#ifndef _aosdfjk02fmasf
#define _aosdfjk02fmasf

#include "../common/args.h"
#include "../common/config.h"
#include "../../input.h"

#define MAXBUTTCONFIG   4
typedef struct {
        uint8  ButtType[MAXBUTTCONFIG];
        uint8  DeviceNum[MAXBUTTCONFIG];
        uint16 ButtonNum[MAXBUTTCONFIG];
        uint32 NumC;
        GUID DeviceInstance[MAXBUTTCONFIG];
        //uint64 DeviceID[MAXBUTTCONFIG];        /* TODO */
} ButtConfig;

extern CFGSTRUCT InputConfig[];
extern ARGPSTRUCT InputArgs[];
void ParseGIInput(FCEUGI *GameInfo);

#define BUTTC_KEYBOARD          0x00
#define BUTTC_JOYSTICK          0x01
#define BUTTC_MOUSE             0x02

#define FCFGD_GAMEPAD   1
#define FCFGD_POWERPAD  2
#define FCFGD_HYPERSHOT 3
#define FCFGD_QUIZKING  4

void InitOtherInput(void);
void SetEmulationSpeed(int type);
int FCEUD_TestCommandState(int c);
void FCEUD_UpdateInput();

extern const char* ScanNames[];
extern CFGSTRUCT HotkeyConfig[];
#endif

#endif