//Implementation file of TASEDIT_SELECTION class

#include "taseditproj.h"

char selection_save_id[SELECTION_ID_LEN] = "SELECTION";

extern MARKERS markers;
extern TASEDIT_LIST tasedit_list;

TASEDIT_SELECTION::TASEDIT_SELECTION()
{
}

void TASEDIT_SELECTION::init(int new_size)
{
	// init vars
	if (new_size > 0)
		history_size = new_size + 1;
	// clear selections history
	free();
	selections_history.resize(history_size);
	history_start_pos = 0;
	history_cursor_pos = -1;
	// create initial selection
	AddNewSelectionToHistory();

	track_selection_changes = true;
}
void TASEDIT_SELECTION::free()
{
	// clear history
	selections_history.resize(0);
	history_total_items = 0;
	clipboard_selection.clear();
}

void TASEDIT_SELECTION::update()
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

}

void TASEDIT_SELECTION::save(EMUFILE *os)
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
	saveSelection(clipboard_selection, os);
}
// returns true if couldn't load
bool TASEDIT_SELECTION::load(EMUFILE *is)
{
	// read "SELECTION" string
	char save_id[SELECTION_ID_LEN];
	if ((int)is->fread(save_id, SELECTION_ID_LEN) < SELECTION_ID_LEN) goto error;
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
	if (loadSelection(clipboard_selection, is)) goto error;
	// all ok
	EnforceSelectionToList();
	return false;
error:
	return true;
}

void TASEDIT_SELECTION::saveSelection(SelectionFrames& selection, EMUFILE *os)
{
	write32le(selection.size(), os);
	if (selection.size())
	{
		for(SelectionFrames::iterator it(selection.begin()); it != selection.end(); it++)
			write32le(*it, os);
	}
}
bool TASEDIT_SELECTION::loadSelection(SelectionFrames& selection, EMUFILE *is)
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
bool TASEDIT_SELECTION::skiploadSelection(EMUFILE *is)
{
	int temp_int, temp_size;
	if (!read32le(&temp_size, is)) return true;
	for(; temp_size > 0; temp_size--)
		if (!read32le(&temp_int, is)) return true;
	return false;
}
// ----------------------------------------------------------
//used to track selection
void TASEDIT_SELECTION::ItemRangeChanged(NMLVODSTATECHANGE* info)
{
	bool ON = !(info->uOldState & LVIS_SELECTED) && (info->uNewState & LVIS_SELECTED);
	bool OFF = (info->uOldState & LVIS_SELECTED) && !(info->uNewState & LVIS_SELECTED);

	if(ON)
		for(int i = info->iFrom; i <= info->iTo; ++i)
			CurrentSelection().insert(i);
	else
		for(int i = info->iFrom; i <= info->iTo; ++i)
			CurrentSelection().erase(i);
}
void TASEDIT_SELECTION::ItemChanged(NMLISTVIEW* info)
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
}
// ----------------------------------------------------------
void TASEDIT_SELECTION::AddNewSelectionToHistory()
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

void TASEDIT_SELECTION::jump(int new_pos)
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
void TASEDIT_SELECTION::undo()
{
	jump(history_cursor_pos - 1);
}
void TASEDIT_SELECTION::redo()
{
	jump(history_cursor_pos + 1);
}

void TASEDIT_SELECTION::MemorizeClipboardSelection()
{
	// copy currently strobed selection data to clipboard_selection
	clipboard_selection = temp_selection;
}
void TASEDIT_SELECTION::ReselectClipboard()
{
	if (clipboard_selection.size() == 0) return;

	ClearSelection();
	CurrentSelection() = clipboard_selection;
	EnforceSelectionToList();
	// also keep selection within list
	update();
}
// ----------------------------------------------------------
void TASEDIT_SELECTION::ClearSelection()
{
	ListView_SetItemState(tasedit_list.hwndList, -1, 0, LVIS_FOCUSED|LVIS_SELECTED);
}
void TASEDIT_SELECTION::ClearRowSelection(int index)
{
	ListView_SetItemState(tasedit_list.hwndList, index, 0, LVIS_SELECTED);
}

void TASEDIT_SELECTION::EnforceSelectionToList()
{
	track_selection_changes = false;
	ClearSelection();
	for(SelectionFrames::reverse_iterator it(CurrentSelection().rbegin()); it != CurrentSelection().rend(); it++)
	{
		ListView_SetItemState(tasedit_list.hwndList, *it, LVIS_SELECTED, LVIS_SELECTED);
	}
	track_selection_changes = true;
}
 
void TASEDIT_SELECTION::SelectAll()
{
	ListView_SetItemState(tasedit_list.hwndList, -1, LVIS_SELECTED, LVIS_SELECTED);
	//RedrawList();
}
void TASEDIT_SELECTION::SetRowSelection(int index)
{
	ListView_SetItemState(tasedit_list.hwndList, index, LVIS_SELECTED, LVIS_SELECTED);
}
void TASEDIT_SELECTION::SetRegionSelection(int start, int end)
{
	for (int i = start; i <= end; ++i)
		ListView_SetItemState(tasedit_list.hwndList, i, LVIS_SELECTED, LVIS_SELECTED);
}
void TASEDIT_SELECTION::SelectMidMarkers()
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
		if (markers.markers_array[upper_marker] & MARKER_FLAG_BIT) break;
	// searching down starting from center+1
	for (lower_marker = center+1; lower_marker < movie_size; ++lower_marker)
		if (markers.markers_array[lower_marker] & MARKER_FLAG_BIT) break;

	// clear selection without clearing focused, because otherwise there's strange bug when quickly pressing Ctrl+A right after clicking on already selected row
	ListView_SetItemState(tasedit_list.hwndList, -1, 0, LVIS_SELECTED);

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
			ListView_SetItemState(tasedit_list.hwndList, i, LVIS_SELECTED, LVIS_SELECTED);
		}
	} else if (upper_border == upper_marker+1 && lower_border == lower_marker-1)
	{
		// already selected all between markers - now select both markers or at least select the marker that is not outside movie range
		if (upper_marker < 0) upper_marker = 0;
		if (lower_marker >= movie_size) lower_marker = movie_size - 1;
		for (int i = upper_marker; i <= lower_marker; ++i)
		{
			ListView_SetItemState(tasedit_list.hwndList, i, LVIS_SELECTED, LVIS_SELECTED);
		}
	} else if (upper_border <= upper_marker && lower_border >= lower_marker)
	{
		// selected all between markers and both markers selected too - now deselect lower marker
		for (int i = upper_marker; i < lower_marker; ++i)
		{
			ListView_SetItemState(tasedit_list.hwndList, i, LVIS_SELECTED, LVIS_SELECTED);
		}
	} else if (upper_border == upper_marker && lower_border == lower_marker-1)
	{
		// selected all between markers and upper marker selected too - now deselect upper marker and (if lower marker < movie_size) reselect lower marker
		if (lower_marker >= movie_size) lower_marker = movie_size - 1;
		for (int i = upper_marker+1; i <= lower_marker; ++i)
		{
			ListView_SetItemState(tasedit_list.hwndList, i, LVIS_SELECTED, LVIS_SELECTED);
		}
	} else if (upper_border == upper_marker+1 && lower_border == lower_marker)
	{
		// selected all between markers and lower marker selected too - now deselect lower marker (return to "selected all between markers")
		for (int i = upper_marker + 1; i < lower_marker; ++i)
		{
			ListView_SetItemState(tasedit_list.hwndList, i, LVIS_SELECTED, LVIS_SELECTED);
		}
	}
}
// getters
int TASEDIT_SELECTION::GetCurrentSelectionSize()
{
	return selections_history[(history_start_pos + history_cursor_pos) % history_size].size();
}
int TASEDIT_SELECTION::GetCurrentSelectionBeginning()
{
	if (selections_history[(history_start_pos + history_cursor_pos) % history_size].size())
		return *selections_history[(history_start_pos + history_cursor_pos) % history_size].begin();
	else
		return -1;
}
bool TASEDIT_SELECTION::CheckFrameSelected(int frame)
{
	if(CurrentSelection().find(frame) == CurrentSelection().end())
		return false;
	return true;
}
SelectionFrames* TASEDIT_SELECTION::MakeStrobe()
{
	// copy current selection to temp_selection
	temp_selection = selections_history[(history_start_pos + history_cursor_pos) % history_size];
	return &temp_selection;
}
SelectionFrames& TASEDIT_SELECTION::GetStrobedSelection()
{
	return temp_selection;
}
// this getter is only for inside-class use
SelectionFrames& TASEDIT_SELECTION::CurrentSelection()
{
	return selections_history[(history_start_pos + history_cursor_pos) % history_size];
}

