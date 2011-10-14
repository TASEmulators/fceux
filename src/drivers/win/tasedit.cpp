#include <set>
#include <fstream>
#include <sstream>

#include "common.h"
#include "taseditlib/taseditproj.h"
#include "fceu.h"
#include "debugger.h"
#include "replay.h"
#include "utils/xstring.h"
#include "Win32InputBox.h"
#include "window.h"
#include "keyboard.h"
#include "joystick.h"
#include "help.h"
#include "main.h"
#include "tasedit.h"

using namespace std;

//to change header font
//http://forums.devx.com/archive/index.php/t-37234.html

int old_multitrack_recording_joypad, multitrack_recording_joypad;
bool old_movie_readonly;
bool TASEdit_focus = false;
bool Tasedit_rewind_now = false;
int listItems;	// number of items per list page
// saved FCEU config
int saved_eoptions;
int saved_EnableAutosave;
extern int EnableAutosave;

// vars saved in cfg file
int TasEdit_wndx, TasEdit_wndy;
bool TASEdit_follow_playback = true;
bool TASEdit_show_lag_frames = true;
bool TASEdit_show_markers = true;
bool TASEdit_bind_markers = true;
bool TASEdit_restore_position = false;
int TASEdit_greenzone_capacity = GREENZONE_CAPACITY_DEFAULT;
extern bool muteTurbo;
bool TASEdit_show_dot = true;
int TasEdit_undo_levels = UNDO_LEVELS_DEFAULT;
bool TASEdit_jump_to_undo = true;

string tasedithelp = "{16CDE0C4-02B0-4A60-A88D-076319909A4D}"; //Name of TASEdit Help page
char buttonNames[NUM_JOYPAD_BUTTONS][2] = {"A", "B", "S", "T", "U", "D", "L", "R"};
char windowCaptions[6][30] = {	"TAS Editor",
								"TAS Editor (Recording All)",
								"TAS Editor (Recording 1P)",
								"TAS Editor (Recording 2P)",
								"TAS Editor (Recording 3P)",
								"TAS Editor (Recording 4P)"};
char bookmarksCaption[2][23] = { " Bookmarks ", " Bookmarks / Branches " };

HWND hwndTasEdit = 0;
HMENU hmenu, hrmenu;
HWND hwndList, hwndHeader;
WNDPROC hwndList_oldWndProc, hwndHeader_oldWndproc;
HWND hwndHistoryList;
WNDPROC hwndHistoryList_oldWndProc;
HWND hwndBookmarksList, hwndBookmarks;
WNDPROC hwndBookmarksList_oldWndProc;
HWND hwndProgressbar, hwndRewind, hwndForward;
HWND hwndRB_RecOff, hwndRB_RecAll, hwndRB_Rec1P, hwndRB_Rec2P, hwndRB_Rec3P, hwndRB_Rec4P;


typedef std::set<int> TSelectionFrames;
static TSelectionFrames selectionFrames;

//hacky.. we need to think about how to convey information from the driver to the movie code.
//add a new fceud_ function?? blehhh maybe
extern EMOVIEMODE movieMode;

// all Taseditor functional modules
TASEDIT_PROJECT project;
INPUT_HISTORY history;
PLAYBACK playback;
GREENZONE greenzone;


void GetDispInfo(NMLVDISPINFO* nmlvDispInfo)
{
	LVITEM& item = nmlvDispInfo->item;
	if(item.mask & LVIF_TEXT)
	{
		switch(item.iSubItem)
		{
			case COLUMN_ICONS:
			if(item.iImage == I_IMAGECALLBACK && item.iItem == currFrameCounter)
				item.iImage = 0;
			else
				item.iImage = -1;
			break;
			case COLUMN_FRAMENUM:
			case COLUMN_FRAMENUM2:
			{
				U32ToDecStr(item.pszText,item.iItem,DIGITS_IN_FRAMENUM);
				break;
			}
			case COLUMN_JOYPAD1_A: case COLUMN_JOYPAD1_B: case COLUMN_JOYPAD1_S: case COLUMN_JOYPAD1_T:
			case COLUMN_JOYPAD1_U: case COLUMN_JOYPAD1_D: case COLUMN_JOYPAD1_L: case COLUMN_JOYPAD1_R:
			case COLUMN_JOYPAD2_A: case COLUMN_JOYPAD2_B: case COLUMN_JOYPAD2_S: case COLUMN_JOYPAD2_T:
			case COLUMN_JOYPAD2_U: case COLUMN_JOYPAD2_D: case COLUMN_JOYPAD2_L: case COLUMN_JOYPAD2_R:
			case COLUMN_JOYPAD3_A: case COLUMN_JOYPAD3_B: case COLUMN_JOYPAD3_S: case COLUMN_JOYPAD3_T:
			case COLUMN_JOYPAD3_U: case COLUMN_JOYPAD3_D: case COLUMN_JOYPAD3_L: case COLUMN_JOYPAD3_R:
			case COLUMN_JOYPAD4_A: case COLUMN_JOYPAD4_B: case COLUMN_JOYPAD4_S: case COLUMN_JOYPAD4_T:
			case COLUMN_JOYPAD4_U: case COLUMN_JOYPAD4_D: case COLUMN_JOYPAD4_L: case COLUMN_JOYPAD4_R:
			{
				int joy = (item.iSubItem - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS;
				int bit = (item.iSubItem - COLUMN_JOYPAD1_A) % NUM_JOYPAD_BUTTONS;
				uint8 data = currMovieData.records[item.iItem].joysticks[joy];
				if(data & (1<<bit))
				{
					item.pszText[0] = MovieRecord::mnemonics[bit];
					item.pszText[1] = 0;
				} else
				{
					if (TASEdit_show_dot)
					{
						item.pszText[0] = 46;	// "."
						item.pszText[1] = 0;
					} else item.pszText[0] = 0;
				}
			}
			break;
		}
	}
}

#define CDDS_SUBITEMPREPAINT       (CDDS_SUBITEM | CDDS_ITEMPREPAINT)
#define CDDS_SUBITEMPOSTPAINT      (CDDS_SUBITEM | CDDS_ITEMPOSTPAINT)
#define CDDS_SUBITEMPREERASE       (CDDS_SUBITEM | CDDS_ITEMPREERASE)
#define CDDS_SUBITEMPOSTERASE      (CDDS_SUBITEM | CDDS_ITEMPOSTERASE)

LONG CustomDraw(NMLVCUSTOMDRAW* msg)
{
	int cell_x, cell_y;
	switch(msg->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;
	case CDDS_ITEMPREPAINT:
		return CDRF_NOTIFYSUBITEMDRAW;
	case CDDS_SUBITEMPREPAINT:
		SelectObject(msg->nmcd.hdc,debugSystem->hFixedFont);
		cell_x = msg->iSubItem;
		cell_y = msg->nmcd.dwItemSpec;

		//msg->clrText = 0xFF0000;

		if(cell_x > COLUMN_ICONS)
		{
			if(cell_x == COLUMN_FRAMENUM || cell_x == COLUMN_FRAMENUM2)
			{
				// frame number
				if (cell_y == history.GetUndoHint())
				{
					// undo hint here
					msg->clrTextBk = UNDOHINT_FRAMENUM_COLOR;
				} else if (cell_y == currFrameCounter || cell_y == playback.GetPauseFrame())
				{
					// current frame
					if(TASEdit_show_markers && currMovieData.frames_flags[cell_y] & MARKER_FLAG_BIT)
						// this frame is also marked
						msg->clrTextBk = CUR_MARKED_FRAMENUM_COLOR;
					else
						msg->clrTextBk = CUR_FRAMENUM_COLOR;
				} else if(TASEdit_show_markers && currMovieData.frames_flags[cell_y] & MARKER_FLAG_BIT)
				{
					// marked frame
					msg->clrTextBk = MARKED_FRAMENUM_COLOR;
				} else if(cell_y < greenzone.greenZoneCount && !greenzone.savestates[cell_y].empty())
				{
					if (TASEdit_show_lag_frames && (currMovieData.frames_flags[cell_y] & LAG_FLAG_BIT))
					{
						// lag frame
						msg->clrTextBk = LAG_FRAMENUM_COLOR;
					} else
					{
						// green zone frame
						msg->clrTextBk = GREENZONE_FRAMENUM_COLOR;
					}
				} else msg->clrTextBk = NORMAL_FRAMENUM_COLOR;
			} else if((cell_x - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS == 0 || (cell_x - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS == 2)
			{
				// pad 1 or 3
				if (cell_y == history.GetUndoHint())
				{
					// undo hint here
					msg->clrTextBk = UNDOHINT_INPUT_COLOR1;
				} else if (cell_y == currFrameCounter || cell_y == playback.GetPauseFrame())
				{
					// current frame
					msg->clrTextBk = CUR_INPUT_COLOR1;
				} else if(cell_y < greenzone.greenZoneCount && !greenzone.savestates[cell_y].empty())
				{
					if (TASEdit_show_lag_frames && (currMovieData.frames_flags[cell_y] & LAG_FLAG_BIT))
					{
						// lag frame
						msg->clrTextBk = LAG_INPUT_COLOR1;
					} else
					{
						// green zone frame
						msg->clrTextBk = GREENZONE_INPUT_COLOR1;
					}
				} else msg->clrTextBk = NORMAL_INPUT_COLOR1;
			} else if((cell_x - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS == 1 || (cell_x - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS == 3)
			{
				// pad 2 or 4
				if (cell_y == history.GetUndoHint())
				{
					// undo hint here
					msg->clrTextBk = UNDOHINT_INPUT_COLOR2;
				} else if (cell_y == currFrameCounter || cell_y == playback.GetPauseFrame())
				{
					// current frame
					msg->clrTextBk = CUR_INPUT_COLOR2;
				} else if(cell_y < greenzone.greenZoneCount && !greenzone.savestates[cell_y].empty())
				{
					if (TASEdit_show_lag_frames && (currMovieData.frames_flags[cell_y] & LAG_FLAG_BIT))
					{
						// lag frame
						msg->clrTextBk = LAG_INPUT_COLOR2;
					} else
					{
						// green zone frame
						msg->clrTextBk = GREENZONE_INPUT_COLOR2;
					}
				} else msg->clrTextBk = NORMAL_INPUT_COLOR2;
			}
		}
		return CDRF_DODEFAULT;
	default:
		return CDRF_DODEFAULT;
	}
}

// called from the rest of the emulator when things happen and the tasedit should change to reflect it
void UpdateTasEdit()
{
	if(!hwndTasEdit) return;

	UpdateList();
	
	greenzone.update();
	playback.update();
	history.update();

	// update window caption
	if (project.old_changed != project.changed || old_movie_readonly != movie_readonly || old_multitrack_recording_joypad != multitrack_recording_joypad)
		RedrawWindowCaption();
	project.update();

	// update Bookmarks/Branches groupbox caption
	if (old_movie_readonly != movie_readonly)
		RedrawBookmarksCaption();

	// update recording radio buttons if user used hotkey to switch R/W
	if (old_movie_readonly != movie_readonly || old_multitrack_recording_joypad != multitrack_recording_joypad)
	{
		UncheckRecordingRadioButtons();
		RecheckRecordingRadioButtons();
	}

}

void UpdateList()
{
	//update the number of items in the list
	int currLVItemCount = ListView_GetItemCount(hwndList);
	int movie_size = currMovieData.getNumRecords();
	if(currLVItemCount != movie_size)
	{
		ListView_SetItemCountEx(hwndList,movie_size,LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);
		// also reduce selection if needed
		if (selectionFrames.size())
		{
			int delete_index;
			while(1)
			{
				delete_index = *selectionFrames.rbegin();
				if (delete_index < movie_size) break;
				// reduce selection manually, because reduced list won't call ItemChanged for these rows
				selectionFrames.erase(delete_index);
				if (!selectionFrames.size()) break;
			}
		}
	}
}

void RedrawWindowCaption()
{
	char windowCaption[300];	// 260 + 30 + 1 + ...
	if (movie_readonly)
		strcpy(windowCaption, windowCaptions[0]);
	else
		strcpy(windowCaption, windowCaptions[multitrack_recording_joypad + 1]);
	// add project name
	std::string projectname = project.GetProjectName();
	if (!projectname.empty())
	{
		strcat(windowCaption, " - ");
		strcat(windowCaption, projectname.c_str());
	}
	// and * if project has unsaved changes
	if (project.changed)
		strcat(windowCaption, "*");
	SetWindowText(hwndTasEdit, windowCaption);
}
void RedrawBookmarksCaption()
{
	SetWindowText(hwndBookmarks, bookmarksCaption[(movie_readonly)?0:1]);
}
void RedrawTasedit()
{
	InvalidateRect(hwndTasEdit,0,FALSE);
}
void RedrawList()
{
	InvalidateRect(hwndList,0,FALSE);
}
void RedrawRow(int index)
{
	ListView_RedrawItems(hwndList,index,index);
}

enum ECONTEXTMENU
{
	CONTEXTMENU_STRAY = 0,
	CONTEXTMENU_SELECTED = 1,
};

void ShowMenu(ECONTEXTMENU which, POINT& pt)
{
	HMENU sub = GetSubMenu(hrmenu,(int)which);
	TrackPopupMenu(sub,0,pt.x,pt.y,TPM_RIGHTBUTTON,hwndTasEdit,0);
}

void StrayClickMenu(LPNMITEMACTIVATE info)
{
	POINT pt = info->ptAction;
	ClientToScreen(hwndList,&pt);
	ShowMenu(CONTEXTMENU_STRAY,pt);
}

void RightClickMenu(LPNMITEMACTIVATE info)
{
	POINT pt = info->ptAction;
	ClientToScreen(hwndList,&pt);
	ShowMenu(CONTEXTMENU_SELECTED,pt);
}

void RightClick(LPNMITEMACTIVATE info)
{
	int index = info->iItem;
	int column = info->iSubItem;

	//stray clicks give a context menu:
	if(index == -1)
	{
		StrayClickMenu(info);
		return;
	}

	//make sure that the click is in our currently selected set.
	//if it is not, then we don't know what to do yet
	if(selectionFrames.find(index) == selectionFrames.end())
	{
		return;
	}
	
	RightClickMenu(info);
}

void MarkersChanged()
{
	project.changed = true;
}

void InputChangedRec()
{
	greenzone.InvalidateGreenZone(history.RegisterInputChanges(MODTYPE_RECORD, currFrameCounter, currFrameCounter));
}

void ToggleJoypadBit(int column_index, int row_index, UINT KeyFlags)
{
	int joy = (column_index - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS;
	int bit = (column_index - COLUMN_JOYPAD1_A) % NUM_JOYPAD_BUTTONS;
	if (KeyFlags & (LVKF_SHIFT|LVKF_CONTROL))
	{
		//update multiple rows 
		for(TSelectionFrames::iterator it(selectionFrames.begin()); it != selectionFrames.end(); it++)
		{
			currMovieData.records[*it].toggleBit(joy,bit);
		}
		greenzone.InvalidateGreenZone(history.RegisterInputChanges(MODTYPE_CHANGE, *selectionFrames.begin(), *selectionFrames.rbegin()));
	} else
	{
		//update one row
		currMovieData.records[row_index].toggleBit(joy,bit);
		if (currMovieData.records[row_index].checkBit(joy,bit))
			greenzone.InvalidateGreenZone(history.RegisterInputChanges(MODTYPE_SET, row_index, row_index));
		else
			greenzone.InvalidateGreenZone(history.RegisterInputChanges(MODTYPE_UNSET, row_index, row_index));
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
		ClearSelection();
		playback.jump(row_index);
		RedrawList();
	} else if(column_index == COLUMN_FRAMENUM || column_index == COLUMN_FRAMENUM2)
	{
		// click on the "frame number" column - set marker if clicked with Alt
		if (info->uKeyFlags & LVKF_ALT)
		{
			// reverse MARKER_FLAG_BIT in pointed frame
			if (currMovieData.frames_flags[row_index] & MARKER_FLAG_BIT)
				currMovieData.frames_flags[row_index] &= ~MARKER_FLAG_BIT;
			else
				currMovieData.frames_flags[row_index] |= MARKER_FLAG_BIT;
			MarkersChanged();
			ListView_SetItemState(hwndList,row_index,0,LVIS_SELECTED);
			//RedrawList();
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
		ClearSelection();
		playback.jump(row_index);
		RedrawList();
	} else if(column_index >= COLUMN_JOYPAD1_A && column_index <= COLUMN_JOYPAD4_R)
	{
		ToggleJoypadBit(column_index, row_index, info->uKeyFlags);
	}
}

void CloneFrames()
{
	int frames = selectionFrames.size();

	currMovieData.records.reserve(currMovieData.getNumRecords() + frames);
	currMovieData.frames_flags.reserve(currMovieData.getNumRecords() + frames);

	//insert frames before each selection, but consecutive selection lines are accounted as single region
	frames = 1;
	TSelectionFrames::reverse_iterator next_it;
	for(TSelectionFrames::reverse_iterator it(selectionFrames.rbegin()); it != selectionFrames.rend(); it++)
	{
		next_it = it;
		next_it++;
		if (next_it == selectionFrames.rend() || (int)*next_it < ((int)*it - 1))
		{
			// end of current region
			currMovieData.cloneRegion(*it,frames);
			frames = 1;
		} else frames++;
	}
	UpdateList();
	greenzone.InvalidateGreenZone(history.RegisterInputChanges(MODTYPE_CLONE, *selectionFrames.begin()));
}

void InsertFrames()
{
	int frames = selectionFrames.size();

	//this is going to be slow.

	//to keep this from being even slower than it would otherwise be, go ahead and reserve records
	currMovieData.records.reserve(currMovieData.getNumRecords() + frames);
	currMovieData.frames_flags.reserve(currMovieData.getNumRecords() + frames);

	//insert frames before each selection, but consecutive selection lines are accounted as single region
	frames = 1;
	TSelectionFrames::reverse_iterator next_it;
	for(TSelectionFrames::reverse_iterator it(selectionFrames.rbegin()); it != selectionFrames.rend(); it++)
	{
		next_it = it;
		next_it++;
		if (next_it == selectionFrames.rend() || (int)*next_it < ((int)*it - 1))
		{
			// end of current region
			currMovieData.insertEmpty(*it,frames);
			frames = 1;
		} else frames++;
	}
	UpdateList();
	greenzone.InvalidateGreenZone(history.RegisterInputChanges(MODTYPE_INSERT, *selectionFrames.begin()));
}

void DeleteFrames()
{
	int start_index = *selectionFrames.begin();
	int end_index = *selectionFrames.rbegin();
	//delete frames on each selection, going backwards
	for(TSelectionFrames::reverse_iterator it(selectionFrames.rbegin()); it != selectionFrames.rend(); it++)
	{
		currMovieData.records.erase(currMovieData.records.begin() + *it);
		if (TASEdit_bind_markers)
			currMovieData.frames_flags.erase(currMovieData.frames_flags.begin() + *it);
	}
	// check if user deleted all frames
	if (!currMovieData.getNumRecords())
		playback.StartFromZero();
	// reduce list
	UpdateList();

	greenzone.InvalidateGreenZone(history.RegisterInputChanges(MODTYPE_DELETE, start_index));
}

void ClearFrames(bool cut)
{
	//clear input on each selection
	for(TSelectionFrames::iterator it(selectionFrames.begin()); it != selectionFrames.end(); it++)
	{
		currMovieData.records[*it].clear();
	}
	if (cut)
		greenzone.InvalidateGreenZone(history.RegisterInputChanges(MODTYPE_CUT, *selectionFrames.begin(), *selectionFrames.rbegin()));
	else
		greenzone.InvalidateGreenZone(history.RegisterInputChanges(MODTYPE_CLEAR, *selectionFrames.begin(), *selectionFrames.rbegin()));
}

void Truncate()
{
	int frame = currFrameCounter;
	if (selectionFrames.size())
	{
		frame=*selectionFrames.begin();
		ClearSelection();
	}
	if (currMovieData.getNumRecords() > frame+1)
	{
		currMovieData.truncateAt(frame+1);
		UpdateList();
		greenzone.InvalidateGreenZone(history.RegisterInputChanges(MODTYPE_TRUNCATE, frame+1));
	}
}

//the column set operation, for setting a button/Marker for a span of selected values
void ColumnSet(int column)
{
	if (column == COLUMN_FRAMENUM || column == COLUMN_FRAMENUM2)
	{
		// Markers column
		//inspect the selected frames, if they are all set, then unset all, else set all
		bool unset_found = false;
		for(TSelectionFrames::iterator it(selectionFrames.begin()); it != selectionFrames.end(); it++)
		{
			if(!(currMovieData.frames_flags[*it] & MARKER_FLAG_BIT))
			{
				unset_found = true;
				break;
			}
		}
		if (unset_found)
		{
			// set all
			for(TSelectionFrames::iterator it(selectionFrames.begin()); it != selectionFrames.end(); it++)
				currMovieData.frames_flags[*it] |= MARKER_FLAG_BIT;
		} else
		{
			// unset all
			for(TSelectionFrames::iterator it(selectionFrames.begin()); it != selectionFrames.end(); it++)
				currMovieData.frames_flags[*it] &= ~MARKER_FLAG_BIT;
		}
		MarkersChanged();
		ClearSelection();
		RedrawList();
	} else
	{
		// buttons column
		int joy = (column - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS;
		if (joy < 0 || joy >= NUM_JOYPADS) return;
		int button = (column - COLUMN_JOYPAD1_A) % NUM_JOYPAD_BUTTONS;
		//inspect the selected frames, if they are all set, then unset all, else set all
		bool newValue = false;
		for(TSelectionFrames::iterator it(selectionFrames.begin()); it != selectionFrames.end(); it++)
		{
			if(!(currMovieData.records[*it].checkBit(joy,button)))
			{
				newValue = true;
				break;
			}
		}
		// apply newValue
		for(TSelectionFrames::iterator it(selectionFrames.begin()); it != selectionFrames.end(); it++)
			currMovieData.records[*it].setBitValue(joy,button,newValue);
		if (newValue)
			greenzone.InvalidateGreenZone(history.RegisterInputChanges(MODTYPE_SET, *selectionFrames.begin(), *selectionFrames.rbegin()));
		else
			greenzone.InvalidateGreenZone(history.RegisterInputChanges(MODTYPE_UNSET, *selectionFrames.begin(), *selectionFrames.rbegin()));
	}
}

void ClearSelection()
{
	ListView_SetItemState(hwndList,-1,0, LVIS_SELECTED);
}
void ClearRowSelection(int index)
{
	ListView_SetItemState(hwndList,index,0, LVIS_SELECTED);
}

void SelectAll()
{
	ListView_SetItemState(hwndList,-1,LVIS_SELECTED, LVIS_SELECTED);
	RedrawList();
}
void SelectMidMarkers()
{
	int center, upper_border, lower_border;
	int upper_marker, lower_marker;
	int movie_size = currMovieData.getNumRecords();

	// if selection size=0 then playback cursor is selected and serves as center
	if (selectionFrames.size())
	{
		upper_border = center = *selectionFrames.begin();
		lower_border = *selectionFrames.rbegin();
	} else lower_border = upper_border = center = currFrameCounter;

	// find markers
	// searching up starting from center-0
	for (upper_marker = center; upper_marker >= 0; upper_marker--)
		if (currMovieData.frames_flags[upper_marker] & MARKER_FLAG_BIT) break;
	// searching down starting from center+1
	for (lower_marker = center+1; lower_marker < movie_size; ++lower_marker)
		if (currMovieData.frames_flags[lower_marker] & MARKER_FLAG_BIT) break;
	
	if (upper_marker == -1 && lower_marker == movie_size)
	{
		SelectAll();
		return;
	}

	ClearSelection();
	// selecting circle:
	if (upper_border > upper_marker+1 || lower_border < lower_marker-1 || lower_border > lower_marker)
	{
		// default: select all between markers
		for (int i = upper_marker+1; i < lower_marker; ++i)
		{
			ListView_SetItemState(hwndList,i,LVIS_SELECTED,LVIS_SELECTED);
		}
	} else if (upper_border == upper_marker+1 && lower_border == lower_marker-1)
	{
		// already selected all between markers - now select both markers or at least select the marker that is not outside movie range
		if (upper_marker < 0) upper_marker = 0;
		if (lower_marker >= movie_size) lower_marker = movie_size - 1;
		for (int i = upper_marker; i <= lower_marker; ++i)
		{
			ListView_SetItemState(hwndList,i,LVIS_SELECTED,LVIS_SELECTED);
		}
	} else if (upper_border <= upper_marker && lower_border >= lower_marker)
	{
		// selected all between markers and both markers selected too - now deselect lower marker
		for (int i = upper_marker; i < lower_marker; ++i)
		{
			ListView_SetItemState(hwndList,i,LVIS_SELECTED,LVIS_SELECTED);
		}
	} else if (upper_border == upper_marker && lower_border == lower_marker-1)
	{
		// selected all between markers and upper marker selected too - now deselect upper marker and (if lower marker < movie_size) reselect lower marker
		if (lower_marker >= movie_size) lower_marker = movie_size - 1;
		for (int i = upper_marker+1; i <= lower_marker; ++i)
		{
			ListView_SetItemState(hwndList,i,LVIS_SELECTED,LVIS_SELECTED);
		}
	} else if (upper_border == upper_marker+1 && lower_border == lower_marker)
	{
		// selected all between markers and lower marker selected too - now deselect lower marker (return to "selected all between markers")
		for (int i = upper_marker + 1; i < lower_marker; ++i)
		{
			ListView_SetItemState(hwndList,i,LVIS_SELECTED,LVIS_SELECTED);
		}
	}
}

bool Copy()
{
	if (selectionFrames.size()==0) return false;

	int cframe=*selectionFrames.begin()-1;
    try 
	{
		int range = *selectionFrames.rbegin() - *selectionFrames.begin()+1;
		//std::string outbuf clipString("TAS");

		std::stringstream clipString;
		clipString << "TAS " << range << std::endl;

		for(TSelectionFrames::iterator it(selectionFrames.begin()); it != selectionFrames.end(); it++)
		{
			if (*it>cframe+1)
			{
				clipString << '+' << (*it-cframe) << '|';
			}
			cframe=*it;

			int cjoy=0;
			for (int joy=0; joy<NUM_JOYPADS; ++joy)
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
						clipString << MovieRecord::mnemonics[bit];
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

	return true;
}
void Cut()
{
	if (Copy())
	{
		ClearFrames(true);
	}
}
bool Paste()
{
	bool result = false;
	if (selectionFrames.size()==0)
		return false;

	int pos = *selectionFrames.begin();

	if (!OpenClipboard(hwndTasEdit))
		return false;
	
	HANDLE hGlobal = GetClipboardData(CF_TEXT);
	if (hGlobal)
	{
		char *pGlobal = (char*)GlobalLock((HGLOBAL)hGlobal);

		// TAS recording info starts with "TAS ".
		if (pGlobal[0]=='T' && pGlobal[1]=='A' && pGlobal[2]=='S')
		{
			int range;

			// Extract number of frames
			sscanf (pGlobal+3, "%d", &range);
			if (currMovieData.getNumRecords() < pos+range)
			{
				currMovieData.insertEmpty(currMovieData.getNumRecords(),pos+range-currMovieData.getNumRecords());
			}

			pGlobal = strchr(pGlobal, '\n');
			int joy=0;
			--pos;
			
			while (pGlobal++ && *pGlobal!='\0')
			{
				char *frame = pGlobal;

				// Detect skipped frames in paste.
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

				currMovieData.records[pos].joysticks[0]=0;
				currMovieData.records[pos].joysticks[1]=0;
				int joy=0;

				while (*frame && *frame != '\n' && *frame !='\r')
				{
					switch (*frame)
					{
					case '|': // Joystick marker
						++joy;
						break;
					default:
						for (int bit=0; bit<NUM_JOYPAD_BUTTONS; ++bit)
						{
							if (*frame==MovieRecord::mnemonics[bit])
							{
								currMovieData.records[pos].joysticks[joy]|=(1<<bit);
								break;
							}
						}
						break;
					}
					++frame;
				}

				pGlobal = strchr(pGlobal, '\n');
			}
			greenzone.InvalidateGreenZone(history.RegisterInputChanges(MODTYPE_PASTE, *selectionFrames.begin()));
			result = true;
		}

		GlobalUnlock(hGlobal);
	}

	CloseClipboard();
	return result;
}
// ---------------------------------------------------------------------------------
//The subclass wndproc for the listview header
LRESULT APIENTRY HeaderWndProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
	case WM_SETCURSOR:
		return true;	// no column resizing
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
		{
			if (selectionFrames.size())
			{
				//perform hit test
				HD_HITTESTINFO info;
				info.pt.x = GET_X_LPARAM(lParam);
				info.pt.y = GET_Y_LPARAM(lParam);
				SendMessage(hWnd,HDM_HITTEST,0,(LPARAM)&info);
				if(info.iItem >= COLUMN_FRAMENUM && info.iItem <= COLUMN_FRAMENUM2)
					ColumnSet(info.iItem);
			}
		}
		return true;
	}
	return CallWindowProc(hwndHeader_oldWndproc,hWnd,msg,wParam,lParam);
}

//The subclass wndproc for the listview
LRESULT APIENTRY ListWndProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
	case WM_CHAR:
	case WM_KILLFOCUS:
		return 0;
	case WM_NOTIFY:
		{
			switch (((LPNMHDR)lParam)->code)
			{
			case HDN_BEGINTRACKW:
			case HDN_BEGINTRACKA:
			case HDN_TRACK:
				return true;	// no column resizing
			}
			break;
		}

	}
	return CallWindowProc(hwndList_oldWndProc,hWnd,msg,wParam,lParam);
}

LRESULT APIENTRY HistoryListWndProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
	case WM_CHAR:
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_KILLFOCUS:
		return 0;
	}
	return CallWindowProc(hwndList_oldWndProc,hWnd,msg,wParam,lParam);
}

void AddFourscore()
{
	// add list columns
	LVCOLUMN lvc;
	lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT;
	lvc.fmt = LVCFMT_CENTER;
	lvc.cx = 21;
	int colidx = COLUMN_JOYPAD3_A;
	for (int joy = 0; joy < 2; ++joy)
	{
		for (int btn = 0; btn < NUM_JOYPAD_BUTTONS; ++btn)
		{
			lvc.pszText = buttonNames[btn];
			ListView_InsertColumn(hwndList, colidx++, &lvc);
		}
	}
	// frame number column again
	lvc.cx = 75;
	lvc.pszText = "Frame#";
	ListView_InsertColumn(hwndList, colidx++, &lvc);
	// enable radiobuttons for 3P/4P multitracking
	EnableWindow(hwndRB_Rec3P, true);
	EnableWindow(hwndRB_Rec4P, true);
	// change eoptions
	FCEUI_SetInputFourscore(true);
}
void RemoveFourscore()
{
	// remove list columns
	for (int i = COLUMN_FRAMENUM2; i >= COLUMN_JOYPAD3_A; --i)
	{
		ListView_DeleteColumn (hwndList, i);
	}
	// disable radiobuttons for 3P/4P multitracking
	EnableWindow(hwndRB_Rec3P, false);
	EnableWindow(hwndRB_Rec4P, false);
	// change eoptions
	FCEUI_SetInputFourscore(false);
}

void UncheckRecordingRadioButtons()
{
	Button_SetCheck(hwndRB_RecOff, BST_UNCHECKED);
	Button_SetCheck(hwndRB_RecAll, BST_UNCHECKED);
	Button_SetCheck(hwndRB_Rec1P, BST_UNCHECKED);
	Button_SetCheck(hwndRB_Rec2P, BST_UNCHECKED);
	Button_SetCheck(hwndRB_Rec3P, BST_UNCHECKED);
	Button_SetCheck(hwndRB_Rec4P, BST_UNCHECKED);
}
void RecheckRecordingRadioButtons()
{
	old_movie_readonly = movie_readonly;
	old_multitrack_recording_joypad = multitrack_recording_joypad;
	if (movie_readonly)
	{
		Button_SetCheck(hwndRB_RecOff, BST_CHECKED);
	} else
	{
		switch(multitrack_recording_joypad)
		{
		case MULTITRACK_RECORDING_ALL:
			Button_SetCheck(hwndRB_RecAll, BST_CHECKED);
			break;
		case MULTITRACK_RECORDING_1P:
			Button_SetCheck(hwndRB_Rec1P, BST_CHECKED);
			break;
		case MULTITRACK_RECORDING_2P:
			Button_SetCheck(hwndRB_Rec2P, BST_CHECKED);
			break;
		case MULTITRACK_RECORDING_3P:
			Button_SetCheck(hwndRB_Rec3P, BST_CHECKED);
			break;
		case MULTITRACK_RECORDING_4P:
			Button_SetCheck(hwndRB_Rec4P, BST_CHECKED);
			break;
		}
	}
}

void OpenProject()
{
	if (!AskSaveProject()) return;
		
	//If OPENFILENAME dialog successful, open up a completely new project instance and scrap the old one
	//Run the project Load() function to pull all info from the .tas file into this new project instance

	const char TPfilter[]="TASEdit Project (*.tas)\0*.tas\0\0";	

	OPENFILENAME ofn;								
	memset(&ofn,0,sizeof(ofn));						
	ofn.lStructSize=sizeof(ofn);					
	ofn.hInstance=fceu_hInstance;
	ofn.lpstrTitle="Open TASEdit Project...";
	ofn.lpstrFilter=TPfilter;

	char nameo[2048];								//File name
	strcpy(nameo, GetRomName());					//For now, just use ROM name

	ofn.lpstrFile=nameo;							
	ofn.nMaxFile=256;
	ofn.Flags=OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_FILEMUSTEXIST;
	string initdir =  FCEU_GetPath(FCEUMKF_MOVIE);	
	ofn.lpstrInitialDir=initdir.c_str();

	if(GetOpenFileName(&ofn))							//If it is a valid filename
	{							
		std::string tempstr = nameo;					//Make a temporary string for filename
		char drv[512], dir[512], name[512], ext[512];	//For getting the filename!
		if(tempstr.rfind(".tas") == std::string::npos)	//If they haven't put ".tas" after it, stick it on ourselves
			tempstr.append(".tas");						
		splitpath(tempstr.c_str(), drv, dir, name, ext);	//Split the path...
		project.SetProjectName(name);
		project.SetProjectFile(tempstr);
		//Set the fm2 name
		std::string thisfm2name = name;
		thisfm2name.append(".fm2");
		project.SetFM2Name(thisfm2name);
		// switch to read-only mode, but first must uncheck radiobuttons explicitly
		if (!movie_readonly) FCEUI_MovieToggleReadOnly();
		multitrack_recording_joypad = MULTITRACK_RECORDING_ALL;
		UncheckRecordingRadioButtons();
		RecheckRecordingRadioButtons();
		playback.reset();
		// remember to update fourscore status
		bool last_fourscore = currMovieData.fourscore;
		// Load project
		project.LoadProject(project.GetProjectFile());
		UpdateList();
		// update fourscore status
		if (last_fourscore && !currMovieData.fourscore)
			RemoveFourscore();
		else if (!last_fourscore && currMovieData.fourscore)
			AddFourscore();
		FollowPlayback();
		RedrawTasedit();
		RedrawWindowCaption();
		RedrawBookmarksCaption();
	}
}

// Saves current project
bool SaveProjectAs()
{
//Save project as new user selected filename
//flag project as not changed

	const char TPfilter[]="TASEdit Project (*.tas)\0*.tas\0All Files (*.*)\0*.*\0\0";	//Filetype filter

	OPENFILENAME ofn;								
	memset(&ofn,0,sizeof(ofn));						
	ofn.lStructSize=sizeof(ofn);					
	ofn.hInstance=fceu_hInstance;
	ofn.lpstrTitle="Save TASEdit Project As...";
	ofn.lpstrFilter=TPfilter;

	char nameo[2048];										//File name
	strcpy(nameo, GetRomName());							//For now, just use ROM name

	ofn.lpstrFile=nameo;									//More parameters
	ofn.lpstrDefExt="tas";
	ofn.nMaxFile=256;
	ofn.Flags=OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;
	string initdir =  FCEU_GetPath(FCEUMKF_MOVIE);			//Initial directory
	ofn.lpstrInitialDir=initdir.c_str();

	if(GetSaveFileName(&ofn))								//If it is a valid filename
	{
		std::string tempstr = nameo;						//Make a temporary string for filename
		char drv[512], dir[512], name[512], ext[512];		//For getting the filename!

		splitpath(tempstr.c_str(), drv, dir, name, ext);	//Split it up...
		project.SetProjectName(name);
		project.SetProjectFile(tempstr);
		std::string thisfm2name = name;
		thisfm2name.append(".fm2");							//Setup the fm2 name
		project.SetFM2Name(thisfm2name);					//Set the project's fm2 name
		project.saveProject();
	} else return false;
	// saved successfully - remove * mark from caption
	RedrawWindowCaption();
	return true;
}
bool SaveProject()
{
	if (!project.saveProject())
		return SaveProjectAs();
	else
		RedrawWindowCaption();
	return true;
}

// returns false if user doesn't want to exit
bool AskSaveProject()
{
	bool changes_found = false;
	if (project.changed) changes_found = true;

	// ask saving project
	if (changes_found)
	{
		int answer = MessageBox(hwndTasEdit, "Save Project changes?", "TAS Edit", MB_YESNOCANCEL);
		if(answer == IDYES)
			return SaveProject();
		return (answer != IDCANCEL);
	}
	return true;
}

void Import()
{

}
//Takes current inputlog and saves it as a .fm2 file
void Export()
{
	//TODO: redesign this
	//Dump project header info into file, then comments & subtitles, then input log
	//This will require special prunctions, ::DumpHeader  ::DumpComments etc
	const char filter[]="FCEUX Movie File (*.fm2)\0*.fm2\0All Files (*.*)\0*.*\0\0";
	char fname[2048] = {0};
	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hInstance=fceu_hInstance;
	ofn.lpstrTitle="Export TAS as...";
	ofn.lpstrFilter=filter;
	ofn.lpstrFile=fname;
	ofn.lpstrDefExt="fm2";
	ofn.nMaxFile=256;
	std::string initdir = FCEU_GetPath(FCEUMKF_MOVIE);
	ofn.lpstrInitialDir=initdir.c_str();
	if(GetSaveFileName(&ofn))
	{
		EMUFILE* osRecordingMovie = FCEUD_UTF8_fstream(fname, "wb");
		currMovieData.dump(osRecordingMovie,false);
		delete osRecordingMovie;
		osRecordingMovie = 0;
	}
}

//used to track selection
void ItemRangeChanged(NMLVODSTATECHANGE* info)
{
	bool ON = !(info->uOldState & LVIS_SELECTED) && (info->uNewState & LVIS_SELECTED);
	bool OFF = (info->uOldState & LVIS_SELECTED) && !(info->uNewState & LVIS_SELECTED);

	if(ON)
		for(int i=info->iFrom;i<=info->iTo;i++)
			selectionFrames.insert(i);
	else
		for(int i=info->iFrom;i<=info->iTo;i++)
			selectionFrames.erase(i);
}
void ItemChanged(NMLISTVIEW* info)
{
	int item = info->iItem;
	
	bool ON = !(info->uOldState & LVIS_SELECTED) && (info->uNewState & LVIS_SELECTED);
	bool OFF = (info->uOldState & LVIS_SELECTED) && !(info->uNewState & LVIS_SELECTED);

	//if the item is -1, apply the change to all items
	if(item == -1)
	{
		if(OFF)
		{
			// clear all
			selectionFrames.clear();
		} else if (ON)
		{
			// select all
			for(int i = currMovieData.getNumRecords() - 1; i >= 0; i--)
			{
				selectionFrames.insert(i);
			}
		}
	}
	else
	{
		if(ON)
			selectionFrames.insert(item);
		else if(OFF) 
			selectionFrames.erase(item);
	}
}

BOOL CALLBACK WndprocTasEdit(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_PAINT:
			{
			}
			break;
		case WM_INITDIALOG:
			if (TasEdit_wndx==-32000) TasEdit_wndx=0; //Just in case
			if (TasEdit_wndy==-32000) TasEdit_wndy=0;
			SetWindowPos(hwndDlg,0,TasEdit_wndx,TasEdit_wndy,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOOWNERZORDER);
			// save references to dialog items
			hwndList = GetDlgItem(hwndDlg, IDC_LIST1);
			listItems = ListView_GetCountPerPage(hwndList);
			hwndHistoryList = GetDlgItem(hwndDlg, IDC_HISTORYLIST);
			hwndBookmarksList = GetDlgItem(hwndDlg, IDC_BOOKMARKSLIST);
			hwndBookmarks = GetDlgItem(hwndDlg, IDC_BOOKMARKS_BOX);
			hwndProgressbar = GetDlgItem(hwndDlg, IDC_PROGRESS1);
			SendMessage(hwndProgressbar, PBM_SETRANGE, 0, MAKELPARAM(0, PROGRESSBAR_WIDTH)); 
			hwndRewind = GetDlgItem(hwndDlg, TASEDIT_REWIND);
			hwndForward = GetDlgItem(hwndDlg, TASEDIT_FORWARD);
			hwndRB_RecOff = GetDlgItem(hwndDlg, IDC_RADIO1);
			hwndRB_RecAll = GetDlgItem(hwndDlg, IDC_RADIO2);
			hwndRB_Rec1P = GetDlgItem(hwndDlg, IDC_RADIO3);
			hwndRB_Rec2P = GetDlgItem(hwndDlg, IDC_RADIO4);
			hwndRB_Rec3P = GetDlgItem(hwndDlg, IDC_RADIO5);
			hwndRB_Rec4P = GetDlgItem(hwndDlg, IDC_RADIO6);
			break; 

		case WM_MOVE:
			{
				if (!IsIconic(hwndDlg))
				{
					RECT wrect;
					GetWindowRect(hwndDlg,&wrect);
					TasEdit_wndx = wrect.left;
					TasEdit_wndy = wrect.top;

					WindowBoundsCheckNoResize(TasEdit_wndx,TasEdit_wndy,wrect.right);
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
					SetWindowLong(hwndDlg, DWL_MSGRESULT, CustomDraw((NMLVCUSTOMDRAW*)lParam));
					return TRUE;
				case LVN_GETDISPINFO:
					GetDispInfo((NMLVDISPINFO*)lParam);
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
					ItemChanged((LPNMLISTVIEW) lParam);
					break;
				case LVN_ODSTATECHANGED:
					ItemRangeChanged((LPNMLVODSTATECHANGE) lParam);
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

		case WM_ACTIVATEAPP:
			if((BOOL)wParam)
				TASEdit_focus = true;
			else
				TASEdit_focus = false;
			return DefWindowProc(hwndDlg,uMsg,wParam,lParam);

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
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
			case ID_FILE_IMPORTFM2:
				Replay_LoadMovie(true);
				//Import(); //adelikat:  Putting the play movie dialog in its place until the import concept is refined.
				break;
			case ID_FILE_EXPORTFM2:
					Export();
				break;
			case ID_TASEDIT_FILE_CLOSE:
				ExitTasEdit();
				break;
			case ID_EDIT_SELECTALL:
				SelectAll();
				break;
			case ACCEL_CTRL_X:
			case ID_TASEDIT_CUT:
				Cut();
				break;
			case ACCEL_CTRL_C:
			case ID_TASEDIT_COPY:
				Copy();
				break;
			case ACCEL_CTRL_V:
			case ID_TASEDIT_PASTE:
				Paste();
				break;
			case ACCEL_CTRL_DELETE:
			case ID_TASEDIT_DELETE:
			case ID_CONTEXT_SELECTED_DELETEFRAMES:
				if (selectionFrames.size()) DeleteFrames();
				break;
			case ACCEL_CTRL_T:
			case ID_EDIT_TRUNCATE:
			case ID_CONTEXT_SELECTED_TRUNCATE:
			case ID_CONTEXT_STRAY_TRUNCATE:
				Truncate();
				break;
			case ID_HELP_TASEDITHELP:
				OpenHelpWindow(tasedithelp);
				//link to TASEdit in help menu
				break;
			case ACCEL_INS:
			case ID_EDIT_INSERT:
			case MENU_CONTEXT_STRAY_INSERTFRAMES:
			case ID_CONTEXT_SELECTED_INSERTFRAMES2:
				{
					int frames = selectionFrames.size();
					if(CWin32InputBox::GetInteger("Insert number of Frames", "How many frames?", frames, hwndDlg) == IDOK)
					{
						if (frames > 0)
						{
							if (selectionFrames.size())
							{
								// insert at selection
								int index = *selectionFrames.begin();
								currMovieData.insertEmpty(index,frames);
								greenzone.InvalidateGreenZone(history.RegisterInputChanges(MODTYPE_INSERT, index));
							} else
							{
								// insert at playback cursor
								currMovieData.insertEmpty(currFrameCounter,frames);
								greenzone.InvalidateGreenZone(history.RegisterInputChanges(MODTYPE_INSERT, currFrameCounter));
							}
						}
					}
				}
				break;
			case ACCEL_CTRL_INSERT:
			case ID_EDIT_INSERTFRAMES:
			case ID_CONTEXT_SELECTED_INSERTFRAMES:
				if (selectionFrames.size()) InsertFrames();
				break;
			case ACCEL_DEL:
			case ID_EDIT_CLEAR:
			case ID_CONTEXT_SELECTED_CLEARFRAMES:
				if (selectionFrames.size()) ClearFrames();
				break;
			case TASEDIT_REWIND_FULL:
				playback.RewindFull();
				break;
			case TASEDIT_PLAYSTOP:
				playback.ToggleEmulationPause();
				break;
			case TASEDIT_FORWARD_FULL:
				playback.ForwardFull();
				break;
			case ACCEL_CTRL_F:
			case CHECK_FOLLOW_CURSOR:
				//switch "Follow playback" flag
				TASEdit_follow_playback ^= 1;
				CheckDlgButton(hwndTasEdit, CHECK_FOLLOW_CURSOR, TASEdit_follow_playback?MF_CHECKED : MF_UNCHECKED);
				// if switched off then jump to selection
				if (TASEdit_follow_playback)
					FollowPlayback();
				else if (selectionFrames.size())
					ListView_EnsureVisible(hwndList,(int)*selectionFrames.begin(),FALSE);
				break;
			case ID_VIEW_SHOW_LAG_FRAMES:
				//switch "Highlight lag frames" flag
				TASEdit_show_lag_frames ^= 1;
				CheckMenuItem(hmenu, ID_VIEW_SHOW_LAG_FRAMES, TASEdit_show_lag_frames?MF_CHECKED : MF_UNCHECKED);
				RedrawList();
				break;
			case ID_VIEW_SHOW_MARKERS:
				//switch "Show Markers" flag
				TASEdit_show_markers ^= 1;
				CheckMenuItem(hmenu, ID_VIEW_SHOW_MARKERS, TASEdit_show_markers?MF_CHECKED : MF_UNCHECKED);
				RedrawList();
				break;
			case ID_VIEW_SHOWDOTINEMPTYCELLS:
				TASEdit_show_dot ^= 1;
				CheckMenuItem(hmenu, ID_VIEW_SHOWDOTINEMPTYCELLS, TASEdit_show_dot?MF_CHECKED : MF_UNCHECKED);
				RedrawList();
				break;
			case ID_VIEW_JUMPWHENMAKINGUNDO:
				TASEdit_jump_to_undo ^= 1;
				CheckMenuItem(hmenu, ID_VIEW_JUMPWHENMAKINGUNDO, TASEdit_jump_to_undo?MF_CHECKED : MF_UNCHECKED);
				break;
			case ACCEL_CTRL_P:
			case CHECK_AUTORESTORE_PLAYBACK:
				//switch "Auto-restore last playback position" flag
				TASEdit_restore_position ^= 1;
				CheckDlgButton(hwndTasEdit,CHECK_AUTORESTORE_PLAYBACK,TASEdit_restore_position?BST_CHECKED:BST_UNCHECKED);
				break;
			case ID_CONFIG_SETGREENZONECAPACITY:
				{
					//open input dialog
					int new_capacity = TASEdit_greenzone_capacity;
					if(CWin32InputBox::GetInteger("Greenzone capacity", "Keep savestates for how many frames?", new_capacity, hwndDlg) == IDOK)
					{
						if (new_capacity < GREENZONE_CAPACITY_MIN)
							new_capacity = GREENZONE_CAPACITY_MIN;
						else if (new_capacity > GREENZONE_CAPACITY_MAX)
							new_capacity = GREENZONE_CAPACITY_MAX;
						if (new_capacity < TASEdit_greenzone_capacity)
						{
							TASEdit_greenzone_capacity = new_capacity;
							greenzone.ClearGreenzoneTail();
							RedrawList();
						} else TASEdit_greenzone_capacity = new_capacity;
					}
					break;
				}
			case ID_CONFIG_SETMAXUNDOLEVELS:
				{
					//open input dialog
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
							history.init(TasEdit_undo_levels);
							// hot changes were cleared, so update list
							RedrawList();
							//RedrawUndoList();
						}
					}
					break;
				}
			case ID_CONFIG_MUTETURBO:
				muteTurbo ^= 1;
				CheckMenuItem(hmenu, ID_CONFIG_MUTETURBO, muteTurbo?MF_CHECKED : MF_UNCHECKED);
				break;
			case ID_CONFIG_BINDMARKERSTOINPUT:
				//switch "Bind Markers to Input" flag
				TASEdit_bind_markers ^= 1;
				CheckMenuItem(hmenu, ID_CONFIG_BINDMARKERSTOINPUT, TASEdit_bind_markers?MF_CHECKED : MF_UNCHECKED);
				break;
			case IDC_PROGRESS_BUTTON:
				// click on progressbar - stop seeking
				if (playback.pauseframe) playback.SeekingStop();
				break;
			case IDC_RADIO1:
				// switch to readonly, no need to recheck radiobuttons
				if (!movie_readonly) FCEUI_MovieToggleReadOnly();
				break;
			case IDC_RADIO2:
				// switch to read+write for all, no need to recheck radiobuttons
				if (movie_readonly) FCEUI_MovieToggleReadOnly();
				multitrack_recording_joypad = MULTITRACK_RECORDING_ALL;
				break;
			case IDC_RADIO3:
				// switch to read+write for 1P, no need to recheck radiobuttons
				if (movie_readonly) FCEUI_MovieToggleReadOnly();
				multitrack_recording_joypad = MULTITRACK_RECORDING_1P;
				break;
			case IDC_RADIO4:
				// switch to read+write for 2P, no need to recheck radiobuttons
				if (movie_readonly) FCEUI_MovieToggleReadOnly();
				multitrack_recording_joypad = MULTITRACK_RECORDING_2P;
				break;
			case IDC_RADIO5:
				// switch to read+write for 3P, no need to recheck radiobuttons
				if (movie_readonly) FCEUI_MovieToggleReadOnly();
				multitrack_recording_joypad = MULTITRACK_RECORDING_3P;
				break;
			case IDC_RADIO6:
				// switch to read+write for 4P, no need to recheck radiobuttons
				if (movie_readonly) FCEUI_MovieToggleReadOnly();
				multitrack_recording_joypad = MULTITRACK_RECORDING_4P;
				break;
			case ACCEL_CTRL_A:
			case ID_EDIT_SELECTMIDMARKERS:
			case ID_SELECTED_SELECTMIDMARKERS:
				SelectMidMarkers();
				break;
			case ACCEL_SHIFT_INS:
			case ID_EDIT_CLONEFRAMES:
			case ID_SELECTED_CLONE:
				if (selectionFrames.size()) CloneFrames();
				break;
			case ACCEL_CTRL_Z:
			case ID_EDIT_UNDO:
				{
					int result = history.undo();
					if (result >= 0)
					{
						UpdateList();
						FollowUndo();
						greenzone.InvalidateGreenZone(result);
					}
					break;
				}
			case ACCEL_CTRL_Y:
			case ID_EDIT_REDO:
				{
					int result = history.redo();
					if (result >= 0)
					{
						UpdateList();
						FollowUndo();
						greenzone.InvalidateGreenZone(result);
					}
					break;
				}

			}
			break;

		default:
			break;
	}
	return FALSE;
}

bool CheckItemVisible(int frame)
{
	int top = ListView_GetTopIndex(hwndList);
	// in fourscore there's horizontal scrollbar which takes one row for itself
	if (frame >= top && frame < top + listItems - (currMovieData.fourscore)?1:0)
		return true;
	return false;
}

void FollowPlayback()
{
	if (TASEdit_follow_playback) ListView_EnsureVisible(hwndList,currFrameCounter,FALSE);
}
void FollowUndo()
{
	int jump_frame = history.GetUndoHint();
	if (TASEdit_jump_to_undo && jump_frame >= 0)
	{
		if (!CheckItemVisible(jump_frame))
		{
			ListView_EnsureVisible(hwndList, currMovieData.getNumRecords()-1, true);
			ListView_EnsureVisible(hwndList, jump_frame, false);
		}
	}
}

void EnterTasEdit()
{
	if(!FCEU_IsValidUI(FCEUI_TASEDIT)) return;
	// window stuff
	if(!hwndTasEdit) hwndTasEdit = CreateDialog(fceu_hInstance,"TASEDIT",hAppWnd,WndprocTasEdit);
	if(hwndTasEdit)
	{
		// save "eoptions"
		saved_eoptions = eoptions;
		// clear "Run in background"
		eoptions &= ~EO_BGRUN;
		// set "Background TASEdit input"
		KeyboardSetBackgroundAccessBit(KEYBACKACCESS_TASEDIT);
		JoystickSetBackgroundAccessBit(JOYBACKACCESS_TASEDIT);
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
		// check option ticks
		CheckDlgButton(hwndTasEdit, CHECK_FOLLOW_CURSOR, TASEdit_follow_playback?MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(hmenu, ID_VIEW_SHOW_LAG_FRAMES, TASEdit_show_lag_frames?MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(hmenu, ID_VIEW_SHOW_MARKERS, TASEdit_show_markers?MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(hmenu, ID_VIEW_JUMPWHENMAKINGUNDO, TASEdit_jump_to_undo?MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(hmenu, ID_CONFIG_BINDMARKERSTOINPUT, TASEdit_bind_markers?MF_CHECKED : MF_UNCHECKED);
		CheckDlgButton(hwndTasEdit,CHECK_AUTORESTORE_PLAYBACK,TASEdit_restore_position?BST_CHECKED:BST_UNCHECKED);
		CheckMenuItem(hmenu, ID_CONFIG_MUTETURBO, muteTurbo?MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(hmenu, ID_VIEW_SHOWDOTINEMPTYCELLS, TASEdit_show_dot?MF_CHECKED : MF_UNCHECKED);

		SetWindowPos(hwndTasEdit,HWND_TOP,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE|SWP_NOOWNERZORDER);

		playback.init();
		greenzone.init();
		// either start new movie or use current movie
		if (movieMode == MOVIEMODE_INACTIVE)
		{
			FCEUI_StopMovie();
			CreateCleanMovie();
			playback.StartFromZero();
		}
		else
		{
			//use current movie to create a new project
			FCEUI_StopMovie();
		}
		// always start work from read-only mode, ready to switch to MULTITRACK_RECORDING_ALL
		movie_readonly = true;
		multitrack_recording_joypad = MULTITRACK_RECORDING_ALL;
		RecheckRecordingRadioButtons();
		movieMode = MOVIEMODE_TASEDIT;

		//prepare the main listview
		ListView_SetExtendedListViewStyleEx(hwndList,LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES,LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
		//subclass the header
		hwndHeader = ListView_GetHeader(hwndList);
		hwndHeader_oldWndproc = (WNDPROC)SetWindowLong(hwndHeader,GWL_WNDPROC,(LONG)HeaderWndProc);
		//subclass the whole listview
		hwndList_oldWndProc = (WNDPROC)SetWindowLong(hwndList,GWL_WNDPROC,(LONG)ListWndProc);
		//setup images for the listview
		HIMAGELIST himglist = ImageList_Create(8,12,ILC_COLOR8 | ILC_MASK,1,1);
		HBITMAP bmp = LoadBitmap(fceu_hInstance,MAKEINTRESOURCE(IDB_TE_ARROW));
		ImageList_AddMasked(himglist, bmp, 0xFF00FF);
		DeleteObject(bmp);
		ListView_SetImageList(hwndList,himglist,LVSIL_SMALL);
		//setup columns
		LVCOLUMN lvc;
		int colidx=0;
		// icons column
		lvc.mask = LVCF_WIDTH;
		lvc.cx = 13;
		ListView_InsertColumn(hwndList, colidx++, &lvc);
		// frame number column
		lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT;
		lvc.fmt = LVCFMT_CENTER;
		lvc.cx = 75;
		lvc.pszText = "Frame#";
		ListView_InsertColumn(hwndList, colidx++, &lvc);
		// pads columns
		lvc.cx = 21;
		// add pads 1 and 2
		for (int joy = 0; joy < 2; ++joy)
		{
			for (int btn = 0; btn < NUM_JOYPAD_BUTTONS; ++btn)
			{
				lvc.pszText = buttonNames[btn];
				ListView_InsertColumn(hwndList, colidx++, &lvc);
			}
		}
		// add pads 3 and 4 and frame_number2
		if (currMovieData.fourscore) AddFourscore();
		UpdateList();

		//prepare the history listview
		ListView_SetExtendedListViewStyleEx(hwndHistoryList,LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES,LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
		//subclass the whole listview
		hwndHistoryList_oldWndProc = (WNDPROC)SetWindowLong(hwndHistoryList,GWL_WNDPROC,(LONG)HistoryListWndProc);
		lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT;
		lvc.cx = 200;
		lvc.fmt = LVCFMT_LEFT;
		ListView_InsertColumn(hwndHistoryList, 0, &lvc);

		// init variables
		project.init();
		history.init(TasEdit_undo_levels);
		SetFocus(hwndHistoryList);
		greenzone.TryDumpIncremental(lagFlag != 0);
		FCEU_DispMessage("Tasedit engaged",0);
	}
}

bool ExitTasEdit()
{
	if (!AskSaveProject()) return false;

	DestroyWindow(hwndTasEdit);
	hwndTasEdit = 0;
	TASEdit_focus = false;
	// restore "eoptions"
	eoptions = saved_eoptions;
	// restore autosaves
	EnableAutosave = saved_EnableAutosave;

	DoPriority();
	UpdateCheckedMenuItems();
	// clear "Background TASEdit input"
	KeyboardClearBackgroundAccessBit(KEYBACKACCESS_TASEDIT);
	JoystickClearBackgroundAccessBit(JOYBACKACCESS_TASEDIT);
	// release memory
	greenzone.clearGreenzone();
	history.free();

	movieMode = MOVIEMODE_INACTIVE;
	FCEU_DispMessage("Tasedit disengaged",0);
	CreateCleanMovie();
	return true;
}

