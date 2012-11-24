/* ---------------------------------------------------------------------------------
Implementation file of SPLICER class
Copyright (c) 2011-2012 AnS

(The MIT License)
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------------
Splicer - Tool for montage
[Singleton]

* implements operations of mass-changing Input: copy/paste, cloning, clearing region, insertion and deletion of frames, truncating
* stores data about the Selection used in last "Copy to Clipboard" operation
* regularly checks the state of current Selection and displays info on GUI, also displays info about Input in Clipboard
* when launching TAS Editor, it checks Clipboard contents
* stores resources: mnemonics of buttons, texts for selection/clipboard info on GUI
------------------------------------------------------------------------------------ */

#include <sstream>
#include "taseditor_project.h"
#include "../Win32InputBox.h"

extern TASEDITOR_WINDOW taseditor_window;
extern TASEDITOR_CONFIG taseditor_config;
extern HISTORY history;
extern MARKERS_MANAGER markers_manager;
extern PLAYBACK playback;
extern GREENZONE greenzone;
extern PIANO_ROLL piano_roll;
extern SELECTION selection;

extern int joysticks_per_frame[NUM_SUPPORTED_INPUT_TYPES];
extern int GetInputType(MovieData& md);

// resources
char buttonNames[NUM_JOYPAD_BUTTONS][2] = {"A", "B", "S", "T", "U", "D", "L", "R"};
char selectionText[] = "Selection: ";
char selectionEmptyText[] = "Selection: no";
char numTextRow[] = "1 row, ";
char numTextRows[] = " rows, ";
char numTextColumn[] = "1 column";
char numTextColumns[] = " columns";
char clipboardText[] = "Clipboard: ";
char clipboardEmptyText[] = "Clipboard: empty";

SPLICER::SPLICER()
{
}

void SPLICER::init()
{
	hwndTextSelection = GetDlgItem(taseditor_window.hwndTasEditor, IDC_TEXT_SELECTION);
	hwndTextClipboard = GetDlgItem(taseditor_window.hwndTasEditor, IDC_TEXT_CLIPBOARD);

	reset();
	if (clipboard_selection.empty())
		CheckClipboard();
	RedrawTextClipboard();
}
void SPLICER::reset()
{
	must_redraw_selection_text = true;
}
void SPLICER::update()
{
	// redraw Selection info text of needed
	if (must_redraw_selection_text)
	{
		int size = selection.GetCurrentSelectionSize();
		if (size)
		{
			char new_text[100];
			strcpy(new_text, selectionText);
			char num[11];
			// rows
			if (size > 1)
			{
				_itoa(size, num, 10);
				strcat(new_text, num);
				strcat(new_text, numTextRows);
			} else
			{
				strcat(new_text, numTextRow);
			}
			// columns
			int columns = NUM_JOYPAD_BUTTONS * joysticks_per_frame[GetInputType(currMovieData)];	// in future the number of columns will depend on selected columns
			if (columns > 1)
			{
				_itoa(columns, num, 10);
				strcat(new_text, num);
				strcat(new_text, numTextColumns);
			} else
			{
				strcat(new_text, numTextColumn);
			}
			SetWindowText(hwndTextSelection, new_text);
		} else
		{
			SetWindowText(hwndTextSelection, selectionEmptyText);
		}
		must_redraw_selection_text = false;
	}
}
// ----------------------------------------------------------------------------------------------
void SPLICER::CloneFrames()
{
	SelectionFrames* current_selection = selection.MakeStrobe();
	int frames = current_selection->size();
	if (!frames) return;

	selection.ClearSelection();			// Selection will be moved down, so that same frames are selected
	bool markers_changed = false;
	currMovieData.records.reserve(currMovieData.getNumRecords() + frames);
	// insert frames before each selection, but consecutive Selection lines are accounted as single region
	SelectionFrames::reverse_iterator next_it;
	SelectionFrames::reverse_iterator current_selection_rend = current_selection->rend();
	int shift = frames;
	frames = 1;
	for(SelectionFrames::reverse_iterator it(current_selection->rbegin()); it != current_selection_rend; it++)
	{
		next_it = it;
		next_it++;
		if (next_it == current_selection_rend || (int)*next_it < ((int)*it - 1))
		{
			// end of current region
			currMovieData.cloneRegion(*it, frames);
			greenzone.laglog.InsertFrame(*it, false, frames);
			if (taseditor_config.bind_markers)
			{
				// Markers are not cloned
				if (markers_manager.insertEmpty(*it,frames))
					markers_changed = true;
			}
			selection.SetRegionSelection((*it) + shift, (*it) + shift + frames);
			shift -= frames;
			// start accumulating next region
			frames = 1;
		} else frames++;
	}
	// check and register changes
	int first_changes = history.RegisterChanges(MODTYPE_CLONE, *current_selection->begin(), -1, 0, NULL, 0, current_selection);
	if (first_changes >= 0)
	{
		greenzone.InvalidateAndCheck(first_changes);
	} else if (markers_changed)
	{
		history.RegisterMarkersChange(MODTYPE_MARKER_SHIFT, *current_selection->begin());
		piano_roll.RedrawList();
	}
	if (markers_changed)
		selection.must_find_current_marker = playback.must_find_current_marker = true;
}

void SPLICER::InsertFrames()
{
	SelectionFrames* current_selection = selection.MakeStrobe();
	int frames = current_selection->size();
	if (!frames) return;

	selection.ClearSelection();			// Selection will be moved down, so that same frames are selected
	bool markers_changed = false;
	currMovieData.records.reserve(currMovieData.getNumRecords() + frames);
	// insert frames before each selection, but consecutive Selection lines are accounted as single region
	SelectionFrames::reverse_iterator next_it;
	SelectionFrames::reverse_iterator current_selection_rend = current_selection->rend();
	int shift = frames;
	frames = 1;
	for(SelectionFrames::reverse_iterator it(current_selection->rbegin()); it != current_selection_rend; it++)
	{
		next_it = it;
		next_it++;
		if (next_it == current_selection_rend || (int)*next_it < ((int)*it - 1))
		{
			// end of current region
			currMovieData.insertEmpty(*it, frames);
			greenzone.laglog.InsertFrame(*it, false, frames);
			if (taseditor_config.bind_markers)
			{
				if (markers_manager.insertEmpty(*it, frames))
					markers_changed = true;
			}
			selection.SetRegionSelection((*it) + shift, (*it) + shift + frames);
			shift -= frames;
			// start accumulating next region
			frames = 1;
		} else frames++;
	}
	// check and register changes
	int first_changes = history.RegisterChanges(MODTYPE_INSERT, *current_selection->begin(), -1, 0, NULL, 0, current_selection);
	if (first_changes >= 0)
	{
		greenzone.InvalidateAndCheck(first_changes);
	} else if (markers_changed)
	{
		history.RegisterMarkersChange(MODTYPE_MARKER_SHIFT, *current_selection->begin());
		piano_roll.RedrawList();
	}
	if (markers_changed)
		selection.must_find_current_marker = playback.must_find_current_marker = true;
}

void SPLICER::InsertNumFrames()
{
	SelectionFrames* current_selection = selection.MakeStrobe();
	int frames = current_selection->size();
	if (CWin32InputBox::GetInteger("Insert number of Frames", "How many frames?", frames, taseditor_window.hwndTasEditor) == IDOK)
	{
		if (frames > 0)
		{
			bool markers_changed = false;
			int index;
			if (current_selection->size())
			{
				// insert at selection
				index = *current_selection->begin();
			} else
			{
				// insert at Playback cursor
				index = currFrameCounter;
			}
			currMovieData.insertEmpty(index, frames);
			greenzone.laglog.InsertFrame(index, false, frames);
			if (taseditor_config.bind_markers)
			{
				if (markers_manager.insertEmpty(index, frames))
					markers_changed = true;
			}
			if (current_selection->size())
			{
				// shift Selection down, so that same frames are selected
				piano_roll.UpdateItemCount();
				selection.ClearSelection();
				SelectionFrames::iterator current_selection_end = current_selection->end();
				for(SelectionFrames::iterator it(current_selection->begin()); it != current_selection_end; it++)
					selection.SetRowSelection((*it) + frames);
			}
			// check and register changes
			int first_changes = history.RegisterChanges(MODTYPE_INSERTNUM, index, -1, frames);
			if (first_changes >= 0)
			{
				greenzone.InvalidateAndCheck(first_changes);
			} else if (markers_changed)
			{
				history.RegisterMarkersChange(MODTYPE_MARKER_SHIFT, index);
				piano_roll.RedrawList();
			}
			if (markers_changed)
				selection.must_find_current_marker = playback.must_find_current_marker = true;
		}
	}
}

void SPLICER::DeleteFrames()
{
	SelectionFrames* current_selection = selection.MakeStrobe();
	if (current_selection->size() == 0) return;

	bool markers_changed = false;
	int start_index = *current_selection->begin();
	int end_index = *current_selection->rbegin();
	SelectionFrames::reverse_iterator current_selection_rend = current_selection->rend();
	// delete frames on each selection, going backwards
	for(SelectionFrames::reverse_iterator it(current_selection->rbegin()); it != current_selection_rend; it++)
	{
		currMovieData.eraseRecords(*it);
		greenzone.laglog.EraseFrame(*it);
		if (taseditor_config.bind_markers)
		{
			if (markers_manager.EraseMarker(*it))
				markers_changed = true;
		}
	}
	// check if user deleted all frames
	if (!currMovieData.getNumRecords())
		playback.StartFromZero();
	// reduce Piano Roll
	piano_roll.UpdateItemCount();
	// check and register changes
	int result = history.RegisterChanges(MODTYPE_DELETE, start_index, -1, 0, NULL, 0, current_selection);
	if (result >= 0)
	{
		greenzone.InvalidateAndCheck(result);
	} else
	{
		// check for special case: user deleted a bunch of empty frames the end of the movie
		greenzone.InvalidateAndCheck(currMovieData.getNumRecords() - 1);
		if (markers_changed)
			history.RegisterMarkersChange(MODTYPE_MARKER_SHIFT, start_index);
	}
	if (markers_changed)
		selection.must_find_current_marker = playback.must_find_current_marker = true;
}

void SPLICER::ClearFrames(SelectionFrames* current_selection)
{
	bool cut = true;
	if (!current_selection)
	{
		cut = false;
		current_selection = selection.MakeStrobe();
		if (current_selection->size() == 0) return;
	}

	// clear Input on each selected frame
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

void SPLICER::Truncate()
{
	int frame = selection.GetCurrentSelectionBeginning();
	if (frame < 0) frame = currFrameCounter;

	if (currMovieData.getNumRecords() > frame+1)
	{
		int last_frame_was = currMovieData.getNumRecords() - 1;
		currMovieData.truncateAt(frame+1);
		bool markers_changed = false;
		if (taseditor_config.bind_markers)
		{
			if (markers_manager.SetMarkersSize(frame+1))
			{
				markers_changed = true;
				selection.must_find_current_marker = playback.must_find_current_marker = true;
			}
		}
		piano_roll.UpdateItemCount();
		int result = history.RegisterChanges(MODTYPE_TRUNCATE, frame + 1);
		if (result >= 0)
		{
			greenzone.InvalidateAndCheck(result);
		} else
		{
			// check for special case: user truncated empty frames of the movie
			greenzone.InvalidateAndCheck(currMovieData.getNumRecords() - 1);
			if (markers_changed)
				history.RegisterMarkersChange(MODTYPE_MARKER_REMOVE, frame+1, last_frame_was);
		}
	}
}

bool SPLICER::Copy(SelectionFrames* current_selection)
{
	if (!current_selection)
	{
		current_selection = selection.MakeStrobe();
		if (current_selection->size() == 0) return false;
	}

	SelectionFrames::iterator current_selection_begin(current_selection->begin());
	SelectionFrames::iterator current_selection_end(current_selection->end());
	int num_joypads = joysticks_per_frame[GetInputType(currMovieData)];
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
			for (int joy = 0; joy < num_joypads; ++joy)
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
		}
		// write data to clipboard
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
	catch (std::bad_alloc e)
	{
		return false;
	}
	// copied successfully
	// memorize currently strobed Selection data to clipboard_selection
	clipboard_selection = *current_selection;
	RedrawTextClipboard();
	return true;
}
void SPLICER::Cut()
{
	SelectionFrames* current_selection = selection.MakeStrobe();
	if (current_selection->size() == 0) return;

	if (Copy(current_selection))
	{
		ClearFrames(current_selection);
	}
}
bool SPLICER::Paste()
{
	SelectionFrames* current_selection = selection.MakeStrobe();
	if (current_selection->size() == 0) return false;

	if (!OpenClipboard(taseditor_window.hwndTasEditor)) return false;

	int num_joypads = joysticks_per_frame[GetInputType(currMovieData)];
	bool result = false;
	int pos = *(current_selection->begin());
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
				markers_manager.update();
			}

			pGlobal = strchr(pGlobal, '\n');
			int joy = 0;
			uint8 new_buttons = 0;
			std::vector<uint8> flash_joy(num_joypads);
			char* frame;
			pos--;
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
					pos++;
				}
				
				if (taseditor_config.superimpose == SUPERIMPOSE_UNCHECKED)
				{
					currMovieData.records[pos].joysticks[0] = 0;
					currMovieData.records[pos].joysticks[1] = 0;
					currMovieData.records[pos].joysticks[2] = 0;
					currMovieData.records[pos].joysticks[3] = 0;
				}
				// read this frame Input
				joy = 0;
				new_buttons = 0;
				while (*frame && *frame != '\n' && *frame !='\r')
				{
					switch (*frame)
					{
					case '|': // Joystick mark
						// flush buttons to movie data
						if (taseditor_config.superimpose == SUPERIMPOSE_CHECKED || (taseditor_config.superimpose == SUPERIMPOSE_INDETERMINATE && new_buttons == 0))
						{
							flash_joy[joy] |= (new_buttons & (~currMovieData.records[pos].joysticks[joy]));		// highlight buttons that are new
							currMovieData.records[pos].joysticks[joy] |= new_buttons;
						} else
						{
							flash_joy[joy] |= new_buttons;		// highlight buttons that were added
							currMovieData.records[pos].joysticks[joy] = new_buttons;
						}
						joy++;
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
					frame++;
				}
				// before going to next frame, flush buttons to movie data
				if (taseditor_config.superimpose == SUPERIMPOSE_CHECKED || (taseditor_config.superimpose == SUPERIMPOSE_INDETERMINATE && new_buttons == 0))
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

			greenzone.InvalidateAndCheck(history.RegisterChanges(MODTYPE_PASTE, *(current_selection->begin()), pos));
			// flash Piano Roll header columns that were changed during paste
			for (int joy = 0; joy < num_joypads; ++joy)
			{
				for (int btn = 0; btn < NUM_JOYPAD_BUTTONS; ++btn)
				{
					if (flash_joy[joy] & (1 << btn))
						piano_roll.SetHeaderColumnLight(COLUMN_JOYPAD1_A + joy * NUM_JOYPAD_BUTTONS + btn, HEADER_LIGHT_MAX);
				}
			}
			result = true;
		} else
		{
			SetWindowText(hwndTextClipboard, clipboardEmptyText);
		}
		GlobalUnlock(hGlobal);
	}
	CloseClipboard();
	return result;
}
bool SPLICER::PasteInsert()
{
	SelectionFrames* current_selection = selection.MakeStrobe();
	if (current_selection->size() == 0) return false;

	if (!OpenClipboard(taseditor_window.hwndTasEditor)) return false;

	SelectionFrames::iterator current_selection_begin(current_selection->begin());
	int num_joypads = joysticks_per_frame[GetInputType(currMovieData)];
	bool result = false;
	bool markers_changed = false;
	int pos = *current_selection_begin;
	HANDLE hGlobal = GetClipboardData(CF_TEXT);
	if (hGlobal)
	{
		char *pGlobal = (char*)GlobalLock((HGLOBAL)hGlobal);

		// TAS recording info starts with "TAS "
		if (pGlobal[0]=='T' && pGlobal[1]=='A' && pGlobal[2]=='S')
		{
			// make sure Markers have the same size as movie
			markers_manager.update();
			// create inserted_set (for Input history hot changes)
			SelectionFrames inserted_set;

			// Extract number of frames
			int range;
			sscanf (pGlobal+3, "%d", &range);

			pGlobal = strchr(pGlobal, '\n');
			char* frame;
			int joy=0;
			std::vector<uint8> flash_joy(num_joypads);
			pos--;
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
						markers_manager.update();
					}
					while (*frame && *frame != '\n' && *frame != '|')
						++frame;
					if (*frame=='|') ++frame;
				} else
				{
					pos++;
				}
				
				// insert new frame
				currMovieData.insertEmpty(pos, 1);
				greenzone.laglog.InsertFrame(pos, false, 1);
				if (taseditor_config.bind_markers)
				{
					if (markers_manager.insertEmpty(pos, 1))
						markers_changed = true;
				}
				inserted_set.insert(pos);

				// read this frame Input
				int joy = 0;
				while (*frame && *frame != '\n' && *frame !='\r')
				{
					switch (*frame)
					{
					case '|': // Joystick mark
						joy++;
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
					frame++;
				}

				pGlobal = strchr(pGlobal, '\n');
			}
			markers_manager.update();
			int first_changes = history.RegisterChanges(MODTYPE_PASTEINSERT, *current_selection_begin, -1, 0, NULL, 0, &inserted_set);
			if (first_changes >= 0)
			{
				greenzone.InvalidateAndCheck(first_changes);
			} else if (markers_changed)
			{
				history.RegisterMarkersChange(MODTYPE_MARKER_SHIFT, *current_selection->begin());
				piano_roll.RedrawList();
			}
			if (markers_changed)
				selection.must_find_current_marker = playback.must_find_current_marker = true;
			// flash Piano Roll header columns that were changed during paste
			for (int joy = 0; joy < num_joypads; ++joy)
			{
				for (int btn = 0; btn < NUM_JOYPAD_BUTTONS; ++btn)
				{
					if (flash_joy[joy] & (1 << btn))
						piano_roll.SetHeaderColumnLight(COLUMN_JOYPAD1_A + joy * NUM_JOYPAD_BUTTONS + btn, HEADER_LIGHT_MAX);
				}
			}
			result = true;
		} else
		{
			SetWindowText(hwndTextClipboard, clipboardEmptyText);
		}
		GlobalUnlock(hGlobal);
	}
	CloseClipboard();
	return result;
}
// ----------------------------------------------------------------------------------------------
// retrieves some information from clipboard to clipboard_selection
void SPLICER::CheckClipboard()
{
	if (OpenClipboard(taseditor_window.hwndTasEditor))
	{
		// check if clipboard contains TAS Editor Input data
		HANDLE hGlobal = GetClipboardData(CF_TEXT);
		if (hGlobal)
		{
			clipboard_selection.clear();
			int current_pos = -1;
			char *pGlobal = (char*)GlobalLock((HGLOBAL)hGlobal);
			// TAS recording info starts with "TAS "
			if (pGlobal[0]=='T' && pGlobal[1]=='A' && pGlobal[2]=='S')
			{
				// Extract number of frames
				int range;
				sscanf (pGlobal+3, "%d", &range);
				pGlobal = strchr(pGlobal, '\n');
			
				while (pGlobal++ && *pGlobal!='\0')
				{
					// Detect skipped frames in paste
					char *frame = pGlobal;
					if (frame[0]=='+')
					{
						current_pos += atoi(frame+1);
						while (*frame && *frame != '\n' && *frame != '|')
							++frame;
						if (*frame=='|') ++frame;
					} else
						current_pos++;
					clipboard_selection.insert(current_pos);
					// skip Input
					pGlobal = strchr(pGlobal, '\n');
				}
			}
			GlobalUnlock(hGlobal);
		}
		CloseClipboard();
	}
}

void SPLICER::RedrawTextClipboard()
{
	if (clipboard_selection.size())
	{
		char new_text[100];
		strcpy(new_text, clipboardText);
		char num[11];
		// rows
		if (clipboard_selection.size() > 1)
		{
			_itoa(clipboard_selection.size(), num, 10);
			strcat(new_text, num);
			strcat(new_text, numTextRows);
		} else
		{
			strcat(new_text, numTextRow);
		}
		// columns
		int columns = NUM_JOYPAD_BUTTONS * joysticks_per_frame[GetInputType(currMovieData)];	// in future the number of columns will depend on selected columns
		if (columns > 1)
		{
			_itoa(columns, num, 10);
			strcat(new_text, num);
			strcat(new_text, numTextColumns);
		} else
		{
			strcat(new_text, numTextColumn);
		}
		SetWindowText(hwndTextClipboard, new_text);
	} else
		SetWindowText(hwndTextClipboard, clipboardEmptyText);
}

SelectionFrames& SPLICER::GetClipboardSelection()
{
	return clipboard_selection;
}


