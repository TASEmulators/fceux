//Implementation file of TASEDITOR_LIST class
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
extern TASEDITOR_SELECTION selection;

extern int GetInputType(MovieData& md);

LRESULT APIENTRY HeaderWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY ListWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
WNDPROC hwndList_oldWndProc = 0, hwndHeader_oldWndproc = 0;

// resources
char list_save_id[LIST_ID_LEN] = "LIST";
char list_skipsave_id[LIST_ID_LEN] = "LISX";
COLORREF hot_changes_colors[16] = { 0x0, 0x5c4c44, 0x854604, 0xab2500, 0xc20006, 0xd6006f, 0xd40091, 0xba00a4, 0x9500ba, 0x7a00cc, 0x5800d4, 0x0045e2, 0x0063ea, 0x0079f4, 0x0092fa, 0x00aaff };
//COLORREF hot_changes_colors[16] = { 0x0, 0x661212, 0x842B4E, 0x652C73, 0x48247D, 0x383596, 0x2947AE, 0x1E53C1, 0x135DD2, 0x116EDA, 0x107EE3, 0x0F8EEB, 0x209FF4, 0x3DB1FD, 0x51C2FF, 0x4DCDFF };
COLORREF header_lights_colors[11] = { 0x0, 0x006311, 0x008500, 0x1dad00, 0x46d100, 0x6ee300, 0x97e800, 0xb8f000, 0xdaf700, 0xffff7e, 0xffffb7 };

TASEDITOR_LIST::TASEDITOR_LIST()
{
}

void TASEDITOR_LIST::init()
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

	header_colors.resize(TOTAL_COLUMNS);

	hwndList = GetDlgItem(taseditor_window.hwndTasEditor, IDC_LIST1);
	// prepare the main listview
	ListView_SetExtendedListViewStyleEx(hwndList, LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_INFOTIP, LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_INFOTIP);
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

}
void TASEDITOR_LIST::free()
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
void TASEDITOR_LIST::reset()
{
	next_header_update_time = 0;
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
void TASEDITOR_LIST::update()
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
		// 1 - update Frame# columns' heads
		if (header_colors[COLUMN_FRAMENUM] > HEADER_LIGHT_HOLD)
		{
			header_colors[COLUMN_FRAMENUM]--;
			changes_made = true;
		} else
		{
			if ((GetAsyncKeyState(VK_MENU) & 0x8000))
			{
				// Alt key is held
				if (header_colors[COLUMN_FRAMENUM] < HEADER_LIGHT_HOLD)
				{
					header_colors[COLUMN_FRAMENUM]++;
					changes_made = true;
				}
			} else
			{
				// Alt key is released
				if (header_colors[COLUMN_FRAMENUM])
				{
					header_colors[COLUMN_FRAMENUM]--;
					changes_made = true;
				}
			}
		}
		header_colors[COLUMN_FRAMENUM2] = header_colors[COLUMN_FRAMENUM];
		// update input columns' heads
		int i = num_columns-1;
		if (i == COLUMN_FRAMENUM2) i--;
		for (; i >= COLUMN_JOYPAD1_A; i--)
		{
			if (header_colors[i] > HEADER_LIGHT_HOLD)
			{
				header_colors[i]--;
				changes_made = true;
			} else
			{
				if (recorder.current_joy[(i - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS] & (1 << ((i - COLUMN_JOYPAD1_A) % NUM_JOYPAD_BUTTONS)))
				{
					// the button is held
					if (header_colors[i] < HEADER_LIGHT_HOLD)
					{
						header_colors[i]++;
						changes_made = true;
					}
				} else
				{
					// the button is released
					if (header_colors[i])
					{
						header_colors[i]--;
						changes_made = true;
					}
				}
			}	
		}
		if (changes_made)
			RedrawHeader();
	}
}

void TASEDITOR_LIST::save(EMUFILE *os, bool really_save)
{
	if (really_save)
	{
		update();
		// write "LIST" string
		os->fwrite(list_save_id, LIST_ID_LEN);
		// write current top item
		int top_item = ListView_GetTopIndex(hwndList);
		write32le(top_item, os);
	} else
	{
		// write "LISX" string
		os->fwrite(list_skipsave_id, LIST_ID_LEN);
	}
}
// returns true if couldn't load
bool TASEDITOR_LIST::load(EMUFILE *is)
{
	reset();
	update();
	// read "LIST" string
	char save_id[LIST_ID_LEN];
	if ((int)is->fread(save_id, LIST_ID_LEN) < LIST_ID_LEN) goto error;
	if (!strcmp(list_skipsave_id, save_id))
	{
		// string says to skip loading List
		FCEU_printf("No list data in the file\n");
		// scroll to the beginning
		ListView_EnsureVisible(hwndList, 0, FALSE);
		return false;
	}
	if (strcmp(list_save_id, save_id)) goto error;		// string is not valid
	// read current top item and scroll list there
	int top_item;
	if (!read32le(&top_item, is)) goto error;
	ListView_EnsureVisible(hwndList, currMovieData.getNumRecords() - 1, FALSE);
	ListView_EnsureVisible(hwndList, top_item, FALSE);
	return false;
error:
	FCEU_printf("Error loading list data\n");
	// scroll to the beginning
	ListView_EnsureVisible(hwndList, 0, FALSE);
	return true;
}
// ----------------------------------------------------------------------
void TASEDITOR_LIST::RedrawList()
{
	InvalidateRect(hwndList, 0, FALSE);
}
void TASEDITOR_LIST::RedrawRow(int index)
{
	ListView_RedrawItems(hwndList, index, index);
}
void TASEDITOR_LIST::RedrawHeader()
{
	InvalidateRect(hwndHeader, 0, FALSE);
}

// -------------------------------------------------------------------------
bool TASEDITOR_LIST::CheckItemVisible(int frame)
{
	int top = ListView_GetTopIndex(hwndList);
	// in fourscore there's horizontal scrollbar which takes one row for itself
	if (frame >= top && frame < top + ListView_GetCountPerPage(hwndList))
		return true;
	return false;
}

void TASEDITOR_LIST::CenterListAt(int frame)
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

void TASEDITOR_LIST::FollowPlayback()
{
	CenterListAt(currFrameCounter);
}
void TASEDITOR_LIST::FollowPlaybackIfNeeded()
{
	if (taseditor_config.follow_playback) ListView_EnsureVisible(hwndList,currFrameCounter,FALSE);
}
void TASEDITOR_LIST::FollowUndo()
{
	int jump_frame = history.GetUndoHint();
	if (taseditor_config.jump_to_undo && jump_frame >= 0)
	{
		if (!CheckItemVisible(jump_frame))
			CenterListAt(jump_frame);
	}
}
void TASEDITOR_LIST::FollowSelection()
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
void TASEDITOR_LIST::FollowPauseframe()
{
	if (playback.pause_frame > 0)
		CenterListAt(playback.pause_frame - 1);
}
void TASEDITOR_LIST::FollowMarker(int marker_id)
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

void TASEDITOR_LIST::SetHeaderColumnLight(int column, int level)
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

void TASEDITOR_LIST::GetDispInfo(NMLVDISPINFO* nmlvDispInfo)
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

LONG TASEDITOR_LIST::CustomDraw(NMLVCUSTOMDRAW* msg)
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

LONG TASEDITOR_LIST::HeaderCustomDraw(NMLVCUSTOMDRAW* msg)
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

void TASEDITOR_LIST::SingleClick(LPNMITEMACTIVATE info)
{
	int row_index = info->iItem;
	if(row_index == -1) return;
	int column_index = info->iSubItem;

	if(column_index == COLUMN_ICONS)
	{
		// click on the "icons" column - jump to the frame
		selection.ClearSelection();
		playback.jump(row_index);
	} else if(column_index == COLUMN_FRAMENUM || column_index == COLUMN_FRAMENUM2)
	{
		// click on the "frame number" column - set marker if clicked with Alt
		if (info->uKeyFlags & LVKF_ALT)
		{
			// reverse MARKER_FLAG_BIT in pointed frame
			markers_manager.ToggleMarker(row_index);
			selection.must_find_current_marker = playback.must_find_current_marker = true;
			if (markers_manager.GetMarker(row_index))
				history.RegisterMarkersChange(MODTYPE_MARKER_SET, row_index);
			else
				history.RegisterMarkersChange(MODTYPE_MARKER_UNSET, row_index);
			RedrawRow(row_index);
		}
	}
	else if(column_index >= COLUMN_JOYPAD1_A && column_index <= COLUMN_JOYPAD4_R)
	{
		ToggleJoypadBit(column_index, row_index, info->uKeyFlags);
	}
}
void TASEDITOR_LIST::DoubleClick(LPNMITEMACTIVATE info)
{
	int row_index = info->iItem;
	if(row_index == -1) return;
	int column_index = info->iSubItem;

	if(column_index == COLUMN_ICONS || column_index == COLUMN_FRAMENUM || column_index == COLUMN_FRAMENUM2)
	{
		// double click sends playback to the frame
		selection.ClearSelection();
		playback.jump(row_index);
	} else if(column_index >= COLUMN_JOYPAD1_A && column_index <= COLUMN_JOYPAD4_R)
	{
		ToggleJoypadBit(column_index, row_index, info->uKeyFlags);
	}
}

void TASEDITOR_LIST::ToggleJoypadBit(int column_index, int row_index, UINT KeyFlags)
{
	int joy = (column_index - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS;
	int bit = (column_index - COLUMN_JOYPAD1_A) % NUM_JOYPAD_BUTTONS;
	if (KeyFlags & (LVKF_SHIFT|LVKF_CONTROL))
	{
		// update multiple rows, using last row index as a flag to decide operation
		SelectionFrames* current_selection = selection.MakeStrobe();
		SelectionFrames::iterator current_selection_end(current_selection->end());
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

void TASEDITOR_LIST::ColumnSet(int column, bool alt_pressed)
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

bool TASEDITOR_LIST::FrameColumnSetPattern()
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
bool TASEDITOR_LIST::FrameColumnSet()
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
bool TASEDITOR_LIST::InputColumnSetPattern(int joy, int button)
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
bool TASEDITOR_LIST::InputColumnSet(int joy, int button)
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
void TASEDITOR_LIST::RightClick(LVHITTESTINFO& info)
{
	int index = info.iItem;
	if(index == -1)
		StrayClickMenu(info);
	else
		RightClickMenu(info);
}
void TASEDITOR_LIST::StrayClickMenu(LVHITTESTINFO& info)
{
	POINT pt = info.pt;
	ClientToScreen(hwndList, &pt);
	HMENU sub = GetSubMenu(hrmenu, CONTEXTMENU_STRAY);
	TrackPopupMenu(sub, 0, pt.x, pt.y, 0, taseditor_window.hwndTasEditor, 0);
}
void TASEDITOR_LIST::RightClickMenu(LVHITTESTINFO& info)
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
	extern TASEDITOR_LIST list;
	switch(msg)
	{
	case WM_SETCURSOR:
		return true;	// no column resizing
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
		{
			if (selection.GetCurrentSelectionSize())
			{
				// perform hit test
				HD_HITTESTINFO info;
				info.pt.x = GET_X_LPARAM(lParam);
				info.pt.y = GET_Y_LPARAM(lParam);
				SendMessage(hWnd,HDM_HITTEST,0,(LPARAM)&info);
				if(info.iItem >= COLUMN_FRAMENUM && info.iItem <= COLUMN_FRAMENUM2)
					list.ColumnSet(info.iItem, (GetKeyState(VK_MENU) < 0));
			}
		}
		return true;
	}
	return CallWindowProc(hwndHeader_oldWndproc, hWnd, msg, wParam, lParam);
}

//The subclass wndproc for the listview
LRESULT APIENTRY ListWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	extern TASEDITOR_LIST list;
	switch(msg)
	{
		case WM_CHAR:
		case WM_KILLFOCUS:
			return 0;
		case WM_NOTIFY:
		{
			if (((LPNMHDR)lParam)->hwndFrom == list.hwndHeader)
			{
				switch (((LPNMHDR)lParam)->code)
				{
				case HDN_BEGINTRACKW:
				case HDN_BEGINTRACKA:
				case HDN_TRACK:
					return true;	// no column resizing
				case NM_CUSTOMDRAW:
					return list.HeaderCustomDraw((NMLVCUSTOMDRAW*)lParam);
				}
			}
			break;
		}
		case WM_KEYDOWN:
		{
			if (!taseditor_config.keyboard_for_listview)
				return 0;
			break;
		}
		case WM_MBUTTONDOWN:
		case WM_MBUTTONDBLCLK:
		{
			playback.MiddleButtonClick();
			return 0;
		}
		case WM_MOUSEWHEEL:
		{
			int fwKeys = GET_KEYSTATE_WPARAM(wParam);
			int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
			if (fwKeys & MK_SHIFT)
			{
				// Shift + scroll = Playback rewind full(speed)/forward full(speed)
				if (zDelta < 0)
					playback.ForwardFull(-zDelta / 120);
				else if (zDelta > 0)
					playback.RewindFull(zDelta / 120);
				return 0;
			} else if (fwKeys & MK_CONTROL)
			{
				// Ctrl + scroll = Selection rewind full(speed)/forward full(speed)
				if (zDelta < 0)
					selection.JumpNextMarker(-zDelta / 120);
				else if (zDelta > 0)
					selection.JumpPrevMarker(zDelta / 120);
				return 0;
			} else if (fwKeys & MK_RBUTTON)
			{
				// Right button + scroll = rewind/forward
				int destination_frame = currFrameCounter - (zDelta / 120);
				if (destination_frame < 0) destination_frame = 0;
				playback.jump(destination_frame);
				return 0;
			}
			break;
		}
		case WM_RBUTTONDOWN:
			if (GetFocus() != list.hwndList)
				SetFocus(list.hwndList);
			return 0;
		case WM_RBUTTONDBLCLK:
			return 0;
		case WM_RBUTTONUP:
		{
			// perform hit test
			LVHITTESTINFO info;
			info.pt.x = GET_X_LPARAM(lParam);
			info.pt.y = GET_Y_LPARAM(lParam);
			ListView_SubItemHitTest(hWnd, (LPARAM)&info);
			// show context menu
			if(info.iSubItem <= COLUMN_FRAMENUM || info.iSubItem >= COLUMN_FRAMENUM2)
				list.RightClick(info);
			return 0;
		}

	}
	return CallWindowProc(hwndList_oldWndProc, hWnd, msg, wParam, lParam);
}
