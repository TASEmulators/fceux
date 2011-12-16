//Implementation file of Input History class (Undo feature)

#include "taseditproj.h"

extern HWND hwndTasEdit;
extern bool TASEdit_bind_markers;
extern bool TASEdit_enable_hot_changes;
extern bool TASEdit_branch_full_movie;
extern bool TASEdit_combine_consecutive_rec;
extern int TasEdit_undo_levels;

LRESULT APIENTRY HistoryListWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
WNDPROC hwndHistoryList_oldWndProc;

extern MARKERS current_markers;
extern BOOKMARKS bookmarks;
extern PLAYBACK playback;
extern TASEDIT_SELECTION selection;
extern GREENZONE greenzone;
extern TASEDIT_PROJECT project;
extern TASEDIT_LIST tasedit_list;

char history_save_id[HISTORY_ID_LEN] = "HISTORY";
char history_skipsave_id[HISTORY_ID_LEN] = "HISTORX";
char modCaptions[37][20] = {" Init",
							" Change",
							" Set",
							" Unset",
							" Insert",
							" Delete",
							" Truncate",
							" Clear",
							" Cut",
							" Paste",
							" PasteInsert",
							" Clone",
							" Record",
							" Import",
							" Branch0 to ",
							" Branch1 to ",
							" Branch2 to ",
							" Branch3 to ",
							" Branch4 to ",
							" Branch5 to ",
							" Branch6 to ",
							" Branch7 to ",
							" Branch8 to ",
							" Branch9 to ",
							" Marker Branch0 to ",
							" Marker Branch1 to ",
							" Marker Branch2 to ",
							" Marker Branch3 to ",
							" Marker Branch4 to ",
							" Marker Branch5 to ",
							" Marker Branch6 to ",
							" Marker Branch7 to ",
							" Marker Branch8 to ",
							" Marker Branch9 to ",
							" Marker Set",
							" Marker Unset",
							" Marker Rename"};
char joypadCaptions[4][5] = {"(1P)", "(2P)", "(3P)", "(4P)"};

INPUT_HISTORY::INPUT_HISTORY()
{
}

void INPUT_HISTORY::init()
{
	// prepare the history listview
	hwndHistoryList = GetDlgItem(hwndTasEdit, IDC_HISTORYLIST);
	ListView_SetExtendedListViewStyleEx(hwndHistoryList, LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES, LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
	// subclass the listview
	hwndHistoryList_oldWndProc = (WNDPROC)SetWindowLong(hwndHistoryList, GWL_WNDPROC, (LONG)HistoryListWndProc);
	LVCOLUMN lvc;
	lvc.mask = LVCF_WIDTH | LVCF_FMT;
	lvc.cx = 500;
	lvc.fmt = LVCFMT_LEFT;
	ListView_InsertColumn(hwndHistoryList, 0, &lvc);

	reset();
}
void INPUT_HISTORY::free()
{
	input_snapshots.resize(0);
	history_total_items = 0;
}
void INPUT_HISTORY::reset()
{
	free();
	// init vars
	history_size = TasEdit_undo_levels + 1;
	undo_hint_pos = old_undo_hint_pos = undo_hint_time = -1;
	old_show_undo_hint = show_undo_hint = false;
	input_snapshots.resize(history_size);
	history_start_pos = 0;
	history_cursor_pos = -1;
	// create initial snapshot
	INPUT_SNAPSHOT inp;
	inp.init(currMovieData, TASEdit_enable_hot_changes);
	strcat(inp.description, modCaptions[0]);
	inp.jump_frame = -1;
	AddInputSnapshotToHistory(inp);
	UpdateHistoryList();
	RedrawHistoryList();
}
void INPUT_HISTORY::update()
{
	// update undo_hint
	if (old_undo_hint_pos != undo_hint_pos && old_undo_hint_pos >= 0)
		tasedit_list.RedrawRow(old_undo_hint_pos);		// not changing bookmarks list
	old_undo_hint_pos = undo_hint_pos;
	old_show_undo_hint = show_undo_hint;
	show_undo_hint = false;
	if (undo_hint_pos >= 0)
	{
		if ((int)clock() < undo_hint_time)
			show_undo_hint = true;
		else
			undo_hint_pos = -1;	// finished hinting
	}
	if (old_show_undo_hint != show_undo_hint)
		tasedit_list.RedrawRow(undo_hint_pos);			// not changing bookmarks list



}

// returns frame of first input change (for greenzone invalidation)
int INPUT_HISTORY::jump(int new_pos)
{
	if (new_pos < 0) new_pos = 0; else if (new_pos >= history_total_items) new_pos = history_total_items-1;
	// if nothing is done, do not invalidate greenzone
	if (new_pos == history_cursor_pos) return -1;

	// make jump
	int old_pos = history_cursor_pos;
	history_cursor_pos = new_pos;
	int real_pos = (history_start_pos + history_cursor_pos) % history_size;
	RedrawHistoryList();

	// create undo_hint
	if (new_pos > old_pos)
		undo_hint_pos = GetCurrentSnapshot().jump_frame;
	else
		undo_hint_pos = GetNextToCurrentSnapshot().jump_frame;
	undo_hint_time = clock() + UNDO_HINT_TIME;
	show_undo_hint = true;

	// update markers
	bool markers_changed = false;
	if (TASEdit_bind_markers)
	{
		if (input_snapshots[real_pos].my_markers.checkMarkersDiff(current_markers))
		{
			input_snapshots[real_pos].copyToMarkers();
			project.SetProjectChanged();
			markers_changed = true;
		}
	}

	// update current movie
	int first_change = input_snapshots[real_pos].findFirstChange(currMovieData);
	if (first_change >= 0)
	{
		input_snapshots[real_pos].toMovie(currMovieData, first_change);
		bookmarks.ChangesMadeSinceBranch();
		// list will be redrawn by greenzone invalidation
	} else if (markers_changed)
	{
		current_markers.update();
		selection.must_find_current_marker = playback.must_find_current_marker = true;
		bookmarks.ChangesMadeSinceBranch();
		tasedit_list.RedrawList();
	} else if (TASEdit_enable_hot_changes)
	{
		// when using Hot Changes, list should be always redrawn, because old changes become less hot
		tasedit_list.RedrawList();
	}

	return first_change;
}
int INPUT_HISTORY::undo()
{
	return jump(history_cursor_pos - 1);
}
int INPUT_HISTORY::redo()
{
	return jump(history_cursor_pos + 1);
}
// ----------------------------
void INPUT_HISTORY::AddInputSnapshotToHistory(INPUT_SNAPSHOT &inp)
{
	// history uses conveyor of snapshots (vector with fixed size) to aviod resizing which is awfully expensive with such large objects as INPUT_SNAPSHOT
	int real_pos;
	if (history_cursor_pos+1 >= history_size)
	{
		// reached the end of available history_size - move history_start_pos (thus deleting oldest snapshot)
		history_cursor_pos = history_size-1;
		history_start_pos = (history_start_pos + 1) % history_size;
	} else
	{
		// didn't reach the end of history yet
		history_cursor_pos++;
		if (history_cursor_pos < history_total_items)
		{
			// overwrite old snapshot
			real_pos = (history_start_pos + history_cursor_pos) % history_size;
			// compare with the snapshot we're going to overwrite, if it's different then truncate history after this item
			if (input_snapshots[real_pos].checkDiff(inp) || input_snapshots[real_pos].my_markers.checkMarkersDiff(inp.my_markers))
			{
				history_total_items = history_cursor_pos+1;
				UpdateHistoryList();
			} else
			{
				// it's not different - don't truncate history, but break the chain of coherent snapshots
				for (int i = history_cursor_pos+1; i < history_total_items; ++i)
				{
					real_pos = (history_start_pos + i) % history_size;
					input_snapshots[real_pos].coherent = false;
				}
			}
		} else
		{
			// add new snapshot
			history_total_items = history_cursor_pos+1;
			UpdateHistoryList();
		}
	}
	// write snapshot
	real_pos = (history_start_pos + history_cursor_pos) % history_size;
	input_snapshots[real_pos] = inp;
	RedrawHistoryList();
}

// returns frame of first actual change
int INPUT_HISTORY::RegisterChanges(int mod_type, int start, int end)
{
	// create new input shanshot
	INPUT_SNAPSHOT inp;
	inp.init(currMovieData, TASEdit_enable_hot_changes);
	inp.mod_type = mod_type;
	// check if there are input differences from latest snapshot
	int real_pos = (history_start_pos + history_cursor_pos) % history_size;
	int first_changes = inp.findFirstChange(input_snapshots[real_pos], start, end);
	if (first_changes >= 0)
	{
		// differences found
		// fill description:
		strcat(inp.description, modCaptions[mod_type]);
		switch (mod_type)
		{
			case MODTYPE_CHANGE:
			case MODTYPE_SET:
			case MODTYPE_UNSET:
			case MODTYPE_TRUNCATE:
			case MODTYPE_CLEAR:
			case MODTYPE_CUT:
			{
				inp.jump_frame = first_changes;
				break;
			}
			case MODTYPE_INSERT:
			case MODTYPE_DELETE:
			case MODTYPE_PASTE:
			case MODTYPE_PASTEINSERT:
			case MODTYPE_CLONE:
			{
				// for these changes user prefers to see frame of attempted change (selection beginning), not frame of actual differences
				inp.jump_frame = start;
				break;
			}
		}
		// add upper and lower frame to description
		char framenum[11];
		_itoa(start, framenum, 10);
		strcat(inp.description, " ");
		strcat(inp.description, framenum);
		if (end > start)
		{
			_itoa(end, framenum, 10);
			strcat(inp.description, "-");
			strcat(inp.description, framenum);
		}
		// set hotchanges
		if (TASEdit_enable_hot_changes)
		{
			// inherit previous hotchanges and set new changes
			switch (mod_type)
			{
				case MODTYPE_DELETE:
					inp.inheritHotChanges_DeleteSelection(&input_snapshots[real_pos]);
					break;
				case MODTYPE_INSERT:
				case MODTYPE_CLONE:
					inp.inheritHotChanges_InsertSelection(&input_snapshots[real_pos]);
					break;
				case MODTYPE_PASTEINSERT:
					inp.inheritHotChanges_PasteInsert(&input_snapshots[real_pos]);
					break;
				case MODTYPE_CHANGE:
				case MODTYPE_SET:
				case MODTYPE_UNSET:
				case MODTYPE_CLEAR:
				case MODTYPE_CUT:
				case MODTYPE_PASTE:
					inp.inheritHotChanges(&input_snapshots[real_pos]);
					inp.fillHotChanges(input_snapshots[real_pos], first_changes, end);
					break;
				case MODTYPE_TRUNCATE:
					inp.copyHotChanges(&input_snapshots[real_pos]);
					// do not add new hotchanges and do not fade old hotchanges, because there was nothing added
					break;
			}
		}
		AddInputSnapshotToHistory(inp);
		bookmarks.ChangesMadeSinceBranch();
	}
	return first_changes;
}
void INPUT_HISTORY::RegisterMarkersChange(int mod_type, int start, int end)
{
	// create new input shanshot
	INPUT_SNAPSHOT inp;
	inp.init(currMovieData, TASEdit_enable_hot_changes);
	inp.mod_type = mod_type;
	// fill description:
	strcat(inp.description, modCaptions[mod_type]);
	inp.jump_frame = start;
	// add the frame to description
	char framenum[11];
	_itoa(start, framenum, 10);
	strcat(inp.description, " ");
	strcat(inp.description, framenum);
	if (end > start)
	{
		_itoa(end, framenum, 10);
		strcat(inp.description, "-");
		strcat(inp.description, framenum);
	}
	if (TASEdit_enable_hot_changes)
		inp.copyHotChanges(&GetCurrentSnapshot());
	AddInputSnapshotToHistory(inp);
	bookmarks.ChangesMadeSinceBranch();
}
void INPUT_HISTORY::RegisterBranching(int mod_type, int first_change, int slot)
{
	// create new input snapshot
	INPUT_SNAPSHOT inp;
	inp.init(currMovieData, TASEdit_enable_hot_changes);
	// fill description: modification type + time of the Branch
	inp.mod_type = mod_type;
	strcat(inp.description, modCaptions[mod_type]);
	strcat(inp.description, bookmarks.bookmarks_array[slot].snapshot.description);
	inp.jump_frame = first_change;
	if (TASEdit_enable_hot_changes)
	{
		if (mod_type < MODTYPE_BRANCH_MARKERS_0)
		{
			// input was changed
			// copy hotchanges of the Branch
			if (TASEdit_branch_full_movie)
			{
				inp.copyHotChanges(&bookmarks.bookmarks_array[slot].snapshot);
			} else
			{
				// input was branched partially, so copy hotchanges only up to and not including jump_frame of the Branch
				inp.copyHotChanges(&bookmarks.bookmarks_array[slot].snapshot, bookmarks.bookmarks_array[slot].snapshot.jump_frame);
			}
		} else
		{
			// input was not changed, only Markers were changed
			inp.copyHotChanges(&GetCurrentSnapshot());
		}
	}
	AddInputSnapshotToHistory(inp);
}
void INPUT_HISTORY::RegisterRecording(int frame_of_change)
{
	int real_pos = (history_start_pos + history_cursor_pos) % history_size;
	INPUT_SNAPSHOT inp;
	inp.init(currMovieData, TASEdit_enable_hot_changes);
	inp.fillJoypadsDiff(input_snapshots[real_pos], frame_of_change);
	inp.mod_type = MODTYPE_RECORD;
	strcat(inp.description, modCaptions[MODTYPE_RECORD]);
	char framenum[11];
	// check if current snapshot is also Recording and maybe it is consecutive recording
	if (TASEdit_combine_consecutive_rec && input_snapshots[real_pos].mod_type == MODTYPE_RECORD && input_snapshots[real_pos].rec_end_frame+1 == frame_of_change && input_snapshots[real_pos].rec_joypad_diff_bits == inp.rec_joypad_diff_bits)
	{
		// clone this snapshot and continue chain of recorded frames
		inp.jump_frame = input_snapshots[real_pos].jump_frame;
		inp.rec_end_frame = frame_of_change;
		// add info which joypads were affected
		int num = (inp.input_type + 1) * 2;	// = joypads_per_frame
		uint32 current_mask = 1;
		for (int i = 0; i < num; ++i)
		{
			if ((inp.rec_joypad_diff_bits & current_mask))
				strcat(inp.description, joypadCaptions[i]);
			current_mask <<= 1;
		}
		// add upper and lower frame to description
		_itoa(inp.jump_frame, framenum, 10);
		strcat(inp.description, " ");
		strcat(inp.description, framenum);
		_itoa(frame_of_change, framenum, 10);
		strcat(inp.description, "-");
		strcat(inp.description, framenum);
		// set hotchanges
		if (TASEdit_enable_hot_changes)
		{
			inp.copyHotChanges(&input_snapshots[real_pos]);
			inp.fillHotChanges(input_snapshots[real_pos], frame_of_change, frame_of_change);
		}
		// replace current snapshot with this cloned snapshot and truncate history here
		input_snapshots[real_pos] = inp;
		history_total_items = history_cursor_pos+1;
		UpdateHistoryList();
		RedrawHistoryList();
	} else
	{
		// not consecutive - add new snapshot to history
		inp.jump_frame = inp.rec_end_frame = frame_of_change;
		// add info which joypads were affected
		int num = (inp.input_type + 1) * 2;	// = joypads_per_frame
		uint32 current_mask = 1;
		for (int i = 0; i < num; ++i)
		{
			if ((inp.rec_joypad_diff_bits & current_mask))
				strcat(inp.description, joypadCaptions[i]);
			current_mask <<= 1;
		}
		// add upper frame to description
		_itoa(frame_of_change, framenum, 10);
		strcat(inp.description, " ");
		strcat(inp.description, framenum);
		// set hotchanges
		if (TASEdit_enable_hot_changes)
		{
			inp.inheritHotChanges(&input_snapshots[real_pos]);
			inp.fillHotChanges(input_snapshots[real_pos], frame_of_change, frame_of_change);
		}
		AddInputSnapshotToHistory(inp);
	}
	bookmarks.ChangesMadeSinceBranch();
}
void INPUT_HISTORY::RegisterImport(MovieData& md, char* filename)
{
	// create new input snapshot
	INPUT_SNAPSHOT inp;
	inp.init(md, TASEdit_enable_hot_changes, (currMovieData.fourscore)?FOURSCORE:NORMAL_2JOYPADS);
	// check if there are input differences from latest snapshot
	int real_pos = (history_start_pos + history_cursor_pos) % history_size;
	int first_changes = inp.findFirstChange(input_snapshots[real_pos]);
	if (first_changes >= 0)
	{
		// differences found
		inp.mod_type = MODTYPE_IMPORT;
		inp.jump_frame = first_changes;
		// fill description:
		strcat(inp.description, modCaptions[inp.mod_type]);
		// add filename to description
		strcat(inp.description, " ");
		strncat(inp.description, filename, SNAPSHOT_DESC_MAX_LENGTH - strlen(inp.description) - 1);
		if (TASEdit_enable_hot_changes)
		{
			// do not inherit old hotchanges, because imported input (most likely) doesn't have direct connection with recent edits, so old hotchanges are irrelevant and should not be copied
			inp.fillHotChanges(input_snapshots[real_pos], first_changes);
		}
		AddInputSnapshotToHistory(inp);
		inp.toMovie(currMovieData);
		tasedit_list.update();
		bookmarks.ChangesMadeSinceBranch();
		project.SetProjectChanged();
		greenzone.InvalidateAndCheck(first_changes);
	} else
	{
		MessageBox(hwndTasEdit, "Imported movie has the same input.\nNo changes were made.", "TAS Editor", MB_OK);
	}
}

void INPUT_HISTORY::save(EMUFILE *os, bool really_save)
{
	if (really_save)
	{
		int real_pos, last_tick = 0;
		// write "HISTORY" string
		os->fwrite(history_save_id, HISTORY_ID_LEN);
		// write vars
		write32le(history_cursor_pos, os);
		write32le(history_total_items, os);
		// write snapshots starting from history_start_pos
		for (int i = 0; i < history_total_items; ++i)
		{
			real_pos = (history_start_pos + i) % history_size;
			input_snapshots[real_pos].save(os);
			playback.SetProgressbar(i, history_total_items);
		}
	} else
	{
		// write "HISTORX" string
		os->fwrite(history_skipsave_id, HISTORY_ID_LEN);
	}
}
// returns true if couldn't load
bool INPUT_HISTORY::load(EMUFILE *is)
{
	int i = -1;
	INPUT_SNAPSHOT inp;
	// delete old snapshots
	input_snapshots.resize(history_size);
	// read "HISTORY" string
	char save_id[HISTORY_ID_LEN];
	if ((int)is->fread(save_id, HISTORY_ID_LEN) < HISTORY_ID_LEN) goto error;
	if (!strcmp(history_skipsave_id, save_id))
	{
		// string says to skip loading History
		FCEU_printf("No history in the file\n");
		reset();
		return false;
	}
	if (strcmp(history_save_id, save_id)) goto error;		// string is not valid
	// read vars
	if (!read32le(&history_cursor_pos, is)) goto error;
	if (!read32le(&history_total_items, is)) goto error;
	if (history_cursor_pos > history_total_items) goto error;
	history_start_pos = 0;
	// read snapshots
	int total = history_total_items;
	if (history_total_items > history_size)
	{
		// user can't afford that much undo levels, skip some snapshots
		int num_snapshots_to_skip = history_total_items - history_size;
		// first try to skip snapshots over history_cursor_pos (future snapshots), because "redo" is less important than "undo"
		int num_redo_snapshots = history_total_items-1 - history_cursor_pos;
		if (num_snapshots_to_skip >= num_redo_snapshots)
		{
			// skip all redo snapshots
			history_total_items = history_cursor_pos+1;
			num_snapshots_to_skip -= num_redo_snapshots;
			// and still need to skip some undo snapshots
			for (i = 0; i < num_snapshots_to_skip; ++i)
				if (inp.skipLoad(is)) goto error;
			total -= num_snapshots_to_skip;
			history_cursor_pos -= num_snapshots_to_skip;
		}
		history_total_items -= num_snapshots_to_skip;
	}
	// load snapshots
	for (i = 0; i < history_total_items; ++i)
	{
		if (input_snapshots[i].load(is)) goto error;
		playback.SetProgressbar(i, history_total_items);
	}
	// skip redo snapshots if needed
	for (; i < total; ++i)
		if (inp.skipLoad(is)) goto error;

	// init vars
	undo_hint_pos = old_undo_hint_pos = undo_hint_time = -1;
	old_show_undo_hint = show_undo_hint = false;

	// everything went well
	UpdateHistoryList();
	RedrawHistoryList();
	return false;
error:
	FCEU_printf("Error loading history\n");
	reset();
	return true;
}
// ----------------------------
void INPUT_HISTORY::GetDispInfo(NMLVDISPINFO* nmlvDispInfo)
{
	LVITEM& item = nmlvDispInfo->item;
	if(item.mask & LVIF_TEXT)
		strcpy(item.pszText, GetItemDesc(item.iItem));
}

LONG INPUT_HISTORY::CustomDraw(NMLVCUSTOMDRAW* msg)
{
	switch(msg->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;
	case CDDS_ITEMPREPAINT:
		{
			if (GetItemCoherence(msg->nmcd.dwItemSpec))
				msg->clrText = HISTORY_NORMAL_COLOR;
			else
				msg->clrText = HISTORY_INCOHERENT_COLOR;
		}
	default:
		return CDRF_DODEFAULT;
	}

}

void INPUT_HISTORY::Click(LPNMITEMACTIVATE info)
{
	// jump to pointed input snapshot
	int item = info->iItem;
	if (item >= 0)
	{
		int result = jump(item);
		if (result >= 0)
		{
			tasedit_list.update();
			tasedit_list.FollowUndo();
			greenzone.InvalidateAndCheck(result);
			return;
		}
	}
	RedrawHistoryList();
}

void INPUT_HISTORY::UpdateHistoryList()
{
	//update the number of items in the history list
	int currLVItemCount = ListView_GetItemCount(hwndHistoryList);
	if(currLVItemCount != history_total_items)
		ListView_SetItemCountEx(hwndHistoryList, history_total_items, LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);
}

void INPUT_HISTORY::RedrawHistoryList()
{
	ListView_SetItemState(hwndHistoryList, history_cursor_pos, LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);
	ListView_EnsureVisible(hwndHistoryList, history_cursor_pos, FALSE);
	InvalidateRect(hwndHistoryList, 0, FALSE);
}
// ----------------------------
INPUT_SNAPSHOT& INPUT_HISTORY::GetCurrentSnapshot()
{
	return input_snapshots[(history_start_pos + history_cursor_pos) % history_size];
}
INPUT_SNAPSHOT& INPUT_HISTORY::GetNextToCurrentSnapshot()
{
	if (history_cursor_pos < history_total_items)
		return input_snapshots[(history_start_pos + history_cursor_pos + 1) % history_size];
	else
		return input_snapshots[(history_start_pos + history_cursor_pos) % history_size];
}
char* INPUT_HISTORY::GetItemDesc(int pos)
{
	return input_snapshots[(history_start_pos + pos) % history_size].description;
}
bool INPUT_HISTORY::GetItemCoherence(int pos)
{
	return input_snapshots[(history_start_pos + pos) % history_size].coherent;
}
int INPUT_HISTORY::GetUndoHint()
{
	if (show_undo_hint)
		return undo_hint_pos;
	else
		return -1;
}
// ---------------------------------------------------------------------------------
LRESULT APIENTRY HistoryListWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
		case WM_CHAR:
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_KILLFOCUS:
			return 0;
		case WM_SYSKEYDOWN:
		{
			if (wParam == VK_F10)
				return 0;
			break;
		}
	}
	return CallWindowProc(hwndHistoryList_oldWndProc, hWnd, msg, wParam, lParam);
}


