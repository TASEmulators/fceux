#include <fstream>
#include "taseditlib/taseditor_project.h"
#include "utils/xstring.h"
#include "keyboard.h"
#include "joystick.h"
#include "main.h"			// for GetRomName
#include "taseditor.h"
#include "version.h"
#include <Shlwapi.h>		// for StrStrI

#pragma comment(lib, "Shlwapi.lib")

using namespace std;

// TAS Editor data
bool Taseditor_rewind_now = false;
bool must_call_manual_lua_function = false;
// note editing/search (probably should be moved to separate class/module)
int marker_note_edit = MARKER_NOTE_EDIT_NONE;
char findnote_string[MAX_NOTE_LEN] = {0};
int search_similar_marker = 0;

// all Taseditor functional modules
TASEDITOR_CONFIG taseditor_config;
TASEDITOR_WINDOW taseditor_window;
TASEDITOR_PROJECT project;
INPUT_HISTORY history;
PLAYBACK playback;
RECORDER recorder;
GREENZONE greenzone;
MARKERS current_markers;
BOOKMARKS bookmarks;
POPUP_DISPLAY popup_display;
TASEDITOR_LIST list;
TASEDITOR_LUA taseditor_lua;
TASEDITOR_SELECTION selection;
SPLICER splicer;

extern int joysticks_per_frame[NUM_SUPPORTED_INPUT_TYPES];
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

// enterframe function
void UpdateTasEditor()
{
	if(!taseditor_window.hwndTasEditor)
	{
		// TAS Editor is not engaged... but we still should run Lua auto function
		TaseditorAutoFunction();
		return;
	}

	// update all modules that need to be updated every frame
	recorder.update();
	list.update();
	current_markers.update();
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

void SingleClick(LPNMITEMACTIVATE info)
{
	int row_index = info->iItem;
	if(row_index == -1) return;
	int column_index = info->iSubItem;

	if(column_index == COLUMN_ICONS)
	{
		// click on the "icons" column - jump to the frame
		selection.ClearSelection();
		playback.jump(row_index);
	} else if(column_index == COLUMN_FRAMENUM || column_index == COLUMN_FRAMENUM2)
	{
		// click on the "frame number" column - set marker if clicked with Alt
		if (info->uKeyFlags & LVKF_ALT)
		{
			// reverse MARKER_FLAG_BIT in pointed frame
			current_markers.ToggleMarker(row_index);
			selection.must_find_current_marker = playback.must_find_current_marker = true;
			if (current_markers.GetMarker(row_index))
				history.RegisterMarkersChange(MODTYPE_MARKER_SET, row_index);
			else
				history.RegisterMarkersChange(MODTYPE_MARKER_UNSET, row_index);
			list.RedrawRow(row_index);
		}
	}
	else if(column_index >= COLUMN_JOYPAD1_A && column_index <= COLUMN_JOYPAD4_R)
	{
		ToggleJoypadBit(column_index, row_index, info->uKeyFlags);
	}
}
void DoubleClick(LPNMITEMACTIVATE info)
{
	int row_index = info->iItem;
	if(row_index == -1) return;
	int column_index = info->iSubItem;

	if(column_index == COLUMN_ICONS || column_index == COLUMN_FRAMENUM || column_index == COLUMN_FRAMENUM2)
	{
		// double click sends playback to the frame
		selection.ClearSelection();
		playback.jump(row_index);
	} else if(column_index >= COLUMN_JOYPAD1_A && column_index <= COLUMN_JOYPAD4_R)
	{
		ToggleJoypadBit(column_index, row_index, info->uKeyFlags);
	}
}

void ToggleJoypadBit(int column_index, int row_index, UINT KeyFlags)
{
	int joy = (column_index - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS;
	int bit = (column_index - COLUMN_JOYPAD1_A) % NUM_JOYPAD_BUTTONS;
	if (KeyFlags & (LVKF_SHIFT|LVKF_CONTROL))
	{
		// update multiple rows, using last row index as a flag to decide operation
		SelectionFrames* current_selection = selection.MakeStrobe();
		SelectionFrames::iterator current_selection_end(current_selection->end());
		if (currMovieData.records[row_index].checkBit(joy, bit))
		{
			// clear range
			for(SelectionFrames::iterator it(current_selection->begin()); it != current_selection_end; it++)
			{
				currMovieData.records[*it].clearBit(joy, bit);
			}
			greenzone.InvalidateAndCheck(history.RegisterChanges(MODTYPE_UNSET, *current_selection->begin(), *current_selection->rbegin()));
		} else
		{
			// set range
			for(SelectionFrames::iterator it(current_selection->begin()); it != current_selection_end; it++)
			{
				currMovieData.records[*it].setBit(joy, bit);
			}
			greenzone.InvalidateAndCheck(history.RegisterChanges(MODTYPE_SET, *current_selection->begin(), *current_selection->rbegin()));
		}
	} else
	{
		// update one row
		currMovieData.records[row_index].toggleBit(joy, bit);
		if (currMovieData.records[row_index].checkBit(joy, bit))
			greenzone.InvalidateAndCheck(history.RegisterChanges(MODTYPE_SET, row_index, row_index));
		else
			greenzone.InvalidateAndCheck(history.RegisterChanges(MODTYPE_UNSET, row_index, row_index));
	}
	
}

void ColumnSet(int column)
{
	if (column == COLUMN_FRAMENUM || column == COLUMN_FRAMENUM2)
		FrameColumnSet();
	else
		InputColumnSet(column);
}
void FrameColumnSet()
{
	SelectionFrames* current_selection = selection.MakeStrobe();
	if (current_selection->size() == 0) return;
	SelectionFrames::iterator current_selection_begin(current_selection->begin());
	SelectionFrames::iterator current_selection_end(current_selection->end());

	// inspect the selected frames, if they are all set, then unset all, else set all
	bool unset_found = false, changes_made = false;
	for(SelectionFrames::iterator it(current_selection_begin); it != current_selection_end; it++)
	{
		if(!current_markers.GetMarker(*it))
		{
			unset_found = true;
			break;
		}
	}
	if (unset_found)
	{
		// set all
		for(SelectionFrames::iterator it(current_selection_begin); it != current_selection_end; it++)
		{
			if(!current_markers.GetMarker(*it))
			{
				if (current_markers.SetMarker(*it))
				{
					changes_made = true;
					list.RedrawRow(*it);
				}
			}
		}
		if (changes_made)
			history.RegisterMarkersChange(MODTYPE_MARKER_SET, *current_selection_begin, *current_selection->rbegin());
	} else
	{
		// unset all
		for(SelectionFrames::iterator it(current_selection_begin); it != current_selection_end; it++)
		{
			if(current_markers.GetMarker(*it))
			{
				current_markers.ClearMarker(*it);
				changes_made = true;
				list.RedrawRow(*it);
			}
		}
		if (changes_made)
			history.RegisterMarkersChange(MODTYPE_MARKER_UNSET, *current_selection_begin, *current_selection->rbegin());
	}
	if (changes_made)
	{
		selection.must_find_current_marker = playback.must_find_current_marker = true;
		list.SetHeaderColumnLight(COLUMN_FRAMENUM, HEADER_LIGHT_MAX);
	}
}
void InputColumnSet(int column)
{
	int joy = (column - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS;
	if (joy < 0 || joy >= joysticks_per_frame[GetInputType(currMovieData)]) return;
	int button = (column - COLUMN_JOYPAD1_A) % NUM_JOYPAD_BUTTONS;

	SelectionFrames* current_selection = selection.MakeStrobe();
	if (current_selection->size() == 0) return;
	SelectionFrames::iterator current_selection_begin(current_selection->begin());
	SelectionFrames::iterator current_selection_end(current_selection->end());

	//inspect the selected frames, if they are all set, then unset all, else set all
	bool newValue = false;
	for(SelectionFrames::iterator it(current_selection_begin); it != current_selection_end; it++)
	{
		if(!(currMovieData.records[*it].checkBit(joy,button)))
		{
			newValue = true;
			break;
		}
	}
	// apply newValue
	for(SelectionFrames::iterator it(current_selection_begin); it != current_selection_end; it++)
		currMovieData.records[*it].setBitValue(joy,button,newValue);
	if (newValue)
	{
		greenzone.InvalidateAndCheck(history.RegisterChanges(MODTYPE_SET, *current_selection_begin, *current_selection->rbegin()));
	} else
	{
		greenzone.InvalidateAndCheck(history.RegisterChanges(MODTYPE_UNSET, *current_selection_begin, *current_selection->rbegin()));
	}
	list.SetHeaderColumnLight(column, HEADER_LIGHT_MAX);
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
			current_markers.reset();
		if(params.author_name != L"") currMovieData.comments.push_back(L"author " + params.author_name);
		
		// reset Taseditor
		greenzone.reset();
		playback.reset();
		playback.StartFromZero();
		bookmarks.reset();
		history.reset();
		list.reset();
		selection.reset();
		splicer.reset();
		recorder.reset();
		popup_display.reset();
		project.reset();
		taseditor_window.RedrawTaseditor();
		taseditor_window.UpdateCaption();
		search_similar_marker = 0;
		marker_note_edit = MARKER_NOTE_EDIT_NONE;
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
	marker_note_edit = MARKER_NOTE_EDIT_NONE;
	SetFocus(list.hwndList);
	// try to load project
	if (project.load(fullname))
	{
		// update FCEUX input config
		FCEUD_SetInput(currMovieData.fourscore, currMovieData.microphone, (ESI)currMovieData.ports[0], (ESI)currMovieData.ports[1], (ESIFC)currMovieData.ports[2]);
		// add new file to Recent menu
		taseditor_window.UpdateRecentProjectsArray(fullname);
		taseditor_window.RedrawTaseditor();
		taseditor_window.UpdateCaption();
		search_similar_marker = 0;
		return true;
	} else
	{
		// failed to load
		taseditor_window.RedrawTaseditor();
		taseditor_window.UpdateCaption();
		search_similar_marker = 0;
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
		// suggest ROM name for this project
		strcpy(nameo, mass_replace(GetRomName(), "|", ".").c_str());	//convert | to . for archive filenames
	else
		// suggest current name
		strncpy(nameo, project.GetProjectName().c_str(), 2047);

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
	if (!project.save())
		return SaveProjectAs();
	else
		taseditor_window.UpdateCaption();
	return true;
}

void SaveCompact_GetCheckboxes(HWND hwndDlg)
{
	taseditor_config.savecompact_binary = (SendDlgItemMessage(hwndDlg, IDC_CHECK_BINARY, BM_GETCHECK, 0, 0) == BST_CHECKED);
	taseditor_config.savecompact_markers = (SendDlgItemMessage(hwndDlg, IDC_CHECK_MARKERS, BM_GETCHECK, 0, 0) == BST_CHECKED);
	taseditor_config.savecompact_bookmarks = (SendDlgItemMessage(hwndDlg, IDC_CHECK_BOOKMARKS, BM_GETCHECK, 0, 0) == BST_CHECKED);
	taseditor_config.savecompact_greenzone = (SendDlgItemMessage(hwndDlg, IDC_CHECK_GREENZONE, BM_GETCHECK, 0, 0) == BST_CHECKED);
	taseditor_config.savecompact_history = (SendDlgItemMessage(hwndDlg, IDC_CHECK_HISTORY, BM_GETCHECK, 0, 0) == BST_CHECKED);
	taseditor_config.savecompact_list = (SendDlgItemMessage(hwndDlg, IDC_CHECK_LIST, BM_GETCHECK, 0, 0) == BST_CHECKED);
	taseditor_config.savecompact_selection = (SendDlgItemMessage(hwndDlg, IDC_CHECK_SELECTION, BM_GETCHECK, 0, 0) == BST_CHECKED);
}

BOOL CALLBACK AboutProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDCANCEL:
					EndDialog(hwndDlg, 0);
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
			CheckDlgButton(hwndDlg, IDC_CHECK_LIST, taseditor_config.savecompact_list?MF_CHECKED : MF_UNCHECKED);
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
		{
			project.save_compact(nameo, taseditor_config.savecompact_binary, taseditor_config.savecompact_markers, taseditor_config.savecompact_bookmarks, taseditor_config.savecompact_greenzone, taseditor_config.savecompact_history, taseditor_config.savecompact_list, taseditor_config.savecompact_selection);
		}
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
				for (int i = 0; i < current_markers.GetMarkersSize(); ++i)
				{
					marker_id = current_markers.GetMarker(i);
					if (marker_id)
					{
						_itoa(i, framenum, 10);
						strcat(framenum, " ");
						subtitle = framenum;
						subtitle.append(current_markers.GetNote(marker_id));
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

bool EnterTasEditor()
{
	if(!FCEU_IsValidUI(FCEUI_TASEDITOR)) return false;
	if(!taseditor_window.hwndTasEditor)
	{
		// start TAS Editor
		taseditor_window.init();
		if(taseditor_window.hwndTasEditor)
		{
			SetTaseditInput();
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
			list.init();
			selection.init();
			splicer.init();
			playback.init();
			greenzone.init();
			recorder.init();
			current_markers.init();
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
				currMovieData.emuVersion = FCEU_VERSION_NUMERIC;
				greenzone.TryDumpIncremental(lagFlag != 0);
			}
			// ensure that movie has correct set of ports/fourscore
			SetInputType(currMovieData, GetInputType(currMovieData));
			// force the input configuration stored in the movie to apply to FCEUX config
			FCEUD_SetInput(currMovieData.fourscore, currMovieData.microphone, (ESI)currMovieData.ports[0], (ESI)currMovieData.ports[1], (ESIFC)currMovieData.ports[2]);
			// reset some modules that need MovidData info
			list.reset();
			recorder.reset();
			// create initial snapshot in history
			history.reset();
			// reset Taseditor variables
			must_call_manual_lua_function = false;
			marker_note_edit = MARKER_NOTE_EDIT_NONE;
			search_similar_marker = 0;
			
			SetFocus(history.hwndHistoryList);		// set focus only once, to show selection cursor
			SetFocus(list.hwndList);
			FCEU_DispMessage("TAS Editor engaged", 0);
			return true;
		} else return false;
	} else return true;
}

bool ExitTasEditor()
{
	if (!AskSaveProject()) return false;

	// destroy window
	taseditor_window.exit();
	// release memory
	list.free();
	current_markers.free();
	greenzone.free();
	bookmarks.free();
	popup_display.free();
	history.free();
	playback.SeekingStop();
	selection.free();
	splicer.free();

	ClearTaseditInput();
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

void SetTaseditInput()
{
	// set "Background TAS Editor input"
	KeyboardSetBackgroundAccessBit(KEYBACKACCESS_TASEDITOR);
	JoystickSetBackgroundAccessBit(JOYBACKACCESS_TASEDITOR);
}
void ClearTaseditInput()
{
	// clear "Background TAS Editor input"
	KeyboardClearBackgroundAccessBit(KEYBACKACCESS_TASEDITOR);
	JoystickClearBackgroundAccessBit(JOYBACKACCESS_TASEDITOR);
}

void UpdateMarkerNote()
{
	if (!marker_note_edit) return;
	char new_text[MAX_NOTE_LEN];
	if (marker_note_edit == MARKER_NOTE_EDIT_UPPER)
	{
		int len = SendMessage(playback.hwndPlaybackMarkerEdit, WM_GETTEXT, MAX_NOTE_LEN, (LPARAM)new_text);
		new_text[len] = 0;
		// check changes
		if (strcmp(current_markers.GetNote(playback.shown_marker).c_str(), new_text))
		{
			current_markers.SetNote(playback.shown_marker, new_text);
			if (playback.shown_marker)
				history.RegisterMarkersChange(MODTYPE_MARKER_RENAME, current_markers.GetMarkerFrame(playback.shown_marker));
			else
				// zeroth marker - just assume it's set on frame 0
				history.RegisterMarkersChange(MODTYPE_MARKER_RENAME, 0);
			// notify selection to change text in lower marker (in case both are showing same marker)
			selection.must_find_current_marker = true;
		}
	} else if (marker_note_edit == MARKER_NOTE_EDIT_LOWER)
	{
		int len = SendMessage(selection.hwndSelectionMarkerEdit, WM_GETTEXT, MAX_NOTE_LEN, (LPARAM)new_text);
		new_text[len] = 0;
		// check changes
		if (strcmp(current_markers.GetNote(selection.shown_marker).c_str(), new_text))
		{
			current_markers.SetNote(selection.shown_marker, new_text);
			if (selection.shown_marker)
				history.RegisterMarkersChange(MODTYPE_MARKER_RENAME, current_markers.GetMarkerFrame(selection.shown_marker));
			else
				// zeroth marker - just assume it's set on frame 0
				history.RegisterMarkersChange(MODTYPE_MARKER_RENAME, 0);
			// notify playback to change text in upper marker (in case both are showing same marker)
			playback.must_find_current_marker = true;
		}
	}
}

BOOL CALLBACK FindNoteProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			if (taseditor_config.findnote_wndx == -32000) taseditor_config.findnote_wndx = 0; //Just in case
			if (taseditor_config.findnote_wndy == -32000) taseditor_config.findnote_wndy = 0;
			SetWindowPos(hwndDlg, 0, taseditor_config.findnote_wndx, taseditor_config.findnote_wndy, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);

			CheckDlgButton(hwndDlg, IDC_MATCH_CASE, taseditor_config.findnote_matchcase?MF_CHECKED : MF_UNCHECKED);
			if (taseditor_config.findnote_search_up)
				Button_SetCheck(GetDlgItem(hwndDlg, IDC_RADIO_UP), BST_CHECKED);
			else
				Button_SetCheck(GetDlgItem(hwndDlg, IDC_RADIO_DOWN), BST_CHECKED);
			HWND hwndEdit = GetDlgItem(hwndDlg, IDC_NOTE_TO_FIND);
			SendMessage(hwndEdit, EM_SETLIMITTEXT, MAX_NOTE_LEN - 1, 0);
			SetWindowText(hwndEdit, findnote_string);
			if (GetDlgCtrlID((HWND)wParam) != IDC_NOTE_TO_FIND)
		    {
				SetFocus(hwndEdit);
				return false;
			}
			return true;
		}
		case WM_MOVE:
		{
			if (!IsIconic(hwndDlg))
			{
				RECT wrect;
				GetWindowRect(hwndDlg, &wrect);
				taseditor_config.findnote_wndx = wrect.left;
				taseditor_config.findnote_wndy = wrect.top;
				WindowBoundsCheckNoResize(taseditor_config.findnote_wndx, taseditor_config.findnote_wndy, wrect.right);
			}
			break;
		}
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDC_NOTE_TO_FIND:
				{
					if(HIWORD(wParam) == EN_CHANGE) 
					{
						if (GetWindowTextLength(GetDlgItem(hwndDlg, IDC_NOTE_TO_FIND)))
							EnableWindow(GetDlgItem(hwndDlg, IDOK), true);
						else
							EnableWindow(GetDlgItem(hwndDlg, IDOK), false);
					}
					break;
				}
				case IDC_RADIO_UP:
					taseditor_config.findnote_search_up = true;
					break;
				case IDC_RADIO_DOWN:
					taseditor_config.findnote_search_up = false;
					break;
				case IDC_MATCH_CASE:
					taseditor_config.findnote_matchcase ^= 1;
					CheckDlgButton(hwndDlg, IDC_MATCH_CASE, taseditor_config.findnote_matchcase?MF_CHECKED : MF_UNCHECKED);
					break;
				case IDOK:
				{
					int len = SendMessage(GetDlgItem(hwndDlg, IDC_NOTE_TO_FIND), WM_GETTEXT, MAX_NOTE_LEN, (LPARAM)findnote_string);
					findnote_string[len] = 0;
					// scan frames from current selection to the border
					int cur_marker = 0;
					bool result;
					int movie_size = currMovieData.getNumRecords();
					int current_frame = selection.GetCurrentSelectionBeginning();
					if (current_frame < 0 && taseditor_config.findnote_search_up)
						current_frame = movie_size;
					while (true)
					{
						// move forward
						if (taseditor_config.findnote_search_up)
						{
							current_frame--;
							if (current_frame < 0)
							{
								MessageBox(taseditor_window.hwndFindNote, "Nothing was found.", "Find Note", MB_OK);
								break;
							}
						} else
						{
							current_frame++;
							if (current_frame >= movie_size)
							{
								MessageBox(taseditor_window.hwndFindNote, "Nothing was found!", "Find Note", MB_OK);
								break;
							}
						}
						// scan marked frames
						cur_marker = current_markers.GetMarker(current_frame);
						if (cur_marker)
						{
							if (taseditor_config.findnote_matchcase)
								result = (strstr(current_markers.GetNote(cur_marker).c_str(), findnote_string) != 0);
							else
								result = (StrStrI(current_markers.GetNote(cur_marker).c_str(), findnote_string) != 0);
							if (result)
							{
								// found note containing searched string - jump there
								selection.JumpToFrame(current_frame);
								break;
							}
						}
					}
					return TRUE;
				}
				case IDCANCEL:
					DestroyWindow(taseditor_window.hwndFindNote);
					taseditor_window.hwndFindNote = 0;
					return TRUE;
			}
			break;
		}
		case WM_CLOSE:
		case WM_QUIT:
		{
			DestroyWindow(taseditor_window.hwndFindNote);
			taseditor_window.hwndFindNote = 0;
			break;
		}
	}
	return FALSE; 
} 

