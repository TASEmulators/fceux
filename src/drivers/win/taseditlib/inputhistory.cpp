//Implementation file of Input History class (Undo feature)

#include "movie.h"
#include "../common.h"
#include "../tasedit.h"
#include "inputsnapshot.h"
#include "inputhistory.h"
#include "playback.h"
#include "greenzone.h"

extern PLAYBACK playback;
extern void FCEU_printf(char *format, ...);
extern HWND hwndHistoryList;
extern GREENZONE greenzone;

char modCaptions[24][12] = {"Init",
							"Change",
							"Set",
							"Unset",
							"Insert",
							"Delete",
							"Truncate",
							"Clear",
							"Cut",
							"Paste",
							"PasteInsert",
							"Clone",
							"Record",
							"Import",
							"Branch0",
							"Branch1",
							"Branch2",
							"Branch3",
							"Branch4",
							"Branch5",
							"Branch6",
							"Branch7",
							"Branch8",
							"Branch9"};
char joypadCaptions[4][5] = {"(1P)", "(2P)", "(3P)", "(4P)"};

INPUT_HISTORY::INPUT_HISTORY()
{

}

void INPUT_HISTORY::init(int new_size)
{
	// init vars
	undo_hint_pos = old_undo_hint_pos = undo_hint_time = -1;
	old_show_undo_hint = show_undo_hint = false;
	history_size = new_size + 1;
	// clear snapshots history
	history_total_items = 0;
	input_snapshots.resize(history_size);
	history_start_pos = 0;
	history_cursor_pos = -1;
	// create initial snapshot
	INPUT_SNAPSHOT inp;
	inp.init(currMovieData);
	strcat(inp.description, modCaptions[0]);
	inp.jump_frame = -1;
	AddInputSnapshotToHistory(inp);

	UpdateHistoryList();
	RedrawHistoryList();
}
void INPUT_HISTORY::free()
{
	input_snapshots.resize(0);
}

void INPUT_HISTORY::update()
{
	// update undo_hint
	if (old_undo_hint_pos != undo_hint_pos && old_undo_hint_pos >= 0) RedrawRow(old_undo_hint_pos);
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
	if (old_show_undo_hint != show_undo_hint) RedrawRow(undo_hint_pos);



}

// returns frame of first input change (for greenzone invalidation)
int INPUT_HISTORY::jump(int new_pos)
{
	if (new_pos < 0) new_pos = 0; else if (new_pos >= history_total_items) new_pos = history_total_items-1;
	// if nothing is done, do not invalidate greenzone
	if (new_pos == history_cursor_pos) return -1;

	// create undo_hint
	if (new_pos < history_cursor_pos)
		undo_hint_pos = GetCurrentSnapshot().jump_frame;
	else
		undo_hint_pos = GetNextToCurrentSnapshot().jump_frame;
	undo_hint_time = clock() + UNDO_HINT_TIME;
	show_undo_hint = true;

	// make jump
	history_cursor_pos = new_pos;
	int real_pos = (history_start_pos + history_cursor_pos) % history_size;

	int first_change = input_snapshots[real_pos].findFirstChange(currMovieData);
	if (first_change < 0) return -1;	// if somehow there's no changes
	
	// update current movie
	input_snapshots[real_pos].toMovie(currMovieData, first_change);
	RedrawHistoryList();

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
		// reached the end of available history_size - move history_start_pos (thus deleting older snapshot)
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
			// compare with the snapshot we're going to overwrite, if it's different then break the chain of coherent snapshots
			if (input_snapshots[real_pos].checkDiff(inp))
			{
				for (int i = history_cursor_pos+1; i < history_total_items; ++i)
				{
					real_pos = (history_start_pos + i) % history_size;
					input_snapshots[real_pos].coherent = false;
				}
			}
		} else
		{
			// add new smapshot
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
int INPUT_HISTORY::RegisterInputChanges(int mod_type, int start, int end)
{
	// create new input shanshot
	INPUT_SNAPSHOT inp;
	inp.init(currMovieData);
	// check if there are differences from latest snapshot
	int real_pos = (history_start_pos + history_cursor_pos) % history_size;
	int first_changes = inp.findFirstChange(input_snapshots[real_pos], start, end);
	if (first_changes >= 0)
	{
		// differences found
		// fade old hot_changes by 1

		// highlight new hot changes

		// fill description
		strcat(inp.description, modCaptions[mod_type]);
		switch (mod_type)
		{
			case MODTYPE_CHANGE:
			case MODTYPE_SET:
			case MODTYPE_UNSET:
			case MODTYPE_TRUNCATE:
			case MODTYPE_CLEAR:
			case MODTYPE_CUT:
			case MODTYPE_IMPORT:
			case MODTYPE_BRANCH_0: case MODTYPE_BRANCH_1:
			case MODTYPE_BRANCH_2: case MODTYPE_BRANCH_3:
			case MODTYPE_BRANCH_4: case MODTYPE_BRANCH_5:
			case MODTYPE_BRANCH_6: case MODTYPE_BRANCH_7:
			case MODTYPE_BRANCH_8: case MODTYPE_BRANCH_9:
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
			case MODTYPE_RECORD:
			{
				// add info which joypads were affected
				int num = (inp.input_type + 1) * 2;		// hacky, only for distingushing between normal2p and fourscore
				for (int i = 0; i < num; ++i)
				{
					if (inp.checkJoypadDiff(input_snapshots[real_pos], first_changes, i))
						strcat(inp.description, joypadCaptions[i]);
				}
				inp.jump_frame = start;
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
		AddInputSnapshotToHistory(inp);
	}
	return first_changes;
}

void INPUT_HISTORY::save(EMUFILE *os)
{
	int real_pos, last_tick = 0;
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
}
void INPUT_HISTORY::load(EMUFILE *is)
{
	int i = -1;
	INPUT_SNAPSHOT inp;
	// delete old snapshots
	input_snapshots.resize(history_size);
	// read vars
	if (!read32le((uint32 *)&history_cursor_pos, is)) goto error;
	if (!read32le((uint32 *)&history_total_items, is)) goto error;
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
				if (!inp.skipLoad(is)) goto error;
			total -= num_snapshots_to_skip;
			history_cursor_pos -= num_snapshots_to_skip;
		}
		history_total_items -= num_snapshots_to_skip;
	}
	// load snapshots
	for (i = 0; i < history_total_items; ++i)
	{
		// skip snapshots if current history_size is less then history_total_items
		if (!input_snapshots[i].load(is)) goto error;
		playback.SetProgressbar(i, history_total_items);
	}
	// skip redo snapshots if needed
	for (; i < total; ++i)
		if (!inp.skipLoad(is)) goto error;

	// init vars
	undo_hint_pos = old_undo_hint_pos = undo_hint_time = -1;
	old_show_undo_hint = show_undo_hint = false;

	UpdateHistoryList();
	RedrawHistoryList();
	return;
error:
	// couldn't load full history - reset it
	FCEU_printf("Error loading history\n");
	init(history_size-1);
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
				msg->clrTextBk = HISTORY_COHERENT_COLOR;
			else
				msg->clrTextBk = HISTORY_NORMAL_COLOR;
			return CDRF_DODEFAULT;
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
			UpdateList();
			FollowUndo();
			greenzone.InvalidateGreenZone(result);
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

