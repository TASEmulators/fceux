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
#include "window.h"

#include "input.h"
#include "directinput/directinput.h"
#include "joystick.h"
#include "utils/bitflags.h"


#define FPOV_CENTER 16
#define AXIS_DEADZONE (0x10000)


typedef struct {
	bool x;
	bool y;
} AXES_;
static vector<AXES_> axisHelperFlag;
	// some temporary flags to handle axes correctly during input config

static int backgroundAccessBits = 0;


int driver::input::joystick::Init()
{
	directinput::joystick::InitDevices();
	return 1;
}

int driver::input::joystick::Kill()
{
	directinput::joystick::ReleaseDevices();
	return 1;
}

void driver::input::joystick::Update()
{
	int count = directinput::joystick::GetDeviceCount();
	for(int i=0; i<count; ++i) {
		directinput::joystick::DEVICE* joy = directinput::joystick::GetDevice(i);
		if(joy != NULL) joy->isUpdated = false;
	}
}

// Convert system pov value [0, 36000) to internal range [0, 4)
static int povToRange(long pov) {
	static const int QUARTER = 9000;
	static const int EIGHT = QUARTER/2;

	if(LOWORD(pov) == 0xFFFF) return FPOV_CENTER;

	pov /= EIGHT;
	pov = (pov >> 1) + (pov & 1);
	pov &= 3;
	return pov;
}

// Test if specified system pov value hits desired internal pov range
// If diagonalPass is true, diagonal povs adjanced to targetpov will also pass
// (when pov hat is used as a d-pad, diagonals will trigger both adjanced directions)
static bool IsPovInRange(long syspov, int targetpov, bool diagonalsPass) {
	static const int QUARTER = 9000;
	static const int EIGHT = QUARTER/2;
	static const int SIXTEENTH = EIGHT/2;

	if(povToRange(syspov) == FPOV_CENTER) return false;

	if(povToRange(syspov) == targetpov) return true; // exact hit

	if(diagonalsPass && (
		povToRange(syspov + SIXTEENTH) == targetpov ||
		povToRange(syspov - SIXTEENTH) == targetpov)) return true;

	return false;
}

bool driver::input::joystick::TestButton(const BtnConfig *btnConfig)
{
	bool isPressed = false;

	for(uint32 btnIdx=0; btnIdx<btnConfig->NumC; btnIdx++) {
		directinput::joystick::DEVICE* device = directinput::joystick::GetDevice(btnConfig->DeviceInstance[btnIdx]);
		if(device == NULL) continue;

		if(btnConfig->ButtType[btnIdx] != BtnConfig::BT_JOYSTICK) continue;

		if(!device->isUpdated) {
			if(!directinput::GetState(device->handle, sizeof(DIJOYSTATE2), &device->state)) {
				continue;
			}
			device->isUpdated = true;
		}

		if(btnConfig->IsAxisButton(btnIdx)) {
			// Axis
			int axisIdx = btnConfig->GetAxisIdx(btnIdx);
			LONG rangeBase = device->ranges.base[axisIdx];
			LONG rangeRange = device->ranges.range[axisIdx];
			LONG srcval = (axisIdx == 0)? device->state.lX:device->state.lY;
			long val = ((int64)srcval - rangeBase) * 0x40000 / rangeRange - 0x20000;
			/* Now, val is of the range -131072 to 131071.  Good enough. */

			if((btnConfig->IsAxisNegative(btnIdx) && val <= -AXIS_DEADZONE) ||
				(!btnConfig->IsAxisNegative(btnIdx) && val >= AXIS_DEADZONE))
			{
				isPressed = true;
				break;
			}
		}
		else if(btnConfig->IsPovButton(btnIdx)) {
			// POV Hat
			int srcpov = device->state.rgdwPOV[btnConfig->GetPovController(btnIdx)];
			int targetDir = btnConfig->GetPovDir(btnIdx);

			if(IsPovInRange(srcpov, targetDir, true)) {
				isPressed = true;
				break;
			}
		}
		else {
			// Button
			if(device->state.rgbButtons[btnConfig->GetJoyButton(btnIdx)] & 0x80) {
				isPressed = true;
				break;
			}
		}
	}

	return isPressed;
}

/* Now the fun configuration test begins. */
void driver::input::joystick::BeginWaitButton(PLATFORM_DIALOG_ARGUMENT hwnd)
{
	unsigned int count = directinput::joystick::GetDeviceCount();
	axisHelperFlag.resize(count);
	for(unsigned int n=0; n<count; ++n) {
		axisHelperFlag[n].x = false;
		axisHelperFlag[n].y = false;

		directinput::SetCoopLevel(directinput::joystick::GetDevice(n)->handle, hwnd, false);
	}
}

int driver::input::joystick::GetButtonPressedTest(JOYINSTANCEID *guid, uint8 *devicenum, uint16 *buttonnum)
{
	unsigned int count = directinput::joystick::GetDeviceCount();
	for(unsigned int devIdx=0; devIdx<count; ++devIdx) {
		directinput::joystick::DEVICE* device = directinput::joystick::GetDevice(devIdx);

		DIJOYSTATE2 immediateState;
		if(!directinput::GetState(device->handle, sizeof(DIJOYSTATE2), &immediateState)) {
			return 0;
		}

		// check normal buttons
		for(int btnIdx = 0; btnIdx < 128; btnIdx++) {
			if((immediateState.rgbButtons[btnIdx] & 0x80) && !(device->state.rgbButtons[btnIdx] & 0x80)) {
				// button pressed in immediate state but not in last saved state
				*devicenum = devIdx;
				*buttonnum = btnIdx;
				*guid = device->guid;
				memcpy(device->state.rgbButtons, immediateState.rgbButtons, 128);

				return 1;
			}
		}
		memcpy(device->state.rgbButtons, immediateState.rgbButtons, 128);

		// check X, Y axii
		long rangeX = device->ranges.range[0];
		if(rangeX != 0) {
			long value = ((int64)immediateState.lX - device->ranges.base[0]) * 0x40000 / rangeX - 0x20000;

			if(axisHelperFlag[devIdx].x && abs(value) >= AXIS_DEADZONE) {
				// activity on X axis
				*guid = device->guid;
				*devicenum = devIdx;
				*buttonnum = BtnConfig::ISAXIS_MASK |
					(0) |
					((value < 0) ? (BtnConfig::ISAXISNEG_MASK):0);

				memcpy(&device->state, &immediateState, sizeof(DIJOYSTATE2));   
				axisHelperFlag[devIdx].x = false;

				return 1;
			}
			else if(abs(value) <= 0x8000) {
				axisHelperFlag[devIdx].x = true;
			}
		}

		long rangeY = device->ranges.range[1];
		if(rangeY != 0) {
			long value = ((int64)immediateState.lY - device->ranges.base[1]) * 0x40000 / rangeY - 0x20000;

			if(abs(value) >= AXIS_DEADZONE && axisHelperFlag[devIdx].y) {
				// activity on Y axis
				*guid = device->guid;
				*devicenum = devIdx;
				*buttonnum = BtnConfig::ISAXIS_MASK |
					(1) |
					((value < 0) ? (BtnConfig::ISAXISNEG_MASK):0);

				memcpy(&device->state, &immediateState, sizeof(DIJOYSTATE2));
				axisHelperFlag[devIdx].y = false;

				return 1;
			}
			else if(abs(value) <= 0x8000) {
				axisHelperFlag[devIdx].y = true;
			}
		}

		// check POV button
		for(int x=0; x<4; x++) {   
			if(povToRange(immediateState.rgdwPOV[x]) != FPOV_CENTER && povToRange(device->state.rgdwPOV[x]) == FPOV_CENTER) {
				*guid = device->guid;
				*devicenum = devIdx;
				*buttonnum = BtnConfig::ISPOVBTN_MASK | (x<<4) | povToRange(immediateState.rgdwPOV[x]);
				memcpy(&device->state, &immediateState, sizeof(DIJOYSTATE2));
				return 1;
			}
		}
		memcpy(&device->state, &immediateState, sizeof(DIJOYSTATE2));
	}

	return 0;
}

void driver::input::joystick::EndWaitButton(PLATFORM_DIALOG_ARGUMENT hwnd)
{
	unsigned int count = directinput::joystick::GetDeviceCount();
	for(unsigned int n=0; n<count; ++n)
	{
		directinput::SetCoopLevel(directinput::joystick::GetDevice(n)->handle, hwnd, false);
	}
}


static void UpdateBackgroundAccess(bool enable)
{
	static bool isEnabled = false;
	if(isEnabled != enable) {
		isEnabled = enable;

		unsigned int count = directinput::joystick::GetDeviceCount();
		for(unsigned int n=0; n<count; ++n) {
			directinput::joystick::DEVICE* device = directinput::joystick::GetDevice(n);
			directinput::SetCoopLevel(device->handle, GetMainHWND(), (backgroundAccessBits!=0));
			directinput::Acquire(device->handle);
		}
	}
}

void driver::input::joystick::SetBackgroundAccessBit(driver::input::joystick::BKGINPUT bit)
{
	FL_SET(backgroundAccessBits, 1<<bit);
	UpdateBackgroundAccess(backgroundAccessBits != 0);
}

void driver::input::joystick::ClearBackgroundAccessBit(driver::input::joystick::BKGINPUT bit)
{
	FL_CLEAR(backgroundAccessBits, 1<<bit);
	UpdateBackgroundAccess(backgroundAccessBits != 0);
}
