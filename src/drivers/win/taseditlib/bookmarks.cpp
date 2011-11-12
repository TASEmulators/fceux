//Implementation file of Bookmarks class

#include "movie.h"
#include "../common.h"
#include "taseditproj.h"
#include "../tasedit.h"
#include "zlib.h"
#include "utils/xstring.h"

#pragma comment(lib, "msimg32.lib")

LRESULT CALLBACK ScrBmpWndProc(HWND, UINT, WPARAM, LPARAM);
char szClassName[] = "BmpTestApp";
HWND hwndScrBmp, scr_bmp_pic;
WNDCLASSEX wincl;
BLENDFUNCTION blend;
extern HWND hwndTasEdit;
extern int TasEdit_wndx, TasEdit_wndy;

char bookmarks_save_id[BOOKMARKS_ID_LEN] = "BOOKMARKS";
char bookmarksCaption[3][23] = { " Bookmarks ", " Bookmarks / Branches ", " Branches " };

// color tables for flashing when saving/loading bookmarks
COLORREF bookmark_flash_colors[3][FLASH_PHASE_MAX+1] = {
	// set
	0x122330, 0x1b3541, 0x254753, 0x2e5964, 0x376b75, 0x417e87, 0x4a8f97, 0x53a1a8, 0x5db3b9, 0x66c5cb, 0x70d7dc, 0x79e9ed, 
	// jump
	0x382309, 0x3c350e, 0x404814, 0x455a19, 0x486c1e, 0x4d7f23, 0x519128, 0x55a32d, 0x5ab532, 0x5ec837, 0x62da3c, 0x66ec41, 
	// unleash
	0x320d23, 0x341435, 0x361b48, 0x38215a, 0x39286c, 0x3b2f7f, 0x3c3691, 0x3e3ca3, 0x4043b5, 0x414ac8, 0x4351da, 0x4457ec };
// corners cursor animation
int corners_cursor_shift[BRANCHES_ANIMATION_FRAMES] = {0, 1, 2, 3, 4, 5, 5, 4, 3, 2, 1, 0 };

extern PLAYBACK playback;
extern GREENZONE greenzone;
extern TASEDIT_PROJECT project;
extern INPUT_HISTORY history;
extern MARKERS markers;

extern HWND hwndBookmarks;
extern HWND hwndBookmarksList;
extern HWND hwndBranchesBitmap;

extern bool TASEdit_show_lag_frames;
extern bool TASEdit_bind_markers;
extern bool TASEdit_branch_full_movie;
extern bool TASEdit_branch_only_when_rec;
extern bool TASEdit_view_branches_tree;

BOOKMARKS::BOOKMARKS()
{
	// create font
	hBookmarksFont = CreateFont(13, 8,				/*Height,Width*/
		0, 0,										/*escapement,orientation*/
		FW_REGULAR, FALSE, FALSE, FALSE,			/*weight, italic, underline, strikeout*/
		ANSI_CHARSET, OUT_DEVICE_PRECIS, CLIP_MASK,	/*charset, precision, clipping*/
		DEFAULT_QUALITY, DEFAULT_PITCH,				/*quality, and pitch*/
		"Courier");									/*font name*/
		
	// fill TrackMouseEvent struct
	tme.cbSize = sizeof(tme);
	tme.dwFlags = TME_LEAVE;
	tme.hwndTrack = hwndBranchesBitmap;
	list_tme.cbSize = sizeof(tme);
	list_tme.dwFlags = TME_LEAVE;
	list_tme.hwndTrack = hwndBookmarksList;
	
	// create BITMAPINFO for scr_bmp
	scr_bmi = (LPBITMAPINFO)malloc(sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
	scr_bmi->bmiHeader.biSize = sizeof(scr_bmi->bmiHeader);
	scr_bmi->bmiHeader.biWidth = SCREENSHOT_WIDTH;
	scr_bmi->bmiHeader.biHeight = -SCREENSHOT_HEIGHT;		// negative value = top-down bmp
	scr_bmi->bmiHeader.biPlanes = 1;
	scr_bmi->bmiHeader.biBitCount = 8;
	scr_bmi->bmiHeader.biCompression = BI_RGB;
	scr_bmi->bmiHeader.biSizeImage = 0;

	// register ScrBmp window class
    wincl.hInstance = fceu_hInstance;
    wincl.lpszClassName = szClassName;
    wincl.lpfnWndProc = ScrBmpWndProc;
    wincl.style = CS_DBLCLKS;
    wincl.cbSize = sizeof(WNDCLASSEX);
    wincl.hIcon = 0;
    wincl.hIconSm = 0;
    wincl.hCursor = 0;
    wincl.lpszMenuName = 0;
    wincl.cbClsExtra = 0;
    wincl.cbWndExtra = 0;
    wincl.hbrBackground = 0;
	if(!RegisterClassEx(&wincl))
		FCEU_printf("Error registering ScrBmp window class\n");
	// create blendfunction
	blend.BlendOp = AC_SRC_OVER;
	blend.BlendFlags = 0;
	blend.AlphaFormat = 0;
	blend.SourceConstantAlpha = 255;


}

void BOOKMARKS::init()
{
	free();
	// init arrays
	BranchX.resize(TOTAL_BOOKMARKS+1);
	BranchY.resize(TOTAL_BOOKMARKS+1);
	BranchPrevX.resize(TOTAL_BOOKMARKS+1);
	BranchPrevY.resize(TOTAL_BOOKMARKS+1);
	BranchCurrX.resize(TOTAL_BOOKMARKS+1);
	BranchCurrY.resize(TOTAL_BOOKMARKS+1);
	for (int i = TOTAL_BOOKMARKS; i >= 0; i--)
	{
		BranchX[i] = BranchPrevX[i] = BranchCurrX[i] = EMPTY_BRANCHES_X;
		BranchY[i] = BranchPrevY[i] = BranchCurrY[i] = EMPTY_BRANCHES_Y_BASE + EMPTY_BRANCHES_Y_FACTOR * ((i + TOTAL_BOOKMARKS - 1) % TOTAL_BOOKMARKS);
	}
	CursorX = CursorPrevX = CloudX = CloudPrevX = BRANCHES_CLOUD_X;
	CursorY = CursorPrevY = BRANCHES_CLOUD_Y;
	reset();
	current_branch = -1;	// -1 = root
	changes_since_current_branch = false;
	fireball_size = 0;

	// set cloud_time and current_pos_time
	SetCurrentPosTime();
	strcpy(cloud_time, current_pos_time);

	// init bookmarks
	bookmarks_array.resize(TOTAL_BOOKMARKS);
	for (int i = 0; i < TOTAL_BOOKMARKS; ++i)
		bookmarks_array[i].init();
	ListView_SetItemCountEx(hwndBookmarksList, TOTAL_BOOKMARKS, LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);

	// find rows top/height (for mouseover hittest calculations)
	RECT temp_rect;
	if (ListView_GetSubItemRect(hwndBookmarksList, 0, 2, LVIR_BOUNDS, &temp_rect))
	{
		branch_row_top = temp_rect.top;
		branch_row_left = temp_rect.left;
		branch_row_height = temp_rect.bottom - temp_rect.top;
	} else
	{
		// couldn't get rect, set default values
		branch_row_top = 0;
		branch_row_height = 14;
	}
	// init GDI stuff
	HDC win_hdc = GetWindowDC(hwndBookmarksList);
	hBitmapDC = CreateCompatibleDC(win_hdc);
	branches_hbitmap = CreateCompatibleBitmap(win_hdc, BRANCHES_BITMAP_WIDTH, BRANCHES_BITMAP_WIDTH);
	hOldBitmap = (HBITMAP)SelectObject(hBitmapDC, branches_hbitmap);
	hBufferDC = CreateCompatibleDC(win_hdc);
	buffer_hbitmap = CreateCompatibleBitmap(win_hdc, BRANCHES_BITMAP_WIDTH, BRANCHES_BITMAP_WIDTH);
	hOldBitmap1 = (HBITMAP)SelectObject(hBufferDC, buffer_hbitmap);
	normal_brush = CreateSolidBrush(0x000000);
	// prepare branches spritesheet
	branchesSpritesheet = LoadBitmap(fceu_hInstance, MAKEINTRESOURCE(IDB_BRANCH_SPRITESHEET));
	hSpritesheetDC = CreateCompatibleDC(win_hdc);
	hOldBitmap2 = (HBITMAP)SelectObject(hSpritesheetDC, branchesSpritesheet);
	// create pens
	normal_pen = CreatePen(PS_SOLID, 1, 0x0);
	select_pen = CreatePen(PS_SOLID, 2, 0xFF9080);
	// prepare screenshot bitmap
	// fill scr_bmp palette with current palette colors
	extern PALETTEENTRY *color_palette;
	for (int i = 0; i < 256; ++i)
	{
		scr_bmi->bmiColors[i].rgbRed = color_palette[i].peRed;
		scr_bmi->bmiColors[i].rgbGreen = color_palette[i].peGreen;
		scr_bmi->bmiColors[i].rgbBlue = color_palette[i].peBlue;
	}
	scr_bmp = CreateDIBSection(win_hdc, scr_bmi, DIB_RGB_COLORS, (void**)&scr_ptr, 0, 0);

	RedrawBookmarksCaption();
	update();
}
void BOOKMARKS::reset()
{
	transition_phase = animation_frame = 0;
	mouse_x = mouse_y = -1;
	screenshot_currently_shown = item_under_mouse = ITEM_UNDER_MOUSE_NONE;
	scr_bmp_phase = 0;
	mouse_over_bitmap = false;
	must_recalculate_branches_tree = must_redraw_branches_tree = must_check_item_under_mouse = true;
	check_flash_shedule = clock() + BOOKMARKS_FLASH_TICK;
	next_animation_time = clock() + BRANCHES_ANIMATION_TICK;
}
void BOOKMARKS::free()
{
	bookmarks_array.resize(0);

	BranchX.resize(0);
	BranchY.resize(0);
	BranchPrevX.resize(0);
	BranchPrevY.resize(0);
	BranchCurrX.resize(0);
	BranchCurrY.resize(0);

	// free GDI stuff
	if (hOldBitmap && hBitmapDC)
	{
		SelectObject(hBitmapDC, hOldBitmap);
		DeleteDC(hBitmapDC);
		hBitmapDC = NULL;
	}
	if (branches_hbitmap)
	{
		DeleteObject(branches_hbitmap);
		branches_hbitmap = NULL;
	}
	if (hOldBitmap1 && hBufferDC)
	{
		SelectObject(hBufferDC, hOldBitmap1);
		DeleteDC(hBufferDC);
		hBufferDC = NULL;
	}
	if (buffer_hbitmap)
	{
		DeleteObject(buffer_hbitmap);
		buffer_hbitmap = NULL;
	}
	if (hOldBitmap2 && hSpritesheetDC)
	{
		SelectObject(hSpritesheetDC, hOldBitmap2);
		DeleteDC(hSpritesheetDC);
		hSpritesheetDC = NULL;
	}
	if (branchesSpritesheet)
	{
		DeleteObject(branchesSpritesheet);
		branchesSpritesheet = NULL;
	}
	if (scr_bmp)
	{
		DeleteObject(scr_bmp);
		scr_bmp = NULL;
	}


}

void BOOKMARKS::update()
{
	// once per 100 milliseconds fade bookmark flashes
	if (clock() > check_flash_shedule)
	{
		check_flash_shedule = clock() + BOOKMARKS_FLASH_TICK;
		for (int i = 0; i < TOTAL_BOOKMARKS; ++i)
		{
			if (bookmarks_array[i].flash_phase)
			{
				bookmarks_array[i].flash_phase--;
				RedrawBookmarksRow((i + TOTAL_BOOKMARKS - 1) % TOTAL_BOOKMARKS);
				must_redraw_branches_tree = true;		// because borders of some branch digit has changed
			}
		}
	}

	if (must_recalculate_branches_tree)
		RecalculateBranchesTree();

	// once per 50 milliseconds update branches_bitmap and scr_bmp 
	if (clock() > next_animation_time)
	{
		// animate branches_bitmap
		next_animation_time = clock() + BRANCHES_ANIMATION_TICK;
		animation_frame = (animation_frame + 1) % BRANCHES_ANIMATION_FRAMES;
		if (edit_mode == EDIT_MODE_BRANCHES)
		{
			// grow or shrink fireball size
			if (changes_since_current_branch)
			{
				fireball_size++;
				if (fireball_size > BRANCHES_FIREBALL_MAX_SIZE) fireball_size = BRANCHES_FIREBALL_MAX_SIZE;
			} else
			{
				fireball_size--;
				if (fireball_size < 0) fireball_size = 0;
			}
			// also update transition from old to new tree
			if (transition_phase)
			{
				transition_phase--;
				must_check_item_under_mouse = must_redraw_branches_tree = true;
			} else if (!must_redraw_branches_tree)
			{
				// just update sprites
				InvalidateRect(hwndBranchesBitmap, 0, FALSE);
			}
		}
		// controls
		if (must_check_item_under_mouse)
			CheckMousePos();
		// render branches_bitmap
		if (must_redraw_branches_tree)
			RedrawBranchesTree();

		// change screenshot_bitmap alpha if needed
		if (item_under_mouse >= 0 && item_under_mouse < TOTAL_BOOKMARKS)
		{
			if (!hwndScrBmp)
			{
				// create window
				hwndScrBmp = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT, szClassName, szClassName, WS_POPUP, TasEdit_wndx + scr_bmp_x, TasEdit_wndy + scr_bmp_y, SCREENSHOT_WIDTH, SCREENSHOT_HEIGHT, hwndTasEdit, NULL, fceu_hInstance, NULL);
				RedrawScreenshotBitmap();
				ShowWindow(hwndScrBmp, SW_SHOWNA);
			}
			// change screenshot_bitmap pic if needed
			if (item_under_mouse != screenshot_currently_shown)
			{
				if (bookmarks_array[item_under_mouse].not_empty)
					ChangeScreenshotBitmap();
			}
			if (scr_bmp_phase < SCR_BMP_PHASE_MAX)
			{
				scr_bmp_phase++;
				// update alpha
				SetLayeredWindowAttributes(hwndScrBmp, 0, (255 * scr_bmp_phase) / SCR_BMP_PHASE_MAX, LWA_ALPHA);
				UpdateLayeredWindow(hwndScrBmp, 0, 0, 0, 0, 0, 0, &blend, ULW_ALPHA);
			}
		} else
		{
			if (scr_bmp_phase > 0)
			{
				scr_bmp_phase--;
				if (hwndScrBmp)
				{
					// update alpha
					SetLayeredWindowAttributes(hwndScrBmp, 0, (255 * scr_bmp_phase) / SCR_BMP_PHASE_MAX, LWA_ALPHA);
					UpdateLayeredWindow(hwndScrBmp, 0, 0, 0, 0, 0, 0, &blend, ULW_ALPHA);
				}
			} else
			{
				// destroy screenshot bitmap window
				scr_bmp_phase = 0;
				if (hwndScrBmp)
				{
					DestroyWindow(hwndScrBmp);
					hwndScrBmp = 0;
				}
			}
		}
	}
}

void BOOKMARKS::set(int slot)
{
	if (slot < 0 || slot >= TOTAL_BOOKMARKS) return;
	int previous_frame = bookmarks_array[slot].snapshot.jump_frame;
	// save time of this slot before rewriting it
	char saved_time[TIME_DESC_LENGTH];
	if (bookmarks_array[slot].not_empty)
		strncpy(saved_time, bookmarks_array[slot].snapshot.description, TIME_DESC_LENGTH-1);
	else
		saved_time[0] = 0;
	
	bookmarks_array[slot].set();

	// if this screenshot is shown on screen - reinit and redraw it
	if (screenshot_currently_shown == slot)
		screenshot_currently_shown = ITEM_UNDER_MOUSE_NONE;

	// inherit current branch
	if (slot != current_branch)
	{
		int parent = bookmarks_array[slot].parent_branch;
		if (parent == -1 && saved_time[0])
		{
			// check if this is the only child of cloud parent, if so then set cloud time to the saved_time
			int i = 0;
			for (; i < TOTAL_BOOKMARKS; ++i)
			{
				if (bookmarks_array[i].not_empty && bookmarks_array[i].parent_branch == -1 && i != slot)
					break;
			}
			if (i >= TOTAL_BOOKMARKS)
				// didn't find another child of cloud
				strcpy(cloud_time, saved_time);
		}
		// before disconnecting from old parent, connect all childs to the parent
		for (int i = 0; i < TOTAL_BOOKMARKS; ++i)
		{
			if (bookmarks_array[i].not_empty && bookmarks_array[i].parent_branch == slot)
				bookmarks_array[i].parent_branch = parent;
		}
		bookmarks_array[slot].parent_branch = current_branch;
	}

	// switch current branch to this branch
	if (slot != current_branch && current_branch >= 0)
		RedrawRowAndBookmark(bookmarks_array[current_branch].snapshot.jump_frame);
	if (slot != current_branch || changes_since_current_branch)
		must_recalculate_branches_tree = true;
	current_branch = slot;
	changes_since_current_branch = false;
	project.SetProjectChanged();

	if (previous_frame >= 0 && previous_frame != currFrameCounter)
		RedrawRowAndBookmark(previous_frame);
	RedrawRowAndBookmark(currFrameCounter);
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
	if (TASEdit_branch_only_when_rec && movie_readonly)
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
				project.SetProjectChanged();
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
			bookmarks_array[slot].unleashed();
		} else if (markers_changed)
		{
			history.RegisterBranch(MODTYPE_BRANCH_MARKERS_0 + slot, first_change, bookmarks_array[slot].snapshot.description);
			RedrawList();
			bookmarks_array[slot].unleashed();
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
				project.SetProjectChanged();
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
			bookmarks_array[slot].unleashed();
		} else if (markers_changed)
		{
			history.RegisterBranch(MODTYPE_BRANCH_MARKERS_0 + slot, first_change, bookmarks_array[slot].snapshot.description);
			RedrawList();
			bookmarks_array[slot].unleashed();
		} else
		{
			// didn't restore anything
			bookmarks_array[slot].jump();
		}
	}

	// if greenzone reduced so much that we can't jump immediately - substitute target frame greenzone with our savestate
	if (greenzone.greenZoneCount <= jump_frame || greenzone.savestates[jump_frame].empty())
	{
		if ((int)greenzone.savestates.size() <= jump_frame)
			greenzone.savestates.resize(jump_frame+1);
		// clear old savestates: from current end of greenzone to new end of greenzone
		if (greenzone.greenZoneCount <= jump_frame)
		{
			for (int i = greenzone.greenZoneCount; i < jump_frame; ++i)
				greenzone.ClearSavestate(i);
			greenzone.greenZoneCount = jump_frame+1;
		}
		// restore savestate for immediate jump
		greenzone.savestates[jump_frame] = bookmarks_array[slot].savestate;
	}
	greenzone.update();

	// jump to the target (bookmarked frame)
	playback.jump(jump_frame);

	// switch current branch to this branch
	if (slot != current_branch && current_branch >= 0)
	{
		RedrawRowAndBookmark(bookmarks_array[current_branch].snapshot.jump_frame);
		RedrawRowAndBookmark(bookmarks_array[slot].snapshot.jump_frame);
	}
	current_branch = slot;
	changes_since_current_branch = false;
	must_recalculate_branches_tree = true;
}

void BOOKMARKS::save(EMUFILE *os)
{
	// write "BOOKMARKS" string
	os->fwrite(bookmarks_save_id, BOOKMARKS_ID_LEN);
	// write cloud time
	os->fwrite(cloud_time, TIME_DESC_LENGTH);
	// write current branch and flag of changes since it
	write8le((uint8)current_branch, os);
	if (changes_since_current_branch)
		write8le((uint8)1, os);
	else
		write8le((uint8)0, os);
	// write current_position time
	os->fwrite(current_pos_time, TIME_DESC_LENGTH);
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
	// read cloud time
	if ((int)is->fread(cloud_time, TIME_DESC_LENGTH) < TIME_DESC_LENGTH) return true;
	// read current branch and flag of changes since it
	uint8 tmp;
	if (!read8le(&tmp, is)) return true;
	current_branch = *(int8*)(&tmp);
	if (!read8le(&tmp, is)) return true;
	changes_since_current_branch = (tmp != 0);
	// read current_position time
	if ((int)is->fread(current_pos_time, TIME_DESC_LENGTH) < TIME_DESC_LENGTH) return true;
	// read all 10 bookmarks
	for (int i = 0; i < TOTAL_BOOKMARKS; ++i)
	{
		if (bookmarks_array[i].load(is)) return true;
	}
	// all ok
	reset();
	RedrawBookmarksCaption();
	return false;
}
// ----------------------------------------------------------
void BOOKMARKS::RedrawBookmarksCaption()
{
	int prev_edit_mode = edit_mode;
	if (TASEdit_branch_only_when_rec && movie_readonly)
	{
		edit_mode = EDIT_MODE_BOOKMARKS;
		ShowWindow(hwndBranchesBitmap, SW_HIDE);
		ShowWindow(hwndBookmarksList, SW_SHOW);
		RedrawBookmarksList();
	} else if (TASEdit_view_branches_tree)
	{
		edit_mode = EDIT_MODE_BRANCHES;
		ShowWindow(hwndBookmarksList, SW_HIDE);
		ShowWindow(hwndBranchesBitmap, SW_SHOW);
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
void BOOKMARKS::RedrawBookmarksList()
{
	if (edit_mode != EDIT_MODE_BRANCHES)
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

void BOOKMARKS::RedrawBranchesTree()
{
	// draw background gradient
	TRIVERTEX vertex[2] ;
	vertex[0].x     = 0;
	vertex[0].y     = 0;
	vertex[0].Red   = 0xC700;
	vertex[0].Green = 0xE700;
	vertex[0].Blue  = 0xF300;
	vertex[0].Alpha = 0x0000;
	vertex[1].x     = BRANCHES_BITMAP_WIDTH;
	vertex[1].y     = BRANCHES_BITMAP_HEIGHT;
	vertex[1].Red   = 0xEB00;
	vertex[1].Green = 0xFA00;
	vertex[1].Blue  = 0xF800;
	vertex[1].Alpha = 0x0000;
	GRADIENT_RECT gRect;
	gRect.UpperLeft  = 0;
	gRect.LowerRight = 1;
	GradientFill(hBitmapDC, vertex, 2, &gRect, 1, GRADIENT_FILL_RECT_H);

	// lines
	int branch_x, branch_y, parent_x, parent_y, child_id;
	SelectObject(hBitmapDC, normal_pen);
	for (int t = Children.size() - 1; t >= 0; t--)
	{
		if (t > 0)
		{
			parent_x = BranchCurrX[t-1];
			parent_y = BranchCurrY[t-1];
		} else
		{
			parent_x = cloud_x;
			parent_y = BRANCHES_CLOUD_Y;
		}
		for (int i = Children[t].size() - 1; i >= 0; i--)
		{
			child_id = Children[t][i];
			if (child_id < TOTAL_BOOKMARKS)
			{
				MoveToEx(hBitmapDC, parent_x, parent_y, 0);
				LineTo(hBitmapDC, BranchCurrX[child_id], BranchCurrY[child_id]);
			}
		}
	}
	// lines for item under mouse
	SelectObject(hBitmapDC, select_pen);
	int branch = item_under_mouse;
	if (item_under_mouse == TOTAL_BOOKMARKS)
		branch = current_branch;
	while (branch >= 0)
	{
		branch_x = BranchCurrX[branch];
		branch_y = BranchCurrY[branch];
		MoveToEx(hBitmapDC, branch_x, branch_y, 0);
		branch = bookmarks_array[branch].parent_branch;
		if (branch >= 0)
		{
			branch_x = BranchCurrX[branch];
			branch_y = BranchCurrY[branch];
		} else
		{
			branch_x = cloud_x;
			branch_y = BRANCHES_CLOUD_Y;
		}
		LineTo(hBitmapDC, branch_x, branch_y);
	}
	if (changes_since_current_branch)
	{
		if (item_under_mouse != TOTAL_BOOKMARKS)
			SelectObject(hBitmapDC, normal_pen);
		if (current_branch >= 0)
		{
			parent_x = BranchCurrX[current_branch];
			parent_y = BranchCurrY[current_branch];
		} else
		{
			parent_x = cloud_x;
			parent_y = BRANCHES_CLOUD_Y;
		}
		MoveToEx(hBitmapDC, parent_x, parent_y, 0);
		branch_x = BranchCurrX[TOTAL_BOOKMARKS];
		branch_y = BranchCurrY[TOTAL_BOOKMARKS];
		LineTo(hBitmapDC, branch_x, branch_y);
	}

	// cloud
	TransparentBlt(hBitmapDC, cloud_x - BRANCHES_CLOUD_HALFWIDTH, BRANCHES_CLOUD_Y - BRANCHES_CLOUD_HALFHEIGHT, BRANCHES_CLOUD_WIDTH, BRANCHES_CLOUD_HEIGHT, hSpritesheetDC, BRANCHES_CLOUD_SPRITESHEET_X, BRANCHES_CLOUD_SPRITESHEET_Y, BRANCHES_CLOUD_WIDTH, BRANCHES_CLOUD_HEIGHT, 0x00FF00);
	
	// branches rectangles
	for (int i = 0; i < TOTAL_BOOKMARKS; ++i)
	{
		temp_rect.left = BranchCurrX[i] - DIGIT_RECT_HALFWIDTH;
		temp_rect.top = BranchCurrY[i] - DIGIT_RECT_HALFHEIGHT;
		temp_rect.right = temp_rect.left + DIGIT_RECT_WIDTH;
		temp_rect.bottom = temp_rect.top + DIGIT_RECT_HEIGHT;
		if (bookmarks_array[i].flash_phase)
		{
			// draw colored rect
			HBRUSH color_brush = CreateSolidBrush(bookmark_flash_colors[bookmarks_array[i].flash_type][bookmarks_array[i].flash_phase]);
			FillRect(hBitmapDC, &temp_rect, color_brush);
			DeleteObject(color_brush);
		} else
		{
			// draw black rect
			FillRect(hBitmapDC, &temp_rect, normal_brush);
		}
	}
	// digits
	for (int i = 0; i < TOTAL_BOOKMARKS; ++i)
	{
		branch_x = BranchCurrX[i] - DIGIT_BITMAP_HALFWIDTH;
		branch_y = BranchCurrY[i] - DIGIT_BITMAP_HALFHEIGHT;
		if(i == current_branch)
		{
			if (i == item_under_mouse)
				BitBlt(hBitmapDC, branch_x, branch_y, DIGIT_BITMAP_WIDTH, DIGIT_BITMAP_HEIGHT, hSpritesheetDC, i * DIGIT_BITMAP_WIDTH + BLUE_DIGITS_SPRITESHEET_DX, MOUSEOVER_DIGITS_SPRITESHEET_DY, SRCCOPY);
			else
				BitBlt(hBitmapDC, branch_x, branch_y, DIGIT_BITMAP_WIDTH, DIGIT_BITMAP_HEIGHT, hSpritesheetDC, i * DIGIT_BITMAP_WIDTH + BLUE_DIGITS_SPRITESHEET_DX, 0, SRCCOPY);
		} else
		{
			if (i == item_under_mouse)
				BitBlt(hBitmapDC, branch_x, branch_y, DIGIT_BITMAP_WIDTH, DIGIT_BITMAP_HEIGHT, hSpritesheetDC, i * DIGIT_BITMAP_WIDTH, MOUSEOVER_DIGITS_SPRITESHEET_DY, SRCCOPY);
			else
				BitBlt(hBitmapDC, branch_x, branch_y, DIGIT_BITMAP_WIDTH, DIGIT_BITMAP_HEIGHT, hSpritesheetDC, i * DIGIT_BITMAP_WIDTH, 0, SRCCOPY);
		}
	}
	// time of item under cursor
	if (item_under_mouse > ITEM_UNDER_MOUSE_NONE)
	{
		if (item_under_mouse == ITEM_UNDER_MOUSE_CLOUD)
			TextOut(hBitmapDC, 0, 0, (LPCSTR)cloud_time, 8);
		else if (item_under_mouse < TOTAL_BOOKMARKS)
			TextOut(hBitmapDC, 0, 0, (LPCSTR)bookmarks_array[item_under_mouse].snapshot.description, 8);
		else
			TextOut(hBitmapDC, 0, 0, (LPCSTR)current_pos_time, 8);
	}
	// finished
	must_redraw_branches_tree = false;
	InvalidateRect(hwndBranchesBitmap, 0, FALSE);
}

void BOOKMARKS::ChangeScreenshotBitmap()
{
	// uncompress
	uLongf destlen = SCREENSHOT_SIZE;
	int e = uncompress(&scr_ptr[0], &destlen, &bookmarks_array[item_under_mouse].saved_screenshot[0], bookmarks_array[item_under_mouse].saved_screenshot.size());
	if (e != Z_OK && e != Z_BUF_ERROR)
	{
		// error decompressing
		FCEU_printf("Error decompressing screenshot %d\n", item_under_mouse);
		// at least fill bitmap with zeros
		memset(&scr_ptr[0], 0, SCREENSHOT_SIZE);
	}
	screenshot_currently_shown = item_under_mouse;
	RedrawScreenshotBitmap();
}
void BOOKMARKS::RedrawScreenshotBitmap()
{
	HBITMAP temp_bmp = (HBITMAP)SendMessage(scr_bmp_pic, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)scr_bmp);
	if (temp_bmp && temp_bmp != scr_bmp)
		DeleteObject(temp_bmp);
}

// this is called by wndproc on WM_PAINT
void BOOKMARKS::PaintBranchesBitmap(HDC hdc)
{
	int branch_x, branch_y;
	// "bg"
	BitBlt(hBufferDC, 0, 0, BRANCHES_BITMAP_WIDTH, BRANCHES_BITMAP_HEIGHT, hBitmapDC, 0, 0, SRCCOPY);
	// "sprites"
	// fireball
	if (fireball_size)
	{
		branch_x = BranchCurrX[TOTAL_BOOKMARKS] - BRANCHES_FIREBALL_HALFWIDTH;
		branch_y = BranchCurrY[TOTAL_BOOKMARKS] - BRANCHES_FIREBALL_HALFHEIGHT;
		if (fireball_size >= BRANCHES_FIREBALL_MAX_SIZE)
		{
			TransparentBlt(hBufferDC, branch_x, branch_y, BRANCHES_FIREBALL_WIDTH, BRANCHES_FIREBALL_HEIGHT, hSpritesheetDC, animation_frame * BRANCHES_FIREBALL_WIDTH + BRANCHES_FIREBALL_SPRITESHEET_X, BRANCHES_FIREBALL_SPRITESHEET_Y, BRANCHES_FIREBALL_WIDTH, BRANCHES_FIREBALL_HEIGHT, 0x00FF00);
		} else
		{
			TransparentBlt(hBufferDC, branch_x, branch_y, BRANCHES_FIREBALL_WIDTH, BRANCHES_FIREBALL_HEIGHT, hSpritesheetDC, BRANCHES_FIREBALL_SPRITESHEET_END_X - fireball_size * BRANCHES_FIREBALL_WIDTH, BRANCHES_FIREBALL_SPRITESHEET_Y, BRANCHES_FIREBALL_WIDTH, BRANCHES_FIREBALL_HEIGHT, 0x00FF00);
		}
	}
	// corners cursor
	branch_x = (CursorX * (BRANCHES_TRANSITION_MAX - transition_phase) + CursorPrevX * transition_phase) / BRANCHES_TRANSITION_MAX;
	branch_y = (CursorY * (BRANCHES_TRANSITION_MAX - transition_phase) + CursorPrevY * transition_phase) / BRANCHES_TRANSITION_MAX;
	int current_corners_cursor_shift = BRANCHES_CORNER_BASE_SHIFT + corners_cursor_shift[animation_frame];
	int corner_x, corner_y;
	// upper left
	corner_x = branch_x - current_corners_cursor_shift - BRANCHES_CORNER_HALFWIDTH;
	corner_y = branch_y - current_corners_cursor_shift - BRANCHES_CORNER_HALFHEIGHT;
	TransparentBlt(hBufferDC, corner_x, corner_y, BRANCHES_CORNER_WIDTH, BRANCHES_CORNER_HEIGHT, hSpritesheetDC, BRANCHES_CORNER1_SPRITESHEET_X, BRANCHES_CORNER1_SPRITESHEET_Y, BRANCHES_CORNER_WIDTH, BRANCHES_CORNER_HEIGHT, 0x00FF00);
	// upper right
	corner_x = branch_x + current_corners_cursor_shift - BRANCHES_CORNER_HALFWIDTH;
	corner_y = branch_y - current_corners_cursor_shift - BRANCHES_CORNER_HALFHEIGHT;
	TransparentBlt(hBufferDC, corner_x, corner_y, BRANCHES_CORNER_WIDTH, BRANCHES_CORNER_HEIGHT, hSpritesheetDC, BRANCHES_CORNER2_SPRITESHEET_X, BRANCHES_CORNER2_SPRITESHEET_Y, BRANCHES_CORNER_WIDTH, BRANCHES_CORNER_HEIGHT, 0x00FF00);
	// lower left
	corner_x = branch_x - current_corners_cursor_shift - BRANCHES_CORNER_HALFWIDTH;
	corner_y = branch_y + current_corners_cursor_shift - BRANCHES_CORNER_HALFHEIGHT;
	TransparentBlt(hBufferDC, corner_x, corner_y, BRANCHES_CORNER_WIDTH, BRANCHES_CORNER_HEIGHT, hSpritesheetDC, BRANCHES_CORNER3_SPRITESHEET_X, BRANCHES_CORNER3_SPRITESHEET_Y, BRANCHES_CORNER_WIDTH, BRANCHES_CORNER_HEIGHT, 0x00FF00);
	// lower right
	corner_x = branch_x + current_corners_cursor_shift - BRANCHES_CORNER_HALFWIDTH;
	corner_y = branch_y + current_corners_cursor_shift - BRANCHES_CORNER_HALFHEIGHT;
	TransparentBlt(hBufferDC, corner_x, corner_y, BRANCHES_CORNER_WIDTH, BRANCHES_CORNER_HEIGHT, hSpritesheetDC, BRANCHES_CORNER4_SPRITESHEET_X, BRANCHES_CORNER4_SPRITESHEET_Y, BRANCHES_CORNER_WIDTH, BRANCHES_CORNER_HEIGHT, 0x00FF00);
	// finish - paste buffer bitmap to screen
	BitBlt(hdc, 0, 0, BRANCHES_BITMAP_WIDTH, BRANCHES_BITMAP_HEIGHT, hBufferDC, 0, 0, SRCCOPY);
}

void BOOKMARKS::MouseMove(int new_x, int new_y)
{
	mouse_x = new_x;
	mouse_y = new_y;
	must_check_item_under_mouse = true;
}
void BOOKMARKS::CheckMousePos()
{
	int prev_item_under_mouse = item_under_mouse;
	if (edit_mode == EDIT_MODE_BRANCHES)
	{
		// Mouse over Branches bitmap
		// first calculate current positions of branch items
		for (int i = 0; i <= TOTAL_BOOKMARKS; ++i)
		{
			BranchCurrX[i] = (BranchX[i] * (BRANCHES_TRANSITION_MAX - transition_phase) + BranchPrevX[i] * transition_phase) / BRANCHES_TRANSITION_MAX;
			BranchCurrY[i] = (BranchY[i] * (BRANCHES_TRANSITION_MAX - transition_phase) + BranchPrevY[i] * transition_phase) / BRANCHES_TRANSITION_MAX;
		}
		cloud_x = (CloudX * (BRANCHES_TRANSITION_MAX - transition_phase) + CloudPrevX * transition_phase) / BRANCHES_TRANSITION_MAX;
		// find item under mouse
		item_under_mouse = ITEM_UNDER_MOUSE_NONE;
		for (int i = 0; i < TOTAL_BOOKMARKS; ++i)
			if (item_under_mouse == ITEM_UNDER_MOUSE_NONE && mouse_x >= BranchCurrX[i] - DIGIT_RECT_HALFWIDTH && mouse_x < BranchCurrX[i] - DIGIT_RECT_HALFWIDTH + DIGIT_RECT_WIDTH && mouse_y >= BranchCurrY[i] - DIGIT_RECT_HALFHEIGHT && mouse_y < BranchCurrY[i] - DIGIT_RECT_HALFHEIGHT + DIGIT_RECT_HEIGHT)
				item_under_mouse = i;
		if (item_under_mouse == ITEM_UNDER_MOUSE_NONE && mouse_x >= cloud_x - BRANCHES_CLOUD_HALFWIDTH && mouse_x < cloud_x - BRANCHES_CLOUD_HALFWIDTH + BRANCHES_CLOUD_WIDTH && mouse_y >= BRANCHES_CLOUD_Y - BRANCHES_CLOUD_HALFHEIGHT && mouse_y < BRANCHES_CLOUD_Y - BRANCHES_CLOUD_HALFHEIGHT + BRANCHES_CLOUD_HEIGHT)
			item_under_mouse = ITEM_UNDER_MOUSE_CLOUD;
		if (item_under_mouse == ITEM_UNDER_MOUSE_NONE && changes_since_current_branch && mouse_x >= BranchCurrX[TOTAL_BOOKMARKS] - DIGIT_RECT_HALFWIDTH && mouse_x < BranchCurrX[TOTAL_BOOKMARKS] - DIGIT_RECT_HALFWIDTH + DIGIT_RECT_WIDTH && mouse_y >= BranchCurrY[TOTAL_BOOKMARKS] - DIGIT_RECT_HALFHEIGHT && mouse_y < BranchCurrY[TOTAL_BOOKMARKS] - DIGIT_RECT_HALFHEIGHT + DIGIT_RECT_HEIGHT)
			item_under_mouse = TOTAL_BOOKMARKS;
		if (prev_item_under_mouse != item_under_mouse)
			must_redraw_branches_tree = true;
	} else if (edit_mode == EDIT_MODE_BOTH)
	{
		// Mouse over Bookmarks list
		if (mouse_x > branch_row_left)
		{
			item_under_mouse = (mouse_y - branch_row_top) / branch_row_height;
			if (item_under_mouse >= 0 && item_under_mouse < TOTAL_BOOKMARKS)
			{
				item_under_mouse = (item_under_mouse + 1) % TOTAL_BOOKMARKS;
				if (!bookmarks_array[item_under_mouse].not_empty)
					item_under_mouse = ITEM_UNDER_MOUSE_NONE;
			}
		} else item_under_mouse = ITEM_UNDER_MOUSE_NONE;
	} else item_under_mouse = ITEM_UNDER_MOUSE_NONE;
	must_check_item_under_mouse = false;
}
// ----------------------------------------------------------------------------------------
void BOOKMARKS::GetDispInfo(NMLVDISPINFO* nmlvDispInfo)
{
	LVITEM& item = nmlvDispInfo->item;
	if(item.mask & LVIF_TEXT)
	{
		switch(item.iSubItem)
		{
			case BOOKMARKS_COLUMN_ICON:
			{
				if ((item.iItem + 1) % TOTAL_BOOKMARKS == current_branch)
					item.iImage = ((item.iItem + 1) % TOTAL_BOOKMARKS) + TOTAL_BOOKMARKS;
				else
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

		if (cell_x == BOOKMARKS_COLUMN_FRAME || (TASEdit_branch_only_when_rec && movie_readonly && cell_x == BOOKMARKS_COLUMN_TIME))
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
		if (cell_x <= BOOKMARKS_COLUMN_FRAME || (TASEdit_branch_only_when_rec && movie_readonly))
			jump((cell_y + 1) % TOTAL_BOOKMARKS);
		else if (cell_x == BOOKMARKS_COLUMN_TIME && (!TASEdit_branch_only_when_rec || !movie_readonly))
			unleash((cell_y + 1) % TOTAL_BOOKMARKS);
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
	if (current_branch >= 0 && bookmarks_array[current_branch].snapshot.jump_frame == frame) return current_branch + TOTAL_BOOKMARKS;	// blue digit

	for (int i = 0; i < TOTAL_BOOKMARKS; ++i)
	{
		if (bookmarks_array[i].snapshot.jump_frame == frame) return i;	// green digit
	}
	return -1;
}

void BOOKMARKS::ChangesMadeSinceBranch()
{
	bool prev_changes_since_current_branch = changes_since_current_branch;
	changes_since_current_branch = true;
	SetCurrentPosTime();
	// recalculate branch tree if previous_changes = false
	if (!prev_changes_since_current_branch)
		must_recalculate_branches_tree = true;
	else if (edit_mode == EDIT_MODE_BRANCHES && item_under_mouse == TOTAL_BOOKMARKS)
		// redraw displayed time if it's currently shown
		must_redraw_branches_tree = true;
}

void BOOKMARKS::SetCurrentPosTime()
{
	time_t raw_time;
	time(&raw_time);
	struct tm * timeinfo = localtime(&raw_time);
	strftime(current_pos_time, TIME_DESC_LENGTH, "%H:%M:%S", timeinfo);
}

void BOOKMARKS::RecalculateBranchesTree()
{
	// save previous values
	for (int i = TOTAL_BOOKMARKS; i >= 0; i--)
	{
		BranchPrevX[i] = (BranchX[i] * (BRANCHES_TRANSITION_MAX - transition_phase) + BranchPrevX[i] * transition_phase) / BRANCHES_TRANSITION_MAX;
		BranchPrevY[i] = (BranchY[i] * (BRANCHES_TRANSITION_MAX - transition_phase) + BranchPrevY[i] * transition_phase) / BRANCHES_TRANSITION_MAX;
	}
	CloudPrevX = (CloudX * (BRANCHES_TRANSITION_MAX - transition_phase) + CloudPrevX * transition_phase) / BRANCHES_TRANSITION_MAX;
	CursorPrevX = (CursorX * (BRANCHES_TRANSITION_MAX - transition_phase) + CursorPrevX * transition_phase) / BRANCHES_TRANSITION_MAX;
	CursorPrevY = (CursorY * (BRANCHES_TRANSITION_MAX - transition_phase) + CursorPrevY * transition_phase) / BRANCHES_TRANSITION_MAX;
	transition_phase = BRANCHES_TRANSITION_MAX;

	// 0. Prepare arrays
	GridX.resize(0);
	GridY.resize(0);
	Children.resize(0);
	GridHeight.resize(0);
	GridX.resize(TOTAL_BOOKMARKS+1);
	GridY.resize(TOTAL_BOOKMARKS+1);
	Children.resize(TOTAL_BOOKMARKS+2);		// 0th subvector is for cloud's children
	GridHeight.resize(TOTAL_BOOKMARKS+1);
	for (int i = TOTAL_BOOKMARKS; i >= 0; i--)
		GridHeight[i] = 1;

	// 1. Define GridX of branches (distribute to levels) and GridHeight of branches
	int current_grid_x = 0;
	std::vector<std::vector<int>> BranchesLevels;

	std::vector<uint8> UndistributedBranches;
	UndistributedBranches.resize(TOTAL_BOOKMARKS);
	for (int i = UndistributedBranches.size()-1; i >= 0; i--)
		UndistributedBranches[i] = i;
	// remove all empty branches
	for (int i = UndistributedBranches.size()-1; i >= 0; i--)
	{
		if (!bookmarks_array[UndistributedBranches[i]].not_empty)
			UndistributedBranches.erase(UndistributedBranches.begin() + i);
	}
	// highest level: cloud (id = -1)
	BranchesLevels.resize(current_grid_x+1);
	BranchesLevels[current_grid_x].resize(1);
	BranchesLevels[current_grid_x][0] = -1;
	// go lower until all branches are arranged to levels
	int current_parent;
	while(UndistributedBranches.size())
	{
		current_grid_x++;
		BranchesLevels.resize(current_grid_x+1);
		BranchesLevels[current_grid_x].resize(0);
		for (int t = BranchesLevels[current_grid_x-1].size()-1; t >= 0; t--)
		{
			current_parent = BranchesLevels[current_grid_x-1][t];
			for (int i = UndistributedBranches.size()-1; i >= 0; i--)
			{
				if (bookmarks_array[UndistributedBranches[i]].parent_branch == current_parent)
				{
					// assign this branch to current level
					GridX[UndistributedBranches[i]] = current_grid_x;
					BranchesLevels[current_grid_x].push_back(UndistributedBranches[i]);
					// also add it to parent's Children array
					Children[current_parent+1].push_back(UndistributedBranches[i]);
					UndistributedBranches.erase(UndistributedBranches.begin() + i);
				}
			}
			if (current_parent >= 0)
			{
				GridHeight[current_parent] = Children[current_parent+1].size();
				if (Children[current_parent+1].size() > 1)
					RecursiveAddHeight(bookmarks_array[current_parent].parent_branch, GridHeight[current_parent] - 1);
				else
					GridHeight[current_parent] = 1;		// its own height
			}
		}
	}
	if (changes_since_current_branch)
	{
		// also define "current_pos" GridX
		if (current_branch >= 0)
		{
			if (Children[current_branch+1].size() < MAX_NUM_CHILDREN)
			{
				// "current_pos" becomes a child of current_branch
				GridX[TOTAL_BOOKMARKS] = GridX[current_branch] + 1;
				if ((int)BranchesLevels.size() <= GridX[TOTAL_BOOKMARKS])
					BranchesLevels.resize(GridX[TOTAL_BOOKMARKS] + 1);
				BranchesLevels[GridX[TOTAL_BOOKMARKS]].push_back(TOTAL_BOOKMARKS);	// ID of "current_pos" = number of bookmarks
				Children[current_branch+1].push_back(TOTAL_BOOKMARKS);
				if (Children[current_branch+1].size() > 1)
					RecursiveAddHeight(current_branch, 1);
			} else
			{
				// special case 0: if there's too many items on one level (more than canvas can show)
				// "current_pos" becomes special branch above current_branch
				GridX[TOTAL_BOOKMARKS] = GridX[current_branch];
				GridY[TOTAL_BOOKMARKS] = GridY[current_branch] - 7;
			}
		} else
		{
			// special case 1: one and (presumably) only child of "cloud"
			GridX[TOTAL_BOOKMARKS] = 1;
			GridY[TOTAL_BOOKMARKS] = 0;
			if ((int)BranchesLevels.size() <= GridX[TOTAL_BOOKMARKS])
				BranchesLevels.resize(GridX[TOTAL_BOOKMARKS] + 1);
			BranchesLevels[GridX[TOTAL_BOOKMARKS]].push_back(TOTAL_BOOKMARKS);
		}
	}
	// define grid_width
	int grid_width, cloud_prefix = 0;
	if (BranchesLevels.size()-1 > 0)
	{
		grid_width = BRANCHES_CANVAS_WIDTH / (BranchesLevels.size()-1);
		if (grid_width < BRANCHES_GRID_MIN_WIDTH)
			grid_width = BRANCHES_GRID_MIN_WIDTH;
		else if (grid_width > BRANCHES_GRID_MAX_WIDTH)
			grid_width = BRANCHES_GRID_MAX_WIDTH;
	} else grid_width = BRANCHES_GRID_MAX_WIDTH;
	if (grid_width < MIN_CLOUD_LINE_LENGTH)
		cloud_prefix = MIN_CLOUD_LINE_LENGTH - grid_width;

	// 2. Define GridY of branches
	RecursiveSetYPos(-1, 0);
	// define grid_halfheight
	int grid_halfheight;
	int totalHeight = 0;
	for (int i = Children[0].size()-1; i >= 0; i--)
		totalHeight += GridHeight[Children[0][i]];
	if (totalHeight)
	{
		grid_halfheight = BRANCHES_CANVAS_HEIGHT / (2 * totalHeight);
		if (grid_halfheight < BRANCHES_GRID_MIN_HALFHEIGHT)
			grid_halfheight = BRANCHES_GRID_MIN_HALFHEIGHT;
		else if (grid_halfheight > BRANCHES_GRID_MAX_HALFHEIGHT)
			grid_halfheight = BRANCHES_GRID_MAX_HALFHEIGHT;
	} else grid_halfheight = BRANCHES_GRID_MAX_HALFHEIGHT;
	// special case 2: if chain of branches is too long, last item (normally it's fireball) goes up
	if (changes_since_current_branch)
	{
		if (GridX[TOTAL_BOOKMARKS] > MAX_CHAIN_LEN)
		{
			GridX[TOTAL_BOOKMARKS] = MAX_CHAIN_LEN;
			GridY[TOTAL_BOOKMARKS] -= 2;
		}
	}
	// special case 3: if some branch crosses upper or lower border
	int parent;
	for (int t = TOTAL_BOOKMARKS; t >= 0; t--)
	{
		if (GridY[t] > MAX_GRID_Y_POS)
		{
			if (t < TOTAL_BOOKMARKS)
				parent = bookmarks_array[t].parent_branch;
			else
				parent = current_branch;
			int pos = MAX_GRID_Y_POS;
			for (int i = 0; i < (int)Children[parent+1].size(); ++i)
			{
				GridY[Children[parent+1][i]] = pos;
				pos -= 2;
			}
		} else if (GridY[t] < -MAX_GRID_Y_POS)
		{
			if (t < TOTAL_BOOKMARKS)
				parent = bookmarks_array[t].parent_branch;
			else
				parent = current_branch;
			int pos = -MAX_GRID_Y_POS;
			for (int i = Children[parent+1].size()-1; i >= 0; i--)
			{
				GridY[Children[parent+1][i]] = pos;
				pos += 2;
			}
		}
	}

	// 3. Set pixel positions of branches
	int max_x = 0;
	for (int i = TOTAL_BOOKMARKS-1; i >= 0; i--)
	{
		if (bookmarks_array[i].not_empty)
		{
			BranchX[i] = cloud_prefix + GridX[i] * grid_width;
			BranchY[i] = BRANCHES_CLOUD_Y + GridY[i] * grid_halfheight;
		} else
		{
			BranchX[i] = EMPTY_BRANCHES_X;
			BranchY[i] = EMPTY_BRANCHES_Y_BASE + EMPTY_BRANCHES_Y_FACTOR * ((i + TOTAL_BOOKMARKS - 1) % TOTAL_BOOKMARKS);
		}
		if (max_x < BranchX[i]) max_x = BranchX[i];
	}
	if (changes_since_current_branch)
	{
		// also set pixel position of "current_pos"
		BranchX[TOTAL_BOOKMARKS] = cloud_prefix + GridX[TOTAL_BOOKMARKS] * grid_width;
		BranchY[TOTAL_BOOKMARKS] = BRANCHES_CLOUD_Y + GridY[TOTAL_BOOKMARKS] * grid_halfheight;
	} else if (current_branch >= 0)
	{
		BranchX[TOTAL_BOOKMARKS] = cloud_prefix + GridX[current_branch] * grid_width;
		BranchY[TOTAL_BOOKMARKS] = BRANCHES_CLOUD_Y + GridY[current_branch] * grid_halfheight;
	} else
	{
		BranchX[TOTAL_BOOKMARKS] = 0;
		BranchY[TOTAL_BOOKMARKS] = BRANCHES_CLOUD_Y;
	}
	if (max_x < BranchX[TOTAL_BOOKMARKS]) max_x = BranchX[TOTAL_BOOKMARKS];
	// align whole tree horizontally
	CloudX = (BRANCHES_BITMAP_WIDTH + 1 - max_x) / 2;
	if (CloudX < 0) CloudX = 0;
	for (int i = TOTAL_BOOKMARKS-1; i >= 0; i--)
		if (bookmarks_array[i].not_empty)
			BranchX[i] += CloudX;
	BranchX[TOTAL_BOOKMARKS] += CloudX;
	// target cursor
	CursorX = BranchX[TOTAL_BOOKMARKS];
	CursorY = BranchY[TOTAL_BOOKMARKS];
	// finished recalculating
	must_recalculate_branches_tree = false;
	must_redraw_branches_tree = true;
}
void BOOKMARKS::RecursiveAddHeight(int branch_num, int amount)
{
	if (branch_num >= 0)
	{
		GridHeight[branch_num] += amount;
		if (bookmarks_array[branch_num].parent_branch >= 0)
			RecursiveAddHeight(bookmarks_array[branch_num].parent_branch, amount);
	}
}
void BOOKMARKS::RecursiveSetYPos(int parent, int parentY)
{
	if (Children[parent+1].size())
	{
		// find total height of children
		int totalHeight = 0;
		for (int i = Children[parent+1].size()-1; i >= 0; i--)
			totalHeight += GridHeight[Children[parent+1][i]];
		// set Y of children and subchildren
		for (int i = Children[parent+1].size()-1; i >= 0; i--)
		{
			int child_id = Children[parent+1][i];
			GridY[child_id] = parentY + GridHeight[child_id] - totalHeight;
			RecursiveSetYPos(child_id, GridY[child_id]);
			parentY += 2 * GridHeight[child_id];
		}
	}
}
// ----------------------------------------------------------------------------------------
LRESULT CALLBACK ScrBmpWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
		case WM_CREATE:
		{
			// create static bitmap placeholder
			scr_bmp_pic = CreateWindow(WC_STATIC, NULL, SS_BITMAP | WS_CHILD | WS_VISIBLE, 0, 0, 255, 255, hwnd, NULL, NULL, NULL);
			return 0;
		}
		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
	}
}


