#include <fstream>
#include <sstream>
#include "taseditlib/taseditproj.h"
#include "utils/xstring.h"
#include "Win32InputBox.h"
#include "keyboard.h"
#include "joystick.h"
#include "help.h"
#include "main.h"
#include "tasedit.h"
#include "version.h"
#include <Shlwapi.h>		// for StrStrI

#pragma comment(lib, "Shlwapi.lib")

using namespace std;

HWND hwndTasEdit = 0;
HMENU hmenu, hrmenu;

bool TASEdit_focus = false;
bool Tasedit_rewind_now = false;

int marker_note_edit = MARKER_NOTE_EDIT_NONE;
char findnote_string[MAX_NOTE_LEN] = {0};

// all Taseditor functional modules
TASEDIT_PROJECT project;
INPUT_HISTORY history;
PLAYBACK playback;
RECORDER recorder;
GREENZONE greenzone;
MARKERS current_markers;
BOOKMARKS bookmarks;
SCREENSHOT_DISPLAY screenshot_display;
TASEDIT_LIST tasedit_list;
TASEDIT_SELECTION selection;

// saved FCEU config
int saved_eoptions;
int saved_EnableAutosave;
extern int EnableAutosave;

extern EMOVIEMODE movieMode;	// maybe we need normal setter for movieMode, to encapsulate it
extern void UpdateCheckedMenuItems();

// vars saved in cfg file (need dedicated storage class?)
int TasEdit_wndx, TasEdit_wndy;
bool TASEdit_follow_playback = true;
bool TASEdit_turbo_seek = true;
bool TASEdit_show_lag_frames = true;
bool TASEdit_show_markers = true;
bool TASEdit_show_branch_screenshots = true;
bool TASEdit_show_branch_tooltips = true;
bool TASEdit_bind_markers = true;
bool TASEdit_empty_marker_notes = true;
bool TASEdit_combine_consecutive_rec = true;
bool TASEdit_use_1p_rec = true;
bool TASEdit_columnset_by_keys = true;
bool TASEdit_keyboard_for_listview = true;
bool TASEdit_superimpose_affects_paste = true;
int TASEdit_superimpose = BST_UNCHECKED;
bool TASEdit_branch_full_movie = true;
bool TASEdit_branch_only_when_rec = false;
bool TASEdit_view_branches_tree = false;
bool TASEdit_branch_scr_hud = true;
bool TASEdit_restore_position = false;
int TASEdit_greenzone_capacity = GREENZONE_CAPACITY_DEFAULT;
int TasEdit_undo_levels = UNDO_LEVELS_DEFAULT;
int TASEdit_autosave_period = AUTOSAVE_PERIOD_DEFAULT;
extern bool muteTurbo;
bool TASEdit_enable_hot_changes = true;
bool TASEdit_jump_to_undo = true;
bool TASEdit_follow_note_context = true;
int TASEdit_last_export_type = EXPORT_TYPE_1P;
bool TASEdit_last_export_subtitles = false;
bool TASEdit_savecompact_binary = true;
bool TASEdit_savecompact_markers = true;
bool TASEdit_savecompact_bookmarks = true;
bool TASEdit_savecompact_greenzone = false;
bool TASEdit_savecompact_history = false;
bool TASEdit_savecompact_list = true;
bool TASEdit_savecompact_selection = false;
bool TASEdit_findnote_matchcase = false;
bool TASEdit_findnote_search_up = false;
bool TASEdit_findnote_reappear = true;

// Recent Menu
HMENU recent_projects_menu;
char* recent_projects[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
const unsigned int MENU_FIRST_RECENT_PROJECT = 55000;
const unsigned int MAX_NUMBER_OF_RECENT_PROJECTS = sizeof(recent_projects)/sizeof(*recent_projects);

// resources
string tasedithelp = "{16CDE0C4-02B0-4A60-A88D-076319909A4D}"; //Name of TAS Editor Help page
char buttonNames[NUM_JOYPAD_BUTTONS][2] = {"A", "B", "S", "T", "U", "D", "L", "R"};
char windowCaptioBase[] = "TAS Editor";
HICON hTaseditorIcon = 0;

// enterframe function
void UpdateTasEdit()
{
	if(!hwndTasEdit) return;

	recorder.update();
	tasedit_list.update();
	current_markers.update();
	greenzone.update();
	playback.update();
	bookmarks.update();
	screenshot_display.update();
	selection.update();
	history.update();
	project.update();
}

void RedrawWindowCaption()
{
	char new_caption[300];
	strcpy(new_caption, windowCaptioBase);
	if (!movie_readonly)
		strcat(new_caption, recorder.GetRecordingCaption());
	// add project name
	std::string projectname = project.GetProjectName();
	if (!projectname.empty())
	{
		strcat(new_caption, " - ");
		strcat(new_caption, projectname.c_str());
	}
	// and * if project has unsaved changes
	if (project.GetProjectChanged())
		strcat(new_caption, "*");
	SetWindowText(hwndTasEdit, new_caption);
}
void RedrawTasedit()
{
	InvalidateRect(hwndTasEdit, 0, FALSE);
}

void StrayClickMenu(LPNMITEMACTIVATE info)
{
	POINT pt = info->ptAction;
	ClientToScreen(tasedit_list.hwndList, &pt);
	HMENU sub = GetSubMenu(hrmenu, CONTEXTMENU_STRAY);
	TrackPopupMenu(sub, 0, pt.x, pt.y, 0, hwndTasEdit, 0);
}
void RightClickMenu(LPNMITEMACTIVATE info)
{
	POINT pt = info->ptAction;
	ClientToScreen(tasedit_list.hwndList, &pt);

	SelectionFrames* current_selection = selection.MakeStrobe();
	if (current_selection->size() == 0)
	{
		StrayClickMenu(info);
		return;
	}
	HMENU sub = GetSubMenu(hrmenu, CONTEXTMENU_SELECTED);
	// inspect current selection and disable inappropriate menu items
	SelectionFrames::iterator current_selection_begin(current_selection->begin());
	SelectionFrames::iterator current_selection_end(current_selection->end());
	bool set_found = false, unset_found = false;
	for(SelectionFrames::iterator it(current_selection_begin); it != current_selection_end; it++)
	{
		if(current_markers.GetMarker(*it))
			set_found = true;
		else 
			unset_found = true;
	}
	if (set_found)
		EnableMenuItem(sub, ID_SELECTED_REMOVEMARKER, MF_BYCOMMAND | MF_ENABLED);
	else
		EnableMenuItem(sub, ID_SELECTED_REMOVEMARKER, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	if (unset_found)
		EnableMenuItem(sub, ID_SELECTED_SETMARKER, MF_BYCOMMAND | MF_ENABLED);
	else
		EnableMenuItem(sub, ID_SELECTED_SETMARKER, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

	TrackPopupMenu(sub, 0, pt.x, pt.y, 0, hwndTasEdit, 0);
}
void RightClick(LPNMITEMACTIVATE info)
{
	int index = info->iItem;
	if(index == -1)
		StrayClickMenu(info);
	else if (selection.CheckFrameSelected(index))
		RightClickMenu(info);
}

void ToggleJoypadBit(int column_index, int row_index, UINT KeyFlags)
{
	int joy = (column_index - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS;
	int bit = (column_index - COLUMN_JOYPAD1_A) % NUM_JOYPAD_BUTTONS;
	if (KeyFlags & (LVKF_SHIFT|LVKF_CONTROL))
	{
		//update multiple rows
		SelectionFrames* current_selection = selection.MakeStrobe();
		SelectionFrames::iterator current_selection_end(current_selection->end());
		for(SelectionFrames::iterator it(current_selection->begin()); it != current_selection_end; it++)
		{
			currMovieData.records[*it].toggleBit(joy,bit);
		}
		greenzone.InvalidateAndCheck(history.RegisterChanges(MODTYPE_CHANGE, *current_selection->begin(), *current_selection->rbegin()));
	} else
	{
		//update one row
		currMovieData.records[row_index].toggleBit(joy,bit);
		if (currMovieData.records[row_index].checkBit(joy,bit))
			greenzone.InvalidateAndCheck(history.RegisterChanges(MODTYPE_SET, row_index, row_index));
		else
			greenzone.InvalidateAndCheck(history.RegisterChanges(MODTYPE_UNSET, row_index, row_index));
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
			tasedit_list.RedrawRow(row_index);
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

void CloneFrames()
{
	SelectionFrames* current_selection = selection.MakeStrobe();
	int frames = current_selection->size();
	if (!frames) return;

	currMovieData.records.reserve(currMovieData.getNumRecords() + frames);
	//insert frames before each selection, but consecutive selection lines are accounted as single region
	frames = 1;
	SelectionFrames::reverse_iterator next_it;
	SelectionFrames::reverse_iterator current_selection_rend = current_selection->rend();
	for(SelectionFrames::reverse_iterator it(current_selection->rbegin()); it != current_selection_rend; it++)
	{
		next_it = it;
		next_it++;
		if (next_it == current_selection_rend || (int)*next_it < ((int)*it - 1))
		{
			// end of current region
			currMovieData.cloneRegion(*it, frames);
			if (TASEdit_bind_markers)
				current_markers.insertEmpty(*it, frames);
			frames = 1;
		} else frames++;
	}
	if (TASEdit_bind_markers)
	{
		current_markers.update();
		selection.must_find_current_marker = playback.must_find_current_marker = true;
	}
	greenzone.InvalidateAndCheck(history.RegisterChanges(MODTYPE_CLONE, *current_selection->begin()));
}

void InsertFrames()
{
	SelectionFrames* current_selection = selection.MakeStrobe();
	int frames = current_selection->size();
	if (!frames) return;

	//to keep this from being even slower than it would otherwise be, go ahead and reserve records
	currMovieData.records.reserve(currMovieData.getNumRecords() + frames);

	//insert frames before each selection, but consecutive selection lines are accounted as single region
	frames = 1;
	SelectionFrames::reverse_iterator next_it;
	SelectionFrames::reverse_iterator current_selection_rend = current_selection->rend();
	for(SelectionFrames::reverse_iterator it(current_selection->rbegin()); it != current_selection_rend; it++)
	{
		next_it = it;
		next_it++;
		if (next_it == current_selection_rend || (int)*next_it < ((int)*it - 1))
		{
			// end of current region
			currMovieData.insertEmpty(*it,frames);
			if (TASEdit_bind_markers)
				current_markers.insertEmpty(*it,frames);
			frames = 1;
		} else frames++;
	}
	if (TASEdit_bind_markers)
	{
		current_markers.update();
		selection.must_find_current_marker = playback.must_find_current_marker = true;
	}
	greenzone.InvalidateAndCheck(history.RegisterChanges(MODTYPE_INSERT, *current_selection->begin()));
}

void InsertNumFrames()
{
	SelectionFrames* current_selection = selection.MakeStrobe();
	int frames = current_selection->size();
	if(CWin32InputBox::GetInteger("Insert number of Frames", "How many frames?", frames, hwndTasEdit) == IDOK)
	{
		if (frames > 0)
		{
			int index;
			if (current_selection->size())
			{
				// insert at selection
				index = *current_selection->begin();
				selection.ClearSelection();
			} else
			{
				// insert at playback cursor
				index = currFrameCounter;
			}
			currMovieData.insertEmpty(index, frames);
			if (TASEdit_bind_markers)
			{
				current_markers.insertEmpty(index, frames);
				selection.must_find_current_marker = playback.must_find_current_marker = true;
			}
			// select inserted rows
			tasedit_list.update();
			selection.SetRegionSelection(index, index + frames - 1);
			greenzone.InvalidateAndCheck(history.RegisterChanges(MODTYPE_INSERT, index));
		}
	}
}

void DeleteFrames()
{
	SelectionFrames* current_selection = selection.MakeStrobe();
	if (current_selection->size() == 0) return;

	int start_index = *current_selection->begin();
	int end_index = *current_selection->rbegin();
	SelectionFrames::reverse_iterator current_selection_rend = current_selection->rend();
	//delete frames on each selection, going backwards
	for(SelectionFrames::reverse_iterator it(current_selection->rbegin()); it != current_selection_rend; it++)
	{
		currMovieData.records.erase(currMovieData.records.begin() + *it);
		if (TASEdit_bind_markers)
			current_markers.EraseMarker(*it);
	}
	if (TASEdit_bind_markers)
		selection.must_find_current_marker = playback.must_find_current_marker = true;
	// check if user deleted all frames
	if (!currMovieData.getNumRecords())
		playback.StartFromZero();
	// reduce list
	tasedit_list.update();

	int result = history.RegisterChanges(MODTYPE_DELETE, start_index);
	if (result >= 0)
	{
		greenzone.InvalidateAndCheck(result);
	} else if (greenzone.greenZoneCount >= currMovieData.getNumRecords())
	{
		greenzone.InvalidateAndCheck(currMovieData.getNumRecords()-1);
	} else tasedit_list.RedrawList();
}

void ClearFrames(SelectionFrames* current_selection)
{
	bool cut = true;
	if (!current_selection)
	{
		cut = false;
		current_selection = selection.MakeStrobe();
		if (current_selection->size() == 0) return;
	}

	//clear input on each selected frame
	SelectionFrames::iterator current_selection_end(current_selection->end());
	for(SelectionFrames::iterator it(current_selection->begin()); it != current_selection_end; it++)
	{
		currMovieData.records[*it].clear();
	}
	if (cut)
		greenzone.InvalidateAndCheck(history.RegisterChanges(MODTYPE_CUT, *current_selection->begin(), *current_selection->rbegin()));
	else
		greenzone.InvalidateAndCheck(history.RegisterChanges(MODTYPE_CLEAR, *current_selection->begin(), *current_selection->rbegin()));
}

void Truncate()
{
	int frame = selection.GetCurrentSelectionBeginning();
	if (frame < 0) frame = currFrameCounter;

	if (currMovieData.getNumRecords() > frame+1)
	{
		currMovieData.truncateAt(frame+1);
		if (TASEdit_bind_markers)
		{
			current_markers.SetMarkersSize(frame+1);
			selection.must_find_current_marker = playback.must_find_current_marker = true;
		}
		tasedit_list.update();
		int result = history.RegisterChanges(MODTYPE_TRUNCATE, frame+1);
		if (result >= 0)
		{
			greenzone.InvalidateAndCheck(result);
		} else if (greenzone.greenZoneCount >= currMovieData.getNumRecords())
		{
			greenzone.InvalidateAndCheck(currMovieData.getNumRecords()-1);
		} else tasedit_list.RedrawList();
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
					tasedit_list.RedrawRow(*it);
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
				tasedit_list.RedrawRow(*it);
			}
		}
		if (changes_made)
			history.RegisterMarkersChange(MODTYPE_MARKER_UNSET, *current_selection_begin, *current_selection->rbegin());
	}
	if (changes_made)
	{
		selection.must_find_current_marker = playback.must_find_current_marker = true;
		tasedit_list.SetHeaderColumnLight(COLUMN_FRAMENUM, HEADER_LIGHT_MAX);
	}
}
void InputColumnSet(int column)
{
	int joy = (column - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS;
	if (joy < 0 || joy >= NUM_JOYPADS) return;
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
	tasedit_list.SetHeaderColumnLight(column, HEADER_LIGHT_MAX);
}

bool Copy(SelectionFrames* current_selection)
{
	if (!current_selection)
	{
		current_selection = selection.MakeStrobe();
		if (current_selection->size() == 0) return false;
	}

	SelectionFrames::iterator current_selection_begin(current_selection->begin());
	SelectionFrames::iterator current_selection_end(current_selection->end());
	int cframe = (*current_selection_begin) - 1;
    try 
	{
		int range = (*current_selection->rbegin() - *current_selection_begin) + 1;

		std::stringstream clipString;
		clipString << "TAS " << range << std::endl;

		for(SelectionFrames::iterator it(current_selection_begin); it != current_selection_end; it++)
		{
			if (*it > cframe+1)
			{
				clipString << '+' << (*it-cframe) << '|';
			}
			cframe=*it;

			int cjoy=0;
			for (int joy = 0; joy < NUM_JOYPADS; ++joy)
			{
				while (currMovieData.records[*it].joysticks[joy] && cjoy<joy) 
				{
					clipString << '|';
					++cjoy;
				}
				for (int bit=0; bit<NUM_JOYPAD_BUTTONS; ++bit)
				{
					if (currMovieData.records[*it].joysticks[joy] & (1<<bit))
					{
						clipString << buttonNames[bit];
					}
				}
			}
			clipString << std::endl;

			if (!OpenClipboard(hwndTasEdit))
				return false;
			EmptyClipboard();

			HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, clipString.str().size()+1);

			if (hGlobal==INVALID_HANDLE_VALUE)
			{
				CloseClipboard();
				return false;
			}
			char *pGlobal = (char*)GlobalLock(hGlobal);
			strcpy(pGlobal, clipString.str().c_str());
			GlobalUnlock(hGlobal);
			SetClipboardData(CF_TEXT, hGlobal);

			CloseClipboard();
		}
		
	}
	catch (std::bad_alloc e)
	{
		return false;
	}
	// copied successfully
	selection.MemorizeClipboardSelection();
	return true;
}
void Cut()
{
	SelectionFrames* current_selection = selection.MakeStrobe();
	if (current_selection->size() == 0) return;

	if (Copy(current_selection))
	{
		ClearFrames(current_selection);
	}
}
bool Paste()
{
	SelectionFrames* current_selection = selection.MakeStrobe();
	if (current_selection->size() == 0) return false;

	if (!OpenClipboard(hwndTasEdit)) return false;

	SelectionFrames::iterator current_selection_begin(current_selection->begin());
	bool result = false;
	int pos = *current_selection_begin;
	HANDLE hGlobal = GetClipboardData(CF_TEXT);
	if (hGlobal)
	{
		char *pGlobal = (char*)GlobalLock((HGLOBAL)hGlobal);

		// TAS recording info starts with "TAS "
		if (pGlobal[0]=='T' && pGlobal[1]=='A' && pGlobal[2]=='S')
		{
			// Extract number of frames
			int range;
			sscanf (pGlobal+3, "%d", &range);
			if (currMovieData.getNumRecords() < pos+range)
			{
				currMovieData.insertEmpty(currMovieData.getNumRecords(),pos+range-currMovieData.getNumRecords());
				current_markers.update();
			}

			pGlobal = strchr(pGlobal, '\n');
			int joy = 0;
			uint8 new_buttons = 0;
			std::vector<uint8> flash_joy(NUM_JOYPADS);
			char* frame;
			--pos;
			while (pGlobal++ && *pGlobal!='\0')
			{
				// Detect skipped frames in paste
				frame = pGlobal;
				if (frame[0]=='+')
				{
					pos += atoi(frame+1);
					while (*frame && *frame != '\n' && *frame!='|')
						++frame;
					if (*frame=='|') ++frame;
				} else
				{
					++pos;
				}
				
				if (!TASEdit_superimpose_affects_paste || TASEdit_superimpose == BST_UNCHECKED)
				{
					currMovieData.records[pos].joysticks[0] = 0;
					currMovieData.records[pos].joysticks[1] = 0;
					currMovieData.records[pos].joysticks[2] = 0;
					currMovieData.records[pos].joysticks[3] = 0;
				}
				// read this frame input
				joy = 0;
				new_buttons = 0;
				while (*frame && *frame != '\n' && *frame !='\r')
				{
					switch (*frame)
					{
					case '|': // Joystick mark
						// flush buttons to movie data
						if (TASEdit_superimpose_affects_paste && (TASEdit_superimpose == BST_CHECKED || (TASEdit_superimpose == BST_INDETERMINATE && new_buttons == 0)))
						{
							flash_joy[joy] |= (new_buttons & (~currMovieData.records[pos].joysticks[joy]));		// highlight buttons that are new
							currMovieData.records[pos].joysticks[joy] |= new_buttons;
						} else
						{
							flash_joy[joy] |= new_buttons;		// highlight buttons that were added
							currMovieData.records[pos].joysticks[joy] = new_buttons;
						}
						++joy;
						new_buttons = 0;
						break;
					default:
						for (int bit = 0; bit < NUM_JOYPAD_BUTTONS; ++bit)
						{
							if (*frame == buttonNames[bit][0])
							{
								new_buttons |= (1<<bit);
								break;
							}
						}
						break;
					}
					++frame;
				}
				// before going to next frame, flush buttons to movie data
				if (TASEdit_superimpose_affects_paste && (TASEdit_superimpose == BST_CHECKED || (TASEdit_superimpose == BST_INDETERMINATE && new_buttons == 0)))
				{
					flash_joy[joy] |= (new_buttons & (~currMovieData.records[pos].joysticks[joy]));		// highlight buttons that are new
					currMovieData.records[pos].joysticks[joy] |= new_buttons;
				} else
				{
					flash_joy[joy] |= new_buttons;		// highlight buttons that were added
					currMovieData.records[pos].joysticks[joy] = new_buttons;
				}
				// find CRLF
				pGlobal = strchr(pGlobal, '\n');
			}

			greenzone.InvalidateAndCheck(history.RegisterChanges(MODTYPE_PASTE, *current_selection_begin));
			// flash list header columns that were changed during paste
			for (int joy = 0; joy < NUM_JOYPADS; ++joy)
			{
				for (int btn = 0; btn < NUM_JOYPAD_BUTTONS; ++btn)
				{
					if (flash_joy[joy] & (1 << btn))
						tasedit_list.SetHeaderColumnLight(COLUMN_JOYPAD1_A + joy * NUM_JOYPAD_BUTTONS + btn, HEADER_LIGHT_MAX);
				}
			}
			result = true;
		}
		GlobalUnlock(hGlobal);
	}
	CloseClipboard();
	return result;
}
bool PasteInsert()
{
	SelectionFrames* current_selection = selection.MakeStrobe();
	if (current_selection->size() == 0) return false;

	if (!OpenClipboard(hwndTasEdit)) return false;

	SelectionFrames::iterator current_selection_begin(current_selection->begin());
	bool result = false;
	int pos = *current_selection_begin;
	HANDLE hGlobal = GetClipboardData(CF_TEXT);
	if (hGlobal)
	{
		char *pGlobal = (char*)GlobalLock((HGLOBAL)hGlobal);

		// TAS recording info starts with "TAS "
		if (pGlobal[0]=='T' && pGlobal[1]=='A' && pGlobal[2]=='S')
		{
			// make sure markers have the same size as movie
			current_markers.update();
			// init inserted_set (for input history hot changes)
			selection.GetInsertedSet().clear();

			// Extract number of frames
			int range;
			sscanf (pGlobal+3, "%d", &range);


			pGlobal = strchr(pGlobal, '\n');
			char* frame;
			int joy=0;
			std::vector<uint8> flash_joy(NUM_JOYPADS);
			--pos;
			while (pGlobal++ && *pGlobal!='\0')
			{
				// Detect skipped frames in paste
				frame = pGlobal;
				if (frame[0]=='+')
				{
					pos += atoi(frame+1);
					if (currMovieData.getNumRecords() < pos)
					{
						currMovieData.insertEmpty(currMovieData.getNumRecords(), pos - currMovieData.getNumRecords());
						current_markers.update();
					}
					while (*frame && *frame != '\n' && *frame != '|')
						++frame;
					if (*frame=='|') ++frame;
				} else
				{
					++pos;
				}
				
				// insert new frame
				currMovieData.insertEmpty(pos, 1);
				if (TASEdit_bind_markers) current_markers.insertEmpty(pos, 1);
				selection.GetInsertedSet().insert(pos);

				// read this frame input
				int joy = 0;
				while (*frame && *frame != '\n' && *frame !='\r')
				{
					switch (*frame)
					{
					case '|': // Joystick mark
						++joy;
						break;
					default:
						for (int bit = 0; bit < NUM_JOYPAD_BUTTONS; ++bit)
						{
							if (*frame == buttonNames[bit][0])
							{
								currMovieData.records[pos].joysticks[joy] |= (1<<bit);
								flash_joy[joy] |= (1<<bit);		// highlight buttons
								break;
							}
						}
						break;
					}
					++frame;
				}

				pGlobal = strchr(pGlobal, '\n');
			}
			current_markers.update();
			if (TASEdit_bind_markers)
				selection.must_find_current_marker = playback.must_find_current_marker = true;
			greenzone.InvalidateAndCheck(history.RegisterChanges(MODTYPE_PASTEINSERT, *current_selection_begin));
			// flash list header columns that were changed during paste
			for (int joy = 0; joy < NUM_JOYPADS; ++joy)
			{
				for (int btn = 0; btn < NUM_JOYPAD_BUTTONS; ++btn)
				{
					if (flash_joy[joy] & (1 << btn))
						tasedit_list.SetHeaderColumnLight(COLUMN_JOYPAD1_A + joy * NUM_JOYPAD_BUTTONS + btn, HEADER_LIGHT_MAX);
				}
			}
			result = true;
		}
		GlobalUnlock(hGlobal);
	}
	CloseClipboard();
	return result;
}

void OpenProject()
{
	if (!AskSaveProject()) return;

	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwndTasEdit;
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
	SetFocus(tasedit_list.hwndList);
	// remember to update fourscore status
	bool last_fourscore = currMovieData.fourscore;
	// try to load project
	if (project.load(fullname))
	{
		// update fourscore status
		if (last_fourscore && !currMovieData.fourscore)
		{
			tasedit_list.RemoveFourscore();
			FCEUD_SetInput(currMovieData.fourscore, currMovieData.microphone, (ESI)currMovieData.ports[0], (ESI)currMovieData.ports[1], (ESIFC)currMovieData.ports[2]);
		} else if (!last_fourscore && currMovieData.fourscore)
		{
			tasedit_list.AddFourscore();
			FCEUD_SetInput(currMovieData.fourscore, currMovieData.microphone, (ESI)currMovieData.ports[0], (ESI)currMovieData.ports[1], (ESIFC)currMovieData.ports[2]);
		}
		UpdateRecentProjectsArray(fullname);
		RedrawTasedit();
		RedrawWindowCaption();
		return true;
	} else
	{
		// failed to load
		RedrawTasedit();
		RedrawWindowCaption();
		return false;
	}
}
void LoadRecentProject(int slot)
{
	char*& fname = recent_projects[slot];
	if(fname && AskSaveProject())
	{
		if (!LoadProject(fname))
		{
			int result = MessageBox(hwndTasEdit, "Remove from list?", "Could Not Open Recent Project", MB_YESNO);
			if (result == IDYES)
				RemoveRecentProject(slot);
		}
	}
}

// Saves current project
bool SaveProjectAs()
{
	const char filter[] = "TAS Editor Projects (*.fm3)\0*.fm3\0All Files (*.*)\0*.*\0\0";
	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwndTasEdit;
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
		UpdateRecentProjectsArray(nameo);
	} else return false;
	// saved successfully - remove * mark from caption
	RedrawWindowCaption();
	return true;
}
bool SaveProject()
{
	if (!project.save())
		return SaveProjectAs();
	else
		RedrawWindowCaption();
	return true;
}

void SaveCompact_GetCheckboxes(HWND hwndDlg)
{
	TASEdit_savecompact_binary = (SendDlgItemMessage(hwndDlg, IDC_CHECK_BINARY, BM_GETCHECK, 0, 0) == BST_CHECKED);
	TASEdit_savecompact_markers = (SendDlgItemMessage(hwndDlg, IDC_CHECK_MARKERS, BM_GETCHECK, 0, 0) == BST_CHECKED);
	TASEdit_savecompact_bookmarks = (SendDlgItemMessage(hwndDlg, IDC_CHECK_BOOKMARKS, BM_GETCHECK, 0, 0) == BST_CHECKED);
	TASEdit_savecompact_greenzone = (SendDlgItemMessage(hwndDlg, IDC_CHECK_GREENZONE, BM_GETCHECK, 0, 0) == BST_CHECKED);
	TASEdit_savecompact_history = (SendDlgItemMessage(hwndDlg, IDC_CHECK_HISTORY, BM_GETCHECK, 0, 0) == BST_CHECKED);
	TASEdit_savecompact_list = (SendDlgItemMessage(hwndDlg, IDC_CHECK_LIST, BM_GETCHECK, 0, 0) == BST_CHECKED);
	TASEdit_savecompact_selection = (SendDlgItemMessage(hwndDlg, IDC_CHECK_SELECTION, BM_GETCHECK, 0, 0) == BST_CHECKED);
}
BOOL CALLBACK SaveCompactProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			SetWindowPos(hwndDlg, 0, TasEdit_wndx + 100, TasEdit_wndy + 200, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
			CheckDlgButton(hwndDlg, IDC_CHECK_BINARY, TASEdit_savecompact_binary?MF_CHECKED : MF_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_CHECK_MARKERS, TASEdit_savecompact_markers?MF_CHECKED : MF_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_CHECK_BOOKMARKS, TASEdit_savecompact_bookmarks?MF_CHECKED : MF_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_CHECK_GREENZONE, TASEdit_savecompact_greenzone?MF_CHECKED : MF_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_CHECK_HISTORY, TASEdit_savecompact_history?MF_CHECKED : MF_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_CHECK_LIST, TASEdit_savecompact_list?MF_CHECKED : MF_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_CHECK_SELECTION, TASEdit_savecompact_selection?MF_CHECKED : MF_UNCHECKED);
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
	if (DialogBox(fceu_hInstance, MAKEINTRESOURCE(IDD_TASEDIT_SAVECOMPACT), hwndTasEdit, SaveCompactProc) > 0)
	{
		const char filter[] = "TAS Editor Projects (*.fm3)\0*.fm3\0All Files (*.*)\0*.*\0\0";
		OPENFILENAME ofn;
		memset(&ofn, 0, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = hwndTasEdit;
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
			project.save_compact(nameo, TASEdit_savecompact_binary, TASEdit_savecompact_markers, TASEdit_savecompact_bookmarks, TASEdit_savecompact_greenzone, TASEdit_savecompact_history, TASEdit_savecompact_list, TASEdit_savecompact_selection);
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
		int answer = MessageBox(hwndTasEdit, "Save Project changes?", "TAS Editor", MB_YESNOCANCEL);
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
	ofn.hwndOwner = hwndTasEdit;
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
			SetWindowPos(hwndDlg, 0, TasEdit_wndx + 100, TasEdit_wndy + 200, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
			switch (TASEdit_last_export_type)
			{
			case EXPORT_TYPE_1P:
				{
					Button_SetCheck(GetDlgItem(hwndDlg, IDC_RADIO_1PLAYER), BST_CHECKED);
					break;
				}
			case EXPORT_TYPE_2P:
				{
					Button_SetCheck(GetDlgItem(hwndDlg, IDC_RADIO_2PLAYERS), BST_CHECKED);
					break;
				}
			case EXPORT_TYPE_FOURSCORE:
				{
					Button_SetCheck(GetDlgItem(hwndDlg, IDC_RADIO_FOURSCORE), BST_CHECKED);
					break;
				}
			}
			CheckDlgButton(hwndDlg, IDC_NOTES_TO_SUBTITLES, TASEdit_last_export_subtitles?MF_CHECKED : MF_UNCHECKED);
			return TRUE;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_RADIO_1PLAYER:
					TASEdit_last_export_type = EXPORT_TYPE_1P;
					break;
				case IDC_RADIO_2PLAYERS:
					TASEdit_last_export_type = EXPORT_TYPE_2P;
					break;
				case IDC_RADIO_FOURSCORE:
					TASEdit_last_export_type = EXPORT_TYPE_FOURSCORE;
					break;
				case IDC_NOTES_TO_SUBTITLES:
					TASEdit_last_export_subtitles ^= 1;
					CheckDlgButton(hwndDlg, IDC_NOTES_TO_SUBTITLES, TASEdit_last_export_subtitles?MF_CHECKED : MF_UNCHECKED);
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
	if (DialogBox(fceu_hInstance, MAKEINTRESOURCE(IDD_TASEDIT_EXPORT), hwndTasEdit, ExportProc) > 0)
	{
		const char filter[] = "FCEUX Movie File (*.fm2)\0*.fm2\0All Files (*.*)\0*.*\0\0";
		char fname[2048];
		strcpy(fname, project.GetFM2Name().c_str());
		OPENFILENAME ofn;
		memset(&ofn, 0, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = hwndTasEdit;
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
			switch (TASEdit_last_export_type)
			{
			case EXPORT_TYPE_1P:
				{
					temp_md.fourscore = false;
					temp_md.ports[0] = SI_GAMEPAD;
					temp_md.ports[1] = SI_NONE;
					break;
				}
			case EXPORT_TYPE_2P:
				{
					temp_md.fourscore = false;
					temp_md.ports[0] = SI_GAMEPAD;
					temp_md.ports[1] = SI_GAMEPAD;
					break;
				}
			case EXPORT_TYPE_FOURSCORE:
				{
					temp_md.fourscore = true;
					break;
				}
			}
			temp_md.loadFrameCount = -1;
			if (TASEdit_last_export_subtitles)
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

BOOL CALLBACK WndprocTasEdit(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_PAINT:
			break;
		case WM_INITDIALOG:
		{
			if (TasEdit_wndx==-32000) TasEdit_wndx=0; //Just in case
			if (TasEdit_wndy==-32000) TasEdit_wndy=0;
			SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)hTaseditorIcon);
			SetWindowPos(hwndDlg, 0, TasEdit_wndx, TasEdit_wndy, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
			break;
		}
		case WM_MOVE:
			{
				if (!IsIconic(hwndDlg))
				{
					RECT wrect;
					GetWindowRect(hwndDlg, &wrect);
					TasEdit_wndx = wrect.left;
					TasEdit_wndy = wrect.top;
					WindowBoundsCheckNoResize(TasEdit_wndx, TasEdit_wndy, wrect.right);
					// also move screenshot display if it's open
					screenshot_display.ParentWindowMoved();
				}
				break;
			}
		case WM_NOTIFY:
			switch(wParam)
			{
			case IDC_LIST1:
				switch(((LPNMHDR)lParam)->code)
				{
				case NM_CUSTOMDRAW:
					SetWindowLong(hwndDlg, DWL_MSGRESULT, tasedit_list.CustomDraw((NMLVCUSTOMDRAW*)lParam));
					return TRUE;
				case LVN_GETDISPINFO:
					tasedit_list.GetDispInfo((NMLVDISPINFO*)lParam);
					break;
				case NM_CLICK:
					SingleClick((LPNMITEMACTIVATE)lParam);
					break;
				case NM_DBLCLK:
					DoubleClick((LPNMITEMACTIVATE)lParam);
					break;
				case NM_RCLICK:
					RightClick((LPNMITEMACTIVATE)lParam);
					break;
				case LVN_ITEMCHANGED:
					selection.ItemChanged((LPNMLISTVIEW) lParam);
					break;
				case LVN_ODSTATECHANGED:
					selection.ItemRangeChanged((LPNMLVODSTATECHANGE) lParam);
					break;
					/*
				case LVN_ENDSCROLL:
					// redraw upper and lower list rows (fix for known WinXP bug)
					int start = ListView_GetTopIndex(hwndList);
					ListView_RedrawItems(hwndList,start,start);
					int end = start + listItems - 1;
					ListView_RedrawItems(hwndList,end,end);
					break;
					*/
				}
				break;
			case IDC_BOOKMARKSLIST:
				switch(((LPNMHDR)lParam)->code)
				{
				case NM_CUSTOMDRAW:
					SetWindowLong(hwndDlg, DWL_MSGRESULT, bookmarks.CustomDraw((NMLVCUSTOMDRAW*)lParam));
					return TRUE;
				case LVN_GETDISPINFO:
					bookmarks.GetDispInfo((NMLVDISPINFO*)lParam);
					break;
				case NM_CLICK:
				case NM_DBLCLK:
					bookmarks.LeftClick((LPNMITEMACTIVATE)lParam);
					break;
				case NM_RCLICK:
					bookmarks.RightClick((LPNMITEMACTIVATE)lParam);
					break;
				}
				break;
			case IDC_HISTORYLIST:
				switch(((LPNMHDR)lParam)->code)
				{
				case NM_CUSTOMDRAW:
					SetWindowLong(hwndDlg, DWL_MSGRESULT, history.CustomDraw((NMLVCUSTOMDRAW*)lParam));
					return TRUE;
				case LVN_GETDISPINFO:
					history.GetDispInfo((NMLVDISPINFO*)lParam);
					break;
				case NM_CLICK:
				case NM_DBLCLK:
				case NM_RCLICK:
					history.Click((LPNMITEMACTIVATE)lParam);
					break;
				}
				break;
			case TASEDIT_PLAYSTOP:
				switch(((LPNMHDR)lParam)->code)
				{
				case NM_CLICK:
				case NM_DBLCLK:
					playback.ToggleEmulationPause();
					break;
				}
				break;
			}
			break;
		case WM_CLOSE:
		case WM_QUIT:
			ExitTasEdit();
			break;
		case WM_ACTIVATE:
			if(LOWORD(wParam))
			{
				TASEdit_focus = true;
				SetTaseditInput();
			} else
			{
				TASEdit_focus = false;
				ClearTaseditInput();
			}
			break;
		case WM_COMMAND:
			{
				unsigned int loword_wparam = LOWORD(wParam);
				// first check clicking Recent submenu item
				if (loword_wparam >= MENU_FIRST_RECENT_PROJECT && loword_wparam < MENU_FIRST_RECENT_PROJECT + MAX_NUMBER_OF_RECENT_PROJECTS)
				{
					LoadRecentProject(loword_wparam - MENU_FIRST_RECENT_PROJECT);
					break;
				}
				// finally check all other commands
				switch(loword_wparam)
				{
				case IDC_PLAYBACK_MARKER_EDIT:
					{
						switch (HIWORD(wParam))
						{
						case EN_SETFOCUS:
							{
								marker_note_edit = MARKER_NOTE_EDIT_UPPER;
								// enable editing
								SendMessage(playback.hwndPlaybackMarkerEdit, EM_SETREADONLY, false, 0);
								// disable FCEUX keyboard
								ClearTaseditInput();
								if (TASEdit_follow_note_context)
									tasedit_list.FollowPlayback();
								break;
							}
						case EN_KILLFOCUS:
							{
								if (marker_note_edit == MARKER_NOTE_EDIT_UPPER)
								{
									UpdateMarkerNote();
									marker_note_edit = MARKER_NOTE_EDIT_NONE;
								}
								// disable editing (make it grayed)
								SendMessage(playback.hwndPlaybackMarkerEdit, EM_SETREADONLY, true, 0);
								// enable FCEUX keyboard
								if (TASEdit_focus)
									SetTaseditInput();
								break;
							}
						}
						break;
					}
				case IDC_SELECTION_MARKER_EDIT:
					{
						switch (HIWORD(wParam))
						{
						case EN_SETFOCUS:
							{
								marker_note_edit = MARKER_NOTE_EDIT_LOWER;
								// enable editing
								SendMessage(selection.hwndSelectionMarkerEdit, EM_SETREADONLY, false, 0); 
								// disable FCEUX keyboard
								ClearTaseditInput();
								if (TASEdit_follow_note_context)
									tasedit_list.FollowSelection();
								break;
							}
						case EN_KILLFOCUS:
							{
								if (marker_note_edit == MARKER_NOTE_EDIT_LOWER)
								{
									UpdateMarkerNote();
									marker_note_edit = MARKER_NOTE_EDIT_NONE;
								}
								// disable editing (make it grayed)
								SendMessage(selection.hwndSelectionMarkerEdit, EM_SETREADONLY, true, 0); 
								// enable FCEUX keyboard
								if (TASEdit_focus)
									SetTaseditInput();
								break;
							}
						}
						break;
					}
				case ID_FILE_OPENPROJECT:
					OpenProject();
					break;
				case ACCEL_CTRL_S:
				case ID_FILE_SAVEPROJECT:
					SaveProject();
					break;
				case ID_FILE_SAVEPROJECTAS:
					SaveProjectAs();
					break;
				case ID_FILE_SAVECOMPACT:
					SaveCompact();
					break;
				case ID_FILE_IMPORT:
					Import();
					break;
				case ID_FILE_EXPORTFM2:
						Export();
					break;
				case ID_TASEDIT_FILE_CLOSE:
					ExitTasEdit();
					break;
				case ID_EDIT_SELECTALL:
					if (marker_note_edit == MARKER_NOTE_EDIT_UPPER)
						SendMessage(playback.hwndPlaybackMarkerEdit, EM_SETSEL, 0, -1); 
					else if (marker_note_edit == MARKER_NOTE_EDIT_LOWER)
						SendMessage(selection.hwndSelectionMarkerEdit, EM_SETSEL, 0, -1); 
					else
						selection.SelectAll();
					break;
				case ACCEL_CTRL_X:
				case ID_TASEDIT_CUT:
					if (marker_note_edit == MARKER_NOTE_EDIT_UPPER)
						SendMessage(playback.hwndPlaybackMarkerEdit, WM_CUT, 0, 0); 
					else if (marker_note_edit == MARKER_NOTE_EDIT_LOWER)
						SendMessage(selection.hwndSelectionMarkerEdit, WM_CUT, 0, 0); 
					else
						Cut();
					break;
				case ACCEL_CTRL_C:
				case ID_TASEDIT_COPY:
					if (marker_note_edit == MARKER_NOTE_EDIT_UPPER)
						SendMessage(playback.hwndPlaybackMarkerEdit, WM_COPY, 0, 0); 
					else if (marker_note_edit == MARKER_NOTE_EDIT_LOWER)
						SendMessage(selection.hwndSelectionMarkerEdit, WM_COPY, 0, 0); 
					else
						Copy();
					break;
				case ACCEL_CTRL_V:
				case ID_TASEDIT_PASTE:
					if (marker_note_edit == MARKER_NOTE_EDIT_UPPER)
						SendMessage(playback.hwndPlaybackMarkerEdit, WM_PASTE, 0, 0); 
					else if (marker_note_edit == MARKER_NOTE_EDIT_LOWER)
						SendMessage(selection.hwndSelectionMarkerEdit, WM_PASTE, 0, 0); 
					else
						Paste();
					break;
				case ACCEL_SHIFT_V:
					{
						// hack to allow entering Shift-V into edit control even though accelerator steals the input
						char insert_v[] = "v";
						char insert_V[] = "V";
						if (marker_note_edit == MARKER_NOTE_EDIT_UPPER)
						{
							if (GetKeyState(VK_CAPITAL) & 1)
								SendMessage(playback.hwndPlaybackMarkerEdit, EM_REPLACESEL, true, (LPARAM)insert_v);
							else
								SendMessage(playback.hwndPlaybackMarkerEdit, EM_REPLACESEL, true, (LPARAM)insert_V);
						} else if (marker_note_edit == MARKER_NOTE_EDIT_LOWER)
						{
							if (GetKeyState(VK_CAPITAL) & 1)
								SendMessage(selection.hwndSelectionMarkerEdit, EM_REPLACESEL, true, (LPARAM)insert_v);
							else
								SendMessage(selection.hwndSelectionMarkerEdit, EM_REPLACESEL, true, (LPARAM)insert_V);
						} else
							PasteInsert();
					}
					break;
				case ID_EDIT_PASTEINSERT:
					if (marker_note_edit == MARKER_NOTE_EDIT_UPPER)
						SendMessage(playback.hwndPlaybackMarkerEdit, WM_PASTE, 0, 0); 
					else if (marker_note_edit == MARKER_NOTE_EDIT_LOWER)
						SendMessage(selection.hwndSelectionMarkerEdit, WM_PASTE, 0, 0); 
					else
						PasteInsert();
					break;
				case ACCEL_CTRL_DELETE:
				case ID_TASEDIT_DELETE:
				case ID_CONTEXT_SELECTED_DELETEFRAMES:
					DeleteFrames();
					break;
				case ACCEL_CTRL_T:
				case ID_EDIT_TRUNCATE:
				case ID_CONTEXT_SELECTED_TRUNCATE:
				case ID_CONTEXT_STRAY_TRUNCATE:
					Truncate();
					break;
				case ID_HELP_TASEDITHELP:
					OpenHelpWindow(tasedithelp);
					//link to TAS Editor in help menu
					break;
				case ACCEL_INS:
				case ID_EDIT_INSERT:
				case MENU_CONTEXT_STRAY_INSERTFRAMES:
				case ID_CONTEXT_SELECTED_INSERTFRAMES2:
					InsertNumFrames();
					break;
				case ACCEL_SHIFT_INS:
				case ID_EDIT_INSERTFRAMES:
				case ID_CONTEXT_SELECTED_INSERTFRAMES:
					InsertFrames();
					break;
				case ACCEL_DEL:
				case ID_EDIT_CLEAR:
				case ID_CONTEXT_SELECTED_CLEARFRAMES:
					if (marker_note_edit == MARKER_NOTE_EDIT_UPPER)
					{
						DWORD sel_start, sel_end;
						SendMessage(playback.hwndPlaybackMarkerEdit, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
						if (sel_start == sel_end)
							SendMessage(playback.hwndPlaybackMarkerEdit, EM_SETSEL, sel_start, sel_start + 1);
						SendMessage(playback.hwndPlaybackMarkerEdit, WM_CLEAR, 0, 0); 
					} else if (marker_note_edit == MARKER_NOTE_EDIT_LOWER)
					{
						DWORD sel_start, sel_end;
						SendMessage(selection.hwndSelectionMarkerEdit, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
						if (sel_start == sel_end)
							SendMessage(selection.hwndSelectionMarkerEdit, EM_SETSEL, sel_start, sel_start + 1);
						SendMessage(selection.hwndSelectionMarkerEdit, WM_CLEAR, 0, 0); 
					} else
						ClearFrames();
					break;
				case TASEDIT_PLAYSTOP:
					playback.ToggleEmulationPause();
					break;
				case CHECK_FOLLOW_CURSOR:
					//switch "Follow playback" flag
					TASEdit_follow_playback ^= 1;
					CheckDlgButton(hwndTasEdit, CHECK_FOLLOW_CURSOR, TASEdit_follow_playback?MF_CHECKED : MF_UNCHECKED);
					// if switched off then jump to selection
					if (TASEdit_follow_playback)
						tasedit_list.FollowPlayback();
					else if (selection.GetCurrentSelectionSize())
						tasedit_list.FollowSelection();
					else if (playback.GetPauseFrame())
						tasedit_list.FollowPauseframe();
					break;
				case CHECK_TURBO_SEEK:
					//switch "Turbo seek" flag
					TASEdit_turbo_seek ^= 1;
					CheckDlgButton(hwndTasEdit, CHECK_TURBO_SEEK, TASEdit_turbo_seek?MF_CHECKED : MF_UNCHECKED);
					// if currently seeking, apply this option immediately
					if (playback.pause_frame)
						turbo = TASEdit_turbo_seek;
					break;
				case ID_VIEW_SHOW_LAG_FRAMES:
					TASEdit_show_lag_frames ^= 1;
					CheckMenuItem(hmenu, ID_VIEW_SHOW_LAG_FRAMES, TASEdit_show_lag_frames?MF_CHECKED : MF_UNCHECKED);
					tasedit_list.RedrawList();
					bookmarks.RedrawBookmarksList();
					break;
				case ID_VIEW_SHOW_MARKERS:
					TASEdit_show_markers ^= 1;
					CheckMenuItem(hmenu, ID_VIEW_SHOW_MARKERS, TASEdit_show_markers?MF_CHECKED : MF_UNCHECKED);
					tasedit_list.RedrawList();		// no need to redraw Bookmarks, as Markers are only shown in main list
					break;
				case ID_VIEW_SHOWBRANCHSCREENSHOTS:
					//switch "Show Branch Screenshots" flag
					TASEdit_show_branch_screenshots ^= 1;
					CheckMenuItem(hmenu, ID_VIEW_SHOWBRANCHSCREENSHOTS, TASEdit_show_branch_screenshots?MF_CHECKED : MF_UNCHECKED);
					break;
				case ID_VIEW_SHOWBRANCHTOOLTIPS:
					//switch "Show Branch Screenshots" flag
					TASEdit_show_branch_tooltips ^= 1;
					CheckMenuItem(hmenu, ID_VIEW_SHOWBRANCHTOOLTIPS, TASEdit_show_branch_tooltips?MF_CHECKED : MF_UNCHECKED);
					break;
				case ID_VIEW_ENABLEHOTCHANGES:
					TASEdit_enable_hot_changes ^= 1;
					CheckMenuItem(hmenu, ID_VIEW_ENABLEHOTCHANGES, TASEdit_enable_hot_changes?MF_CHECKED : MF_UNCHECKED);
					tasedit_list.RedrawList();		// redraw buttons text
					break;
				case ID_VIEW_JUMPWHENMAKINGUNDO:
					TASEdit_jump_to_undo ^= 1;
					CheckMenuItem(hmenu, ID_VIEW_JUMPWHENMAKINGUNDO, TASEdit_jump_to_undo?MF_CHECKED : MF_UNCHECKED);
					break;
				case ID_VIEW_FOLLOWMARKERNOTECONTEXT:
					TASEdit_follow_note_context ^= 1;
					CheckMenuItem(hmenu, ID_VIEW_FOLLOWMARKERNOTECONTEXT, TASEdit_follow_note_context?MF_CHECKED : MF_UNCHECKED);
					break;
				case ACCEL_CTRL_P:
				case CHECK_AUTORESTORE_PLAYBACK:
					//switch "Auto-restore last playback position" flag
					TASEdit_restore_position ^= 1;
					CheckDlgButton(hwndTasEdit,CHECK_AUTORESTORE_PLAYBACK,TASEdit_restore_position?BST_CHECKED:BST_UNCHECKED);
					break;
				case ID_CONFIG_SETGREENZONECAPACITY:
					{
						int new_capacity = TASEdit_greenzone_capacity;
						if(CWin32InputBox::GetInteger("Greenzone capacity", "Keep savestates for how many frames?\n(actual limit of savestates can be 5 times more than the number provided)", new_capacity, hwndDlg) == IDOK)
						{
							if (new_capacity < GREENZONE_CAPACITY_MIN)
								new_capacity = GREENZONE_CAPACITY_MIN;
							else if (new_capacity > GREENZONE_CAPACITY_MAX)
								new_capacity = GREENZONE_CAPACITY_MAX;
							if (new_capacity < TASEdit_greenzone_capacity)
							{
								TASEdit_greenzone_capacity = new_capacity;
								greenzone.GreenzoneCleaning();
							} else TASEdit_greenzone_capacity = new_capacity;
						}
						break;
					}
				case ID_CONFIG_SETMAXUNDOLEVELS:
					{
						int new_size = TasEdit_undo_levels;
						if(CWin32InputBox::GetInteger("Max undo levels", "Keep history of how many changes?", new_size, hwndDlg) == IDOK)
						{
							if (new_size < UNDO_LEVELS_MIN)
								new_size = UNDO_LEVELS_MIN;
							else if (new_size > UNDO_LEVELS_MAX)
								new_size = UNDO_LEVELS_MAX;
							if (new_size != TasEdit_undo_levels)
							{
								TasEdit_undo_levels = new_size;
								history.reset();
								selection.reset();
								// hot changes were cleared, so update list
								tasedit_list.RedrawList();
							}
						}
						break;
					}
				case ID_CONFIG_SETAUTOSAVEPERIOD:
					{
						int new_period = TASEdit_autosave_period;
						if(CWin32InputBox::GetInteger("Autosave period", "How many minutes may the project stay not saved after being changed?\n(0 = no autosaves)", new_period, hwndDlg) == IDOK)
						{
							if (new_period < AUTOSAVE_PERIOD_MIN)
								new_period = AUTOSAVE_PERIOD_MIN;
							else if (new_period > AUTOSAVE_PERIOD_MAX)
								new_period = AUTOSAVE_PERIOD_MAX;
							TASEdit_autosave_period = new_period;
							project.SheduleNextAutosave();	
						}
						break;
					}
				case ID_CONFIG_BRANCHESRESTOREFULLMOVIE:
					//switch "Branches restore entire Movie" flag
					TASEdit_branch_full_movie ^= 1;
					CheckMenuItem(hmenu, ID_CONFIG_BRANCHESRESTOREFULLMOVIE, TASEdit_branch_full_movie?MF_CHECKED : MF_UNCHECKED);
					break;
				case ID_CONFIG_BRANCHESWORKONLYWHENRECORDING:
					//switch "Branches work only when Recording" flag
					TASEdit_branch_only_when_rec ^= 1;
					CheckMenuItem(hmenu, ID_CONFIG_BRANCHESWORKONLYWHENRECORDING, TASEdit_branch_only_when_rec?MF_CHECKED : MF_UNCHECKED);
					bookmarks.RedrawBookmarksCaption();
					break;
				case ID_CONFIG_HUDINBRANCHSCREENSHOTS:
					//switch "HUD in Branch screenshots" flag
					TASEdit_branch_scr_hud ^= 1;
					CheckMenuItem(hmenu, ID_CONFIG_HUDINBRANCHSCREENSHOTS, TASEdit_branch_scr_hud?MF_CHECKED : MF_UNCHECKED);
					break;
				case ID_CONFIG_BINDMARKERSTOINPUT:
					TASEdit_bind_markers ^= 1;
					CheckMenuItem(hmenu, ID_CONFIG_BINDMARKERSTOINPUT, TASEdit_bind_markers?MF_CHECKED : MF_UNCHECKED);
					break;
				case ID_CONFIG_EMPTYNEWMARKERNOTES:
					TASEdit_empty_marker_notes ^= 1;
					CheckMenuItem(hmenu, ID_CONFIG_EMPTYNEWMARKERNOTES, TASEdit_empty_marker_notes?MF_CHECKED : MF_UNCHECKED);
					break;
				case ID_CONFIG_REAPPEARINGFINDNOTEDIALOG:
					TASEdit_findnote_reappear ^= 1;
					CheckMenuItem(hmenu, ID_CONFIG_REAPPEARINGFINDNOTEDIALOG, TASEdit_findnote_reappear?MF_CHECKED : MF_UNCHECKED);
					break;
				case ID_CONFIG_COMBINECONSECUTIVERECORDINGS:
					//switch "Combine consecutive Recordings" flag
					TASEdit_combine_consecutive_rec ^= 1;
					CheckMenuItem(hmenu, ID_CONFIG_COMBINECONSECUTIVERECORDINGS, TASEdit_combine_consecutive_rec?MF_CHECKED : MF_UNCHECKED);
					break;
				case ID_CONFIG_USE1PFORRECORDING:
					//switch "Use 1P keys for single Recordings" flag
					TASEdit_use_1p_rec ^= 1;
					CheckMenuItem(hmenu, ID_CONFIG_USE1PFORRECORDING, TASEdit_use_1p_rec?MF_CHECKED : MF_UNCHECKED);
					break;
				case ID_CONFIG_USEINPUTKEYSFORCOLUMNSET:
					TASEdit_columnset_by_keys ^= 1;
					CheckMenuItem(hmenu, ID_CONFIG_USEINPUTKEYSFORCOLUMNSET, TASEdit_columnset_by_keys?MF_CHECKED : MF_UNCHECKED);
					break;
				case ID_CONFIG_KEYBOARDCONTROLSINLISTVIEW:
					TASEdit_keyboard_for_listview ^= 1;
					CheckMenuItem(hmenu, ID_CONFIG_KEYBOARDCONTROLSINLISTVIEW, TASEdit_keyboard_for_listview?MF_CHECKED : MF_UNCHECKED);
					break;
				case ID_CONFIG_SUPERIMPOSE_AFFECTS_PASTE:
					TASEdit_superimpose_affects_paste ^= 1;
					CheckMenuItem(hmenu, ID_CONFIG_SUPERIMPOSE_AFFECTS_PASTE, TASEdit_superimpose_affects_paste?MF_CHECKED : MF_UNCHECKED);
					break;
				case ID_CONFIG_MUTETURBO:
					muteTurbo ^= 1;
					CheckMenuItem(hmenu, ID_CONFIG_MUTETURBO, muteTurbo?MF_CHECKED : MF_UNCHECKED);
					break;
				case IDC_PROGRESS_BUTTON:
					// click on progressbar - stop seeking
					if (playback.GetPauseFrame()) playback.SeekingStop();
					break;
				case IDC_BRANCHES_BUTTON:
					// click on "Bookmarks/Branches" - switch "View Tree of branches"
					TASEdit_view_branches_tree ^= 1;
					bookmarks.RedrawBookmarksCaption();
					break;
				case IDC_RECORDING:
					// toggle readonly, no need to recheck radiobuttons
					FCEUI_MovieToggleReadOnly();
					CheckDlgButton(hwndTasEdit, IDC_RECORDING, movie_readonly?BST_UNCHECKED : BST_CHECKED);
					break;
				case IDC_RADIO2:
					recorder.multitrack_recording_joypad = MULTITRACK_RECORDING_ALL;
					break;
				case IDC_RADIO3:
					recorder.multitrack_recording_joypad = MULTITRACK_RECORDING_1P;
					break;
				case IDC_RADIO4:
					recorder.multitrack_recording_joypad = MULTITRACK_RECORDING_2P;
					break;
				case IDC_RADIO5:
					recorder.multitrack_recording_joypad = MULTITRACK_RECORDING_3P;
					break;
				case IDC_RADIO6:
					recorder.multitrack_recording_joypad = MULTITRACK_RECORDING_4P;
					break;
				case IDC_SUPERIMPOSE:
					// 3 states of "Superimpose" checkbox
					if (TASEdit_superimpose == BST_UNCHECKED)
						TASEdit_superimpose = BST_CHECKED;
					else if (TASEdit_superimpose == BST_CHECKED)
						TASEdit_superimpose = BST_INDETERMINATE;
					else TASEdit_superimpose = BST_UNCHECKED;
					CheckDlgButton(hwndTasEdit, IDC_SUPERIMPOSE, TASEdit_superimpose);
					break;
				case ACCEL_CTRL_A:
					if (marker_note_edit == MARKER_NOTE_EDIT_UPPER)
						SendMessage(playback.hwndPlaybackMarkerEdit, EM_SETSEL, 0, -1); 
					else if (marker_note_edit == MARKER_NOTE_EDIT_LOWER)
						SendMessage(selection.hwndSelectionMarkerEdit, EM_SETSEL, 0, -1); 
					else
						selection.SelectMidMarkers();
					break;
				case ID_EDIT_SELECTMIDMARKERS:
				case ID_SELECTED_SELECTMIDMARKERS:
					selection.SelectMidMarkers();
					break;
				case ACCEL_CTRL_INSERT:
				case ID_EDIT_CLONEFRAMES:
				case ID_SELECTED_CLONE:
					CloneFrames();
					break;
				case ACCEL_CTRL_Z:
				case ID_EDIT_UNDO:
					{
						if (marker_note_edit == MARKER_NOTE_EDIT_UPPER)
						{
							SendMessage(playback.hwndPlaybackMarkerEdit, WM_UNDO, 0, 0); 
						} else if (marker_note_edit == MARKER_NOTE_EDIT_LOWER)
						{
							SendMessage(selection.hwndSelectionMarkerEdit, WM_UNDO, 0, 0); 
						} else
						{
							int result = history.undo();
							if (result >= 0)
							{
								tasedit_list.update();
								tasedit_list.FollowUndo();
								greenzone.InvalidateAndCheck(result);
							}
						}
						break;
					}
				case ACCEL_CTRL_Y:
				case ID_EDIT_REDO:
					{
						int result = history.redo();
						if (result >= 0)
						{
							tasedit_list.update();
							tasedit_list.FollowUndo();
							greenzone.InvalidateAndCheck(result);
						}
						break;
					}
				case ID_EDIT_SELECTIONUNDO:
				case ACCEL_CTRL_Q:
					{
						selection.undo();
						tasedit_list.FollowSelection();
						break;
					}
				case ID_EDIT_SELECTIONREDO:
				case ACCEL_CTRL_W:
					{
						selection.redo();
						tasedit_list.FollowSelection();
						break;
					}
				case ID_EDIT_RESELECTCLIPBOARD:
				case ACCEL_CTRL_B:
					{
						selection.ReselectClipboard();
						tasedit_list.FollowSelection();
						break;
					}
				case IDC_TEXT_SELECTION_BUTTON:
					{
						tasedit_list.FollowSelection();
						break;
					}
				case ID_SELECTED_SETMARKER:
					{
						SelectionFrames* current_selection = selection.MakeStrobe();
						if (current_selection->size())
						{
							SelectionFrames::iterator current_selection_begin(current_selection->begin());
							SelectionFrames::iterator current_selection_end(current_selection->end());
							bool changes_made = false;
							for(SelectionFrames::iterator it(current_selection_begin); it != current_selection_end; it++)
							{
								if(!current_markers.GetMarker(*it))
								{
									if (current_markers.SetMarker(*it))
									{
										changes_made = true;
										tasedit_list.RedrawRow(*it);
									}
								}
							}
							if (changes_made)
							{
								selection.must_find_current_marker = playback.must_find_current_marker = true;
								history.RegisterMarkersChange(MODTYPE_MARKER_SET, *current_selection_begin, *current_selection->rbegin());
							}
						}
						break;
					}
				case ID_SELECTED_REMOVEMARKER:
					{
						SelectionFrames* current_selection = selection.MakeStrobe();
						if (current_selection->size())
						{
							SelectionFrames::iterator current_selection_begin(current_selection->begin());
							SelectionFrames::iterator current_selection_end(current_selection->end());
							bool changes_made = false;
							for(SelectionFrames::iterator it(current_selection_begin); it != current_selection_end; it++)
							{
								if(current_markers.GetMarker(*it))
								{
									current_markers.ClearMarker(*it);
									changes_made = true;
									tasedit_list.RedrawRow(*it);
								}
							}
							if (changes_made)
							{
								selection.must_find_current_marker = playback.must_find_current_marker = true;
								history.RegisterMarkersChange(MODTYPE_MARKER_UNSET, *current_selection_begin, *current_selection->rbegin());
							}
						}
						break;
					}
				case ACCEL_SHIFT_PGUP:
					if (!playback.jump_was_used_this_frame)
						playback.RewindFull();
					break;
				case ACCEL_SHIFT_PGDN:
					if (!playback.jump_was_used_this_frame)
						playback.ForwardFull();
					break;
				case ACCEL_CTRL_PGUP:
					selection.JumpPrevMarker();
					break;
				case ACCEL_CTRL_PGDN:
					selection.JumpNextMarker();
					break;
				case ACCEL_CTRL_F:
				case ID_EDIT_FINDNOTE:
					FindNote();
					break;
				case TASEDIT_FIND_BEST_SIMILAR_MARKER:
					FindSimilarMarker();
					break;
				case TASEDIT_FIND_NEXT_SIMILAR_MARKER:
					// reset search_offset to 0
					FindSimilarMarker();
					break;


				}
				break;
			}
		case WM_SYSKEYDOWN:
		{
			if (wParam == VK_F10)
				return 0;
			break;
		}
		default:
			break;
	}
	return FALSE;
}

bool EnterTasEdit()
{
	if(!FCEU_IsValidUI(FCEUI_TASEDIT)) return false;
	if(!hwndTasEdit)
	{
		hTaseditorIcon = (HICON)LoadImage(fceu_hInstance, MAKEINTRESOURCE(IDI_ICON3), IMAGE_ICON, 16, 16, LR_DEFAULTSIZE);
		hwndTasEdit = CreateDialog(fceu_hInstance,"TASEDIT", hAppWnd, WndprocTasEdit);
		if(hwndTasEdit)
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

			hmenu = GetMenu(hwndTasEdit);
			hrmenu = LoadMenu(fceu_hInstance,"TASEDITCONTEXTMENUS");
			recent_projects_menu = CreateMenu();
			UpdateRecentProjectsMenu();
			// check option ticks
			CheckDlgButton(hwndTasEdit, CHECK_FOLLOW_CURSOR, TASEdit_follow_playback?MF_CHECKED : MF_UNCHECKED);
			CheckDlgButton(hwndTasEdit, CHECK_TURBO_SEEK, TASEdit_turbo_seek?MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hmenu, ID_VIEW_SHOW_LAG_FRAMES, TASEdit_show_lag_frames?MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hmenu, ID_VIEW_SHOW_MARKERS, TASEdit_show_markers?MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hmenu, ID_VIEW_SHOWBRANCHSCREENSHOTS, TASEdit_show_branch_screenshots?MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hmenu, ID_VIEW_SHOWBRANCHTOOLTIPS, TASEdit_show_branch_tooltips?MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hmenu, ID_VIEW_JUMPWHENMAKINGUNDO, TASEdit_jump_to_undo?MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hmenu, ID_VIEW_FOLLOWMARKERNOTECONTEXT, TASEdit_follow_note_context?MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hmenu, ID_VIEW_ENABLEHOTCHANGES, TASEdit_enable_hot_changes?MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hmenu, ID_CONFIG_BRANCHESRESTOREFULLMOVIE, TASEdit_branch_full_movie?MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hmenu, ID_CONFIG_BRANCHESWORKONLYWHENRECORDING, TASEdit_branch_only_when_rec?MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hmenu, ID_CONFIG_HUDINBRANCHSCREENSHOTS, TASEdit_branch_scr_hud?MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hmenu, ID_CONFIG_BINDMARKERSTOINPUT, TASEdit_bind_markers?MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hmenu, ID_CONFIG_EMPTYNEWMARKERNOTES, TASEdit_empty_marker_notes?MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hmenu, ID_CONFIG_REAPPEARINGFINDNOTEDIALOG, TASEdit_findnote_reappear?MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hmenu, ID_CONFIG_COMBINECONSECUTIVERECORDINGS, TASEdit_combine_consecutive_rec?MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hmenu, ID_CONFIG_USE1PFORRECORDING, TASEdit_use_1p_rec?MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hmenu, ID_CONFIG_USEINPUTKEYSFORCOLUMNSET, TASEdit_columnset_by_keys?MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hmenu, ID_CONFIG_KEYBOARDCONTROLSINLISTVIEW, TASEdit_keyboard_for_listview?MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hmenu, ID_CONFIG_SUPERIMPOSE_AFFECTS_PASTE, TASEdit_superimpose_affects_paste?MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hmenu, ID_CONFIG_MUTETURBO, muteTurbo?MF_CHECKED : MF_UNCHECKED);
			CheckDlgButton(hwndTasEdit,CHECK_AUTORESTORE_PLAYBACK,TASEdit_restore_position?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndTasEdit, IDC_SUPERIMPOSE, TASEdit_superimpose);

			SetWindowPos(hwndTasEdit, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE|SWP_NOOWNERZORDER);
			// init modules
			greenzone.init();
			playback.init();
			// either start new movie or use current movie
			if (FCEUMOV_Mode(MOVIEMODE_INACTIVE))
			{
				FCEUI_StopMovie();
				CreateCleanMovie();
				playback.StartFromZero();
			} else
			{
				// use current movie to create a new project
				if (currMovieData.savestate.size() != 0)
				{
					FCEUD_PrintError("This version of TAS Editor doesn't work with movies starting from savestate.");
					// delete savestate, but preserve input
					currMovieData.savestate.clear();
				}
				FCEUI_StopMovie();
				currMovieData.emuVersion = FCEU_VERSION_NUMERIC;
				greenzone.TryDumpIncremental(lagFlag != 0);
			}
			// switch to taseditor mode
			movieMode = MOVIEMODE_TASEDIT;
			currMovieData.ports[0] = SI_GAMEPAD;
			currMovieData.ports[1] = SI_GAMEPAD;
			//force the input configuration stored in the movie to apply
			FCEUD_SetInput(currMovieData.fourscore, currMovieData.microphone, (ESI)currMovieData.ports[0], (ESI)currMovieData.ports[1], (ESIFC)currMovieData.ports[2]);
			// init variables
			recorder.init();
			tasedit_list.init();
			current_markers.init();
			project.init();
			bookmarks.init();
			screenshot_display.init();
			history.init();
			selection.init();

			marker_note_edit = MARKER_NOTE_EDIT_NONE;
			SetFocus(history.hwndHistoryList);		// set focus only once, to show selection cursor
			SetFocus(tasedit_list.hwndList);
			FCEU_DispMessage("TAS Editor engaged", 0);
			return true;
		} else return false;
	} else return true;
}

bool ExitTasEdit()
{
	if (!AskSaveProject()) return false;

	DestroyWindow(hwndTasEdit);
	hwndTasEdit = 0;
	TASEdit_focus = false;
	DestroyIcon(hTaseditorIcon);
	hTaseditorIcon = 0;
	ClearTaseditInput();
	// restore "eoptions"
	eoptions = saved_eoptions;
	// restore autosaves
	EnableAutosave = saved_EnableAutosave;
	DoPriority();
	UpdateCheckedMenuItems();
	// release memory
	tasedit_list.free();
	current_markers.free();
	greenzone.free();
	bookmarks.free();
	screenshot_display.free();
	history.free();
	playback.SeekingStop();
	selection.free();
	// switch off taseditor mode
	movieMode = MOVIEMODE_INACTIVE;
	FCEU_DispMessage("TAS Editor disengaged", 0);
	CreateCleanMovie();
	return true;
}

void SetTaseditInput()
{
	// set "Background TAS Editor input"
	KeyboardSetBackgroundAccessBit(KEYBACKACCESS_TASEDIT);
	JoystickSetBackgroundAccessBit(JOYBACKACCESS_TASEDIT);
}
void ClearTaseditInput()
{
	// clear "Background TAS Editor input"
	KeyboardClearBackgroundAccessBit(KEYBACKACCESS_TASEDIT);
	JoystickClearBackgroundAccessBit(JOYBACKACCESS_TASEDIT);
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
			SetWindowPos(hwndDlg, 0, TasEdit_wndx + 70, TasEdit_wndy + 160, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
			CheckDlgButton(hwndDlg, IDC_MATCH_CASE, TASEdit_findnote_matchcase?MF_CHECKED : MF_UNCHECKED);
			if (TASEdit_findnote_search_up)
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
		case WM_COMMAND:
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
					TASEdit_findnote_search_up = true;
					break;
				case IDC_RADIO_DOWN:
					TASEdit_findnote_search_up = false;
					break;
				case IDC_MATCH_CASE:
					TASEdit_findnote_matchcase ^= 1;
					CheckDlgButton(hwndDlg, IDC_MATCH_CASE, TASEdit_findnote_matchcase?MF_CHECKED : MF_UNCHECKED);
					break;
				case IDOK:
				{
					int len = SendMessage(GetDlgItem(hwndDlg, IDC_NOTE_TO_FIND), WM_GETTEXT, MAX_NOTE_LEN, (LPARAM)findnote_string);
					findnote_string[len] = 0;
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

void FindNote()
{
	selection.update();
	int movie_size = currMovieData.getNumRecords();
	int entries_found = 0;
	int current_frame;
	int cur_marker = 0;
	bool result;

	do
	{
		if (DialogBox(fceu_hInstance, MAKEINTRESOURCE(IDD_TASEDIT_FINDNOTE), hwndTasEdit, FindNoteProc) > 0 && strlen(findnote_string))
		{
			current_frame = selection.GetCurrentSelectionBeginning();
			if (TASEdit_findnote_search_up)
				if (current_frame < 0)
					current_frame = movie_size;
			while (true)
			{
				// move forward
				if (TASEdit_findnote_search_up)
				{
					current_frame--;
					if (current_frame < 0)
					{
						if (entries_found)
							MessageBox(hwndTasEdit, "No more entries found.", "Find Note", MB_OK);
						else
							MessageBox(hwndTasEdit, "Nothing was found!", "Find Note", MB_OK);
						break;
					}
				} else
				{
					current_frame++;
					if (current_frame >= movie_size)
					{
						if (entries_found)
							MessageBox(hwndTasEdit, "No more entries found.", "Find Note", MB_OK);
						else
							MessageBox(hwndTasEdit, "Nothing was found!", "Find Note", MB_OK);
						break;
					}
				}
				// scan marked frames
				cur_marker = current_markers.GetMarker(current_frame);
				if (cur_marker)
				{
					if (TASEdit_findnote_matchcase)
						result = (strstr(current_markers.GetNote(cur_marker).c_str(), findnote_string) != 0);
					else
						result = (StrStrI(current_markers.GetNote(cur_marker).c_str(), findnote_string) != 0);
					if (result)
					{
						// found note containing searched string - jump there
						entries_found++;
						selection.JumpToFrame(current_frame);
						selection.update();
						break;
					}
				}
			}
		} else break;
	} while (TASEdit_findnote_reappear);
}

void FindSimilarMarker()
{
	char playback_marker_text[MAX_NOTE_LEN];
	strcpy(playback_marker_text, current_markers.GetNote(playback.shown_marker).c_str());

	// check if playback_marker_text is empty
	if (!playback_marker_text[0])
	{
		MessageBox(hwndTasEdit, "Marker Note under Playback cursor is empty!", "Find Similar Note", MB_OK);
		return;
	}






}
// --------------------------------------------------------------------------------------------
void UpdateRecentProjectsMenu()
{
	MENUITEMINFO moo;
	int x;
	moo.cbSize = sizeof(moo);
	moo.fMask = MIIM_SUBMENU | MIIM_STATE;
	GetMenuItemInfo(GetSubMenu(hmenu, 0), ID_TASEDIT_FILE_RECENT, FALSE, &moo);
	moo.hSubMenu = recent_projects_menu;
	moo.fState = recent_projects[0] ? MFS_ENABLED : MFS_GRAYED;
	SetMenuItemInfo(GetSubMenu(hmenu, 0), ID_TASEDIT_FILE_RECENT, FALSE, &moo);

	// Remove all recent files submenus
	for(x = 0; x < MAX_NUMBER_OF_RECENT_PROJECTS; x++)
	{
		RemoveMenu(recent_projects_menu, MENU_FIRST_RECENT_PROJECT + x, MF_BYCOMMAND);
	}
	// Recreate the menus
	for(x = MAX_NUMBER_OF_RECENT_PROJECTS - 1; x >= 0; x--)
	{  
		// Skip empty strings
		if(!recent_projects[x]) continue;

		string tmp = recent_projects[x];
		// clamp this string to 128 chars
		if(tmp.size() > 128)
			tmp = tmp.substr(0, 128);

		moo.cbSize = sizeof(moo);
		moo.fMask = MIIM_DATA | MIIM_ID | MIIM_TYPE;
		// Insert the menu item
		moo.cch = tmp.size();
		moo.fType = 0;
		moo.wID = MENU_FIRST_RECENT_PROJECT + x;
		moo.dwTypeData = (LPSTR)tmp.c_str();
		InsertMenuItem(recent_projects_menu, 0, 1, &moo);
	}

	// if recent_projects is empty, "Recent" manu should be grayed
	int i;
	for (i = 0; i < MAX_NUMBER_OF_RECENT_PROJECTS; ++i)
		if (recent_projects[i]) break;
	if (i < MAX_NUMBER_OF_RECENT_PROJECTS)
		EnableMenuItem(hmenu, ID_TASEDIT_FILE_RECENT, MF_ENABLED);
	else
		EnableMenuItem(hmenu, ID_TASEDIT_FILE_RECENT, MF_GRAYED);

	DrawMenuBar(hwndTasEdit);
}
void UpdateRecentProjectsArray(const char* addString)
{
	// find out if the filename is already in the recent files list
	for(unsigned int x = 0; x < MAX_NUMBER_OF_RECENT_PROJECTS; x++)
	{
		if(recent_projects[x])
		{
			if(!strcmp(recent_projects[x], addString))    // Item is already in list
			{
				// If the filename is in the file list don't add it again, move it up in the list instead
				char* tmp = recent_projects[x];			// save pointer
				for(int y = x; y; y--)
					// Move items down.
					recent_projects[y] = recent_projects[y - 1];
				// Put item on top.
				recent_projects[0] = tmp;
				UpdateRecentProjectsMenu();
				return;
			}
		}
	}
	// The filename wasn't found in the list. That means we need to add it.
	// If there's no space left in the recent files list, get rid of the last item in the list
	if(recent_projects[MAX_NUMBER_OF_RECENT_PROJECTS-1])
		free(recent_projects[MAX_NUMBER_OF_RECENT_PROJECTS-1]);
	// Move other items down
	for(unsigned int x = MAX_NUMBER_OF_RECENT_PROJECTS-1; x; x--)
		recent_projects[x] = recent_projects[x-1];
	// Add new item
	recent_projects[0] = (char*)malloc(strlen(addString) + 1);
	strcpy(recent_projects[0], addString);

	UpdateRecentProjectsMenu();
}
void RemoveRecentProject(unsigned int which)
{
	if (which >= MAX_NUMBER_OF_RECENT_PROJECTS) return;
	// Remove the item
	if(recent_projects[which])
		free(recent_projects[which]);
	// If the item is not the last one in the list, shift the remaining ones up
	if (which < MAX_NUMBER_OF_RECENT_PROJECTS-1)
	{
		// Move the remaining items up
		for(unsigned int x = which+1; x < MAX_NUMBER_OF_RECENT_PROJECTS; ++x)
		{
			recent_projects[x-1] = recent_projects[x];	// Shift each remaining item up by 1
		}
	}
	recent_projects[MAX_NUMBER_OF_RECENT_PROJECTS-1] = 0;	// Clear out the last item since it is empty now

	UpdateRecentProjectsMenu();
}
