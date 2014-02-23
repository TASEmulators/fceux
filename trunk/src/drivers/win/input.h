/*
	NOTE: for purposes of this header and related source files, "gamepad" is
	an internal emulated NES/Famicom device, and actual hardware devices are
	referred to as "joysticks"
*/

#ifndef WIN_INPUT_H
#define WIN_INPUT_H

#include "drivers/common/input.h"
#include "drivers/common/args.h"
#include "drivers/common/config.h"
#include "../../input.h"


// Show input configuration dialog
void ConfigInput(void);

// Init/Deinit input devices
bool InitInputDriver(void);
void KillInputDriver(void);

// Enable/disable virtual keyboard handling
void SetInputKeyboard(bool on);
bool GetInputKeyboard(void);

// Get/set turbo A-B alternation
void SetAutoFireDesynch(bool DesynchOn);
bool GetAutoFireDesynch(void);

// U+D, L+R at the same time
bool GetAllowUDLR(void);
void SetAllowUDLR(bool allow);

// Get/set keys to add or remove autoholds
int GetAutoholdsAddKey(void);
void SetAutoholdsAddKey(int k);

int GetAutoholdsClearKey(void);
void SetAutoholdsClearKey(int k);

// Get autohold masks for four gamepads
// used for input visualization
uint8 const (& GetAutoHoldMask(void))[4];

// Get current key configurations for virtual input devices
BtnConfig* GetGamePadConfig(void);
BtnConfig* GetPowerPadConfig(void);
BtnConfig* GetFamiTrainerConfig(void);
BtnConfig* GetFamiKeyBoardConfig(void);
BtnConfig* GetSuborKeyBoardConfig(void);
BtnConfig* GetMahjongConfig(void);
BtnConfig* GetPartyTapConfig(void);

// Get 'physical' or 'mechanical' state of the gamepad
// Returned state represents state of buttons on the gamepad, rather than
// state of lines on input port (with no autohold/turbo/etc applied)
// Used for recording and input visualization
uint32 GetGamepadPressedPhysical(void);

// Update input
#define UPDATEINPUT_KEYBOARD (0x1)
#define UPDATEINPUT_JOYSTICKS (0x2)
#define UPDATEINPUT_COMMANDS (UPDATEINPUT_KEYBOARD|0x4) // NOTE also updates keyboard
#define UPDATEINPUT_EVERYTHING (UPDATEINPUT_KEYBOARD|UPDATEINPUT_JOYSTICKS|UPDATEINPUT_COMMANDS|0x8)
void FCEUD_UpdateInput(int flags);

// Get input ports (types of devices currently plugged in)
ESI GetPluggedIn(int port);
void SetPluggedIn(int port, ESI device);
ESIFC GetPluggedInEx(void);
void SetPluggedInEx(ESIFC device);


// Force set input configuration
// FIXME really only need one generalized version
void FCEUD_SetInput(bool fourscore, bool microphone, ESI port0, ESI port1, ESIFC fcexp);
void FCEUD_SetInput(ESI port0, ESI port1, ESIFC fcexp);

// FIXME bloature
// Use of unified and abstract configuration facility is strongly
// recommended for all config-related imports/exports.
// Moving presets mechanisms off of driver layer is recommended.
void CopyConfigToPreset(unsigned int presetIdx);
void PresetExport(int presetNum);
void PresetImport(int presetNum);
void FCEUI_UseInputPreset(int preset);


// see win/config.h
int (&_FIXME_GetInputPortsArr(void))[3];
int& _FIXME_GetAddAutoholdsKeyVar(void);
int& _FIXME_GetClearAutoholdsKeyVar(void);
int& _FIXME_GetAllowUDLRVar(void);
int& _FIXME_GetDesynchAutoFireVar(void);
CFGSTRUCT* _FIXME_GetCommandsConfigVar(void);
CFGSTRUCT* _FIXME_GetInputConfigVar(void);

#endif // WIN_INPUT_H
