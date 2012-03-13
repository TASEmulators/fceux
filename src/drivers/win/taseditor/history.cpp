/* ---------------------------------------------------------------------------------
Implementation file of History class
Copyright (c) 2011-2012 AnS

(The MIT License)
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------------
History - History of movie modifications
[Singleton]

* stores array of snapshots and pointer to current (last) snapshot
* saves and loads the data from a project file. On error: clears the array and starts new history by making snapshot of current movie data
* on demand: checks the difference between the last snapshot and current movie, and makes a decision to create new point of rollback. In special cases it can create a point of rollback without checking the difference, assuming that caller already checked it
* implements all restoring operations: undo, redo, revert to any snapshot from the array
* also stores the state of "undo pointer"
* regularly updates the state of "undo pointer"
* implements the working of History List: creating, redrawing, clicks, auto-scrolling
* stores resources: save id, ids and names of all possible types of modification, timings of "undo pointer"
------------------------------------------------------------------------------------ */

#include "taseditor_project.h"

LRESULT APIENTRY HistoryListWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
WNDPROC hwndHistoryList_oldWndProc;

extern TASEDITOR_CONFIG taseditor_config;
extern TASEDITOR_WINDOW taseditor_window;
extern MARKERS_MANAGER markers_manager;
extern BOOKMARKS bookmarks;
extern PLAYBACK playback;
extern SELECTION selection;
extern GREENZONE greenzone;
extern TASEDITOR_PROJECT project;
extern PIANO_ROLL piano_roll;
extern TASEDITOR_LUA taseditor_lua;

extern int joysticks_per_frame[NUM_SUPPORTED_INPUT_TYPES];
extern int GetInputType(MovieData& md);

extern Window_items_struct window_items[];

char history_save_id[HISTORY_ID_LEN] = "HISTORY";
char history_skipsave_id[HISTORY_ID_LEN] = "HISTORX";
char modCaptions[MODTYPES_TOTAL][20] = {" Init Project",
							" Undefined",
							" Set",
							" Unset",
							" Pattern",
							" Insert",
							" Insert#",
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
							" Marker Pattern",
							" Marker Rename",
							" Marker Drag",
							" Marker Swap",
							" Marker Shift",
							" LUA Marker Set",
							" LUA Marker Unset",
							" LUA Marker Rename",
							" LUA Change" };
char LuaCaptionPrefix[6] = " LUA ";
char joypadCaptions[4][5] = {"(1P)", "(2P)", "(3P)", "(4P)"};

HISTORY::HISTORY()
{
}

void HISTORY::init()
{
	// prepare the history listview
	hwndHistoryList = GetDlgItem(taseditor_window.hwndTasEditor, IDC_HISTORYLIST);
	ListView_SetExtendedListViewStyleEx(hwndHistoryList, LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES, LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
	// subclass the listview
	hwndHistoryList_oldWndProc = (WNDPROC)SetWindowLong(hwndHistoryList, GWL_WNDPROC, (LONG)HistoryListWndProc);
	LVCOLUMN lvc;
	lvc.mask = LVCF_WIDTH | LVCF_FMT;
	lvc.cx = 500;
	lvc.fmt = LVCFMT_LEFT;
	ListView_InsertColumn(hwndHistoryList, 0, &lvc);
}
void HISTORY::free()
{
	snapshots.resize(0);
	history_total_items = 0;
}
void HISTORY::reset()
{
	free();
	// init vars
	history_size = taseditor_config.undo_levels + 1;
	undo_hint_pos = old_undo_hint_pos = undo_hint_time = -1;
	old_show_undo_hint = show_undo_hint = false;
	snapshots.resize(history_size);
	history_start_pos = 0;
	history_cursor_pos = -1;
	// create initial snapshot
	SNAPSHOT inp;
	inp.init(currMovieData, taseditor_config.enable_hot_changes);
	strcat(inp.description, modCaptions[MODTYPE_INIT]);
	inp.jump_frame = -1;
	AddSnapshotToHistory(inp);
	UpdateHistoryList();
	RedrawHistoryList();
}
void HISTORY::update()
{
	// update undo_hint
	if (old_undo_hint_pos != undo_hint_pos && old_undo_hint_pos >= 0)
		piano_roll.RedrawRow(old_undo_hint_pos);		// not changing Bookmarks List
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
		piano_roll.RedrawRow(undo_hint_pos);			// not changing Bookmarks List
}

void HISTORY::HistorySizeChanged()
{
	int new_history_size = taseditor_config.undo_levels + 1;
	std::vector<SNAPSHOT> new_snapshots(new_history_size);
	int pos = history_cursor_pos, source_pos = history_cursor_pos;
	if (pos >= new_history_size)
		pos = new_history_size - 1;
	int new_history_cursor_pos = pos;
	// copy old "undo" snapshots
	while (pos >= 0)
	{
		new_snapshots[pos] = snapshots[(history_start_pos + source_pos) % history_size];
		pos--;
		source_pos--;
	}
	// copy old "redo" snapshots
	int num_redo_snapshots = history_total_items - (history_cursor_pos + 1);
	int space_available = new_history_size - (new_history_cursor_pos + 1);
	int i = (num_redo_snapshots <= space_available) ? num_redo_snapshots : space_available;
	int new_history_total_items = new_history_cursor_pos + i + 1;
	for (; i > 0; i--)
		new_snapshots[new_history_cursor_pos + i] = snapshots[(history_start_pos + history_cursor_pos + i) % history_size];
	// finish
	snapshots = new_snapshots;
	history_size = new_history_size;
	history_start_pos = 0;
	history_cursor_pos = new_history_cursor_pos;
	history_total_items = new_history_total_items;
	UpdateHistoryList();
	RedrawHistoryList();
}

// returns frame of first input change (for greenzone invalidation)
int HISTORY::jump(int new_pos)
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
	if (taseditor_config.bind_markers)
	{
		if (snapshots[real_pos].MarkersDifferFromCurrent())
		{
			snapshots[real_pos].copyToMarkers();
			project.SetProjectChanged();
			markers_changed = true;
		}
	}

	// update current movie
	int first_change = snapshots[real_pos].findFirstChange(currMovieData);
	if (first_change >= 0)
	{
		snapshots[real_pos].toMovie(currMovieData, first_change);
		selection.must_find_current_marker = playback.must_find_current_marker = true;
		bookmarks.ChangesMadeSinceBranch();
		// and Piano Roll will be redrawn by greenzone invalidation
	} else if (markers_changed)
	{
		markers_manager.update();
		selection.must_find_current_marker = playback.must_find_current_marker = true;
		bookmarks.ChangesMadeSinceBranch();
		piano_roll.RedrawList();
		piano_roll.FollowUndo();
	} else if (taseditor_config.enable_hot_changes)
	{
		// when using Hot Changes, Piano Roll should be always redrawn, because old changes become less hot
		piano_roll.RedrawList();
	}

	return first_change;
}

void HISTORY::undo()
{
	int result = jump(history_cursor_pos - 1);
	if (result >= 0)
	{
		piano_roll.update();
		piano_roll.FollowUndo();
		greenzone.InvalidateAndCheck(result);
	}
	return;
}
void HISTORY::redo()
{
	int result = jump(history_cursor_pos + 1);
	if (result >= 0)
	{
		piano_roll.update();
		piano_roll.FollowUndo();
		greenzone.InvalidateAndCheck(result);
	}
	return;
}
// ----------------------------
void HISTORY::AddSnapshotToHistory(SNAPSHOT &inp)
{
	// history uses conveyor of snapshots (vector with fixed size) to aviod resizing which is awfully expensive with such large objects as SNAPSHOT
	if (history_total_items >= history_size)
	{
		// reached the end of available history_size - move history_start_pos (thus deleting oldest snapshot)
		history_cursor_pos = history_size-1;
		history_start_pos = (history_start_pos + 1) % history_size;
	} else
	{
		// didn't reach the end of history yet
		history_cursor_pos++;
		history_total_items = history_cursor_pos+1;
		UpdateHistoryList();
	}
	// write snapshot
	int real_pos = (history_start_pos + history_cursor_pos) % history_size;
	snapshots[real_pos] = inp;
	RedrawHistoryList();
}

// returns frame of first actual change
int HISTORY::RegisterChanges(int mod_type, int start, int end, const char* comment)
{
	// create new shanshot
	SNAPSHOT inp;
	inp.init(currMovieData, taseditor_config.enable_hot_changes);
	inp.mod_type = mod_type;
	// check if there are input differences from latest snapshot
	int real_pos = (history_start_pos + history_cursor_pos) % history_size;
	int first_changes = inp.findFirstChange(snapshots[real_pos], start, end);
	if (first_changes >= 0)
	{
		// differences found
		// fill description:
		strcat(inp.description, modCaptions[mod_type]);
		switch (mod_type)
		{
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
			case MODTYPE_CLONE:
			case MODTYPE_PATTERN:
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
		// add comment if there is one specified
		if (comment)
		{
			strcat(inp.description, " ");
			strncat(inp.description, comment, SNAPSHOT_DESC_MAX_LENGTH - strlen(inp.description) - 1);
		}
		// set hotchanges
		if (taseditor_config.enable_hot_changes)
		{
			// inherit previous hotchanges and set new changes
			switch (mod_type)
			{
				case MODTYPE_DELETE:
					inp.inheritHotChanges_DeleteSelection(&snapshots[real_pos]);
					break;
				case MODTYPE_INSERT:
				case MODTYPE_CLONE:
					inp.inheritHotChanges_InsertSelection(&snapshots[real_pos]);
					break;
				case MODTYPE_SET:
				case MODTYPE_UNSET:
				case MODTYPE_CLEAR:
				case MODTYPE_CUT:
				case MODTYPE_PASTE:
				case MODTYPE_PATTERN:
					inp.inheritHotChanges(&snapshots[real_pos]);
					inp.fillHotChanges(snapshots[real_pos], first_changes, end);
					break;
				case MODTYPE_TRUNCATE:
					inp.copyHotChanges(&snapshots[real_pos]);
					// do not add new hotchanges and do not fade old hotchanges, because there was nothing added
					break;
			}
		}
		AddSnapshotToHistory(inp);
		bookmarks.ChangesMadeSinceBranch();
	}
	return first_changes;
}
int HISTORY::RegisterInsertNum(int start, int frames)
{
	// create new shanshot
	SNAPSHOT inp;
	inp.init(currMovieData, taseditor_config.enable_hot_changes);
	inp.mod_type = MODTYPE_INSERTNUM;
	// check if there are input differences from latest snapshot
	int real_pos = (history_start_pos + history_cursor_pos) % history_size;
	int first_changes = inp.findFirstChange(snapshots[real_pos], start);
	if (first_changes >= 0)
	{
		// differences found
		// fill description:
		strcat(inp.description, modCaptions[inp.mod_type]);
		inp.jump_frame = start;
		char framenum[11];
		// add number of inserted frames to description
		_itoa(frames, framenum, 10);
		strcat(inp.description, framenum);
		// add upper frame to description
		_itoa(start, framenum, 10);
		strcat(inp.description, " ");
		strcat(inp.description, framenum);
		// set hotchanges
		if (taseditor_config.enable_hot_changes)
			inp.inheritHotChanges_InsertNum(&snapshots[real_pos], start, frames);
		AddSnapshotToHistory(inp);
		bookmarks.ChangesMadeSinceBranch();
	}
	return first_changes;
}
int HISTORY::RegisterPasteInsert(int start, SelectionFrames& inserted_set)
{
	// create new shanshot
	SNAPSHOT inp;
	inp.init(currMovieData, taseditor_config.enable_hot_changes);
	inp.mod_type = MODTYPE_PASTEINSERT;
	// check if there are input differences from latest snapshot
	int real_pos = (history_start_pos + history_cursor_pos) % history_size;
	int first_changes = inp.findFirstChange(snapshots[real_pos], start);
	if (first_changes >= 0)
	{
		// differences found
		// fill description:
		strcat(inp.description, modCaptions[inp.mod_type]);
		// for PasteInsert user prefers to see frame of attempted change (selection beginning), not frame of actual differences
		inp.jump_frame = start;
		// add upper frame to description
		char framenum[11];
		_itoa(start, framenum, 10);
		strcat(inp.description, " ");
		strcat(inp.description, framenum);
		// set hotchanges
		if (taseditor_config.enable_hot_changes)
			inp.inheritHotChanges_PasteInsert(&snapshots[real_pos], inserted_set);
		AddSnapshotToHistory(inp);
		bookmarks.ChangesMadeSinceBranch();
	}
	return first_changes;
}
void HISTORY::RegisterMarkersChange(int mod_type, int start, int end, const char* comment)
{
	// create new shanshot
	SNAPSHOT inp;
	inp.init(currMovieData, taseditor_config.enable_hot_changes);
	inp.mod_type = mod_type;
	// fill description:
	strcat(inp.description, modCaptions[mod_type]);
	inp.jump_frame = start;
	// add the frame to description
	char framenum[11];
	_itoa(start, framenum, 10);
	strcat(inp.description, " ");
	strcat(inp.description, framenum);
	if (end > start || mod_type == MODTYPE_MARKER_DRAG || mod_type == MODTYPE_MARKER_SWAP)
	{
		if (mod_type == MODTYPE_MARKER_DRAG)
			strcat(inp.description, "=>");
		else if (mod_type == MODTYPE_MARKER_SWAP)
			strcat(inp.description, "<=>");
		else
			strcat(inp.description, "-");
		_itoa(end, framenum, 10);
		strcat(inp.description, framenum);
	}
	// add comment if there is one specified
	if (comment)
	{
		strcat(inp.description, " ");
		strncat(inp.description, comment, SNAPSHOT_DESC_MAX_LENGTH - strlen(inp.description) - 1);
	}
	// input hotchanges aren't changed
	if (taseditor_config.enable_hot_changes)
		inp.copyHotChanges(&GetCurrentSnapshot());
	AddSnapshotToHistory(inp);
	bookmarks.ChangesMadeSinceBranch();
	project.SetProjectChanged();
}
void HISTORY::RegisterBranching(int mod_type, int first_change, int slot)
{
	// create new snapshot
	SNAPSHOT inp;
	inp.init(currMovieData, taseditor_config.enable_hot_changes);
	// fill description: modification type + time of the Branch
	inp.mod_type = mod_type;
	strcat(inp.description, modCaptions[mod_type]);
	strcat(inp.description, bookmarks.bookmarks_array[slot].snapshot.description);
	inp.jump_frame = first_change;
	if (taseditor_config.enable_hot_changes)
	{
		if (mod_type < MODTYPE_BRANCH_MARKERS_0)
		{
			// input was changed
			// copy hotchanges of the Branch
			if (taseditor_config.branch_full_movie)
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
	AddSnapshotToHistory(inp);
}
void HISTORY::RegisterRecording(int frame_of_change)
{
	int real_pos = (history_start_pos + history_cursor_pos) % history_size;
	SNAPSHOT inp;
	inp.init(currMovieData, taseditor_config.enable_hot_changes);
	inp.fillJoypadsDiff(snapshots[real_pos], frame_of_change);
	inp.mod_type = MODTYPE_RECORD;
	strcat(inp.description, modCaptions[MODTYPE_RECORD]);
	char framenum[11];
	// check if current snapshot is also Recording and maybe it is consecutive recording
	if (taseditor_config.combine_consecutive_rec
		&& snapshots[real_pos].mod_type == MODTYPE_RECORD							// a) also Recording
		&& snapshots[real_pos].rec_end_frame+1 == frame_of_change					// b) consecutive
		&& snapshots[real_pos].rec_joypad_diff_bits == inp.rec_joypad_diff_bits)	// c) recorded same set of joysticks
	{
		// clone this snapshot and continue chain of recorded frames
		inp.jump_frame = snapshots[real_pos].jump_frame;
		inp.rec_end_frame = frame_of_change;
		// add info which joypads were affected
		int num = joysticks_per_frame[inp.input_type];
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
		if (taseditor_config.enable_hot_changes)
		{
			inp.copyHotChanges(&snapshots[real_pos]);
			inp.fillHotChanges(snapshots[real_pos], frame_of_change, frame_of_change);
		}
		// replace current snapshot with this cloned snapshot and truncate history here
		snapshots[real_pos] = inp;
		history_total_items = history_cursor_pos+1;
		UpdateHistoryList();
		RedrawHistoryList();
	} else
	{
		// not consecutive - add new snapshot to history
		inp.jump_frame = inp.rec_end_frame = frame_of_change;
		// add info which joypads were affected
		int num = joysticks_per_frame[inp.input_type];
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
		if (taseditor_config.enable_hot_changes)
		{
			inp.inheritHotChanges(&snapshots[real_pos]);
			inp.fillHotChanges(snapshots[real_pos], frame_of_change, frame_of_change);
		}
		AddSnapshotToHistory(inp);
	}
	bookmarks.ChangesMadeSinceBranch();
}
void HISTORY::RegisterImport(MovieData& md, char* filename)
{
	// create new snapshot
	SNAPSHOT inp;
	inp.init(md, taseditor_config.enable_hot_changes, GetInputType(currMovieData));
	// check if there are input differences from latest snapshot
	int real_pos = (history_start_pos + history_cursor_pos) % history_size;
	int first_changes = inp.findFirstChange(snapshots[real_pos]);
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
		if (taseditor_config.enable_hot_changes)
		{
			// do not inherit old hotchanges, because imported input (most likely) doesn't have direct connection with recent edits, so old hotchanges are irrelevant and should not be copied
			inp.fillHotChanges(snapshots[real_pos], first_changes);
		}
		AddSnapshotToHistory(inp);
		inp.toMovie(currMovieData);
		piano_roll.update();
		bookmarks.ChangesMadeSinceBranch();
		project.SetProjectChanged();
		greenzone.InvalidateAndCheck(first_changes);
	} else
	{
		MessageBox(taseditor_window.hwndTasEditor, "Imported movie has the same input.\nNo changes were made.", "TAS Editor", MB_OK);
	}
}
int HISTORY::RegisterLuaChanges(const char* name, int start, bool InsertionDeletion_was_made)
{
	// create new shanshot
	SNAPSHOT inp;
	inp.init(currMovieData, taseditor_config.enable_hot_changes);
	inp.mod_type = MODTYPE_LUA_CHANGE;
	// check if there are input differences from latest snapshot
	int real_pos = (history_start_pos + history_cursor_pos) % history_size;
	int first_changes = inp.findFirstChange(snapshots[real_pos], start);
	if (first_changes >= 0)
	{
		// differences found
		// fill description:
		if (name[0])
		{
			// user provided custom name of operation
			strcat(inp.description, LuaCaptionPrefix);
			strncat(inp.description, name, LUACHANGES_NAME_MAX_LEN);
		} else
		{
			// set default name
			strcat(inp.description, modCaptions[inp.mod_type]);
		}
		inp.jump_frame = first_changes;
		// add upper frame to description
		char framenum[11];
		_itoa(first_changes, framenum, 10);
		strcat(inp.description, " ");
		strcat(inp.description, framenum);
		// set hotchanges
		if (taseditor_config.enable_hot_changes)
		{
			if (InsertionDeletion_was_made)
			{
				// do it hard way: take old hot_changes and insert/delete rows to create a snapshot that is comparable to inp
				if (inp.input_type == snapshots[real_pos].input_type)
				{
					// create temp copy of current snapshot (we need it as a container for hot_changes)
					SNAPSHOT hotchanges_snapshot = snapshots[real_pos];
					if (hotchanges_snapshot.has_hot_changes)
					{
						hotchanges_snapshot.FadeHotChanges();
					} else
					{
						hotchanges_snapshot.has_hot_changes = true;
						hotchanges_snapshot.hot_changes.resize(joysticks_per_frame[inp.input_type] * hotchanges_snapshot.size * HOTCHANGE_BYTES_PER_JOY);
					}
					// insert/delete frames in hotchanges_snapshot, so that it will be the same size as inp
					taseditor_lua.InsertDelete_rows_to_Snaphot(hotchanges_snapshot);
					inp.copyHotChanges(&hotchanges_snapshot);
					inp.fillHotChanges(hotchanges_snapshot, first_changes);
				}
			} else
			{
				// easy way: inp.size is equal to currentsnapshot.size, so we can simply inherit hotchanges
				inp.inheritHotChanges(&snapshots[real_pos]);
				inp.fillHotChanges(snapshots[real_pos], first_changes);
			}
		}
		AddSnapshotToHistory(inp);
		bookmarks.ChangesMadeSinceBranch();
	}
	return first_changes;
}

void HISTORY::save(EMUFILE *os, bool really_save)
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
			snapshots[real_pos].save(os);
			playback.SetProgressbar(i, history_total_items);
		}
	} else
	{
		// write "HISTORX" string
		os->fwrite(history_skipsave_id, HISTORY_ID_LEN);
	}
}
// returns true if couldn't load
bool HISTORY::load(EMUFILE *is)
{
	int i = -1;
	SNAPSHOT inp;
	// delete old snapshots
	snapshots.resize(history_size);
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
		if (snapshots[i].load(is)) goto error;
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
void HISTORY::GetDispInfo(NMLVDISPINFO* nmlvDispInfo)
{
	LVITEM& item = nmlvDispInfo->item;
	if(item.mask & LVIF_TEXT)
		strcpy(item.pszText, GetItemDesc(item.iItem));
}

LONG HISTORY::CustomDraw(NMLVCUSTOMDRAW* msg)
{
	switch(msg->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;
	case CDDS_ITEMPREPAINT:
		msg->clrText = HISTORY_NORMAL_COLOR;
	default:
		return CDRF_DODEFAULT;
	}

}

void HISTORY::Click(int row_index)
{
	// jump to pointed snapshot
	if (row_index >= 0)
	{
		int result = jump(row_index);
		if (result >= 0)
		{
			piano_roll.update();
			piano_roll.FollowUndo();
			greenzone.InvalidateAndCheck(result);
			return;
		}
	}
	RedrawHistoryList();
}

void HISTORY::UpdateHistoryList()
{
	//update the number of items in the history list
	int currLVItemCount = ListView_GetItemCount(hwndHistoryList);
	if(currLVItemCount != history_total_items)
		ListView_SetItemCountEx(hwndHistoryList, history_total_items, LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);
}

void HISTORY::RedrawHistoryList()
{
	ListView_SetItemState(hwndHistoryList, history_cursor_pos, LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);
	ListView_EnsureVisible(hwndHistoryList, history_cursor_pos, FALSE);
	InvalidateRect(hwndHistoryList, 0, FALSE);
}
// ----------------------------
SNAPSHOT& HISTORY::GetCurrentSnapshot()
{
	return snapshots[(history_start_pos + history_cursor_pos) % history_size];
}
SNAPSHOT& HISTORY::GetNextToCurrentSnapshot()
{
	if (history_cursor_pos < history_total_items)
		return snapshots[(history_start_pos + history_cursor_pos + 1) % history_size];
	else
		return snapshots[(history_start_pos + history_cursor_pos) % history_size];
}
char* HISTORY::GetItemDesc(int pos)
{
	return snapshots[(history_start_pos + pos) % history_size].description;
}
int HISTORY::GetUndoHint()
{
	if (show_undo_hint)
		return undo_hint_pos;
	else
		return -1;
}
bool HISTORY::CursorOverHistoryList()
{
	POINT p;
	if (GetCursorPos(&p))
	{
		RECT wrect;
		GetWindowRect(hwndHistoryList, &wrect);
		ScreenToClient(hwndHistoryList, &p);
		if (p.x >= 0
			&& p.y >= 0
			&& p.x < window_items[HISTORYLIST_IN_WINDOWITEMS].width
			&& p.y < (wrect.bottom - wrect.top))
			return true;
	}
	return false;
}
// ---------------------------------------------------------------------------------
LRESULT APIENTRY HistoryListWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	extern HISTORY history;
	switch(msg)
	{
		case WM_CHAR:
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_KILLFOCUS:
			return 0;
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		{
			if (GetFocus() != hWnd)
				SetFocus(hWnd);
			// perform hit test
			LVHITTESTINFO info;
			info.pt.x = GET_X_LPARAM(lParam);
			info.pt.y = GET_Y_LPARAM(lParam);
			ListView_SubItemHitTest(hWnd, (LPARAM)&info);
			history.Click(info.iItem);
			return 0;
		}
		case WM_MBUTTONDOWN:
		case WM_MBUTTONDBLCLK:
		{
			if (GetFocus() != hWnd)
				SetFocus(hWnd);
			playback.MiddleButtonClick();
			return 0;
		}
		case WM_RBUTTONDOWN:
		case WM_RBUTTONDBLCLK:
			if (GetFocus() != hWnd)
				SetFocus(hWnd);
			return 0;
		case WM_MOUSEWHEEL:
		{
			if (!history.CursorOverHistoryList())
				return SendMessage(piano_roll.hwndList, msg, wParam, lParam);
			break;
		}
		case WM_MOUSEWHEEL_RESENT:
		{
			// this is message from Piano Roll
			// it means that cursor is currently over History List, and user scrolls the wheel (although focus may be on some other window)
			// ensure that wParam's low-order word is 0 (so fwKeys = 0)
			CallWindowProc(hwndHistoryList_oldWndProc, hWnd, WM_MOUSEWHEEL, wParam & ~(LOWORD(-1)), lParam);
			return 0;
		}
        case WM_MOUSEACTIVATE:
			if (GetFocus() != hWnd)
				SetFocus(hWnd);
            break;

	}
	return CallWindowProc(hwndHistoryList_oldWndProc, hWnd, msg, wParam, lParam);
}


