/* ---------------------------------------------------------------------------------
Main TAS Editor file
Copyright (c) 2011-2012 AnS

(The MIT License)
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------------
Main - Logical center of the program
[Singleton]

* the point of launching TAS Editor from emulator
* the point of quitting from TAS Editor
* regularly (at the end of every frame) updates all modules that need regular update
* implements operations of the "File" menu: creating New project, opening a file, saving, compact saving, import, export
* stores Autofire Patterns data and their loading/generating code
* stores resources: patterns filename, id of buttonpresses in patterns
------------------------------------------------------------------------------------ */

#include <fstream>
#include "taseditor/taseditor_project.h"
#include "utils/xstring.h"
#include "main.h"			// for GetRomName
#include "taseditor.h"

using namespace std;

// TAS Editor data
bool emulator_must_run_taseditor = false;
bool Taseditor_rewind_now = false;
bool must_call_manual_lua_function = false;
std::vector<std::string> autofire_patterns_names;
std::vector<std::vector<uint8>> autofire_patterns;

// all Taseditor functional modules
TASEDITOR_CONFIG taseditor_config;
TASEDITOR_WINDOW taseditor_window;
TASEDITOR_PROJECT project;
HISTORY history;
PLAYBACK playback;
RECORDER recorder;
GREENZONE greenzone;
MARKERS_MANAGER markers_manager;
BOOKMARKS bookmarks;
POPUP_DISPLAY popup_display;
PIANO_ROLL piano_roll;
TASEDITOR_LUA taseditor_lua;
SELECTION selection;
SPLICER splicer;

extern int joysticks_per_frame[NUM_SUPPORTED_INPUT_TYPES];
extern bool turbo;
// temporarily saved FCEUX config
int saved_eoptions;
int saved_EnableAutosave;
extern int EnableAutosave;
// FCEUX
extern EMOVIEMODE movieMode;	// maybe we need normal setter for movieMode, to encapsulate it
extern void UpdateCheckedMenuItems();
// lua engine
extern void TaseditorAutoFunction();
extern void TaseditorManualFunction();

// resources
char patternsFilename[] = "\\taseditor_patterns.txt";
char autofire_patterns_flagpress = 49;	//	"1"

bool EnterTasEditor()
{
	if(!FCEU_IsValidUI(FCEUI_TASEDITOR)) return false;
	if(!taseditor_window.hwndTasEditor)
	{
		// start TAS Editor

		// init/load autofire_patterns
		int total_patterns = 0;
		autofire_patterns.resize(total_patterns);
		autofire_patterns_names.resize(total_patterns);
		char nameo[2048];
		strncpy(nameo, FCEU_GetPath(FCEUMKF_TASEDITOR).c_str(), 2047);
		strncat(nameo, patternsFilename, 2047 - strlen(nameo));
		EMUFILE_FILE ifs(nameo, "rb");
		if(!ifs.fail())
		{
			std::string tempstr1, tempstr2;
			while (ReadString(&ifs, tempstr1))
			{
				if (ReadString(&ifs, tempstr2))
				{
					total_patterns++;
					// save the name
					autofire_patterns_names.push_back(tempstr1);
					// parse 2nd string to sequence of 1s and 0s
					autofire_patterns.resize(total_patterns);
					autofire_patterns[total_patterns - 1].resize(tempstr2.size());
					for (int i = tempstr2.size() - 1; i >= 0; i--)
					{
						if (tempstr2[i] == autofire_patterns_flagpress)
							autofire_patterns[total_patterns - 1][i] = 1;
						else
							autofire_patterns[total_patterns - 1][i] = 0;
					}
				}
			}
		} else
		{
			FCEU_printf("Could not load tools\\taseditor_patterns.txt!\n");
		}
		if (autofire_patterns.size() == 0)
		{
			FCEU_printf("Will be using default set of patterns...\n");
			autofire_patterns.resize(4);
			autofire_patterns_names.resize(4);
			// Default Pattern 0: Alternating (1010...)
			autofire_patterns_names[0] = "Alternating (1010...)";
			autofire_patterns[0].resize(2);
			autofire_patterns[0][0] = 1;
			autofire_patterns[0][1] = 0;
			// Default Pattern 1: Alternating at 30FPS (11001100...)
			autofire_patterns_names[1] = "Alternating at 30FPS (11001100...)";
			autofire_patterns[1].resize(4);
			autofire_patterns[1][0] = 1;
			autofire_patterns[1][1] = 1;
			autofire_patterns[1][2] = 0;
			autofire_patterns[1][3] = 0;
			// Default Pattern 2: One Quarter (10001000...)
			autofire_patterns_names[2] = "One Quarter (10001000...)";
			autofire_patterns[2].resize(4);
			autofire_patterns[2][0] = 1;
			autofire_patterns[2][1] = 0;
			autofire_patterns[2][2] = 0;
			autofire_patterns[2][3] = 0;
			// Default Pattern 3: Tap'n'Hold (1011111111111111111111111111111111111...)
			autofire_patterns_names[3] = "Tap'n'Hold (101111111...)";
			autofire_patterns[3].resize(1000);
			autofire_patterns[3][0] = 1;
			autofire_patterns[3][1] = 0;
			for (int i = 2; i < 1000; ++i)
				autofire_patterns[3][i] = 1;
		}
		// reset current_pattern if it's outside the range
		if (taseditor_config.current_pattern < 0 || taseditor_config.current_pattern >= (int)autofire_patterns.size())
			taseditor_config.current_pattern = 0;

		// create window
		taseditor_window.init();
		if(taseditor_window.hwndTasEditor)
		{
			// save "eoptions"
			saved_eoptions = eoptions;
			// set "Run in background"
			eoptions |= EO_BGRUN;
			// "Set high-priority thread"
			eoptions |= EO_HIGHPRIO;
			DoPriority();
			// clear "Disable speed throttling"
			eoptions &= ~EO_NOTHROTTLE;
			// switch off autosaves
			saved_EnableAutosave = EnableAutosave;
			EnableAutosave = 0;
			UpdateCheckedMenuItems();
			
			// init modules
			piano_roll.init();
			selection.init();
			splicer.init();
			playback.init();
			greenzone.init();
			recorder.init();
			markers_manager.init();
			project.init();
			bookmarks.init();
			popup_display.init();
			history.init();
			taseditor_lua.init();
			// either start new movie or use current movie
			if (FCEUMOV_Mode(MOVIEMODE_INACTIVE))
			{
				// create new movie
				FCEUI_StopMovie();
				movieMode = MOVIEMODE_TASEDITOR;
				CreateCleanMovie();
				playback.StartFromZero();
			} else
			{
				// use current movie to create a new project
				if (currMovieData.savestate.size() != 0)
				{
					FCEUD_PrintError("This version of TAS Editor doesn't work with movies starting from savestate.");
					// delete savestate, but preserve input anyway
					currMovieData.savestate.clear();
				}
				FCEUI_StopMovie();
				movieMode = MOVIEMODE_TASEDITOR;
			}
			// ensure that movie has correct set of ports/fourscore
			SetInputType(currMovieData, GetInputType(currMovieData));
			// force the input configuration stored in the movie to apply to FCEUX config
			FCEUD_SetInput(currMovieData.fourscore, currMovieData.microphone, (ESI)currMovieData.ports[0], (ESI)currMovieData.ports[1], (ESIFC)currMovieData.ports[2]);
			// reset some modules that need MovidData info
			piano_roll.reset();
			recorder.reset();
			// create initial snapshot in history
			history.reset();
			// reset Taseditor variables
			must_call_manual_lua_function = false;
			
			SetFocus(history.hwndHistoryList);		// set focus only once, to show selection cursor
			SetFocus(piano_roll.hwndList);
			FCEU_DispMessage("TAS Editor engaged", 0);
			taseditor_window.RedrawTaseditor();
			return true;
		} else return false;
	} else return true;
}
// returns false if couldn't real a string containing at least one char
bool ReadString(EMUFILE *is, std::string& dest)
{
	dest.resize(0);
	int c;
	while (true)
	{
		c = is->fgetc();
		if (c < 0) break;
		if (c == 10 || c == 13)		// end of line
		{
			if (dest.size())
				break;		// already collected at least one char
			else
				continue;	// skip the char and continue searching
		} else
		{
			dest.push_back(c);
		}
	}
	return dest.size() != 0;
}

bool ExitTasEditor()
{
	if (!AskSaveProject()) return false;

	// free autofire_patterns
	autofire_patterns.resize(0);
	autofire_patterns_names.resize(0);

	// destroy window
	taseditor_window.exit();
	// release memory
	piano_roll.free();
	markers_manager.free();
	greenzone.free();
	bookmarks.free();
	popup_display.free();
	history.free();
	playback.SeekingStop();
	selection.free();

	// restore "eoptions"
	eoptions = saved_eoptions;
	// restore autosaves
	EnableAutosave = saved_EnableAutosave;
	DoPriority();
	UpdateCheckedMenuItems();
	// switch off taseditor mode
	movieMode = MOVIEMODE_INACTIVE;
	FCEU_DispMessage("TAS Editor disengaged", 0);
	CreateCleanMovie();
	return true;
}

// everyframe function
void UpdateTasEditor()
{
	if(!taseditor_window.hwndTasEditor)
	{
		// TAS Editor is not engaged... but we still should run Lua auto function
		TaseditorAutoFunction();
		if (emulator_must_run_taseditor)
		{
			char fullname[512];
			strcpy(fullname, curMovieFilename);
			if (EnterTasEditor())
				LoadProject(fullname);
			emulator_must_run_taseditor = false;
		}
		return;
	}

	// update all modules that need to be updated every frame
	recorder.update();
	piano_roll.update();
	markers_manager.update();
	greenzone.update();
	playback.update();
	bookmarks.update();
	popup_display.update();
	selection.update();
	splicer.update();
	history.update();
	project.update();
	
	// run Lua functions if needed
	if (taseditor_config.enable_auto_function)
		TaseditorAutoFunction();
	if (must_call_manual_lua_function)
	{
		TaseditorManualFunction();
		must_call_manual_lua_function = false;
	}
}

BOOL CALLBACK NewProjectProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static struct NewProjectParameters* p = NULL;
	switch (message)
	{
		case WM_INITDIALOG:
			p = (struct NewProjectParameters*)lParam;
			p->input_type = GetInputType(currMovieData);
			p->copy_current_input = p->copy_current_markers = false;
			if (strlen(taseditor_config.last_author))
			{
				// convert UTF8 char* string to Unicode wstring
				wchar_t saved_author_name[AUTHOR_MAX_LEN] = {0};
				MultiByteToWideChar(CP_UTF8, 0, taseditor_config.last_author, -1, saved_author_name, AUTHOR_MAX_LEN);
				p->author_name = saved_author_name;
			} else
			{
				p->author_name = L"";
			}
			switch (p->input_type)
			{
			case INPUT_TYPE_1P:
				{
					Button_SetCheck(GetDlgItem(hwndDlg, IDC_RADIO_1PLAYER), BST_CHECKED);
					break;
				}
			case INPUT_TYPE_2P:
				{
					Button_SetCheck(GetDlgItem(hwndDlg, IDC_RADIO_2PLAYERS), BST_CHECKED);
					break;
				}
			case INPUT_TYPE_FOURSCORE:
				{
					Button_SetCheck(GetDlgItem(hwndDlg, IDC_RADIO_FOURSCORE), BST_CHECKED);
					break;
				}
			}
			SendMessage(GetDlgItem(hwndDlg, IDC_EDIT_AUTHOR), CCM_SETUNICODEFORMAT, TRUE, 0);
			SetDlgItemTextW(hwndDlg, IDC_EDIT_AUTHOR, (LPCWSTR)(p->author_name.c_str()));
			return 0;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_RADIO_1PLAYER:
					p->input_type = INPUT_TYPE_1P;
					break;
				case IDC_RADIO_2PLAYERS:
					p->input_type = INPUT_TYPE_2P;
					break;
				case IDC_RADIO_FOURSCORE:
					p->input_type = INPUT_TYPE_FOURSCORE;
					break;
				case IDC_COPY_INPUT:
					p->copy_current_input ^= 1;
					CheckDlgButton(hwndDlg, IDC_COPY_INPUT, p->copy_current_input?MF_CHECKED : MF_UNCHECKED);
					break;
				case IDC_COPY_MARKERS:
					p->copy_current_markers ^= 1;
					CheckDlgButton(hwndDlg, IDC_COPY_MARKERS, p->copy_current_markers?MF_CHECKED : MF_UNCHECKED);
					break;
				case IDOK:
				{
					// save author name in params and in taseditor_config (converted to multibyte char*)
					wchar_t author_name[AUTHOR_MAX_LEN] = {0};
					GetDlgItemTextW(hwndDlg, IDC_EDIT_AUTHOR, (LPWSTR)author_name, AUTHOR_MAX_LEN);
					p->author_name = author_name;
					if (p->author_name == L"")
						taseditor_config.last_author[0] = 0;
					else
						// convert Unicode wstring to UTF8 char* string
						WideCharToMultiByte(CP_UTF8, 0, (p->author_name).c_str(), -1, taseditor_config.last_author, AUTHOR_MAX_LEN, 0, 0);
					EndDialog(hwndDlg, 1);
					return TRUE;
				}
				case IDCANCEL:
					EndDialog(hwndDlg, 0);
					return TRUE;
			}
			break;
	}
	return FALSE; 
}

void NewProject()
{
	if (!AskSaveProject()) return;

	static struct NewProjectParameters params;
	if (DialogBoxParam(fceu_hInstance, MAKEINTRESOURCE(IDD_TASEDITOR_NEWPROJECT), taseditor_window.hwndTasEditor, NewProjectProc, (LPARAM)&params) > 0)
	{
		CreateCleanMovie();
		
		// apply selected options
		SetInputType(currMovieData, params.input_type);
		if (params.copy_current_input)
			// copy input from current snapshot (from history)
			history.GetCurrentSnapshot().toMovie(currMovieData);
		if (!params.copy_current_markers)
			markers_manager.reset();
		if(params.author_name != L"") currMovieData.comments.push_back(L"author " + params.author_name);
		
		// reset Taseditor
		project.init();			// new project has blank name
		greenzone.reset();
		playback.reset();
		playback.StartFromZero();
		bookmarks.reset();
		history.reset();
		piano_roll.reset();
		selection.reset();
		splicer.reset();
		recorder.reset();
		popup_display.reset();
		taseditor_window.RedrawTaseditor();
		taseditor_window.UpdateCaption();
	}
}

void OpenProject()
{
	if (!AskSaveProject()) return;

	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = taseditor_window.hwndTasEditor;
	ofn.hInstance = fceu_hInstance;
	ofn.lpstrTitle = "Open TAS Editor Project";
	const char filter[] = "TAS Editor Projects (*.fm3)\0*.fm3\0All Files (*.*)\0*.*\0\0";
	ofn.lpstrFilter = filter;

	char nameo[2048];
	strcpy(nameo, mass_replace(GetRomName(), "|", ".").c_str());	//convert | to . for archive filenames

	ofn.lpstrFile = nameo;							
	ofn.nMaxFile = 2048;
	ofn.Flags = OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_FILEMUSTEXIST;
	string initdir = FCEU_GetPath(FCEUMKF_MOVIE);	
	ofn.lpstrInitialDir = initdir.c_str();

	if(GetOpenFileName(&ofn))							// If it is a valid filename
	{
		LoadProject(nameo);
	}
}
bool LoadProject(char* fullname)
{
	// try to load project
	if (project.load(fullname))
	{
		// update FCEUX input config
		FCEUD_SetInput(currMovieData.fourscore, currMovieData.microphone, (ESI)currMovieData.ports[0], (ESI)currMovieData.ports[1], (ESIFC)currMovieData.ports[2]);
		// add new file to Recent menu
		taseditor_window.UpdateRecentProjectsArray(fullname);
		taseditor_window.RedrawTaseditor();
		taseditor_window.UpdateCaption();
		return true;
	} else
	{
		// failed to load
		taseditor_window.RedrawTaseditor();
		taseditor_window.UpdateCaption();
		return false;
	}
}

// Saves current project
bool SaveProjectAs()
{
	const char filter[] = "TAS Editor Projects (*.fm3)\0*.fm3\0All Files (*.*)\0*.*\0\0";
	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = taseditor_window.hwndTasEditor;
	ofn.hInstance = fceu_hInstance;
	ofn.lpstrTitle = "Save TAS Editor Project As...";
	ofn.lpstrFilter = filter;

	char nameo[2048];
	if (project.GetProjectName().empty())
	{
		// suggest ROM name for this project
		strcpy(nameo, mass_replace(GetRomName(), "|", ".").c_str());	//convert | to . for archive filenames
		// add .fm3 extension
		strncat(nameo, ".fm3", 2047);
	} else
	{
		// suggest current name
		strncpy(nameo, project.GetProjectName().c_str(), 2047);
	}

	ofn.lpstrFile = nameo;
	ofn.lpstrDefExt = "fm3";
	ofn.nMaxFile = 2048;
	ofn.Flags = OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;
	string initdir = FCEU_GetPath(FCEUMKF_MOVIE);			//Initial directory
	ofn.lpstrInitialDir = initdir.c_str();

	if(GetSaveFileName(&ofn))								//If it is a valid filename
	{
		project.RenameProject(nameo);
		project.save();
		taseditor_window.UpdateRecentProjectsArray(nameo);
	} else return false;
	// saved successfully - remove * mark from caption
	taseditor_window.UpdateCaption();
	return true;
}
bool SaveProject()
{
	if (project.GetProjectFile().empty())
	{
		return SaveProjectAs();
	} else
	{
		project.save();
		taseditor_window.UpdateCaption();
	}
	return true;
}

void SaveCompact_GetCheckboxes(HWND hwndDlg)
{
	taseditor_config.savecompact_binary = (SendDlgItemMessage(hwndDlg, IDC_CHECK_BINARY, BM_GETCHECK, 0, 0) == BST_CHECKED);
	taseditor_config.savecompact_markers = (SendDlgItemMessage(hwndDlg, IDC_CHECK_MARKERS, BM_GETCHECK, 0, 0) == BST_CHECKED);
	taseditor_config.savecompact_bookmarks = (SendDlgItemMessage(hwndDlg, IDC_CHECK_BOOKMARKS, BM_GETCHECK, 0, 0) == BST_CHECKED);
	taseditor_config.savecompact_greenzone = (SendDlgItemMessage(hwndDlg, IDC_CHECK_GREENZONE, BM_GETCHECK, 0, 0) == BST_CHECKED);
	taseditor_config.savecompact_history = (SendDlgItemMessage(hwndDlg, IDC_CHECK_HISTORY, BM_GETCHECK, 0, 0) == BST_CHECKED);
	taseditor_config.savecompact_piano_roll = (SendDlgItemMessage(hwndDlg, IDC_CHECK_PIANO_ROLL, BM_GETCHECK, 0, 0) == BST_CHECKED);
	taseditor_config.savecompact_selection = (SendDlgItemMessage(hwndDlg, IDC_CHECK_SELECTION, BM_GETCHECK, 0, 0) == BST_CHECKED);
}

BOOL CALLBACK AboutProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			SendMessage(GetDlgItem(hWnd, IDC_TASEDITOR_NAME), WM_SETFONT, (WPARAM)piano_roll.hTaseditorAboutFont, 0);
			break;
		}
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDCANCEL:
					EndDialog(hWnd, 0);
					return TRUE;
			}
			break;
		}
	}
	return FALSE; 
}

BOOL CALLBACK SaveCompactProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			SetWindowPos(hwndDlg, 0, taseditor_config.wndx + 100, taseditor_config.wndy + 200, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
			CheckDlgButton(hwndDlg, IDC_CHECK_BINARY, taseditor_config.savecompact_binary?MF_CHECKED : MF_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_CHECK_MARKERS, taseditor_config.savecompact_markers?MF_CHECKED : MF_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_CHECK_BOOKMARKS, taseditor_config.savecompact_bookmarks?MF_CHECKED : MF_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_CHECK_GREENZONE, taseditor_config.savecompact_greenzone?MF_CHECKED : MF_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_CHECK_HISTORY, taseditor_config.savecompact_history?MF_CHECKED : MF_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_CHECK_PIANO_ROLL, taseditor_config.savecompact_piano_roll?MF_CHECKED : MF_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_CHECK_SELECTION, taseditor_config.savecompact_selection?MF_CHECKED : MF_UNCHECKED);
			return TRUE;
		}
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDOK:
					SaveCompact_GetCheckboxes(hwndDlg);
					EndDialog(hwndDlg, 1);
					return TRUE;
				case IDCANCEL:
					SaveCompact_GetCheckboxes(hwndDlg);
					EndDialog(hwndDlg, 0);
					return TRUE;
			}
			break;
		}
		case WM_CLOSE:
		case WM_QUIT:
		{
			SaveCompact_GetCheckboxes(hwndDlg);
			break;
		}
	}
	return FALSE; 
}

void SaveCompact()
{
	if (DialogBox(fceu_hInstance, MAKEINTRESOURCE(IDD_TASEDITOR_SAVECOMPACT), taseditor_window.hwndTasEditor, SaveCompactProc) > 0)
	{
		const char filter[] = "TAS Editor Projects (*.fm3)\0*.fm3\0All Files (*.*)\0*.*\0\0";
		OPENFILENAME ofn;
		memset(&ofn, 0, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = taseditor_window.hwndTasEditor;
		ofn.hInstance = fceu_hInstance;
		ofn.lpstrTitle = "Save Compact";
		ofn.lpstrFilter = filter;

		char nameo[2048];
		if (project.GetProjectName().empty())
			// suggest ROM name for this project
			strcpy(nameo, mass_replace(GetRomName(), "|", ".").c_str());	//convert | to . for archive filenames
		else
			// suggest current name
			strcpy(nameo, project.GetProjectName().c_str());
		// add "-compact" if there's no such suffix
		if (!strstr(nameo, "-compact"))
			strcat(nameo, "-compact");

		ofn.lpstrFile = nameo;
		ofn.lpstrDefExt = "fm3";
		ofn.nMaxFile = 2048;
		ofn.Flags = OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;
		string initdir = FCEU_GetPath(FCEUMKF_MOVIE);			//Initial directory
		ofn.lpstrInitialDir = initdir.c_str();

		if(GetSaveFileName(&ofn))								//If it is a valid filename
			project.save(nameo, taseditor_config.savecompact_binary, taseditor_config.savecompact_markers, taseditor_config.savecompact_bookmarks, taseditor_config.savecompact_greenzone, taseditor_config.savecompact_history, taseditor_config.savecompact_piano_roll, taseditor_config.savecompact_selection);
	}
}

// returns false if user doesn't want to exit
bool AskSaveProject()
{
	bool changes_found = false;
	if (project.GetProjectChanged()) changes_found = true;

	// ask saving project
	if (changes_found)
	{
		int answer = MessageBox(taseditor_window.hwndTasEditor, "Save Project changes?", "TAS Editor", MB_YESNOCANCEL);
		if(answer == IDYES)
			return SaveProject();
		return (answer != IDCANCEL);
	}
	return true;
}

extern bool LoadFM2(MovieData& movieData, EMUFILE* fp, int size, bool stopAfterHeader);
void Import()
{
	const char filter[] = "FCEUX Movie Files (*.fm2), TAS Editor Projects (*.fm3)\0*.fm2;*.fm3\0All Files (*.*)\0*.*\0\0";
	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = taseditor_window.hwndTasEditor;
	ofn.hInstance = fceu_hInstance;
	ofn.lpstrTitle = "Import";
	ofn.lpstrFilter = filter;
	char nameo[2048] = {0};
	ofn.lpstrFile = nameo;							
	ofn.nMaxFile = 2048;
	ofn.Flags = OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_FILEMUSTEXIST;
	string initdir = FCEU_GetPath(FCEUMKF_MOVIE);	
	ofn.lpstrInitialDir = initdir.c_str();

	if(GetOpenFileName(&ofn))
	{							
		EMUFILE_FILE ifs(nameo, "rb");
		// Load input to temporary moviedata
		MovieData md;
		if (LoadFM2(md, &ifs, ifs.size(), false))
		{
			// loaded successfully, now register input changes
			char drv[512], dir[512], name[1024], ext[512];
			splitpath(nameo, drv, dir, name, ext);
			strcat(name, ext);
			history.RegisterImport(md, name);
		} else
		{
			FCEUD_PrintError("Error loading movie data!");
		}
	}
}

BOOL CALLBACK ExportProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
			SetWindowPos(hwndDlg, 0, taseditor_config.wndx + 100, taseditor_config.wndy + 200, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
			switch (taseditor_config.last_export_type)
			{
			case INPUT_TYPE_1P:
				{
					Button_SetCheck(GetDlgItem(hwndDlg, IDC_RADIO_1PLAYER), BST_CHECKED);
					break;
				}
			case INPUT_TYPE_2P:
				{
					Button_SetCheck(GetDlgItem(hwndDlg, IDC_RADIO_2PLAYERS), BST_CHECKED);
					break;
				}
			case INPUT_TYPE_FOURSCORE:
				{
					Button_SetCheck(GetDlgItem(hwndDlg, IDC_RADIO_FOURSCORE), BST_CHECKED);
					break;
				}
			}
			CheckDlgButton(hwndDlg, IDC_NOTES_TO_SUBTITLES, taseditor_config.last_export_subtitles?MF_CHECKED : MF_UNCHECKED);
			return TRUE;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_RADIO_1PLAYER:
					taseditor_config.last_export_type = INPUT_TYPE_1P;
					break;
				case IDC_RADIO_2PLAYERS:
					taseditor_config.last_export_type = INPUT_TYPE_2P;
					break;
				case IDC_RADIO_FOURSCORE:
					taseditor_config.last_export_type = INPUT_TYPE_FOURSCORE;
					break;
				case IDC_NOTES_TO_SUBTITLES:
					taseditor_config.last_export_subtitles ^= 1;
					CheckDlgButton(hwndDlg, IDC_NOTES_TO_SUBTITLES, taseditor_config.last_export_subtitles?MF_CHECKED : MF_UNCHECKED);
					break;
				case IDOK:
					EndDialog(hwndDlg, 1);
					return TRUE;
				case IDCANCEL:
					EndDialog(hwndDlg, 0);
					return TRUE;
			}
			break;
	}
	return FALSE; 
} 

void Export()
{
	if (DialogBox(fceu_hInstance, MAKEINTRESOURCE(IDD_TASEDITOR_EXPORT), taseditor_window.hwndTasEditor, ExportProc) > 0)
	{
		const char filter[] = "FCEUX Movie File (*.fm2)\0*.fm2\0All Files (*.*)\0*.*\0\0";
		char fname[2048];
		strcpy(fname, project.GetFM2Name().c_str());
		OPENFILENAME ofn;
		memset(&ofn, 0, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = taseditor_window.hwndTasEditor;
		ofn.hInstance = fceu_hInstance;
		ofn.lpstrTitle = "Export to FM2";
		ofn.lpstrFilter = filter;
		ofn.lpstrFile = fname;
		ofn.lpstrDefExt = "fm2";
		ofn.nMaxFile = 2048;
		ofn.Flags = OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;
		std::string initdir = FCEU_GetPath(FCEUMKF_MOVIE);
		ofn.lpstrInitialDir = initdir.c_str();
		if(GetSaveFileName(&ofn))
		{
			EMUFILE* osRecordingMovie = FCEUD_UTF8_fstream(fname, "wb");
			// create copy of current movie data
			MovieData temp_md = currMovieData;
			// modify the copy according to selected type of export
			SetInputType(temp_md, taseditor_config.last_export_type);
			temp_md.loadFrameCount = -1;
			// also add subtitles if needed
			if (taseditor_config.last_export_subtitles)
			{
				// convert Marker Notes to Movie Subtitles
				char framenum[11];
				std::string subtitle;
				int marker_id;
				for (int i = 0; i < markers_manager.GetMarkersSize(); ++i)
				{
					marker_id = markers_manager.GetMarker(i);
					if (marker_id)
					{
						_itoa(i, framenum, 10);
						strcat(framenum, " ");
						subtitle = framenum;
						subtitle.append(markers_manager.GetNote(marker_id));
						temp_md.subtitles.push_back(subtitle);
					}
				}
			}
			// dump to disk
			temp_md.dump(osRecordingMovie, false);
			delete osRecordingMovie;
			osRecordingMovie = 0;
		}
	}
}

int GetInputType(MovieData& md)
{
	if (md.fourscore)
		return INPUT_TYPE_FOURSCORE;
	else if (md.ports[0] == md.ports[1] == SI_GAMEPAD)
		return INPUT_TYPE_2P;
	else
		return INPUT_TYPE_1P;
}
void SetInputType(MovieData& md, int new_input_type)
{
	switch (new_input_type)
	{
		case INPUT_TYPE_1P:
		{
			md.fourscore = false;
			md.ports[0] = SI_GAMEPAD;
			md.ports[1] = SI_NONE;
			break;
		}
		case INPUT_TYPE_2P:
		{
			md.fourscore = false;
			md.ports[0] = SI_GAMEPAD;
			md.ports[1] = SI_GAMEPAD;
			break;
		}
		case INPUT_TYPE_FOURSCORE:
		{
			md.fourscore = true;
			md.ports[0] = SI_GAMEPAD;
			md.ports[1] = SI_GAMEPAD;
			break;
		}
	}
}

// this getter contains formula to decide whether to record or replay movie
bool TaseditorIsRecording()
{
	if (movie_readonly || playback.pause_frame > currFrameCounter)
		return false;		// replay
	return true;			// record
}