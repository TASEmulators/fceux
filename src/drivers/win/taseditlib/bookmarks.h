//Specification file for Bookmarks class

#include "bookmark.h"

#define TOTAL_BOOKMARKS 10

#define BOOKMARKS_FLASH_TICK 50		// in milliseconds

// listview columns
#define BOOKMARKS_COLUMN_ICON 0
#define BOOKMARKS_COLUMN_FRAME 1
#define BOOKMARKS_COLUMN_TIME 2

#define BOOKMARKS_ID_LEN 10

class BOOKMARKS
{
public:
	BOOKMARKS();
	void init();
	void free();
	void update();

	void save(EMUFILE *os);
	bool load(EMUFILE *is);

	void set(int slot);
	void jump(int slot);
	void unleash(int slot);

	void GetDispInfo(NMLVDISPINFO* nmlvDispInfo);
	LONG CustomDraw(NMLVCUSTOMDRAW* msg);
	void LeftClick(LPNMITEMACTIVATE info);
	void RightClick(LPNMITEMACTIVATE info);

	int FindBookmarkAtFrame(int frame);

	void RedrawBookmarksCaption();
	void RedrawBookmarksList();
	void RedrawChangedBookmarks(int frame);
	void RedrawBookmarksRow(int index);

	std::vector<BOOKMARK> bookmarks_array;

private:
	HFONT hBookmarksFont;
	int check_flash_shedule;

};
