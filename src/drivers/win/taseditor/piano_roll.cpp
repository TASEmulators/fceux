/* ---------------------------------------------------------------------------------
Implementation file of PIANO_ROLL class
Copyright (c) 2011-2012 AnS

(The MIT License)
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------------
Piano Roll - Piano Roll interface
[Singleton]

* implements the working of Piano Roll List: creating, redrawing, scrolling, clicks
* on demand: scrolls visible area of the List to any given item: to Playback Cursor, to Selection Cursor, to "undo pointer", to a Marker
* saves and loads current position of vertical scrolling from a project file. On error: scrolls the List to the beginning
* implements the working of Piano Roll List Header: creating, redrawing, animating, mouseover, clicks
* on demand: launches flashes in the List Header
* regularly updates the size of the List according to current movie input, also updates lights in the List Header according to button presses data from Recorder and Alt key state
* implements the working of mouse wheel: List scrolling, Playback cursor movement, Selection cursor movement
* implements context menu on Right-click
* stores resources: save id, ids of columns, widths of columns, tables of colors, gradient of Hot Changes, gradient of Header flashings, timings of flashes, all fonts used in TAS Editor, images
------------------------------------------------------------------------------------ */

#include "taseditor_project.h"
#include "utils/xstring.h"
#include "uxtheme.h"

#pragma comment(lib, "UxTheme.lib")

extern int joysticks_per_frame[NUM_SUPPORTED_INPUT_TYPES];
extern char buttonNames[NUM_JOYPAD_BUTTONS][2];

extern std::vector<std::string> autofire_patterns_names;
extern std::vector<std::vector<uint8>> autofire_patterns;

extern TASEDITOR_CONFIG taseditor_config;
extern TASEDITOR_WINDOW taseditor_window;
extern BOOKMARKS bookmarks;
extern PLAYBACK playback;
extern RECORDER recorder;
extern GREENZONE greenzone;
extern HISTORY history;
extern MARKERS_MANAGER markers_manager;
extern SELECTION selection;

extern int GetInputType(MovieData& md);

LRESULT APIENTRY HeaderWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY ListWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
WNDPROC hwndList_oldWndProc = 0, hwndHeader_oldWndproc = 0;

// resources
char piano_roll_save_id[PIANO_ROLL_ID_LEN] = "PIANO_ROLL";
char piano_roll_skipsave_id[PIANO_ROLL_ID_LEN] = "PIANO_ROLX";
COLORREF hot_changes_colors[16] = { 0x0, 0x5c4c44, 0x854604, 0xab2500, 0xc20006, 0xd6006f, 0xd40091, 0xba00a4, 0x9500ba, 0x7a00cc, 0x5800d4, 0x0045e2, 0x0063ea, 0x0079f4, 0x0092fa, 0x00aaff };
//COLORREF hot_changes_colors[16] = { 0x0, 0x661212, 0x842B4E, 0x652C73, 0x48247D, 0x383596, 0x2947AE, 0x1E53C1, 0x135DD2, 0x116EDA, 0x107EE3, 0x0F8EEB, 0x209FF4, 0x3DB1FD, 0x51C2FF, 0x4DCDFF };
COLORREF header_lights_colors[11] = { 0x0, 0x007313, 0x009100, 0x1daf00, 0x42c700, 0x65d900, 0x91e500, 0xb0f000, 0xdaf700, 0xf0fc7c, 0xfcffba };

PIANO_ROLL::PIANO_ROLL()
{
}

void PIANO_ROLL::init()
{
	free();
	// create fonts for main listview
	hMainListFont = CreateFont(14, 7,				/*Height,Width*/
		0, 0,										/*escapement,orientation*/
		FW_BOLD, FALSE, FALSE, FALSE,				/*weight, italic, underline, strikeout*/
		ANSI_CHARSET, OUT_DEVICE_PRECIS, CLIP_MASK,	/*charset, precision, clipping*/
		DEFAULT_QUALITY, DEFAULT_PITCH,				/*quality, and pitch*/
		"Arial");									/*font name*/
	hMainListSelectFont = CreateFont(15, 10,		/*Height,Width*/
		0, 0,										/*escapement,orientation*/
		FW_BOLD, FALSE, FALSE, FALSE,				/*weight, italic, underline, strikeout*/
		ANSI_CHARSET, OUT_DEVICE_PRECIS, CLIP_MASK,	/*charset, precision, clipping*/
		DEFAULT_QUALITY, DEFAULT_PITCH,				/*quality, and pitch*/
		"Courier New");								/*font name*/
	// create fonts for Marker notes fields
	hMarkersFont = CreateFont(16, 8,				/*Height,Width*/
		0, 0,										/*escapement,orientation*/
		FW_BOLD, FALSE, FALSE, FALSE,				/*weight, italic, underline, strikeout*/
		ANSI_CHARSET, OUT_DEVICE_PRECIS, CLIP_MASK,	/*charset, precision, clipping*/
		DEFAULT_QUALITY, DEFAULT_PITCH,				/*quality, and pitch*/
		"Arial");									/*font name*/
	hMarkersEditFont = CreateFont(16, 7,			/*Height,Width*/
		0, 0,										/*escapement,orientation*/
		FW_NORMAL, FALSE, FALSE, FALSE,				/*weight, italic, underline, strikeout*/
		ANSI_CHARSET, OUT_DEVICE_PRECIS, CLIP_MASK,	/*charset, precision, clipping*/
		DEFAULT_QUALITY, DEFAULT_PITCH,				/*quality, and pitch*/
		"Arial");									/*font name*/
	bg_brush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));

	hwndList = GetDlgItem(taseditor_window.hwndTasEditor, IDC_LIST1);
	// prepare the main listview
	ListView_SetExtendedListViewStyleEx(hwndList, LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES, LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
	// subclass the header
	hwndHeader = ListView_GetHeader(hwndList);
	hwndHeader_oldWndproc = (WNDPROC)SetWindowLong(hwndHeader, GWL_WNDPROC, (LONG)HeaderWndProc);
	// subclass the whole listview
	hwndList_oldWndProc = (WNDPROC)SetWindowLong(hwndList, GWL_WNDPROC, (LONG)ListWndProc);
	// disable Visual Themes for header
	SetWindowTheme(hwndHeader, L"", L"");
	// setup images for the listview
	himglist = ImageList_Create(9, 13, ILC_COLOR8 | ILC_MASK, 1, 1);
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
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_TE_ARROW));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	bmp = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_TE_GREEN_ARROW));
	ImageList_AddMasked(himglist, bmp, 0xFFFFFF);
	DeleteObject(bmp);
	ListView_SetImageList(hwndList, himglist, LVSIL_SMALL);
	// setup 0th column
	LVCOLUMN lvc;
	// icons column
	lvc.mask = LVCF_WIDTH;
	lvc.cx = COLUMN_ICONS_WIDTH;
	ListView_InsertColumn(hwndList, 0, &lvc);

	hrmenu = LoadMenu(fceu_hInstance,"TASEDITORCONTEXTMENUS");
	header_colors.resize(TOTAL_COLUMNS);
	// fill TrackMouseEvent struct
	tme.cbSize = sizeof(tme);
	tme.dwFlags = TME_LEAVE;
	tme.hwndTrack = hwndHeader;

}
void PIANO_ROLL::free()
{
	if (hMainListFont)
	{
		DeleteObject(hMainListFont);
		hMainListFont = 0;
	}
	if (hMainListSelectFont)
	{
		DeleteObject(hMainListSelectFont);
		hMainListSelectFont = 0;
	}
	if (hMarkersFont)
	{
		DeleteObject(hMarkersFont);
		hMarkersFont = 0;
	}
	if (hMarkersEditFont)
	{
		DeleteObject(hMarkersEditFont);
		hMarkersEditFont = 0;
	}
	if (bg_brush)
	{
		DeleteObject(bg_brush);
		bg_brush = 0;
	}
	if (himglist)
	{
		ImageList_Destroy(himglist);
		himglist = 0;
	}
	header_colors.resize(0);
}
void PIANO_ROLL::reset()
{
	next_header_update_time = header_item_under_mouse = 0;
	// delete all columns except 0th
	while (ListView_DeleteColumn(hwndList, 1)) {}
	// setup columns
	num_columns = 1;
	LVCOLUMN lvc;
	// frame number column
	lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT;
	lvc.fmt = LVCFMT_CENTER;
	lvc.cx = COLUMN_FRAMENUM_WIDTH;
	lvc.pszText = "Frame#";
	ListView_InsertColumn(hwndList, num_columns++, &lvc);
	// pads columns
	lvc.cx = COLUMN_BUTTON_WIDTH;
	int num_joysticks = joysticks_per_frame[GetInputType(currMovieData)];
	for (int joy = 0; joy < num_joysticks; ++joy)
	{
		for (int btn = 0; btn < NUM_JOYPAD_BUTTONS; ++btn)
		{
			lvc.pszText = buttonNames[btn];
			ListView_InsertColumn(hwndList, num_columns++, &lvc);
		}
	}
	// add 2nd frame number column if needed
	if (num_columns >= NUM_COLUMNS_NEED_2ND_FRAMENUM)
	{
		
		lvc.cx = COLUMN_FRAMENUM_WIDTH;
		lvc.pszText = "Frame#";
		ListView_InsertColumn(hwndList, num_columns++, &lvc);
	}
}
void PIANO_ROLL::update()
{
	//update the number of items in the list
	int currLVItemCount = ListView_GetItemCount(hwndList);
	int movie_size = currMovieData.getNumRecords();
	if(currLVItemCount != movie_size)
		ListView_SetItemCountEx(hwndList, movie_size, LVSICF_NOSCROLL|LVSICF_NOINVALIDATEALL);

	// once per 40 milliseconds update colors alpha
	if (clock() > next_header_update_time)
	{
		next_header_update_time = clock() + HEADER_LIGHT_UPDATE_TICK;
		bool changes_made = false;
		int light_value = 0;
		// 1 - update Frame# columns' heads
		if (GetAsyncKeyState(VK_MENU) & 0x8000)
			light_value = HEADER_LIGHT_HOLD;
		else if (header_item_under_mouse == COLUMN_FRAMENUM || header_item_under_mouse == COLUMN_FRAMENUM2)
			light_value = (selection.GetCurrentSelectionSize() > 0) ? HEADER_LIGHT_MOUSEOVER_SEL : HEADER_LIGHT_MOUSEOVER;
		if (header_colors[COLUMN_FRAMENUM] < light_value)
		{
			header_colors[COLUMN_FRAMENUM]++;
			changes_made = true;
		} else if (header_colors[COLUMN_FRAMENUM] > light_value)
		{
			header_colors[COLUMN_FRAMENUM]--;
			changes_made = true;
		}
		header_colors[COLUMN_FRAMENUM2] = header_colors[COLUMN_FRAMENUM];
		// 2 - update input columns' heads
		int i = num_columns-1;
		if (i == COLUMN_FRAMENUM2) i--;
		for (; i >= COLUMN_JOYPAD1_A; i--)
		{
			light_value = 0;
			if (recorder.current_joy[(i - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS] & (1 << ((i - COLUMN_JOYPAD1_A) % NUM_JOYPAD_BUTTONS)))
				light_value = HEADER_LIGHT_HOLD;
			else if (header_item_under_mouse == i)
				light_value = (selection.GetCurrentSelectionSize() > 0) ? HEADER_LIGHT_MOUSEOVER_SEL : HEADER_LIGHT_MOUSEOVER;
			if (header_colors[i] < light_value)
			{
				header_colors[i]++;
				changes_made = true;
			} else if (header_colors[i] > light_value)
			{
				header_colors[i]--;
				changes_made = true;
			}
		}
		// 3 - redraw
		if (changes_made)
			RedrawHeader();
	}
}

void PIANO_ROLL::save(EMUFILE *os, bool really_save)
{
	if (really_save)
	{
		update();
		// write "PIANO_ROLL" string
		os->fwrite(piano_roll_save_id, PIANO_ROLL_ID_LEN);
		// write current top item
		int top_item = ListView_GetTopIndex(hwndList);
		write32le(top_item, os);
	} else
	{
		// write "PIANO_ROLX" string
		os->fwrite(piano_roll_skipsave_id, PIANO_ROLL_ID_LEN);
	}
}
// returns true if couldn't load
bool PIANO_ROLL::load(EMUFILE *is)
{
	reset();
	update();
	// read "PIANO_ROLL" string
	char save_id[PIANO_ROLL_ID_LEN];
	if ((int)is->fread(save_id, PIANO_ROLL_ID_LEN) < PIANO_ROLL_ID_LEN) goto error;
	if (!strcmp(piano_roll_skipsave_id, save_id))
	{
		// string says to skip loading Piano Roll
		FCEU_printf("No Piano Roll data in the file\n");
		// scroll to the beginning
		ListView_EnsureVisible(hwndList, 0, FALSE);
		return false;
	}
	if (strcmp(piano_roll_save_id, save_id)) goto error;		// string is not valid
	// read current top item and scroll Piano Roll there
	int top_item;
	if (!read32le(&top_item, is)) goto error;
	ListView_EnsureVisible(hwndList, currMovieData.getNumRecords() - 1, FALSE);
	ListView_EnsureVisible(hwndList, top_item, FALSE);
	return false;
error:
	FCEU_printf("Error loading Piano Roll data\n");
	// scroll to the beginning
	ListView_EnsureVisible(hwndList, 0, FALSE);
	return true;
}
// ----------------------------------------------------------------------
void PIANO_ROLL::RedrawList()
{
	InvalidateRect(hwndList, 0, FALSE);
}
void PIANO_ROLL::RedrawRow(int index)
{
	ListView_RedrawItems(hwndList, index, index);
}
void PIANO_ROLL::RedrawHeader()
{
	InvalidateRect(hwndHeader, 0, FALSE);
}

// -------------------------------------------------------------------------
bool PIANO_ROLL::CheckItemVisible(int frame)
{
	int top = ListView_GetTopIndex(hwndList);
	// in fourscore there's horizontal scrollbar which takes one row for itself
	if (frame >= top && frame < top + ListView_GetCountPerPage(hwndList))
		return true;
	return false;
}

void PIANO_ROLL::CenterListAt(int frame)
{
	int list_items = ListView_GetCountPerPage(hwndList);
	int lower_border = (list_items - 1) / 2;
	int upper_border = (list_items - 1) - lower_border;
	int index = frame + lower_border;
	if (index >= currMovieData.getNumRecords())
		index = currMovieData.getNumRecords()-1;
	ListView_EnsureVisible(hwndList, index, false);
	index = frame - upper_border;
	if (index < 0)
		index = 0;
	ListView_EnsureVisible(hwndList, index, false);
}

void PIANO_ROLL::FollowPlayback()
{
	CenterListAt(currFrameCounter);
}
void PIANO_ROLL::FollowPlaybackIfNeeded()
{
	if (taseditor_config.follow_playback) ListView_EnsureVisible(hwndList,currFrameCounter,FALSE);
}
void PIANO_ROLL::FollowUndo()
{
	int jump_frame = history.GetUndoHint();
	if (taseditor_config.jump_to_undo && jump_frame >= 0)
	{
		if (!CheckItemVisible(jump_frame))
			CenterListAt(jump_frame);
	}
}
void PIANO_ROLL::FollowSelection()
{
	SelectionFrames* current_selection = selection.MakeStrobe();
	if (current_selection->size() == 0) return;

	int list_items = ListView_GetCountPerPage(hwndList);
	int selection_start = *current_selection->begin();
	int selection_end = *current_selection->rbegin();
	int selection_items = 1 + selection_end - selection_start;
	
	if (selection_items <= list_items)
	{
		// selected region can fit in screen
		int lower_border = (list_items - selection_items) / 2;
		int upper_border = (list_items - selection_items) - lower_border;
		int index = selection_end + lower_border;
		if (index >= currMovieData.getNumRecords())
			index = currMovieData.getNumRecords()-1;
		ListView_EnsureVisible(hwndList, index, false);
		index = selection_start - upper_border;
		if (index < 0)
			index = 0;
		ListView_EnsureVisible(hwndList, index, false);
	} else
	{
		// selected region is too big to fit in screen
		// oh well, just center at selection_start
		CenterListAt(selection_start);
	}
}
void PIANO_ROLL::FollowPauseframe()
{
	if (playback.pause_frame > 0)
		CenterListAt(playback.pause_frame - 1);
}
void PIANO_ROLL::FollowMarker(int marker_id)
{
	if (marker_id > 0)
	{
		int frame = markers_manager.GetMarkerFrame(marker_id);
		if (frame >= 0)
			CenterListAt(frame);
	} else
	{
		ListView_EnsureVisible(hwndList, 0, false);
	}
}

void PIANO_ROLL::SetHeaderColumnLight(int column, int level)
{
	if (column < COLUMN_FRAMENUM || column >= num_columns || level < 0 || level > HEADER_LIGHT_MAX)
		return;

	if (header_colors[column] != level)
	{
		header_colors[column] = level;
		RedrawHeader();
		next_header_update_time = clock() + HEADER_LIGHT_UPDATE_TICK;
	}
}

void PIANO_ROLL::GetDispInfo(NMLVDISPINFO* nmlvDispInfo)
{
	LVITEM& item = nmlvDispInfo->item;
	if(item.mask & LVIF_TEXT)
	{
		switch(item.iSubItem)
		{
			case COLUMN_ICONS:
			{
				if(item.iImage == I_IMAGECALLBACK)
				{
					item.iImage = bookmarks.FindBookmarkAtFrame(item.iItem);
					if (item.iImage < 0)
					{
						if (item.iItem == currFrameCounter)
							item.iImage = ARROW_IMAGE_ID;
						else if (item.iItem == playback.lost_position_frame-1)
							item.iImage = GREEN_ARROW_IMAGE_ID;
					}
				}
				break;
			}
			case COLUMN_FRAMENUM:
			case COLUMN_FRAMENUM2:
			{
				U32ToDecStr(item.pszText, item.iItem, DIGITS_IN_FRAMENUM);
				break;
			}
			case COLUMN_JOYPAD1_A: case COLUMN_JOYPAD1_B: case COLUMN_JOYPAD1_S: case COLUMN_JOYPAD1_T:
			case COLUMN_JOYPAD1_U: case COLUMN_JOYPAD1_D: case COLUMN_JOYPAD1_L: case COLUMN_JOYPAD1_R:
			case COLUMN_JOYPAD2_A: case COLUMN_JOYPAD2_B: case COLUMN_JOYPAD2_S: case COLUMN_JOYPAD2_T:
			case COLUMN_JOYPAD2_U: case COLUMN_JOYPAD2_D: case COLUMN_JOYPAD2_L: case COLUMN_JOYPAD2_R:
			case COLUMN_JOYPAD3_A: case COLUMN_JOYPAD3_B: case COLUMN_JOYPAD3_S: case COLUMN_JOYPAD3_T:
			case COLUMN_JOYPAD3_U: case COLUMN_JOYPAD3_D: case COLUMN_JOYPAD3_L: case COLUMN_JOYPAD3_R:
			case COLUMN_JOYPAD4_A: case COLUMN_JOYPAD4_B: case COLUMN_JOYPAD4_S: case COLUMN_JOYPAD4_T:
			case COLUMN_JOYPAD4_U: case COLUMN_JOYPAD4_D: case COLUMN_JOYPAD4_L: case COLUMN_JOYPAD4_R:
			{
				int joy = (item.iSubItem - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS;
				int bit = (item.iSubItem - COLUMN_JOYPAD1_A) % NUM_JOYPAD_BUTTONS;
				uint8 data = ((int)currMovieData.records.size() > item.iItem) ? currMovieData.records[item.iItem].joysticks[joy] : 0;
				if(data & (1<<bit))
				{
					item.pszText[0] = buttonNames[bit][0];
					item.pszText[2] = 0;
				} else
				{
					if (taseditor_config.enable_hot_changes && history.GetCurrentSnapshot().GetHotChangeInfo(item.iItem, item.iSubItem - COLUMN_JOYPAD1_A))
					{
						item.pszText[0] = 45;	// "-"
						item.pszText[1] = 0;
					} else item.pszText[0] = 0;
				}
			}
			break;
		}
	}
}

LONG PIANO_ROLL::CustomDraw(NMLVCUSTOMDRAW* msg)
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
		cell_y = msg->nmcd.dwItemSpec;

		if(cell_x > COLUMN_ICONS)
		{
			// text color
			if(taseditor_config.enable_hot_changes && cell_x >= COLUMN_JOYPAD1_A && cell_x <= COLUMN_JOYPAD4_R)
				msg->clrText = hot_changes_colors[history.GetCurrentSnapshot().GetHotChangeInfo(cell_y, cell_x - COLUMN_JOYPAD1_A)];
			else
				msg->clrText = NORMAL_TEXT_COLOR;
			// bg color and text font
			if(cell_x == COLUMN_FRAMENUM || cell_x == COLUMN_FRAMENUM2)
			{
				// font
				if(markers_manager.GetMarker(cell_y))
					SelectObject(msg->nmcd.hdc, hMainListSelectFont);
				else
					SelectObject(msg->nmcd.hdc, hMainListFont);
				// bg
				// frame number
				if (cell_y == history.GetUndoHint())
				{
					// undo hint here
					if (taseditor_config.show_markers && markers_manager.GetMarker(cell_y))
					{
						msg->clrTextBk = (taseditor_config.bind_markers) ? BINDMARKED_UNDOHINT_FRAMENUM_COLOR : MARKED_UNDOHINT_FRAMENUM_COLOR;
					} else
					{
						msg->clrTextBk = UNDOHINT_FRAMENUM_COLOR;
					}
				} else if (cell_y == currFrameCounter || cell_y == (playback.GetFlashingPauseFrame() - 1))
				{
					// current frame
					if (taseditor_config.show_markers && markers_manager.GetMarker(cell_y))
					{
						msg->clrTextBk = (taseditor_config.bind_markers) ? CUR_BINDMARKED_FRAMENUM_COLOR : CUR_MARKED_FRAMENUM_COLOR;
					} else
					{
						msg->clrTextBk = CUR_FRAMENUM_COLOR;
					}
				} else if (taseditor_config.show_markers && markers_manager.GetMarker(cell_y))
				{
					// marked frame
					msg->clrTextBk = (taseditor_config.bind_markers) ? BINDMARKED_FRAMENUM_COLOR : MARKED_FRAMENUM_COLOR;
				} else
				{
					if(cell_y < greenzone.greenZoneCount)
					{
						if (!greenzone.savestates[cell_y].empty())
						{
							if (taseditor_config.show_lag_frames && greenzone.lag_history[cell_y])
								msg->clrTextBk = LAG_FRAMENUM_COLOR;
							else
								msg->clrTextBk = GREENZONE_FRAMENUM_COLOR;
						} else if ((!greenzone.savestates[cell_y & EVERY16TH].empty() && (int)greenzone.savestates.size() > (cell_y | 0xF) + 1 && !greenzone.savestates[(cell_y | 0xF) + 1].empty())
							|| (!greenzone.savestates[cell_y & EVERY8TH].empty() && (int)greenzone.savestates.size() > (cell_y | 0x7) + 1 && !greenzone.savestates[(cell_y | 0x7) + 1].empty())
							|| (!greenzone.savestates[cell_y & EVERY4TH].empty() && (int)greenzone.savestates.size() > (cell_y | 0x3) + 1 && !greenzone.savestates[(cell_y | 0x3) + 1].empty())
							|| (!greenzone.savestates[cell_y & EVERY2ND].empty() && !greenzone.savestates[(cell_y | 0x1) + 1].empty()))
						{
							if (taseditor_config.show_lag_frames && greenzone.lag_history[cell_y])
								msg->clrTextBk = PALE_LAG_FRAMENUM_COLOR;
							else
								msg->clrTextBk = PALE_GREENZONE_FRAMENUM_COLOR;
						} else msg->clrTextBk = NORMAL_FRAMENUM_COLOR;
					} else msg->clrTextBk = NORMAL_FRAMENUM_COLOR;
				}
			} else if((cell_x - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS == 0 || (cell_x - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS == 2)
			{
				// pad 1 or 3
				// font
				SelectObject(msg->nmcd.hdc, hMainListFont);
				// bg
				if (cell_y == history.GetUndoHint())
				{
					// undo hint here
					msg->clrTextBk = UNDOHINT_INPUT_COLOR1;
				} else if (cell_y == currFrameCounter || cell_y == (playback.GetFlashingPauseFrame() - 1))
				{
					// current frame
					msg->clrTextBk = CUR_INPUT_COLOR1;
				} else if(cell_y < greenzone.greenZoneCount)
				{
					if (!greenzone.savestates[cell_y].empty())
					{
						if (taseditor_config.show_lag_frames && greenzone.lag_history[cell_y])
							msg->clrTextBk = LAG_INPUT_COLOR1;
						else
							msg->clrTextBk = GREENZONE_INPUT_COLOR1;
					} else if ((!greenzone.savestates[cell_y & EVERY16TH].empty() && (int)greenzone.savestates.size() > (cell_y | 0xF) + 1 && !greenzone.savestates[(cell_y | 0xF) + 1].empty())
						|| (!greenzone.savestates[cell_y & EVERY8TH].empty() && (int)greenzone.savestates.size() > (cell_y | 0x7) + 1 && !greenzone.savestates[(cell_y | 0x7) + 1].empty())
						|| (!greenzone.savestates[cell_y & EVERY4TH].empty() && (int)greenzone.savestates.size() > (cell_y | 0x3) + 1 && !greenzone.savestates[(cell_y | 0x3) + 1].empty())
						|| (!greenzone.savestates[cell_y & EVERY2ND].empty() && !greenzone.savestates[(cell_y | 0x1) + 1].empty()))
					{
						if (taseditor_config.show_lag_frames && greenzone.lag_history[cell_y])
							msg->clrTextBk = PALE_LAG_INPUT_COLOR1;
						else
							msg->clrTextBk = PALE_GREENZONE_INPUT_COLOR1;
					} else msg->clrTextBk = NORMAL_INPUT_COLOR1;
				} else msg->clrTextBk = NORMAL_INPUT_COLOR1;
			} else if((cell_x - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS == 1 || (cell_x - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS == 3)
			{
				// pad 2 or 4
				// font
				SelectObject(msg->nmcd.hdc, hMainListFont);
				// bg
				if (cell_y == history.GetUndoHint())
				{
					// undo hint here
					msg->clrTextBk = UNDOHINT_INPUT_COLOR2;
				} else if (cell_y == currFrameCounter || cell_y == (playback.GetFlashingPauseFrame() - 1))
				{
					// current frame
					msg->clrTextBk = CUR_INPUT_COLOR2;
				} else if(cell_y < greenzone.greenZoneCount)
				{
					if (!greenzone.savestates[cell_y].empty())
					{
						if (taseditor_config.show_lag_frames && greenzone.lag_history[cell_y])
							msg->clrTextBk = LAG_INPUT_COLOR2;
						else
							msg->clrTextBk = GREENZONE_INPUT_COLOR2;
					} else if ((!greenzone.savestates[cell_y & EVERY16TH].empty() && (int)greenzone.savestates.size() > (cell_y | 0xF) + 1 && !greenzone.savestates[(cell_y | 0xF) + 1].empty())
						|| (!greenzone.savestates[cell_y & EVERY8TH].empty() && (int)greenzone.savestates.size() > (cell_y | 0x7) + 1 && !greenzone.savestates[(cell_y | 0x7) + 1].empty())
						|| (!greenzone.savestates[cell_y & EVERY4TH].empty() && (int)greenzone.savestates.size() > (cell_y | 0x3) + 1 && !greenzone.savestates[(cell_y | 0x3) + 1].empty())
						|| (!greenzone.savestates[cell_y & EVERY2ND].empty() && !greenzone.savestates[(cell_y | 0x1) + 1].empty()))
					{
						if (taseditor_config.show_lag_frames && greenzone.lag_history[cell_y])
							msg->clrTextBk = PALE_LAG_INPUT_COLOR2;
						else
							msg->clrTextBk = PALE_GREENZONE_INPUT_COLOR2;
					} else msg->clrTextBk = NORMAL_INPUT_COLOR2;
				} else msg->clrTextBk = NORMAL_INPUT_COLOR2;
			}
		}
	default:
		return CDRF_DODEFAULT;
	}
}

LONG PIANO_ROLL::HeaderCustomDraw(NMLVCUSTOMDRAW* msg)
{
	switch(msg->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		SelectObject(msg->nmcd.hdc, hMainListFont);
		return CDRF_NOTIFYITEMDRAW;
	case CDDS_ITEMPREPAINT:
		{
			int cell_x = msg->nmcd.dwItemSpec;
			if (cell_x < num_columns)
			{
				int cur_color = header_colors[cell_x];
				if (cur_color)
					SetTextColor(msg->nmcd.hdc, header_lights_colors[cur_color]);
			}
		}
	default:
		return CDRF_DODEFAULT;
	}
}

void PIANO_ROLL::ToggleJoypadBit(int column_index, int row_index, UINT KeyFlags)
{
	int joy = (column_index - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS;
	int bit = (column_index - COLUMN_JOYPAD1_A) % NUM_JOYPAD_BUTTONS;
	if (KeyFlags & (MK_SHIFT|MK_CONTROL))
	{
		// update multiple rows, using last row index as a flag to decide operation
		SelectionFrames* current_selection = selection.MakeStrobe();
		SelectionFrames::iterator current_selection_end(current_selection->end());
		if (current_selection->size())
		{
			if (currMovieData.records[row_index].checkBit(joy, bit))
			{
				// clear range
				for(SelectionFrames::iterator it(current_selection->begin()); it != current_selection_end; it++)
				{
					currMovieData.records[*it].clearBit(joy, bit);
				}
				greenzone.InvalidateAndCheck(history.RegisterChanges(MODTYPE_UNSET, *current_selection->begin(), *current_selection->rbegin()));
			} else
			{
				// set range
				for(SelectionFrames::iterator it(current_selection->begin()); it != current_selection_end; it++)
				{
					currMovieData.records[*it].setBit(joy, bit);
				}
				greenzone.InvalidateAndCheck(history.RegisterChanges(MODTYPE_SET, *current_selection->begin(), *current_selection->rbegin()));
			}
		}
	} else
	{
		// update one row
		currMovieData.records[row_index].toggleBit(joy, bit);
		if (currMovieData.records[row_index].checkBit(joy, bit))
			greenzone.InvalidateAndCheck(history.RegisterChanges(MODTYPE_SET, row_index, row_index));
		else
			greenzone.InvalidateAndCheck(history.RegisterChanges(MODTYPE_UNSET, row_index, row_index));
	}
	
}

void PIANO_ROLL::ColumnSet(int column, bool alt_pressed)
{
	if (column == COLUMN_FRAMENUM || column == COLUMN_FRAMENUM2)
	{
		// user clicked on "Frame#" - apply ColumnSet to Markers
		if (alt_pressed)
		{
			if (FrameColumnSetPattern())
				SetHeaderColumnLight(COLUMN_FRAMENUM, HEADER_LIGHT_MAX);
		} else
		{
			if (FrameColumnSet())
				SetHeaderColumnLight(COLUMN_FRAMENUM, HEADER_LIGHT_MAX);
		}
	} else
	{
		// user clicked on Input column - apply ColumnSet to Input
		int joy = (column - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS;
		int button = (column - COLUMN_JOYPAD1_A) % NUM_JOYPAD_BUTTONS;
		if (alt_pressed)
		{
			if (InputColumnSetPattern(joy, button))
				SetHeaderColumnLight(column, HEADER_LIGHT_MAX);
		} else
		{
			if (InputColumnSet(joy, button))
				SetHeaderColumnLight(column, HEADER_LIGHT_MAX);
		}
	}
}

bool PIANO_ROLL::FrameColumnSetPattern()
{
	SelectionFrames* current_selection = selection.MakeStrobe();
	if (current_selection->size() == 0) return false;
	SelectionFrames::iterator current_selection_begin(current_selection->begin());
	SelectionFrames::iterator current_selection_end(current_selection->end());
	int pattern_offset = 0, current_pattern = taseditor_config.current_pattern;
	bool changes_made = false;

	for(SelectionFrames::iterator it(current_selection_begin); it != current_selection_end; it++)
	{
		// skip lag frames
		if (taseditor_config.pattern_skips_lag && greenzone.GetLagHistoryAtFrame(*it))
			continue;
		if (autofire_patterns[current_pattern][pattern_offset])
		{
			if(!markers_manager.GetMarker(*it))
			{
				if (markers_manager.SetMarker(*it))
				{
					changes_made = true;
					RedrawRow(*it);
				}
			}
		} else
		{
			if(markers_manager.GetMarker(*it))
			{
				markers_manager.ClearMarker(*it);
				changes_made = true;
				RedrawRow(*it);
			}
		}
		pattern_offset++;
		if (pattern_offset >= (int)autofire_patterns[current_pattern].size())
			pattern_offset -= autofire_patterns[current_pattern].size();
	}
	if (changes_made)
	{
		history.RegisterMarkersChange(MODTYPE_MARKER_PATTERN, *current_selection_begin, *current_selection->rbegin(), autofire_patterns_names[current_pattern].c_str());
		selection.must_find_current_marker = playback.must_find_current_marker = true;
		return true;
	} else
		return false;
}
bool PIANO_ROLL::FrameColumnSet()
{
	SelectionFrames* current_selection = selection.MakeStrobe();
	if (current_selection->size() == 0) return false;
	SelectionFrames::iterator current_selection_begin(current_selection->begin());
	SelectionFrames::iterator current_selection_end(current_selection->end());

	// inspect the selected frames, if they are all set, then unset all, else set all
	bool unset_found = false, changes_made = false;
	for(SelectionFrames::iterator it(current_selection_begin); it != current_selection_end; it++)
	{
		if(!markers_manager.GetMarker(*it))
		{
			unset_found = true;
			break;
		}
	}
	if (unset_found)
	{
		// set all
		for(SelectionFrames::iterator it(current_selection_begin); it != current_selection_end; it++)
		{
			if(!markers_manager.GetMarker(*it))
			{
				if (markers_manager.SetMarker(*it))
				{
					changes_made = true;
					RedrawRow(*it);
				}
			}
		}
		if (changes_made)
			history.RegisterMarkersChange(MODTYPE_MARKER_SET, *current_selection_begin, *current_selection->rbegin());
	} else
	{
		// unset all
		for(SelectionFrames::iterator it(current_selection_begin); it != current_selection_end; it++)
		{
			if(markers_manager.GetMarker(*it))
			{
				markers_manager.ClearMarker(*it);
				changes_made = true;
				RedrawRow(*it);
			}
		}
		if (changes_made)
			history.RegisterMarkersChange(MODTYPE_MARKER_UNSET, *current_selection_begin, *current_selection->rbegin());
	}
	if (changes_made)
		selection.must_find_current_marker = playback.must_find_current_marker = true;
	return changes_made;
}
bool PIANO_ROLL::InputColumnSetPattern(int joy, int button)
{
	if (joy < 0 || joy >= joysticks_per_frame[GetInputType(currMovieData)]) return false;

	SelectionFrames* current_selection = selection.MakeStrobe();
	if (current_selection->size() == 0) return false;
	SelectionFrames::iterator current_selection_begin(current_selection->begin());
	SelectionFrames::iterator current_selection_end(current_selection->end());
	int pattern_offset = 0, current_pattern = taseditor_config.current_pattern;

	for(SelectionFrames::iterator it(current_selection_begin); it != current_selection_end; it++)
	{
		// skip lag frames
		if (taseditor_config.pattern_skips_lag && greenzone.GetLagHistoryAtFrame(*it))
			continue;
		currMovieData.records[*it].setBitValue(joy, button, autofire_patterns[current_pattern][pattern_offset] != 0);
		pattern_offset++;
		if (pattern_offset >= (int)autofire_patterns[current_pattern].size())
			pattern_offset -= autofire_patterns[current_pattern].size();
	}
	int first_changes = history.RegisterChanges(MODTYPE_PATTERN, *current_selection_begin, *current_selection->rbegin(), autofire_patterns_names[current_pattern].c_str());
	if (first_changes >= 0)
	{
		greenzone.InvalidateAndCheck(first_changes);
		return true;
	} else
		return false;
}
bool PIANO_ROLL::InputColumnSet(int joy, int button)
{
	if (joy < 0 || joy >= joysticks_per_frame[GetInputType(currMovieData)]) return false;

	SelectionFrames* current_selection = selection.MakeStrobe();
	if (current_selection->size() == 0) return false;
	SelectionFrames::iterator current_selection_begin(current_selection->begin());
	SelectionFrames::iterator current_selection_end(current_selection->end());

	//inspect the selected frames, if they are all set, then unset all, else set all
	bool newValue = false;
	for(SelectionFrames::iterator it(current_selection_begin); it != current_selection_end; it++)
	{
		if(!(currMovieData.records[*it].checkBit(joy,button)))
		{
			newValue = true;
			break;
		}
	}
	// apply newValue
	for(SelectionFrames::iterator it(current_selection_begin); it != current_selection_end; it++)
		currMovieData.records[*it].setBitValue(joy,button,newValue);

	int first_changes;
	if (newValue)
	{
		first_changes = history.RegisterChanges(MODTYPE_SET, *current_selection_begin, *current_selection->rbegin());
	} else
	{
		first_changes = history.RegisterChanges(MODTYPE_UNSET, *current_selection_begin, *current_selection->rbegin());
	}
	if (first_changes >= 0)
	{
		greenzone.InvalidateAndCheck(first_changes);
		return true;
	} else
		return false;
}
// ----------------------------------------------------
void PIANO_ROLL::RightClick(LVHITTESTINFO& info)
{
	int index = info.iItem;
	if(index == -1)
		StrayClickMenu(info);
	else
		RightClickMenu(info);
}
void PIANO_ROLL::StrayClickMenu(LVHITTESTINFO& info)
{
	POINT pt = info.pt;
	ClientToScreen(hwndList, &pt);
	HMENU sub = GetSubMenu(hrmenu, CONTEXTMENU_STRAY);
	TrackPopupMenu(sub, 0, pt.x, pt.y, 0, taseditor_window.hwndTasEditor, 0);
}
void PIANO_ROLL::RightClickMenu(LVHITTESTINFO& info)
{
	SelectionFrames* current_selection = selection.MakeStrobe();
	if (current_selection->size() == 0)
	{
		StrayClickMenu(info);
		return;
	}
	HMENU sub = GetSubMenu(hrmenu, CONTEXTMENU_SELECTED);
	// inspect current selection and disable inappropriate menu items
	SelectionFrames::iterator current_selection_begin(current_selection->begin());
	SelectionFrames::iterator current_selection_end(current_selection->end());
	bool set_found = false, unset_found = false;
	for(SelectionFrames::iterator it(current_selection_begin); it != current_selection_end; it++)
	{
		if(markers_manager.GetMarker(*it))
			set_found = true;
		else 
			unset_found = true;
	}
	if (set_found)
		EnableMenuItem(sub, ID_SELECTED_REMOVEMARKER, MF_BYCOMMAND | MF_ENABLED);
	else
		EnableMenuItem(sub, ID_SELECTED_REMOVEMARKER, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	if (unset_found)
		EnableMenuItem(sub, ID_SELECTED_SETMARKER, MF_BYCOMMAND | MF_ENABLED);
	else
		EnableMenuItem(sub, ID_SELECTED_SETMARKER, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

	POINT pt = info.pt;
	ClientToScreen(hwndList, &pt);
	TrackPopupMenu(sub, 0, pt.x, pt.y, 0, taseditor_window.hwndTasEditor, 0);
}
// -------------------------------------------------------------------------
LRESULT APIENTRY HeaderWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	extern PIANO_ROLL piano_roll;
	switch(msg)
	{
	case WM_SETCURSOR:
		// no column resizing cursor, always show arrow
		SetCursor(LoadCursor(0, IDC_ARROW));
		return true;
	case WM_MOUSEMOVE:
	{
		// perform hit test
		HD_HITTESTINFO info;
		info.pt.x = GET_X_LPARAM(lParam) + HEADER_DX_FIX;
		info.pt.y = GET_Y_LPARAM(lParam);
		SendMessage(hWnd, HDM_HITTEST, 0, (LPARAM)&info);
		piano_roll.header_item_under_mouse = info.iItem;
		// ensure that WM_MOUSELEAVE will be catched
		TrackMouseEvent(&piano_roll.tme);
		break;
	}
	case WM_MOUSELEAVE:
	{
		piano_roll.header_item_under_mouse = -1;
		break;
	}
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
		{
			if (selection.GetCurrentSelectionSize())
			{
				// perform hit test
				HD_HITTESTINFO info;
				info.pt.x = GET_X_LPARAM(lParam) + HEADER_DX_FIX;
				info.pt.y = GET_Y_LPARAM(lParam);
				SendMessage(hWnd, HDM_HITTEST, 0, (LPARAM)&info);
				if(info.iItem >= COLUMN_FRAMENUM && info.iItem <= COLUMN_FRAMENUM2)
					piano_roll.ColumnSet(info.iItem, (GetKeyState(VK_MENU) < 0));
			}
		}
		return true;
	}
	return CallWindowProc(hwndHeader_oldWndproc, hWnd, msg, wParam, lParam);
}

// The subclass wndproc for the listview
LRESULT APIENTRY ListWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	extern PIANO_ROLL piano_roll;
	switch(msg)
	{
		case WM_CHAR:
		case WM_KILLFOCUS:
			return 0;
		case WM_NOTIFY:
		{
			if (((LPNMHDR)lParam)->hwndFrom == piano_roll.hwndHeader)
			{
				switch (((LPNMHDR)lParam)->code)
				{
				case HDN_BEGINTRACKW:
				case HDN_BEGINTRACKA:
				case HDN_TRACK:
					return true;	// no column resizing
				case NM_CUSTOMDRAW:
					return piano_roll.HeaderCustomDraw((NMLVCUSTOMDRAW*)lParam);
				}
			}
			break;
		}
		case WM_KEYDOWN:
		{
			if (!taseditor_config.keyboard_for_piano_roll)
				return 0;
			break;
		}
		case WM_TIMER:
			// disable timer of entering edit mode (there's no edit mode anyway)
			return 0;
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		{
			bool alt_pressed = (GetKeyState(VK_MENU) < 0);
			int fwKeys = GET_KEYSTATE_WPARAM(wParam);
			// perform hit test
			LVHITTESTINFO info;
			info.pt.x = GET_X_LPARAM(lParam);
			info.pt.y = GET_Y_LPARAM(lParam);
			ListView_SubItemHitTest(hWnd, (LPARAM)&info);
			int row_index = info.iItem;
			int column_index = info.iSubItem;
			if(row_index >= 0)
			{
				if(column_index == COLUMN_ICONS)
				{
					// click on the "icons" column - jump to the frame
					selection.ClearSelection();
					playback.jump(row_index);
				} else if(column_index == COLUMN_FRAMENUM || column_index == COLUMN_FRAMENUM2)
				{
					// clicked on the "Frame#" column
					if (msg == WM_LBUTTONDBLCLK && !alt_pressed)
					{
						// doubleclick - jump to the frame
						selection.ClearSelection();
						playback.jump(row_index);
					} else
					{
						// set marker if clicked with Alt
						if (alt_pressed)
						{
							markers_manager.ToggleMarker(row_index);
							selection.must_find_current_marker = playback.must_find_current_marker = true;
							if (markers_manager.GetMarker(row_index))
								history.RegisterMarkersChange(MODTYPE_MARKER_SET, row_index);
							else
								history.RegisterMarkersChange(MODTYPE_MARKER_UNSET, row_index);
							piano_roll.RedrawRow(row_index);
						}
						// also select the row by calling hwndList_oldWndProc
						PostMessage(hWnd, WM_LBUTTONUP, 0, 0);		// ensure that oldWndProc will exit its modal message loop immediately
						CallWindowProc(hwndList_oldWndProc, hWnd, msg, wParam, lParam);
					}
				} else if(column_index >= COLUMN_JOYPAD1_A && column_index <= COLUMN_JOYPAD4_R)
				{
					// clicked on input
					// first call old wndproc to set selection on the row
					if (alt_pressed)
						wParam |= MK_SHIFT;		// Alt should select region, just like Shift
					if (msg == WM_LBUTTONDOWN)
						PostMessage(hWnd, WM_LBUTTONUP, 0, 0);		// ensure that oldWndProc will exit its modal message loop immediately
					CallWindowProc(hwndList_oldWndProc, hWnd, msg, wParam, lParam);
					if (alt_pressed)
					{
						int joy = (column_index - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS;
						int button = (column_index - COLUMN_JOYPAD1_A) % NUM_JOYPAD_BUTTONS;
						piano_roll.InputColumnSetPattern(joy, button);
					} else
					{
						piano_roll.ToggleJoypadBit(column_index, row_index, GET_KEYSTATE_WPARAM(wParam));
					}
				}
			}
			return 0;
		}
		case WM_MBUTTONDOWN:
		case WM_MBUTTONDBLCLK:
		{
			playback.MiddleButtonClick();
			return 0;
		}
		case WM_MOUSEWHEEL:
		{
			bool alt_pressed = (GetKeyState(VK_MENU) < 0);
			int fwKeys = GET_KEYSTATE_WPARAM(wParam);
			int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
			if (fwKeys & MK_SHIFT)
			{
				// Shift + wheel = Playback rewind full(speed)/forward full(speed)
				if (zDelta < 0)
					playback.ForwardFull(-zDelta / 120);
				else if (zDelta > 0)
					playback.RewindFull(zDelta / 120);
				return 0;
			} else if (fwKeys & MK_CONTROL)
			{
				// Ctrl + wheel = Selection rewind full(speed)/forward full(speed)
				if (zDelta < 0)
					selection.JumpNextMarker(-zDelta / 120);
				else if (zDelta > 0)
					selection.JumpPrevMarker(zDelta / 120);
				return 0;
			} else if (alt_pressed || fwKeys & MK_RBUTTON)
			{
				// Right button + wheel = Alt + wheel = rewind/forward
				// if both Right button and Alt are pressed, move 2x faster
				if (alt_pressed && fwKeys & MK_RBUTTON)
					zDelta *= BOOST_WHEN_BOTH_RIGHTBUTTON_AND_ALT_PRESSED;
				int destination_frame = currFrameCounter - (zDelta / 120);
				if (destination_frame < 0) destination_frame = 0;
				int lastCursor = currFrameCounter;
				playback.jump(destination_frame);
				if (lastCursor != currFrameCounter)
				{
					// redraw row where Playback cursor was (in case there's two or more WM_MOUSEWHEEL messages before playback.update())
					piano_roll.RedrawRow(lastCursor);
					bookmarks.RedrawChangedBookmarks(lastCursor);
				}
				return 0;
			}
			break;
		}
		case WM_RBUTTONDOWN:
		case WM_RBUTTONDBLCLK:
			if (GetFocus() != hWnd)
				SetFocus(hWnd);
			return 0;
		case WM_RBUTTONUP:
		{
			// perform hit test
			LVHITTESTINFO info;
			info.pt.x = GET_X_LPARAM(lParam);
			info.pt.y = GET_Y_LPARAM(lParam);
			ListView_SubItemHitTest(hWnd, (LPARAM)&info);
			// show context menu if user right-clicked on Frame#
			if(info.iSubItem <= COLUMN_FRAMENUM || info.iSubItem >= COLUMN_FRAMENUM2)
				piano_roll.RightClick(info);
			return 0;
		}
        case WM_MOUSEACTIVATE:
			if (GetFocus() != hWnd)
				SetFocus(hWnd);
            break;

	}
	return CallWindowProc(hwndList_oldWndProc, hWnd, msg, wParam, lParam);
}
