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

using namespace std;

HWND hwndTasEdit = 0;
HMENU hmenu, hrmenu;

bool TASEdit_focus = false;
bool Tasedit_rewind_now = false;

// all Taseditor functional modules
TASEDIT_PROJECT project;
INPUT_HISTORY history;
PLAYBACK playback;
RECORDER recorder;
GREENZONE greenzone;
MARKERS markers;
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
bool TASEdit_bind_markers = true;
bool TASEdit_use_1p_rec = true;
bool TASEdit_combine_consecutive_rec = true;
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

// resources
string tasedithelp = "{16CDE0C4-02B0-4A60-A88D-076319909A4D}"; //Name of TASEdit Help page
char buttonNames[NUM_JOYPAD_BUTTONS][2] = {"A", "B", "S", "T", "U", "D", "L", "R"};
char windowCaptioBase[] = "TAS Editor";
extern char recordingCaptions[5][30];

// enterframe function
void UpdateTasEdit()
{
	if(!hwndTasEdit) return;

	tasedit_list.update();
	markers.update();
	greenzone.update();
	playback.update();
	recorder.update();
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
		strcat(new_caption, recordingCaptions[recorder.multitrack_recording_joypad]);
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

void ShowMenu(ECONTEXTMENU which, POINT& pt)
{
	HMENU sub = GetSubMenu(hrmenu,(int)which);
	TrackPopupMenu(sub,0,pt.x,pt.y,TPM_RIGHTBUTTON,hwndTasEdit,0);
}

void StrayClickMenu(LPNMITEMACTIVATE info)
{
	POINT pt = info->ptAction;
	ClientToScreen(tasedit_list.hwndList, &pt);
	ShowMenu(CONTEXTMENU_STRAY, pt);
}

void RightClickMenu(LPNMITEMACTIVATE info)
{
	POINT pt = info->ptAction;
	ClientToScreen(tasedit_list.hwndList, &pt);
	ShowMenu(CONTEXTMENU_SELECTED, pt);
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

	if (selection.CheckFrameSelected(index))
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
			markers.ToggleMarker(row_index);
			if (markers.GetMarker(row_index))
				history.RegisterChanges(MODTYPE_MARKER_SET, row_index);
			else
				history.RegisterChanges(MODTYPE_MARKER_UNSET, row_index);
			project.SetProjectChanged();
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
				markers.insertEmpty(*it, frames);
			frames = 1;
		} else frames++;
	}
	markers.update();
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
				markers.insertEmpty(*it,frames);
			frames = 1;
		} else frames++;
	}
	markers.update();
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
				markers.insertEmpty(index, frames);
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
			markers.EraseMarker(*it);
	}
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
			markers.truncateAt(frame+1);
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

//the column set operation, for setting a button/Marker for a span of selected values
void ColumnSet(int column)
{
	SelectionFrames* current_selection = selection.MakeStrobe();
	if (current_selection->size() == 0) return;

	SelectionFrames::iterator current_selection_begin(current_selection->begin());
	SelectionFrames::iterator current_selection_end(current_selection->end());
	if (column == COLUMN_FRAMENUM || column == COLUMN_FRAMENUM2)
	{
		// Markers column
		// inspect the selected frames, if they are all set, then unset all, else set all
		bool unset_found = false;
		for(SelectionFrames::iterator it(current_selection_begin); it != current_selection_end; it++)
		{
			if(!markers.GetMarker(*it))
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
				markers.SetMarker(*it);
				tasedit_list.RedrawRow(*it);
			}
			history.RegisterChanges(MODTYPE_MARKER_SET, *current_selection_begin, *current_selection->rbegin());
		} else
		{
			// unset all
			for(SelectionFrames::iterator it(current_selection_begin); it != current_selection_end; it++)
			{
				markers.ClearMarker(*it);
				tasedit_list.RedrawRow(*it);
			}
			history.RegisterChanges(MODTYPE_MARKER_UNSET, *current_selection_begin, *current_selection->rbegin());
		}
		project.SetProjectChanged();
		// no need to RedrawList();
	} else
	{
		// buttons column
		int joy = (column - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS;
		if (joy < 0 || joy >= NUM_JOYPADS) return;
		int button = (column - COLUMN_JOYPAD1_A) % NUM_JOYPAD_BUTTONS;
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
	}
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

		// TAS recording info starts with "TAS ".
		if (pGlobal[0]=='T' && pGlobal[1]=='A' && pGlobal[2]=='S')
		{
			// Extract number of frames
			int range;
			sscanf (pGlobal+3, "%d", &range);
			if (currMovieData.getNumRecords() < pos+range)
			{
				currMovieData.insertEmpty(currMovieData.getNumRecords(),pos+range-currMovieData.getNumRecords());
				markers.update();
			}

			pGlobal = strchr(pGlobal, '\n');
			int joy = 0;
			uint8 new_buttons = 0;
			char* frame;

			--pos;
			while (pGlobal++ && *pGlobal!='\0')
			{
				frame = pGlobal;
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
							currMovieData.records[pos].joysticks[joy] |= new_buttons;
						else
							currMovieData.records[pos].joysticks[joy] = new_buttons;
						++joy;
						new_buttons = 0;
						break;
					default:
						for (int bit=0; bit<NUM_JOYPAD_BUTTONS; ++bit)
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
					currMovieData.records[pos].joysticks[joy] |= new_buttons;
				else
					currMovieData.records[pos].joysticks[joy] = new_buttons;

				// find CRLF
				pGlobal = strchr(pGlobal, '\n');
			}

			greenzone.InvalidateAndCheck(history.RegisterChanges(MODTYPE_PASTE, *current_selection_begin));
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

		// TAS recording info starts with "TAS ".
		if (pGlobal[0]=='T' && pGlobal[1]=='A' && pGlobal[2]=='S')
		{
			// make sure markers have the same size as movie
			markers.update();
			// init inserted_set (for input history hot changes)
			selection.GetInsertedSet().clear();

			// Extract number of frames
			int range;
			sscanf (pGlobal+3, "%d", &range);


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
					if (currMovieData.getNumRecords() < pos)
					{
						currMovieData.insertEmpty(currMovieData.getNumRecords(), pos - currMovieData.getNumRecords());
						markers.update();
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
				if (TASEdit_bind_markers) markers.insertEmpty(pos, 1);
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
								break;
							}
						}
						break;
					}
					++frame;
				}

				pGlobal = strchr(pGlobal, '\n');
			}
			markers.update();
			greenzone.InvalidateAndCheck(history.RegisterChanges(MODTYPE_PASTEINSERT, *current_selection_begin));
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

	const char TPfilter[]="TASEdit Project (*.tas)\0*.tas\0\0";	

	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hwndOwner = hwndTasEdit;
	ofn.hInstance=fceu_hInstance;
	ofn.lpstrTitle="Open TASEdit Project...";
	ofn.lpstrFilter=TPfilter;

	char nameo[2048];
	strcpy(nameo, mass_replace(GetRomName(),"|",".").c_str());	//convert | to . for archive filenames

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
		// remember to update fourscore status
		bool last_fourscore = currMovieData.fourscore;
		// Load project
		project.LoadProject(project.GetProjectFile());
		// update fourscore status
		if (last_fourscore && !currMovieData.fourscore)
			tasedit_list.RemoveFourscore();
		else if (!last_fourscore && currMovieData.fourscore)
			tasedit_list.AddFourscore();
		RedrawTasedit();
		RedrawWindowCaption();
	}
}

// Saves current project
bool SaveProjectAs()
{
	const char TPfilter[]="TASEdit Project (*.tas)\0*.tas\0All Files (*.*)\0*.*\0\0";	//Filetype filter

	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hwndOwner = hwndTasEdit;
	ofn.hInstance=fceu_hInstance;
	ofn.lpstrTitle="Save TASEdit Project As...";
	ofn.lpstrFilter=TPfilter;

	char nameo[2048];
	strcpy(nameo, mass_replace(GetRomName(),"|",".").c_str());	//convert | to . for archive filenames
	ofn.lpstrFile = nameo;
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

void Import()
{


}
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
	ofn.hwndOwner = hwndTasEdit;
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

BOOL CALLBACK WndprocTasEdit(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_PAINT:
			break;
		case WM_INITDIALOG:
			if (TasEdit_wndx==-32000) TasEdit_wndx=0; //Just in case
			if (TasEdit_wndy==-32000) TasEdit_wndy=0;
			SetWindowPos(hwndDlg, 0, TasEdit_wndx, TasEdit_wndy, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
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
				GotFocus();
			else
				LostFocus();
			break;
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
				Import();
				break;
			case ID_FILE_EXPORTFM2:
					Export();
				break;
			case ID_TASEDIT_FILE_CLOSE:
				ExitTasEdit();
				break;
			case ID_EDIT_SELECTALL:
				selection.SelectAll();
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
			case ACCEL_SHIFT_V:
			case ID_EDIT_PASTEINSERT:
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
				//link to TASEdit in help menu
				break;
			case ACCEL_INS:
			case ID_EDIT_INSERT:
			case MENU_CONTEXT_STRAY_INSERTFRAMES:
			case ID_CONTEXT_SELECTED_INSERTFRAMES2:
				InsertNumFrames();
				break;
			case ACCEL_CTRL_INSERT:
			case ID_EDIT_INSERTFRAMES:
			case ID_CONTEXT_SELECTED_INSERTFRAMES:
				InsertFrames();
				break;
			case ACCEL_DEL:
			case ID_EDIT_CLEAR:
			case ID_CONTEXT_SELECTED_CLEARFRAMES:
				ClearFrames();
				break;
			case TASEDIT_PLAYSTOP:
				playback.ToggleEmulationPause();
				break;
			case ACCEL_CTRL_F:
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
				//switch "Highlight lag frames" flag
				TASEdit_show_lag_frames ^= 1;
				CheckMenuItem(hmenu, ID_VIEW_SHOW_LAG_FRAMES, TASEdit_show_lag_frames?MF_CHECKED : MF_UNCHECKED);
				tasedit_list.RedrawList();
				bookmarks.RedrawBookmarksList();
				break;
			case ID_VIEW_SHOW_MARKERS:
				//switch "Show Markers" flag
				TASEdit_show_markers ^= 1;
				CheckMenuItem(hmenu, ID_VIEW_SHOW_MARKERS, TASEdit_show_markers?MF_CHECKED : MF_UNCHECKED);
				tasedit_list.RedrawList();		// no need to redraw Bookmarks, as Markers are only shown in main list
				break;
			case ID_VIEW_SHOWBRANCHSCREENSHOTS:
				//switch "Show Branch Screenshots" flag
				TASEdit_show_branch_screenshots ^= 1;
				CheckMenuItem(hmenu, ID_VIEW_SHOWBRANCHSCREENSHOTS, TASEdit_show_branch_screenshots?MF_CHECKED : MF_UNCHECKED);
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
							history.init(TasEdit_undo_levels);
							selection.init(TasEdit_undo_levels);
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
				//switch "Bind Markers to Input" flag
				TASEdit_bind_markers ^= 1;
				CheckMenuItem(hmenu, ID_CONFIG_BINDMARKERSTOINPUT, TASEdit_bind_markers?MF_CHECKED : MF_UNCHECKED);
				break;
			case ID_CONFIG_USE1PFORRECORDING:
				//switch "Use 1P keys for single Recordings" flag
				TASEdit_use_1p_rec ^= 1;
				CheckMenuItem(hmenu, ID_CONFIG_USE1PFORRECORDING, TASEdit_use_1p_rec?MF_CHECKED : MF_UNCHECKED);
				break;
			case ID_CONFIG_COMBINECONSECUTIVERECORDINGS:
				//switch "Combine consecutive Recordings" flag
				TASEdit_combine_consecutive_rec ^= 1;
				CheckMenuItem(hmenu, ID_CONFIG_COMBINECONSECUTIVERECORDINGS, TASEdit_combine_consecutive_rec?MF_CHECKED : MF_UNCHECKED);
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
			case IDC_RADIO1:
				// switch to readonly, no need to recheck radiobuttons
				if (!movie_readonly) FCEUI_MovieToggleReadOnly();
				break;
			case IDC_RADIO2:
				// switch to read+write for all, no need to recheck radiobuttons
				if (movie_readonly) FCEUI_MovieToggleReadOnly();
				recorder.multitrack_recording_joypad = MULTITRACK_RECORDING_ALL;
				break;
			case IDC_RADIO3:
				// switch to read+write for 1P, no need to recheck radiobuttons
				if (movie_readonly) FCEUI_MovieToggleReadOnly();
				recorder.multitrack_recording_joypad = MULTITRACK_RECORDING_1P;
				break;
			case IDC_RADIO4:
				// switch to read+write for 2P, no need to recheck radiobuttons
				if (movie_readonly) FCEUI_MovieToggleReadOnly();
				recorder.multitrack_recording_joypad = MULTITRACK_RECORDING_2P;
				break;
			case IDC_RADIO5:
				// switch to read+write for 3P, no need to recheck radiobuttons
				if (movie_readonly) FCEUI_MovieToggleReadOnly();
				recorder.multitrack_recording_joypad = MULTITRACK_RECORDING_3P;
				break;
			case IDC_RADIO6:
				// switch to read+write for 4P, no need to recheck radiobuttons
				if (movie_readonly) FCEUI_MovieToggleReadOnly();
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
			case ID_EDIT_SELECTMIDMARKERS:
			case ID_SELECTED_SELECTMIDMARKERS:
				selection.SelectMidMarkers();
				break;
			case ACCEL_SHIFT_INS:
			case ID_EDIT_CLONEFRAMES:
			case ID_SELECTED_CLONE:
				CloneFrames();
				break;
			case ACCEL_CTRL_Z:
			case ID_EDIT_UNDO:
				{
					int result = history.undo();
					if (result >= 0)
					{
						tasedit_list.update();
						tasedit_list.FollowUndo();
						greenzone.InvalidateAndCheck(result);
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

			}
			break;
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

void EnterTasEdit()
{
	if(!FCEU_IsValidUI(FCEUI_TASEDIT)) return;
	// window stuff
	if(!hwndTasEdit) hwndTasEdit = CreateDialog(fceu_hInstance,"TASEDIT",hAppWnd,WndprocTasEdit);
	if(hwndTasEdit)
	{
		// save "eoptions"
		saved_eoptions = eoptions;
		// set "Run in background"
		eoptions |= EO_BGRUN;
		GotFocus();
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
		CheckDlgButton(hwndTasEdit, CHECK_TURBO_SEEK, TASEdit_turbo_seek?MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(hmenu, ID_VIEW_SHOW_LAG_FRAMES, TASEdit_show_lag_frames?MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(hmenu, ID_VIEW_SHOW_MARKERS, TASEdit_show_markers?MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(hmenu, ID_VIEW_SHOWBRANCHSCREENSHOTS, TASEdit_show_branch_screenshots?MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(hmenu, ID_VIEW_JUMPWHENMAKINGUNDO, TASEdit_jump_to_undo?MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(hmenu, ID_VIEW_ENABLEHOTCHANGES, TASEdit_enable_hot_changes?MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(hmenu, ID_CONFIG_BRANCHESRESTOREFULLMOVIE, TASEdit_branch_full_movie?MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(hmenu, ID_CONFIG_BRANCHESWORKONLYWHENRECORDING, TASEdit_branch_only_when_rec?MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(hmenu, ID_CONFIG_HUDINBRANCHSCREENSHOTS, TASEdit_branch_scr_hud?MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(hmenu, ID_CONFIG_BINDMARKERSTOINPUT, TASEdit_bind_markers?MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(hmenu, ID_CONFIG_USE1PFORRECORDING, TASEdit_use_1p_rec?MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(hmenu, ID_CONFIG_COMBINECONSECUTIVERECORDINGS, TASEdit_combine_consecutive_rec?MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(hmenu, ID_CONFIG_SUPERIMPOSE_AFFECTS_PASTE, TASEdit_superimpose_affects_paste?MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(hmenu, ID_CONFIG_MUTETURBO, muteTurbo?MF_CHECKED : MF_UNCHECKED);
		CheckDlgButton(hwndTasEdit,CHECK_AUTORESTORE_PLAYBACK,TASEdit_restore_position?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hwndTasEdit, IDC_SUPERIMPOSE, TASEdit_superimpose);

		SetWindowPos(hwndTasEdit, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE|SWP_NOOWNERZORDER);
		// init modules
		FCEU_printf("1");
		greenzone.init();
		FCEU_printf("2");
		playback.init();
		// either start new movie or use current movie
		if (currMovieData.savestate.size() != 0)
		{
			FCEUD_PrintError("This version of TAS Editor doesn't work with movies starting from savestate.");
			// delete savestate, but preserve input
			currMovieData.savestate.clear();
		}
		if (FCEUMOV_Mode(MOVIEMODE_INACTIVE))
		{
			FCEUI_StopMovie();
			CreateCleanMovie();
			playback.StartFromZero();
		} else
		{
			// use current movie to create a new project
			FCEUI_StopMovie();
			greenzone.TryDumpIncremental(lagFlag != 0);
		}
		// switch to tasedit mode
		movieMode = MOVIEMODE_TASEDIT;
		// init variables
		recorder.init();
		tasedit_list.init();
		markers.init();
		project.init();
		bookmarks.init();
		screenshot_display.init();
		history.init(TasEdit_undo_levels);
		selection.init(TasEdit_undo_levels);
		SetFocus(history.hwndHistoryList);		// set focus only once, to show selection cursor
		SetFocus(tasedit_list.hwndList);
		FCEU_DispMessage("Tasedit engaged", 0);
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
	tasedit_list.free();
	markers.free();
	greenzone.clearGreenzone();
	bookmarks.free();
	screenshot_display.free();
	history.free();
	playback.SeekingStop();
	selection.free();
	// switch off tasedit mode
	movieMode = MOVIEMODE_INACTIVE;
	FCEU_DispMessage("Tasedit disengaged", 0);
	CreateCleanMovie();
	return true;
}

void GotFocus()
{
	TASEdit_focus = true;
	// set "Background TASEdit input"
	KeyboardSetBackgroundAccessBit(KEYBACKACCESS_TASEDIT);
	JoystickSetBackgroundAccessBit(JOYBACKACCESS_TASEDIT);
}
void LostFocus()
{
	TASEdit_focus = false;
	// clear "Background TASEdit input"
	KeyboardClearBackgroundAccessBit(KEYBACKACCESS_TASEDIT);
	JoystickClearBackgroundAccessBit(JOYBACKACCESS_TASEDIT);
}

