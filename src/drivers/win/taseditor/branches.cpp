/* ---------------------------------------------------------------------------------
Implementation file of Branches class
Copyright (c) 2011-2012 AnS

(The MIT License)
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------------
Branches - Manager of Branches
[Singleton]

* stores info about Branches (relations of Bookmarks) and the id of current Branch
* also stores the time of the last modification (see fireball) and the time of the root of Branches Tree (see cloudlet)
* finds best place for every new Bookmark in the Branches Tree
* saves and loads the data from a project file. On error: sends warning to caller
* implements the working of Branches Tree: creating, recalculating relations, animating, redrawing, mouseover
* on demand: reacts on project changes and recalculates Branches Tree
* regularly updates animations in Branches Tree
* stores resources: coordinates for building Branches Tree, animation timings
------------------------------------------------------------------------------------ */

#include "taseditor_project.h"
#include "utils/xstring.h"
#include "zlib.h"

#pragma comment(lib, "msimg32.lib")

LRESULT APIENTRY BranchesBitmapWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
WNDPROC hwndBranchesBitmap_oldWndProc;

extern TASEDITOR_CONFIG taseditor_config;
extern TASEDITOR_WINDOW taseditor_window;
extern POPUP_DISPLAY popup_display;
extern PLAYBACK playback;
extern SELECTION selection;
extern GREENZONE greenzone;
extern TASEDITOR_PROJECT project;
extern HISTORY history;
extern PIANO_ROLL piano_roll;
extern MARKERS_MANAGER markers_manager;
extern BOOKMARKS bookmarks;

extern COLORREF bookmark_flash_colors[TOTAL_COMMANDS][FLASH_PHASE_MAX+1];

// resources
// corners cursor animation
int corners_cursor_shift[BRANCHES_ANIMATION_FRAMES] = {0, 1, 2, 3, 4, 5, 5, 4, 3, 2, 1, 0 };

BRANCHES::BRANCHES()
{
}

void BRANCHES::init()
{
	free();

	// subclass BranchesBitmap
	hwndBranchesBitmap_oldWndProc = (WNDPROC)SetWindowLong(bookmarks.hwndBranchesBitmap, GWL_WNDPROC, (LONG)BranchesBitmapWndProc);

	// init arrays
	BranchX.resize(TOTAL_BOOKMARKS+1);
	BranchY.resize(TOTAL_BOOKMARKS+1);
	BranchPrevX.resize(TOTAL_BOOKMARKS+1);
	BranchPrevY.resize(TOTAL_BOOKMARKS+1);
	BranchCurrX.resize(TOTAL_BOOKMARKS+1);
	BranchCurrY.resize(TOTAL_BOOKMARKS+1);

	// init GDI stuff
	HDC win_hdc = GetWindowDC(bookmarks.hwndBranchesBitmap);
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

	reset();
	next_animation_time = 0;

	update();
}
void BRANCHES::free()
{
	parents.resize(0);
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
}
void BRANCHES::reset()
{
	parents.resize(TOTAL_BOOKMARKS);
	for (int i = TOTAL_BOOKMARKS-1; i >= 0; i--)
		parents[i] = ITEM_UNDER_MOUSE_CLOUD;

	// set positions of slots to default coordinates
	for (int i = TOTAL_BOOKMARKS; i >= 0; i--)
	{
		BranchX[i] = BranchPrevX[i] = BranchCurrX[i] = EMPTY_BRANCHES_X;
		BranchY[i] = BranchPrevY[i] = BranchCurrY[i] = EMPTY_BRANCHES_Y_BASE + EMPTY_BRANCHES_Y_FACTOR * ((i + TOTAL_BOOKMARKS - 1) % TOTAL_BOOKMARKS);
	}
	CursorX = CursorPrevX = CloudX = CloudPrevX = BRANCHES_CLOUD_X;
	CursorY = CursorPrevY = BRANCHES_CLOUD_Y;

	reset_vars();

	current_branch = ITEM_UNDER_MOUSE_CLOUD;
	changes_since_current_branch = false;
	fireball_size = 0;

	// set cloud_time and current_pos_time
	SetCurrentPosTime();
	strcpy(cloud_time, current_pos_time);
}
void BRANCHES::reset_vars()
{
	transition_phase = animation_frame = 0;
	must_recalculate_branches_tree = must_redraw_branches_tree = true;
	next_animation_time = clock() + BRANCHES_ANIMATION_TICK;
}

void BRANCHES::update()
{
	if (must_recalculate_branches_tree)
		RecalculateBranchesTree();

	// once per 50 milliseconds update branches_bitmap
	if (clock() > next_animation_time)
	{
		// animate branches_bitmap
		next_animation_time = clock() + BRANCHES_ANIMATION_TICK;
		animation_frame = (animation_frame + 1) % BRANCHES_ANIMATION_FRAMES;
		if (bookmarks.edit_mode == EDIT_MODE_BRANCHES)
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
				// recalculate current positions of branch items
				for (int i = 0; i <= TOTAL_BOOKMARKS; ++i)
				{
					BranchCurrX[i] = (BranchX[i] * (BRANCHES_TRANSITION_MAX - transition_phase) + BranchPrevX[i] * transition_phase) / BRANCHES_TRANSITION_MAX;
					BranchCurrY[i] = (BranchY[i] * (BRANCHES_TRANSITION_MAX - transition_phase) + BranchPrevY[i] * transition_phase) / BRANCHES_TRANSITION_MAX;
				}
				cloud_x = (CloudX * (BRANCHES_TRANSITION_MAX - transition_phase) + CloudPrevX * transition_phase) / BRANCHES_TRANSITION_MAX;
				must_redraw_branches_tree = true;
				bookmarks.must_check_item_under_mouse = true;
			} else if (!must_redraw_branches_tree)
			{
				// just update sprites
				InvalidateRect(bookmarks.hwndBranchesBitmap, 0, FALSE);
			}
			if (must_redraw_branches_tree)
				RedrawBranchesTree();
		}
	}
}

void BRANCHES::save(EMUFILE *os)
{
	// write cloud time
	os->fwrite(cloud_time, TIME_DESC_LENGTH);
	// write current branch and flag of changes since it
	write32le(current_branch, os);
	if (changes_since_current_branch)
		write8le((uint8)1, os);
	else
		write8le((uint8)0, os);
	// write current_position time
	os->fwrite(current_pos_time, TIME_DESC_LENGTH);
	// write all 10 parents
	for (int i = 0; i < TOTAL_BOOKMARKS; ++i)
		write32le(parents[i], os);
}
// returns true if couldn't load
bool BRANCHES::load(EMUFILE *is)
{
	// read cloud time
	if ((int)is->fread(cloud_time, TIME_DESC_LENGTH) < TIME_DESC_LENGTH) goto error;
	// read current branch and flag of changes since it
	uint8 tmp;
	if (!read32le(&current_branch, is)) goto error;
	if (!read8le(&tmp, is)) goto error;
	changes_since_current_branch = (tmp != 0);
	// read current_position time
	if ((int)is->fread(current_pos_time, TIME_DESC_LENGTH) < TIME_DESC_LENGTH) goto error;
	// read all 10 parents
	for (int i = 0; i < TOTAL_BOOKMARKS; ++i)
		if (!read32le(&parents[i], is)) goto error;
	// all ok
	reset_vars();
	return false;
error:
	FCEU_printf("Error loading branches\n");
	return true;
}
// ----------------------------------------------------------
void BRANCHES::RedrawBranchesTree()
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
	int branch = bookmarks.item_under_mouse;
	if (branch == TOTAL_BOOKMARKS)
		branch = current_branch;
	while (branch >= 0)
	{
		branch_x = BranchCurrX[branch];
		branch_y = BranchCurrY[branch];
		MoveToEx(hBitmapDC, branch_x, branch_y, 0);
		branch = parents[branch];
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
		if (bookmarks.item_under_mouse != TOTAL_BOOKMARKS)
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
		if (bookmarks.bookmarks_array[i].flash_phase)
		{
			// draw colored rect
			HBRUSH color_brush = CreateSolidBrush(bookmark_flash_colors[bookmarks.bookmarks_array[i].flash_type][bookmarks.bookmarks_array[i].flash_phase]);
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
			if (i == bookmarks.item_under_mouse)
				BitBlt(hBitmapDC, branch_x, branch_y, DIGIT_BITMAP_WIDTH, DIGIT_BITMAP_HEIGHT, hSpritesheetDC, i * DIGIT_BITMAP_WIDTH + BLUE_DIGITS_SPRITESHEET_DX, MOUSEOVER_DIGITS_SPRITESHEET_DY, SRCCOPY);
			else
				BitBlt(hBitmapDC, branch_x, branch_y, DIGIT_BITMAP_WIDTH, DIGIT_BITMAP_HEIGHT, hSpritesheetDC, i * DIGIT_BITMAP_WIDTH + BLUE_DIGITS_SPRITESHEET_DX, 0, SRCCOPY);
		} else
		{
			if (i == bookmarks.item_under_mouse)
				BitBlt(hBitmapDC, branch_x, branch_y, DIGIT_BITMAP_WIDTH, DIGIT_BITMAP_HEIGHT, hSpritesheetDC, i * DIGIT_BITMAP_WIDTH, MOUSEOVER_DIGITS_SPRITESHEET_DY, SRCCOPY);
			else
				BitBlt(hBitmapDC, branch_x, branch_y, DIGIT_BITMAP_WIDTH, DIGIT_BITMAP_HEIGHT, hSpritesheetDC, i * DIGIT_BITMAP_WIDTH, 0, SRCCOPY);
		}
	}
	SetBkMode(hBitmapDC, TRANSPARENT);
	// jump_frame of item under cursor (except cloud - it doesn't have particular frame)
	if (bookmarks.item_under_mouse > ITEM_UNDER_MOUSE_CLOUD)
	{
		char framenum_string[DIGITS_IN_FRAMENUM+1] = {0};
		if (bookmarks.item_under_mouse < TOTAL_BOOKMARKS)
			U32ToDecStr(framenum_string, bookmarks.bookmarks_array[bookmarks.item_under_mouse].snapshot.jump_frame, DIGITS_IN_FRAMENUM);
		else
			U32ToDecStr(framenum_string, currFrameCounter, DIGITS_IN_FRAMENUM);
		SetTextColor(hBitmapDC, BRANCHES_TEXT_SHADOW_COLOR);
		TextOut(hBitmapDC, BRANCHES_BITMAP_FRAMENUM_X + 1, BRANCHES_BITMAP_FRAMENUM_Y + 1, (LPCSTR)framenum_string, DIGITS_IN_FRAMENUM);
		SetTextColor(hBitmapDC, BRANCHES_TEXT_COLOR);
		TextOut(hBitmapDC, BRANCHES_BITMAP_FRAMENUM_X, BRANCHES_BITMAP_FRAMENUM_Y, (LPCSTR)framenum_string, DIGITS_IN_FRAMENUM);
	}
	// time of item under cursor
	if (bookmarks.item_under_mouse > ITEM_UNDER_MOUSE_NONE)
	{
		if (bookmarks.item_under_mouse == ITEM_UNDER_MOUSE_CLOUD)
		{
			// draw shadow of text
			SetTextColor(hBitmapDC, BRANCHES_TEXT_SHADOW_COLOR);
			TextOut(hBitmapDC, BRANCHES_BITMAP_TIME_X + 1, BRANCHES_BITMAP_TIME_Y + 1, (LPCSTR)cloud_time, TIME_DESC_LENGTH-1);
			SetTextColor(hBitmapDC, BRANCHES_TEXT_COLOR);
			TextOut(hBitmapDC, BRANCHES_BITMAP_TIME_X, BRANCHES_BITMAP_TIME_Y, (LPCSTR)cloud_time, TIME_DESC_LENGTH-1);
		} else if (bookmarks.item_under_mouse < TOTAL_BOOKMARKS)
		{
			SetTextColor(hBitmapDC, BRANCHES_TEXT_SHADOW_COLOR);
			TextOut(hBitmapDC, BRANCHES_BITMAP_TIME_X + 1, BRANCHES_BITMAP_TIME_Y + 1, (LPCSTR)bookmarks.bookmarks_array[bookmarks.item_under_mouse].snapshot.description, TIME_DESC_LENGTH-1);
			SetTextColor(hBitmapDC, BRANCHES_TEXT_COLOR);
			TextOut(hBitmapDC, BRANCHES_BITMAP_TIME_X, BRANCHES_BITMAP_TIME_Y, (LPCSTR)bookmarks.bookmarks_array[bookmarks.item_under_mouse].snapshot.description, TIME_DESC_LENGTH-1);
		} else	// fireball - current_pos_time
		{
			SetTextColor(hBitmapDC, BRANCHES_TEXT_SHADOW_COLOR);
			TextOut(hBitmapDC, BRANCHES_BITMAP_TIME_X + 1, BRANCHES_BITMAP_TIME_Y + 1, (LPCSTR)current_pos_time, TIME_DESC_LENGTH-1);
			SetTextColor(hBitmapDC, BRANCHES_TEXT_COLOR);
			TextOut(hBitmapDC, BRANCHES_BITMAP_TIME_X, BRANCHES_BITMAP_TIME_Y, (LPCSTR)current_pos_time, TIME_DESC_LENGTH-1);
		}
	}
	// finished
	must_redraw_branches_tree = false;
	InvalidateRect(bookmarks.hwndBranchesBitmap, 0, FALSE);
}

// this is called by wndproc on WM_PAINT
void BRANCHES::PaintBranchesBitmap(HDC hdc)
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
// ----------------------------------------------------------------------------------------
int BRANCHES::GetCurrentBranch()
{
	return current_branch;
}

// this function finds best place for a new Bookmark in the Branches Tree
void BRANCHES::HandleBookmarkSet(int slot, char* slot_time)
{
	if (slot != current_branch)
	{
		// inherit current branch, forget previous parent
		int parent = parents[slot];
		if (parent == ITEM_UNDER_MOUSE_CLOUD && slot_time[0])
		{
			// check if this was the only child of cloud parent, if so then set cloud time to the saved_time
			int i = 0;
			for (; i < TOTAL_BOOKMARKS; ++i)
			{
				if (bookmarks.bookmarks_array[i].not_empty && parents[i] == ITEM_UNDER_MOUSE_CLOUD && i != slot)
					break;
			}
			if (i >= TOTAL_BOOKMARKS)
				// didn't find another child of cloud, so after this slot disconnects, cloud will have 0 children
				// it will mean that old cloud  disappears and new cloud is formed where the slot was
				strcpy(cloud_time, slot_time);
		}
		// before disconnecting from old parent, connect all childs to the old parent
		for (int i = 0; i < TOTAL_BOOKMARKS; ++i)
		{
			if (bookmarks.bookmarks_array[i].not_empty && parents[i] == slot)
				parents[i] = parent;
		}
		parents[slot] = current_branch;
	}

	// if parent is invalid (first_change < parent.jump_frame) then find better parent
	int factor = 0;
	// also if parent == cloud, then try to find better parent
	int parent = parents[slot];
	if (parent != ITEM_UNDER_MOUSE_CLOUD)
		factor = bookmarks.bookmarks_array[slot].snapshot.findFirstChange(bookmarks.bookmarks_array[parent].snapshot);
	if (parent == ITEM_UNDER_MOUSE_CLOUD || (factor >= 0 && factor < bookmarks.bookmarks_array[parent].snapshot.jump_frame))
	{
		// find highest frame of change
		std::vector<int> DecisiveFactor(TOTAL_BOOKMARKS);
		int best_branch = ITEM_UNDER_MOUSE_CLOUD;
		for (int i = TOTAL_BOOKMARKS-1; i >= 0; i--)
		{
			if (i != slot && i != parent && bookmarks.bookmarks_array[i].not_empty && bookmarks.bookmarks_array[slot].snapshot.size >= bookmarks.bookmarks_array[i].snapshot.jump_frame)
			{
				factor = bookmarks.bookmarks_array[slot].snapshot.findFirstChange(bookmarks.bookmarks_array[i].snapshot);
				if (factor < 0)
				{
					// this branch is identical to this slot
					DecisiveFactor[i] = 2 * bookmarks.bookmarks_array[i].snapshot.size;
				} else if (factor >= bookmarks.bookmarks_array[i].snapshot.jump_frame)
				{
					// hey, this branch could be our new parent...
					DecisiveFactor[i] = 2 * factor;
				} else
					DecisiveFactor[i] = 0;
			} else
			{
				DecisiveFactor[i] = 0;
			}
		}
		// add +1 as a bonus to current parents and grandparents (a bit of nepotism here)
		while (parent >= 0)
		{
			if (DecisiveFactor[parent])
				DecisiveFactor[parent]++;
			parent = parents[parent];
		}
		// find max
		factor = 0;
		for (int i = TOTAL_BOOKMARKS-1; i >= 0; i--)
		{
			if (DecisiveFactor[i] && DecisiveFactor[i] > factor)
			{
				factor = DecisiveFactor[i];
				best_branch = i;
			}
		}
		parent = parents[slot];
		if (parent != best_branch)
		{
			// before disconnecting from old parent, connect all childs to the old parent
			for (int i = 0; i < TOTAL_BOOKMARKS; ++i)
			{
				if (bookmarks.bookmarks_array[i].not_empty && parents[i] == slot)
					parents[i] = parent;
			}
			// found new parent
			parents[slot] = best_branch;
			must_recalculate_branches_tree = true;
		}
	}
	// switch current branch to this branch
	if (slot != current_branch || changes_since_current_branch)
		must_recalculate_branches_tree = true;
	current_branch = slot;
	changes_since_current_branch = false;
}
void BRANCHES::HandleBookmarkDeploy(int slot)
{
	current_branch = slot;
	changes_since_current_branch = false;
	must_recalculate_branches_tree = true;
}

void BRANCHES::ChangesMadeSinceBranch()
{
	bool prev_changes_since_current_branch = changes_since_current_branch;
	changes_since_current_branch = true;
	SetCurrentPosTime();
	// recalculate branch tree if previous_changes = false
	if (!prev_changes_since_current_branch)
		must_recalculate_branches_tree = true;
	else if (bookmarks.item_under_mouse == TOTAL_BOOKMARKS)
		must_redraw_branches_tree = true;	// to redraw fireball's time
}

void BRANCHES::FindItemUnderMouse(int mouse_x, int mouse_y)
{
	int prev_item_under_mouse = bookmarks.item_under_mouse;
	bookmarks.item_under_mouse = ITEM_UNDER_MOUSE_NONE;
	for (int i = 0; i < TOTAL_BOOKMARKS; ++i)
		if (bookmarks.item_under_mouse == ITEM_UNDER_MOUSE_NONE && mouse_x >= BranchCurrX[i] - DIGIT_RECT_HALFWIDTH_COLLISION && mouse_x < BranchCurrX[i] - DIGIT_RECT_HALFWIDTH_COLLISION + DIGIT_RECT_WIDTH_COLLISION && mouse_y >= BranchCurrY[i] - DIGIT_RECT_HALFHEIGHT_COLLISION && mouse_y < BranchCurrY[i] - DIGIT_RECT_HALFHEIGHT_COLLISION + DIGIT_RECT_HEIGHT_COLLISION)
			bookmarks.item_under_mouse = i;
	if (bookmarks.item_under_mouse == ITEM_UNDER_MOUSE_NONE && mouse_x >= cloud_x - BRANCHES_CLOUD_HALFWIDTH && mouse_x < cloud_x - BRANCHES_CLOUD_HALFWIDTH + BRANCHES_CLOUD_WIDTH && mouse_y >= BRANCHES_CLOUD_Y - BRANCHES_CLOUD_HALFHEIGHT && mouse_y < BRANCHES_CLOUD_Y - BRANCHES_CLOUD_HALFHEIGHT + BRANCHES_CLOUD_HEIGHT)
		bookmarks.item_under_mouse = ITEM_UNDER_MOUSE_CLOUD;
	if (bookmarks.item_under_mouse == ITEM_UNDER_MOUSE_NONE && changes_since_current_branch && mouse_x >= BranchCurrX[TOTAL_BOOKMARKS] - DIGIT_RECT_HALFWIDTH_COLLISION && mouse_x < BranchCurrX[TOTAL_BOOKMARKS] - DIGIT_RECT_HALFWIDTH_COLLISION + DIGIT_RECT_WIDTH_COLLISION && mouse_y >= BranchCurrY[TOTAL_BOOKMARKS] - DIGIT_RECT_HALFHEIGHT_COLLISION && mouse_y < BranchCurrY[TOTAL_BOOKMARKS] - DIGIT_RECT_HALFHEIGHT_COLLISION + DIGIT_RECT_HEIGHT_COLLISION)
		bookmarks.item_under_mouse = TOTAL_BOOKMARKS;
	if (prev_item_under_mouse != bookmarks.item_under_mouse)
		must_redraw_branches_tree = true;
}

void BRANCHES::SetCurrentPosTime()
{
	time_t raw_time;
	time(&raw_time);
	struct tm * timeinfo = localtime(&raw_time);
	strftime(current_pos_time, TIME_DESC_LENGTH, "%H:%M:%S", timeinfo);
}

void BRANCHES::RecalculateBranchesTree()
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
		if (!bookmarks.bookmarks_array[UndistributedBranches[i]].not_empty)
			UndistributedBranches.erase(UndistributedBranches.begin() + i);
	}
	// highest level: cloud (id = -1)
	BranchesLevels.resize(current_grid_x+1);
	BranchesLevels[current_grid_x].resize(1);
	BranchesLevels[current_grid_x][0] = ITEM_UNDER_MOUSE_CLOUD;
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
				if (parents[UndistributedBranches[i]] == current_parent)
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
					RecursiveAddHeight(parents[current_parent], GridHeight[current_parent] - 1);
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
			if (Children[current_branch+1].size() < MAX_NUM_CHILDREN_ON_CANVAS_HEIGHT)
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
				// special case 0: if there's too many children on one level (more than canvas can show)
				// then "current_pos" becomes special branch above current_branch
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
	RecursiveSetYPos(ITEM_UNDER_MOUSE_CLOUD, 0);
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
				parent = parents[t];
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
				parent = parents[t];
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
	// special case 4: if cloud has all 10 children, then one child will be out of canvas
	if (Children[0].size() == TOTAL_BOOKMARKS)
	{
		// find this child and move it to be visible
		for (int t = TOTAL_BOOKMARKS - 1; t >= 0; t--)
		{
			if (GridY[t] > MAX_GRID_Y_POS)
			{
				GridY[t] = MAX_GRID_Y_POS;
				GridX[t] -= 2;
				// also move fireball to position near this branch
				if (changes_since_current_branch && current_branch == t)
				{
					GridY[TOTAL_BOOKMARKS] = GridY[t];
					GridX[TOTAL_BOOKMARKS] = GridX[t] + 1;
				}
				break;
			} else if (GridY[t] < -MAX_GRID_Y_POS)
			{
				GridY[t] = -MAX_GRID_Y_POS;
				GridX[t] -= 2;
				// also move fireball to position near this branch
				if (changes_since_current_branch && current_branch == t)
				{
					GridY[TOTAL_BOOKMARKS] = GridY[t];
					GridX[TOTAL_BOOKMARKS] = GridX[t] + 1;
				}
				break;
			}
		}
	}

	// 3. Set pixel positions of branches
	int max_x = 0;
	for (int i = TOTAL_BOOKMARKS-1; i >= 0; i--)
	{
		if (bookmarks.bookmarks_array[i].not_empty)
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
		if (bookmarks.bookmarks_array[i].not_empty)
			BranchX[i] += CloudX;
	BranchX[TOTAL_BOOKMARKS] += CloudX;
	// target cursor
	CursorX = BranchX[TOTAL_BOOKMARKS];
	CursorY = BranchY[TOTAL_BOOKMARKS];
	// finished recalculating
	must_recalculate_branches_tree = false;
	must_redraw_branches_tree = true;
}
void BRANCHES::RecursiveAddHeight(int branch_num, int amount)
{
	if (branch_num >= 0)
	{
		GridHeight[branch_num] += amount;
		if (parents[branch_num] >= 0)
			RecursiveAddHeight(parents[branch_num], amount);
	}
}
void BRANCHES::RecursiveSetYPos(int parent, int parentY)
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
LRESULT APIENTRY BranchesBitmapWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	extern BRANCHES branches;
	switch(msg)
	{
		case WM_MOUSEMOVE:
		{
			if (!bookmarks.mouse_over_bitmap)
			{
				bookmarks.mouse_over_bitmap = true;
				bookmarks.tme.hwndTrack = hWnd;
				TrackMouseEvent(&bookmarks.tme);
			}
			bookmarks.MouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			break;
		}
		case WM_MOUSELEAVE:
		{
			bookmarks.mouse_over_bitmap = false;
			bookmarks.MouseMove(-1, -1);
			break;
		}
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			branches.PaintBranchesBitmap(BeginPaint(hWnd, &ps));
			EndPaint(hWnd, &ps);
			return 0;
		}
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONDBLCLK:
		{
			if (GetFocus() != hWnd)
				SetFocus(hWnd);
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
			return SendMessage(piano_roll.hwndList, msg, wParam, lParam);

	}
	return CallWindowProc(hwndBranchesBitmap_oldWndProc, hWnd, msg, wParam, lParam);
}


