// Specification file for Branches class

#define BRANCHES_ANIMATION_TICK 40		// animate at 25FPS
#define BRANCHES_TRANSITION_MAX 12
#define CURSOR_MIN_DISTANCE 1.0
#define CURSOR_MAX_DISTANCE 256.0
#define CURSOR_MIN_SPEED 1.0

// floating "empty" branches
#define MAX_FLOATING_PHASE 4

// branches bitmap
#define BRANCHES_BITMAP_WIDTH 170
#define BRANCHES_BITMAP_HEIGHT 145
#define BRANCHES_ANIMATION_FRAMES 12
#define BRANCHES_BITMAP_FRAMENUM_X 2
#define BRANCHES_BITMAP_FRAMENUM_Y 1
#define BRANCHES_BITMAP_TIME_X 2
#define BRANCHES_BITMAP_TIME_Y BRANCHES_BITMAP_HEIGHT - 17
#define BRANCHES_TEXT_SHADOW_COLOR 0xFFFFFF
#define BRANCHES_TEXT_COLOR 0x7F0000
// constants for drawing branches tree
#define BRANCHES_CANVAS_WIDTH 140
#define BRANCHES_CANVAS_HEIGHT 130
#define BRANCHES_CLOUD_X 14
#define BRANCHES_CLOUD_Y 72
#define BRANCHES_GRID_MIN_WIDTH 14
#define BRANCHES_GRID_MAX_WIDTH 30
#define MIN_CLOUD_LINE_LENGTH 19
#define MIN_CLOUD_X 12
#define BASE_HORIZONTAL_SHIFT 10
#define BRANCHES_GRID_MIN_HALFHEIGHT 8
#define BRANCHES_GRID_MAX_HALFHEIGHT 12
#define EMPTY_BRANCHES_X_BASE 4
#define EMPTY_BRANCHES_Y_BASE 9
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
#define DIGIT_RECT_WIDTH_COLLISION (DIGIT_RECT_WIDTH + 2)
#define DIGIT_RECT_HALFWIDTH DIGIT_RECT_WIDTH/2
#define DIGIT_RECT_HALFWIDTH_COLLISION (DIGIT_RECT_HALFWIDTH + 1)
#define DIGIT_RECT_HEIGHT 15
#define DIGIT_RECT_HEIGHT_COLLISION (DIGIT_RECT_HEIGHT + 2)
#define DIGIT_RECT_HALFHEIGHT DIGIT_RECT_HEIGHT/2
#define DIGIT_RECT_HALFHEIGHT_COLLISION (DIGIT_RECT_HALFHEIGHT + 1)
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
#define BRANCHES_CORNER_WIDTH 6
#define BRANCHES_CORNER_HALFWIDTH BRANCHES_CORNER_WIDTH/2
#define BRANCHES_CORNER_HEIGHT 6
#define BRANCHES_CORNER_HALFHEIGHT BRANCHES_CORNER_HEIGHT/2
#define BRANCHES_CORNER1_SPRITESHEET_X 206
#define BRANCHES_CORNER1_SPRITESHEET_Y 0
#define BRANCHES_CORNER2_SPRITESHEET_X 212
#define BRANCHES_CORNER2_SPRITESHEET_Y 0
#define BRANCHES_CORNER3_SPRITESHEET_X 206
#define BRANCHES_CORNER3_SPRITESHEET_Y 6
#define BRANCHES_CORNER4_SPRITESHEET_X 212
#define BRANCHES_CORNER4_SPRITESHEET_Y 6
#define BRANCHES_CORNER_BASE_SHIFT 7
#define BRANCHES_MINIARROW_SPRITESHEET_X 180
#define BRANCHES_MINIARROW_SPRITESHEET_Y 15
#define BRANCHES_MINIARROW_WIDTH 3
#define BRANCHES_MINIARROW_HALFWIDTH BRANCHES_MINIARROW_WIDTH/2
#define BRANCHES_MINIARROW_HEIGHT 5
#define BRANCHES_MINIARROW_HALFHEIGHT BRANCHES_MINIARROW_HEIGHT/2

#define BRANCHES_SELECTED_SLOT_DX -6
#define BRANCHES_SELECTED_SLOT_DY -6
#define BRANCHES_SELECTED_SLOT_WIDTH 13
#define BRANCHES_SELECTED_SLOT_HEIGHT 13

#define FIRST_DIFFERENCE_UNKNOWN -2

class BRANCHES
{
public:
	BRANCHES();
	void init();
	void free();
	void reset();
	void reset_vars();
	void update();

	void save(EMUFILE *os);
	bool load(EMUFILE *is);

	int GetParentOf(int child);
	int GetCurrentBranch();
	bool GetChangesSinceCurrentBranch();

	bool IsSafeToShowBranchesData();

	void RedrawBranchesTree();
	void PaintBranchesBitmap(HDC hdc);

	void HandleBookmarkSet(int slot);
	void HandleBookmarkDeploy(int slot);
	void HandleHistoryJump(int new_current_branch, bool new_changes_since_current_branch);

	void InvalidateBranchSlot(int slot);
	int GetFirstDifference(int first_branch, int second_branch);
	int FindFullTimelineForBranch(int branch_num);
	void ChangesMadeSinceBranch();

	int FindItemUnderMouse(int mouse_x, int mouse_y);

	// not saved vars
	bool must_redraw_branches_tree;
	bool must_recalculate_branches_tree;
	int branch_rightclicked;

private:
	void SetCurrentPosTime();

	void RecalculateParents();
	void RecalculateBranchesTree();
	void RecursiveAddHeight(int branch_num, int amount);
	void RecursiveSetYPos(int parent, int parentY);

	// saved vars
	std::vector<int> parents;
	int current_branch;
	bool changes_since_current_branch;
	char cloud_time[TIME_DESC_LENGTH];
	char current_pos_time[TIME_DESC_LENGTH];
	std::vector<std::vector<int>> cached_first_difference;
	std::vector<int8> cached_timelines;		// stores id of the last branch on the timeline of every Branch. Sometimes it's the id of the Branch itself, but sometimes it's an id of its child/grandchild that shares the same Input

	// not saved vars
	int transition_phase;
	int animation_frame;
	int next_animation_time;
	int playback_x, playback_y;
	double cursor_x, cursor_y;
	std::vector<int> BranchX;				// in pixels
	std::vector<int> BranchY;
	std::vector<int> BranchPrevX;
	std::vector<int> BranchPrevY;
	std::vector<int> BranchCurrX;
	std::vector<int> BranchCurrY;
	int CloudX, CloudPrevX, cloud_x;
	int fireball_size;
	int latest_drawn_item_under_mouse;

	// GDI stuff
	HBRUSH normal_brush, border_brush, selected_slot_brush;
	RECT temp_rect;
	HPEN normal_pen, timeline_pen, select_pen;
	HBITMAP branches_hbitmap, hOldBitmap, buffer_hbitmap, hOldBitmap1, branchesSpritesheet, hOldBitmap2;
	HDC hBitmapDC, hBufferDC, hSpritesheetDC;
	TRIVERTEX vertex[2];
	GRADIENT_RECT gRect;
	RECT branches_bitmap_rect;

	// temps
	std::vector<int> GridX;				// in grid units
	std::vector<int> GridY;
	std::vector<int> GridHeight;
	std::vector<std::vector<uint8>> Children;

};
