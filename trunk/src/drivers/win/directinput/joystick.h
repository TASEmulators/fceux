#ifndef DIRECTINPUTJOYSTICK_H
#define DIRECTINPUTJOYSTICK_H

#include "drivers/win/common.h"
#include "dinput.h"


namespace directinput {
namespace joystick {
	typedef struct {
		LONG base[3]; // base values for three axes
		LONG range[3]; // ranges for three axes
	} JOY_RANGE;

	typedef struct joystick_ {
		LPDIRECTINPUTDEVICE7 handle;
		GUID guid;
		DIJOYSTATE2 state;
		bool isUpdated;
		bool isValid;
		JOY_RANGE ranges;

		joystick_()
			: handle(NULL)
			, isUpdated(false)
			, isValid(false)
		{}
	} DEVICE;

	typedef uint8 DEVIDX;


	// Init/release actual devices
	void InitDevices(void);
	void ReleaseDevices(void);

		
	// Returns joystick with specified GUID
	// or NULL if joystick cannot be found
	DEVICE* GetDevice(GUID guid);
		
	// Returns number of joysticks available
	DEVIDX GetDeviceCount(void);
		
	// Returns joystick at specified index
	// or NULL if index is out of valid range
	DEVICE* GetDevice(DEVIDX idx);
};
};

#endif DIRECTINPUTJOYSTICK_H
