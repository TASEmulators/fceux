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
* on demand: checks the difference between the last snapshot's Inputlog and current movie Input, and makes a decision to create new point of rollback. In special cases it can create a point of rollback without checking the difference, assuming that caller already checked it
* implements all restoring operations: undo, redo, revert to any snapshot from the array
* also stores the state of "undo pointer"
* regularly updates the state of "undo pointer"
* regularly (when emulator is paused) searches for uncompressed items in the History Log and compresses first found item
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
	ListView_SetExtendedListViewStyleEx(hwndHistoryList, LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES, LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
	// subclass the listview
	hwndHistoryList_oldWndProc = (WNDPROC)SetWindowLong(hwndHistoryList, GWL_WNDPROC, (LONG)HistoryListWndProc);
	LVCOLUMN lvc;
	lvc.mask = LVCF_WIDTH | LVCF_FMT;
	lvc.cx = HISTORY_LIST_WIDTH;
	lvc.fmt = LVCFMT_LEFT;
	ListView_InsertColumn(hwndHistoryList, 0, &lvc);
	// shedule first autocompression
	next_autocompress_time = clock() + TIME_BETWEEN_AUTOCOMPRESSIONS;
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
	SNAPSHOT snap;
	snap.init(currMovieData, taseditor_config.enable_hot_changes);
	snap.mod_type = MODTYPE_INIT;
	strcat(snap.description, modCaptions[snap.mod_type]);
	snap.keyframe = -1;
	snap.start_frame = 0;
	snap.end_frame = snap.inputlog.size - 1;
	AddItemToHistory(snap);
	UpdateHistoryList();
	RedrawHistoryList();
}
void HISTORY::update()
{
	// Update undo_hint
	if (old_undo_hint_pos != undo_hint_pos && old_undo_hint_pos >= 0)
		piano_roll.RedrawRow(old_undo_hint_pos);
	old_undo_hint_pos = undo_hint_pos;
	old_show_undo_hint = show_undo_hint;
	show_undo_hint = false;
	if (undo_hint_pos >= 0)
	{
		if ((int)clock() < undo_hint_time)
			show_undo_hint = true;
		else
			undo_hint_pos = -1;		// finished hinting
	}
	if (old_show_undo_hint != show_undo_hint)
		piano_roll.RedrawRow(undo_hint_pos);

	// When cpu is idle, compress items from time to time
	if (clock() > next_autocompress_time)
	{
		if (FCEUI_EmulationPaused())
		{
			// search for first occurence of an item containing non-compressed snapshot
			int real_pos;
			for (int i = history_total_items - 1; i >= 0; i--)
			{
				real_pos = (history_start_pos + i) % history_size;
				if (!snapshots[real_pos].Get_already_compressed())
				{
					snapshots[real_pos].compress_data();
					break;
				} else if (backup_bookmarks[real_pos].not_empty && backup_bookmarks[real_pos].snapshot.Get_already_compressed())
				{
					backup_bookmarks[real_pos].snapshot.compress_data();
					break;
				}
			}
		}
		next_autocompress_time = clock() + TIME_BETWEEN_AUTOCOMPRESSIONS;
	}
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

// returns frame of first Input change (for Greenzone invalidation)
int HISTORY::JumpInTime(int new_pos)
{
	if (new_pos < 0)
		new_pos = 0;
	else if (new_pos >= history_total_items)
		new_pos = history_total_items - 1;
	// if nothing is done, do not invalidate Greenzone
	if (new_pos == history_cursor_pos)
		return -1;

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
				frames_to_redraw.push_back(bookmarks.bookmarks_array[slot].snapshot.keyframe);
				bookmarks.bookmarks_array[slot] = backup_bookmarks[real_pos];
				frames_to_redraw.push_back(bookmarks.bookmarks_array[slot].snapshot.keyframe);
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
				frames_to_redraw.push_back(bookmarks.bookmarks_array[slot].snapshot.keyframe);
				bookmarks.bookmarks_array[slot] = backup_bookmarks[real_pos];
				frames_to_redraw.push_back(bookmarks.bookmarks_array[slot].snapshot.keyframe);
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
				frames_to_redraw.push_back(bookmarks.bookmarks_array[old_current_branch].snapshot.keyframe);
				bookmarks_to_redraw.push_back(old_current_branch);
			}
			if (current_branch != ITEM_UNDER_MOUSE_CLOUD)
			{
				frames_to_redraw.push_back(bookmarks.bookmarks_array[current_branch].snapshot.keyframe);
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
		undo_hint_pos = GetCurrentSnapshot().keyframe;		// redo
	else
		undo_hint_pos = GetNextToCurrentSnapshot().keyframe;	// undo
	undo_hint_time = clock() + UNDO_HINT_TIME;
	show_undo_hint = true;

	real_pos = (history_start_pos + history_cursor_pos) % history_size;

	// update Markers
	bool markers_changed = false;
	if (snapshots[real_pos].MarkersDifferFromCurrent())
	{
		snapshots[real_pos].copyToMarkers();
		project.SetProjectChanged();
		markers_changed = true;
	}

	// revert current movie data
	int first_changes = snapshots[real_pos].inputlog.findFirstChange(currMovieData);
	if (first_changes >= 0)
	{
		snapshots[real_pos].inputlog.toMovie(currMovieData, first_changes);
		if (markers_changed)
			markers_manager.update();
		selection.must_find_current_marker = playback.must_find_current_marker = true;
		project.SetProjectChanged();
	} else if (markers_changed)
	{
		markers_manager.update();
		selection.must_find_current_marker = playback.must_find_current_marker = true;
		project.SetProjectChanged();
	}

	// revert Greenzone's LagLog
	// but if Greenzone's LagLog has the same data + more lag data of the same timeline, then don't revert but truncate
	int first_lag_changes = greenzone.laglog.findFirstChange(snapshots[real_pos].laglog);
	int greenzone_log_size = greenzone.laglog.GetSize();
	int new_log_size = snapshots[real_pos].laglog.GetSize();
	if ((first_lag_changes < 0 || (first_lag_changes > new_log_size && first_lag_changes > greenzone_log_size)) && greenzone_log_size > new_log_size)
	{
		if (first_changes >= 0 && (first_lag_changes > first_changes || first_lag_changes < 0))
			// truncate after the timeline starts to differ
			first_lag_changes = first_changes;
		greenzone.laglog.InvalidateFrom(first_lag_changes);
	} else
	{
		greenzone.laglog = snapshots[real_pos].laglog;
	}

	piano_roll.UpdateItemCount();
	piano_roll.FollowUndo();
	piano_roll.RedrawList();	// even though the Greenzone invalidation most likely will also sent the command to redraw

	// Greenzone should be invalidated after the frame of Lag changes if this frame is less than the frame of Input changes
	if (first_lag_changes >= 0 && (first_changes > first_lag_changes || first_changes < 0))
		first_changes = first_lag_changes;
	return first_changes;
}

void HISTORY::undo()
{
	int result = JumpInTime(history_cursor_pos - 1);
	if (result >= 0)
		greenzone.InvalidateAndCheck(result);
	return;
}
void HISTORY::redo()
{
	int result = JumpInTime(history_cursor_pos + 1);
	if (result >= 0)
		greenzone.InvalidateAndCheck(result);
	return;
}
// ----------------------------
void HISTORY::AddItemToHistory(SNAPSHOT &snap, int cur_branch)
{
	// history uses conveyor of items (vector with fixed size) to aviod frequent reallocations caused by vector resizing, which would be awfully expensive with such large objects as SNAPSHOT and BOOKMARK
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
	snapshots[real_pos] = snap;
	backup_bookmarks[real_pos].free();
	backup_current_branch[real_pos] = cur_branch;
	RedrawHistoryList();
}
void HISTORY::AddItemToHistory(SNAPSHOT &snap, int cur_branch, BOOKMARK &bookm)
{
	// history uses conveyor of items (vector with fixed size) to aviod frequent reallocations caused by vector resizing, which would be awfully expensive with such large objects as SNAPSHOT and BOOKMARK
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
	snapshots[real_pos] = snap;
	backup_bookmarks[real_pos] = bookm;
	backup_current_branch[real_pos] = cur_branch;
	RedrawHistoryList();
}
// --------------------------------------------------------------------
// Here goes the set of functions that register project changes and log them into History log

// returns frame of first actual change
int HISTORY::RegisterChanges(int mod_type, int start, int end, int size, const char* comment, int consecutive_tag, SelectionFrames* frameset)
{
	// create new shanshot
	SNAPSHOT snap;
	snap.init(currMovieData, taseditor_config.enable_hot_changes);
	// check if there are Input differences from latest snapshot
	int real_pos = (history_start_pos + history_cursor_pos) % history_size;
	int first_changes = snap.inputlog.findFirstChange(snapshots[real_pos].inputlog, start, end);
	// for lag-affecting operations only:
	// Greenzone should be invalidated after the frame of Lag changes if this frame is less than the frame of Input changes
	int first_lag_changes = -1;
	if (end == -1)
		first_lag_changes = snap.laglog.findFirstChange(snapshots[real_pos].laglog);

	if (first_changes >= 0)
	{
		// differences found
		char framenum[11];
		// fill description:
		snap.mod_type = mod_type;
		strcat(snap.description, modCaptions[snap.mod_type]);
		// set keyframe
		switch (mod_type)
		{
			case MODTYPE_SET:
			case MODTYPE_UNSET:
			case MODTYPE_TRUNCATE:
			case MODTYPE_CLEAR:
			case MODTYPE_CUT:
			{
				snap.keyframe = first_changes;
				break;
			}
			case MODTYPE_INSERT:
			case MODTYPE_INSERTNUM:
			case MODTYPE_PASTEINSERT:
			case MODTYPE_PASTE:
			case MODTYPE_CLONE:
			case MODTYPE_DELETE:
			case MODTYPE_PATTERN:
			{
				// for these changes user prefers to see frame of attempted change (Selection cursor position), not frame of actual differences
				snap.keyframe = start;
				break;
			}
		}
		// set start_frame, end_frame, consecutive_tag
		// normal operations
		snap.start_frame = start;
		if (mod_type == MODTYPE_INSERTNUM)
		{
			snap.end_frame = start + size - 1;
			_itoa(size, framenum, 10);
			strcat(snap.description, framenum);
		} else
		{
			snap.end_frame = end;
		}
		snap.consecutive_tag = consecutive_tag;
		if (consecutive_tag && taseditor_config.combine_consecutive && snapshots[real_pos].mod_type == snap.mod_type && snapshots[real_pos].consecutive_tag == snap.consecutive_tag)
		{
			// combine Drawing with previous snapshot
			if (snap.keyframe > snapshots[real_pos].keyframe)
				snap.keyframe = snapshots[real_pos].keyframe;
			if (snap.start_frame > snapshots[real_pos].start_frame)
				snap.start_frame = snapshots[real_pos].start_frame;
			if (snap.end_frame < snapshots[real_pos].end_frame)
				snap.end_frame = snapshots[real_pos].end_frame;
			// add upper and lower frame to description
			strcat(snap.description, " ");
			_itoa(snap.start_frame, framenum, 10);
			strcat(snap.description, framenum);
			if (snap.end_frame > snap.start_frame)
			{
				strcat(snap.description, "-");
				_itoa(snap.end_frame, framenum, 10);
				strcat(snap.description, framenum);
			}
			// add comment if there is one specified
			if (comment)
			{
				strcat(snap.description, " ");
				strncat(snap.description, comment, SNAPSHOT_DESC_MAX_LENGTH - strlen(snap.description) - 1);
			}
			// set hotchanges
			if (taseditor_config.enable_hot_changes)
			{
				snap.inputlog.copyHotChanges(&snapshots[real_pos].inputlog);
				snap.inputlog.fillHotChanges(snapshots[real_pos].inputlog, first_changes, end);
			}
			// replace current snapshot with this cloned snapshot and truncate history here
			snapshots[real_pos] = snap;
			history_total_items = history_cursor_pos+1;
			UpdateHistoryList();
			RedrawHistoryList();
		} else
		{
			// don't combine
			// add upper and lower frame to description
			strcat(snap.description, " ");
			_itoa(snap.start_frame, framenum, 10);
			strcat(snap.description, framenum);
			if (snap.end_frame > snap.start_frame)
			{
				strcat(snap.description, "-");
				_itoa(snap.end_frame, framenum, 10);
				strcat(snap.description, framenum);
			}
			// add comment if there is one specified
			if (comment)
			{
				strcat(snap.description, " ");
				strncat(snap.description, comment, SNAPSHOT_DESC_MAX_LENGTH - strlen(snap.description) - 1);
			}
			// set hotchanges
			if (taseditor_config.enable_hot_changes)
			{
				// inherit previous hotchanges and set new changes
				switch (mod_type)
				{
					case MODTYPE_DELETE:
						snap.inputlog.inheritHotChanges_DeleteSelection(&snapshots[real_pos].inputlog, frameset);
						break;
					case MODTYPE_INSERT:
					case MODTYPE_CLONE:
						snap.inputlog.inheritHotChanges_InsertSelection(&snapshots[real_pos].inputlog, frameset);
						break;
					case MODTYPE_INSERTNUM:
						snap.inputlog.inheritHotChanges_InsertNum(&snapshots[real_pos].inputlog, start, size, true);
						break;
					case MODTYPE_SET:
					case MODTYPE_UNSET:
					case MODTYPE_CLEAR:
					case MODTYPE_CUT:
					case MODTYPE_PASTE:
					case MODTYPE_PATTERN:
						snap.inputlog.inheritHotChanges(&snapshots[real_pos].inputlog);
						snap.inputlog.fillHotChanges(snapshots[real_pos].inputlog, first_changes, end);
						break;
					case MODTYPE_PASTEINSERT:
						snap.inputlog.inheritHotChanges_PasteInsert(&snapshots[real_pos].inputlog, frameset);
						break;
					case MODTYPE_TRUNCATE:
						snap.inputlog.copyHotChanges(&snapshots[real_pos].inputlog);
						// do not add new hotchanges and do not fade old hotchanges, because there was nothing added
						break;
				}
			}
			AddItemToHistory(snap);
		}
		branches.ChangesMadeSinceBranch();
		project.SetProjectChanged();
	}
	if (first_lag_changes >= 0 && (first_changes > first_lag_changes || first_changes < 0))
		first_changes = first_lag_changes;
	return first_changes;
}
int HISTORY::RegisterAdjustLag(int start, int size)
{
	// create new shanshot
	SNAPSHOT snap;
	snap.init(currMovieData, taseditor_config.enable_hot_changes);
	// check if there are Input differences from latest snapshot
	int real_pos = (history_start_pos + history_cursor_pos) % history_size;
	SNAPSHOT& current_snap = snapshots[real_pos];
	int first_changes = snap.inputlog.findFirstChange(current_snap.inputlog, start, -1);
	if (first_changes >= 0)
	{
		// differences found - combine Adjustment with current snapshot
		// copy all properties of current snapshot
		snap.keyframe = current_snap.keyframe;
		snap.start_frame = current_snap.start_frame;
		snap.end_frame = current_snap.end_frame;
		snap.consecutive_tag = current_snap.consecutive_tag;
		//if (current_snap.mod_type == MODTYPE_RECORD && size < 0 && current_snap.consecutive_tag == first_changes) snap.consecutive_tag--;		// make sure that consecutive Recordings work even when there's AdjustUp inbetween
		snap.rec_joypad_diff_bits = current_snap.rec_joypad_diff_bits;
		snap.mod_type = current_snap.mod_type;
		strcpy(snap.description, current_snap.description);
		// set hotchanges
		if (taseditor_config.enable_hot_changes)
		{
			if (size < 0)
				// it was Adjust Up
				snap.inputlog.inheritHotChanges_DeleteNum(&snapshots[real_pos].inputlog, start, 0 - size, false);
			else
				// it was Adjust Down
				snap.inputlog.inheritHotChanges_InsertNum(&snapshots[real_pos].inputlog, start, 1, false);
		}
		// replace current snapshot with this cloned snapshot and don't truncate history
		snapshots[real_pos] = snap;
		UpdateHistoryList();
		RedrawHistoryList();
		branches.ChangesMadeSinceBranch();
		project.SetProjectChanged();
	}
	return first_changes;
}
void HISTORY::RegisterMarkersChange(int mod_type, int start, int end, const char* comment)
{
	// create new shanshot
	SNAPSHOT snap;
	snap.init(currMovieData, taseditor_config.enable_hot_changes);
	// fill description:
	snap.mod_type = mod_type;
	strcat(snap.description, modCaptions[mod_type]);
	snap.keyframe = start;
	snap.start_frame = start;
	snap.end_frame = end;
	// add the frame to description
	char framenum[11];
	strcat(snap.description, " ");
	_itoa(snap.start_frame, framenum, 10);
	strcat(snap.description, framenum);
	if (snap.end_frame > snap.start_frame || mod_type == MODTYPE_MARKER_DRAG || mod_type == MODTYPE_MARKER_SWAP)
	{
		if (mod_type == MODTYPE_MARKER_DRAG)
			strcat(snap.description, "=>");
		else if (mod_type == MODTYPE_MARKER_SWAP)
			strcat(snap.description, "<=>");
		else
			strcat(snap.description, "-");
		_itoa(snap.end_frame, framenum, 10);
		strcat(snap.description, framenum);
	}
	// add comment if there is one specified
	if (comment)
	{
		strcat(snap.description, " ");
		strncat(snap.description, comment, SNAPSHOT_DESC_MAX_LENGTH - strlen(snap.description) - 1);
	}
	// Hotchanges aren't changed
	if (taseditor_config.enable_hot_changes)
		snap.inputlog.copyHotChanges(&GetCurrentSnapshot().inputlog);
	AddItemToHistory(snap);
	branches.ChangesMadeSinceBranch();
	project.SetProjectChanged();
}
void HISTORY::RegisterBookmarkSet(int slot, BOOKMARK& backup_copy, int old_current_branch)
{
	// create new snapshot
	SNAPSHOT snap;
	snap.init(currMovieData, taseditor_config.enable_hot_changes);
	// fill description: modification type + keyframe of the Bookmark
	snap.mod_type = MODTYPE_BOOKMARK_0 + slot;
	strcat(snap.description, modCaptions[snap.mod_type]);
	snap.start_frame = snap.end_frame = snap.keyframe = bookmarks.bookmarks_array[slot].snapshot.keyframe;
	char framenum[11];
	strcat(snap.description, " ");
	_itoa(snap.keyframe, framenum, 10);
	strcat(snap.description, framenum);
	if (taseditor_config.enable_hot_changes)
		snap.inputlog.copyHotChanges(&GetCurrentSnapshot().inputlog);
	AddItemToHistory(snap, old_current_branch, backup_copy);
	project.SetProjectChanged();
}
int HISTORY::RegisterBranching(int slot, bool markers_changed)
{
	// create new snapshot
	SNAPSHOT snap;
	snap.init(currMovieData, taseditor_config.enable_hot_changes);
	// check if there are Input differences from latest snapshot
	int real_pos = (history_start_pos + history_cursor_pos) % history_size;
	int first_changes = snap.inputlog.findFirstChange(snapshots[real_pos].inputlog);
	if (first_changes >= 0)
	{
		// differences found
		// fill description: modification type + time of the Branch
		snap.mod_type = MODTYPE_BRANCH_0 + slot;
		strcat(snap.description, modCaptions[snap.mod_type]);
		strcat(snap.description, bookmarks.bookmarks_array[slot].snapshot.description);
		snap.keyframe = first_changes;
		snap.start_frame = first_changes;
		snap.end_frame = -1;
		if (taseditor_config.enable_hot_changes)
			// copy hotchanges of the Branch
			snap.inputlog.copyHotChanges(&bookmarks.bookmarks_array[slot].snapshot.inputlog);
		AddItemToHistory(snap, branches.GetCurrentBranch());
		project.SetProjectChanged();
	} else if (markers_changed)
	{
		// fill description: modification type + time of the Branch
		snap.mod_type = MODTYPE_BRANCH_MARKERS_0 + slot;
		strcat(snap.description, modCaptions[snap.mod_type]);
		strcat(snap.description, bookmarks.bookmarks_array[slot].snapshot.description);
		snap.keyframe = bookmarks.bookmarks_array[slot].snapshot.keyframe;
		snap.start_frame = 0;
		snap.end_frame = -1;
		// Input was not changed, only Markers were changed
		if (taseditor_config.enable_hot_changes)
			snap.inputlog.copyHotChanges(&GetCurrentSnapshot().inputlog);
		AddItemToHistory(snap, branches.GetCurrentBranch());
		project.SetProjectChanged();
	}
	// revert Greenzone's LagLog (and snap's LagLog too) to bookmarked state
	// but if Greenzone's LagLog has the same data + more lag data of the same timeline, then don't revert but truncate
	int first_lag_changes = greenzone.laglog.findFirstChange(bookmarks.bookmarks_array[slot].snapshot.laglog);
	int greenzone_log_size = greenzone.laglog.GetSize();
	int bookmarked_log_size = bookmarks.bookmarks_array[slot].snapshot.laglog.GetSize();
	if ((first_lag_changes < 0 || (first_lag_changes > bookmarked_log_size && first_lag_changes > greenzone_log_size)) && greenzone_log_size > bookmarked_log_size)
	{
		if (first_changes >= 0 && (first_lag_changes > first_changes || first_lag_changes < 0))
			// truncate after the timeline starts to differ
			first_lag_changes = first_changes;
		greenzone.laglog.InvalidateFrom(first_lag_changes);
		snap.laglog.InvalidateFrom(first_lag_changes);
	} else
	{
		greenzone.laglog = bookmarks.bookmarks_array[slot].snapshot.laglog;
		snap.laglog = greenzone.laglog;
	}
	// Greenzone should be invalidated after the frame of Lag changes if this frame is less than the frame of Input changes
	if (first_lag_changes >= 0 && (first_changes > first_lag_changes || first_changes < 0))
		first_changes = first_lag_changes;
	return first_changes;
}
void HISTORY::RegisterRecording(int frame_of_change)
{
	int real_pos = (history_start_pos + history_cursor_pos) % history_size;
	SNAPSHOT snap;
	snap.init(currMovieData, taseditor_config.enable_hot_changes);
	snap.rec_joypad_diff_bits = snap.inputlog.fillJoypadsDiff(snapshots[real_pos].inputlog, frame_of_change);
	// fill description:
	snap.mod_type = MODTYPE_RECORD;
	strcat(snap.description, modCaptions[MODTYPE_RECORD]);
	char framenum[11];
	// check if current snapshot is also Recording and maybe it is consecutive recording
	if (taseditor_config.combine_consecutive
		&& snapshots[real_pos].mod_type == MODTYPE_RECORD							// a) also Recording
		&& snapshots[real_pos].consecutive_tag == frame_of_change - 1				// b) consecutive (previous frame)
		&& snapshots[real_pos].rec_joypad_diff_bits == snap.rec_joypad_diff_bits)	// c) recorded same set of joysticks
	{
		// clone this snapshot and continue chain of recorded frames
		snap.keyframe = snapshots[real_pos].keyframe;
		snap.start_frame = snapshots[real_pos].keyframe;
		snap.end_frame = frame_of_change;
		snap.consecutive_tag = frame_of_change;
		// add info which joypads were affected
		int num = joysticks_per_frame[snap.inputlog.input_type];
		uint32 current_mask = 1;
		for (int i = 0; i < num; ++i)
		{
			if ((snap.rec_joypad_diff_bits & current_mask))
				strcat(snap.description, joypadCaptions[i]);
			current_mask <<= 1;
		}
		// add upper and lower frame to description
		strcat(snap.description, " ");
		_itoa(snap.start_frame, framenum, 10);
		strcat(snap.description, framenum);
		strcat(snap.description, "-");
		_itoa(snap.end_frame, framenum, 10);
		strcat(snap.description, framenum);
		// set hotchanges
		if (taseditor_config.enable_hot_changes)
		{
			snap.inputlog.copyHotChanges(&snapshots[real_pos].inputlog);
			snap.inputlog.fillHotChanges(snapshots[real_pos].inputlog, frame_of_change, frame_of_change);
		}
		// replace current snapshot with this cloned snapshot and truncate history here
		snapshots[real_pos] = snap;
		history_total_items = history_cursor_pos+1;
		UpdateHistoryList();
		RedrawHistoryList();
	} else
	{
		// not consecutive - add new snapshot to history
		snap.keyframe = snap.start_frame = snap.end_frame = snap.consecutive_tag = frame_of_change;
		// add info which joypads were affected
		int num = joysticks_per_frame[snap.inputlog.input_type];
		uint32 current_mask = 1;
		for (int i = 0; i < num; ++i)
		{
			if ((snap.rec_joypad_diff_bits & current_mask))
				strcat(snap.description, joypadCaptions[i]);
			current_mask <<= 1;
		}
		// add upper frame to description
		strcat(snap.description, " ");
		_itoa(frame_of_change, framenum, 10);
		strcat(snap.description, framenum);
		// set hotchanges
		if (taseditor_config.enable_hot_changes)
		{
			snap.inputlog.inheritHotChanges(&snapshots[real_pos].inputlog);
			snap.inputlog.fillHotChanges(snapshots[real_pos].inputlog, frame_of_change, frame_of_change);
		}
		AddItemToHistory(snap);
	}
	branches.ChangesMadeSinceBranch();
	project.SetProjectChanged();
}
int HISTORY::RegisterImport(MovieData& md, char* filename)
{
	// create new snapshot
	SNAPSHOT snap;
	snap.init(md, taseditor_config.enable_hot_changes, GetInputType(currMovieData));
	// check if there are Input differences from latest snapshot
	int real_pos = (history_start_pos + history_cursor_pos) % history_size;
	int first_changes = snap.inputlog.findFirstChange(snapshots[real_pos].inputlog);
	if (first_changes >= 0)
	{
		// differences found
		snap.keyframe = first_changes;
		snap.start_frame = 0;
		snap.end_frame = snap.inputlog.size - 1;
		// fill description:
		snap.mod_type = MODTYPE_IMPORT;
		strcat(snap.description, modCaptions[snap.mod_type]);
		// add filename to description
		strcat(snap.description, " ");
		strncat(snap.description, filename, SNAPSHOT_DESC_MAX_LENGTH - strlen(snap.description) - 1);
		if (taseditor_config.enable_hot_changes)
		{
			// do not inherit old hotchanges, because imported Input (most likely) doesn't have direct connection with recent edits, so old hotchanges are irrelevant and should not be copied
			snap.inputlog.fillHotChanges(snapshots[real_pos].inputlog, first_changes);
		}
		AddItemToHistory(snap);
		// Replace current movie data with this snapshot's InputLog, not changing Greenzone's LagLog
		snap.inputlog.toMovie(currMovieData);
		piano_roll.UpdateItemCount();
		branches.ChangesMadeSinceBranch();
		project.SetProjectChanged();
	}
	return first_changes;
}
int HISTORY::RegisterLuaChanges(const char* name, int start, bool InsertionDeletion_was_made)
{
	// create new shanshot
	SNAPSHOT snap;
	snap.init(currMovieData, taseditor_config.enable_hot_changes);
	// check if there are Input differences from latest snapshot
	int real_pos = (history_start_pos + history_cursor_pos) % history_size;
	int first_changes = snap.inputlog.findFirstChange(snapshots[real_pos].inputlog, start);
	// for lag-affecting operations only:
	// Greenzone should be invalidated after the frame of Lag changes if this frame is less than the frame of Input changes
	int first_lag_changes = -1;
	if (InsertionDeletion_was_made)
		first_lag_changes = snap.laglog.findFirstChange(snapshots[real_pos].laglog);

	if (first_changes >= 0)
	{
		// differences found
		// fill description:
		snap.mod_type = MODTYPE_LUA_CHANGE;
		if (name[0])
		{
			// user provided custom name of operation
			strcat(snap.description, LuaCaptionPrefix);
			strncat(snap.description, name, LUACHANGES_NAME_MAX_LEN);
		} else
		{
			// set default name
			strcat(snap.description, modCaptions[snap.mod_type]);
		}
		snap.keyframe = first_changes;
		snap.start_frame = start;
		snap.end_frame = -1;
		// add upper frame to description
		char framenum[11];
		strcat(snap.description, " ");
		_itoa(first_changes, framenum, 10);
		strcat(snap.description, framenum);
		// set hotchanges
		if (taseditor_config.enable_hot_changes)
		{
			if (InsertionDeletion_was_made)
			{
				// do it hard way: take old hot_changes and insert/delete rows to create a snapshot that is comparable to the snap
				if (snap.inputlog.input_type == snapshots[real_pos].inputlog.input_type)
				{
					// create temp copy of current snapshot (we need it as a container for hot_changes)
					SNAPSHOT hotchanges_snapshot = snapshots[real_pos];
					if (hotchanges_snapshot.inputlog.has_hot_changes)
					{
						hotchanges_snapshot.inputlog.FadeHotChanges();
					} else
					{
						hotchanges_snapshot.inputlog.has_hot_changes = true;
						hotchanges_snapshot.inputlog.Init_HotChanges();
					}
					// insert/delete frames in hotchanges_snapshot, so that it will be the same size as the snap
					taseditor_lua.InsertDelete_rows_to_Snaphot(hotchanges_snapshot);
					snap.inputlog.copyHotChanges(&hotchanges_snapshot.inputlog);
					snap.inputlog.fillHotChanges(hotchanges_snapshot.inputlog, first_changes);
				}
			} else
			{
				// easy way: snap.size is equal to currentsnapshot.size, so we can simply inherit hotchanges
				snap.inputlog.inheritHotChanges(&snapshots[real_pos].inputlog);
				snap.inputlog.fillHotChanges(snapshots[real_pos].inputlog, first_changes);
			}
		}
		AddItemToHistory(snap);
		branches.ChangesMadeSinceBranch();
		project.SetProjectChanged();
	}
	if (first_lag_changes >= 0 && (first_changes > first_lag_changes || first_changes < 0))
		first_changes = first_lag_changes;
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
			if (i / SAVING_HISTORY_PROGRESSBAR_UPDATE_RATE > last_tick)
			{
				playback.SetProgressbar(i, history_total_items);
				last_tick = i / PROGRESSBAR_UPDATE_RATE;
			}
		}
	} else
	{
		// write "HISTORX" string
		os->fwrite(history_skipsave_id, HISTORY_ID_LEN);
	}
}
// returns true if couldn't load
bool HISTORY::load(EMUFILE *is, unsigned int offset)
{
	int i = -1;
	SNAPSHOT snap;
	BOOKMARK bookm;

	if (offset)
	{
		if (is->fseek(offset, SEEK_SET)) goto error;
	} else
	{
		reset();
		return false;
	}
	// read "HISTORY" string
	char save_id[HISTORY_ID_LEN];
	if ((int)is->fread(save_id, HISTORY_ID_LEN) < HISTORY_ID_LEN) goto error;
	if (!strcmp(history_skipsave_id, save_id))
	{
		// string says to skip loading History
		FCEU_printf("No History in the file\n");
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
				if (snap.skipLoad(is)) goto error;
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
		if (snap.skipLoad(is)) goto error;
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
	FCEU_printf("Error loading History\n");
	reset();
	return true;
}
// ----------------------------
void HISTORY::GetDispInfo(NMLVDISPINFO* nmlvDispInfo)
{
	LVITEM& item = nmlvDispInfo->item;
	if (item.mask & LVIF_TEXT)
		strcpy(item.pszText, GetItemDesc(item.iItem));
}

LONG HISTORY::CustomDraw(NMLVCUSTOMDRAW* msg)
{
	switch(msg->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;
	case CDDS_ITEMPREPAINT:
	{
		msg->clrText = HISTORY_NORMAL_COLOR;
		msg->clrTextBk = HISTORY_NORMAL_BG_COLOR;
		// if this row is not "current History item" then check if it's "related" to current
		int row = msg->nmcd.dwItemSpec;
		if (row != history_cursor_pos)
		{
			int current_start_frame = snapshots[(history_start_pos + history_cursor_pos) % history_size].start_frame;
			int current_end_frame = snapshots[(history_start_pos + history_cursor_pos) % history_size].end_frame;
			int row_start_frame = snapshots[(history_start_pos + row) % history_size].start_frame;
			int row_end_frame = snapshots[(history_start_pos + row) % history_size].end_frame;
			if (current_end_frame >= 0)
			{
				if (row_end_frame >= 0)
				{
					// both items have defined ends, check if they intersect
					if (row_start_frame <= current_end_frame && row_end_frame >= current_start_frame)
						msg->clrTextBk = HISTORY_RELATED_BG_COLOR;
				} else
				{
					// current item has defined end, check if the row item falls into the segment
					if (row_start_frame >= current_start_frame && row_start_frame <= current_end_frame)
						msg->clrTextBk = HISTORY_RELATED_BG_COLOR;
				}
			} else
			{
				if (row_end_frame >= 0)
				{
					// row item has defined end, check if current item falls into the segment
					if (current_start_frame >= row_start_frame && current_start_frame <= row_end_frame)
						msg->clrTextBk = HISTORY_RELATED_BG_COLOR;
				} else
				{
					// both items don't have defined ends, check if they are at the same frame
					if (row_start_frame == current_start_frame)
						msg->clrTextBk = HISTORY_RELATED_BG_COLOR;
				}
			}
		}
	}
	default:
		return CDRF_DODEFAULT;
	}

}

void HISTORY::Click(int row_index)
{
	// jump in time to pointed item
	if (row_index >= 0)
	{
		int result = JumpInTime(row_index);
		if (result >= 0)
			greenzone.InvalidateAndCheck(result);
	}
}

void HISTORY::UpdateHistoryList()
{
	//update the number of items in the history list
	int currLVItemCount = ListView_GetItemCount(hwndHistoryList);
	if (currLVItemCount != history_total_items)
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
		// return current snapshot
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


