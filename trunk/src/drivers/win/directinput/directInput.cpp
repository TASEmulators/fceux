#include "directinput.h"


static LPDIRECTINPUT7 directInputHandle = NULL;


bool directinput::Create(HINSTANCE appInst) {
	HRESULT status = DirectInputCreateEx(appInst,
		DIRECTINPUT_VERSION,
		IID_IDirectInput7,
		(LPVOID *)&directInputHandle,
		NULL);

	if(FAILED(status)) {
		directInputHandle = NULL;
		return false;
	}

	return true;
}

bool directinput::IsCreated() {
	return (directInputHandle != NULL);
}

void directinput::Destroy() {
	if(directInputHandle) {
		IDirectInput7_Release(directInputHandle);
		directInputHandle = NULL;
	}
}

LPDIRECTINPUT7 directinput::GetDirectInputHandle() {
	return directInputHandle;
}

LPDIRECTINPUTDEVICE7 directinput::CreateDevice(REFGUID guid) {
	if(directInputHandle != NULL) {
		LPDIRECTINPUTDEVICE7 device;
		// create device
		if(DI_OK == IDirectInput7_CreateDeviceEx(directInputHandle, guid, IID_IDirectInputDevice7, (LPVOID *)&device, 0)) {
			// assign suitable data format
			LPCDIDATAFORMAT fmt = (guid == GUID_SysKeyboard)? &c_dfDIKeyboard:&c_dfDIJoystick2;
			if(DI_OK == IDirectInputDevice7_SetDataFormat(device, fmt)) {
				return device;
			}
			IDirectInputDevice7_Release(device);
		}
	}

	return NULL;
}

void directinput::DestroyDevice(LPDIRECTINPUTDEVICE7 device) {
	if(device != NULL) {
		IDirectInputDevice7_Unacquire(device);
		IDirectInputDevice7_Release(device);
	}
}

bool directinput::Acquire(LPDIRECTINPUTDEVICE7 device) {
	HRESULT status = DIERR_INPUTLOST;
	while(status == DIERR_INPUTLOST || status == DIERR_NOTACQUIRED) {
		status = IDirectInputDevice7_Acquire(device);
	}
	// at this point whatever the status is we can't do anything else
	// to acquire device

	return SUCCEEDED(status);
}

bool directinput::SetCoopLevel(LPDIRECTINPUTDEVICE7 device, HWND window, bool allowBackgroundAccess) {
	IDirectInputDevice7_Unacquire(device);
	DWORD flags = DISCL_NONEXCLUSIVE | ((allowBackgroundAccess)? DISCL_BACKGROUND:DISCL_FOREGROUND);
	return (DI_OK == IDirectInputDevice7_SetCooperativeLevel(device, window, flags));
}

bool directinput::GetState(LPDIRECTINPUTDEVICE7 device, DWORD size, LPVOID data) {
	HRESULT status = IDirectInputDevice7_Poll(device);
	if(status != DI_OK && status != DI_NOEFFECT) {
		Acquire(device);
		status = IDirectInputDevice7_Poll(device);
	}

	status = IDirectInputDevice7_GetDeviceState(device, size, data);
	if(status != DI_OK) {
		memset(data, 0, size);
		return false;
	}

	return true;
}
