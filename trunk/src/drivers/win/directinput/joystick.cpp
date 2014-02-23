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

/*
	DirectInput joystick wrapper/helper
*/

#include <vector>
#include <map>

#include "drivers/win/directinput/joystick.h"
#include "drivers/win/window.h"
#include "drivers/win/directinput/directinput.h"


#define MAXJOYSTICKS 0xFF

/* Joystick GUID stored in BtnConfig is sufficient to look up the
device it is referring to, so we'll use GUID when working with
configs and leave indexes for the enumeration cases */
struct compareGUID_ {
    bool operator()(GUID a, GUID b) const {
        return (memcmp(&a, &b, sizeof(GUID)) < 0);
    }
};

typedef std::map<GUID, directinput::joystick::DEVIDX, compareGUID_> GUID2IDXMAP;
static GUID2IDXMAP guidToIdx;

static std::vector<directinput::joystick::DEVICE*> devices;


static void GetJoyRange(LPDIRECTINPUTDEVICE7 device, directinput::joystick::JOY_RANGE& ranges) {
	memset(&ranges, 0, sizeof(directinput::joystick::JOY_RANGE));
	
	DIPROPRANGE props;
	memset(&props, 0, sizeof(DIPROPRANGE));
	props.diph.dwSize = sizeof(DIPROPRANGE);
	props.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	props.diph.dwHow = DIPH_BYOFFSET;
	
	props.diph.dwObj = DIJOFS_X;
	HRESULT status = IDirectInputDevice7_GetProperty(device, DIPROP_RANGE, &props.diph);
	if(status == DI_OK || status == S_FALSE) {
		ranges.base[0] = props.lMin;
		ranges.range[0] = props.lMax - props.lMin;
	}

	props.diph.dwObj = DIJOFS_Y;
	status = IDirectInputDevice7_GetProperty(device, DIPROP_RANGE, &props.diph);
	if(status == DI_OK || status == S_FALSE) {
		ranges.base[1] = props.lMin;
		ranges.range[1] = props.lMax - props.lMin;
	}

	props.diph.dwObj = DIJOFS_Z;
	status = IDirectInputDevice7_GetProperty(device, DIPROP_RANGE, &props.diph);
	if(status == DI_OK || status == S_FALSE) {
		ranges.base[2] = props.lMin;
		ranges.range[2] = props.lMax - props.lMin;
	}
}

static BOOL CALLBACK OnJoystickFound_cb(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef) {
	if(devices.size() < MAXJOYSTICKS-1) {
		directinput::joystick::DEVICE* device = new directinput::joystick::DEVICE();
		if(device != NULL) {
			device->handle = directinput::CreateDevice(lpddi->guidInstance);
			if(device->handle != NULL) {
				if(directinput::SetCoopLevel(device->handle, *(HWND *)pvRef, false)) {
						// NOTE hardcoded foreground access mode; as enumeration is done once on startup applying actual mode here is meaningless
					GetJoyRange(device->handle, device->ranges);

					device->guid = lpddi->guidInstance;
					guidToIdx.insert( std::pair<GUID, directinput::joystick::DEVIDX>(lpddi->guidInstance, devices.size()) );

					directinput::Acquire(device->handle);

					device->isValid = true;
					devices.push_back(device);

					return DIENUM_CONTINUE;
				}

				directinput::DestroyDevice(device->handle);
				device->handle = NULL;
			}

			delete device;
		}
	}
	else {
		// no room left for more joysticks
		return DIENUM_STOP;
	}

	return DIENUM_CONTINUE;
}

void directinput::joystick::InitDevices() {
	HWND hwnd = GetMainHWND();
	LPDIRECTINPUT7 di = directinput::GetDirectInputHandle();
	IDirectInput7_EnumDevices(di, DIDEVTYPE_JOYSTICK, OnJoystickFound_cb, (LPVOID *)&hwnd, DIEDFL_ATTACHEDONLY);
}

void directinput::joystick::ReleaseDevices() {
	std::vector<directinput::joystick::DEVICE*>::reverse_iterator i;
	while(1) {
		i = devices.rbegin();
		if(i == devices.rend()) break;
		else {
			directinput::DestroyDevice((*i)->handle);
			delete (*i);
			devices.pop_back();
		}
	}
}

directinput::joystick::DEVIDX directinput::joystick::GetDeviceCount() {
	return devices.size();
}

directinput::joystick::DEVICE* directinput::joystick::GetDevice(directinput::joystick::DEVIDX idx) {
	if(idx < devices.size()) {
		return devices[idx];
	}

	return NULL;
}

directinput::joystick::DEVICE* directinput::joystick::GetDevice(GUID guid) {
	GUID2IDXMAP::iterator i = guidToIdx.find(guid);
	if(i == guidToIdx.end()) return NULL;

	return devices[i->second];
}
