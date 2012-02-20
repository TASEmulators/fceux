//Implementation file of TASEDITOR_SELECTION class
#include "taseditor_project.h"

extern TASEDITOR_CONFIG taseditor_config;
extern TASEDITOR_WINDOW taseditor_window;
extern MARKERS_MANAGER markers_manager;
extern TASEDITOR_LIST list;
extern SPLICER splicer;

extern int joysticks_per_frame[NUM_SUPPORTED_INPUT_TYPES];

LRESULT APIENTRY LowerMarkerEditWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
WNDPROC selectionMarkerEdit_oldWndproc;

// resources
char selection_save_id[SELECTION_ID_LEN] = "SELECTION";
char selection_skipsave_id[SELECTION_ID_LEN] = "SELECTIOX";
char lowerMarkerText[] = "Marker ";

TASEDITOR_SELECTION::TASEDITOR_SELECTION()
{
}

void TASEDITOR_SELECTION::init()
{
	hwndPrevMarker = GetDlgItem(taseditor_window.hwndTasEditor, TASEDITOR_PREV_MARKER);
	hwndNextMarker = GetDlgItem(taseditor_window.hwndTasEditor, TASEDITOR_NEXT_MARKER);
	hwndSelectionMarker = GetDlgItem(taseditor_window.hwndTasEditor, IDC_SELECTION_MARKER);
	SendMessage(hwndSelectionMarker, WM_SETFONT, (WPARAM)list.hMarkersFont, 0);
	hwndSelectionMarkerEdit = GetDlgItem(taseditor_window.hwndTasEditor, IDC_SELECTION_MARKER_EDIT);
	SendMessage(hwndSelectionMarkerEdit, EM_SETLIMITTEXT, MAX_NOTE_LEN - 1, 0);
	SendMessage(hwndSelectionMarkerEdit, WM_SETFONT, (WPARAM)list.hMarkersEditFont, 0);
	// subclass the edit control
	selectionMarkerEdit_oldWndproc = (WNDPROC)SetWindowLong(hwndSelectionMarkerEdit, GWL_WNDPROC, (LONG)LowerMarkerEditWndProc);

	reset();
}
void TASEDITOR_SELECTION::free()
{
	// clear history
	selections_history.resize(0);
	history_total_items = 0;
	temp_selection.clear();
}
void TASEDITOR_SELECTION::reset()
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
void TASEDITOR_SELECTION::reset_vars()
{
	old_prev_marker_button_state = prev_marker_button_state = false;
	old_next_marker_button_state = next_marker_button_state = false;
	must_find_current_marker = true;
}
void TASEDITOR_SELECTION::update()
{
	// keep selection within movie limits
	if (CurrentSelection().size())
	{
		int delete_index;
		int movie_size = currMovieData.getNumRecords();
		while(1)
		{
			delete_index = *CurrentSelection().rbegin();
			if (delete_index < movie_size) break;
			CurrentSelection().erase(delete_index);
			if (!CurrentSelection().size()) break;
		}
	}

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

	// track changes of selection beginning
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

void TASEDITOR_SELECTION::RedrawMarker()
{
	// redraw marker num
	char new_text[MAX_NOTE_LEN] = {0};
	if (shown_marker <= 9999)		// if there's too many digits in the number then don't show the word "Marker" before the number
		strcpy(new_text, lowerMarkerText);
	char num[11];
	_itoa(shown_marker, num, 10);
	strcat(new_text, num);
	SetWindowText(hwndSelectionMarker, new_text);
	// change marker note
	strcpy(new_text, markers_manager.GetNote(shown_marker).c_str());
	SetWindowText(hwndSelectionMarkerEdit, new_text);
}

void TASEDITOR_SELECTION::JumpPrevMarker(int speed)
{
	// if nothing is selected, consider playback cursor as current selection
	int index = GetCurrentSelectionBeginning();
	if (index < 0) index = currFrameCounter;
	// jump trough "speed" amount of previous markers
	while (speed > 0)
	{
		for (index--; index >= 0; index--)
			if (markers_manager.GetMarker(index)) break;
		speed--;
	}
	if (index >= 0)
		JumpToFrame(index);
	else
		JumpToFrame(0);
}
void TASEDITOR_SELECTION::JumpNextMarker(int speed)
{
	// if nothing is selected, consider playback cursor as current selection
	int index = GetCurrentSelectionBeginning();
	if (index < 0) index = currFrameCounter;
	int last_frame = currMovieData.getNumRecords()-1;
	// jump trough "speed" amount of previous markers
	while (speed > 0)
	{
		for (++index; index <= last_frame; ++index)
			if (markers_manager.GetMarker(index)) break;
		speed--;
	}
	if (index <= last_frame)
		JumpToFrame(index);
	else
		JumpToFrame(last_frame);
}
void TASEDITOR_SELECTION::JumpToFrame(int frame)
{
	ClearSelection();
	SetRowSelection(frame);
	list.FollowSelection();
}
// ----------------------------------------------------------
void TASEDITOR_SELECTION::save(EMUFILE *os, bool really_save)
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
bool TASEDITOR_SELECTION::load(EMUFILE *is)
{
	// read "SELECTION" string
	char save_id[SELECTION_ID_LEN];
	if ((int)is->fread(save_id, SELECTION_ID_LEN) < SELECTION_ID_LEN) goto error;
	if (!strcmp(selection_skipsave_id, save_id))
	{
		// string says to skip loading Selection
		FCEU_printf("No selection in the file\n");
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
	FCEU_printf("Error loading selection\n");
	reset();
	return true;
}

void TASEDITOR_SELECTION::saveSelection(SelectionFrames& selection, EMUFILE *os)
{
	write32le(selection.size(), os);
	if (selection.size())
	{
		for(SelectionFrames::iterator it(selection.begin()); it != selection.end(); it++)
			write32le(*it, os);
	}
}
bool TASEDITOR_SELECTION::loadSelection(SelectionFrames& selection, EMUFILE *is)
{
	int temp_int, temp_size;
	selection.clear();
	if (!read32le(&temp_size, is)) return true;
	selection.clear();
	for(; temp_size > 0; temp_size--)
	{
		if (!read32le(&temp_int, is)) return true;
		selection.insert(temp_int);
	}
	return false;
}
bool TASEDITOR_SELECTION::skiploadSelection(EMUFILE *is)
{
	int temp_size;
	if (!read32le(&temp_size, is)) return true;
	if (is->fseek(temp_size * sizeof(int), SEEK_CUR)) return true;
	return false;
}
// ----------------------------------------------------------
//used to track selection
void TASEDITOR_SELECTION::ItemRangeChanged(NMLVODSTATECHANGE* info)
{
	bool ON = !(info->uOldState & LVIS_SELECTED) && (info->uNewState & LVIS_SELECTED);
	bool OFF = (info->uOldState & LVIS_SELECTED) && !(info->uNewState & LVIS_SELECTED);

	if(ON)
		for(int i = info->iFrom; i <= info->iTo; ++i)
			CurrentSelection().insert(i);
	else
		for(int i = info->iFrom; i <= info->iTo; ++i)
			CurrentSelection().erase(i);

	splicer.must_redraw_selection_text = true;
}
void TASEDITOR_SELECTION::ItemChanged(NMLISTVIEW* info)
{
	int item = info->iItem;
	
	bool ON = !(info->uOldState & LVIS_SELECTED) && (info->uNewState & LVIS_SELECTED);
	bool OFF = (info->uOldState & LVIS_SELECTED) && !(info->uNewState & LVIS_SELECTED);

	//if the item is -1, apply the change to all items
	if(item == -1)
	{
		if(OFF)
		{
			// clear all (actually add new empty selection to history)
			if (CurrentSelection().size() && track_selection_changes)
				AddNewSelectionToHistory();
		} else if (ON)
		{
			// select all
			for(int i = currMovieData.getNumRecords() - 1; i >= 0; i--)
			{
				CurrentSelection().insert(i);
			}
		}
	} else
	{
		if(ON)
			CurrentSelection().insert(item);
		else if(OFF) 
			CurrentSelection().erase(item);
	}

	splicer.must_redraw_selection_text = true;
}
// ----------------------------------------------------------
void TASEDITOR_SELECTION::AddNewSelectionToHistory()
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

void TASEDITOR_SELECTION::jump(int new_pos)
{
	if (new_pos < 0) new_pos = 0; else if (new_pos >= history_total_items) new_pos = history_total_items-1;
	if (new_pos == history_cursor_pos) return;

	// make jump
	history_cursor_pos = new_pos;
	// update list items
	EnforceSelectionToList();
	// also keep  selection within list
	update();
}
void TASEDITOR_SELECTION::undo()
{
	jump(history_cursor_pos - 1);
}
void TASEDITOR_SELECTION::redo()
{
	jump(history_cursor_pos + 1);
}
// ----------------------------------------------------------
void TASEDITOR_SELECTION::ClearSelection()
{
	ListView_SetItemState(list.hwndList, -1, 0, LVIS_FOCUSED|LVIS_SELECTED);
}
void TASEDITOR_SELECTION::ClearRowSelection(int index)
{
	ListView_SetItemState(list.hwndList, index, 0, LVIS_SELECTED);
}

void TASEDITOR_SELECTION::EnforceSelectionToList()
{
	track_selection_changes = false;
	ClearSelection();
	for(SelectionFrames::reverse_iterator it(CurrentSelection().rbegin()); it != CurrentSelection().rend(); it++)
	{
		ListView_SetItemState(list.hwndList, *it, LVIS_SELECTED, LVIS_SELECTED);
	}
	track_selection_changes = true;
}
 
void TASEDITOR_SELECTION::SelectAll()
{
	ListView_SetItemState(list.hwndList, -1, LVIS_SELECTED, LVIS_SELECTED);
}
void TASEDITOR_SELECTION::SetRowSelection(int index)
{
	ListView_SetItemState(list.hwndList, index, LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);
}
void TASEDITOR_SELECTION::SetRegionSelection(int start, int end)
{
	for (int i = start; i <= end; ++i)
		ListView_SetItemState(list.hwndList, i, LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);
}
void TASEDITOR_SELECTION::SelectBetweenMarkers()
{
	int center, upper_border, lower_border;
	int upper_marker, lower_marker;
	int movie_size = currMovieData.getNumRecords();

	// if selection size=0 then playback cursor is selected and serves as center
	if (CurrentSelection().size())
	{
		upper_border = center = *CurrentSelection().begin();
		lower_border = *CurrentSelection().rbegin();
	} else lower_border = upper_border = center = currFrameCounter;

	// find markers
	// searching up starting from center-0
	for (upper_marker = center; upper_marker >= 0; upper_marker--)
		if (markers_manager.GetMarker(upper_marker)) break;
	// searching down starting from center+1
	for (lower_marker = center+1; lower_marker < movie_size; ++lower_marker)
		if (markers_manager.GetMarker(lower_marker)) break;

	// clear selection without clearing focused, because otherwise there's strange bug when quickly pressing Ctrl+A right after clicking on already selected row
	ListView_SetItemState(list.hwndList, -1, 0, LVIS_SELECTED);

	// special case
	if (upper_marker == -1 && lower_marker == movie_size)
	{
		SelectAll();
		return;
	}

	// selecting circle:
	if (upper_border > upper_marker+1 || lower_border < lower_marker-1 || lower_border > lower_marker)
	{
		// default: select all between markers
		for (int i = upper_marker+1; i < lower_marker; ++i)
		{
			ListView_SetItemState(list.hwndList, i, LVIS_SELECTED, LVIS_SELECTED);
		}
	} else if (upper_border == upper_marker+1 && lower_border == lower_marker-1)
	{
		// already selected all between markers - now select both markers or at least select the marker that is not outside movie range
		if (upper_marker < 0) upper_marker = 0;
		if (lower_marker >= movie_size) lower_marker = movie_size - 1;
		for (int i = upper_marker; i <= lower_marker; ++i)
		{
			ListView_SetItemState(list.hwndList, i, LVIS_SELECTED, LVIS_SELECTED);
		}
	} else if (upper_border <= upper_marker && lower_border >= lower_marker)
	{
		// selected all between markers and both markers selected too - now deselect lower marker
		for (int i = upper_marker; i < lower_marker; ++i)
		{
			ListView_SetItemState(list.hwndList, i, LVIS_SELECTED, LVIS_SELECTED);
		}
	} else if (upper_border == upper_marker && lower_border == lower_marker-1)
	{
		// selected all between markers and upper marker selected too - now deselect upper marker and (if lower marker < movie_size) reselect lower marker
		if (lower_marker >= movie_size) lower_marker = movie_size - 1;
		for (int i = upper_marker+1; i <= lower_marker; ++i)
		{
			ListView_SetItemState(list.hwndList, i, LVIS_SELECTED, LVIS_SELECTED);
		}
	} else if (upper_border == upper_marker+1 && lower_border == lower_marker)
	{
		// selected all between markers and lower marker selected too - now deselect lower marker (return to "selected all between markers")
		for (int i = upper_marker + 1; i < lower_marker; ++i)
		{
			ListView_SetItemState(list.hwndList, i, LVIS_SELECTED, LVIS_SELECTED);
		}
	}
}
void TASEDITOR_SELECTION::ReselectClipboard()
{
	SelectionFrames clipboard_selection = splicer.GetClipboardSelection();
	if (clipboard_selection.size() == 0) return;

	ClearSelection();
	CurrentSelection() = clipboard_selection;
	EnforceSelectionToList();
	// also keep selection within list
	update();
}

// getters
int TASEDITOR_SELECTION::GetCurrentSelectionSize()
{
	return selections_history[(history_start_pos + history_cursor_pos) % history_size].size();
}
int TASEDITOR_SELECTION::GetCurrentSelectionBeginning()
{
	if (selections_history[(history_start_pos + history_cursor_pos) % history_size].size())
		return *selections_history[(history_start_pos + history_cursor_pos) % history_size].begin();
	else
		return -1;
}
bool TASEDITOR_SELECTION::CheckFrameSelected(int frame)
{
	if(CurrentSelection().find(frame) == CurrentSelection().end())
		return false;
	return true;
}
SelectionFrames* TASEDITOR_SELECTION::MakeStrobe()
{
	// copy current selection to temp_selection
	temp_selection = selections_history[(history_start_pos + history_cursor_pos) % history_size];
	return &temp_selection;
}
SelectionFrames& TASEDITOR_SELECTION::GetStrobedSelection()
{
	return temp_selection;
}

// this getter is private
SelectionFrames& TASEDITOR_SELECTION::CurrentSelection()
{
	return selections_history[(history_start_pos + history_cursor_pos) % history_size];
}
// -------------------------------------------------------------------------
LRESULT APIENTRY LowerMarkerEditWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (markers_manager.marker_note_edit == MARKER_NOTE_EDIT_LOWER)
	{
		extern PLAYBACK playback;
		extern TASEDITOR_SELECTION selection;
		switch(msg)
		{
		case WM_CHAR:
		case WM_KEYDOWN:
			switch(wParam)
			{
			case VK_ESCAPE:
				// revert text to original note text
				SetWindowText(selection.hwndSelectionMarkerEdit, markers_manager.GetNote(selection.shown_marker).c_str());
				SetFocus(list.hwndList);
				return 0;
			case VK_RETURN:
				// exit and save text changes
				SetFocus(list.hwndList);
				return 0;
			case VK_TAB:
				// switch to upper edit control (also exit and save text changes)
				SetFocus(playback.hwndPlaybackMarkerEdit);
				return 0;
			}
			break;
		}
	}
	return CallWindowProc(selectionMarkerEdit_oldWndproc, hWnd, msg, wParam, lParam);
}

