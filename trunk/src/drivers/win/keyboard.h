#include "input.h"


namespace driver {
namespace input {
namespace keyboard {
	static const int KEYCOUNT = 256;

	typedef enum {
		BKGINPUT_GENERAL = 1,
		BKGINPUT_TASEDITOR
	} BKGINPUT;


	int Init(void);
	void Kill(void);

	void Update(void);


	// Returns true if any of buttons listed in btnConfig is pressed
	// Make sure to Update() before testing buttons
	bool TestButton(const BtnConfig* bc);

	typedef uint8 KeysState[KEYCOUNT];
	typedef const uint8 (& KeysState_const)[KEYCOUNT];

	// Returned array indicates currently pressed keys
	KeysState_const GetState(void);
	// Returned array indicates keys that were just pressed
	KeysState_const GetStateJustPressed(void);

	// Set/clear particular bit of the background kb input variable
	// Background input is enabled if any bit is set and disabled when
	// all bits are cleared
	void SetBackgroundAccessBit(int bit);
	void ClearBackgroundAccessBit(int bit);
};
};
};

#ifdef KEYBOARDTRANSFORMER_SPECIFIC
	// !!!Special feature, do not use for anything besides 'Keyboard
	// Transformer' board
	unsigned int *GetKeyboardAutorepeated(void);
#endif // KEYBOARDTRANSFORMER_SPECIFIC
