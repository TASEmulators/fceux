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
	Input configuration UI - Windows implementation
*/

#define _WIN32_IE	0x0550
#include <windows.h>
#include <commctrl.h>

#include <sstream>
#include <iomanip>

#include "inputConfigUI.h"
#include "drivers/win/common.h"
#include "drivers/win/window.h"
#include "drivers/win/input.h"
#include "drivers/win/joystick.h"
#include "drivers/win/gui.h"
#include "utils/bitflags.h"
#include "../../movie.h"
#include "../../version.h"
#include "../../input/gamepad.h"
#include "../../input/fkb.h"
#include "../../input/suborkb.h"


// main
#define KEY_MASK_SCANCODE 0x7F0000
#define KEY_MASK_EXTBIT 0x1000000
#define BTNNUM_TO_WINCODE(num) (((num << 16) & KEY_MASK_SCANCODE) | ((num << 17) & KEY_MASK_EXTBIT))
#define WINCODE_TO_BTNNUM(cod) (((cod & KEY_MASK_SCANCODE) >> 16) | ((cod & KEY_MASK_EXTBIT) >> 17))
const unsigned int NUMBER_OF_PORTS = 2; // number of (normal) input ports
// which input devices can be configured
static const BOOL configurable_nes[SI_COUNT + 1] = { FALSE, TRUE, FALSE, TRUE, TRUE, FALSE };
static const BOOL configurable_fam[SIFC_COUNT + 1] = { FALSE,FALSE,FALSE,FALSE, TRUE,TRUE,TRUE,FALSE, TRUE,TRUE,TRUE,TRUE, FALSE,FALSE,FALSE };
HWND hwndMainDlg;

// device config
static BtnConfig *DeviceConfigButtons = 0;
static const char *DeviceConfigTitle = 0;
static int DeviceConfigButtonsCount = 0;
static int VirtualDeviceType = 0;
static int VirtualDevicePort = 0;

// button config
static HWND hwndNormalDlg;
static BtnConfig dummy_;
static BtnConfig* targetBtnCfg = NULL;
#define WM_USER_SIMPLEKEYDOWN (WM_USER + 666)
#define ID_TIMER (666)
HWND hwndNormalDlgParent;

// simplified button config
HWND hwndSimplifiedDlg;
HWND hwndSimplifiedDlgParent;

static void UpdateComboPad(HWND dlg, WORD id)
{
	unsigned int sel_input = id - COMBO_PAD1;

	// Update the user input type
	SetPluggedIn(sel_input, (ESI)SendDlgItemMessage(
		dlg,
		id,
		CB_GETCURSEL,
		0,
		(LPARAM)(LPSTR)0
		));

	// Enable or disable the configuration button
	EnableWindow(
		GetDlgItem(dlg, id + 2),
		configurable_nes[GetPluggedIn(sel_input)]
	);

	// Update the text field
	SetDlgItemText(
		dlg,
		TXT_PAD1 + sel_input,
		(LPTSTR)ESI_Name(GetPluggedIn(sel_input))
	);
}

static void UpdateFourscoreState(HWND dlg)
{
	//(inverse logic:)
	BOOL enable = (eoptions & EO_FOURSCORE)?FALSE:TRUE;

	EnableWindow(GetDlgItem(dlg,BTN_PORT1),enable);
	EnableWindow(GetDlgItem(dlg,BTN_PORT2),enable);
	EnableWindow(GetDlgItem(dlg,COMBO_PAD1),enable);
	EnableWindow(GetDlgItem(dlg,COMBO_PAD2),enable);
	EnableWindow(GetDlgItem(dlg,TXT_PAD1),enable);
	EnableWindow(GetDlgItem(dlg,TXT_PAD2),enable);
	
	if(!enable) {
		//change the inputs to gamepad
		SendMessage(GetDlgItem(dlg,COMBO_PAD1),CB_SETCURSEL,SI_GAMEPAD,0);
		SendMessage(GetDlgItem(dlg,COMBO_PAD2),CB_SETCURSEL,SI_GAMEPAD,0);
		UpdateComboPad(dlg,COMBO_PAD1);
		UpdateComboPad(dlg,COMBO_PAD2);
		SetDlgItemText(dlg,TXT_PAD1,ESI_Name(SI_GAMEPAD));
		SetDlgItemText(dlg,TXT_PAD2,ESI_Name(SI_GAMEPAD));
	}
}

static void UpdateComboFam(HWND dlg)
{
	// Update the user input type of the famicom
	SetPluggedInEx((ESIFC)SendDlgItemMessage(
		dlg,
		COMBO_FAM,
		CB_GETCURSEL,
		0,
		(LPARAM)(LPSTR)0
		));

	// Enable or disable the configuration button
	EnableWindow(
		GetDlgItem(dlg, BTN_FAM),
		configurable_fam[GetPluggedInEx()]
	);

	// Update the text field
	SetDlgItemText(
		dlg,
		TXT_FAM,
		(LPTSTR)ESIFC_Name(GetPluggedInEx())
	);
}


static void MakeButtString(const BtnConfig& btnCfg, std::string& outstr)
{
	std::stringstream strstr;
	for(uint32 btnIdx=0; btnIdx<btnCfg.NumC; btnIdx++) {
		if(btnIdx > 0) strstr << ", ";

		if(btnCfg.ButtType[btnIdx] == BtnConfig::BT_KEYBOARD) {
			strstr << "KB: ";
			char keyname[16];
			if(GetKeyNameText(BTNNUM_TO_WINCODE(btnCfg.GetScanCode(btnIdx)), keyname, 16)) {
				strstr << keyname;
			}
			else {
				// GetKeyNameText wasn't able to provide a name for the key, then just show scancode
				strstr << std::right << std::setw(3) << btnCfg.GetScanCode(btnIdx);
			}
		}
		else if(btnCfg.ButtType[btnIdx] == BtnConfig::BT_JOYSTICK) {
			strstr << "JS " << ((unsigned int)btnCfg.DeviceNum[btnIdx]);
			if(btnCfg.IsAxisButton(btnIdx)) {
				char *asel[3]={"x","y","z"};
				strstr << " axis " << asel[btnCfg.GetAxisIdx(btnIdx)] << (btnCfg.IsAxisNegative(btnIdx)? "-":"+");
			}
			else if(btnCfg.IsPovButton(btnIdx)) {
				strstr << " hat " << ((int)btnCfg.GetPovController(btnIdx)) << ':' << ((int)btnCfg.GetPovDir(btnIdx));
			}
			else {
				strstr << " button " << ((int)btnCfg.GetJoyButton(btnIdx));
			}
		}
	}

	outstr = strstr.str();
}

static void SetDlgButtonsString(HWND dlg, int textID, const BtnConfig& btnCfg)
{
	std::string str;
	MakeButtString(btnCfg, str);
	SetDlgItemText(dlg, textID, str.c_str());
}

static BOOL CALLBACK ButtonConfigDialogProc(HWND dlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static bool isFirstButton = true;
	bool closeDlg = false;

	switch(uMsg) {
	case WM_DESTROY:
		hwndNormalDlg = NULL;
		break;
	case WM_TIMER:
		{
			JOYINSTANCEID guid;
			uint8 devicenum;
			uint16 buttonnum;

			if(driver::input::joystick::GetButtonPressedTest(&guid, &devicenum, &buttonnum)) {
				// Got button pressed
				if(isFirstButton) {
					targetBtnCfg->NumC = 0;
					isFirstButton = false;
				}
				int btnIdx = targetBtnCfg->NumC;
				targetBtnCfg->ButtType[btnIdx] = BtnConfig::BT_JOYSTICK;
				targetBtnCfg->DeviceNum[btnIdx] = devicenum;
				targetBtnCfg->AssignJoyButton(btnIdx, buttonnum);
				targetBtnCfg->DeviceInstance[btnIdx] = guid;

				// Stop config if the user pushes the same button twice in a row.
				if(btnIdx &&
					targetBtnCfg->ButtType[btnIdx] == targetBtnCfg->ButtType[btnIdx-1] &&
					targetBtnCfg->DeviceNum[btnIdx] == targetBtnCfg->DeviceNum[btnIdx-1] &&
					targetBtnCfg->ButtonNum[btnIdx] == targetBtnCfg->ButtonNum[btnIdx-1])
				{
					closeDlg = true;
				}
				else {
					targetBtnCfg->NumC++;

					// Stop config if we reached our maximum button limit.
					if(targetBtnCfg->NumC >= MAXBUTTCONFIG) closeDlg = true;

					SetDlgButtonsString(dlg, LBL_DWBDIALOG_TEXT, *targetBtnCfg);
				}
			}
		}
		break;
	case WM_USER_SIMPLEKEYDOWN:
		// received button press via windows message
		if(isFirstButton) {
			targetBtnCfg->NumC = 0;
			isFirstButton = false;

			SetDlgButtonsString(dlg, LBL_DWBDIALOG_TEXT, *targetBtnCfg);
		}

		{
			int btnIdx = targetBtnCfg->NumC;

			targetBtnCfg->ButtType[btnIdx] = BtnConfig::BT_KEYBOARD;
			targetBtnCfg->DeviceNum[btnIdx] = 0;
			targetBtnCfg->AssignScanCode(btnIdx, lParam & 0xFF);

			//Stop config if the user pushes the same button twice in a row.
			if(btnIdx &&
				targetBtnCfg->ButtType[btnIdx]==targetBtnCfg->ButtType[btnIdx-1] &&
				targetBtnCfg->DeviceNum[btnIdx]==targetBtnCfg->DeviceNum[btnIdx-1] &&
				targetBtnCfg->ButtonNum[btnIdx]==targetBtnCfg->ButtonNum[btnIdx-1])
			{
				closeDlg = true;
			}
			else {
				targetBtnCfg->NumC++;
				//Stop config if we reached our maximum button limit.
				if(targetBtnCfg->NumC >= MAXBUTTCONFIG) closeDlg = true;

				SetDlgButtonsString(dlg, LBL_DWBDIALOG_TEXT, *targetBtnCfg);
			}
		}
		break;
	case WM_INITDIALOG:
		isFirstButton = true;

		driver::input::joystick::BeginWaitButton(dlg);
		SetTimer(dlg, ID_TIMER, 25, 0); //Every 25ms.

		SetDlgButtonsString(dlg, LBL_DWBDIALOG_TEXT, *targetBtnCfg);
		break;
	case WM_CLOSE:
	case WM_QUIT:
		closeDlg = true;
		break;
	case WM_COMMAND:
		switch(wParam & 0xFFFF) {
		case BTN_CLEAR:
			{
				// clear configuration
				targetBtnCfg->NumC = 0;

				SetDlgButtonsString(dlg, LBL_DWBDIALOG_TEXT, *targetBtnCfg);
			}
			break;
		case BTN_CLOSE:
			closeDlg = true;
			break;
		}
	}

	if(closeDlg) {
		KillTimer(dlg, ID_TIMER);
		driver::input::joystick::EndWaitButton(hAppWnd);
		SetForegroundWindow(GetParent(dlg));
		DestroyWindow(dlg);
	}

	return 0;
}

static void DoButtonConfigDlg(const uint8 *title, BtnConfig& bc) {
	targetBtnCfg = &bc;

	hwndNormalDlg = CreateDialog(fceu_hInstance, "DWBDIALOG", hwndNormalDlgParent, ButtonConfigDialogProc);
	EnableWindow(hwndNormalDlgParent, FALSE);
	SetWindowText(hwndNormalDlg, (char*)title);
	ShowWindow(hwndNormalDlg, SW_SHOWNORMAL);

	while(hwndNormalDlg) {
		MSG msg;
		while(PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE)) {
			if(GetMessage(&msg, 0, 0, 0) > 0) {
				if(msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN) {
					PostMessage(hwndNormalDlg, WM_USER_SIMPLEKEYDOWN, 0, WINCODE_TO_BTNNUM(msg.lParam));
					continue;
				}

				if(msg.message == WM_SYSCOMMAND) continue;

				if(!IsDialogMessage(hwndNormalDlg, &msg)) {
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		}
		Sleep(10);
	}

	EnableWindow(hwndNormalDlgParent, TRUE);
}


static BOOL CALLBACK VirtualDeviceConfigProc(HWND dlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	bool closeDlg = false;

	switch(uMsg) {
	case WM_INITDIALOG:
		SetWindowText(dlg, DeviceConfigTitle);
		if(VirtualDeviceType == SI_GAMEPAD) {
			// assign labels for two gamepads
			char buf[32];
			sprintf(buf, "Virtual Gamepad %d", VirtualDevicePort+1);
			SetDlgItemText(dlg, GRP_GAMEPAD1, buf);

			sprintf(buf, "Virtual Gamepad %d", VirtualDevicePort+3);
			SetDlgItemText(dlg, GRP_GAMEPAD2, buf);
		}
		break;
	case WM_CLOSE:
	case WM_QUIT:
		closeDlg = true;
		break;
	case WM_COMMAND:
		{
			int ctrlId = LOWORD(wParam);
			int configIdx = ctrlId - 300;
			if(configIdx >= 0 && configIdx < DeviceConfigButtonsCount) {
				char btnText[128];
				btnText[0] = '\0';
				GetDlgItemText(dlg, ctrlId, btnText, 128);
				hwndNormalDlgParent = dlg;
				DoButtonConfigDlg((uint8*)btnText, DeviceConfigButtons[configIdx]);
			}
			else switch(LOWORD(wParam)) {
			case BTN_CLOSE:
				closeDlg = true;
				break;
			}
		}
	}

	if(closeDlg) EndDialog(dlg, 0);

	return 0;
}

static void DoVirtualDeviceConfigDlg(const char *title, char *dlgTemplate, BtnConfig *buttons, int buttonsCount) {
	DeviceConfigTitle = title;
	DeviceConfigButtons = buttons;
	DeviceConfigButtonsCount = buttonsCount;
	DialogBox(fceu_hInstance, dlgTemplate, hwndMainDlg, VirtualDeviceConfigProc);
}


static int DoSimplifiedButtonConfigDlg(const uint8* title) {
	int btnCode = 0;

	hwndSimplifiedDlg = CreateDialog(fceu_hInstance, "DWBDIALOGSIMPLE", hwndSimplifiedDlgParent, NULL);
	EnableWindow(hwndSimplifiedDlgParent, 0);
	SetWindowText(hwndSimplifiedDlg, (char*)title);
	ShowWindow(hwndSimplifiedDlg, SW_SHOWNORMAL);

	while(hwndSimplifiedDlg) {
		MSG msg;
		while(PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE)) {
			if(GetMessage(&msg, 0, 0, 0) > 0) {
				if(msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN) {
					btnCode = WINCODE_TO_BTNNUM(msg.lParam);
					if(btnCode == 1) btnCode = 0; // convert Esc to nothing
					goto done;
				}

				if(msg.message == WM_SYSCOMMAND) continue;

				if(!IsDialogMessage(hwndSimplifiedDlg, &msg)) {
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		}
		Sleep(10);
	}

done:
	EndDialog(hwndSimplifiedDlg, NULL);
	EnableWindow(hwndSimplifiedDlgParent, TRUE);

	return btnCode;
}


// Main input configuration dialog proc
static BOOL CALLBACK ConfigInputDialogProc(HWND dlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	hwndMainDlg = dlg;

	switch(uMsg) {
	case WM_INITDIALOG:
		// Update the disable UDLR checkbox based on the current value
		CheckDlgButton(dlg, BTN_ALLOW_LRUD, (GetAllowUDLR())? BST_CHECKED:BST_UNCHECKED);

		//update the fourscore checkbox
		CheckDlgButton(dlg, CHECK_ENABLE_FOURSCORE, FL_TEST(eoptions, EO_FOURSCORE)? BST_CHECKED:BST_UNCHECKED);

		//update the microphone checkbox
		CheckDlgButton(dlg, CHECK_ENABLE_MICROPHONE, FCEUI_GetInputMicrophone()? BST_CHECKED:BST_UNCHECKED);

		// Initialize the controls for the input ports
		for(unsigned int port = 0; port < NUMBER_OF_PORTS; port++) {
			// Initialize the combobox
			for(unsigned int current_device = 0; current_device < SI_COUNT + 1; ++current_device) {
				SendDlgItemMessage(dlg,
					COMBO_PAD1 + port,
					CB_ADDSTRING,
					0,
					(LPARAM)ESI_Name((ESI)current_device)
				);
			}

			// Fix to deal with corrupted config. 
			if (GetPluggedIn(port)>SI_COUNT || GetPluggedIn(port)<0)
				SetPluggedIn(port, SI_UNSET); 

			// Update the combobox selection according to the
			// currently selected input mode.
			SendDlgItemMessage(dlg,
				COMBO_PAD1 + port,
				CB_SETCURSEL,
				GetPluggedIn(port),
				(LPARAM)0
				);

			// Enable the configuration button if necessary.
			EnableWindow(
				GetDlgItem(dlg, BTN_PORT1 + port),
				configurable_nes[GetPluggedIn(port)]
			);

			// Update the label that displays the input device.
			SetDlgItemText(
				dlg,
				TXT_PAD1 + port,
				(LPTSTR)ESI_Name(GetPluggedIn(port))
			);
		}

		// Initialize the Famicom combobox
		for(unsigned current_device = 0; current_device < SIFC_COUNT + 1; ++current_device) {
			SendDlgItemMessage(
				dlg,
				COMBO_FAM,
				CB_ADDSTRING,
				0,
				(LPARAM)(LPSTR)ESIFC_Name((ESIFC)current_device)
			);
		}

		if (GetPluggedInEx()>SIFC_COUNT || GetPluggedInEx()<0) {
			SetPluggedInEx(SIFC_UNSET);
		}

		// Update the combobox selection according to the
		// currently selected input mode.
		SendDlgItemMessage(
			dlg,
			COMBO_FAM,
			CB_SETCURSEL,
			GetPluggedInEx(),
			(LPARAM)0
			);

		// Enable the configuration button if necessary.
		EnableWindow(
			GetDlgItem(dlg, BTN_FAM),
			configurable_fam[GetPluggedInEx()]
		);

		// Update the label that displays the input device.
		SetDlgItemText(
			dlg,
			TXT_FAM,
			(LPTSTR)ESIFC_Name(GetPluggedInEx())
		);

		{
			// Initialize the auto key controls
			char btext[128];
			if(GetAutoholdsAddKey() != 0) {
				if(!GetKeyNameText(BTNNUM_TO_WINCODE(GetAutoholdsAddKey()), btext, 128)) {
					sprintf(btext, "KB: %d", GetAutoholdsAddKey());
				}
			}
			else {
				sprintf(btext, "not assigned");
			}
			SetDlgItemText(dlg, LBL_AUTO_HOLD, btext);

			if(GetAutoholdsClearKey() != 0) {
				if(!GetKeyNameText(BTNNUM_TO_WINCODE(GetAutoholdsClearKey()), btext, 128)) {
					sprintf(btext, "KB: %d", GetAutoholdsClearKey());
				}
			}
			else {
				sprintf(btext, "not assigned");
			}
			SetDlgItemText(dlg, LBL_CLEAR_AH, btext);
		}

		CenterWindowOnScreen(dlg);
		UpdateFourscoreState(dlg);

		if (!FCEUMOV_Mode(MOVIEMODE_INACTIVE)) {
			// disable changing fourscore and Input ports while a movie is recorded/played
			EnableWindow(GetDlgItem(dlg, CHECK_ENABLE_FOURSCORE), FALSE);
			EnableWindow(GetDlgItem(dlg, CHECK_ENABLE_MICROPHONE), FALSE);
			EnableWindow(GetDlgItem(dlg, COMBO_PAD1), FALSE);
			EnableWindow(GetDlgItem(dlg, COMBO_PAD2), FALSE);
			EnableWindow(GetDlgItem(dlg, COMBO_FAM), FALSE);
		}

		break;

	case WM_CLOSE:
	case WM_QUIT:
		EndDialog(dlg, 0);
		break;

	case WM_COMMAND:
		if(HIWORD(wParam) == CBN_SELENDOK) {
			switch(LOWORD(wParam)) {
			case COMBO_PAD1:
			case COMBO_PAD2:
				UpdateComboPad(dlg, LOWORD(wParam));
				break;

			case COMBO_FAM:
				UpdateComboFam(dlg);
				break;
			}
		}
		else if( HIWORD(wParam) == BN_CLICKED ) {
			switch(LOWORD(wParam)) {
			case BTN_ALLOW_LRUD:
				SetAllowUDLR(!GetAllowUDLR());
				FCEU_printf("Allow UDLR toggled.\n");
				break;
			case CHECK_ENABLE_FOURSCORE:
				FL_FROMBOOL(eoptions, EO_FOURSCORE, !FL_TEST(eoptions, EO_FOURSCORE));
				FCEU_printf("Fourscore toggled to %s\n",(eoptions & EO_FOURSCORE)?"ON":"OFF");
				UpdateFourscoreState(dlg);
				break;
			case CHECK_ENABLE_MICROPHONE:
				FCEUI_SetInputMicrophone(!FCEUI_GetInputMicrophone());
				FCEU_printf("Microphone toggled to %s\n", FCEUI_GetInputMicrophone()? "ON":"OFF");
				break;
			case BTN_FAM:
				{
					const char *text = ESIFC_Name(GetPluggedInEx());

					VirtualDeviceType = VirtualDevicePort = 0;

					switch(GetPluggedInEx())
					{
					case SIFC_FTRAINERA:
					case SIFC_FTRAINERB:
						DoVirtualDeviceConfigDlg(text, "POWERPADDIALOG", GetFamiTrainerConfig(), 12);
						break;
					case SIFC_FKB:
						DoVirtualDeviceConfigDlg(text, "FKBDIALOG", GetFamiKeyBoardConfig(), NUMKEYS_FAMIKB);
						break;
					case SIFC_PEC586KB:
					case SIFC_SUBORKB:
						DoVirtualDeviceConfigDlg(text, "SUBORKBDIALOG", GetSuborKeyBoardConfig(), NUMKEYS_SUBORKB);
						break;
					case SIFC_MAHJONG:
						DoVirtualDeviceConfigDlg(text, "MAHJONGDIALOG", GetMahjongConfig(), 21);
						break;
					case SIFC_PARTYTAP:
						DoVirtualDeviceConfigDlg(text, "PARTYTAPDIALOG", GetPartyTapConfig(), 6);
						break;
					}
				}

				break;

			case BTN_PORT2:
			case BTN_PORT1:
				{
					int which = LOWORD(wParam) - BTN_PORT1;
					const char *text = ESI_Name(GetPluggedIn(which));

					VirtualDeviceType = VirtualDevicePort = 0;

					switch(GetPluggedIn(which)) {
					case SI_GAMEPAD:
						{
							BtnConfig tmp[GPAD_NUMKEYS*2]; // reserve room for two gpad configs
							BtnConfig* target = GetGamePadConfig() + (which * GPAD_NUMKEYS);

							uint32 count = GPAD_NUMKEYS * sizeof(BtnConfig);
							memcpy(tmp, target, count); // 1st to cinfigure (gpad 0 or 1)
							memcpy(tmp + GPAD_NUMKEYS, target + GPAD_NUMKEYS * 2, count); // 2nd to configure (gpad 2 or 3)

							VirtualDeviceType = SI_GAMEPAD;
							VirtualDevicePort = which;
							DoVirtualDeviceConfigDlg(text, "GAMEPADDIALOG", tmp, GPAD_NUMKEYS*2);

							memcpy(target, tmp, count); // copy back first set to 0/1
							memcpy(target + GPAD_NUMKEYS * 2, tmp + GPAD_NUMKEYS, count); // copy back second set to gpad 2/3
						}
						break;

					case SI_POWERPADA:
					case SI_POWERPADB:
						DoVirtualDeviceConfigDlg(text, "POWERPADDIALOG", &(GetPowerPadConfig()[which]), 12);
						break;
					}
				}

				break;

			case BTN_PRESET_SET1:
				MessageBox(0, "Current input configuration has been set as Preset 1.", FCEU_NAME, MB_ICONINFORMATION | MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
				CopyConfigToPreset(0);
				break;
			case BTN_PRESET_SET2:
				MessageBox(0, "Current input configuration has been set as Preset 2.", FCEU_NAME, MB_ICONINFORMATION | MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
				CopyConfigToPreset(1);
				break;
			case BTN_PRESET_SET3:
				MessageBox(0, "Current input configuration has been set as Preset 3.", FCEU_NAME, MB_ICONINFORMATION | MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
				CopyConfigToPreset(2);
				break;

			case BTN_PRESET_EXPORT1: PresetExport(1); break;
			case BTN_PRESET_EXPORT2: PresetExport(2); break;
			case BTN_PRESET_EXPORT3: PresetExport(3); break;

			case BTN_PRESET_IMPORT1: PresetImport(1); break;
			case BTN_PRESET_IMPORT2: PresetImport(2); break;
			case BTN_PRESET_IMPORT3: PresetImport(3); break;

			case BTN_AUTO_HOLD: // auto-hold button
				{
					char btext[128] = { 0 };

					GetDlgItemText(dlg, BTN_AUTO_HOLD, btext, sizeof(btext) );

					hwndSimplifiedDlgParent = dlg;
					int button = DoSimplifiedButtonConfigDlg((uint8*)btext);

					if(button) {
						if(!GetKeyNameText(BTNNUM_TO_WINCODE(button), btext, 128)) {
							sprintf(btext, "KB: %d", button);
						}
					}
					else {
						sprintf(btext, "not assigned");
					}
					SetDlgItemText(dlg, LBL_AUTO_HOLD, btext);

					SetAutoholdsAddKey(button);
				}
				break;

			case BTN_CLEAR_AH: // auto-hold clear button
				{
					char btext[128] = { 0 };

					GetDlgItemText(dlg, BTN_CLEAR_AH, btext, 128);

					hwndSimplifiedDlgParent = dlg;
					int button = DoSimplifiedButtonConfigDlg((uint8*)btext);

					if(button) {
						if( !GetKeyNameText(BTNNUM_TO_WINCODE(button), btext, sizeof(btext))) {
							sprintf(btext, "KB: %d", button);
						}
					}
					else {
						sprintf(btext, "not assigned");
					}
					SetDlgItemText(dlg, LBL_CLEAR_AH, btext);

					SetAutoholdsClearKey(button);
				}
				break;

			case BTN_CLOSE:
				EndDialog(dlg, 0);
				break;
			}
		}
	}

	return 0;
}






void driver::input::ui::RootDialog() {
	DialogBox(fceu_hInstance, "INPUTCONFIG", GetMainHWND(), ConfigInputDialogProc);
}
