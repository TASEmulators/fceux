//Implementation file of TASEDIT_LIST class

#include "taseditproj.h"
#include "utils/xstring.h"

extern HWND hwndTasEdit;
extern char buttonNames[NUM_JOYPAD_BUTTONS][2];
extern void ColumnSet(int column);

extern BOOKMARKS bookmarks;
extern PLAYBACK playback;
extern RECORDER recorder;
extern GREENZONE greenzone;
extern INPUT_HISTORY history;
extern MARKERS markers;
extern TASEDIT_SELECTION selection;

extern bool TASEdit_enable_hot_changes;
extern bool TASEdit_show_markers;
extern bool TASEdit_show_lag_frames;
extern bool TASEdit_follow_playback;
extern bool TASEdit_jump_to_undo;

LRESULT APIENTRY HeaderWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY ListWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
WNDPROC hwndList_oldWndProc, hwndHeader_oldWndproc;

// resources
COLORREF hot_changes_colors[16] = { 0x0, 0x5c4c44, 0x854604, 0xab2500, 0xc20006, 0xd6006f, 0xd40091, 0xba00a4, 0x9500ba, 0x7a00cc, 0x5800d4, 0x0045e2, 0x0063ea, 0x0079f4, 0x0092fa, 0x00aaff };
//COLORREF hot_changes_colors[16] = { 0x0, 0x661212, 0x842B4E, 0x652C73, 0x48247D, 0x383596, 0x2947AE, 0x1E53C1, 0x135DD2, 0x116EDA, 0x107EE3, 0x0F8EEB, 0x209FF4, 0x3DB1FD, 0x51C2FF, 0x4DCDFF };

char list_save_id[LIST_ID_LEN] = "LIST";

TASEDIT_LIST::TASEDIT_LIST()
{

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

}

void TASEDIT_LIST::init()
{
	free();
	hwndList = GetDlgItem(hwndTasEdit, IDC_LIST1);
	// prepare the main listview
	ListView_SetExtendedListViewStyleEx(hwndList, LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES, LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
	// subclass the header
	hwndHeader = ListView_GetHeader(hwndList);
	hwndHeader_oldWndproc = (WNDPROC)SetWindowLong(hwndHeader, GWL_WNDPROC, (LONG)HeaderWndProc);
	// subclass the whole listview
	hwndList_oldWndProc = (WNDPROC)SetWindowLong(hwndList, GWL_WNDPROC, (LONG)ListWndProc);
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
	ListView_SetImageList(hwndList, himglist, LVSIL_SMALL);
	// setup columns
	LVCOLUMN lvc;
	int colidx=0;
	// icons column
	lvc.mask = LVCF_WIDTH;
	lvc.cx = 13;
	ListView_InsertColumn(hwndList, colidx++, &lvc);
	// frame number column
	lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT;
	lvc.fmt = LVCFMT_CENTER;
	lvc.cx = 75;
	lvc.pszText = "Frame#";
	ListView_InsertColumn(hwndList, colidx++, &lvc);
	// pads columns
	lvc.cx = 21;
	// add pads 1 and 2
	for (int joy = 0; joy < 2; ++joy)
	{
		for (int btn = 0; btn < NUM_JOYPAD_BUTTONS; ++btn)
		{
			lvc.pszText = buttonNames[btn];
			ListView_InsertColumn(hwndList, colidx++, &lvc);
		}
	}
	// add pads 3 and 4 and frame_number2
	if (currMovieData.fourscore) AddFourscore();

	listItems = ListView_GetCountPerPage(hwndList);

	update();
	reset();
}
void TASEDIT_LIST::free()
{
	if (himglist)
	{
		ImageList_Destroy(himglist);
		himglist = 0;
	}

}
void TASEDIT_LIST::reset()
{
	// scroll to the beginning
	ListView_EnsureVisible(hwndList, 0, FALSE);
}
void TASEDIT_LIST::update()
{
	//update the number of items in the list
	int currLVItemCount = ListView_GetItemCount(hwndList);
	int movie_size = currMovieData.getNumRecords();
	if(currLVItemCount != movie_size)
		ListView_SetItemCountEx(hwndList, movie_size, LVSICF_NOSCROLL|LVSICF_NOINVALIDATEALL);


}

void TASEDIT_LIST::save(EMUFILE *os)
{
	update();
	// write "LIST" string
	os->fwrite(list_save_id, LIST_ID_LEN);
	// write current top item
	int top_item = ListView_GetTopIndex(hwndList);
	write32le(top_item, os);

}
// returns true if couldn't load
bool TASEDIT_LIST::load(EMUFILE *is)
{
	update();
	// read "LIST" string
	char save_id[LIST_ID_LEN];
	if ((int)is->fread(save_id, LIST_ID_LEN) < LIST_ID_LEN) goto error;
	if (strcmp(list_save_id, save_id)) goto error;		// string is not valid
	// read current top item and scroll list there
	int top_item;
	if (!read32le(&top_item, is)) goto error;
	ListView_EnsureVisible(hwndList, currMovieData.getNumRecords() - 1, FALSE);
	ListView_EnsureVisible(hwndList, top_item, FALSE);

	return false;
error:
	return true;
}
// ----------------------------------------------------------------------
void TASEDIT_LIST::AddFourscore()
{
	// add list columns
	LVCOLUMN lvc;
	lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT;
	lvc.fmt = LVCFMT_CENTER;
	lvc.cx = 21;
	int colidx = COLUMN_JOYPAD3_A;
	for (int joy = 0; joy < 2; ++joy)
	{
		for (int btn = 0; btn < NUM_JOYPAD_BUTTONS; ++btn)
		{
			lvc.pszText = buttonNames[btn];
			ListView_InsertColumn(hwndList, colidx++, &lvc);
		}
	}
	// frame number column again
	lvc.cx = 75;
	lvc.pszText = "Frame#";
	ListView_InsertColumn(hwndList, colidx++, &lvc);
	// enable radiobuttons for 3P/4P multitracking
	EnableWindow(recorder.hwndRB_Rec3P, true);
	EnableWindow(recorder.hwndRB_Rec4P, true);
	// change eoptions
	FCEUI_SetInputFourscore(true);
}
void TASEDIT_LIST::RemoveFourscore()
{
	// remove list columns
	for (int i = COLUMN_FRAMENUM2; i >= COLUMN_JOYPAD3_A; --i)
	{
		ListView_DeleteColumn (hwndList, i);
	}
	// disable radiobuttons for 3P/4P multitracking
	EnableWindow(recorder.hwndRB_Rec3P, false);
	EnableWindow(recorder.hwndRB_Rec4P, false);
	// change eoptions
	FCEUI_SetInputFourscore(false);
}

void TASEDIT_LIST::RedrawList()
{
	InvalidateRect(hwndList, 0, FALSE);
}
void TASEDIT_LIST::RedrawRow(int index)
{
	ListView_RedrawItems(hwndList, index, index);
}


// -------------------------------------------------------------------------
bool TASEDIT_LIST::CheckItemVisible(int frame)
{
	int top = ListView_GetTopIndex(hwndList);
	// in fourscore there's horizontal scrollbar which takes one row for itself
	if (frame >= top && frame < top + listItems - (currMovieData.fourscore)?1:0)
		return true;
	return false;
}

void TASEDIT_LIST::FollowPlayback()
{
	if (TASEdit_follow_playback) ListView_EnsureVisible(hwndList,currFrameCounter,FALSE);
}
void TASEDIT_LIST::FollowUndo()
{
	int jump_frame = history.GetUndoHint();
	if (TASEdit_jump_to_undo && jump_frame >= 0)
	{
		if (!CheckItemVisible(jump_frame))
		{
			// center list at jump_frame
			int list_items = listItems;
			if (currMovieData.fourscore) list_items--;
			int lower_border = (list_items - 1) / 2;
			int upper_border = (list_items - 1) - lower_border;
			int index = jump_frame + lower_border;
			if (index >= currMovieData.getNumRecords())
				index = currMovieData.getNumRecords()-1;
			ListView_EnsureVisible(hwndList, index, false);
			index = jump_frame - upper_border;
			if (index < 0)
				index = 0;
			ListView_EnsureVisible(hwndList, index, false);
		}
	}
}
void TASEDIT_LIST::FollowSelection()
{
	SelectionFrames* current_selection = selection.MakeStrobe();
	if (current_selection->size() == 0) return;

	int list_items = listItems;
	if (currMovieData.fourscore) list_items--;
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
		// just center at selection_start
		int lower_border = (list_items - 1) / 2;
		int upper_border = (list_items - 1) - lower_border;
		int index = selection_start + lower_border;
		if (index >= currMovieData.getNumRecords())
			index = currMovieData.getNumRecords()-1;
		ListView_EnsureVisible(hwndList, index, false);
		index = selection_start - upper_border;
		if (index < 0)
			index = 0;
		ListView_EnsureVisible(hwndList, index, false);
	}
}
void TASEDIT_LIST::FollowPauseframe()
{
	int jump_frame = playback.GetPauseFrame();
	if (jump_frame >= 0)
	{
		// center list at jump_frame
		int list_items = listItems;
		if (currMovieData.fourscore) list_items--;
		int lower_border = (list_items - 1) / 2;
		int upper_border = (list_items - 1) - lower_border;
		int index = jump_frame + lower_border;
		if (index >= currMovieData.getNumRecords())
			index = currMovieData.getNumRecords()-1;
		ListView_EnsureVisible(hwndList, index, false);
		index = jump_frame - upper_border;
		if (index < 0)
			index = 0;
		ListView_EnsureVisible(hwndList, index, false);
	}
}

void TASEDIT_LIST::GetDispInfo(NMLVDISPINFO* nmlvDispInfo)
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
				uint8 data = currMovieData.records[item.iItem].joysticks[joy];
				if(data & (1<<bit))
				{
					item.pszText[0] = buttonNames[bit][0];
					item.pszText[2] = 0;
				} else
				{
					if (TASEdit_enable_hot_changes && history.GetCurrentSnapshot().GetHotChangeInfo(item.iItem, item.iSubItem - COLUMN_JOYPAD1_A))
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

LONG TASEDIT_LIST::CustomDraw(NMLVCUSTOMDRAW* msg)
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
			if(TASEdit_enable_hot_changes && cell_x >= COLUMN_JOYPAD1_A && cell_x <= COLUMN_JOYPAD4_R)
			{
				msg->clrText = hot_changes_colors[history.GetCurrentSnapshot().GetHotChangeInfo(cell_y, cell_x - COLUMN_JOYPAD1_A)];
			} else msg->clrText = NORMAL_TEXT_COLOR;
			// bg color and text font
			if(cell_x == COLUMN_FRAMENUM || cell_x == COLUMN_FRAMENUM2)
			{
				// frame number
				if (cell_y == history.GetUndoHint())
				{
					// undo hint here
					if(TASEdit_show_markers && markers.GetMarker(cell_y))
					{
						SelectObject(msg->nmcd.hdc, hMainListSelectFont);
						msg->clrTextBk = MARKED_UNDOHINT_FRAMENUM_COLOR;
					} else
					{
						SelectObject(msg->nmcd.hdc, hMainListFont);
						msg->clrTextBk = UNDOHINT_FRAMENUM_COLOR;
					}
				} else if (cell_y == currFrameCounter || cell_y == (playback.GetPauseFrame() - 1))
				{
					// current frame
					if(TASEdit_show_markers && markers.GetMarker(cell_y))
					{
						// this frame is also marked
						SelectObject(msg->nmcd.hdc, hMainListSelectFont);
						msg->clrTextBk = CUR_MARKED_FRAMENUM_COLOR;
					} else
					{
						SelectObject(msg->nmcd.hdc, hMainListFont);
						msg->clrTextBk = CUR_FRAMENUM_COLOR;
					}
				} else if(TASEdit_show_markers && markers.GetMarker(cell_y))
				{
					// marked frame
					SelectObject(msg->nmcd.hdc, hMainListSelectFont);
					msg->clrTextBk = MARKED_FRAMENUM_COLOR;
				} else
				{
					SelectObject(msg->nmcd.hdc, hMainListFont);
					if(cell_y < greenzone.greenZoneCount)
					{
						if (!greenzone.savestates[cell_y].empty())
						{
							if (TASEdit_show_lag_frames && greenzone.lag_history[cell_y])
								msg->clrTextBk = LAG_FRAMENUM_COLOR;
							else
								msg->clrTextBk = GREENZONE_FRAMENUM_COLOR;
						} else if ((!greenzone.savestates[cell_y & EVERY16TH].empty() && (int)greenzone.savestates.size() > (cell_y | 0xF) + 1 && !greenzone.savestates[(cell_y | 0xF) + 1].empty())
							|| (!greenzone.savestates[cell_y & EVERY8TH].empty() && (int)greenzone.savestates.size() > (cell_y | 0x7) + 1 && !greenzone.savestates[(cell_y | 0x7) + 1].empty())
							|| (!greenzone.savestates[cell_y & EVERY4TH].empty() && (int)greenzone.savestates.size() > (cell_y | 0x3) + 1 && !greenzone.savestates[(cell_y | 0x3) + 1].empty())
							|| (!greenzone.savestates[cell_y & EVERY2ND].empty() && !greenzone.savestates[(cell_y | 0x1) + 1].empty()))
						{
							if (TASEdit_show_lag_frames && greenzone.lag_history[cell_y])
								msg->clrTextBk = PALE_LAG_FRAMENUM_COLOR;
							else
								msg->clrTextBk = PALE_GREENZONE_FRAMENUM_COLOR;
						} else msg->clrTextBk = NORMAL_FRAMENUM_COLOR;
					} else msg->clrTextBk = NORMAL_FRAMENUM_COLOR;
				}
			} else if((cell_x - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS == 0 || (cell_x - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS == 2)
			{
				// pad 1 or 3
				SelectObject(msg->nmcd.hdc, hMainListFont);
				if (cell_y == history.GetUndoHint())
				{
					// undo hint here
					msg->clrTextBk = UNDOHINT_INPUT_COLOR1;
				} else if (cell_y == currFrameCounter || cell_y == (playback.GetPauseFrame() - 1))
				{
					// current frame
					msg->clrTextBk = CUR_INPUT_COLOR1;
				} else if(cell_y < greenzone.greenZoneCount)
				{
					if (!greenzone.savestates[cell_y].empty())
					{
						if (TASEdit_show_lag_frames && greenzone.lag_history[cell_y])
							msg->clrTextBk = LAG_INPUT_COLOR1;
						else
							msg->clrTextBk = GREENZONE_INPUT_COLOR1;
					} else if ((!greenzone.savestates[cell_y & EVERY16TH].empty() && (int)greenzone.savestates.size() > (cell_y | 0xF) + 1 && !greenzone.savestates[(cell_y | 0xF) + 1].empty())
						|| (!greenzone.savestates[cell_y & EVERY8TH].empty() && (int)greenzone.savestates.size() > (cell_y | 0x7) + 1 && !greenzone.savestates[(cell_y | 0x7) + 1].empty())
						|| (!greenzone.savestates[cell_y & EVERY4TH].empty() && (int)greenzone.savestates.size() > (cell_y | 0x3) + 1 && !greenzone.savestates[(cell_y | 0x3) + 1].empty())
						|| (!greenzone.savestates[cell_y & EVERY2ND].empty() && !greenzone.savestates[(cell_y | 0x1) + 1].empty()))
					{
						if (TASEdit_show_lag_frames && greenzone.lag_history[cell_y])
							msg->clrTextBk = PALE_LAG_INPUT_COLOR1;
						else
							msg->clrTextBk = PALE_GREENZONE_INPUT_COLOR1;
					} else msg->clrTextBk = NORMAL_INPUT_COLOR1;
				} else msg->clrTextBk = NORMAL_INPUT_COLOR1;
			} else if((cell_x - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS == 1 || (cell_x - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS == 3)
			{
				// pad 2 or 4
				SelectObject(msg->nmcd.hdc, hMainListFont);
				if (cell_y == history.GetUndoHint())
				{
					// undo hint here
					msg->clrTextBk = UNDOHINT_INPUT_COLOR2;
				} else if (cell_y == currFrameCounter || cell_y == (playback.GetPauseFrame() - 1))
				{
					// current frame
					msg->clrTextBk = CUR_INPUT_COLOR2;
				} else if(cell_y < greenzone.greenZoneCount)
				{
					if (!greenzone.savestates[cell_y].empty())
					{
						if (TASEdit_show_lag_frames && greenzone.lag_history[cell_y])
							msg->clrTextBk = LAG_INPUT_COLOR2;
						else
							msg->clrTextBk = GREENZONE_INPUT_COLOR2;
					} else if ((!greenzone.savestates[cell_y & EVERY16TH].empty() && (int)greenzone.savestates.size() > (cell_y | 0xF) + 1 && !greenzone.savestates[(cell_y | 0xF) + 1].empty())
						|| (!greenzone.savestates[cell_y & EVERY8TH].empty() && (int)greenzone.savestates.size() > (cell_y | 0x7) + 1 && !greenzone.savestates[(cell_y | 0x7) + 1].empty())
						|| (!greenzone.savestates[cell_y & EVERY4TH].empty() && (int)greenzone.savestates.size() > (cell_y | 0x3) + 1 && !greenzone.savestates[(cell_y | 0x3) + 1].empty())
						|| (!greenzone.savestates[cell_y & EVERY2ND].empty() && !greenzone.savestates[(cell_y | 0x1) + 1].empty()))
					{
						if (TASEdit_show_lag_frames && greenzone.lag_history[cell_y])
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
// -------------------------------------------------------------------------
LRESULT APIENTRY HeaderWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_SETCURSOR:
		return true;	// no column resizing
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
		{
			if (selection.GetCurrentSelectionSize())
			{
				//perform hit test
				HD_HITTESTINFO info;
				info.pt.x = GET_X_LPARAM(lParam);
				info.pt.y = GET_Y_LPARAM(lParam);
				SendMessage(hWnd,HDM_HITTEST,0,(LPARAM)&info);
				if(info.iItem >= COLUMN_FRAMENUM && info.iItem <= COLUMN_FRAMENUM2)
					ColumnSet(info.iItem);
			}
		}
		return true;
	}
	return CallWindowProc(hwndHeader_oldWndproc, hWnd, msg, wParam, lParam);
}

//The subclass wndproc for the listview
LRESULT APIENTRY ListWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
		case WM_CHAR:
		case WM_KILLFOCUS:
			return 0;
		case WM_NOTIFY:
		{
			switch (((LPNMHDR)lParam)->code)
			{
			case HDN_BEGINTRACKW:
			case HDN_BEGINTRACKA:
			case HDN_TRACK:
				return true;	// no column resizing
			}
			break;
		}
		case WM_SYSKEYDOWN:
		{
			if (wParam == VK_F10)
				return 0;
			break;
		}
	}
	return CallWindowProc(hwndList_oldWndProc, hWnd, msg, wParam, lParam);
}


