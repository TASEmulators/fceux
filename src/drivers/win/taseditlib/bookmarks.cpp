//Implementation file of Bookmarks class

#include "movie.h"
#include "../common.h"
#include "taseditproj.h"
#include "../tasedit.h"
#include "zlib.h"
#include "utils/xstring.h"

char bookmarks_save_id[BOOKMARKS_ID_LEN] = "BOOKMARKS";
char bookmarksCaption[2][23] = { " Bookmarks ", " Bookmarks / Branches " };

// color tables for flashing when saving/loading bookmarks
COLORREF bookmark_flash_colors[3][FLASH_PHASE_MAX+1] = {
	// set
	0x122330, 0x1b3541, 0x254753, 0x2e5964, 0x376b75, 0x417e87, 0x4a8f97, 0x53a1a8, 0x5db3b9, 0x66c5cb, 0x70d7dc, 0x79e9ed, 
	// jump
	0x382309, 0x3c350e, 0x404814, 0x455a19, 0x486c1e, 0x4d7f23, 0x519128, 0x55a32d, 0x5ab532, 0x5ec837, 0x62da3c, 0x66ec41, 
	// unleash
	0x320d23, 0x341435, 0x361b48, 0x38215a, 0x39286c, 0x3b2f7f, 0x3c3691, 0x3e3ca3, 0x4043b5, 0x414ac8, 0x4351da, 0x4457ec };

extern PLAYBACK playback;
extern GREENZONE greenzone;
extern TASEDIT_PROJECT project;
extern INPUT_HISTORY history;
extern MARKERS markers;

extern HWND hwndBookmarks;
extern HWND hwndBookmarksList;
extern bool TASEdit_show_lag_frames;
extern bool TASEdit_bind_markers;
extern bool TASEdit_branch_full_movie;

BOOKMARKS::BOOKMARKS()
{

}

void BOOKMARKS::init()
{
	free();
	bookmarks_array.resize(TOTAL_BOOKMARKS);
	for (int i = 0; i < TOTAL_BOOKMARKS; ++i)
	{
		bookmarks_array[i].init();
	}
	ListView_SetItemCountEx(hwndBookmarksList, TOTAL_BOOKMARKS, LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);

	// create font
	hBookmarksFont = CreateFont(13, 8,				/*Height,Width*/
		0, 0,										/*escapement,orientation*/
		FW_REGULAR, FALSE, FALSE, FALSE,			/*weight, italic, underline, strikeout*/
		ANSI_CHARSET, OUT_DEVICE_PRECIS, CLIP_MASK,	/*charset, precision, clipping*/
		DEFAULT_QUALITY, DEFAULT_PITCH,				/*quality, and pitch*/
		"Courier");									/*font name*/
	
	check_flash_shedule = clock() + BOOKMARKS_FLASH_TICK;
	RedrawBookmarksList();
}
void BOOKMARKS::free()
{
	bookmarks_array.resize(0);

}

void BOOKMARKS::update()
{
	// once per 50 milliseconds fade bookmark flashes
	if (clock() > check_flash_shedule)
	{
		check_flash_shedule = clock() + BOOKMARKS_FLASH_TICK;
		for (int i = 0; i < TOTAL_BOOKMARKS; ++i)
		{
			if (bookmarks_array[i].flash_phase)
			{
				bookmarks_array[i].flash_phase--;
				RedrawBookmarksRow((i + TOTAL_BOOKMARKS - 1) % TOTAL_BOOKMARKS);
			}
		}
	}
}

void BOOKMARKS::set(int slot)
{
	if (slot < 0 || slot >= TOTAL_BOOKMARKS) return;
	int previous_frame = bookmarks_array[slot].snapshot.jump_frame;
	bookmarks_array[slot].set();
	if (previous_frame >= 0 && previous_frame != currFrameCounter)
		RedrawRow(previous_frame);
	RedrawRow(currFrameCounter);

	RedrawBookmarksRow((slot + TOTAL_BOOKMARKS - 1) % TOTAL_BOOKMARKS);

}

void BOOKMARKS::jump(int slot)
{
	if (slot < 0 || slot >= TOTAL_BOOKMARKS) return;
	if (bookmarks_array[slot].not_empty)
	{
		int frame = bookmarks_array[slot].snapshot.jump_frame;
		playback.jump(frame);
		if (playback.pauseframe)
			FollowPauseframe();
		else
			FollowPlayback();
		bookmarks_array[slot].jump();
	}
}

void BOOKMARKS::unleash(int slot)
{
	if (movie_readonly)
	{
		jump(slot);
		return;
	}
	if (slot < 0 || slot >= TOTAL_BOOKMARKS) return;
	if (!bookmarks_array[slot].not_empty) return;
	int jump_frame = bookmarks_array[slot].snapshot.jump_frame;

	// revert movie to the input_snapshot state
	if (TASEdit_branch_full_movie)
	{
		// update Markers
		bool markers_changed = false;
		if (TASEdit_bind_markers)
		{
			if (bookmarks_array[slot].snapshot.checkMarkersDiff())
			{
				bookmarks_array[slot].snapshot.toMarkers();
				project.changed = true;
				markers_changed = true;
			}
		}
		// update current movie
		int first_change = bookmarks_array[slot].snapshot.findFirstChange(currMovieData);
		if (first_change >= 0)
		{
			// restore entire movie
			currMovieData.records.resize(bookmarks_array[slot].snapshot.size);
			bookmarks_array[slot].snapshot.toMovie(currMovieData, first_change);
			UpdateList();
			history.RegisterBranch(MODTYPE_BRANCH_0 + slot, first_change, bookmarks_array[slot].snapshot.description);
			greenzone.Invalidate(first_change);
			bookmarks_array[slot].unleash();
		} else if (markers_changed)
		{
			history.RegisterBranch(MODTYPE_BRANCH_MARKERS_0 + slot, first_change, bookmarks_array[slot].snapshot.description);
			RedrawList();
			bookmarks_array[slot].unleash();
		} else
		{
			// didn't restore anything
			bookmarks_array[slot].jump();
		}
	} else
	{
		// update Markers
		bool markers_changed = false;
		if (TASEdit_bind_markers)
		{
			if (bookmarks_array[slot].snapshot.checkMarkersDiff(jump_frame))
			{
				bookmarks_array[slot].snapshot.copyToMarkers(jump_frame-1);
				project.changed = true;
				markers_changed = true;
			}
		}
		// update current movie
		int first_change = bookmarks_array[slot].snapshot.findFirstChange(currMovieData, 0, jump_frame);
		if (first_change >= 0 && first_change < jump_frame)
		{
			// restore movie up to and not including bookmarked frame (imitating old TASing method)
			if (currMovieData.getNumRecords() <= jump_frame) currMovieData.records.resize(jump_frame+1);	// but if old movie is shorter, include last frame as blank frame
			bookmarks_array[slot].snapshot.toMovie(currMovieData, first_change, jump_frame-1);
			UpdateList();
			history.RegisterBranch(MODTYPE_BRANCH_0 + slot, first_change, bookmarks_array[slot].snapshot.description);
			greenzone.Invalidate(first_change);
			bookmarks_array[slot].unleash();
		} else if (markers_changed)
		{
			history.RegisterBranch(MODTYPE_BRANCH_MARKERS_0 + slot, first_change, bookmarks_array[slot].snapshot.description);
			RedrawList();
			bookmarks_array[slot].unleash();
		} else
		{
			// didn't restore anything
			bookmarks_array[slot].jump();
		}
	}

	// if greenzone reduced so much that we can't jump immediately - substitute target frame greenzone with our savestate
	if (greenzone.greenZoneCount <= jump_frame || greenzone.savestates[jump_frame].empty())
	{
		// clear old savestates: from current end of greenzone to new end of greenzone
		for (int i = greenzone.greenZoneCount; i < jump_frame; ++i)
			greenzone.ClearSavestate(i);
		// restore savestate for immediate jump
		greenzone.savestates[jump_frame] = bookmarks_array[slot].savestate;
		// and move greenzone end to this new position
		if (greenzone.greenZoneCount <= jump_frame) greenzone.greenZoneCount = jump_frame+1;
	}

	// jump to the target (bookmarked frame)
	playback.jump(jump_frame);
}

void BOOKMARKS::save(EMUFILE *os)
{
	// write "BOOKMARKS" string
	os->fwrite(bookmarks_save_id, BOOKMARKS_ID_LEN);
	// write all 10 bookmarks
	for (int i = 0; i < TOTAL_BOOKMARKS; ++i)
	{
		bookmarks_array[i].save(os);
	}
}
// returns true if couldn't load
bool BOOKMARKS::load(EMUFILE *is)
{
	// read "BOOKMARKS" string
	char save_id[BOOKMARKS_ID_LEN];
	if ((int)is->fread(save_id, BOOKMARKS_ID_LEN) < BOOKMARKS_ID_LEN) return true;
	if (strcmp(bookmarks_save_id, save_id)) return true;		// string is not valid
	// read all 10 bookmarks
	for (int i = 0; i < TOTAL_BOOKMARKS; ++i)
	{
		if (bookmarks_array[i].load(is)) return true;
	}
	// all ok
	check_flash_shedule = clock() + BOOKMARKS_FLASH_TICK;
	return false;
}
// ----------------------------------------------------------
void BOOKMARKS::RedrawBookmarksCaption()
{
	SetWindowText(hwndBookmarks, bookmarksCaption[(movie_readonly)?0:1]);
	RedrawBookmarksList();
}
void BOOKMARKS::RedrawBookmarksList()
{
	InvalidateRect(hwndBookmarksList, 0, FALSE);
}
void BOOKMARKS::RedrawChangedBookmarks(int frame)
{
	for (int i = 0; i < TOTAL_BOOKMARKS; ++i)
	{
		if (bookmarks_array[i].snapshot.jump_frame == frame)
			RedrawBookmarksRow((i + TOTAL_BOOKMARKS - 1) % TOTAL_BOOKMARKS);
	}
}
void BOOKMARKS::RedrawBookmarksRow(int index)
{
	ListView_RedrawItems(hwndBookmarksList, index, index);
}

void BOOKMARKS::GetDispInfo(NMLVDISPINFO* nmlvDispInfo)
{
	LVITEM& item = nmlvDispInfo->item;
	if(item.mask & LVIF_TEXT)
	{
		switch(item.iSubItem)
		{
			case BOOKMARKS_COLUMN_ICON:
			{
				item.iImage = (item.iItem + 1) % TOTAL_BOOKMARKS;
				break;
			}
			case BOOKMARKS_COLUMN_FRAME:
			{
				if (bookmarks_array[(item.iItem + 1) % TOTAL_BOOKMARKS].not_empty)
					U32ToDecStr(item.pszText, bookmarks_array[(item.iItem + 1) % TOTAL_BOOKMARKS].snapshot.jump_frame, DIGITS_IN_FRAMENUM);
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

		if (cell_x == BOOKMARKS_COLUMN_FRAME || (movie_readonly && cell_x == BOOKMARKS_COLUMN_TIME))
		{
			if (bookmarks_array[cell_y].not_empty)
			{
				// frame number
				SelectObject(msg->nmcd.hdc, hBookmarksFont);
				int frame = bookmarks_array[cell_y].snapshot.jump_frame;
				if (frame == currFrameCounter || frame == playback.GetPauseFrame())
				{
					// current frame
					msg->clrTextBk = CUR_FRAMENUM_COLOR;
				} else if (frame < greenzone.greenZoneCount)
				{
					if (!greenzone.savestates[frame].empty())
					{
						if (TASEdit_show_lag_frames && greenzone.lag_history[frame])
							msg->clrTextBk = LAG_FRAMENUM_COLOR;
						else
							msg->clrTextBk = GREENZONE_FRAMENUM_COLOR;
					} else if ((!greenzone.savestates[frame & EVERY16TH].empty() && (int)greenzone.savestates.size() > (frame | 0xF) + 1 && !greenzone.savestates[(frame | 0xF) + 1].empty())
						|| (!greenzone.savestates[frame & EVERY8TH].empty() && (int)greenzone.savestates.size() > (frame | 0x7) + 1 && !greenzone.savestates[(frame | 0x7) + 1].empty())
						|| (!greenzone.savestates[frame & EVERY4TH].empty() && (int)greenzone.savestates.size() > (frame | 0x3) + 1 && !greenzone.savestates[(frame | 0x3) + 1].empty())
						|| (!greenzone.savestates[frame & EVERY2ND].empty() && !greenzone.savestates[(frame | 0x1) + 1].empty()))
					{
						if (TASEdit_show_lag_frames && greenzone.lag_history[frame])
							msg->clrTextBk = PALE_LAG_FRAMENUM_COLOR;
						else
							msg->clrTextBk = PALE_GREENZONE_FRAMENUM_COLOR;
					} else msg->clrTextBk = NORMAL_FRAMENUM_COLOR;
				} else msg->clrTextBk = NORMAL_FRAMENUM_COLOR;
			} else msg->clrTextBk = 0xFFFFFF;		// empty bookmark
		} else if (cell_x == BOOKMARKS_COLUMN_TIME)
		{
			if (bookmarks_array[cell_y].not_empty)
			{
				// frame number
				SelectObject(msg->nmcd.hdc, hBookmarksFont);
				int frame = bookmarks_array[cell_y].snapshot.jump_frame;
				if (frame == currFrameCounter || frame == playback.GetPauseFrame())
				{
					// current frame
					msg->clrTextBk = CUR_INPUT_COLOR1;
				} else if (frame < greenzone.greenZoneCount)
				{
					if (!greenzone.savestates[frame].empty())
					{
						if (TASEdit_show_lag_frames && greenzone.lag_history[frame])
							msg->clrTextBk = LAG_INPUT_COLOR1;
						else
							msg->clrTextBk = GREENZONE_INPUT_COLOR1;
					} else if ((!greenzone.savestates[frame & EVERY16TH].empty() && (int)greenzone.savestates.size() > (frame | 0xF) + 1 && !greenzone.savestates[(frame | 0xF) + 1].empty())
						|| (!greenzone.savestates[frame & EVERY8TH].empty() && (int)greenzone.savestates.size() > (frame | 0x7) + 1 && !greenzone.savestates[(frame | 0x7) + 1].empty())
						|| (!greenzone.savestates[frame & EVERY4TH].empty() && (int)greenzone.savestates.size() > (frame | 0x3) + 1 && !greenzone.savestates[(frame | 0x3) + 1].empty())
						|| (!greenzone.savestates[frame & EVERY2ND].empty() && !greenzone.savestates[(frame | 0x1) + 1].empty()))
					{
						if (TASEdit_show_lag_frames && greenzone.lag_history[frame])
							msg->clrTextBk = PALE_LAG_INPUT_COLOR1;
						else
							msg->clrTextBk = PALE_GREENZONE_INPUT_COLOR1;
					} else msg->clrTextBk = NORMAL_INPUT_COLOR1;
				} else msg->clrTextBk = NORMAL_INPUT_COLOR1;
			} else msg->clrTextBk = 0xFFFFFF;		// empty bookmark
		}
	default:
		return CDRF_DODEFAULT;
	}
}

void BOOKMARKS::LeftClick(LPNMITEMACTIVATE info)
{
	int cell_x = info->iSubItem;
	int cell_y = info->iItem;
	if (cell_y >= 0 && cell_x >= 0)
	{
		if (cell_x <= BOOKMARKS_COLUMN_FRAME || movie_readonly)
			jump((cell_y + 1) % TOTAL_BOOKMARKS);
		else if (cell_x == BOOKMARKS_COLUMN_TIME && !movie_readonly)
			unleash((cell_y + 1) % TOTAL_BOOKMARKS);
		//RedrawBookmarksList();
	}
	// remove selection
	ListView_SetItemState(hwndBookmarksList, -1, 0, LVIS_FOCUSED|LVIS_SELECTED);
}
void BOOKMARKS::RightClick(LPNMITEMACTIVATE info)
{
	int cell_y = info->iItem;
	if (cell_y >= 0)
		set((cell_y + 1) % TOTAL_BOOKMARKS);
	// remove selection
	ListView_SetItemState(hwndBookmarksList, -1, 0, LVIS_FOCUSED|LVIS_SELECTED);
}

int BOOKMARKS::FindBookmarkAtFrame(int frame)
{
	for (int i = 0; i < TOTAL_BOOKMARKS; ++i)
	{
		if (bookmarks_array[i].snapshot.jump_frame == frame) return i;
	}
	return -1;
}


