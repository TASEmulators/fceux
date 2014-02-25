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

#ifndef COMMON_INPUT_H
#define COMMON_INPUT_H

#ifdef WIN32
#include <Windows.h>
typedef GUID JOYINSTANCEID;
#else
typedef const char* JOYINSTANCEID; // TODO SDL joystick name
#endif

#include "types.h"


#define MAXBUTTCONFIG 4

/* BtnConfig
	Mapping of hardware buttons/keys/axes to single virtual 'button'.
	Can contain up to MAXBUTTCONFIG hardware keys associated with button.
*/
typedef struct btncfg_ {
public:
	typedef enum {
		BT_UNSET = -1,
		BT_KEYBOARD = 0,
		BT_JOYSTICK
	} ButtonType;

public:
    ButtonType ButtType[MAXBUTTCONFIG]; // device type
    uint8 DeviceNum[MAXBUTTCONFIG]; // device index
    uint16 ButtonNum[MAXBUTTCONFIG]; // button information
    uint32 NumC; // number of hardware keys defined in this config
    JOYINSTANCEID DeviceInstance[MAXBUTTCONFIG]; // platform-specific device identifier
		// TODO must lookup DeviceNum internally based on JOYINSTANCEID; don't use DeviceNum
		// directly as it might and will change from session to session

public:
	// ButtonNum masks
	static const uint32 ISAXIS_MASK = 0x8000;
	static const uint32 ISAXISNEG_MASK = 0x4000;
	static const uint32 AXISIDX_MASK = 0x3;
	static const uint32 ISPOVBTN_MASK = 0x2000;
	static const uint32 POVDIR_MASK = 0x3;
	static const uint32 POVCONTROLLER_MASK = 0x10;
	static const uint32 JOYBTN_MASK = 0x7f;

public:
	bool IsAxisButton(uint8 which) const;
	bool IsAxisNegative(uint8 which) const;
	uint8 GetAxisIdx(uint8 which) const;
	
	bool IsPovButton(uint8 which) const;
	uint8 GetPovDir(uint8 which) const;
	uint8 GetPovController(uint8 which) const;
	
	uint8 GetJoyButton(uint8 which) const;

	uint16 GetScanCode(uint8 which) const;

public:
	void AssignScanCode(uint8 which, uint16 scode);
	void AssignJoyButton(uint8 which, uint16 btnnum);
} BtnConfig;

#endif // COMMON_INPUT_H
