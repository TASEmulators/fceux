#include "common.h"
#include <string>
#include <string.h>
#include <fstream>

using namespace std;

//Externs
extern int CurrentState;					//Declared in src/state.cpp
extern bool FCEUSS_Load(const char *fname); //Declared in src/state.cpp
extern string GetBackupFileName();			//Declared in src/state.cpp

bool CheckBackupSaveStateExist();	//Checks if backupsavestate exists
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
	std::string initdir = FCEU_GetPath(FCEUMKF_STATE);
	ofn.lpstrInitialDir = initdir.c_str();
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
	std::string initdir = FCEU_GetPath(FCEUMKF_STATE);
	ofn.lpstrInitialDir = initdir.c_str();
	ofn.nMaxFile=256;
	ofn.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;

	if(GetOpenFileName(&ofn))
	{
		// Load save state if a file was selected.
		FCEUI_LoadState(nameo);
	}
}

bool CheckBackupSaveStateExist()
{
	//This function simply checks to see if the backup savestate of the appropriate filename exists
	string filename = GetBackupFileName(); //Get backup savestate filename
		
	//Check if this filename exists
	fstream test;
	test.open(filename.c_str(),fstream::in);
		
	if (test.fail())
	{
		test.close();
		return false;
	}
	else
	{
		test.close();
		return true;
	}
}

