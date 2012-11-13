/* ---------------------------------------------------------------------------------
Implementation file of SELECTION class
Copyright (c) 2011-2012 AnS

(The MIT License)
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------------
Selection - Manager of selections
[Singleton]

* contains definition of the type "Set of selected frames"
* stores array of Sets of selected frames (History of selections)
* saves and loads the data from a project file. On error: clears the array and starts new history by making empty selection
* constantly tracks changes in selected rows of Piano Roll List, and makes a decision to create new point of Selection rollback
* implements all Selection restoring operations: undo, redo
* on demand: changes current selection: remove selection, jump to a frame with Selection cursor, select region, select all, select between Markers, reselect clipboard
* regularly ensures that Selection doesn't go beyond curent Piano Roll limits, detects if Selection moved to another Marker and updates Note in the lower text field
* implements the working of lower buttons << and >> (jumping on Markers)
* also here's the code of lower text field (for editing Marker Notes)
* stores resource: save id, lower text field prefix
------------------------------------------------------------------------------------ */

#include "taseditor_project.h"
#include "../taseditor.h"

extern TASEDITOR_CONFIG taseditor_config;
extern TASEDITOR_WINDOW taseditor_window;
extern MARKERS_MANAGER markers_manager;
extern PIANO_ROLL piano_roll;
extern SPLICER splicer;
extern EDITOR editor;
extern GREENZONE greenzone;

extern int joysticks_per_frame[NUM_SUPPORTED_INPUT_TYPES];

LRESULT APIENTRY LowerMarkerEditWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
WNDPROC selectionMarkerEdit_oldWndproc;

// resources
char selection_save_id[SELECTION_ID_LEN] = "SELECTION";
char selection_skipsave_id[SELECTION_ID_LEN] = "SELECTIOX";
char lowerMarkerText[] = "Marker ";

SELECTION::SELECTION()
{
}

void SELECTION::init()
{
	hwndPrevMarker = GetDlgItem(taseditor_window.hwndTasEditor, TASEDITOR_PREV_MARKER);
	hwndNextMarker = GetDlgItem(taseditor_window.hwndTasEditor, TASEDITOR_NEXT_MARKER);
	hwndSelectionMarker = GetDlgItem(taseditor_window.hwndTasEditor, IDC_SELECTION_MARKER);
	SendMessage(hwndSelectionMarker, WM_SETFONT, (WPARAM)piano_roll.hMarkersFont, 0);
	hwndSelectionMarkerEdit = GetDlgItem(taseditor_window.hwndTasEditor, IDC_SELECTION_MARKER_EDIT);
	SendMessage(hwndSelectionMarkerEdit, EM_SETLIMITTEXT, MAX_NOTE_LEN - 1, 0);
	SendMessage(hwndSelectionMarkerEdit, WM_SETFONT, (WPARAM)piano_roll.hMarkersEditFont, 0);
	// subclass the edit control
	selectionMarkerEdit_oldWndproc = (WNDPROC)SetWindowLong(hwndSelectionMarkerEdit, GWL_WNDPROC, (LONG)LowerMarkerEditWndProc);

	reset();
}
void SELECTION::free()
{
	// clear history
	selections_history.resize(0);
	history_total_items = 0;
	temp_selection.clear();
}
void SELECTION::reset()
{
	free();
	// init vars
	shown_marker = 0;
	last_selection_beginning = -1;
	history_size = taseditor_config.undo_levels + 1;
	selections_history.resize(history_size);
	history_start_pos = 0;
	history_cursor_pos = -1;
	// create initial selection
	AddNewSelectionToHistory();
	track_selection_changes = true;
	reset_vars();
}
void SELECTION::reset_vars()
{
	old_prev_marker_button_state = prev_marker_button_state = false;
	old_next_marker_button_state = next_marker_button_state = false;
	must_find_current_marker = true;
}
void SELECTION::update()
{
	UpdateSelectionSize();

	// update << and >> buttons
	old_prev_marker_button_state = prev_marker_button_state;
	prev_marker_button_state = ((Button_GetState(hwndPrevMarker) & BST_PUSHED) != 0);
	if (prev_marker_button_state)
	{
		if (!old_prev_marker_button_state)
		{
			button_hold_time = clock();
			JumpPrevMarker();
		} else if (button_hold_time + HOLD_REPEAT_DELAY < clock())
		{
			JumpPrevMarker();
		}
	}
	old_next_marker_button_state = next_marker_button_state;
	next_marker_button_state = (Button_GetState(hwndNextMarker) & BST_PUSHED) != 0;
	if (next_marker_button_state)
	{
		if (!old_next_marker_button_state)
		{
			button_hold_time = clock();
			JumpNextMarker();
		} else if (button_hold_time + HOLD_REPEAT_DELAY < clock())
		{
			JumpNextMarker();
		}
	}

	// track changes of Selection beginning (Selection cursor)
	if (last_selection_beginning != GetCurrentSelectionBeginning())
	{
		last_selection_beginning = GetCurrentSelectionBeginning();
		must_find_current_marker = true;
	}

	// update "Selection's Marker text" if needed
	if (must_find_current_marker)
	{
		markers_manager.UpdateMarkerNote();
		shown_marker = markers_manager.GetMarkerUp(last_selection_beginning);
		RedrawMarker();
		must_find_current_marker = false;
	}

}

void SELECTION::UpdateSelectionSize()
{
	// keep Selection within Piano Roll limits
	if (CurrentSelection().size())
	{
		int delete_index;
		int movie_size = currMovieData.getNumRecords();
		while (true)
		{
			delete_index = *CurrentSelection().rbegin();
			if (delete_index < movie_size) break;
			CurrentSelection().erase(delete_index);
			if (!CurrentSelection().size()) break;
		}
	}
}

void SELECTION::HistorySizeChanged()
{
	int new_history_size = taseditor_config.undo_levels + 1;
	std::vector<SelectionFrames> new_selections_history(new_history_size);
	int pos = history_cursor_pos, source_pos = history_cursor_pos;
	if (pos >= new_history_size)
		pos = new_history_size - 1;
	int new_history_cursor_pos = pos;
	// copy old "undo" snapshots
	while (pos >= 0)
	{
		new_selections_history[pos] = selections_history[(history_start_pos + source_pos) % history_size];
		pos--;
		source_pos--;
	}
	// copy old "redo" snapshots
	int num_redo_snapshots = history_total_items - (history_cursor_pos + 1);
	int space_available = new_history_size - (new_history_cursor_pos + 1);
	int i = (num_redo_snapshots <= space_available) ? num_redo_snapshots : space_available;
	int new_history_total_items = new_history_cursor_pos + i + 1;
	for (; i > 0; i--)
		new_selections_history[new_history_cursor_pos + i] = selections_history[(history_start_pos + history_cursor_pos + i) % history_size];
	// finish
	selections_history = new_selections_history;
	history_size = new_history_size;
	history_start_pos = 0;
	history_cursor_pos = new_history_cursor_pos;
	history_total_items = new_history_total_items;
}

void SELECTION::RedrawMarker()
{
	// redraw Marker num
	char new_text[MAX_NOTE_LEN] = {0};
	if (shown_marker <= 9999)		// if there's too many digits in the number then don't show the word "Marker" before the number
		strcpy(new_text, lowerMarkerText);
	char num[11];
	_itoa(shown_marker, num, 10);
	strcat(new_text, num);
	strcat(new_text, " ");
	SetWindowText(hwndSelectionMarker, new_text);
	// change Marker Note
	strcpy(new_text, markers_manager.GetNote(shown_marker).c_str());
	SetWindowText(hwndSelectionMarkerEdit, new_text);
}

void SELECTION::JumpPrevMarker(int speed)
{
	// if nothing is selected, consider Playback cursor as current selection
	int index = GetCurrentSelectionBeginning();
	if (index < 0) index = currFrameCounter;
	// jump trough "speed" amount of previous Markers
	while (speed > 0)
	{
		for (index--; index >= 0; index--)
			if (markers_manager.GetMarker(index)) break;
		speed--;
	}
	if (index >= 0)
		JumpToFrame(index);							// jump to the Marker
	else
		JumpToFrame(0);								// jump to the beginning of Piano Roll
}
void SELECTION::JumpNextMarker(int speed)
{
	// if nothing is selected, consider Playback cursor as current selection
	int index = GetCurrentSelectionBeginning();
	if (index < 0) index = currFrameCounter;
	int last_frame = currMovieData.getNumRecords() - 1;		// the end of Piano Roll
	// jump trough "speed" amount of previous Markers
	while (speed > 0)
	{
		for (++index; index <= last_frame; ++index)
			if (markers_manager.GetMarker(index)) break;
		speed--;
	}
	if (index <= last_frame)
		JumpToFrame(index);			// jump to Marker
	else
		JumpToFrame(last_frame);	// jump to the end of Piano Roll
}
void SELECTION::JumpToFrame(int frame)
{
	ClearSelection();
	SetRowSelection(frame);
	piano_roll.FollowSelection();
}
// ----------------------------------------------------------
void SELECTION::save(EMUFILE *os, bool really_save)
{
	if (really_save)
	{
		// write "SELECTION" string
		os->fwrite(selection_save_id, SELECTION_ID_LEN);
		// write vars
		write32le(history_cursor_pos, os);
		write32le(history_total_items, os);
		// write selections starting from history_start_pos
		for (int i = 0; i < history_total_items; ++i)
		{
			saveSelection(selections_history[(history_start_pos + i) % history_size], os);
		}
		// write clipboard_selection
		saveSelection(splicer.GetClipboardSelection(), os);
	} else
	{
		// write "SELECTIOX" string
		os->fwrite(selection_skipsave_id, SELECTION_ID_LEN);
	}
}
// returns true if couldn't load
bool SELECTION::load(EMUFILE *is, unsigned int offset)
{
	if (offset)
	{
		if (is->fseek(offset, SEEK_SET)) goto error;
	} else
	{
		reset();
		return false;
	}
	// read "SELECTION" string
	char save_id[SELECTION_ID_LEN];
	if ((int)is->fread(save_id, SELECTION_ID_LEN) < SELECTION_ID_LEN) goto error;
	if (!strcmp(selection_skipsave_id, save_id))
	{
		// string says to skip loading Selection
		FCEU_printf("No Selection in the file\n");
		reset();
		return false;
	}
	if (strcmp(selection_save_id, save_id)) goto error;		// string is not valid
	// read vars
	if (!read32le(&history_cursor_pos, is)) goto error;
	if (!read32le(&history_total_items, is)) goto error;
	if (history_cursor_pos > history_total_items) goto error;
	history_start_pos = 0;
	// read selections
	int i;
	int total = history_total_items;
	if (history_total_items > history_size)
	{
		// user can't afford that much undo levels, skip some selections
		int num_selections_to_skip = history_total_items - history_size;
		// first try to skip selections over history_cursor_pos (future selections), because "redo" is less important than "undo"
		int num_redo_selections = history_total_items-1 - history_cursor_pos;
		if (num_selections_to_skip >= num_redo_selections)
		{
			// skip all redo selections
			history_total_items = history_cursor_pos+1;
			num_selections_to_skip -= num_redo_selections;
			// and still need to skip some undo selections
			for (i = 0; i < num_selections_to_skip; ++i)
				if (skiploadSelection(is)) goto error;
			total -= num_selections_to_skip;
			history_cursor_pos -= num_selections_to_skip;
		}
		history_total_items -= num_selections_to_skip;
	}
	// load selections
	for (i = 0; i < history_total_items; ++i)
	{
		if (loadSelection(selections_history[i], is)) goto error;
	}
	// skip redo selections if needed
	for (; i < total; ++i)
		if (skiploadSelection(is)) goto error;
	
	// read clipboard_selection
	if (loadSelection(splicer.GetClipboardSelection(), is)) goto error;
	// all ok
	EnforceSelectionToList();
	reset_vars();
	return false;
error:
	FCEU_printf("Error loading Selection\n");
	reset();
	return true;
}

void SELECTION::saveSelection(SelectionFrames& selection, EMUFILE *os)
{
	write32le(selection.size(), os);
	if (selection.size())
	{
		for(SelectionFrames::iterator it(selection.begin()); it != selection.end(); it++)
			write32le(*it, os);
	}
}
bool SELECTION::loadSelection(SelectionFrames& selection, EMUFILE *is)
{
	int temp_int, temp_size;
	if (!read32le(&temp_size, is)) return true;
	selection.clear();
	for(; temp_size > 0; temp_size--)
	{
		if (!read32le(&temp_int, is)) return true;
		selection.insert(temp_int);
	}
	return false;
}
bool SELECTION::skiploadSelection(EMUFILE *is)
{
	int temp_size;
	if (!read32le(&temp_size, is)) return true;
	if (is->fseek(temp_size * sizeof(int), SEEK_CUR)) return true;
	return false;
}
// ----------------------------------------------------------
// used to track selection
void SELECTION::ItemRangeChanged(NMLVODSTATECHANGE* info)
{
	bool ON = !(info->uOldState & LVIS_SELECTED) && (info->uNewState & LVIS_SELECTED);
	bool OFF = (info->uOldState & LVIS_SELECTED) && !(info->uNewState & LVIS_SELECTED);

	if (ON)
		for(int i = info->iFrom; i <= info->iTo; ++i)
			CurrentSelection().insert(i);
	else
		for(int i = info->iFrom; i <= info->iTo; ++i)
			CurrentSelection().erase(i);

	splicer.must_redraw_selection_text = true;
}
void SELECTION::ItemChanged(NMLISTVIEW* info)
{
	int item = info->iItem;
	
	bool ON = !(info->uOldState & LVIS_SELECTED) && (info->uNewState & LVIS_SELECTED);
	bool OFF = (info->uOldState & LVIS_SELECTED) && !(info->uNewState & LVIS_SELECTED);

	//if the item is -1, apply the change to all items
	if (item == -1)
	{
		if (OFF)
		{
			// clear all (actually add new empty Selection to history)
			if (CurrentSelection().size() && track_selection_changes)
				AddNewSelectionToHistory();
		} else if (ON)
		{
			// select all
			for(int i = currMovieData.getNumRecords() - 1; i >= 0; i--)
				CurrentSelection().insert(i);
		}
	} else
	{
		if (ON)
			CurrentSelection().insert(item);
		else if (OFF) 
			CurrentSelection().erase(item);
	}

	splicer.must_redraw_selection_text = true;
}
// ----------------------------------------------------------
void SELECTION::AddNewSelectionToHistory()
{
	// create new empty selection
	SelectionFrames selectionFrames;
	// increase current position
	// history uses conveyor of selections (vector with fixed size) to avoid resizing
	if (history_cursor_pos+1 >= history_size)
	{
		// reached the end of available history_size - move history_start_pos (thus deleting oldest selection)
		history_cursor_pos = history_size-1;
		history_start_pos = (history_start_pos + 1) % history_size;
	} else
	{
		// didn't reach the end of history yet
		history_cursor_pos++;
		if (history_cursor_pos >= history_total_items)
			history_total_items = history_cursor_pos+1;
	}
	// add
	selections_history[(history_start_pos + history_cursor_pos) % history_size] = selectionFrames;
}
void SELECTION::AddCurrentSelectionToHistory()
{
	// create the copy of current selection
	SelectionFrames selectionFrames = selections_history[(history_start_pos + history_cursor_pos) % history_size];
	// increase current position
	// history uses conveyor of selections (vector with fixed size) to avoid resizing
	if (history_cursor_pos+1 >= history_size)
	{
		// reached the end of available history_size - move history_start_pos (thus deleting oldest selection)
		history_cursor_pos = history_size-1;
		history_start_pos = (history_start_pos + 1) % history_size;
	} else
	{
		// didn't reach the end of history yet
		history_cursor_pos++;
		if (history_cursor_pos >= history_total_items)
			history_total_items = history_cursor_pos+1;
	}
	// add
	selections_history[(history_start_pos + history_cursor_pos) % history_size] = selectionFrames;
}

void SELECTION::JumpInTime(int new_pos)
{
	if (new_pos < 0) new_pos = 0; else if (new_pos >= history_total_items) new_pos = history_total_items-1;
	if (new_pos == history_cursor_pos) return;

	// make jump
	history_cursor_pos = new_pos;
	// update Piano Roll items
	EnforceSelectionToList();
	// also keep Selection within Piano Roll
	UpdateSelectionSize();
}
void SELECTION::undo()
{
	JumpInTime(history_cursor_pos - 1);
}
void SELECTION::redo()
{
	JumpInTime(history_cursor_pos + 1);
}
// ----------------------------------------------------------
bool SELECTION::GetRowSelection(int index)
{
	return ListView_GetItemState(piano_roll.hwndList, index, LVIS_SELECTED) != 0;
}

void SELECTION::ClearSelection()
{
	ListView_SetItemState(piano_roll.hwndList, -1, 0, LVIS_SELECTED);
}
void SELECTION::ClearRowSelection(int index)
{
	ListView_SetItemState(piano_roll.hwndList, index, 0, LVIS_SELECTED);
}
void SELECTION::ClearRegionSelection(int start, int end)
{
	for (int i = start; i < end; ++i)
		ListView_SetItemState(piano_roll.hwndList, i, 0, LVIS_SELECTED);
}

void SELECTION::SelectAll()
{
	ListView_SetItemState(piano_roll.hwndList, -1, LVIS_SELECTED, LVIS_SELECTED);
}
void SELECTION::SetRowSelection(int index)
{
	ListView_SetItemState(piano_roll.hwndList, index, LVIS_SELECTED, LVIS_SELECTED);
}
void SELECTION::SetRegionSelection(int start, int end)
{
	for (int i = start; i < end; ++i)
		ListView_SetItemState(piano_roll.hwndList, i, LVIS_SELECTED, LVIS_SELECTED);
}

void SELECTION::SetRegionSelectionPattern(int start, int end)
{
	int pattern_offset = 0, current_pattern = taseditor_config.current_pattern;
	for (int i = start; i <= end; ++i)
	{
		// skip lag frames
		if (taseditor_config.pattern_skips_lag && greenzone.laglog.GetLagInfoAtFrame(i) == LAGGED_YES)
			continue;
		if (editor.autofire_patterns[current_pattern][pattern_offset])
		{
			ListView_SetItemState(piano_roll.hwndList, i, LVIS_SELECTED, LVIS_SELECTED);
		} else
		{
			ListView_SetItemState(piano_roll.hwndList, i, 0, LVIS_SELECTED);
		}
		pattern_offset++;
		if (pattern_offset >= (int)editor.autofire_patterns[current_pattern].size())
			pattern_offset -= editor.autofire_patterns[current_pattern].size();
	}
}
void SELECTION::SelectBetweenMarkers()
{
	int center, upper_border, lower_border;
	int upper_marker, lower_marker;
	int movie_size = currMovieData.getNumRecords();

	// if nothing is selected then Playback cursor serves as Selection cursor
	if (CurrentSelection().size())
	{
		upper_border = center = *CurrentSelection().begin();
		lower_border = *CurrentSelection().rbegin();
	} else lower_border = upper_border = center = currFrameCounter;

	// find Markers
	// searching up starting from center-0
	for (upper_marker = center; upper_marker >= 0; upper_marker--)
		if (markers_manager.GetMarker(upper_marker)) break;
	// searching down starting from center+1
	for (lower_marker = center+1; lower_marker < movie_size; ++lower_marker)
		if (markers_manager.GetMarker(lower_marker)) break;

	ClearSelection();

	// special case
	if (upper_marker == -1 && lower_marker == movie_size)
	{
		SelectAll();
		return;
	}

	// selecting circle: 1-2-3-4-1-2-3-4...
	if (upper_border > upper_marker+1 || lower_border < lower_marker-1 || lower_border > lower_marker)
	{
		// 1 - default: select all between Markers, not including lower Marker
		if (upper_marker < 0) upper_marker = 0;
		for (int i = upper_marker; i < lower_marker; ++i)
		{
			ListView_SetItemState(piano_roll.hwndList, i, LVIS_SELECTED, LVIS_SELECTED);
		}
	} else if (upper_border == upper_marker && lower_border == lower_marker-1)
	{
		// 2 - selected all between Markers and upper Marker selected too: select all between Markers, not including Markers
		for (int i = upper_marker+1; i < lower_marker; ++i)
		{
			ListView_SetItemState(piano_roll.hwndList, i, LVIS_SELECTED, LVIS_SELECTED);
		}
	} else if (upper_border == upper_marker+1 && lower_border == lower_marker-1)
	{
		// 3 - selected all between Markers, nut including Markers: select all between Markers, not including upper Marker
		if (lower_marker >= movie_size) lower_marker = movie_size - 1;
		for (int i = upper_marker+1; i <= lower_marker; ++i)
		{
			ListView_SetItemState(piano_roll.hwndList, i, LVIS_SELECTED, LVIS_SELECTED);
		}
	} else if (upper_border == upper_marker+1 && lower_border == lower_marker)
	{
		// 4 - selected all between Markers and lower Marker selected too: select all bertween Markers, including Markers
		if (upper_marker < 0) upper_marker = 0;
		if (lower_marker >= movie_size) lower_marker = movie_size - 1;
		for (int i = upper_marker; i <= lower_marker; ++i)
		{
			ListView_SetItemState(piano_roll.hwndList, i, LVIS_SELECTED, LVIS_SELECTED);
		}
	} else
	{
		// return to 1
		if (upper_marker < 0) upper_marker = 0;
		for (int i = upper_marker; i < lower_marker; ++i)
		{
			ListView_SetItemState(piano_roll.hwndList, i, LVIS_SELECTED, LVIS_SELECTED);
		}
	}
}
void SELECTION::ReselectClipboard()
{
	SelectionFrames clipboard_selection = splicer.GetClipboardSelection();
	if (clipboard_selection.size() == 0) return;

	ClearSelection();
	CurrentSelection() = clipboard_selection;
	EnforceSelectionToList();
	// also keep Selection within Piano Roll
	UpdateSelectionSize();
}

void SELECTION::Transpose(int shift)
{
	if (!shift) return;
	SelectionFrames* current_selection = MakeStrobe();
	if (current_selection->size())
	{
		ClearSelection();
		int pos;
		if (shift > 0)
		{
			int movie_size = currMovieData.getNumRecords();
			SelectionFrames::reverse_iterator current_selection_rend(current_selection->rend());
			for(SelectionFrames::reverse_iterator it(current_selection->rbegin()); it != current_selection_rend; it++)
			{
				pos = (*it) + shift;
				if (pos < movie_size)
					ListView_SetItemState(piano_roll.hwndList, pos, LVIS_SELECTED, LVIS_SELECTED);
			}
		} else
		{
			SelectionFrames::iterator current_selection_end(current_selection->end());
			for(SelectionFrames::iterator it(current_selection->begin()); it != current_selection_end; it++)
			{
				pos = (*it) + shift;
				if (pos >= 0)
					ListView_SetItemState(piano_roll.hwndList, pos, LVIS_SELECTED, LVIS_SELECTED);
			}
		}
	}
}

void SELECTION::EnforceSelectionToList()
{
	track_selection_changes = false;
	ClearSelection();
	for(SelectionFrames::reverse_iterator it(CurrentSelection().rbegin()); it != CurrentSelection().rend(); it++)
	{
		ListView_SetItemState(piano_roll.hwndList, *it, LVIS_SELECTED, LVIS_SELECTED);
	}
	track_selection_changes = true;
}

// getters
int SELECTION::GetCurrentSelectionSize()
{
	return selections_history[(history_start_pos + history_cursor_pos) % history_size].size();
}
int SELECTION::GetCurrentSelectionBeginning()
{
	if (selections_history[(history_start_pos + history_cursor_pos) % history_size].size())
		return *selections_history[(history_start_pos + history_cursor_pos) % history_size].begin();
	else
		return -1;
}
int SELECTION::GetCurrentSelectionEnd()
{
	if (selections_history[(history_start_pos + history_cursor_pos) % history_size].size())
		return *selections_history[(history_start_pos + history_cursor_pos) % history_size].rbegin();
	else
		return -1;
}
bool SELECTION::CheckFrameSelected(int frame)
{
	if (CurrentSelection().find(frame) == CurrentSelection().end())
		return false;
	return true;
}
SelectionFrames* SELECTION::MakeStrobe()
{
	// copy current Selection to temp_selection
	temp_selection = selections_history[(history_start_pos + history_cursor_pos) % history_size];
	return &temp_selection;
}

// this getter is private
SelectionFrames& SELECTION::CurrentSelection()
{
	return selections_history[(history_start_pos + history_cursor_pos) % history_size];
}
// -------------------------------------------------------------------------
LRESULT APIENTRY LowerMarkerEditWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	extern PLAYBACK playback;
	extern SELECTION selection;
	switch(msg)
	{
		case WM_SETFOCUS:
		{
			markers_manager.marker_note_edit = MARKER_NOTE_EDIT_LOWER;
			// enable editing
			SendMessage(selection.hwndSelectionMarkerEdit, EM_SETREADONLY, false, 0); 
			// disable FCEUX keyboard
			ClearTaseditorInput();
			break;
		}
		case WM_KILLFOCUS:
		{
			if (markers_manager.marker_note_edit == MARKER_NOTE_EDIT_LOWER)
			{
				markers_manager.UpdateMarkerNote();
				markers_manager.marker_note_edit = MARKER_NOTE_EDIT_NONE;
			}
			// disable editing (make the bg grayed)
			SendMessage(selection.hwndSelectionMarkerEdit, EM_SETREADONLY, true, 0); 
			// enable FCEUX keyboard
			if (taseditor_window.TASEditor_focus)
				SetTaseditorInput();
			break;
		}
		case WM_CHAR:
		case WM_KEYDOWN:
		{
			if (markers_manager.marker_note_edit == MARKER_NOTE_EDIT_LOWER)
			{
				switch(wParam)
				{
					case VK_ESCAPE:
						// revert text to original note text
						SetWindowText(selection.hwndSelectionMarkerEdit, markers_manager.GetNote(selection.shown_marker).c_str());
						SetFocus(piano_roll.hwndList);
						return 0;
					case VK_RETURN:
						// exit and save text changes
						SetFocus(piano_roll.hwndList);
						return 0;
					case VK_TAB:
					{
						// switch to upper edit control (also exit and save text changes)
						SetFocus(playback.hwndPlaybackMarkerEdit);
						// scroll to the Marker
						if (taseditor_config.follow_note_context)
							piano_roll.FollowMarker(playback.shown_marker);
						return 0;
					}
				}
			}
			break;
		}
		case WM_MBUTTONDOWN:
		case WM_MBUTTONDBLCLK:
		{
			playback.MiddleButtonClick();
			return 0;
		}
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		{
			// scroll to the Marker
			if (taseditor_config.follow_note_context)
				piano_roll.FollowMarker(selection.shown_marker);
			break;
		}
	}
	return CallWindowProc(selectionMarkerEdit_oldWndproc, hWnd, msg, wParam, lParam);
}

