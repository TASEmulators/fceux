// Specification file for Bookmarks class

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

	TOTAL_COMMANDS
};

#define ITEM_UNDER_MOUSE_NONE -2
#define ITEM_UNDER_MOUSE_CLOUD -1

#define BOOKMARKS_FLASH_TICK 100		// in milliseconds

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

	void command(int command_id, int slot = -1);

	void GetDispInfo(NMLVDISPINFO* nmlvDispInfo);
	LONG CustomDraw(NMLVCUSTOMDRAW* msg);
	void LeftClick(int column_index, int row_index);
	void RightClick(int column_index, int row_index);

	int FindBookmarkAtFrame(int frame);

	void RedrawBookmarksCaption();
	void RedrawBookmarksList();
	void RedrawChangedBookmarks(int frame);
	void RedrawBookmarksRow(int index);

	void MouseMove(int new_x, int new_y);
	void FindItemUnderMouse();

	// saved vars
	std::vector<BOOKMARK> bookmarks_array;

	// not saved vars
	int edit_mode;
	bool must_check_item_under_mouse;
	bool mouse_over_bitmap, mouse_over_bookmarkslist;
	int item_under_mouse;
	TRACKMOUSEEVENT tme, list_tme;
	HWND hwndBookmarksList, hwndBranchesBitmap, hwndBookmarks;

private:
	void set(int slot);
	void jump(int slot);
	void deploy(int slot);

	// not saved vars
	std::vector<int> commands;
	int check_flash_shedule;
	int mouse_x, mouse_y;
	int list_row_top;
	int list_row_left;
	int list_row_height;

	// GDI stuff
	HFONT hBookmarksFont;

};
