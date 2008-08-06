#include "common.h"

/**
* Show an Save File dialog and save a savegame state to the selected file.
**/
void FCEUD_SaveStateAs()
{
	const char filter[] = FCEU_NAME" Save State (*.fc?)\0*.fc?\0";
	char nameo[2048];
	OPENFILENAME ofn;

	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hInstance = fceu_hInstance;
	ofn.lpstrTitle = "Save State As...";
	ofn.lpstrFilter = filter;
	ofn.lpstrDefExt = "fcs";
	nameo[0] = 0;
	ofn.lpstrFile = nameo;
	ofn.nMaxFile = 256;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	if(GetSaveFileName(&ofn))
	{
		FCEUI_SaveState(nameo);
	}
}

/**
* Show an Open File dialog and load a savegame state from the selected file.
**/
void FCEUD_LoadStateFrom()
{
	const char filter[]= FCEU_NAME" Save State (*.fc?)\0*.fc?\0";
	char nameo[2048];
	OPENFILENAME ofn;

	// Create and show an Open File dialog.
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hInstance=fceu_hInstance;
	ofn.lpstrTitle="Load State From...";
	ofn.lpstrFilter=filter;
	nameo[0]=0;
	ofn.lpstrFile=nameo;
	ofn.nMaxFile=256;
	ofn.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;

	if(GetOpenFileName(&ofn))
	{
		// Load save state if a file was selected.
		FCEUI_LoadState(nameo);
	}
}

