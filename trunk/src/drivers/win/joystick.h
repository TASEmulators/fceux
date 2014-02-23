#ifndef _JOYSTICK_H_
#define _JOYSTICK_H_

#include "common.h"
#include "dinput.h"
#include "input.h"
#include "directinput/joystick.h"


namespace driver {
namespace input {
namespace joystick {
	#ifdef WIN32
		typedef HWND PLATFORM_DIALOG_ARGUMENT;
	#else
		// TODO
	#endif

	typedef enum {
		BKGINPUT_GENERAL = 1,
		BKGINPUT_TASEDITOR
	} BKGINPUT;


	int Init(void);
	int Kill(void);


	// Called during normal emulator operation, not during button configuration.
	void Update(void);

	// Returns true if any of buttons listed in btnConfig is pressed
	// Make sure to Update() before testing buttons
	bool TestButton(const BtnConfig *bc);

	void BeginWaitButton(PLATFORM_DIALOG_ARGUMENT arg);
	// Updates input, returns 0 if no buttons pressed, otherwise fills
	// arguments with button parameters and returns 1
	int GetButtonPressedTest(JOYINSTANCEID *guid, uint8 *devicenum, uint16 *buttonnum);
	void EndWaitButton(PLATFORM_DIALOG_ARGUMENT arg);


	// Change background access mode for joysticks
	void SetBackgroundAccessBit(BKGINPUT bit);
	void ClearBackgroundAccessBit(BKGINPUT bit);
};
};
};

#endif // _JOYSTICK_H_
