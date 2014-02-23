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

#include <string>
#include <sstream>
#include <iomanip>

#ifdef WIN32
#include "directinput/directinput.h"
#include "drivers/win/inputConfigUI.h"
#else
TODO
#endif

#include "common.h"
#include "input.h"
#include "keyboard.h"
#include "joystick.h"
#include "fceu.h"
#include "movie.h"
#include "window.h"
#include "keyscan.h"
#include "../../input/gamepad.h"
#include "../../input/fkb.h"
#include "../../input/suborkb.h"
#include "../../input/powerpad.h"
#include "../../input/hypershot.h"
#include "../../input/mahjong.h"
#include "../../input/quiz.h" // aka partytap
#include "../../input/toprider.h"
#include "../../input/ftrainer.h"
#include "utils/bitflags.h"


static uint32 GamepadData = 0; // button state flags, byte per gamepad
static uint32 GamepadPrevRawData = 0; // prevous RAW gamepad state, for internal use
static uint32 PowerPadData[2]; // powerpad state
static uint8 fkbkeys[NUMKEYS_FAMIKB]; // famicom keyboard state
static uint8 suborkbkeys[NUMKEYS_SUBORKB]; // subor keyboard state
static uint8 PartyTapData = 0; // button state flags (6 bits)
static uint8 HyperShotData = 0; // button state flags (4 bits)
static uint32 MahjongData = 0; // button state flags (21 bits)
static uint32 FTrainerData = 0; // button state flags (12 bits)
static uint8 TopRiderData = 0; // button state flags (8 bits)
static uint8 BBattlerData[1+13+1]; // FIXME never initialized or updated

#include "inputDevKeyMap.h" // insert device key maps

#define NUMPRESETS 3
static BtnConfig GamePadPresets[3][GPAD_COUNT][GPAD_NUMKEYS] = {{GPAD_NOCFG(),GPAD_NOCFG(),GPAD_NOCFG(),GPAD_NOCFG()},
{GPAD_NOCFG(),GPAD_NOCFG(),GPAD_NOCFG(),GPAD_NOCFG()},
{GPAD_NOCFG(),GPAD_NOCFG(),GPAD_NOCFG(),GPAD_NOCFG()}};

static bool inputEmuKeyboard = false; // process emulated keyboard flag
static int allowUDLR = 0; // allow U+D, L+R on d-pad
static int DesynchAutoFire = 0; // turbo A and B not at same time

static int addBtnToAutoholdsKey = 0; // key assigned to enable adding pressed buttons to autoholds
static int clearAllAutoholdsKey = 0; // key assigned to clear all prevously added autoholds

static uint8 gpadAutoHoldMask[GPAD_COUNT] = {0, 0, 0, 0}; // flags for button autoholds
	// if a bit is set, corresponding button will be pressed when assigned key is
	// released, and released when assigned key is pressed
static bool addPressedButtonsToAutohold = false;
static bool clearAutoholdButtons = false;

static CFGSTRUCT CommandsConfig[] = {
	"CommandMapping", _FIXME_GetCommandMappingVar(), _FIXME_GetCommandMappingVarSize(),
	ENDCFGSTRUCT
};

static CFGSTRUCT InputConfig[] = {
	AC(PowerPadConfig),
	AC(PartyTapConfig),
	AC(FamiTrainerConfig),
	AC(HyperShotConfig),
	AC(MahjongConfig),
	AC(GamePadConfig),
	NAC("GamePadPreset1", GamePadPresets[0]),
	NAC("GamePadPreset2", GamePadPresets[1]),
	NAC("GamePadPreset3", GamePadPresets[2]),
	AC(FamiKeyBoardConfig),
	AC(SuborKeyBoardConfig),
	ENDCFGSTRUCT
};

static uint32 MouseData[3];


// FIXME: union hack to satisfy current config code, which requres InputPorts to be int[3]
typedef union inputports_ {
	int InputPorts_configAdapter[3];
	struct {
		ESI inputPorts[2]; // types of peripherals plugged into normal ports
		ESIFC inputPortEx; // type of peripheral plugged into expansion port
	};

	inputports_() {
		inputPorts[0] = SI_GAMEPAD;
		inputPorts[1] = SI_NONE;
		inputPortEx = SIFC_NONE;
	}
} InputPorts;
static InputPorts inputPorts;

ESI GetPluggedIn(int port) {
	assert(port >= 0 && port < 2);
	return inputPorts.inputPorts[port];
}

void SetPluggedIn(int port, ESI device) {
	assert(port >= 0 && port < 2);
	inputPorts.inputPorts[port] = device;
}

ESIFC GetPluggedInEx() {
	return inputPorts.inputPortEx;
}

void SetPluggedInEx(ESIFC device) {
	inputPorts.inputPortEx = device;
}

//Initializes the emulator with the current input port configuration
static void ApplyInputPorts()
{
	bool useFourscore = (FL_TEST(eoptions, EO_FOURSCORE) != 0);
	// normal ports
	if(useFourscore) {
		FCEUI_SetInput(0, SI_GAMEPAD, &GamepadData, 0);
		FCEUI_SetInput(1, SI_GAMEPAD, &GamepadData, 0);
	}
	else {
		for(int i=0; i<2; ++i) {
			void* inputDataPtr = NULL;
			switch(inputPorts.inputPorts[i]) {
			case SI_POWERPADA:
			case SI_POWERPADB:
				inputDataPtr = &PowerPadData[i];
				break;
			case SI_GAMEPAD:
				inputDataPtr = &GamepadData;
				break;
			case SI_ARKANOID:
				inputDataPtr = MouseData;
				break;
			case SI_ZAPPER:
				inputDataPtr = MouseData;
				break;
			}
			FCEUI_SetInput(i, inputPorts.inputPorts[i], inputDataPtr, 0);
		}
	}
	FCEUI_SetInputFourscore(useFourscore);

	// expansion port
	void* inputDataPtr = NULL;
	switch(inputPorts.inputPortEx) {
	case SIFC_SHADOW:
	case SIFC_OEKAKIDS:
	case SIFC_ARKANOID:
		inputDataPtr = MouseData;
		break;
	case SIFC_FKB:
		inputDataPtr = fkbkeys;
		break;
	case SIFC_PEC586KB:
	case SIFC_SUBORKB:
		inputDataPtr = suborkbkeys;
		break;
	case SIFC_HYPERSHOT:
		inputDataPtr = &HyperShotData;
		break;
	case SIFC_MAHJONG:
		inputDataPtr = &MahjongData;
		break;
	case SIFC_PARTYTAP:
		inputDataPtr = &PartyTapData;
		break;
	case SIFC_TOPRIDER:
		inputDataPtr = &TopRiderData;
		break;
	case SIFC_BBATTLER:
		inputDataPtr = BBattlerData;
		break;
	case SIFC_FTRAINERA:
	case SIFC_FTRAINERB:
		inputDataPtr = &FTrainerData;
		break;
	}

	FCEUI_SetInputFC(inputPorts.inputPortEx, inputDataPtr, 0);
}

// Bring up the input configuration dialog
void ConfigInput()
{
	driver::input::ui::RootDialog();

	// if a game is running, apply new configuration
	if(GameInfo) ApplyInputPorts();
}

static int TestCommandState_cb(EMUCMD cmd);

bool InitInputDriver()
{
	if(!directinput::Create(fceu_hInstance)) {
		FCEUD_PrintError("DirectInput: Error creating DirectInput object.");
		return false;
	}

	driver::input::keyboard::Init();
	driver::input::joystick::Init();

	FCEUI_SetTestCommandHotkeyCallback(TestCommandState_cb);

	return true;
}

void KillInputDriver()
{
	if(directinput::IsCreated()) {
		driver::input::joystick::Kill();
		driver::input::keyboard::Kill();
		directinput::Destroy();
	}

	FCEUI_SetTestCommandHotkeyCallback(NULL);
}

void SetInputKeyboard(bool on) {
	inputEmuKeyboard = on;
}

bool GetInputKeyboard() {
	return inputEmuKeyboard;
}

void SetAutoFireDesynch(bool DesynchOn)
{
	DesynchAutoFire = (DesynchOn)? 1:0;
}

bool GetAutoFireDesynch()
{
	return (DesynchAutoFire != 0);
}

bool GetAllowUDLR() {
	return (allowUDLR != 0);
}

void SetAllowUDLR(bool allow) {
	allowUDLR = (allow)? 1:0;
}

int GetAutoholdsAddKey() {
	return addBtnToAutoholdsKey;
}

void SetAutoholdsAddKey(int k) {
	addBtnToAutoholdsKey = k;
}

int GetAutoholdsClearKey() {
	return clearAllAutoholdsKey;
}

void SetAutoholdsClearKey(int k) {
	clearAllAutoholdsKey = k;
}

BtnConfig* GetGamePadConfig() {
	return &(GamePadConfig[0][0]);
}

BtnConfig* GetPowerPadConfig() {
	return &(PowerPadConfig[0][0]);
}
	
BtnConfig* GetFamiTrainerConfig() {
	return FamiTrainerConfig;
}

BtnConfig* GetFamiKeyBoardConfig() {
	return FamiKeyBoardConfig;
}

BtnConfig* GetSuborKeyBoardConfig() {
	return SuborKeyBoardConfig;
}

BtnConfig* GetMahjongConfig() {
	return MahjongConfig;
}

BtnConfig* GetPartyTapConfig() {
	return PartyTapConfig;
}

uint8 const (& GetAutoHoldMask(void))[4] {
	return gpadAutoHoldMask;
}

// Test if a button listed in config is pressed
// Returns 1 if button is pressed, zero otherwise
static int TestButton(BtnConfig *bc)
{
	if(driver::input::keyboard::TestButton(bc) || driver::input::joystick::TestButton(bc)) {
		return 1;
	}

	return 0;
}

uint32 GetGamepadPressedPhysical()
{
	uint8 gpadState[4] = {0,};

	for(int gpadIdx=0; gpadIdx<GPAD_COUNT; gpadIdx++) {
		for(int btnIdx=0; btnIdx<8; btnIdx++) {
			if(TestButton(&GamePadConfig[gpadIdx][btnIdx])) {
				FL_SET(gpadState[gpadIdx], 1<<btnIdx);
			}
		}

		if(!allowUDLR) {
			if(FL_TEST(gpadState[gpadIdx], GPAD_UP) && FL_TEST(gpadState[gpadIdx], GPAD_DOWN)) {
				FL_CLEAR(gpadState[gpadIdx], GPAD_UP|GPAD_DOWN);
			}
			if(FL_TEST(gpadState[gpadIdx], GPAD_LEFT) && FL_TEST(gpadState[gpadIdx], GPAD_RIGHT)) {
				FL_CLEAR(gpadState[gpadIdx], GPAD_LEFT|GPAD_RIGHT);
			}
		}
	}

	return (gpadState[3] << 24)|(gpadState[2] << 16)|(gpadState[1] << 8)|(gpadState[0]);
}

// Update gamepad state data
static void UpdateGamepad()
{
	if(FCEUMOV_Mode(MOVIEMODE_PLAY))
		return;

	uint8 gpadState[GPAD_COUNT] = {0,};

	for(int gpadIdx=0; gpadIdx<GPAD_COUNT; ++gpadIdx) {
		for(int btnIdx=0; btnIdx<8; ++btnIdx) {
			int btnMask = 1<<btnIdx;
			if(TestButton(&GamePadConfig[gpadIdx][btnIdx])) {
				FL_SET(gpadState[gpadIdx], btnMask);

				if(addPressedButtonsToAutohold && !FL_TEST((GamepadPrevRawData >> (gpadIdx*8)) & 0xFF, btnMask)) {
					// button just went down
					FL_FROMBOOL(gpadAutoHoldMask[gpadIdx], btnMask, !FL_TEST(gpadAutoHoldMask[gpadIdx], btnMask)); // toggle bit
				}
			}
		}

		// save raw state
		FL_CLEAR(GamepadPrevRawData, (0xFF << (gpadIdx*8)));
		FL_SET(GamepadPrevRawData, (gpadState[gpadIdx] << (gpadIdx*8)));

		// apply U+D L+R limitation
		if(!allowUDLR) {
			if(FL_TEST(gpadState[gpadIdx], GPAD_UP) && FL_TEST(gpadState[gpadIdx], GPAD_DOWN)) {
				FL_CLEAR(gpadState[gpadIdx], GPAD_UP|GPAD_DOWN);
			}
			if(FL_TEST(gpadState[gpadIdx], GPAD_LEFT) && FL_TEST(gpadState[gpadIdx], GPAD_RIGHT)) {
				FL_CLEAR(gpadState[gpadIdx], GPAD_LEFT|GPAD_RIGHT);
			}
		}

		// apply "alternator" (turbo buttons)
		int alternator = GetRapidAlternatorState();
		if(TestButton(&GamePadConfig[gpadIdx][8])) { // A
			FL_SET(gpadState[gpadIdx], (alternator)? 1:0);
		}
		if(DesynchAutoFire) (alternator)? 0:1; // flip alternator state for (B) if desync enabled
		if(TestButton(&GamePadConfig[gpadIdx][9])) { // B
			FL_SET(gpadState[gpadIdx], (alternator)? 2:0);
		}
	}

	// report autohold mask
	// FIXME is this needed? there is neat graphical autoholds feedback we have besides this
	if(addPressedButtonsToAutohold) {
		std::string str;
		int disppos = 38;
		{
			stringstream strstr;
			for(int gpadIdx=0; gpadIdx<GPAD_COUNT; ++gpadIdx) {
				if(gpadIdx>0) strstr << ' ';
				strstr << gpadIdx;
				uint32 p = gpadAutoHoldMask[gpadIdx];
				strstr << (FL_TEST(p, GPAD_LEFT)?'<':' ')
					<< (FL_TEST(p, GPAD_UP)?'^':' ')
					<< (FL_TEST(p, GPAD_RIGHT)?'>':' ')
					<< (FL_TEST(p, GPAD_DOWN)?'v':' ')
					<< (FL_TEST(p, GPAD_A)?'A':' ')
					<< (FL_TEST(p, GPAD_B)?'B':' ')
					<< (FL_TEST(p, GPAD_START)?'S':' ')
					<< (FL_TEST(p, GPAD_SELECT)?'s':' ');
			}

			str = strstr.str();
		}

		// Cut empty chunks off the string
		if(gpadAutoHoldMask[3] == 0) {
			if(gpadAutoHoldMask[2] == 0) {
				if(gpadAutoHoldMask[1] == 0) {
					str.resize(9); // nothing on pads 2, 3, 4
				}
				else {
					str.resize(19); // nothing on pads 3, 4
				}
				disppos = 30; // also text fit in single line
			}
			else {
				str.resize(30); // nothing on pad 4
			}
		}

		FCEU_DispMessage("Held:\n%s", disppos, str.c_str());
	}

	if(clearAutoholdButtons) {
		FCEU_DispMessage("Held:          ", 30);
		gpadAutoHoldMask[0] = 0;
		gpadAutoHoldMask[1] = 0;
		gpadAutoHoldMask[2] = 0;
		gpadAutoHoldMask[3] = 0;
	}

	// apply auto-hold
	for(int gpadIdx=0; gpadIdx<GPAD_COUNT; ++gpadIdx) {
		if(gpadAutoHoldMask[gpadIdx] != 0) gpadState[gpadIdx] ^= gpadAutoHoldMask[gpadIdx];
	}

	GamepadData = (gpadState[3] << 24)|(gpadState[2] << 16)|(gpadState[1] << 8)|(gpadState[0]);
}

// Update Power pad state data
static void UpdatePowerPad(int which)
{
	BtnConfig* ppadtsc = PowerPadConfig[which];

	PowerPadData[which] = 0;
	for(int x=0; x<NUMKEYS_POWERPAD; ++x) {
		if(TestButton(&ppadtsc[x])) FL_SET(PowerPadData[which], 1<<x);
	}
}

// Update Famicom keyboard state data
static void UpdateFamiKeyBoard()
{
	for(int x=0; x<NUMKEYS_FAMIKB; ++x) {
		fkbkeys[x] = (TestButton(&FamiKeyBoardConfig[x]))? 1:0;
	}
}

// Update Subor keyboard state data
static void UpdateSuborKeyBoard()
{
	for(int x=0; x<NUMKEYS_SUBORKB; ++x) {
		suborkbkeys[x] = (TestButton(&SuborKeyBoardConfig[x]))? 1:0;
	}
}

// Update Hypershot state data
static void UpdateHyperShot()
{
	HyperShotData = 0;
	for(int x=0; x<NUMKEYS_HYPERSHOT; ++x) {
		if(TestButton(&HyperShotConfig[x])) {
			FL_SET(HyperShotData, 1 << x);
		}
	}
}

// Update Mahjong state data
static void UpdateMahjong()
{
	MahjongData = 0;
	for(int x=0; x<NUMKEYS_MAHJONG; x++) {  
		if(TestButton(&MahjongConfig[x])) {
			FL_SET(MahjongData, 1 << x);
		}
	}
}

// Update Partytap state data
static void UpdatePartyTap()
{
	PartyTapData = 0;
	for(int x=0; x<NUMKEYS_PARTYTAP; ++x) {
		if(TestButton(&PartyTapConfig[x])) {
			FL_SET(PartyTapData, 1 << x);
		}
	}

}

// Update Top rider state data
static void UpdateTopRider()
{
	TopRiderData = 0;
	for(int x=0; x<NUMKEYS_TOPRIDER; ++x) {
		if(TestButton(&TopRiderConfig[x])) {
			FL_SET(TopRiderData, 1 << x);
		}
	}
}

// Update Famitrainer state data
static void UpdateFamiTrainer()
{
	FTrainerData = 0;
	for(int x=0; x<NUMKEYS_FAMITRAINER; ++x) {
		if(TestButton(&FamiTrainerConfig[x])) {
			FL_SET(FTrainerData, 1 << x);
		}
	}
}

// callback for FCEUI_HandleEmuCommands()
// returns 1 if command hotkey is pressed, 0 otherwise
static int TestCommandState_cb(EMUCMD cmd)
{
	driver::input::keyboard::KeysState_const keys = driver::input::keyboard::GetState();

	KeyCombo keyCombo = GetCommandKeyCombo(cmd);
	int keyCode = keyCombo.getKey();
	uint32 expectedModifiers = keyCombo.getModifiers();
	uint32 actualModifiers = 0;
	// NOTE in the command key combo modifier keys are simplified to only left modifier keys,
	// here we also set left modifier when actually right modifier was pressed
	if(keys[SCAN_LEFTALT] && keyCode != SCAN_LEFTALT) FL_SET(actualModifiers, KeyCombo::LALT_BIT);
	if(keys[SCAN_RIGHTALT] && keyCode != SCAN_RIGHTALT) FL_SET(actualModifiers, KeyCombo::LALT_BIT);
	if(keys[SCAN_LEFTCONTROL] && keyCode != SCAN_LEFTCONTROL) FL_SET(actualModifiers, KeyCombo::LCTRL_BIT);
	if(keys[SCAN_RIGHTCONTROL] && keyCode != SCAN_RIGHTCONTROL) FL_SET(actualModifiers, KeyCombo::LCTRL_BIT);
	if(keys[SCAN_LEFTSHIFT] && keyCode != SCAN_LEFTSHIFT) FL_SET(actualModifiers, KeyCombo::LSHIFT_BIT);
	if(keys[SCAN_RIGHTSHIFT] && keyCode != SCAN_RIGHTSHIFT) FL_SET(actualModifiers, KeyCombo::LSHIFT_BIT);

	if(actualModifiers == expectedModifiers) {
		return (keys[keyCode])? 1 : 0;
	}

	return 0;
}


void FCEUD_UpdateInput(int flags)
{
	assert(flags != 0); // must specify

	if(FL_TEST(flags, UPDATEINPUT_KEYBOARD)) {
		driver::input::keyboard::Update();
		// check add/clear autoholds buttons
		driver::input::keyboard::KeysState_const keys = driver::input::keyboard::GetState();
		addPressedButtonsToAutohold = ( addBtnToAutoholdsKey != 0 && keys[addBtnToAutoholdsKey] != 0 );
		clearAutoholdButtons = ( clearAllAutoholdsKey != 0 && keys[clearAllAutoholdsKey] != 0 );
	}
	
	if(FL_TEST(flags, UPDATEINPUT_JOYSTICKS)) {
		driver::input::joystick::Update();
	}

	if(FL_TEST(flags, UPDATEINPUT_COMMANDS)) {
		FCEUI_HandleEmuCommands();
	}

	if(FL_TEST(flags, UPDATEINPUT_EVERYTHING) == UPDATEINPUT_EVERYTHING) {
		bool needGPadUpdate = false;
		bool needMouseUpdate = false;
		for(int x=0; x<2; x++) {
			switch(inputPorts.inputPorts[x]) {
			case SI_GAMEPAD:
				needGPadUpdate = true;
				break;
			case SI_ARKANOID:
				needMouseUpdate = true;
				break;
			case SI_ZAPPER:
				needMouseUpdate = true;
				break;
			case SI_POWERPADA:
			case SI_POWERPADB:
				UpdatePowerPad(x);
				break;
			}
		}

		if(needGPadUpdate) UpdateGamepad();

		switch(inputPorts.inputPortEx) {
		case SIFC_OEKAKIDS:
		case SIFC_ARKANOID:
		case SIFC_SHADOW:
			needMouseUpdate = true;
			break;
		case SIFC_FKB:
			if(inputEmuKeyboard) UpdateFamiKeyBoard();
			break;
		case SIFC_PEC586KB:
		case SIFC_SUBORKB:
			if(inputEmuKeyboard) UpdateSuborKeyBoard();
			break;
		case SIFC_HYPERSHOT:
			UpdateHyperShot();
			break;
		case SIFC_MAHJONG:
			UpdateMahjong();
			break;
		case SIFC_PARTYTAP:
			UpdatePartyTap();
			break;
		case SIFC_FTRAINERB:
		case SIFC_FTRAINERA:
			UpdateFamiTrainer();
			break;
		case SIFC_TOPRIDER:
			UpdateTopRider();
			break;
		}

		if(needMouseUpdate) {
			if(FCEUMOV_Mode() != MOVIEMODE_PLAY) {	//FatRatKnight: Moved this if out of the function
				GetMouseData(MouseData);			//A more concise fix may be desired.
			}
		}
	}
}

void FCEUD_SetInput(bool fourscore, bool microphone, ESI port0, ESI port1, ESIFC fcexp)
{
	FL_FROMBOOL(eoptions, EO_FOURSCORE, fourscore);

	FCEUI_SetInputMicrophone(microphone);

	inputPorts.inputPorts[0] = port0;
	inputPorts.inputPorts[1] = port1;
	inputPorts.inputPortEx = fcexp;

	ApplyInputPorts();
}

void FCEUD_SetInput(ESI port0, ESI port1, ESIFC fcexp) {
	if(port0 != SI_UNSET)
		inputPorts.inputPorts[0] = port0;
	if(port1 != SI_UNSET)
		inputPorts.inputPorts[1] = port1;
	if(fcexp != SIFC_UNSET)
		inputPorts.inputPortEx = fcexp;

	ApplyInputPorts();
}

void FCEUI_UseInputPreset(int preset) {
	assert(preset < NUMPRESETS);
	memcpy(GamePadConfig, GamePadPresets[preset], sizeof(GamePadConfig));

	FCEU_DispMessage("Using input preset %d.",0,preset+1);
}

void CopyConfigToPreset(unsigned int presetIdx) {
	assert(presetIdx < NUMPRESETS);
	memcpy(GamePadPresets[presetIdx], GamePadConfig, sizeof(GamePadConfig));
}

void PresetExport(int presetNum) {
	std::string initdir = FCEU_GetPath(FCEUMKF_INPUT);
	const char filter[]="Input Preset File (*.pre)\0*.pre\0All Files (*.*)\0*.*\0\0";
	char nameo[2048];
	nameo[0] = '\0'; //No default filename

	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hInstance = fceu_hInstance;
	ofn.lpstrTitle = "Export Input Preset To...";
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = nameo;
	ofn.lpstrDefExt = "pre";
	ofn.nMaxFile = 256;
	ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	ofn.lpstrInitialDir = initdir.c_str();

	if(GetSaveFileName(&ofn)) {
		BtnConfig* datap = &GamePadPresets[presetNum][0][0];
		unsigned int size = sizeof(GamePadPresets[presetNum]);

		FILE* fp = FCEUD_UTF8fopen(nameo, "w");
		fwrite(datap, 1, size, fp);
		fclose(fp);
	}
}

void PresetImport(int presetNum) {
	std::string initdir = FCEU_GetPath(FCEUMKF_INPUT);
	const char filter[]="Input Preset File (*.pre)\0*.pre\0\0";
	char nameo[2048];
	nameo[0] = '\0';

	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hInstance = fceu_hInstance;
	ofn.lpstrTitle = "Import Input Preset......";
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = nameo;
	ofn.nMaxFile = 256;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrInitialDir = initdir.c_str();

	if(GetOpenFileName(&ofn)) {
		BtnConfig* data = &GamePadPresets[presetNum][0][0];
		unsigned int size = sizeof(GamePadPresets[presetNum]);

		FILE* fp = FCEUD_UTF8fopen(nameo, "r");
		fread(data, 1, size, fp);
		fclose(fp);
	}
}


int (&_FIXME_GetInputPortsArr(void))[3] {
	return inputPorts.InputPorts_configAdapter;
}

int& _FIXME_GetAddAutoholdsKeyVar() {
	return addBtnToAutoholdsKey;
}

int& _FIXME_GetClearAutoholdsKeyVar() {
	return clearAllAutoholdsKey;
}

int& _FIXME_GetAllowUDLRVar() {
	return allowUDLR;
}

int& _FIXME_GetDesynchAutoFireVar() {
	return DesynchAutoFire;
}

CFGSTRUCT* _FIXME_GetCommandsConfigVar() {
	return &CommandsConfig[0];
}

CFGSTRUCT* _FIXME_GetInputConfigVar() {
	return &InputConfig[0];
}
