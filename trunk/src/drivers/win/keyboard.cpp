/* FCE Ultra - NES/Famicom Emulator
*
* Copyright notice for this file:
*  Copyright (C) 2002 Xodnizel
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

#include "common.h"
#include "dinput.h"

#include "main.h"
#include "window.h"
#include "throttle.h"
#include "input.h"
#include "directinput/directinput.h"
#include "keyboard.h"
#include "keyscan.h"
#include "utils/bitflags.h"


static int backgroundInputBits = 0;
static LPDIRECTINPUTDEVICE7 deviceHandle=0;

static driver::input::keyboard::KeysState keyStates = {0,}; // last registered key states
	// zero for released keys, non-zero for pressed key
static driver::input::keyboard::KeysState keyStatesDownEvent = {0,}; // non-zero only for keys that were pressed down on last state update
static driver::input::keyboard::KeysState keys_jd_lock = {0,}; // set after key was pressed and held down through at least one state update
static driver::input::keyboard::KeysState validKeyMask = {0,}; // contains 0 for key array elements that aren't mapped to anything

#ifdef KEYBOARDTRANSFORMER_SPECIFIC
#define KEY_REPEAT_INITIAL_DELAY ((!isSoundEnabled || fps_scale < 64) ? (16) : (64)) // must be >= 0 and <= 255
#define KEY_REPEAT_REPEATING_DELAY (6) // must be >= 1 and <= 255

static unsigned int keyStatesAutorepeated[driver::input::keyboard::KEYCOUNT] = {0,}; // key states with autorepeat applied
#endif // KEYBOARDTRANSFORMER_SPECIFIC


int driver::input::keyboard::Init() {
	if(deviceHandle)
		return(1);

	deviceHandle = directinput::CreateDevice(GUID_SysKeyboard);
	if(deviceHandle == NULL)
	{
		FCEUD_PrintError("DirectInput: Error creating keyboard device.");
		return 0;
	}

	if(!directinput::SetCoopLevel(deviceHandle, GetMainHWND(), (backgroundInputBits!=0)))
	{
		FCEUD_PrintError("DirectInput: Error setting keyboard cooperative level.");
		return 0;
	}
	
	directinput::Acquire(deviceHandle);

	// any key with scancode not in this list will be ignored
	validKeyMask[DIK_ESCAPE] = 1;
	validKeyMask[DIK_1] = 1;
	validKeyMask[DIK_2] = 1;
	validKeyMask[DIK_3] = 1;
	validKeyMask[DIK_4] = 1;
	validKeyMask[DIK_5] = 1;
	validKeyMask[DIK_6] = 1;
	validKeyMask[DIK_7] = 1;
	validKeyMask[DIK_8] = 1;
	validKeyMask[DIK_9] = 1;
	validKeyMask[DIK_0] = 1;
	validKeyMask[DIK_MINUS] = 1;
	validKeyMask[DIK_EQUALS] = 1;
	validKeyMask[DIK_BACK] = 1;
	validKeyMask[DIK_TAB] = 1;
	validKeyMask[DIK_Q] = 1;
	validKeyMask[DIK_W] = 1;
	validKeyMask[DIK_E] = 1;
	validKeyMask[DIK_R] = 1;
	validKeyMask[DIK_T] = 1;
	validKeyMask[DIK_Y] = 1;
	validKeyMask[DIK_U] = 1;
	validKeyMask[DIK_I] = 1;
	validKeyMask[DIK_O] = 1;
	validKeyMask[DIK_P] = 1;
	validKeyMask[DIK_LBRACKET] = 1;
	validKeyMask[DIK_RBRACKET] = 1;
	validKeyMask[DIK_RETURN] = 1;
	validKeyMask[DIK_LCONTROL] = 1;
	validKeyMask[DIK_A] = 1;
	validKeyMask[DIK_S] = 1;
	validKeyMask[DIK_D] = 1;
	validKeyMask[DIK_F] = 1;
	validKeyMask[DIK_G] = 1;
	validKeyMask[DIK_H] = 1;
	validKeyMask[DIK_J] = 1;
	validKeyMask[DIK_K] = 1;
	validKeyMask[DIK_L] = 1;
	validKeyMask[DIK_SEMICOLON] = 1;
	validKeyMask[DIK_APOSTROPHE] = 1;
	validKeyMask[DIK_GRAVE] = 1;
	validKeyMask[DIK_LSHIFT] = 1;
	validKeyMask[DIK_BACKSLASH] = 1;
	validKeyMask[DIK_Z] = 1;
	validKeyMask[DIK_X] = 1;
	validKeyMask[DIK_C] = 1;
	validKeyMask[DIK_V] = 1;
	validKeyMask[DIK_B] = 1;
	validKeyMask[DIK_N] = 1;
	validKeyMask[DIK_M] = 1;
	validKeyMask[DIK_COMMA] = 1;
	validKeyMask[DIK_PERIOD] = 1;
	validKeyMask[DIK_SLASH] = 1;
	validKeyMask[DIK_RSHIFT] = 1;
	validKeyMask[DIK_MULTIPLY] = 1;
	validKeyMask[DIK_LMENU] = 1;
	validKeyMask[DIK_SPACE] = 1;
	validKeyMask[DIK_CAPITAL] = 1;
	validKeyMask[DIK_F1] = 1;
	validKeyMask[DIK_F2] = 1;
	validKeyMask[DIK_F3] = 1;
	validKeyMask[DIK_F4] = 1;
	validKeyMask[DIK_F5] = 1;
	validKeyMask[DIK_F6] = 1;
	validKeyMask[DIK_F7] = 1;
	validKeyMask[DIK_F8] = 1;
	validKeyMask[DIK_F9] = 1;
	validKeyMask[DIK_F10] = 1;
	validKeyMask[DIK_NUMLOCK] = 1;
	validKeyMask[DIK_SCROLL] = 1;
	validKeyMask[DIK_NUMPAD7] = 1;
	validKeyMask[DIK_NUMPAD8] = 1;
	validKeyMask[DIK_NUMPAD9] = 1;
	validKeyMask[DIK_SUBTRACT] = 1;
	validKeyMask[DIK_NUMPAD4] = 1;
	validKeyMask[DIK_NUMPAD5] = 1;
	validKeyMask[DIK_NUMPAD6] = 1;
	validKeyMask[DIK_ADD] = 1;
	validKeyMask[DIK_NUMPAD1] = 1;
	validKeyMask[DIK_NUMPAD2] = 1;
	validKeyMask[DIK_NUMPAD3] = 1;
	validKeyMask[DIK_NUMPAD0] = 1;
	validKeyMask[DIK_DECIMAL] = 1;
	validKeyMask[DIK_OEM_102] = 1;
	validKeyMask[DIK_F11] = 1;
	validKeyMask[DIK_F12] = 1;
	validKeyMask[DIK_F13] = 1;
	validKeyMask[DIK_F14] = 1;
	validKeyMask[DIK_F15] = 1;
	validKeyMask[DIK_KANA] = 1;
	validKeyMask[DIK_ABNT_C1] = 1;
	validKeyMask[DIK_CONVERT] = 1;
	validKeyMask[DIK_NOCONVERT] = 1;
	validKeyMask[DIK_YEN] = 1;
	validKeyMask[DIK_ABNT_C2] = 1;
	validKeyMask[DIK_NUMPADEQUALS] = 1;
	validKeyMask[DIK_PREVTRACK] = 1;
	validKeyMask[DIK_AT] = 1;
	validKeyMask[DIK_COLON] = 1;
	validKeyMask[DIK_UNDERLINE] = 1;
	validKeyMask[DIK_KANJI] = 1;
	validKeyMask[DIK_STOP] = 1;
	validKeyMask[DIK_AX] = 1;
	validKeyMask[DIK_UNLABELED] = 1;
	validKeyMask[DIK_NEXTTRACK] = 1;
	validKeyMask[DIK_NUMPADENTER] = 1;
	validKeyMask[DIK_RCONTROL] = 1;
	validKeyMask[DIK_MUTE] = 1;
	validKeyMask[DIK_CALCULATOR] = 1;
	validKeyMask[DIK_PLAYPAUSE] = 1;
	validKeyMask[DIK_MEDIASTOP] = 1;
	validKeyMask[DIK_VOLUMEDOWN] = 1;
	validKeyMask[DIK_VOLUMEUP] = 1;
	validKeyMask[DIK_WEBHOME] = 1;
	validKeyMask[DIK_NUMPADCOMMA] = 1;
	validKeyMask[DIK_DIVIDE] = 1;
	validKeyMask[DIK_SYSRQ] = 1;
	validKeyMask[DIK_RMENU] = 1;
	validKeyMask[DIK_PAUSE] = 1;
	validKeyMask[DIK_HOME] = 1;
	validKeyMask[DIK_UP] = 1;
	validKeyMask[DIK_PRIOR] = 1;
	validKeyMask[DIK_LEFT] = 1;
	validKeyMask[DIK_RIGHT] = 1;
	validKeyMask[DIK_END] = 1;
	validKeyMask[DIK_DOWN] = 1;
	validKeyMask[DIK_NEXT] = 1;
	validKeyMask[DIK_INSERT] = 1;
	validKeyMask[DIK_DELETE] = 1;
	validKeyMask[DIK_LWIN] = 1;
	validKeyMask[DIK_RWIN] = 1;
	validKeyMask[DIK_APPS] = 1;
	validKeyMask[DIK_POWER] = 1;
	validKeyMask[DIK_SLEEP] = 1;
	validKeyMask[DIK_WAKE] = 1;
	validKeyMask[DIK_WEBSEARCH] = 1;
	validKeyMask[DIK_WEBFAVORITES] = 1;
	validKeyMask[DIK_WEBREFRESH] = 1;
	validKeyMask[DIK_WEBSTOP] = 1;
	validKeyMask[DIK_WEBFORWARD] = 1;
	validKeyMask[DIK_WEBBACK] = 1;
	validKeyMask[DIK_MYCOMPUTER] = 1;
	validKeyMask[DIK_MAIL] = 1;
	validKeyMask[DIK_MEDIASELECT] = 1;

	return 1;
}

void driver::input::keyboard::Kill()
{
	if(deviceHandle) directinput::DestroyDevice(deviceHandle);
	deviceHandle = 0;
}

void driver::input::keyboard::Update()
{
	unsigned char deviceState[256];
	bool gotState = directinput::GetState(deviceHandle, sizeof(deviceState), deviceState);

	for(int k = 0 ; k < KEYCOUNT ; ++k) {
		if(k < sizeof(deviceState) && validKeyMask[k] && FL_TEST(deviceState[k], 0x80) != 0) {
			keyStates[k] = 1;
		}
		else keyStates[k] = 0; // deactivate key

		// key-down detection
		if(keyStates[k] == 0) {
			// key isn't pressed
			keyStatesDownEvent[k] = 0;
			keys_jd_lock[k] = 0;
		}
		else if(keys_jd_lock[k] != 0) {
			// key was pressed before and already got through its 'down' event
		}
		else {
			// key is entering 'down' state or still in it from before
			if(++keyStatesDownEvent[k] > 1) {
				keyStatesDownEvent[k] = 0;
				keys_jd_lock[k] = 1;
			}
		}
	}

#ifdef KEYBOARDTRANSFORMER_SPECIFIC
	// autorepeat keys
	for(int k = 0 ; k < KEYCOUNT ; ++k) {
		if(keyStatesDownEvent[k] != 0) {
			// key just went down, set up for repeat
			keyStatesAutorepeated[k] = KEY_REPEAT_INITIAL_DELAY;
		}
		else if(keys_jd_lock[k] != 0) {
			// key remains pressed
			if(keyStatesAutorepeated[k] > 0) {
				--keyStatesAutorepeated[k]; // advance counter
			}
			else {
				keyStatesAutorepeated[k] = KEY_REPEAT_REPEATING_DELAY; // restart counter
			}
		}
		else keyStatesAutorepeated[k] = 0;
	}
#endif // KEYBOARDTRANSFORMER_SPECIFIC
}


bool driver::input::keyboard::TestButton(const BtnConfig* bc) {
	for(uint32 x=0; x<bc->NumC; ++x) {
		if(bc->ButtType[x] == BtnConfig::BT_KEYBOARD) {
			if(keyStates[bc->GetScanCode(x)]) {
				return true;
			}
		}
	}

	return false;
}

driver::input::keyboard::KeysState_const driver::input::keyboard::GetState() {
	return keyStates;
}

driver::input::keyboard::KeysState_const driver::input::keyboard::GetStateJustPressed() {
	return keyStatesDownEvent;
}

#ifdef KEYBOARDTRANSFORMER_SPECIFIC
unsigned int *GetKeyboardAutorepeated() {
	return keyStatesAutorepeated;
}
#endif // KEYBOARDTRANSFORMER_SPECIFIC


static void UpdateBackgroundAccess(bool enable) {
	if(!deviceHandle)
		return;

	static bool bkgModeEnabled = false;
	if(bkgModeEnabled != enable) {
		bkgModeEnabled = enable;

		if(!directinput::SetCoopLevel(deviceHandle, GetMainHWND(), bkgModeEnabled))
		{
			FCEUD_PrintError("DirectInput: Error setting keyboard cooperative level.");
			return;
		}

		directinput::Acquire(deviceHandle);
	}
}

void driver::input::keyboard::SetBackgroundAccessBit(int bit)
{
	FL_SET(backgroundInputBits, 1<<bit);
	UpdateBackgroundAccess(backgroundInputBits != 0);
}

void driver::input::keyboard::ClearBackgroundAccessBit(int bit)
{
	FL_CLEAR(backgroundInputBits, 1<<bit);
	UpdateBackgroundAccess(backgroundInputBits != 0);
}
