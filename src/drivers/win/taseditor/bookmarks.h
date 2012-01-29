//Specification file for Bookmarks class
#include "bookmark.h"

#define TOTAL_BOOKMARKS 10

enum
{
	EDIT_MODE_BOOKMARKS = 0,
	EDIT_MODE_BOTH = 1,
	EDIT_MODE_BRANCHES = 2,
};

enum COMMANDS
{
	COMMAND_SET = 0,
	COMMAND_JUMP = 1,
	COMMAND_DEPLOY = 2,
	COMMAND_DELETE = 3,			// not implemented, probably useless
};

#define ITEM_UNDER_MOUSE_NONE -2
#define ITEM_UNDER_MOUSE_CLOUD -1

#define BOOKMARKS_FLASH_TICK 100		// in milliseconds
#define BRANCHES_ANIMATION_TICK 50		// animate at 20FPS
#define BRANCHES_TRANSITION_MAX 8

// branches bitmap
#define BRANCHES_BITMAP_WIDTH 170
#define BRANCHES_BITMAP_HEIGHT 145
#define BRANCHES_ANIMATION_FRAMES 12
#define BRANCHES_BITMAP_FRAMENUM_X 0
#define BRANCHES_BITMAP_FRAMENUM_Y 0
#define BRANCHES_BITMAP_TIME_X 0
#define BRANCHES_BITMAP_TIME_Y BRANCHES_BITMAP_HEIGHT - 17
// constants for drawing branches tree
#define BRANCHES_CANVAS_WIDTH 146
#define BRANCHES_CANVAS_HEIGHT 130
#define BRANCHES_CLOUD_X 14
#define BRANCHES_CLOUD_Y 72
#define BRANCHES_GRID_MIN_WIDTH 14
#define BRANCHES_GRID_MAX_WIDTH 30
#define MIN_CLOUD_LINE_LENGTH 19
#define BRANCHES_GRID_MIN_HALFHEIGHT 8
#define BRANCHES_GRID_MAX_HALFHEIGHT 12
#define EMPTY_BRANCHES_X -6
#define EMPTY_BRANCHES_Y_BASE 8
#define EMPTY_BRANCHES_Y_FACTOR 14
#define MAX_NUM_CHILDREN_ON_CANVAS_HEIGHT 9
#define MAX_CHAIN_LEN 10
#define MAX_GRID_Y_POS 8
// spritesheet
#define DIGIT_BITMAP_WIDTH 9
#define DIGIT_BITMAP_HALFWIDTH DIGIT_BITMAP_WIDTH/2
#define DIGIT_BITMAP_HEIGHT 13
#define DIGIT_BITMAP_HALFHEIGHT DIGIT_BITMAP_HEIGHT/2
#define BLUE_DIGITS_SPRITESHEET_DX DIGIT_BITMAP_WIDTH*TOTAL_BOOKMARKS
#define MOUSEOVER_DIGITS_SPRITESHEET_DY DIGIT_BITMAP_HEIGHT
#define DIGIT_RECT_WIDTH 11
#define DIGIT_RECT_HALFWIDTH DIGIT_RECT_WIDTH/2
#define DIGIT_RECT_HEIGHT 15
#define DIGIT_RECT_HALFHEIGHT DIGIT_RECT_HEIGHT/2
#define BRANCHES_CLOUD_WIDTH 26
#define BRANCHES_CLOUD_HALFWIDTH BRANCHES_CLOUD_WIDTH/2
#define BRANCHES_CLOUD_HEIGHT 15
#define BRANCHES_CLOUD_HALFHEIGHT BRANCHES_CLOUD_HEIGHT/2
#define BRANCHES_CLOUD_SPRITESHEET_X 180
#define BRANCHES_CLOUD_SPRITESHEET_Y 0
#define BRANCHES_FIREBALL_WIDTH 10
#define BRANCHES_FIREBALL_HALFWIDTH BRANCHES_FIREBALL_WIDTH/2
#define BRANCHES_FIREBALL_HEIGHT 10
#define BRANCHES_FIREBALL_HALFHEIGHT BRANCHES_FIREBALL_HEIGHT/2
#define BRANCHES_FIREBALL_SPRITESHEET_X 0
#define BRANCHES_FIREBALL_SPRITESHEET_Y 26
#define BRANCHES_FIREBALL_MAX_SIZE 5
#define BRANCHES_FIREBALL_SPRITESHEET_END_X 160
#define BRANCHES_CORNER_WIDTH 7
#define BRANCHES_CORNER_HALFWIDTH BRANCHES_CORNER_WIDTH/2
#define BRANCHES_CORNER_HEIGHT 7
#define BRANCHES_CORNER_HALFHEIGHT BRANCHES_CORNER_HEIGHT/2
#define BRANCHES_CORNER1_SPRITESHEET_X 206
#define BRANCHES_CORNER1_SPRITESHEET_Y 0
#define BRANCHES_CORNER2_SPRITESHEET_X 213
#define BRANCHES_CORNER2_SPRITESHEET_Y 0
#define BRANCHES_CORNER3_SPRITESHEET_X 206
#define BRANCHES_CORNER3_SPRITESHEET_Y 7
#define BRANCHES_CORNER4_SPRITESHEET_X 213
#define BRANCHES_CORNER4_SPRITESHEET_Y 7
#define BRANCHES_CORNER_BASE_SHIFT 6
// listview columns
enum
{
	BOOKMARKS_COLUMN_ICON = 0,
	BOOKMARKS_COLUMN_FRAME = 1,
	BOOKMARKS_COLUMN_TIME = 2,
};

#define BOOKMARKS_ID_LEN 10
#define TIME_DESC_LENGTH 9		// "HH:MM:SS"

#define DEFAULT_BOOKMARK 1

class BOOKMARKS
{
public:
	BOOKMARKS();
	void init();
	void free();
	void reset();
	void reset_vars();
	void update();

	void save(EMUFILE *os, bool really_save = true);
	bool load(EMUFILE *is);

	void command(int command_id, int slot);

	void GetDispInfo(NMLVDISPINFO* nmlvDispInfo);
	LONG CustomDraw(NMLVCUSTOMDRAW* msg);
	void LeftClick(LPNMITEMACTIVATE info);
	void RightClick(LPNMITEMACTIVATE info);

	int FindBookmarkAtFrame(int frame);
	int GetCurrentBranch();

	void RedrawBookmarksCaption();
	void RedrawBookmarksList();
	void RedrawChangedBookmarks(int frame);
	void RedrawBookmarksRow(int index);

	void RedrawBranchesTree();
	void PaintBranchesBitmap(HDC hdc);

	void MouseMove(int new_x, int new_y);
	void CheckMousePos();

	void ChangesMadeSinceBranch();

	void RecalculateBranchesTree();
	void RecursiveAddHeight(int branch_num, int amount);
	void RecursiveSetYPos(int parent, int parentY);

	// saved vars
	std::vector<BOOKMARK> bookmarks_array;

	// not saved vars
	int branch_row_top;
	int branch_row_left;
	int branch_row_height;
	bool mouse_over_bitmap, mouse_over_bookmarkslist;
	int item_under_mouse;
	TRACKMOUSEEVENT tme, list_tme;

private:
	void set(int slot);
	void jump(int slot);
	void deploy(int slot);

	void SetCurrentPosTime();

	// also saved vars
	int current_branch;
	bool changes_since_current_branch;
	char cloud_time[TIME_DESC_LENGTH];
	char current_pos_time[TIME_DESC_LENGTH];

	// not saved vars
	std::vector<int> commands;
	int check_flash_shedule;
	int edit_mode;
	int animation_frame;
	int next_animation_time;
	bool must_check_item_under_mouse;
	bool must_redraw_branches_tree;
	bool must_recalculate_branches_tree;
	std::vector<int> BranchX;				// in pixels
	std::vector<int> BranchY;
	std::vector<int> BranchPrevX;
	std::vector<int> BranchPrevY;
	std::vector<int> BranchCurrX;
	std::vector<int> BranchCurrY;
	int CloudX, CloudPrevX, cloud_x;
	int CursorX, CursorPrevX, CursorY, CursorPrevY;
	int transition_phase;
	int fireball_size;
	int mouse_x, mouse_y;

	HWND hwndBookmarksList, hwndBookmarks;
	HWND hwndBranchesBitmap;

	// GDI stuff
	HFONT hBookmarksFont;
	HBRUSH normal_brush;
	RECT temp_rect;
	HPEN normal_pen, select_pen;
	HBITMAP branches_hbitmap, hOldBitmap, buffer_hbitmap, hOldBitmap1, branchesSpritesheet, hOldBitmap2;
	HDC hBitmapDC, hBufferDC, hSpritesheetDC;

	// temps
	std::vector<int> GridX;				// in grid units
	std::vector<int> GridY;
	std::vector<int> GridHeight;
	std::vector<std::vector<uint8>> Children;

};
