/* ---------------------------------------------------------------------------------
Implementation file of Bookmarks class
Copyright (c) 2011-2012 AnS

(The MIT License)
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------------
Bookmarks/Branches - Manager of Bookmarks
[Singleton]

* stores 10 Bookmarks
* implements all operations with Bookmarks: initialization, setting Bookmarks, jumping to Bookmarks, deploying Branches
* saves and loads the data from a project file. On error: resets all Bookmarks and Branches
* implements the working of Bookmarks List: creating, redrawing, mouseover, clicks
* regularly updates flashings in Bookmarks List
* on demand: updates colors of rows in Bookmarks List, reflecting conditions of respective Piano Roll rows
* stores resources: save id, ids of commands, captions for panel, gradients for flashings, id of default slot
------------------------------------------------------------------------------------ */

#include "taseditor_project.h"
#include "utils/xstring.h"
#include "zlib.h"

#pragma comment(lib, "msimg32.lib")

LRESULT APIENTRY BookmarksListWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
WNDPROC hwndBookmarksList_oldWndProc;

extern TASEDITOR_CONFIG taseditor_config;
extern TASEDITOR_WINDOW taseditor_window;
extern POPUP_DISPLAY popup_display;
extern PLAYBACK playback;
extern RECORDER recorder;
extern SELECTION selection;
extern GREENZONE greenzone;
extern TASEDITOR_PROJECT project;
extern HISTORY history;
extern PIANO_ROLL piano_roll;
extern MARKERS_MANAGER markers_manager;
extern BRANCHES branches;

// resources
char bookmarks_save_id[BOOKMARKS_ID_LEN] = "BOOKMARKS";
char bookmarks_skipsave_id[BOOKMARKS_ID_LEN] = "BOOKMARKX";
char bookmarksCaption[3][23] = { " Bookmarks ", " Bookmarks / Branches ", " Branches " };
// color tables for flashing when saving/loading bookmarks
COLORREF bookmark_flash_colors[TOTAL_COMMANDS][FLASH_PHASE_MAX+1] = {
	// set
	//0x122330, 0x1b3541, 0x254753, 0x2e5964, 0x376b75, 0x417e87, 0x4a8f97, 0x53a1a8, 0x5db3b9, 0x66c5cb, 0x70d7dc, 0x79e9ed, 
	0x0d1241, 0x111853, 0x161e64, 0x1a2575, 0x1f2b87, 0x233197, 0x2837a8, 0x2c3db9, 0x3144cb, 0x354adc, 0x3a50ed, 0x3f57ff, 
	// jump
	0x14350f, 0x1c480f, 0x235a0f, 0x2a6c0f, 0x317f10, 0x38910f, 0x3fa30f, 0x46b50f, 0x4dc80f, 0x54da0f, 0x5bec0f, 0x63ff10, 
	// deploy
	0x43171d, 0x541d21, 0x652325, 0x762929, 0x872f2c, 0x983530, 0xa93b34, 0xba4137, 0xcb463b, 0xdc4c3f, 0xed5243, 0xff5947 };

BOOKMARKS::BOOKMARKS()
{
	// fill TrackMouseEvent struct
	tme.cbSize = sizeof(tme);
	tme.dwFlags = TME_LEAVE;
	tme.hwndTrack = NULL;
	list_tme.cbSize = sizeof(tme);
	list_tme.dwFlags = TME_LEAVE;
	list_tme.hwndTrack = NULL;
}

void BOOKMARKS::init()
{
	free();
	hwndBookmarksList = GetDlgItem(taseditor_window.hwndTasEditor, IDC_BOOKMARKSLIST);
	hwndBranchesBitmap = GetDlgItem(taseditor_window.hwndTasEditor, IDC_BRANCHES_BITMAP);
	hwndBookmarks = GetDlgItem(taseditor_window.hwndTasEditor, IDC_BOOKMARKS_BOX);

	// prepare bookmarks listview
	ListView_SetExtendedListViewStyleEx(hwndBookmarksList, LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES, LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
	// subclass the listview
	hwndBookmarksList_oldWndProc = (WNDPROC)SetWindowLong(hwndBookmarksList, GWL_WNDPROC, (LONG)BookmarksListWndProc);
	// setup images for the listview
	himglist = ImageList_Create(11, 13, ILC_COLOR8 | ILC_MASK, 1, 1);
	HBITMAP bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP0));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP1));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP2));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP3));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP4));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP5));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP6));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP7));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP8));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP9));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP10));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP11));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP12));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP13));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP14));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP15));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP16));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP17));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP18));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP19));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP_SELECTED0));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP_SELECTED1));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP_SELECTED2));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP_SELECTED3));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP_SELECTED4));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP_SELECTED5));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP_SELECTED6));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP_SELECTED7));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP_SELECTED8));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP_SELECTED9));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP_SELECTED10));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP_SELECTED11));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP_SELECTED12));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP_SELECTED13));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP_SELECTED14));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP_SELECTED15));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP_SELECTED16));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP_SELECTED17));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP_SELECTED18));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BITMAP_SELECTED19));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	ListView_SetImageList(hwndBookmarksList, himglist, LVSIL_SMALL);
	// setup columns
	LVCOLUMN lvc;
	// icons column
	lvc.mask = LVCF_WIDTH;
	lvc.cx = BOOKMARKSLIST_COLUMN_ICONS_WIDTH;
	ListView_InsertColumn(hwndBookmarksList, 0, &lvc);
	// keyframe column
	lvc.mask = LVCF_WIDTH | LVCF_FMT;
	lvc.fmt = LVCFMT_CENTER;
	lvc.cx = BOOKMARKSLIST_COLUMN_FRAMENUM_WIDTH;
	ListView_InsertColumn(hwndBookmarksList, 1, &lvc);
	// time column
	lvc.cx = BOOKMARKSLIST_COLUMN_TIME_WIDTH;
	ListView_InsertColumn(hwndBookmarksList, 2, &lvc);
	ListView_SetItemCountEx(hwndBookmarksList, TOTAL_BOOKMARKS, LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);

	reset();
	selected_slot = DEFAULT_SLOT;
	// find rows top/height (for mouseover hittest calculations)
	RECT temp_rect;
	if (ListView_GetSubItemRect(hwndBookmarksList, 0, 2, LVIR_BOUNDS, &temp_rect) && temp_rect.bottom != temp_rect.top)
	{
		list_row_top = temp_rect.top;
		list_row_left = temp_rect.left;
		list_row_height = temp_rect.bottom - temp_rect.top;
	} else
	{
		// couldn't get rect, set default values
		list_row_top = 0;
		list_row_left = BOOKMARKSLIST_COLUMN_ICONS_WIDTH + BOOKMARKSLIST_COLUMN_FRAMENUM_WIDTH;
		list_row_height = 14;
	}
	RedrawBookmarksCaption();
}
void BOOKMARKS::free()
{
	bookmarks_array.resize(0);
	if (himglist)
	{
		ImageList_Destroy(himglist);
		himglist = 0;
	}
}
void BOOKMARKS::reset()
{
	// delete all commands if there are any
	commands.resize(0);
	// init bookmarks
	bookmarks_array.resize(0);
	bookmarks_array.resize(TOTAL_BOOKMARKS);
	for (int i = 0; i < TOTAL_BOOKMARKS; ++i)
		bookmarks_array[i].init();
	reset_vars();
}
void BOOKMARKS::reset_vars()
{
	mouse_x = mouse_y = -1;
	item_under_mouse = ITEM_UNDER_MOUSE_NONE;
	mouse_over_bitmap = false;
	must_check_item_under_mouse = true;
	bookmark_leftclicked = bookmark_rightclicked = ITEM_UNDER_MOUSE_NONE;
	check_flash_shedule = clock() + BOOKMARKS_FLASH_TICK;
}

void BOOKMARKS::update()
{
	// execute all commands accumulated during last frame
	for (int i = 0; (i + 1) < (int)commands.size(); )	// FIFO
	{
		int command_id = commands[i++];
		int slot = commands[i++];
		switch (command_id)
		{
			case COMMAND_SET:
				set(slot);
				break;
			case COMMAND_JUMP:
				jump(slot);
				break;
			case COMMAND_DEPLOY:
				deploy(slot);
				break;
			case COMMAND_SELECT:
				if (selected_slot != slot)
				{
					int old_selected_slot = selected_slot;
					selected_slot = slot;
					RedrawBookmark(old_selected_slot);
					RedrawBookmark(selected_slot);
				}
				break;
		}
	}
	commands.resize(0);

	// once per 100 milliseconds update bookmark flashes
	if (clock() > check_flash_shedule)
	{
		check_flash_shedule = clock() + BOOKMARKS_FLASH_TICK;
		for (int i = 0; i < TOTAL_BOOKMARKS; ++i)
		{
			if (bookmark_rightclicked == i || bookmark_leftclicked == i)
			{
				if (bookmarks_array[i].flash_phase != FLASH_PHASE_BUTTONHELD)
				{
					bookmarks_array[i].flash_phase = FLASH_PHASE_BUTTONHELD;
					RedrawBookmark(i);
					branches.must_redraw_branches_tree = true;		// because border of branch digit has changed
				}
			} else
			{
				if (bookmarks_array[i].flash_phase > 0)
				{
					bookmarks_array[i].flash_phase--;
					RedrawBookmark(i);
					branches.must_redraw_branches_tree = true;		// because border of branch digit has changed
				}
			}
		}
	}

	// controls
	if (must_check_item_under_mouse)
	{
		if (edit_mode == EDIT_MODE_BRANCHES)
			item_under_mouse = branches.FindItemUnderMouse(mouse_x, mouse_y);
		else if (edit_mode == EDIT_MODE_BOTH)
			item_under_mouse = FindItemUnderMouse();
		else
			item_under_mouse = ITEM_UNDER_MOUSE_NONE;
		must_check_item_under_mouse = false;
	}
}

// stores commands in array for update() function
void BOOKMARKS::command(int command_id, int slot)
{
	if (slot < 0)
		slot = selected_slot;
	if (slot >= 0 && slot < TOTAL_BOOKMARKS)
	{
		commands.push_back(command_id);
		commands.push_back(slot);
	}
}

void BOOKMARKS::set(int slot)
{
	if (slot < 0 || slot >= TOTAL_BOOKMARKS) return;

	// First save changes in edited note (in case it's being currently edited)
	markers_manager.UpdateMarkerNote();

	int previous_frame = bookmarks_array[slot].snapshot.keyframe;
	if (bookmarks_array[slot].checkDiffFromCurrent())
	{
		BOOKMARK backup_copy(bookmarks_array[slot]);
		bookmarks_array[slot].set();
		// rebuild Branches Tree
		int old_current_branch = branches.GetCurrentBranch();
		branches.HandleBookmarkSet(slot);
		if (slot != old_current_branch && old_current_branch != ITEM_UNDER_MOUSE_CLOUD)
		{
			// current_branch was switched to slot, redraw Bookmarks List to change the color of digits
			piano_roll.RedrawRow(bookmarks_array[old_current_branch].snapshot.keyframe);
			RedrawChangedBookmarks(bookmarks_array[old_current_branch].snapshot.keyframe);
		}
		// also redraw List rows
		if (previous_frame >= 0 && previous_frame != currFrameCounter)
		{
			piano_roll.RedrawRow(previous_frame);
			RedrawChangedBookmarks(previous_frame);
		}
		piano_roll.RedrawRow(currFrameCounter);
		RedrawChangedBookmarks(currFrameCounter);
		// if screenshot of the slot is currently shown - reinit and redraw the picture
		if (popup_display.screenshot_currently_shown == slot)
			popup_display.screenshot_currently_shown = ITEM_UNDER_MOUSE_NONE;

		history.RegisterBookmarkSet(slot, backup_copy, old_current_branch);
		must_check_item_under_mouse = true;
		FCEU_DispMessage("Branch %d saved.", 0, slot);
	}
}

void BOOKMARKS::jump(int slot)
{
	if (slot < 0 || slot >= TOTAL_BOOKMARKS) return;
	if (bookmarks_array[slot].not_empty)
	{
		int frame = bookmarks_array[slot].snapshot.keyframe;
		playback.jump(frame);
		piano_roll.FollowPauseframe();
		bookmarks_array[slot].jumped();
	}
}

void BOOKMARKS::deploy(int slot)
{
	recorder.state_was_loaded_in_readwrite_mode = true;
	if (taseditor_config.old_branching_controls && movie_readonly)
	{
		jump(slot);
		return;
	}
	if (slot < 0 || slot >= TOTAL_BOOKMARKS) return;
	if (!bookmarks_array[slot].not_empty) return;

	int keyframe = bookmarks_array[slot].snapshot.keyframe;
	bool markers_changed = false;
	// revert Markers to the Bookmarked state
	if (bookmarks_array[slot].snapshot.MarkersDifferFromCurrent())
	{
		bookmarks_array[slot].snapshot.copyToMarkers();
		markers_changed = true;
	}
	// revert current movie data to the Bookmarked state
	if (taseditor_config.branch_full_movie)
	{
		bookmarks_array[slot].snapshot.inputlog.toMovie(currMovieData);
	} else
	{
		// restore movie up to and not including bookmarked frame (simulating old TASing method)
		if (keyframe)
			bookmarks_array[slot].snapshot.inputlog.toMovie(currMovieData, 0, keyframe - 1);
		else
			currMovieData.truncateAt(0);
		// add empty frame at the end (at keyframe)
		currMovieData.insertEmpty(-1, 1);
	}

	int first_change = history.RegisterBranching(slot, markers_changed);	// this also reverts Greenzone's LagLog if needed
	if (first_change >= 0)
	{
		selection.must_find_current_marker = playback.must_find_current_marker = true;
		piano_roll.UpdateItemCount();
		greenzone.Invalidate(first_change);
		bookmarks_array[slot].deployed();
	} else if (markers_changed)
	{
		selection.must_find_current_marker = playback.must_find_current_marker = true;
		bookmarks_array[slot].deployed();
	} else
	{
		// didn't change anything in current movie
		bookmarks_array[slot].jumped();
	}

	// jump to the target (bookmarked frame)
	if (greenzone.SavestateIsEmpty(keyframe))
		greenzone.WriteSavestate(keyframe, bookmarks_array[slot].savestate);
	playback.jump(keyframe, true);
	// switch current branch to this branch
	int old_current_branch = branches.GetCurrentBranch();
	branches.HandleBookmarkDeploy(slot);
	if (slot != old_current_branch && old_current_branch != ITEM_UNDER_MOUSE_CLOUD)
	{
		piano_roll.RedrawRow(bookmarks_array[old_current_branch].snapshot.keyframe);
		RedrawChangedBookmarks(bookmarks_array[old_current_branch].snapshot.keyframe);
		piano_roll.RedrawRow(keyframe);
		RedrawChangedBookmarks(keyframe);
	}
	FCEU_DispMessage("Branch %d loaded.", 0, slot);
	piano_roll.RedrawList();	// even though the Greenzone invalidation most likely have already sent the command to redraw
}

void BOOKMARKS::save(EMUFILE *os, bool really_save)
{
	if (really_save)
	{
		// write "BOOKMARKS" string
		os->fwrite(bookmarks_save_id, BOOKMARKS_ID_LEN);
		// write all 10 bookmarks
		for (int i = 0; i < TOTAL_BOOKMARKS; ++i)
			bookmarks_array[i].save(os);
		// write branches
		branches.save(os);
	} else
	{
		// write "BOOKMARKX" string
		os->fwrite(bookmarks_skipsave_id, BOOKMARKS_ID_LEN);
	}
}
// returns true if couldn't load
bool BOOKMARKS::load(EMUFILE *is, unsigned int offset)
{
	if (offset)
	{
		if (is->fseek(offset, SEEK_SET)) goto error;
	} else
	{
		reset();
		branches.reset();
		return false;
	}
	// read "BOOKMARKS" string
	char save_id[BOOKMARKS_ID_LEN];
	if ((int)is->fread(save_id, BOOKMARKS_ID_LEN) < BOOKMARKS_ID_LEN) goto error;
	if (!strcmp(bookmarks_skipsave_id, save_id))
	{
		// string says to skip loading Bookmarks
		FCEU_printf("No Bookmarks in the file\n");
		reset();
		branches.reset();
		return false;
	}
	if (strcmp(bookmarks_save_id, save_id)) goto error;		// string is not valid
	// read all 10 bookmarks
	for (int i = 0; i < TOTAL_BOOKMARKS; ++i)
		if (bookmarks_array[i].load(is)) goto error;
	// read branches
	if (branches.load(is)) goto error;
	// all ok
	reset_vars();
	RedrawBookmarksCaption();
	return false;
error:
	FCEU_printf("Error loading Bookmarks\n");
	reset();
	branches.reset();
	return true;
}
// ----------------------------------------------------------
void BOOKMARKS::RedrawBookmarksCaption()
{
	int prev_edit_mode = edit_mode;
	if (taseditor_config.view_branches_tree)
	{
		edit_mode = EDIT_MODE_BRANCHES;
		ShowWindow(hwndBookmarksList, SW_HIDE);
		ShowWindow(hwndBranchesBitmap, SW_SHOW);
	} else if (taseditor_config.old_branching_controls && movie_readonly)
	{
		edit_mode = EDIT_MODE_BOOKMARKS;
		ShowWindow(hwndBranchesBitmap, SW_HIDE);
		ShowWindow(hwndBookmarksList, SW_SHOW);
		RedrawBookmarksList();
	} else
	{
		edit_mode = EDIT_MODE_BOTH;
		ShowWindow(hwndBranchesBitmap, SW_HIDE);
		ShowWindow(hwndBookmarksList, SW_SHOW);
		RedrawBookmarksList();
	}
	if (prev_edit_mode != edit_mode)
		must_check_item_under_mouse = true;
	SetWindowText(hwndBookmarks, bookmarksCaption[edit_mode]);
}
void BOOKMARKS::RedrawBookmarksList(bool erase_bg)
{
	if (edit_mode != EDIT_MODE_BRANCHES)
		InvalidateRect(hwndBookmarksList, 0, erase_bg);
}
void BOOKMARKS::RedrawChangedBookmarks(int frame)
{
	for (int i = 0; i < TOTAL_BOOKMARKS; ++i)
	{
		if (bookmarks_array[i].snapshot.keyframe == frame)
			RedrawBookmark(i);
	}
}
void BOOKMARKS::RedrawBookmark(int bookmark_number)
{
	RedrawListRow((bookmark_number + TOTAL_BOOKMARKS - 1) % TOTAL_BOOKMARKS);
}
void BOOKMARKS::RedrawListRow(int row_index)
{
	ListView_RedrawItems(hwndBookmarksList, row_index, row_index);
}

void BOOKMARKS::MouseMove(int new_x, int new_y)
{
	mouse_x = new_x;
	mouse_y = new_y;
	must_check_item_under_mouse = true;
}
int BOOKMARKS::FindItemUnderMouse()
{
	int item = ITEM_UNDER_MOUSE_NONE;
	RECT wrect;
	GetClientRect(hwndBookmarksList, &wrect);
	if (mouse_x >= list_row_left && mouse_x < wrect.right - wrect.left && mouse_y >= list_row_top && mouse_y < wrect.bottom - wrect.top)
	{
		int row_under_mouse = (mouse_y - list_row_top) / list_row_height;
		if (row_under_mouse >= 0 && row_under_mouse < TOTAL_BOOKMARKS)
			item = (row_under_mouse + 1) % TOTAL_BOOKMARKS;
	}
	return item;
}

int BOOKMARKS::GetSelectedSlot()
{
	return selected_slot;
}
// ----------------------------------------------------------------------------------------
void BOOKMARKS::GetDispInfo(NMLVDISPINFO* nmlvDispInfo)
{
	LVITEM& item = nmlvDispInfo->item;
	if (item.mask & LVIF_TEXT)
	{
		switch(item.iSubItem)
		{
			case BOOKMARKS_COLUMN_ICON:
			{
				if ((item.iItem + 1) % TOTAL_BOOKMARKS == branches.GetCurrentBranch())
					item.iImage = ((item.iItem + 1) % TOTAL_BOOKMARKS) + TOTAL_BOOKMARKS;
				else
					item.iImage = (item.iItem + 1) % TOTAL_BOOKMARKS;
				if (taseditor_config.old_branching_controls)
				{
					if ((item.iItem + 1) % TOTAL_BOOKMARKS == selected_slot)
						item.iImage += BOOKMARKS_SELECTED;
				}
				break;
			}
			case BOOKMARKS_COLUMN_FRAME:
			{
				if (bookmarks_array[(item.iItem + 1) % TOTAL_BOOKMARKS].not_empty)
					U32ToDecStr(item.pszText, bookmarks_array[(item.iItem + 1) % TOTAL_BOOKMARKS].snapshot.keyframe, DIGITS_IN_FRAMENUM);
				break;
			}
			case BOOKMARKS_COLUMN_TIME:
			{
				if (bookmarks_array[(item.iItem + 1) % TOTAL_BOOKMARKS].not_empty)
					strcpy(item.pszText, bookmarks_array[(item.iItem + 1) % TOTAL_BOOKMARKS].snapshot.description);
			}
			break;
		}
	}
}

LONG BOOKMARKS::CustomDraw(NMLVCUSTOMDRAW* msg)
{
	int cell_x, cell_y;
	switch(msg->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;
	case CDDS_ITEMPREPAINT:
		return CDRF_NOTIFYSUBITEMDRAW;
	case CDDS_SUBITEMPREPAINT:
		cell_x = msg->iSubItem;
		cell_y = (msg->nmcd.dwItemSpec + 1) % TOTAL_BOOKMARKS;
		
		// flash with text color when needed
		if (bookmarks_array[cell_y].flash_phase)
			msg->clrText = bookmark_flash_colors[bookmarks_array[cell_y].flash_type][bookmarks_array[cell_y].flash_phase];

		if (cell_x == BOOKMARKS_COLUMN_FRAME || (taseditor_config.old_branching_controls && movie_readonly && cell_x == BOOKMARKS_COLUMN_TIME))
		{
			if (bookmarks_array[cell_y].not_empty)
			{
				// frame number
				SelectObject(msg->nmcd.hdc, piano_roll.hMainListFont);
				int frame = bookmarks_array[cell_y].snapshot.keyframe;
				if (frame == currFrameCounter || frame == (playback.GetFlashingPauseFrame() - 1))
				{
					// current frame
					msg->clrTextBk = CUR_FRAMENUM_COLOR;
				} else if (frame < greenzone.GetSize())
				{
					if (!greenzone.SavestateIsEmpty(frame))
					{
						if (greenzone.laglog.GetLagInfoAtFrame(frame) == LAGGED_YES)
							msg->clrTextBk = LAG_FRAMENUM_COLOR;
						else
							msg->clrTextBk = GREENZONE_FRAMENUM_COLOR;
					} else if (!greenzone.SavestateIsEmpty(cell_y & EVERY16TH)
						|| !greenzone.SavestateIsEmpty(cell_y & EVERY8TH)
						|| !greenzone.SavestateIsEmpty(cell_y & EVERY4TH)
						|| !greenzone.SavestateIsEmpty(cell_y & EVERY2ND))
					{
						if (greenzone.laglog.GetLagInfoAtFrame(frame) == LAGGED_YES)
							msg->clrTextBk = PALE_LAG_FRAMENUM_COLOR;
						else
							msg->clrTextBk = PALE_GREENZONE_FRAMENUM_COLOR;
					} else msg->clrTextBk = NORMAL_FRAMENUM_COLOR;
				} else msg->clrTextBk = NORMAL_FRAMENUM_COLOR;
			} else msg->clrTextBk = NORMAL_BACKGROUND_COLOR;	// empty bookmark
		} else if (cell_x == BOOKMARKS_COLUMN_TIME)
		{
			if (bookmarks_array[cell_y].not_empty)
			{
				// frame number
				SelectObject(msg->nmcd.hdc, piano_roll.hMainListFont);
				int frame = bookmarks_array[cell_y].snapshot.keyframe;
				if (frame == currFrameCounter || frame == (playback.GetFlashingPauseFrame() - 1))
				{
					// current frame
					msg->clrTextBk = CUR_INPUT_COLOR1;
				} else if (frame < greenzone.GetSize())
				{
					if (!greenzone.SavestateIsEmpty(frame))
					{
						if (greenzone.laglog.GetLagInfoAtFrame(frame) == LAGGED_YES)
							msg->clrTextBk = LAG_INPUT_COLOR1;
						else
							msg->clrTextBk = GREENZONE_INPUT_COLOR1;
					} else if (!greenzone.SavestateIsEmpty(cell_y & EVERY16TH)
						|| !greenzone.SavestateIsEmpty(cell_y & EVERY8TH)
						|| !greenzone.SavestateIsEmpty(cell_y & EVERY4TH)
						|| !greenzone.SavestateIsEmpty(cell_y & EVERY2ND))
					{
						if (greenzone.laglog.GetLagInfoAtFrame(frame) == LAGGED_YES)
							msg->clrTextBk = PALE_LAG_INPUT_COLOR1;
						else
							msg->clrTextBk = PALE_GREENZONE_INPUT_COLOR1;
					} else msg->clrTextBk = NORMAL_INPUT_COLOR1;
				} else msg->clrTextBk = NORMAL_INPUT_COLOR1;
			} else msg->clrTextBk = NORMAL_BACKGROUND_COLOR;		// empty bookmark
		}
	default:
		return CDRF_DODEFAULT;
	}
}

void BOOKMARKS::LeftClick()
{
	if (column_clicked <= BOOKMARKS_COLUMN_FRAME || (taseditor_config.old_branching_controls && movie_readonly))
		command(COMMAND_JUMP, bookmark_leftclicked);
	else if (column_clicked == BOOKMARKS_COLUMN_TIME && (!taseditor_config.old_branching_controls || !movie_readonly))
		command(COMMAND_DEPLOY, bookmark_leftclicked);
}
void BOOKMARKS::RightClick()
{
	if (bookmark_rightclicked >= 0)
		command(COMMAND_SET, bookmark_rightclicked);
}

int BOOKMARKS::FindBookmarkAtFrame(int frame)
{
	int cur_bookmark = branches.GetCurrentBranch();
	if (cur_bookmark != ITEM_UNDER_MOUSE_CLOUD && bookmarks_array[cur_bookmark].snapshot.keyframe == frame)
		return cur_bookmark + TOTAL_BOOKMARKS;	// blue digit has highest priority when drawing
	for (int i = 0; i < TOTAL_BOOKMARKS; ++i)
	{
		cur_bookmark = (i + 1) % TOTAL_BOOKMARKS;
		if (bookmarks_array[cur_bookmark].not_empty && bookmarks_array[cur_bookmark].snapshot.keyframe == frame)
			return cur_bookmark;	// green digit
	}
	return -1;		// no Bookmarks at the frame
}
// ----------------------------------------------------------------------------------------
LRESULT APIENTRY BookmarksListWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	extern BOOKMARKS bookmarks;
	switch(msg)
	{
		case WM_CHAR:
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SETFOCUS:
		case WM_KILLFOCUS:
			return 0;
		case WM_MOUSEMOVE:
		{
			if (!bookmarks.mouse_over_bookmarkslist)
			{
				bookmarks.mouse_over_bookmarkslist = true;
				bookmarks.list_tme.hwndTrack = hWnd;
				TrackMouseEvent(&bookmarks.list_tme);
			}
			bookmarks.MouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			break;
		}
		case WM_MOUSELEAVE:
		{
			bookmarks.mouse_over_bookmarkslist = false;
			bookmarks.MouseMove(-1, -1);
			break;
		}
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		{
			if (GetFocus() != hWnd)
				SetFocus(hWnd);
			LVHITTESTINFO info;
			info.pt.x = GET_X_LPARAM(lParam);
			info.pt.y = GET_Y_LPARAM(lParam);
			ListView_SubItemHitTest(hWnd, (LPARAM)&info);
			if (info.iItem >= 0 && bookmarks.bookmark_rightclicked < 0)
			{
				bookmarks.bookmark_leftclicked = (info.iItem + 1) % TOTAL_BOOKMARKS;
				bookmarks.column_clicked = info.iSubItem;
				if (bookmarks.column_clicked <= BOOKMARKS_COLUMN_FRAME || (taseditor_config.old_branching_controls && movie_readonly))
					bookmarks.bookmarks_array[bookmarks.bookmark_leftclicked].flash_type = FLASH_TYPE_JUMP;
				else if (bookmarks.column_clicked == BOOKMARKS_COLUMN_TIME && (!taseditor_config.old_branching_controls || !movie_readonly))
					bookmarks.bookmarks_array[bookmarks.bookmark_leftclicked].flash_type = FLASH_TYPE_DEPLOY;
				SetCapture(hWnd);
			}
			return 0;
		}
		case WM_LBUTTONUP:
		{
			LVHITTESTINFO info;
			info.pt.x = GET_X_LPARAM(lParam);
			info.pt.y = GET_Y_LPARAM(lParam);
			RECT wrect;
			GetClientRect(hWnd, &wrect);
			if (info.pt.x >= 0 && info.pt.x < wrect.right - wrect.left && info.pt.y >= 0 && info.pt.y < wrect.bottom - wrect.top)
			{
				ListView_SubItemHitTest(hWnd, (LPARAM)&info);
				if (bookmarks.bookmark_leftclicked == (info.iItem + 1) % TOTAL_BOOKMARKS && bookmarks.column_clicked == info.iSubItem)
					bookmarks.LeftClick();
			}
			ReleaseCapture();
			bookmarks.bookmark_leftclicked = -1;
			return 0;
		}
		case WM_RBUTTONDOWN:
		case WM_RBUTTONDBLCLK:
		{
			if (GetFocus() != hWnd)
				SetFocus(hWnd);
			LVHITTESTINFO info;
			info.pt.x = GET_X_LPARAM(lParam);
			info.pt.y = GET_Y_LPARAM(lParam);
			ListView_SubItemHitTest(hWnd, (LPARAM)&info);
			if (info.iItem >= 0 && bookmarks.bookmark_leftclicked < 0)
			{
				bookmarks.bookmark_rightclicked = (info.iItem + 1) % TOTAL_BOOKMARKS;
				bookmarks.column_clicked = info.iSubItem;
				bookmarks.bookmarks_array[bookmarks.bookmark_rightclicked].flash_type = FLASH_TYPE_SET;
				SetCapture(hWnd);
			}
			return 0;
		}
		case WM_RBUTTONUP:
		{
			LVHITTESTINFO info;
			info.pt.x = GET_X_LPARAM(lParam);
			info.pt.y = GET_Y_LPARAM(lParam);
			RECT wrect;
			GetClientRect(hWnd, &wrect);
			if (info.pt.x >= 0 && info.pt.x < wrect.right - wrect.left && info.pt.y >= 0 && info.pt.y < wrect.bottom - wrect.top)
			{
				ListView_SubItemHitTest(hWnd, (LPARAM)&info);
				if (bookmarks.bookmark_rightclicked == (info.iItem + 1) % TOTAL_BOOKMARKS && bookmarks.column_clicked == info.iSubItem)
					bookmarks.RightClick();
			}
			ReleaseCapture();
			bookmarks.bookmark_rightclicked = ITEM_UNDER_MOUSE_NONE;
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
		case WM_MOUSEWHEEL:
		{
			bookmarks.bookmark_rightclicked = ITEM_UNDER_MOUSE_NONE;	// ensure that accidental rightclick on BookmarksList won't set Bookmarks when user does rightbutton + wheel
			return SendMessage(piano_roll.hwndList, msg, wParam, lParam);
		}
	}
	return CallWindowProc(hwndBookmarksList_oldWndProc, hWnd, msg, wParam, lParam);
}

