#ifndef DIRECTINPUT_H
#define DIRECTINPUT_H

#include "drivers/win/common.h"
#include "dinput.h"


namespace directinput {
	bool Create(HINSTANCE appInst);
	bool IsCreated(void);
	void Destroy(void);
	LPDIRECTINPUT7 GetDirectInputHandle(void);

	LPDIRECTINPUTDEVICE7 CreateDevice(REFGUID guid);
	void DestroyDevice(LPDIRECTINPUTDEVICE7 device);

	bool Acquire(LPDIRECTINPUTDEVICE7 device);

	// Set cooperative level on a device instance
	// device must be acquired after call to this
	bool SetCoopLevel(LPDIRECTINPUTDEVICE7 device, HWND window, bool allowBackgroundAccess);

	// Will acquire device if necessary, poll it and get state
	// if getting state fails wipes data to 0 and returns false
	bool GetState(LPDIRECTINPUTDEVICE7 device, DWORD size, LPVOID data);
};

#endif // DIRECTINPUT_H
