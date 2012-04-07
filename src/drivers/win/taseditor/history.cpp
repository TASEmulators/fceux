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

* stores array of History items (snapshots, backup_bookmarks, backup_current_branch) and pointer to current snapshot
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
extern BRANCHES branches;
extern PLAYBACK playback;
extern SELECTION selection;
extern GREENZONE greenzone;
extern TASEDITOR_PROJECT project;
extern PIANO_ROLL piano_roll;
extern POPUP_DISPLAY popup_display;
extern TASEDITOR_LUA taseditor_lua;

extern int joysticks_per_frame[NUM_SUPPORTED_INPUT_TYPES];
extern int GetInputType(MovieData& md);

extern Window_items_struct window_items[];

char history_save_id[HISTORY_ID_LEN] = "HISTORY";
char history_skipsave_id[HISTORY_ID_LEN] = "HISTORX";
char modCaptions[MODTYPES_TOTAL][20] = {" Initialization",
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
										" Bookmark0",
										" Bookmark1",
										" Bookmark2",
										" Bookmark3",
										" Bookmark4",
										" Bookmark5",
										" Bookmark6",
										" Bookmark7",
										" Bookmark8",
										" Bookmark9",
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
										" Marker Remove",
										" Marker Pattern",
										" Marker Rename",
										" Marker Drag",
										" Marker Swap",
										" Marker Shift",
										" LUA Marker Set",
										" LUA Marker Remove",
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
	backup_bookmarks.resize(0);
	backup_current_branch.resize(0);
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
	backup_bookmarks.resize(history_size);
	backup_current_branch.resize(history_size);
	history_start_pos = 0;
	history_cursor_pos = -1;
	// create initial snapshot
	SNAPSHOT inp;
	inp.init(currMovieData, taseditor_config.enable_hot_changes);
	strcat(inp.description, modCaptions[MODTYPE_INIT]);
	inp.jump_frame = -1;
	inp.start_frame = 0;
	inp.end_frame = inp.size - 1;
	AddItemToHistory(inp);
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
	std::vector<BOOKMARK> new_backup_bookmarks(new_history_size);
	std::vector<int8> new_backup_current_branch(new_history_size);
	int pos = history_cursor_pos, source_pos = history_cursor_pos;
	if (pos >= new_history_size)
		pos = new_history_size - 1;
	int new_history_cursor_pos = pos;
	// copy old "undo" items
	while (pos >= 0)
	{
		new_snapshots[pos] = snapshots[(history_start_pos + source_pos) % history_size];
		new_backup_bookmarks[pos] = backup_bookmarks[(history_start_pos + source_pos) % history_size];
		new_backup_current_branch[pos] = backup_current_branch[(history_start_pos + source_pos) % history_size];
		pos--;
		source_pos--;
	}
	// copy old "redo" items
	int num_redo_items = history_total_items - (history_cursor_pos + 1);
	int space_available = new_history_size - (new_history_cursor_pos + 1);
	int i = (num_redo_items <= space_available) ? num_redo_items : space_available;
	int new_history_total_items = new_history_cursor_pos + i + 1;
	for (; i > 0; i--)
	{
		new_snapshots[new_history_cursor_pos + i] = snapshots[(history_start_pos + history_cursor_pos + i) % history_size];
		new_backup_bookmarks[new_history_cursor_pos + i] = backup_bookmarks[(history_start_pos + history_cursor_pos + i) % history_size];
		new_backup_current_branch[new_history_cursor_pos + i] = backup_current_branch[(history_start_pos + history_cursor_pos + i) % history_size];
	}
	// finish
	snapshots = new_snapshots;
	backup_bookmarks = new_backup_bookmarks;
	backup_current_branch = new_backup_current_branch;
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
	RedrawHistoryList();

	int real_pos, mod_type, slot, current_branch = branches.GetCurrentBranch();
	bool bookmarks_changed = false, changes_since_current_branch = false;
	bool old_changes_since_current_branch = branches.GetChangesSinceCurrentBranch();
	// restore Bookmarks/Branches
	std::vector<uint8> bookmarks_to_redraw;
	std::vector<int> frames_to_redraw;
	if (new_pos > old_pos)
	{
		// redo
		for (int i = old_pos + 1; i <= new_pos; ++i)
		{
			real_pos = (history_start_pos + i) % history_size;
			mod_type = snapshots[real_pos].mod_type;
			if (mod_type >= MODTYPE_BOOKMARK_0 && mod_type <= MODTYPE_BRANCH_MARKERS_9)
			{
				current_branch = (mod_type - MODTYPE_BOOKMARK_0) % TOTAL_BOOKMARKS;
				changes_since_current_branch = false;
			} else
			{
				changes_since_current_branch = true;
			}
			if (mod_type >= MODTYPE_BOOKMARK_0 && mod_type <= MODTYPE_BOOKMARK_9)
			{
				// swap Bookmark and its backup version
				slot = (mod_type - MODTYPE_BOOKMARK_0) % TOTAL_BOOKMARKS;
				BOOKMARK temp_bookmark(bookmarks.bookmarks_array[slot]);
				frames_to_redraw.push_back(bookmarks.bookmarks_array[slot].snapshot.jump_frame);
				bookmarks.bookmarks_array[slot] = backup_bookmarks[real_pos];
				frames_to_redraw.push_back(bookmarks.bookmarks_array[slot].snapshot.jump_frame);
				bookmarks_to_redraw.push_back(slot);
				backup_bookmarks[real_pos] = temp_bookmark;
				branches.InvalidateBranchSlot(slot);
				bookmarks_changed = true;
			}
		}
	} else
	{
		// undo
		for (int i = old_pos; i > new_pos; i--)
		{
			real_pos = (history_start_pos + i) % history_size;
			mod_type = snapshots[real_pos].mod_type;
			if (mod_type >= MODTYPE_BOOKMARK_0 && mod_type <= MODTYPE_BRANCH_MARKERS_9)
				current_branch = backup_current_branch[real_pos];
			if (mod_type >= MODTYPE_BOOKMARK_0 && mod_type <= MODTYPE_BOOKMARK_9)
			{
				// swap Bookmark and its backup version
				slot = (mod_type - MODTYPE_BOOKMARK_0) % TOTAL_BOOKMARKS;
				BOOKMARK temp_bookmark(bookmarks.bookmarks_array[slot]);
				frames_to_redraw.push_back(bookmarks.bookmarks_array[slot].snapshot.jump_frame);
				bookmarks.bookmarks_array[slot] = backup_bookmarks[real_pos];
				frames_to_redraw.push_back(bookmarks.bookmarks_array[slot].snapshot.jump_frame);
				bookmarks_to_redraw.push_back(slot);
				backup_bookmarks[real_pos] = temp_bookmark;
				branches.InvalidateBranchSlot(slot);
				bookmarks_changed = true;
			}
		}
		real_pos = (history_start_pos + new_pos) % history_size;
		mod_type = snapshots[real_pos].mod_type;
		if (mod_type >= MODTYPE_BOOKMARK_0 && mod_type <= MODTYPE_BRANCH_MARKERS_9)
		{
			current_branch = (mod_type - MODTYPE_BOOKMARK_0) % TOTAL_BOOKMARKS;
			changes_since_current_branch = false;
		} else if (GetCategoryOfOperation(mod_type) != CATEGORY_OTHER)
		{
			changes_since_current_branch = true;
		}
	}
	int old_current_branch = branches.GetCurrentBranch();
	if (bookmarks_changed || current_branch != old_current_branch || changes_since_current_branch != old_changes_since_current_branch)
	{
		branches.HandleHistoryJump(current_branch, changes_since_current_branch);
		if (current_branch != old_current_branch)
		{
			// current_branch was switched, redraw Bookmarks List to change the color of digits
			if (old_current_branch != ITEM_UNDER_MOUSE_CLOUD)
			{
				frames_to_redraw.push_back(bookmarks.bookmarks_array[old_current_branch].snapshot.jump_frame);
				bookmarks_to_redraw.push_back(old_current_branch);
			}
			if (current_branch != ITEM_UNDER_MOUSE_CLOUD)
			{
				frames_to_redraw.push_back(bookmarks.bookmarks_array[current_branch].snapshot.jump_frame);
				bookmarks_to_redraw.push_back(current_branch);
			}
		}
		bookmarks.must_check_item_under_mouse = true;
		project.SetProjectChanged();
	}
	// redraw Piano Roll rows and Bookmarks List rows
	for (int i = frames_to_redraw.size() - 1; i >= 0; i--)
		piano_roll.RedrawRow(frames_to_redraw[i]);
	for (int i = bookmarks_to_redraw.size() - 1; i >= 0; i--)
	{
		bookmarks.RedrawBookmark(bookmarks_to_redraw[i]);
		// if screenshot of the slot is currently shown - reinit and redraw the picture
		if (popup_display.screenshot_currently_shown == bookmarks_to_redraw[i])
			popup_display.screenshot_currently_shown = ITEM_UNDER_MOUSE_NONE;
	}

	// create undo_hint
	if (new_pos > old_pos)
		undo_hint_pos = GetCurrentSnapshot().jump_frame;		// redo
	else
		undo_hint_pos = GetNextToCurrentSnapshot().jump_frame;	// undo
	undo_hint_time = clock() + UNDO_HINT_TIME;
	show_undo_hint = true;

	real_pos = (history_start_pos + history_cursor_pos) % history_size;
	// update Markers
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

	// update current movie data
	int first_change = snapshots[real_pos].findFirstChange(currMovieData);
	if (first_change >= 0)
	{
		snapshots[real_pos].toMovie(currMovieData, first_change);
		selection.must_find_current_marker = playback.must_find_current_marker = true;
		// Piano Roll Redraw and ProjectChanged will be called by greenzone invalidation
	} else if (markers_changed)
	{
		markers_manager.update();
		selection.must_find_current_marker = playback.must_find_current_marker = true;
		piano_roll.RedrawList();
		project.SetProjectChanged();
	} else if (taseditor_config.enable_hot_changes)
	{
		// when using Hot Changes, Piano Roll should be always redrawn, because old changes become less hot
		piano_roll.RedrawList();
	}
	piano_roll.UpdateItemCount();
	piano_roll.FollowUndo();
	return first_change;
}

void HISTORY::undo()
{
	int result = jump(history_cursor_pos - 1);
	if (result >= 0)
		greenzone.InvalidateAndCheck(result);
	return;
}
void HISTORY::redo()
{
	int result = jump(history_cursor_pos + 1);
	if (result >= 0)
		greenzone.InvalidateAndCheck(result);
	return;
}
// ----------------------------
void HISTORY::AddItemToHistory(SNAPSHOT &inp, int cur_branch)
{
	// history uses conveyor of items (vector with fixed size) to aviod frequent resizing, which would be awfully expensive with such large objects as SNAPSHOT and BOOKMARK
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
	// write data
	int real_pos = (history_start_pos + history_cursor_pos) % history_size;
	snapshots[real_pos] = inp;
	backup_bookmarks[real_pos].free();
	backup_current_branch[real_pos] = cur_branch;
	RedrawHistoryList();
}
void HISTORY::AddItemToHistory(SNAPSHOT &inp, int cur_branch, BOOKMARK &bookm)
{
	// history uses conveyor of items (vector with fixed size) to aviod frequent resizing, which would be awfully expensive with such large objects as SNAPSHOT and BOOKMARK
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
	// write data
	int real_pos = (history_start_pos + history_cursor_pos) % history_size;
	snapshots[real_pos] = inp;
	backup_bookmarks[real_pos] = bookm;
	backup_current_branch[real_pos] = cur_branch;
	RedrawHistoryList();
}

// returns frame of first actual change
int HISTORY::RegisterChanges(int mod_type, int start, int end, const char* comment, int consecutive_tag)
{
	// create new shanshot
	SNAPSHOT inp;
	inp.init(currMovieData, taseditor_config.enable_hot_changes);
	// check if there are input differences from latest snapshot
	int real_pos = (history_start_pos + history_cursor_pos) % history_size;
	int first_changes = inp.findFirstChange(snapshots[real_pos], start, end);
	if (first_changes >= 0)
	{
		// differences found
		// fill description:
		inp.mod_type = mod_type;
		strcat(inp.description, modCaptions[inp.mod_type]);
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
		inp.start_frame = start;
		inp.end_frame = end;
		inp.consecutive_tag = consecutive_tag;
		if (consecutive_tag && taseditor_config.combine_consecutive && snapshots[real_pos].mod_type == inp.mod_type && snapshots[real_pos].consecutive_tag == inp.consecutive_tag)
		{
			// combine with previous snapshot
			if (inp.jump_frame > snapshots[real_pos].jump_frame)
				inp.jump_frame = snapshots[real_pos].jump_frame;
			if (inp.start_frame > snapshots[real_pos].start_frame)
				inp.start_frame = snapshots[real_pos].start_frame;
			if (inp.end_frame < snapshots[real_pos].end_frame)
				inp.end_frame = snapshots[real_pos].end_frame;
			// add upper and lower frame to description
			char framenum[11];
			strcat(inp.description, " ");
			_itoa(inp.start_frame, framenum, 10);
			strcat(inp.description, framenum);
			if (inp.end_frame > inp.start_frame)
			{
				strcat(inp.description, "-");
				_itoa(inp.end_frame, framenum, 10);
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
				inp.copyHotChanges(&snapshots[real_pos]);
				inp.fillHotChanges(snapshots[real_pos], first_changes, end);
			}
			// replace current snapshot with this cloned snapshot and truncate history here
			snapshots[real_pos] = inp;
			history_total_items = history_cursor_pos+1;
			UpdateHistoryList();
			RedrawHistoryList();
		} else
		{
			// don't combine
			// add upper and lower frame to description
			char framenum[11];
			strcat(inp.description, " ");
			_itoa(inp.start_frame, framenum, 10);
			strcat(inp.description, framenum);
			if (inp.end_frame > inp.start_frame)
			{
				strcat(inp.description, "-");
				_itoa(inp.end_frame, framenum, 10);
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
			AddItemToHistory(inp);
		}
		branches.ChangesMadeSinceBranch();
	}
	return first_changes;
}
int HISTORY::RegisterInsertNum(int start, int frames)
{
	// create new shanshot
	SNAPSHOT inp;
	inp.init(currMovieData, taseditor_config.enable_hot_changes);
	// check if there are input differences from latest snapshot
	int real_pos = (history_start_pos + history_cursor_pos) % history_size;
	int first_changes = inp.findFirstChange(snapshots[real_pos], start);
	if (first_changes >= 0)
	{
		// differences found
		// fill description:
		inp.mod_type = MODTYPE_INSERTNUM;
		strcat(inp.description, modCaptions[inp.mod_type]);
		inp.jump_frame = start;
		inp.start_frame = start;
		inp.end_frame = start + frames - 1;
		char framenum[11];
		// add number of inserted frames to description
		_itoa(frames, framenum, 10);
		strcat(inp.description, framenum);
		// add upper frame to description
		strcat(inp.description, " ");
		_itoa(inp.start_frame, framenum, 10);
		strcat(inp.description, framenum);
		// set hotchanges
		if (taseditor_config.enable_hot_changes)
			inp.inheritHotChanges_InsertNum(&snapshots[real_pos], start, frames);
		AddItemToHistory(inp);
		branches.ChangesMadeSinceBranch();
	}
	return first_changes;
}
int HISTORY::RegisterPasteInsert(int start, SelectionFrames& inserted_set)
{
	// create new shanshot
	SNAPSHOT inp;
	inp.init(currMovieData, taseditor_config.enable_hot_changes);
	// check if there are input differences from latest snapshot
	int real_pos = (history_start_pos + history_cursor_pos) % history_size;
	int first_changes = inp.findFirstChange(snapshots[real_pos], start);
	if (first_changes >= 0)
	{
		// differences found
		// fill description:
		inp.mod_type = MODTYPE_PASTEINSERT;
		strcat(inp.description, modCaptions[inp.mod_type]);
		// for PasteInsert user prefers to see frame of attempted change (selection beginning), not frame of actual differences
		inp.jump_frame = start;
		inp.start_frame = start;
		inp.end_frame = -1;
		// add upper frame to description
		char framenum[11];
		strcat(inp.description, " ");
		_itoa(inp.start_frame, framenum, 10);
		strcat(inp.description, framenum);
		// set hotchanges
		if (taseditor_config.enable_hot_changes)
			inp.inheritHotChanges_PasteInsert(&snapshots[real_pos], inserted_set);
		AddItemToHistory(inp);
		branches.ChangesMadeSinceBranch();
	}
	return first_changes;
}
void HISTORY::RegisterMarkersChange(int mod_type, int start, int end, const char* comment)
{
	// create new shanshot
	SNAPSHOT inp;
	inp.init(currMovieData, taseditor_config.enable_hot_changes);
	// fill description:
	inp.mod_type = mod_type;
	strcat(inp.description, modCaptions[mod_type]);
	inp.jump_frame = start;
	inp.start_frame = start;
	inp.end_frame = end;
	// add the frame to description
	char framenum[11];
	strcat(inp.description, " ");
	_itoa(inp.start_frame, framenum, 10);
	strcat(inp.description, framenum);
	if (inp.end_frame > inp.start_frame || mod_type == MODTYPE_MARKER_DRAG || mod_type == MODTYPE_MARKER_SWAP)
	{
		if (mod_type == MODTYPE_MARKER_DRAG)
			strcat(inp.description, "=>");
		else if (mod_type == MODTYPE_MARKER_SWAP)
			strcat(inp.description, "<=>");
		else
			strcat(inp.description, "-");
		_itoa(inp.end_frame, framenum, 10);
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
	AddItemToHistory(inp);
	branches.ChangesMadeSinceBranch();
	project.SetProjectChanged();
}
void HISTORY::RegisterBookmarkSet(int slot, BOOKMARK& backup_copy, int old_current_branch)
{
	// create new snapshot
	SNAPSHOT inp;
	inp.init(currMovieData, taseditor_config.enable_hot_changes);
	// fill description: modification type + jump_frame of the Bookmark
	inp.mod_type = MODTYPE_BOOKMARK_0 + slot;
	strcat(inp.description, modCaptions[inp.mod_type]);
	inp.start_frame = inp.end_frame = inp.jump_frame = bookmarks.bookmarks_array[slot].snapshot.jump_frame;
	char framenum[11];
	strcat(inp.description, " ");
	_itoa(inp.jump_frame, framenum, 10);
	strcat(inp.description, framenum);
	if (taseditor_config.enable_hot_changes)
		inp.copyHotChanges(&GetCurrentSnapshot());
	AddItemToHistory(inp, old_current_branch, backup_copy);
}
void HISTORY::RegisterBranching(int mod_type, int first_change, int slot, int old_current_branch)
{
	// create new snapshot
	SNAPSHOT inp;
	inp.init(currMovieData, taseditor_config.enable_hot_changes);
	// fill description: modification type + time of the Branch
	inp.mod_type = mod_type;
	strcat(inp.description, modCaptions[inp.mod_type]);
	strcat(inp.description, bookmarks.bookmarks_array[slot].snapshot.description);
	inp.jump_frame = first_change;
	inp.start_frame = first_change;
	inp.end_frame = -1;
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
	AddItemToHistory(inp, old_current_branch);
}
void HISTORY::RegisterRecording(int frame_of_change)
{
	int real_pos = (history_start_pos + history_cursor_pos) % history_size;
	SNAPSHOT inp;
	inp.init(currMovieData, taseditor_config.enable_hot_changes);
	inp.fillJoypadsDiff(snapshots[real_pos], frame_of_change);
	// fill description:
	inp.mod_type = MODTYPE_RECORD;
	strcat(inp.description, modCaptions[MODTYPE_RECORD]);
	char framenum[11];
	// check if current snapshot is also Recording and maybe it is consecutive recording
	if (taseditor_config.combine_consecutive
		&& snapshots[real_pos].mod_type == MODTYPE_RECORD							// a) also Recording
		&& snapshots[real_pos].consecutive_tag == frame_of_change - 1				// b) consecutive (previous frame)
		&& snapshots[real_pos].rec_joypad_diff_bits == inp.rec_joypad_diff_bits)	// c) recorded same set of joysticks
	{
		// clone this snapshot and continue chain of recorded frames
		inp.jump_frame = snapshots[real_pos].jump_frame;
		inp.start_frame = snapshots[real_pos].jump_frame;
		inp.end_frame = frame_of_change;
		inp.consecutive_tag = frame_of_change;
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
		strcat(inp.description, " ");
		_itoa(inp.start_frame, framenum, 10);
		strcat(inp.description, framenum);
		strcat(inp.description, "-");
		_itoa(inp.end_frame, framenum, 10);
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
		inp.jump_frame = inp.start_frame = inp.end_frame = inp.consecutive_tag = frame_of_change;
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
		strcat(inp.description, " ");
		_itoa(frame_of_change, framenum, 10);
		strcat(inp.description, framenum);
		// set hotchanges
		if (taseditor_config.enable_hot_changes)
		{
			inp.inheritHotChanges(&snapshots[real_pos]);
			inp.fillHotChanges(snapshots[real_pos], frame_of_change, frame_of_change);
		}
		AddItemToHistory(inp);
	}
	branches.ChangesMadeSinceBranch();
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
		inp.jump_frame = first_changes;
		inp.start_frame = 0;
		inp.end_frame = inp.size - 1;
		// fill description:
		inp.mod_type = MODTYPE_IMPORT;
		strcat(inp.description, modCaptions[inp.mod_type]);
		// add filename to description
		strcat(inp.description, " ");
		strncat(inp.description, filename, SNAPSHOT_DESC_MAX_LENGTH - strlen(inp.description) - 1);
		if (taseditor_config.enable_hot_changes)
		{
			// do not inherit old hotchanges, because imported input (most likely) doesn't have direct connection with recent edits, so old hotchanges are irrelevant and should not be copied
			inp.fillHotChanges(snapshots[real_pos], first_changes);
		}
		AddItemToHistory(inp);
		inp.toMovie(currMovieData);
		piano_roll.UpdateItemCount();
		branches.ChangesMadeSinceBranch();
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
	// check if there are input differences from latest snapshot
	int real_pos = (history_start_pos + history_cursor_pos) % history_size;
	int first_changes = inp.findFirstChange(snapshots[real_pos], start);
	if (first_changes >= 0)
	{
		// differences found
		// fill description:
		inp.mod_type = MODTYPE_LUA_CHANGE;
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
		inp.start_frame = start;
		inp.end_frame = -1;
		// add upper frame to description
		char framenum[11];
		strcat(inp.description, " ");
		_itoa(first_changes, framenum, 10);
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
		AddItemToHistory(inp);
		branches.ChangesMadeSinceBranch();
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
		// write items starting from history_start_pos
		for (int i = 0; i < history_total_items; ++i)
		{
			real_pos = (history_start_pos + i) % history_size;
			snapshots[real_pos].save(os);
			backup_bookmarks[real_pos].save(os);
			os->fwrite(&backup_current_branch[real_pos], 1);
			playback.SetProgressbar(i, history_total_items);
		}
	} else
	{
		// write "HISTORX" string
		os->fwrite(history_skipsave_id, HISTORY_ID_LEN);
	}
}
// returns true if couldn't load
bool HISTORY::load(EMUFILE *is, bool really_load)
{
	if (!really_load)
	{
		reset();
		return false;
	}
	int i = -1;
	SNAPSHOT inp;
	BOOKMARK bookm;
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
	// delete old items
	snapshots.resize(history_size);
	backup_bookmarks.resize(history_size);
	backup_current_branch.resize(history_size);
	// read vars
	if (!read32le(&history_cursor_pos, is)) goto error;
	if (!read32le(&history_total_items, is)) goto error;
	if (history_cursor_pos > history_total_items) goto error;
	history_start_pos = 0;
	// read items
	int total = history_total_items;
	if (history_total_items > history_size)
	{
		// user can't afford that much undo levels, skip some items
		int num_items_to_skip = history_total_items - history_size;
		// first try to skip items over history_cursor_pos (future items), because "redo" is less important than "undo"
		int num_redo_items = history_total_items-1 - history_cursor_pos;
		if (num_items_to_skip >= num_redo_items)
		{
			// skip all redo items
			history_total_items = history_cursor_pos+1;
			num_items_to_skip -= num_redo_items;
			// and still need to skip some undo items
			for (i = 0; i < num_items_to_skip; ++i)
			{
				if (inp.skipLoad(is)) goto error;
				if (bookm.skipLoad(is)) goto error;
				if (is->fseek(1, SEEK_CUR)) goto error;		// backup_current_branch
			}
			total -= num_items_to_skip;
			history_cursor_pos -= num_items_to_skip;
		}
		history_total_items -= num_items_to_skip;
	}
	// load items
	for (i = 0; i < history_total_items; ++i)
	{
		if (snapshots[i].load(is)) goto error;
		if (backup_bookmarks[i].load(is)) goto error;
		if (is->fread(&backup_current_branch[i], 1) != 1) goto error;
		playback.SetProgressbar(i, history_total_items);
	}
	// skip redo items if needed
	for (; i < total; ++i)
	{
		if (inp.skipLoad(is)) goto error;
		if (bookm.skipLoad(is)) goto error;
		if (is->fseek(1, SEEK_CUR)) goto error;		// backup_current_branch
	}

	// everything went well
	// init vars
	undo_hint_pos = old_undo_hint_pos = undo_hint_time = -1;
	old_show_undo_hint = show_undo_hint = false;
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
			greenzone.InvalidateAndCheck(result);
	}
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
int HISTORY::GetCategoryOfOperation(int mod_type)
{
	switch (mod_type)
	{
		case MODTYPE_INIT:
		case MODTYPE_UNDEFINED:
			return CATEGORY_OTHER;
		case MODTYPE_SET:
		case MODTYPE_UNSET:
		case MODTYPE_PATTERN:
			return CATEGORY_INPUT_CHANGE;
		case MODTYPE_INSERT:
		case MODTYPE_INSERTNUM:
		case MODTYPE_DELETE:
		case MODTYPE_TRUNCATE:
			return CATEGORY_INPUT_MARKERS_CHANGE;
		case MODTYPE_CLEAR:
		case MODTYPE_CUT:
		case MODTYPE_PASTE:
			return CATEGORY_INPUT_CHANGE;
		case MODTYPE_PASTEINSERT:
		case MODTYPE_CLONE:
			return CATEGORY_INPUT_MARKERS_CHANGE;
		case MODTYPE_RECORD:
		case MODTYPE_IMPORT:
			return CATEGORY_INPUT_CHANGE;
		case MODTYPE_BOOKMARK_0:
		case MODTYPE_BOOKMARK_1:
		case MODTYPE_BOOKMARK_2:
		case MODTYPE_BOOKMARK_3:
		case MODTYPE_BOOKMARK_4:
		case MODTYPE_BOOKMARK_5:
		case MODTYPE_BOOKMARK_6:
		case MODTYPE_BOOKMARK_7:
		case MODTYPE_BOOKMARK_8:
		case MODTYPE_BOOKMARK_9:
			return CATEGORY_OTHER;
		case MODTYPE_BRANCH_0:
		case MODTYPE_BRANCH_1:
		case MODTYPE_BRANCH_2:
		case MODTYPE_BRANCH_3:
		case MODTYPE_BRANCH_4:
		case MODTYPE_BRANCH_5:
		case MODTYPE_BRANCH_6:
		case MODTYPE_BRANCH_7:
		case MODTYPE_BRANCH_8:
		case MODTYPE_BRANCH_9:
			return CATEGORY_INPUT_MARKERS_CHANGE;
		case MODTYPE_BRANCH_MARKERS_0:
		case MODTYPE_BRANCH_MARKERS_1:
		case MODTYPE_BRANCH_MARKERS_2:
		case MODTYPE_BRANCH_MARKERS_3:
		case MODTYPE_BRANCH_MARKERS_4:
		case MODTYPE_BRANCH_MARKERS_5:
		case MODTYPE_BRANCH_MARKERS_6:
		case MODTYPE_BRANCH_MARKERS_7:
		case MODTYPE_BRANCH_MARKERS_8:
		case MODTYPE_BRANCH_MARKERS_9:
		case MODTYPE_MARKER_SET:
		case MODTYPE_MARKER_REMOVE:
		case MODTYPE_MARKER_PATTERN:
		case MODTYPE_MARKER_RENAME:
		case MODTYPE_MARKER_DRAG:
		case MODTYPE_MARKER_SWAP:
		case MODTYPE_MARKER_SHIFT:
		case MODTYPE_LUA_MARKER_SET:
		case MODTYPE_LUA_MARKER_REMOVE:
		case MODTYPE_LUA_MARKER_RENAME:
			return CATEGORY_MARKERS_CHANGE;
		case MODTYPE_LUA_CHANGE:
			return CATEGORY_INPUT_MARKERS_CHANGE;
	}
	// if undefined
	return CATEGORY_OTHER;
}

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
		ScreenToClient(hwndHistoryList, &p);
		RECT wrect;
		GetWindowRect(hwndHistoryList, &wrect);
		if (p.x >= 0
			&& p.y >= 0
			&& p.x < (wrect.right - wrect.left)
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


