
#include <windows.h>
#include "common.h"
#include "fceu.h"
#include "types.h"
#include "x6502.h"
#include "cart.h"
#include "ines.h"

#include "resource.h"
#include "header_editor.h"


// VS System type
char* vsSysList[] = {
	"Normal",
	"RBI Baseball",
	"TKO Boxing",
	"Super Xevious",
	"Ice Climber",
	"Dual Normal",
	"Dual Raid on Bungeling Bay",
	0
};

// VS PPU type
char* vsPPUList[] = {
	"RP2C03B",
	"RP2C03G",
	"RP2C04-0001",
	"RP2C04-0002",
	"RP2C04-0003",
	"RP2C04-0004",
	"RC2C03B",
	"RC2C03C",
	"RC2C05-01 ($2002 AND $??=$1B)",
	"RC2C05-02 ($2002 AND $3F=$3D)",
	"RC2C05-03 ($2002 AND $1F=$1C)",
	"RC2C05-04 ($2002 AND $1F=$1B)",
	"RC2C05-05 ($2002 AND $1F=$??)",
	"Reserved",
	"Reserved",
	"Reserved",
	0
};

// Extend console type
char* extConsoleList[] = {
	"Normal",
	"VS. System",
	"Playchoice 10",
	"Bit Corp. Creator",
	"V.R. Tech. VT01 monochrome",
	"V.R. Tech. VT01 red / cyan",
	"V.R. Tech. VT02",
	"V.R. Tech. VT03",
	"V.R. Tech. VT09",
	"V.R. Tech. VT32",
	"V.R. Tech. VT369",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	0
};

// Default input device list
char* inputDevList[] = {
	"Unspecified",
	"Standard NES / Famicom Controllers",
	"Four-score (NES)",
	"Four-score (Famicom)",
	"VS. System",
	"VS. System (controllers swapped)",
	"VS. Pinball (J)",
	"VS. Zapper",
	"Zapper",
	"Double Zappers",
	"Bandai Hyper Shot",
	"Power Pad Side A",
	"Power Pad Side B",
	"Family Trainer Side A",
	"Family Trainer Side B",
	"Arkanoid Paddle (NES)",
	"Arkanoid Paddle (Famicom)",
	"Double Arkanoid Paddle",
	"Konami Hyper Shot",
	"Pachinko",
	"Exciting Boxing Punching Bag",
	"Jissen Mahjong",
	"Party Tap",
	"Oeka Kids Tablet",
	"Barcode Reader",
	"Miracle Piano",
	"Pokkun Moguraa",
	"Top Rider",
	"Double-Fisted",
	"Famicom 3D",
	"Doremikko Keyboard",
	"R.O.B. Gyro Set",
	"Famicom Data Recorder",
	"ASCII Turbo File",
	"IGS Storage Battle Box",
	"Family BASIC Keyboard",
	"PEC-586 Keyboard",
	"Bit Corp. Keyboard",
	"Subor Keyboard",
	"Subor Keyboard + Mouse A",
	"Subor Keyboard + Mouse B",
	"SNES Mouse",
	"Multicart",
	"Double SNES controllers",
	"RacerMate Bicycle",
	"U-Force",
	"R.O.B. Stack-Up",
	0
};

static HFONT hFont, hNewFont;

char** dropDownList[] = {
	vsSysList, vsPPUList, extConsoleList, inputDevList, 0
};

int dropDownIdList[] = {
	IDC_VS_SYSTEM_COMBO,
	IDC_VS_PPU_COMBO,
	IDC_SYSTEM_EXTEND_COMBO,
	IDC_INPUT_DEVICE_COMBO,
	0
};

HWND hHeadEditor = NULL;

bool LoadHeader(HWND parent, iNES_HEADER* header)
{
	int error = 0;
	enum errors {
		OK,
		OPEN_FAILED,
		INVALID_HEADER,
		FDS_HEADER,
		UNIF_HEADER,
		NSF_HEADER//,
//		NSFE_HEADER,
//		NSF2_HEADER
	};

	FCEUFILE* fp = FCEU_fopen(LoadedRomFName, NULL, "rb", NULL);
	if (!GameInfo)
		strcpy(LoadedRomFName, fp->fullFilename.c_str());

	if (fp)
	{
		if (FCEU_fread(header, 1, sizeof(iNES_HEADER), fp) == sizeof(iNES_HEADER) && !memcmp(header, "NES\x1A", 4))
			header->cleanup();
		else if (!memcmp(header, "FDS\x1A", 4))
			error = errors::FDS_HEADER;
		else if (!memcmp(header, "UNIF", 4))
			error = errors::UNIF_HEADER;
		else if (!memcmp(header, "NESM", 4))
			error = errors::NSF_HEADER;
		/*		else if (!memcmp(header, "NSFE", 4))
					error = errors::NSFE_HEADER;
				else if (!memcmp(header, "NESM\2", 4))
					error = errors::NSF2_HEADER;
		*/		else
			error = errors::INVALID_HEADER;
		FCEU_fclose(fp);
	}
	else
		error = errors::OPEN_FAILED;

	if (error)
	{
		switch (error)
		{
		case errors::OPEN_FAILED:
		{
			char buf[1024];
			sprintf(buf, "Error opening %s!", LoadedRomFName);
			MessageBox(parent, buf, "iNES Header Editor", MB_OK | MB_ICONERROR);
			break;
		}
		case errors::INVALID_HEADER:
			MessageBox(parent, "Invalid iNES header.", "iNES Header Editor", MB_OK | MB_ICONERROR);
			break;
		case errors::FDS_HEADER:
			MessageBox(parent, "Editing header of an FDS file is not supported.", "iNES Header Editor", MB_OK | MB_ICONERROR);
			break;
		case errors::UNIF_HEADER:
			MessageBox(parent, "Editing header of a UNIF file is not supported.", "iNES Header Editor", MB_OK | MB_ICONERROR);
			break;
		case errors::NSF_HEADER:
			//			case errors::NSF2_HEADER:
			//			case errors::NSFE_HEADER:
			MessageBox(parent, "Editing header of an NSF file is not supported.", "iNES Header Editor", MB_OK | MB_ICONERROR);
			break;
		}
		return false;
	}

	return true;
}

HWND InitHeaderEditDialog(HWND hwnd, iNES_HEADER* header)
{
	hHeadEditor = hwnd;


	// these contols don't bother the standard
	EnableWindow(GetDlgItem(hwnd, IDC_INESHEADER_GROUP), TRUE);

	// Resotore button
	EnableWindow(GetDlgItem(hwnd, IDC_RESTORE_BUTTON), TRUE);
	// Save as... button
	EnableWindow(GetDlgItem(hwnd, IDSAVE), TRUE);

	// Version groupbox
	EnableWindow(GetDlgItem(hwnd, IDC_VERSION_GROUP), TRUE);
	// Standard
	EnableWindow(GetDlgItem(hwnd, IDC_RADIO_VERSION_STANDARD), TRUE);
	// iNES 2.0
	EnableWindow(GetDlgItem(hwnd, IDC_RADIO_VERSION_INES20), TRUE);

	// Mapper groupbox
	EnableWindow(GetDlgItem(hwnd, IDC_MAPPER_GROUP), TRUE);
	// Mapper#
	EnableWindow(GetDlgItem(hwnd, IDC_MAPPER_TEXT), TRUE);
	EnableWindow(GetDlgItem(hwnd, IDC_MAPPER_COMBO), TRUE);

	// PRG groupbox
	EnableWindow(GetDlgItem(hwnd, IDC_PRG_GROUP), TRUE);
	// PRG ROM
	EnableWindow(GetDlgItem(hwnd, IDC_PRGROM_TEXT), TRUE);
	EnableWindow(GetDlgItem(hwnd, IDC_PRGROM_COMBO), TRUE);
	// PRG RAM will be finally determined in ToggleUnofficialPrgRamPresent()
	// EnableWindow(GetDlgItem(hwnd, IDC_PRGRAM_TEXT), TRUE);
	// EnableWindow(GetDlgItem(hwnd, IDC_PRGRAM_COMBO), TRUE);
	// CHR ROM
	EnableWindow(GetDlgItem(hwnd, IDC_CHR_GROUP), TRUE);
	EnableWindow(GetDlgItem(hwnd, IDC_CHRROM_TEXT), TRUE);
	EnableWindow(GetDlgItem(hwnd, IDC_CHRROM_COMBO), TRUE);

	// Mirroring groupbox
	EnableWindow(GetDlgItem(hwnd, IDC_MIRRORING_GROUP), TRUE);
	// Horizontal
	EnableWindow(GetDlgItem(hwnd, IDC_RADIO_MIRR_HORIZONTAL), TRUE);
	// Vertical
	EnableWindow(GetDlgItem(hwnd, IDC_RADIO_MIRR_VERTICAL), TRUE);
	// Four-screen
	EnableWindow(GetDlgItem(hwnd, IDC_RADIO_MIRR_4SCREEN), TRUE);

	// Region Groupbox
	EnableWindow(GetDlgItem(hwnd, IDC_REGION_GROUP), TRUE);
	// NTSC
	EnableWindow(GetDlgItem(hwnd, IDC_RADIO_REGION_NTSC), TRUE);
	// PAL
	EnableWindow(GetDlgItem(hwnd, IDC_RADIO_REGION_PAL), TRUE);
	// Multiple will be finally determined in ToggleUnofficialPrgRamPresent()
	// EnableWindow(GetDlgItem(hwnd, IDC_RADIO_REGION_DUAL), TRUE);

	// Trainer
	EnableWindow(GetDlgItem(hwnd, IDC_CHECK_TRAINER), TRUE);

	// System Groupbox
	EnableWindow(GetDlgItem(hwnd, IDC_SYSTEM_GROUP), TRUE);
	// Normal
	EnableWindow(GetDlgItem(hwnd, IDC_RADIO_SYSTEM_NORMAL), TRUE);
	// VS
	EnableWindow(GetDlgItem(hwnd, IDC_RADIO_SYSTEM_VS), TRUE);
	// Playchoice-10 will be finally determined in ToggleUnofficialPropertiesEnabled()
	// EnableWindow(GetDlgItem(hwnd, IDC_RADIO_SYSTEM_PLAYCHOICE10), TRUE);



	// Limit text
	// Sub Mapper#
	SendDlgItemMessage(hwnd, IDC_SUBMAPPER_EDIT, EM_SETLIMITTEXT, 2, 0);
	// Misc. ROM(s)
	SendDlgItemMessage(hwnd, IDC_MISCELLANEOUS_ROMS_EDIT, EM_SETLIMITTEXT, 1, 0);

	// Assign ID to the sub edit control in these comboboxes
	// PRG ROM
	SetWindowLongPtr(GetWindow(GetDlgItem(hwnd, IDC_PRGROM_COMBO), GW_CHILD), GWL_ID, IDC_PRGROM_EDIT);
	// PRG RAM
	SetWindowLongPtr(GetWindow(GetDlgItem(hwnd, IDC_PRGRAM_COMBO), GW_CHILD), GWL_ID, IDC_PRGRAM_EDIT);
	// PRG NVRAM
	SetWindowLongPtr(GetWindow(GetDlgItem(hwnd, IDC_PRGNVRAM_COMBO), GW_CHILD), GWL_ID, IDC_PRGNVRAM_EDIT);
	// CHR ROM
	SetWindowLongPtr(GetWindow(GetDlgItem(hwnd, IDC_CHRROM_COMBO), GW_CHILD), GWL_ID, IDC_CHRROM_EDIT);
	// CHR RAM
	SetWindowLongPtr(GetWindow(GetDlgItem(hwnd, IDC_CHRRAM_COMBO), GW_CHILD), GWL_ID, IDC_CHRRAM_EDIT);
	// CHR NVRAM
	SetWindowLongPtr(GetWindow(GetDlgItem(hwnd, IDC_CHRNVRAM_COMBO), GW_CHILD), GWL_ID, IDC_CHRNVRAM_EDIT);


	// Change the default wndproc of these control to limit their text
	// PRG ROM
	DefaultEditCtrlProc = (WNDPROC)SetWindowLongPtr(GetDlgItem(GetDlgItem(hwnd, IDC_PRGROM_COMBO), IDC_PRGROM_EDIT), GWLP_WNDPROC, (LONG_PTR)FilterEditCtrlProc);
	// PRG RAM
	SetWindowLongPtr(GetDlgItem(GetDlgItem(hwnd, IDC_PRGRAM_COMBO), IDC_PRGRAM_EDIT), GWLP_WNDPROC, (LONG_PTR)FilterEditCtrlProc);
	// PRG NVRAM
	SetWindowLongPtr(GetDlgItem(GetDlgItem(hwnd, IDC_PRGNVRAM_COMBO), IDC_PRGNVRAM_EDIT), GWLP_WNDPROC, (LONG_PTR)FilterEditCtrlProc);
	// CHR ROM
	SetWindowLongPtr(GetDlgItem(GetDlgItem(hwnd, IDC_CHRROM_COMBO), IDC_CHRROM_EDIT), GWLP_WNDPROC, (LONG_PTR)FilterEditCtrlProc);
	// CHR RAM
	SetWindowLongPtr(GetDlgItem(GetDlgItem(hwnd, IDC_CHRRAM_COMBO), IDC_CHRRAM_EDIT), GWLP_WNDPROC, (LONG_PTR)FilterEditCtrlProc);
	// CHR NVRAM
	SetWindowLongPtr(GetDlgItem(GetDlgItem(hwnd, IDC_CHRNVRAM_COMBO), IDC_CHRNVRAM_EDIT), GWLP_WNDPROC, (LONG_PTR)FilterEditCtrlProc);


	ToggleINES20(hwnd, IsDlgButtonChecked(hwnd, IDC_RADIO_VERSION_INES20) == BST_CHECKED);

	char buf[256];
	for (int i = 0; dropDownIdList[i]; ++i)
		for (int j = 0; dropDownList[i][j]; ++j)
		{
			sprintf(buf, dropDownList[i] == inputDevList ? "$%02X %s" : "$%X %s", j, dropDownList[i][j]);
			SendDlgItemMessage(hwnd, dropDownIdList[i], CB_SETITEMDATA, SendDlgItemMessage(hwnd, dropDownIdList[i], CB_ADDSTRING, 0, (LPARAM)buf), j);
		}

	// Mapper#
	extern BMAPPINGLocal bmap[];
	for (int i = 0; bmap[i].init; ++i)
	{
		sprintf(buf, "%d %s", bmap[i].number, bmap[i].name);
		SendDlgItemMessage(hwnd, IDC_MAPPER_COMBO, CB_SETITEMDATA, SendDlgItemMessage(hwnd, IDC_MAPPER_COMBO, CB_ADDSTRING, 0, (LPARAM)buf), bmap[i].number);
	}

	// add usually used size strings
	strcpy(buf, "0B");
	SendDlgItemMessage(hwnd, IDC_PRGROM_COMBO, CB_SETITEMDATA, SendDlgItemMessage(hwnd, IDC_PRGROM_COMBO, CB_ADDSTRING, 0, (LPARAM)buf), 0);
	SendDlgItemMessage(hwnd, IDC_CHRROM_COMBO, CB_SETITEMDATA, SendDlgItemMessage(hwnd, IDC_CHRROM_COMBO, CB_ADDSTRING, 0, (LPARAM)buf), 0);
	SendDlgItemMessage(hwnd, IDC_PRGRAM_COMBO, CB_SETITEMDATA, SendDlgItemMessage(hwnd, IDC_PRGRAM_COMBO, CB_ADDSTRING, 0, (LPARAM)buf), 0);
	SendDlgItemMessage(hwnd, IDC_CHRRAM_COMBO, CB_SETITEMDATA, SendDlgItemMessage(hwnd, IDC_CHRRAM_COMBO, CB_ADDSTRING, 0, (LPARAM)buf), 0);
	SendDlgItemMessage(hwnd, IDC_PRGNVRAM_COMBO, CB_SETITEMDATA, SendDlgItemMessage(hwnd, IDC_PRGNVRAM_COMBO, CB_ADDSTRING, 0, (LPARAM)buf), 0);
	SendDlgItemMessage(hwnd, IDC_CHRNVRAM_COMBO, CB_SETITEMDATA, SendDlgItemMessage(hwnd, IDC_CHRNVRAM_COMBO, CB_ADDSTRING, 0, (LPARAM)buf), 0);

	int size = 128;
	while (size <= 2048 * 1024)
	{
		if (size >= 1024 << 3)
		{
			// The size of CHR ROM must be multiple of 8KB
			sprintf(buf, "%dKB", size / 1024);
			SendDlgItemMessage(hwnd, IDC_CHRROM_COMBO, CB_SETITEMDATA, SendDlgItemMessage(hwnd, IDC_CHRROM_COMBO, CB_ADDSTRING, 0, (LPARAM)buf), size);

			// The size of PRG ROM must be multiple of 16KB
			if (size >= 1024 * 16)
			{
				// PRG ROM
				sprintf(buf, "%dKB", size / 1024);
				SendDlgItemMessage(hwnd, IDC_PRGROM_COMBO, CB_SETITEMDATA, SendDlgItemMessage(hwnd, IDC_PRGROM_COMBO, CB_ADDSTRING, 0, (LPARAM)buf), size);
			}
		}

		if (size >= 1024)
			sprintf(buf, "%dKB", size / 1024);
		else
			sprintf(buf, "%dB", size);

		SendDlgItemMessage(hwnd, IDC_PRGRAM_COMBO, CB_SETITEMDATA, SendDlgItemMessage(hwnd, IDC_PRGRAM_COMBO, CB_ADDSTRING, 0, (LPARAM)buf), size);
		SendDlgItemMessage(hwnd, IDC_CHRRAM_COMBO, CB_SETITEMDATA, SendDlgItemMessage(hwnd, IDC_CHRRAM_COMBO, CB_ADDSTRING, 0, (LPARAM)buf), size);
		SendDlgItemMessage(hwnd, IDC_PRGNVRAM_COMBO, CB_SETITEMDATA, SendDlgItemMessage(hwnd, IDC_PRGNVRAM_COMBO, CB_ADDSTRING, 0, (LPARAM)buf), size);
		SendDlgItemMessage(hwnd, IDC_CHRNVRAM_COMBO, CB_SETITEMDATA, SendDlgItemMessage(hwnd, IDC_CHRNVRAM_COMBO, CB_ADDSTRING, 0, (LPARAM)buf), size);

		size <<= 1;
	}


	SetHeaderData(hwnd, header);

	return hwnd;

}

void ToggleINES20(HWND hwnd, bool ines20)
{
	// only ines 2.0 has these values
	// these are always have when in 2.0
	// Submapper#
	EnableWindow(GetDlgItem(hwnd, IDC_SUBMAPPER_TEXT), ines20);
	EnableWindow(GetDlgItem(hwnd, IDC_SUBMAPPER_EDIT), ines20);
	// PRG NVRAM
	EnableWindow(GetDlgItem(hwnd, IDC_PRGNVRAM_COMBO), ines20);
	EnableWindow(GetDlgItem(hwnd, IDC_PRGNVRAM_TEXT), ines20);
	// CHR RAM
	EnableWindow(GetDlgItem(hwnd, IDC_CHRRAM_COMBO), ines20);
	EnableWindow(GetDlgItem(hwnd, IDC_CHRRAM_TEXT), ines20);
	// CHR NVRAM
	EnableWindow(GetDlgItem(hwnd, IDC_CHRNVRAM_COMBO), ines20);
	EnableWindow(GetDlgItem(hwnd, IDC_CHRNVRAM_TEXT), ines20);
	// Dendy in Region Groupbox
	EnableWindow(GetDlgItem(hwnd, IDC_RADIO_REGION_DENDY), ines20);
	// Multiple in Regtion Groupbox
	EnableWindow(GetDlgItem(hwnd, IDC_RADIO_REGION_DUAL), ines20);
	// Extend in System Groupbox
	EnableWindow(GetDlgItem(hwnd, IDC_RADIO_SYSTEM_EXTEND), ines20);
	// Input Device
	EnableWindow(GetDlgItem(hwnd, IDC_INPUT_DEVICE_COMBO), ines20);
	EnableWindow(GetDlgItem(hwnd, IDC_INPUT_DEVICE_TEXT), ines20);
	// Miscellaneous ROMs
	EnableWindow(GetDlgItem(hwnd, IDC_MISCELLANEOUS_ROMS_EDIT), ines20);
	EnableWindow(GetDlgItem(hwnd, IDC_MISCELLANEOUS_ROMS_TEXT), ines20);

	// enable extend dialog only when ines 2.0 and extend button is checked
	ToggleExtendSystemList(hwnd, ines20 && IsDlgButtonChecked(hwnd, IDC_RADIO_SYSTEM_EXTEND) == BST_CHECKED);
	// enable vs dialog only when ines 2.0 and vs button is checked
	ToggleVSSystemGroup(hwnd, ines20 && IsDlgButtonChecked(hwnd, IDC_RADIO_SYSTEM_VS) == BST_CHECKED);


	// hide "Battery / PRG-NVRAM" checkbox in ines 2.0 and show PRG NVRAM combo box, negative on the contary, because ines 1.0 has nowhere to set actural size, it can only determin it exists or not.
	ShowWindow(GetDlgItem(hwnd, IDC_CHECK_BATTERYNVRAM), ines20 ? SW_HIDE : SW_SHOW);
	ShowWindow(GetDlgItem(hwnd, IDC_PRGNVRAM_COMBO), ines20 ? SW_SHOW : SW_HIDE);
	ShowWindow(GetDlgItem(hwnd, IDC_PRGNVRAM_TEXT), ines20 ? SW_SHOW : SW_HIDE);

	EnableWindow(GetDlgItem(hwnd, IDC_CHECK_BATTERYNVRAM), TRUE);
	EnableWindow(GetDlgItem(hwnd, IDC_PRGNVRAM_COMBO), TRUE);
	EnableWindow(GetDlgItem(hwnd, IDC_PRGNVRAM_TEXT), TRUE);


	// 10th byte for the unofficial ines properties
	// Unofficial Properties
	EnableWindow(GetDlgItem(hwnd, IDC_CHECK_UNOFFICIAL), !ines20);
	ToggleUnofficialPropertiesEnabled(hwnd, ines20, IsDlgButtonChecked(hwnd, IDC_CHECK_UNOFFICIAL) == BST_CHECKED);

	// ines 1.0 doesn't support Dendy region, switch it to PAL
	if (!ines20 && IsDlgButtonChecked(hwnd, IDC_RADIO_REGION_DENDY) == BST_CHECKED)
		CheckRadioButton(hwnd, IDC_RADIO_REGION_NTSC, IDC_RADIO_REGION_DENDY, IDC_RADIO_REGION_PAL);
	// ines 1.0 doesn't support extend system, switch it to normal
	if (!ines20 && IsDlgButtonChecked(hwnd, IDC_RADIO_SYSTEM_EXTEND) == BST_CHECKED)
		CheckRadioButton(hwnd, IDC_RADIO_SYSTEM_NORMAL, IDC_RADIO_SYSTEM_EXTEND, IDC_RADIO_SYSTEM_NORMAL);

}

void ToggleExtendSystemList(HWND hwnd, bool enable)
{
	// Extend combo box only enabled when in iNES 2.0 and Extend System was chosen
	// Extend combo box
	EnableWindow(GetDlgItem(hwnd, IDC_EXTEND_SYSTEM_GROUP), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_SYSTEM_EXTEND_COMBO), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_EXTEND_SYSTEM_TEXT), enable);
}

void ToggleVSSystemGroup(HWND hwnd, bool enable)
{
	// VS System Groupbox only enabled when in iNES 2.0 and VS System in System groupbox is chosen
	// VS System Groupbox
	EnableWindow(GetDlgItem(hwnd, IDC_VS_SYSTEM_GROUP), enable);
	// System
	EnableWindow(GetDlgItem(hwnd, IDC_VS_SYSTEM_COMBO), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_VS_SYSTEM_TEXT), enable);
	// PPU
	EnableWindow(GetDlgItem(hwnd, IDC_VS_PPU_COMBO), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_VS_PPU_TEXT), enable);
}

void ToggleUnofficialPropertiesEnabled(HWND hwnd, bool ines20, bool check)
{
	bool sub_enable = !ines20 && check;

	// Unofficial Properties only available in ines 1.0
	EnableWindow(GetDlgItem(hwnd, IDC_UNOFFICIAL_GROUP), sub_enable);
	EnableWindow(GetDlgItem(hwnd, IDC_CHECK_UNOFFICIAL_PRGRAM), sub_enable);
	EnableWindow(GetDlgItem(hwnd, IDC_CHECK_UNOFFICIAL_EXTRA_REGION), sub_enable);
	EnableWindow(GetDlgItem(hwnd, IDC_CHECK_UNOFFICIAL_BUS_CONFLICT), sub_enable);
	// when unofficial properties is enabled or in ines 2.0 standard, Playchoice-10 in System groupbox is available
	EnableWindow(GetDlgItem(hwnd, IDC_RADIO_SYSTEM_PLAYCHOICE10), ines20 || sub_enable);

	ToggleUnofficialPrgRamPresent(hwnd, ines20, check, IsDlgButtonChecked(hwnd, IDC_CHECK_UNOFFICIAL_PRGRAM) == BST_CHECKED);
	ToggleUnofficialExtraRegionCode(hwnd, ines20, check, IsDlgButtonChecked(hwnd, IDC_CHECK_UNOFFICIAL_EXTRA_REGION) == BST_CHECKED);

	// Playchoice-10 is not available in ines 1.0 and unofficial is not checked, switch back to normal
	if (!ines20 && !check && IsDlgButtonChecked(hwnd, IDC_RADIO_SYSTEM_PLAYCHOICE10) == BST_CHECKED)
		CheckRadioButton(hwnd, IDC_RADIO_SYSTEM_NORMAL, IDC_RADIO_SYSTEM_EXTEND, IDC_RADIO_SYSTEM_NORMAL);
}

void ToggleUnofficialExtraRegionCode(HWND hwnd, bool ines20, bool unofficial_check, bool check)
{
	// The unofficial byte to determine whether multiple region is valid

	// Multiple in Region Groupbox
	EnableWindow(GetDlgItem(hwnd, IDC_RADIO_REGION_DUAL), ines20 || unofficial_check && check);

	// Dual region is not avalable when in ines 1.0 and extra region in unofficial is not checked, switch it back to NTSC 
	if (!ines20 && (!unofficial_check || !check) && IsDlgButtonChecked(hwnd, IDC_RADIO_REGION_DUAL) == BST_CHECKED)
		CheckRadioButton(hwnd, IDC_RADIO_REGION_NTSC, IDC_RADIO_REGION_DENDY, IDC_RADIO_REGION_NTSC);
}

void ToggleUnofficialPrgRamPresent(HWND hwnd, bool ines20, bool unofficial_check, bool check) {
	// The unofficial byte to determine wheter PRGRAM exists
	// PRG RAM
	bool enable = ines20 || !unofficial_check || check;
	// When unofficial 10th byte was not set, the PRG RAM is defaultly enabled
	EnableWindow(GetDlgItem(hwnd, IDC_PRGRAM_TEXT), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_PRGRAM_COMBO), enable);
}

INT_PTR CALLBACK HeaderEditorProc(HWND hDlg, UINT uMsg, WPARAM wP, LPARAM lP)
{

	static iNES_HEADER* header;

	switch (uMsg) {
	case WM_INITDIALOG:
	{
		header = (iNES_HEADER*)lP;
		if (!header)
			goto wm_close;
		else
			InitHeaderEditDialog(hDlg, header);

		// Put the window aside the main window when in game.
		if (GameInfo)
			CalcSubWindowPos(hDlg, NULL);

		SetDlgItemText(hDlg, IDC_ROM_FILE_EDIT, LoadedRomFName);

		char textHeader[16 * 3];
		sprintf(textHeader, "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X", header->ID[0], header->ID[1], header->ID[2], header->ID[3], header->ROM_size, header->VROM_size, header->ROM_size, header->ROM_type2, header->ROM_type3, header->Upper_ROM_VROM_size, header->RAM_size, header->VRAM_size, header->TV_system, header->VS_hardware, header->reserved[0], header->reserved[1]);
		SetDlgItemText(hDlg, IDC_HEX_HEADER_EDIT, textHeader);

		hFont = (HFONT)SendMessage(hDlg, WM_GETFONT, 0, 0);
		LOGFONT lf;
		GetObject(hFont, sizeof(LOGFONT), &lf);
		strcpy(lf.lfFaceName, "Courier New");
		hNewFont = CreateFontIndirect(&lf);
		SendDlgItemMessage(hDlg, IDC_HEX_HEADER_EDIT, WM_SETFONT, (WPARAM)hNewFont, FALSE);
	}
	break;
	case WM_COMMAND:
		switch (HIWORD(wP))
		{
			case EN_UPDATE:
				switch (LOWORD(wP))
				{
					case IDC_SUBMAPPER_EDIT:
					case IDC_MISCELLANEOUS_ROMS_EDIT:
						WriteHeaderData(hDlg);
				}
				break;
			case CBN_EDITUPDATE:
			case CBN_SELCHANGE:
				switch (LOWORD(wP))
				{
					case IDC_MAPPER_COMBO:
					case IDC_PRGROM_COMBO:
					case IDC_PRGRAM_COMBO:
					case IDC_PRGNVRAM_COMBO:
					case IDC_CHRROM_COMBO:
					case IDC_CHRRAM_COMBO:
					case IDC_CHRNVRAM_COMBO:
					case IDC_VS_SYSTEM_COMBO:
					case IDC_VS_PPU_COMBO:
					case IDC_SYSTEM_EXTEND_COMBO:
					case IDC_INPUT_DEVICE_COMBO:
						WriteHeaderData(hDlg);
				}
				break;
			case BN_CLICKED:
			{
				switch (LOWORD(wP))
				{
					case IDC_RADIO_VERSION_STANDARD:
						ToggleINES20(hDlg, false);
						WriteHeaderData(hDlg);
						break;
					case IDC_RADIO_VERSION_INES20:
						ToggleINES20(hDlg, true);
						WriteHeaderData(hDlg);
						break;
					case IDC_RADIO_SYSTEM_NORMAL:
					case IDC_RADIO_SYSTEM_PLAYCHOICE10:
					case IDC_RADIO_SYSTEM_EXTEND:
						ToggleExtendSystemList(hDlg, IsDlgButtonChecked(hDlg, IDC_RADIO_SYSTEM_EXTEND) == BST_CHECKED);
						WriteHeaderData(hDlg);
					case IDC_RADIO_SYSTEM_VS:
						// Both ines 1.0 and 2.0 can trigger VS System, but only 2.0 enables the extra detailed properties
						ToggleVSSystemGroup(hDlg, IsDlgButtonChecked(hDlg, IDC_RADIO_VERSION_INES20) == BST_CHECKED && IsDlgButtonChecked(hDlg, IDC_RADIO_SYSTEM_VS) == BST_CHECKED);
						WriteHeaderData(hDlg);
						break;
					case IDC_CHECK_UNOFFICIAL:
						ToggleUnofficialPropertiesEnabled(hDlg, false, IsDlgButtonChecked(hDlg, IDC_CHECK_UNOFFICIAL) == BST_CHECKED);
						WriteHeaderData(hDlg);
						break;
					case IDC_CHECK_UNOFFICIAL_PRGRAM:
						ToggleUnofficialPrgRamPresent(hDlg, false, true, IsDlgButtonChecked(hDlg, IDC_CHECK_UNOFFICIAL_PRGRAM) == BST_CHECKED);
						WriteHeaderData(hDlg);
						break;
					case IDC_CHECK_UNOFFICIAL_EXTRA_REGION:
						ToggleUnofficialExtraRegionCode(hDlg, false, true, IsDlgButtonChecked(hDlg, IDC_CHECK_UNOFFICIAL_EXTRA_REGION) == BST_CHECKED);
						WriteHeaderData(hDlg);
						break;
					case IDC_RESTORE_BUTTON:
						SetHeaderData(hDlg, header);
						char textHeader[16 * 3];
						sprintf(textHeader, "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X", header->ID[0], header->ID[1], header->ID[2], header->ID[3], header->ROM_size, header->VROM_size, header->ROM_size, header->ROM_type2, header->ROM_type3, header->Upper_ROM_VROM_size, header->RAM_size, header->VRAM_size, header->TV_system, header->VS_hardware, header->reserved[0], header->reserved[1]);
						SetDlgItemText(hDlg, IDC_HEX_HEADER_EDIT, textHeader);
						break;
					case IDSAVE:
					{
						iNES_HEADER newHeader;
						if (WriteHeaderData(hDlg, &newHeader))
						{
							char path[4096] = { 0 };
							if (ShowINESFileBox(hDlg, path, &newHeader))
								SaveINESFile(hDlg, path, &newHeader);
						}
					}
					break;
					case IDC_CHECK_BATTERYNVRAM:
					case IDC_RADIO_MIRR_HORIZONTAL:
					case IDC_RADIO_MIRR_VERTICAL:
					case IDC_RADIO_MIRR_4SCREEN:
					case IDC_RADIO_REGION_NTSC:
					case IDC_RADIO_REGION_PAL:
					case IDC_RADIO_REGION_DENDY:
					case IDC_RADIO_REGION_DUAL:
					case IDC_CHECK_TRAINER:
					case IDC_CHECK_UNOFFICIAL_BUS_CONFLICT:
						WriteHeaderData(hDlg);
						break;
					case IDCLOSE:
						goto wm_close;
					}
			}
		}
		break;
	case WM_CLOSE:
	case WM_QUIT:
	wm_close:
		if (GameInfo)
			DestroyWindow(hDlg);
		else
		{
			EndDialog(hDlg, 0);
			LoadedRomFName[0] = 0;
		}
		DeleteObject(hFont);
		DeleteObject(hNewFont);
		hFont = NULL;
		hNewFont = NULL;
		break;
	case WM_DESTROY:
		hHeadEditor = NULL;
		free(header);
		break;
	}
	return false;
}

void DoHeadEdit()
{
	if (hHeadEditor)
	{
		ShowWindow(hHeadEditor, SW_SHOWNORMAL);
		SetForegroundWindow(hHeadEditor);
	}
	else
	{
		iNES_HEADER* header = (iNES_HEADER*)calloc(1, sizeof(iNES_HEADER));
		if (GameInfo)
		{
			if (LoadHeader(hAppWnd, header))
				CreateDialogParam(fceu_hInstance, MAKEINTRESOURCE(IDD_EDIT_HEADER), hAppWnd, HeaderEditorProc, (LPARAM)header);
			else
				free(header);
		}
		else {
			// temporarily borrow LoadedRomFName, when no game is loaded, it is unused.
			LoadedRomFName[0] = 0;
			if (ShowINESFileBox(hAppWnd) && LoadHeader(hAppWnd, header))
				DialogBoxParam(fceu_hInstance, MAKEINTRESOURCE(IDD_EDIT_HEADER), hAppWnd, HeaderEditorProc, (LPARAM)header);
			else
				free(header);
		}
	}
}

void SetHeaderData(HWND hwnd, iNES_HEADER* header) {

	// Temporary buffers
	char buf[256];

	bool ines20 = (header->ROM_type2 & 0xC) == 8;
	bool unofficial = false;

	// Check iNES 2.0
	CheckRadioButton(hwnd, IDC_RADIO_VERSION_STANDARD, IDC_RADIO_VERSION_INES20, ines20 ? IDC_RADIO_VERSION_INES20 : IDC_RADIO_VERSION_STANDARD);

	// Mapper#
	int mapper = header->ROM_type >> 4 | header->ROM_type2 & 0xF0;
	if (ines20)
		mapper |= (header->ROM_type3 & 0x0F) << 8;
	sprintf(buf, "%d ", mapper);
	if (SendDlgItemMessage(hwnd, IDC_MAPPER_COMBO, CB_SELECTSTRING, 0, (LPARAM)buf) == CB_ERR)
		SetDlgItemText(hwnd, IDC_MAPPER_COMBO, buf);

	// Sub Mapper
	sprintf(buf, "%d", ines20 ? header->ROM_type3 >> 4 : 0);
	SetDlgItemText(hwnd, IDC_SUBMAPPER_EDIT, buf);

	// PRG ROM
	strcpy(buf, "0B");
	int prg_rom = header->ROM_size;
	if (ines20) {
		if ((header->Upper_ROM_VROM_size & 0xF) == 0xF)
			prg_rom = pow(2, header->ROM_size >> 2) * ((header->ROM_size & 3) * 2 + 1);
		else {
			prg_rom |= (header->Upper_ROM_VROM_size & 0xF) << 8;
			prg_rom *= 1024 * 16;
		}
	}
	else
		prg_rom *= 1024 * 16;

	if (prg_rom < 1024 || prg_rom % 1024 != 0)
		sprintf(buf, "%dB", prg_rom);
	else
		sprintf(buf, "%dKB", prg_rom / 1024);

	if (SendDlgItemMessage(hwnd, IDC_PRGROM_COMBO, CB_SELECTSTRING, 0, (LPARAM)buf) == CB_ERR)
		SetDlgItemText(hwnd, IDC_PRGROM_COMBO, buf);

	// PRG RAM
	strcpy(buf, "0B");
	if (ines20)
	{
		int shift = header->RAM_size & 0xF;
		if (shift)
		{
			int prg_ram = 64 << shift;
			if (prg_ram >= 1024)
				sprintf(buf, "%dKB", prg_ram / 1024);
			else sprintf(buf, "%dB", prg_ram);
		}

	}
	else
	{
		if (!(header->RAM_size & 0x10) && header->ROM_type3)
			sprintf(buf, "%dKB", header->ROM_type3 ? 1 : header->ROM_type3 * 8);
	}
	if (SendDlgItemMessage(hwnd, IDC_PRGRAM_COMBO, CB_SELECTSTRING, 0, (LPARAM)buf) == CB_ERR)
		SetDlgItemText(hwnd, IDC_PRGRAM_COMBO, buf);

	// PRG NVRAM
	strcpy(buf, "0B");
	if (ines20)
	{
		int shift = header->RAM_size >> 4;
		if (shift)
		{
			int prg_nvram = 64 << shift;
			if (prg_nvram >= 1024)
				sprintf(buf, "%dKB", prg_nvram / 1024);
			else sprintf(buf, "%dB", prg_nvram);
		}
	}
	else
		CheckDlgButton(hwnd, IDC_CHECK_BATTERYNVRAM, header->ROM_type & 0x2 ? BST_CHECKED : BST_UNCHECKED);

	if (SendDlgItemMessage(hwnd, IDC_PRGNVRAM_COMBO, CB_SELECTSTRING, 0, (LPARAM)buf) == CB_ERR)
		SetDlgItemText(hwnd, IDC_PRGNVRAM_COMBO, buf);



	// CHR ROM
	strcpy(buf, "0B");
	int chr_rom = header->VROM_size;
	if (ines20)
	{
		if ((header->Upper_ROM_VROM_size & 0xF0) == 0xF0)
			chr_rom = pow(2, header->VROM_size >> 2) * (((header->VROM_size & 3) * 2) + 1);
		else
		{
			chr_rom |= (header->Upper_ROM_VROM_size & 0xF0) << 4;
			chr_rom *= 8 * 1024;
		}
	}
	else
		chr_rom *= 8 * 1024;

	if (chr_rom < 1024 || chr_rom % 1024 != 0)
		sprintf(buf, "%dB", chr_rom);
	else
		sprintf(buf, "%dKB", chr_rom / 1024);
	if (SendDlgItemMessage(hwnd, IDC_CHRROM_COMBO, CB_SELECTSTRING, 0, (LPARAM)buf) == CB_ERR)
		SetDlgItemText(hwnd, IDC_CHRROM_COMBO, buf);

	// CHR RAM
	sprintf(buf, "0B");
	if (ines20)
	{
		int shift = header->VRAM_size & 0xF;
		if (shift)
		{
			int chr_ram = 64 << shift;
			if (chr_ram >= 1024)
				sprintf(buf, "%dKB", chr_ram / 1024);
			else sprintf(buf, "%dB", chr_ram);
		}
	}
	if (SendDlgItemMessage(hwnd, IDC_CHRRAM_COMBO, CB_SELECTSTRING, 0, (LPARAM)buf) == CB_ERR)
		SetDlgItemText(hwnd, IDC_CHRRAM_COMBO, buf);

	// CHR NVRAM
	sprintf(buf, "0B");
	if (ines20)
	{
		int shift = header->VRAM_size >> 4;
		if (shift)
		{
			int chr_nvram = 64 << shift;
			if (chr_nvram >= 1024)
				sprintf(buf, "%dKB", chr_nvram / 1024);
			else sprintf(buf, "%dB", chr_nvram);
		}
	}
	if (SendDlgItemMessage(hwnd, IDC_CHRNVRAM_COMBO, CB_SELECTSTRING, 0, (LPARAM)buf) == CB_ERR)
		SetDlgItemText(hwnd, IDC_CHRNVRAM_COMBO, buf);

	// Mirroring
	CheckRadioButton(hwnd, IDC_RADIO_MIRR_HORIZONTAL, IDC_RADIO_MIRR_4SCREEN, header->ROM_type & 8 ? IDC_RADIO_MIRR_4SCREEN : header->ROM_type & 1 ? IDC_RADIO_MIRR_VERTICAL : IDC_RADIO_MIRR_HORIZONTAL);

	// Region
	if (ines20)
	{
		int region = header->TV_system & 3;
		switch (region) {
		case 0:
			// NTSC
			CheckRadioButton(hwnd, IDC_RADIO_REGION_NTSC, IDC_RADIO_REGION_DENDY, IDC_RADIO_REGION_NTSC);
			break;
		case 1:
			// PAL
			CheckRadioButton(hwnd, IDC_RADIO_REGION_NTSC, IDC_RADIO_REGION_DENDY, IDC_RADIO_REGION_PAL);
			break;
		case 2:
			// Dual
			CheckRadioButton(hwnd, IDC_RADIO_REGION_NTSC, IDC_RADIO_REGION_DENDY, IDC_RADIO_REGION_DUAL);
			break;
		case 3:
			// Dendy
			CheckRadioButton(hwnd, IDC_RADIO_REGION_NTSC, IDC_RADIO_REGION_DENDY, IDC_RADIO_REGION_DENDY);
		}
	}
	else {
		// Only the unofficial 10th byte has a dual region, we must check it first.
		int region = header->RAM_size & 3;
		if (region == 3 || region == 1)
		{
			// Check the unofficial checkmark and the region code
			CheckRadioButton(hwnd, IDC_RADIO_REGION_NTSC, IDC_RADIO_REGION_DENDY, IDC_RADIO_REGION_DUAL);
			unofficial = true;
		}
		else
			// When the region in unofficial byte is inconsistent with the official byte, based on the official byte
			CheckRadioButton(hwnd, IDC_RADIO_REGION_NTSC, IDC_RADIO_REGION_DENDY, header->Upper_ROM_VROM_size & 1 ? IDC_RADIO_REGION_PAL : IDC_RADIO_REGION_NTSC);
	}

	// System
	int system = header->ROM_type2 & 3;
	switch (system)
	{
	default:
		// Normal
		case 0:
			CheckRadioButton(hwnd, IDC_RADIO_SYSTEM_NORMAL, IDC_RADIO_SYSTEM_EXTEND, IDC_RADIO_SYSTEM_NORMAL);
			break;
		// VS. System
		case 1:
			CheckRadioButton(hwnd, IDC_RADIO_SYSTEM_NORMAL, IDC_RADIO_SYSTEM_EXTEND, IDC_RADIO_SYSTEM_VS);
			break;
		// PlayChoice-10
		case 2:
			CheckRadioButton(hwnd, IDC_RADIO_SYSTEM_NORMAL, IDC_RADIO_SYSTEM_EXTEND, IDC_RADIO_SYSTEM_PLAYCHOICE10);
		// PlayChoice-10 is an unofficial setting for ines 1.0
		unofficial = !ines20;
		break;
		// Extend
		case 3:
			// The 7th byte is different between ines 1.0 and 2.0, so we need to check it
			if (ines20) CheckRadioButton(hwnd, IDC_RADIO_SYSTEM_NORMAL, IDC_RADIO_SYSTEM_EXTEND, IDC_RADIO_SYSTEM_EXTEND);
	}

	// it's quite ambiguous to put them here, but it's better to have a default selection than leave the dropdown blank, because user might switch to ines 2.0 anytime
	// Hardware type
	int hardware = header->VS_hardware >> 4;
	if (SendDlgItemMessage(hwnd, IDC_VS_SYSTEM_COMBO, CB_SETCURSEL, hardware, 0) == CB_ERR)
	{
		sprintf(buf, "$%X", hardware);
		SetDlgItemText(hwnd, IDC_VS_SYSTEM_COMBO, buf);
	}
	// PPU type
	int ppu = header->VS_hardware & 0xF;
	if (SendDlgItemMessage(hwnd, IDC_VS_PPU_COMBO, CB_SETCURSEL, ppu, 0) == CB_ERR)
	{
		sprintf(buf, "$%X", ppu);
		SetDlgItemText(hwnd, IDC_VS_SYSTEM_COMBO, buf);
	}
	// Extend Console
	if (SendDlgItemMessage(hwnd, IDC_SYSTEM_EXTEND_COMBO, CB_SETCURSEL, ppu, 0) == CB_ERR)
	{
		sprintf(buf, "$%X", ppu);
		SetDlgItemText(hwnd, IDC_VS_SYSTEM_COMBO, buf);
	}

	// Input Device:
	int input = header->reserved[1] & 0x1F;
	if (SendDlgItemMessage(hwnd, IDC_INPUT_DEVICE_COMBO, CB_SETCURSEL, input, 0) == CB_ERR)
	{
		sprintf(buf, "$%02X", input);
		SetDlgItemText(hwnd, IDC_INPUT_DEVICE_COMBO, buf);
	}

	// Miscellaneous ROM Area(s)
	sprintf(buf, "%d", header->reserved[0] & 3);
	SetDlgItemText(hwnd, IDC_MISCELLANEOUS_ROMS_EDIT, buf);

	// Trainer
	CheckDlgButton(hwnd, IDC_CHECK_TRAINER, header->ROM_type & 4 ? BST_CHECKED : BST_UNCHECKED);

	// Unofficial Properties Checkmark
	CheckDlgButton(hwnd, IDC_CHECK_UNOFFICIAL, unofficial ? BST_CHECKED : BST_UNCHECKED);

	// Switch the UI to the proper version
	ToggleINES20(hwnd, ines20);
}

bool WriteHeaderData(HWND hwnd, iNES_HEADER* header)
{
	// Temporary buffers
	char buf[256];

	iNES_HEADER _header;
	memset(&_header, 0, sizeof(iNES_HEADER));

	// Check iNES 2.0
	bool ines20 = IsDlgButtonChecked(hwnd, IDC_RADIO_VERSION_INES20) == BST_CHECKED;
	// iNES 1.0 unofficial byte available
	bool unofficial = !ines20 && IsDlgButtonChecked(hwnd, IDC_CHECK_UNOFFICIAL) == BST_CHECKED;

	// iNES 2.0 signature
	if (ines20)
		_header.ROM_type2 |= 8;

	// Mapper
	int mapper;
	if (!GetComboBoxListItemData(hwnd, IDC_MAPPER_COMBO, &mapper, buf, header))
	{
		if (header) {
			MessageBox(hwnd, "The mapper# you have entered is invalid. Please enter a decimal number or select an item from the dropdown list.", "Error", MB_OK | MB_ICONERROR);
			SetFocus(GetDlgItem(hwnd, IDC_MAPPER_COMBO));
			SendDlgItemMessage(hwnd, IDC_MAPPER_COMBO, EM_SETSEL, 0, -1);
		}
		else
			SetDlgItemText(hwnd, IDC_HEX_HEADER_EDIT, "");
		return false;
	}

	if (mapper < 4096)
	{
		_header.ROM_type |= (mapper & 0xF) << 4;
		_header.ROM_type2 |= (mapper & 0xF0);
		if (mapper >= 256)
		{
			if (ines20)
				_header.ROM_type3 |= mapper >> 8;
			else
			{
				if (header) {
					sprintf(buf, "Mapper# should be less than %d in iNES %d.0 format.", 256, 1);
					MessageBox(hwnd, buf, "Error", MB_OK | MB_ICONERROR);
					SetFocus(GetDlgItem(hwnd, IDC_MAPPER_COMBO));
					SendDlgItemMessage(hwnd, IDC_MAPPER_COMBO, EM_SETSEL, 0, -1);
				}
				else
					SetDlgItemText(hwnd, IDC_HEX_HEADER_EDIT, "");

				return false;
			}
		}
	}
	else {
		if (header) {
			sprintf(buf, "Mapper# should be less than %d in iNES %d.0 format.", 4096, 2);
			MessageBox(hwnd, buf, "Error", MB_OK | MB_ICONERROR);
			SetFocus(GetDlgItem(hwnd, IDC_MAPPER_COMBO));
			SendDlgItemMessage(hwnd, IDC_MAPPER_COMBO, EM_SETSEL, 0, -1);
		}
		else
			SetDlgItemText(hwnd, IDC_HEX_HEADER_EDIT, "");

		return false;
	}

	// Sub mapper
	if (ines20) {
		GetDlgItemText(hwnd, IDC_SUBMAPPER_EDIT, buf, 256);
		int submapper;
		if (sscanf(buf, "%d", &submapper) > 0)
		{
			if (submapper < 16)
				_header.ROM_type3 |= submapper << 4;
			else
			{
				if (header) {
					MessageBox(hwnd, "The sub mapper# should less than 16 in iNES 2.0 format.", "Error", MB_OK | MB_ICONERROR);
					SetFocus(GetDlgItem(hwnd, IDC_SUBMAPPER_EDIT));
					SendDlgItemMessage(hwnd, IDC_SUBMAPPER_EDIT, EM_SETSEL, 0, -1);
				}
				else
					SetDlgItemText(hwnd, IDC_HEX_HEADER_EDIT, "");

				return false;
			}
		}
		else
		{
			if (header) {
				MessageBox(hwnd, "The sub mapper# you have entered is invalid. Please enter a decimal number.", "Error", MB_OK | MB_ICONERROR);
				SetFocus(GetDlgItem(hwnd, IDC_SUBMAPPER_EDIT));
				SendDlgItemMessage(hwnd, IDC_SUBMAPPER_EDIT, EM_SETSEL, 0, -1);
			}
			else
				SetDlgItemText(hwnd, IDC_HEX_HEADER_EDIT, "");

			return false;
		}
	}

	// PRG ROM
	int prg_rom;
	if (GetComboBoxByteSize(hwnd, IDC_PRGROM_COMBO, &prg_rom, header) == 0)
	{
		// max value which a iNES 2.0 header can hold
		if (prg_rom < 16 * 1024 * 0xEFF)
		{
			// try to fit the irregular value with the alternative way
			if (prg_rom % (16 * 1024) != 0)
			{
				if (ines20)
				{
					// try to calculate the nearest value
					bool fit = false;
					int result = 0x7FFFFFFF;
					for (int multiplier = 0; multiplier < 4; ++multiplier)
					{
						for (int exponent = 0; exponent < 64; ++exponent)
						{
							int new_result = pow(2, exponent) * (multiplier * 2 + 1);
							if (new_result == prg_rom)
							{
								_header.Upper_ROM_VROM_size |= 0xF;
								_header.ROM_size |= exponent << 2;
								_header.ROM_size |= multiplier & 3;
								fit = true;
								break;
							}
							if (new_result > prg_rom && result > new_result)
								result = new_result;
						}
						if (fit) break;
					}

					if (!fit)
					{
						if (header) {
							int result10 = (prg_rom / 16 / 1024 + 1) * 16 * 1024;
							if (result10 < result)
								result = result10;
							char buf2[64];
							if (result % 1024 != 0)
								sprintf(buf2, "%dB", result);
							else
								sprintf(buf2, "%dKB", result / 1024);
							sprintf(buf, "PRG ROM size you entered is invalid in iNES 2.0, do you want to set to its nearest value %s?", buf2);
							if (MessageBox(hwnd, buf, "Error", MB_YESNO | MB_ICONERROR) == IDYES)
								SetDlgItemText(hwnd, IDC_PRGROM_COMBO, buf2);
							else
							{
								SetFocus(GetDlgItem(hwnd, IDC_PRGROM_COMBO));
								SendDlgItemMessage(hwnd, IDC_PRGROM_COMBO, EM_SETSEL, 0, -1);
							}
							return false;
						}
						else
							SetDlgItemText(hwnd, IDC_HEX_HEADER_EDIT, "");

					}
				}
				else {
					if (header) {
						MessageBox(hwnd, "PRG ROM size must be multiple of 16KB in iNES 1.0", "Error", MB_OK | MB_ICONERROR);
						SetFocus(GetDlgItem(hwnd, IDC_PRGROM_COMBO));
						SendDlgItemMessage(hwnd, IDC_PRGROM_COMBO, EM_SETSEL, 0, -1);
					}
					else
						SetDlgItemText(hwnd, IDC_HEX_HEADER_EDIT, "");
					// ines 1.0 can't handle this kind of value
					return false;
				}
			}
			// it's multiple size of 16KB
			else {
				// it can be fitted in iNES 1.0
				if (prg_rom < 16 * 1024 * 0xFF)
					_header.ROM_size |= prg_rom / 16 / 1024 & 0xFF;
				else
				{
					if (ines20)
						_header.Upper_ROM_VROM_size |= prg_rom / 16 / 1024 >> 8 & 0xF;
					else {
						if (header)
						{
							MessageBox(hwnd, "PRG ROM size exceeded the limit of iNES 1.0 (4080KB).", "Error", MB_OK | MB_ICONERROR);
							SetFocus(GetDlgItem(hwnd, IDC_PRGROM_COMBO));
							SendDlgItemMessage(hwnd, IDC_PRGROM_COMBO, EM_SETSEL, 0, -1);
						}
						else
							SetDlgItemText(hwnd, IDC_HEX_HEADER_EDIT, "");
						return false;
					}
				}
			}
		}
		// A too large size
		else {
			if (header)
			{
				MessageBox(hwnd, "PRG ROM size you entered is too large to fit into a cartridge, by the way this is an NES emulator, not for XBOX360 or PlayStation2.", "Error", MB_OK | MB_ICONERROR);
				SetFocus(GetDlgItem(hwnd, IDC_PRGROM_COMBO));
				SendDlgItemMessage(hwnd, IDC_PRGROM_COMBO, EM_SETSEL, 0, -1);
			}
			else
				SetDlgItemText(hwnd, IDC_HEX_HEADER_EDIT, "");
			return false;
		}
	}
	else
		return false;

	// PRG RAM
	if (ines20 || IsDlgButtonChecked(hwnd, IDC_CHECK_UNOFFICIAL) == BST_UNCHECKED || IsDlgButtonChecked(hwnd, IDC_CHECK_UNOFFICIAL_PRGRAM) == BST_CHECKED)
	{
		int prg_ram;
		if (GetComboBoxByteSize(hwnd, IDC_PRGRAM_COMBO, &prg_ram, header) == 0)
		{
			if (ines20)
			{
				if (prg_ram < 64 << 0xF)
				{
					if (prg_ram % 64 == 0)
						_header.RAM_size |= (int)log2(prg_ram / 64);
					else
					{
						if (header)
						{
							MessageBox(hwnd, "Invalid PRG RAM size", "Error", MB_OK | MB_ICONERROR);
							SetFocus(GetDlgItem(hwnd, IDC_PRGRAM_COMBO));
							SendDlgItemMessage(hwnd, IDC_PRGRAM_COMBO, EM_SETSEL, 0, -1);
						}
						else
							SetDlgItemText(hwnd, IDC_HEX_HEADER_EDIT, "");

						return false;
					}
				}
				else {
					if (header)
					{
						MessageBox(hwnd, "PRG RAM size exceeded the limit (4096KB)", "Error", MB_OK | MB_ICONERROR);
						SetFocus(GetDlgItem(hwnd, IDC_PRGRAM_COMBO));
						SendDlgItemMessage(hwnd, IDC_PRGRAM_COMBO, EM_SETSEL, 0, -1);
					}
					else
						SetDlgItemText(hwnd, IDC_HEX_HEADER_EDIT, "");

					return false;
				}
			}
			else {
				if (prg_ram < 8 * 1024 * 255)
				{
					if (prg_ram % (8 * 1024) == 0)
						_header.ROM_type3 |= prg_ram / 8 / 1024;
					else {
						if (header)
						{
							MessageBox(hwnd, "PRG RAM size must be multiple of 8KB in iNES 1.0", "Error", MB_OK | MB_ICONERROR);
							SetFocus(GetDlgItem(hwnd, IDC_PRGRAM_COMBO));
							SendDlgItemMessage(hwnd, IDC_PRGRAM_COMBO, EM_SETSEL, 0, -1);
						}
						else
							SetDlgItemText(hwnd, IDC_HEX_HEADER_EDIT, "");

						return false;
					}
				}
				else {
					if (header)
					{
						MessageBox(hwnd, "PRG RAM size exceeded the limit (2040KB)", "Error", MB_OK | MB_ICONERROR);
						SetFocus(GetDlgItem(hwnd, IDC_PRGRAM_COMBO));
						SendDlgItemMessage(hwnd, IDC_PRGRAM_COMBO, EM_SETSEL, 0, -1);
					}
					else
						SetDlgItemText(hwnd, IDC_HEX_HEADER_EDIT, "");

					return false;
				}
			}
		}
		else
			return false;
	}


	// PRG NVRAM
	if (ines20)
	{
		// only iNES 2.0 has value for PRG VMRAM
		int prg_nvram;
		if (GetComboBoxByteSize(hwnd, IDC_PRGNVRAM_COMBO, &prg_nvram, header) == 0)
		{
			if (prg_nvram < 64 << 0xF)
			{
				if (prg_nvram % 64 == 0)
					_header.RAM_size |= (int)log2(prg_nvram / 64) << 4;
				else
				{
					if (header)
					{
						MessageBox(hwnd, "Invalid PRG NVRAM size", "Error", MB_OK | MB_ICONERROR);
						SetFocus(GetDlgItem(hwnd, IDC_PRGNVRAM_COMBO));
						SendDlgItemMessage(hwnd, IDC_PRGNVRAM_COMBO, EM_SETSEL, 0, -1);
					}
					else
						SetDlgItemText(hwnd, IDC_HEX_HEADER_EDIT, "");

					return false;
				}
			}
			else
			{
				if (header)
				{
					MessageBox(hwnd, "PRG NVRAM size exceeded the limit (4096KB)", "Error", MB_OK | MB_ICONERROR);
					SetFocus(GetDlgItem(hwnd, IDC_PRGNVRAM_COMBO));
					SendDlgItemMessage(hwnd, IDC_PRGNVRAM_COMBO, EM_SETSEL, 0, -1);
				}
				else
					SetDlgItemText(hwnd, IDC_HEX_HEADER_EDIT, "");

				return false;
			}

			if (prg_nvram != 0)
				_header.ROM_type |= 2;

		}
		else
			return false;
	}
	else {
		// iNES 1.0 is much simpler, it has only 1 bit for check
		if (IsDlgButtonChecked(hwnd, IDC_CHECK_BATTERYNVRAM) == BST_CHECKED)
			_header.ROM_type |= 2;
	}

	// CHR ROM
	int chr_rom;
	if (GetComboBoxByteSize(hwnd, IDC_CHRROM_COMBO, &chr_rom, header) == 0)
	{
		// max value which a iNES 2.0 header can hold
		if (chr_rom < 8 * 1024 * 0xEFF)
		{
			// try to fit the irregular value with the alternative way
			if (chr_rom % (8 * 1024) != 0)
			{
				if (ines20)
				{
					// try to calculate the nearest value
					bool fit = false;
					int result = 0;
					for (int multiplier = 0; multiplier < 4; ++multiplier)
					{
						for (int exponent = 0; exponent < 64; ++exponent)
						{
							int new_result = pow(2, exponent) * (multiplier * 2 + 1);
							if (new_result == chr_rom)
							{
								_header.Upper_ROM_VROM_size |= 0xF0;
								_header.VROM_size |= exponent << 2;
								_header.VROM_size |= multiplier & 3;
								fit = true;
								break;
							}
							if (result > new_result && new_result > chr_rom)
								result = new_result;
						}
						if (fit) break;
					}

					if (!fit)
					{
						int result10 = (chr_rom / 1024 / 8 + 1) * 8 * 1024;
						if (result10 < result)
							result = result10;
						char buf2[64];
						if (result % 1024 != 0)
							sprintf(buf2, "%dB", result);
						else
							sprintf(buf2, "%dKB", result / 1024);
						sprintf(buf, "CHR ROM size you entered is invalid in iNES 2.0, do you want to set to its nearest value %s?", buf2);
						if (MessageBox(hwnd, buf, "Error", MB_YESNO | MB_ICONERROR) == IDYES)
							SetDlgItemText(hwnd, IDC_CHRROM_COMBO, buf2);
						else
						{
							if (header)
							{
								SetFocus(GetDlgItem(hwnd, IDC_CHRROM_COMBO));
								SendDlgItemMessage(hwnd, IDC_CHRROM_COMBO, EM_SETSEL, 0, -1);
							}
							else
								SetDlgItemText(hwnd, IDC_HEX_HEADER_EDIT, "");

							return false;
						}
					}
				}
				else {
					// ines 1.0 can't handle this kind of value
					if (header)
					{
						MessageBox(hwnd, "CHR ROM size must be multiple of 8KB in iNES 1.0", "Error", MB_OK | MB_ICONERROR);
						SetFocus(GetDlgItem(hwnd, IDC_CHRROM_COMBO));
						SendDlgItemMessage(hwnd, IDC_CHRROM_COMBO, EM_SETSEL, 0, -1);
					}
					else
						SetDlgItemText(hwnd, IDC_HEX_HEADER_EDIT, "");

					return false;
				}
			}
			// it's multiple size of 8KB
			else {
				// it can be fitted in iNES 1.0
				if (chr_rom < 8 * 1024 * 0xFF)
					_header.VROM_size |= chr_rom / 8 / 1024 & 0xFF;
				else
				{
					if (ines20)
						_header.Upper_ROM_VROM_size |= chr_rom / 8 / 1024 >> 4 & 0xF0;
					else
					{
						if (header)
						{
							MessageBox(hwnd, "CHR ROM size exceeded the limit of iNES 1.0 (2040KB).", "Error", MB_OK | MB_ICONERROR);
							SetFocus(GetDlgItem(hwnd, IDC_PRGROM_COMBO));
							SendDlgItemMessage(hwnd, IDC_PRGROM_COMBO, EM_SETSEL, 0, -1);
						}
						else
							SetDlgItemText(hwnd, IDC_HEX_HEADER_EDIT, "");

						return false;
					}
				}
			}
		}
		// A too large size
		else {
			if (header)
			{
				MessageBox(hwnd, "CHR ROM size you entered cannot be fitted in iNES 2.0.", "Error", MB_OK | MB_ICONERROR);
				SetFocus(GetDlgItem(hwnd, IDC_CHRROM_COMBO));
				SendDlgItemMessage(hwnd, IDC_CHRROM_COMBO, EM_SETSEL, 0, -1);
			}
			else
				SetDlgItemText(hwnd, IDC_HEX_HEADER_EDIT, "");

			return false;
		}
	}
	else
		return false;

	// CHR RAM
	if (ines20)
	{
		// only iNES 2.0 has value for CHR RAM
		int chr_ram;
		if (GetComboBoxByteSize(hwnd, IDC_CHRRAM_COMBO, &chr_ram, header) == 0)
		{
			if (chr_ram < 64 << 0xF)
			{
				if (chr_ram % 64 == 0)
					_header.VRAM_size |= (int)log2(chr_ram / 64);
				else
				{
					if (header)
					{
						MessageBox(hwnd, "Invalid CHR RAM size", "Error", MB_OK | MB_ICONERROR);
						SetFocus(GetDlgItem(hwnd, IDC_CHRRAM_COMBO));
						SendDlgItemMessage(hwnd, IDC_CHRRAM_COMBO, EM_SETSEL, 0, -1);
					}
					else
						SetDlgItemText(hwnd, IDC_HEX_HEADER_EDIT, "");

					return false;
				}
			}
			else {
				if (header)
				{
					MessageBox(hwnd, "CHR RAM size exceeded the limit (4096KB)", "Error", MB_OK | MB_ICONERROR);
					SetFocus(GetDlgItem(hwnd, IDC_CHRRAM_COMBO));
					SendDlgItemMessage(hwnd, IDC_CHRRAM_COMBO, EM_SETSEL, 0, -1);
				}
				else
					SetDlgItemText(hwnd, IDC_HEX_HEADER_EDIT, "");

				return false;
			}
		}
		else
			return false;
	}

	// CHR NVRAM
	if (ines20)
	{
		// only iNES 2.0 has value for CHR NVRAM
		int chr_nvram;
		if (GetComboBoxByteSize(hwnd, IDC_CHRNVRAM_COMBO, &chr_nvram, header) == 0)
		{
			if (chr_nvram < 64 << 0xF)
			{
				if (chr_nvram % 64 == 0)
					_header.VRAM_size |= (int)log2(chr_nvram / 64) << 4;
				else {
					if (header)
					{
						MessageBox(hwnd, "Invalid CHR NVRAM size", "Error", MB_OK | MB_ICONERROR);
						SetFocus(GetDlgItem(hwnd, IDC_CHRNVRAM_COMBO));
						SendDlgItemMessage(hwnd, IDC_CHRNVRAM_COMBO, EM_SETSEL, 0, -1);
					}
					else
						SetDlgItemText(hwnd, IDC_HEX_HEADER_EDIT, "");

					return false;
				}
			}
			else {
				if (header)
				{
					MessageBox(hwnd, "CHR NVRAM size exceeded the limit (4096KB)", "Error", MB_OK | MB_ICONERROR);
					SetFocus(GetDlgItem(hwnd, IDC_CHRNVRAM_COMBO));
					SendDlgItemMessage(hwnd, IDC_CHRNVRAM_COMBO, EM_SETSEL, 0, -1);
				}
				else
					SetDlgItemText(hwnd, IDC_HEX_HEADER_EDIT, "");

				return false;
			}

			if (chr_nvram != 0)
				_header.ROM_type |= 2;
		}
		else
			return false;
	}

	// Mirroring
	if (IsDlgButtonChecked(hwnd, IDC_RADIO_MIRR_4SCREEN) == BST_CHECKED)
		_header.ROM_type |= 8;
	else if (IsDlgButtonChecked(hwnd, IDC_RADIO_MIRR_VERTICAL) == BST_CHECKED)
		_header.ROM_type |= 1;

	// Region
	if (IsDlgButtonChecked(hwnd, IDC_RADIO_REGION_PAL) == BST_CHECKED)
	{
		if (ines20)
			_header.TV_system |= 1;
		else {
			_header.Upper_ROM_VROM_size |= 1;
			if (IsDlgButtonChecked(hwnd, IDC_CHECK_UNOFFICIAL) == BST_CHECKED && IsDlgButtonChecked(hwnd, IDC_CHECK_UNOFFICIAL_EXTRA_REGION) == BST_CHECKED)
				_header.RAM_size |= 2;
		}
	}
	else if (IsDlgButtonChecked(hwnd, IDC_RADIO_REGION_DUAL) == BST_CHECKED)
	{
		if (ines20)
			_header.TV_system |= 2;
		else
			_header.RAM_size |= 3;
	}
	else if (IsDlgButtonChecked(hwnd, IDC_RADIO_REGION_DENDY) == BST_CHECKED)
		_header.TV_system |= 3;


	// System
	if (IsDlgButtonChecked(hwnd, IDC_RADIO_SYSTEM_VS) == BST_CHECKED)
	{
		_header.ROM_type2 |= 1;
		if (ines20) {
			// VS System type
			int system;
			if (GetComboBoxListItemData(hwnd, IDC_VS_SYSTEM_COMBO, &system, buf, header) && system <= 0xF)
				_header.VS_hardware |= (system & 0xF) << 4;
			else
			{
				if (header)
				{
					MessageBox(hwnd, "Invalid VS System hardware type.", "Error", MB_OK | MB_ICONERROR);
					SetFocus(GetDlgItem(hwnd, IDC_VS_SYSTEM_COMBO));
					SendDlgItemMessage(hwnd, IDC_VS_SYSTEM_COMBO, EM_SETSEL, 0, -1);
				}
				else
					SetDlgItemText(hwnd, IDC_HEX_HEADER_EDIT, "");

				return false;
			}
			// VS PPU type
			int ppu;
			if (GetComboBoxListItemData(hwnd, IDC_VS_PPU_COMBO, &ppu, buf, header) && system <= 0xF)
				_header.VS_hardware |= ppu & 0xF;
			else
			{
				if (header)
				{
					MessageBox(hwnd, "Invalid VS System PPU type.", "Error", MB_OK | MB_ICONERROR);
					SetFocus(GetDlgItem(hwnd, IDC_VS_PPU_COMBO));
					SendDlgItemMessage(hwnd, IDC_VS_PPU_COMBO, EM_SETSEL, 0, -1);
				}
				else
					SetDlgItemText(hwnd, IDC_HEX_HEADER_EDIT, "");

				return false;
			}
		}
	}
	else if (IsDlgButtonChecked(hwnd, IDC_RADIO_SYSTEM_PLAYCHOICE10) == BST_CHECKED)
		_header.ROM_type2 |= 2;
	else if (IsDlgButtonChecked(hwnd, IDC_RADIO_SYSTEM_EXTEND) == BST_CHECKED)
	{
		// Extend System
		_header.ROM_type2 |= 3;
		int extend;
		if (GetComboBoxListItemData(hwnd, IDC_SYSTEM_EXTEND_COMBO, &extend, buf, header) && extend <= 0x3F)
			_header.VS_hardware |= extend & 0x3F;
		else
		{
			if (header)
			{
				MessageBox(hwnd, "Invalid extend system type", "Error", MB_OK | MB_ICONERROR);
				SetFocus(GetDlgItem(hwnd, IDC_SYSTEM_EXTEND_COMBO));
				SendDlgItemMessage(hwnd, IDC_SYSTEM_EXTEND_COMBO, EM_SETSEL, 0, -1);
			}
			else
				SetDlgItemText(hwnd, IDC_HEX_HEADER_EDIT, "");

			return false;
		}
	}

	// Input device
	if (ines20)
	{
		int input;
		if (GetComboBoxListItemData(hwnd, IDC_INPUT_DEVICE_COMBO, &input, buf, header) && input <= 0x3F)
			_header.reserved[1] |= input & 0x3F;
		else
		{
			if (header)
			{
				MessageBox(hwnd, "Invalid input device.", "Error", MB_OK | MB_ICONERROR);
				SetFocus(GetDlgItem(hwnd, IDC_INPUT_DEVICE_COMBO));
				SendDlgItemMessage(hwnd, IDC_INPUT_DEVICE_COMBO, EM_SETSEL, 0, -1);
			}
			else
				SetDlgItemText(hwnd, IDC_HEX_HEADER_EDIT, "");

			return false;
		}
	}

	// Miscellanous ROM(s)
	if (ines20)
	{
		GetDlgItemText(hwnd, IDC_MISCELLANEOUS_ROMS_EDIT, buf, 256);
		int misc_roms = 0;
		if (sscanf(buf, "%d", &misc_roms) < 1)
		{
			if (header)
			{
				MessageBox(hwnd, "Invalid miscellanous ROM(s) count. If you don't know what value should be, we recommend to set it to 0.", "Error", MB_OK | MB_ICONERROR);
				SetFocus(GetDlgItem(hwnd, IDC_MISCELLANEOUS_ROMS_EDIT));
				SendDlgItemMessage(hwnd, IDC_MISCELLANEOUS_ROMS_EDIT, EM_SETSEL, 0, -1);
			}
			else
				SetDlgItemText(hwnd, IDC_HEX_HEADER_EDIT, "");

			return false;
		}
		if (misc_roms > 3)
		{
			if (header)
			{
				MessageBox(hwnd, "Miscellanous ROM(s) count has exceeded the limit of iNES 2.0 (3)", "Error", MB_OK | MB_ICONERROR);
				SetFocus(GetDlgItem(hwnd, IDC_MISCELLANEOUS_ROMS_EDIT));
				SendDlgItemMessage(hwnd, IDC_MISCELLANEOUS_ROMS_EDIT, EM_SETSEL, 0, -1);
			}
			else
				SetDlgItemText(hwnd, IDC_HEX_HEADER_EDIT, "");

			return false;
		}
		_header.reserved[0] |= misc_roms & 3;
	}

	// iNES 1.0 unofficial properties
	if (!ines20 && IsDlgButtonChecked(hwnd, IDC_CHECK_UNOFFICIAL) == BST_CHECKED)
	{
		// bus conflict configure in unoffcial bit of iNES 1.0
		if (IsDlgButtonChecked(hwnd, IDC_CHECK_UNOFFICIAL_BUS_CONFLICT) == BST_CHECKED)
			_header.RAM_size |= 0x20;
		// PRG RAM non exist bit flag
		if (IsDlgButtonChecked(hwnd, IDC_CHECK_UNOFFICIAL_PRGRAM) == BST_UNCHECKED)
			_header.RAM_size |= 0x10;
	}

	// Trainer
	if (IsDlgButtonChecked(hwnd, IDC_CHECK_TRAINER) == BST_CHECKED)
		_header.ROM_type |= 4;

	extern BMAPPINGLocal bmap[];
	bool fceux_support = false;
	for (int i = 0; bmap[i].init; ++i)
	{
		if (mapper == bmap[i].number)
		{
			fceux_support = true;
			break;
		}
	}

	if (!fceux_support)
	{
		if (header)
		{
			sprintf(buf, "FCEUX doesn't support iNES Mapper# %d, this is not a serious problem, but the ROM will not be run in FCEUX properly.\nDo you want to continue?", mapper);
			if (MessageBox(hwnd, buf, "Error", MB_YESNO | MB_ICONWARNING) == IDNO)
			{
				SetFocus(GetDlgItem(hwnd, IDC_MAPPER_COMBO));
				SendDlgItemMessage(hwnd, IDC_MAPPER_COMBO, EM_SETSEL, 0, -1);
			}
		}
		else
			SetDlgItemText(hwnd, IDC_HEX_HEADER_EDIT, "");

		return false;
	}

	memcpy(_header.ID, "NES\x1A", 4);
	if (header)
		memcpy(header, &_header, sizeof(iNES_HEADER));
	else
	{
		sprintf(buf, "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X", _header.ID[0], _header.ID[1], _header.ID[2], _header.ID[3], _header.ROM_size, _header.VROM_size, _header.ROM_type, _header.ROM_type2, _header.ROM_type3, _header.Upper_ROM_VROM_size, _header.RAM_size, _header.VRAM_size, _header.TV_system, _header.VS_hardware, _header.reserved[0], _header.reserved[1]);
		SetDlgItemText(hwnd, IDC_HEX_HEADER_EDIT, buf);
	}

	return true;
}

int GetComboBoxByteSize(HWND hwnd, UINT id, int* value, iNES_HEADER* header)
{
	char* name = "";
	char buf[256];

	enum errors {
		OK = 0,
		FORMAT_ERR,
		MINUS_ERR,
		UNIT_ERR
	};

	int err = errors::OK;

	switch (id)
	{
		case IDC_PRGROM_COMBO: name = "PRG ROM"; break;
		case IDC_PRGRAM_COMBO: name = "PRG RAM"; break;
		case IDC_PRGNVRAM_COMBO: name = "PRG NVRAM"; break;
		case IDC_CHRROM_COMBO: name = "CHR ROM"; break;
		case IDC_CHRRAM_COMBO: name = "CHR RAM"; break;
		case IDC_CHRNVRAM_COMBO: name = "CHR NVRAM"; break;
	}

	if (!GetComboBoxListItemData(hwnd, id, value, buf, header))
	{
		char buf2[4];
		if (sscanf(buf, "%d%3[KMB]", value, buf2) < 2 || !strcmp(buf2, ""))
			err = errors::FORMAT_ERR;
		else
		{
			if (*value < 0)
				err = errors::MINUS_ERR;
			else if (!strcmp(buf2, "KB") || !strcmp(buf2, "K"))
				*value *= 1024;
			else if (!strcmp(buf2, "MB") || !strcmp(buf2, "M"))
				*value *= 1024 * 1024;
			else if (strcmp(buf2, "B"))
				err = errors::UNIT_ERR;
		}
	}

	if (header)
	{
		switch (err)
		{
			case errors::FORMAT_ERR:
				sprintf(buf, "%s size you entered is invalid, it should be positive decimal integer followed with unit, e.g. 1024B, 128KB, 4MB", name);
				break;
			case errors::UNIT_ERR:
				sprintf(buf, "The unit of %s size you entered is invalid, it must be B, KB or MB", name);
				break;
			case errors::MINUS_ERR:
				sprintf(buf, "Negative value of %s is not supported by iNES header.", name);
				break;
		}

		if (err)
		{
			MessageBox(hwnd, buf, "Error", MB_OK | MB_ICONERROR);
			SetFocus(GetDlgItem(hwnd, id));
			SendDlgItemMessage(hwnd, id, EM_SETSEL, 0, -1);
		}
	}
	else
		SetDlgItemText(hwnd, IDC_HEX_HEADER_EDIT, "");

	return err;
}

bool GetComboBoxListItemData(HWND hwnd, UINT id, int* value, char* buf, iNES_HEADER* header)
{

	bool success = true;
	*value = SendDlgItemMessage(hwnd, id, CB_GETCURSEL, 0, 0);
	if (*value != CB_ERR)
		*value = SendDlgItemMessage(hwnd, id, CB_GETITEMDATA, *value, 0);
	else {
		GetDlgItemText(hwnd, id, buf, 256);

		if (header)
			*value = SendDlgItemMessage(hwnd, id, CB_SELECTSTRING, 0, (LPARAM)buf);
		else
			*value = SendDlgItemMessage(hwnd, id, CB_FINDSTRINGEXACT, 0, (LPARAM)buf);

		if (*value != CB_ERR)
			*value = SendDlgItemMessage(hwnd, id, CB_GETITEMDATA, *value, 0);
		else
		{
			switch (id)
			{
				default:
					success = false;
					break;
				case IDC_MAPPER_COMBO:
					if (!(success = sscanf(buf, "%d", value) > 0))
						success = SearchByString(hwnd, id, value, buf, header);
					else if (header)
						SetDlgItemText(hwnd, id, buf);
					break;
				case IDC_VS_SYSTEM_COMBO:
				case IDC_VS_PPU_COMBO:
				case IDC_SYSTEM_EXTEND_COMBO:
					if (!(success = sscanf(buf, "$%X", (unsigned int *)value) > 0))
						success = SearchByString(hwnd, id, value, buf, header);
					else if (header)
						SetDlgItemText(hwnd, id, buf);
					break;
				case IDC_INPUT_DEVICE_COMBO:
					if (success = sscanf(buf, "$%X", (unsigned int *)value) > 0)
					{
						char buf2[8];
						sprintf(buf2, "$%02X", *value);
						if (header && SendDlgItemMessage(hwnd, id, CB_SELECTSTRING, 0, (LPARAM)buf2) == CB_ERR)
							SetDlgItemText(hwnd, id, buf);
					}
					else
						success = SearchByString(hwnd, id, value, buf, header);
					break;
			}

			if (header && !success)
				SetDlgItemText(hwnd, id, buf);
		}
	}

	return success;
}

// Warning: when in save mode, the content of buf might be overwritten by the save filename which user changed.
bool ShowINESFileBox(HWND parent, char* buf, iNES_HEADER* header)
{
	char *filename = NULL, *path = NULL;
	bool success = true;

	if (header)
	{
		// When open this dialog for saving prpose, the buf must be a separate buf.
		if (buf && buf != LoadedRomFName)
		{
			extern char* GetRomName(bool force = false);
			extern char* GetRomPath(bool force = false);
			filename = GetRomName(true);
			char* second = strchr(filename, '|');
			if (second)
			{
				char* _filename = (char*)calloc(1, 2048);
				strcpy(_filename, second + 1);
				char* third = strrchr(filename, '\\');
				if (third)
					strcpy(_filename, third + 1);
				free(filename);
				filename = _filename;
			}
			char header_str[32];
			sprintf(header_str, " [%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X].nes", header->ROM_size, header->VROM_size, header->ROM_size, header->ROM_type2, header->ROM_type3, header->Upper_ROM_VROM_size, header->RAM_size, header->VRAM_size, header->TV_system, header->VS_hardware, header->reserved[0], header->reserved[1]);
			strcat(filename, header_str);
			path = GetRomPath(true);
		}
		else
			success = false;
	}
	else {
		if (!buf)
			buf = LoadedRomFName;
		filename = (char*)calloc(1, 2048);
		path = (char*)calloc(1, 2048);
	}

	if (success)
	{
		OPENFILENAME ofn;
		memset(&ofn, 0, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.lpstrTitle = header ? "Save NES file" : "Open NES file";
		ofn.lpstrFilter = "NES ROM file (*.nes)\0*.nes\0All files (*.*)\0*.*\0\0";
		ofn.hInstance = fceu_hInstance;
		ofn.hwndOwner = parent;
		ofn.lpstrFile = filename;
		ofn.nMaxFile = 2048;
		ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
		ofn.lpstrInitialDir = path;
		ofn.lpstrDefExt = "nes";

		if (header ? GetSaveFileName(&ofn) : GetOpenFileName(&ofn))
			strcpy(buf, filename);
		else
			success = false;
	}

	if (filename) free(filename);
	if (path) free(path);

	return success;
}

bool SaveINESFile(HWND hwnd, char* path, iNES_HEADER* header)
{
	char buf[4096];
	const char* ext[] = { "nes", 0 };

	// Source file
	FCEUFILE* source = FCEU_fopen(LoadedRomFName, NULL, "rb", 0, -1, ext);
	if (!source)
	{
		sprintf(buf, "Opening source file %s failed.", LoadedRomFName);
		MessageBox(hwnd, buf, "iNES Header Editor", MB_OK | MB_ICONERROR);
		return false;
	}

	// Destination file
	FILE* target = FCEUD_UTF8fopen(path, "wb");
	if (!target)
	{
		sprintf(buf, "Creating target file %s failed.", path);
		MessageBox(hwnd, buf, "iNES Header Editor", MB_OK | MB_ICONERROR);
		FCEU_fclose(source);
		return false;
	}

	memset(buf, 0, sizeof(buf));

	// Write the header first
	fwrite(header, 1, sizeof(iNES_HEADER), target);
	// Skip the original header
	FCEU_fseek(source, sizeof(iNES_HEADER), SEEK_SET);
	int len;
	while (len = FCEU_fread(buf, 1, sizeof(buf), source))
		fwrite(buf, len, 1, target);

	FCEU_fclose(source);
	fclose(target);

	return true;

}

bool SearchByString(HWND hwnd, UINT id, int* value, char* buf, iNES_HEADER* header)
{
	if (buf[0] != ' ' && buf[0] != 0)
	{
		if (id == IDC_MAPPER_COMBO)
		{
			extern BMAPPINGLocal bmap[];
			for (int i = 0; bmap[i].init; ++i)
			{
				if (!stricmp(buf, bmap[i].name))
				{
					if (header)
						SendDlgItemMessage(hwnd, id, CB_SETCURSEL, i, 0);
					*value = SendDlgItemMessage(hwnd, id, CB_GETITEMDATA, i, 0);
					return true;
				}
			}
		}
		else
		{
			for (int i = 0; dropDownIdList[i]; ++i)
			{
				if (dropDownIdList[i] == id)
				{
					char** checkList = dropDownList[i];
					for (int j = 0; checkList[j]; ++j)
					{
						if (!stricmp(buf, checkList[j]))
						{
							if (header)
								SendDlgItemMessage(hwnd, id, CB_SETCURSEL, j, 0);
							*value = SendDlgItemMessage(hwnd, id, CB_GETITEMDATA, j, 0);
							return true;
						}
					}
					break;
				}
			}
		}
	}

	return false;
}
