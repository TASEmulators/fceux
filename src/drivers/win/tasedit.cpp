#include <fstream>
#include <sstream>
#include "taseditlib/taseditor_project.h"
#include "utils/xstring.h"
#include "keyboard.h"
#include "joystick.h"
#include "main.h"			// for GetRomName
#include "tasedit.h"
#include "version.h"
#include <Shlwapi.h>		// for StrStrI
#include "Win32InputBox.h"

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
char buttonNames[NUM_JOYPAD_BUTTONS][2] = {"A", "B", "S", "T", "U", "D", "L", "R"};

// enterframe function
void UpdateTasEdit()
{
	if(!taseditor_window.hwndTasEditor)
	{
		// TAS Editor is not engaged... but we still should run Lua auto function
		TaseditorAutoFunction();
		return;
	}

	// update all modules that need to be updated preiodically
	recorder.update();
	list.update();
	current_markers.update();
	greenzone.update();
	playback.update();
	bookmarks.update();
	popup_display.update();
	selection.update();
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
			if (taseditor_config.bind_markers)
				current_markers.insertEmpty(*it, frames);
			frames = 1;
		} else frames++;
	}
	if (taseditor_config.bind_markers)
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
			if (taseditor_config.bind_markers)
				current_markers.insertEmpty(*it,frames);
			frames = 1;
		} else frames++;
	}
	if (taseditor_config.bind_markers)
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
	if(CWin32InputBox::GetInteger("Insert number of Frames", "How many frames?", frames, taseditor_window.hwndTasEditor) == IDOK)
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
			if (taseditor_config.bind_markers)
			{
				current_markers.insertEmpty(index, frames);
				selection.must_find_current_marker = playback.must_find_current_marker = true;
			}
			// select inserted rows
			list.update();
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
		if (taseditor_config.bind_markers)
			current_markers.EraseMarker(*it);
	}
	if (taseditor_config.bind_markers)
		selection.must_find_current_marker = playback.must_find_current_marker = true;
	// check if user deleted all frames
	if (!currMovieData.getNumRecords())
		playback.StartFromZero();
	// reduce list
	list.update();

	int result = history.RegisterChanges(MODTYPE_DELETE, start_index);
	if (result >= 0)
	{
		greenzone.InvalidateAndCheck(result);
	} else if (greenzone.greenZoneCount >= currMovieData.getNumRecords())
	{
		greenzone.InvalidateAndCheck(currMovieData.getNumRecords()-1);
	} else list.RedrawList();
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
		if (taseditor_config.bind_markers)
		{
			current_markers.SetMarkersSize(frame+1);
			selection.must_find_current_marker = playback.must_find_current_marker = true;
		}
		list.update();
		int result = history.RegisterChanges(MODTYPE_TRUNCATE, frame+1);
		if (result >= 0)
		{
			greenzone.InvalidateAndCheck(result);
		} else if (greenzone.greenZoneCount >= currMovieData.getNumRecords())
		{
			greenzone.InvalidateAndCheck(currMovieData.getNumRecords()-1);
		} else list.RedrawList();
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
	list.SetHeaderColumnLight(column, HEADER_LIGHT_MAX);
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

			if (!OpenClipboard(taseditor_window.hwndTasEditor))
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

	if (!OpenClipboard(taseditor_window.hwndTasEditor)) return false;

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
				
				if (!taseditor_config.superimpose_affects_paste || taseditor_config.superimpose == BST_UNCHECKED)
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
						if (taseditor_config.superimpose_affects_paste && (taseditor_config.superimpose == BST_CHECKED || (taseditor_config.superimpose == BST_INDETERMINATE && new_buttons == 0)))
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
				if (taseditor_config.superimpose_affects_paste && (taseditor_config.superimpose == BST_CHECKED || (taseditor_config.superimpose == BST_INDETERMINATE && new_buttons == 0)))
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
						list.SetHeaderColumnLight(COLUMN_JOYPAD1_A + joy * NUM_JOYPAD_BUTTONS + btn, HEADER_LIGHT_MAX);
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

	if (!OpenClipboard(taseditor_window.hwndTasEditor)) return false;

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
				if (taseditor_config.bind_markers) current_markers.insertEmpty(pos, 1);
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
			if (taseditor_config.bind_markers)
				selection.must_find_current_marker = playback.must_find_current_marker = true;
			greenzone.InvalidateAndCheck(history.RegisterChanges(MODTYPE_PASTEINSERT, *current_selection_begin));
			// flash list header columns that were changed during paste
			for (int joy = 0; joy < NUM_JOYPADS; ++joy)
			{
				for (int btn = 0; btn < NUM_JOYPAD_BUTTONS; ++btn)
				{
					if (flash_joy[joy] & (1 << btn))
						list.SetHeaderColumnLight(COLUMN_JOYPAD1_A + joy * NUM_JOYPAD_BUTTONS + btn, HEADER_LIGHT_MAX);
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
	// remember to update fourscore status
	bool last_fourscore = currMovieData.fourscore;
	// try to load project
	if (project.load(fullname))
	{
		// update fourscore status
		if (last_fourscore && !currMovieData.fourscore)
		{
			list.RemoveFourscore();
			FCEUD_SetInput(currMovieData.fourscore, currMovieData.microphone, (ESI)currMovieData.ports[0], (ESI)currMovieData.ports[1], (ESIFC)currMovieData.ports[2]);
		} else if (!last_fourscore && currMovieData.fourscore)
		{
			list.AddFourscore();
			FCEUD_SetInput(currMovieData.fourscore, currMovieData.microphone, (ESI)currMovieData.ports[0], (ESI)currMovieData.ports[1], (ESIFC)currMovieData.ports[2]);
		}
		taseditor_window.UpdateRecentProjectsArray(fullname);
		taseditor_window.RedrawWindow();
		taseditor_window.RedrawCaption();
		search_similar_marker = 0;
		return true;
	} else
	{
		// failed to load
		taseditor_window.RedrawWindow();
		taseditor_window.RedrawCaption();
		search_similar_marker = 0;
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
	taseditor_window.RedrawCaption();
	return true;
}
bool SaveProject()
{
	if (!project.save())
		return SaveProjectAs();
	else
		taseditor_window.RedrawCaption();
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
	if (DialogBox(fceu_hInstance, MAKEINTRESOURCE(IDD_TASEDIT_SAVECOMPACT), taseditor_window.hwndTasEditor, SaveCompactProc) > 0)
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
			CheckDlgButton(hwndDlg, IDC_NOTES_TO_SUBTITLES, taseditor_config.last_export_subtitles?MF_CHECKED : MF_UNCHECKED);
			return TRUE;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_RADIO_1PLAYER:
					taseditor_config.last_export_type = EXPORT_TYPE_1P;
					break;
				case IDC_RADIO_2PLAYERS:
					taseditor_config.last_export_type = EXPORT_TYPE_2P;
					break;
				case IDC_RADIO_FOURSCORE:
					taseditor_config.last_export_type = EXPORT_TYPE_FOURSCORE;
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
	if (DialogBox(fceu_hInstance, MAKEINTRESOURCE(IDD_TASEDIT_EXPORT), taseditor_window.hwndTasEditor, ExportProc) > 0)
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
			switch (taseditor_config.last_export_type)
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

bool EnterTasEdit()
{
	if(!FCEU_IsValidUI(FCEUI_TASEDIT)) return false;
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
				movieMode = MOVIEMODE_TASEDIT;
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
				movieMode = MOVIEMODE_TASEDIT;
				currMovieData.emuVersion = FCEU_VERSION_NUMERIC;
				greenzone.TryDumpIncremental(lagFlag != 0);
			}
			// now create initiasl snapshot in history
			history.reset();
			// force the input configuration stored in the movie to apply to FCEUX config
			currMovieData.ports[0] = SI_GAMEPAD;
			currMovieData.ports[1] = SI_GAMEPAD;
			FCEUD_SetInput(currMovieData.fourscore, currMovieData.microphone, (ESI)currMovieData.ports[0], (ESI)currMovieData.ports[1], (ESIFC)currMovieData.ports[2]);
			
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

bool ExitTasEdit()
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

