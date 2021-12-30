// Specification file for Bookmarks class
#pragma once
#include <vector>
#include <time.h>

#include <QFont>
#include <QTimer>
#include <QWidget>
#include <QScrollBar>

#include "fceu.h"
#include "Qt/TasEditor/bookmark.h"

#define TOTAL_BOOKMARKS 10

enum BOOKMARKS_EDIT_MODES
{
	EDIT_MODE_BOOKMARKS = 0,
	EDIT_MODE_BOTH = 1,
	EDIT_MODE_BRANCHES = 2,
};

enum BOOKMARK_COMMANDS
{
	COMMAND_SET = 0,
	COMMAND_JUMP = 1,
	COMMAND_DEPLOY = 2,
	COMMAND_SELECT = 3,
	COMMAND_DELETE = 4,			// not implemented, probably useless

	// ...
	TOTAL_BOOKMARK_COMMANDS
};

#define BOOKMARKSLIST_COLUMN_ICONS_WIDTH 15
#define BOOKMARKSLIST_COLUMN_FRAMENUM_WIDTH 74
#define BOOKMARKSLIST_COLUMN_TIMESTAMP_WIDTH 80

#define BOOKMARKS_BITMAPS_SELECTED 20

#define ITEM_UNDER_MOUSE_NONE (-2)
#define ITEM_UNDER_MOUSE_CLOUD (-1)
#define ITEM_UNDER_MOUSE_FIREBALL (TOTAL_BOOKMARKS)

#define BOOKMARKS_FLASH_TICK (CLOCKS_PER_SEC / 10) // 10 Hz

// listview columns
enum
{
	BOOKMARKSLIST_COLUMN_ICON = 0,
	BOOKMARKSLIST_COLUMN_FRAME = 1,
	BOOKMARKSLIST_COLUMN_TIME = 2,
};

#define BOOKMARKS_ID_LEN 10
#define TIMESTAMP_LENGTH 9		// "HH:MM:SS"

#define DEFAULT_SLOT 1

class BOOKMARKS : public QWidget
{
	Q_OBJECT

public:
	BOOKMARKS(QWidget *parent = 0);
	~BOOKMARKS(void);
	void init();
	void free();
	void reset();
	void reset_vars();
	void update();

	void setFont( QFont &font );
	void save(EMUFILE *os, bool really_save = true);
	bool load(EMUFILE *is, unsigned int offset);

	void command(int command_id, int slot = -1);

	void handleLeftClick();
	void handleRightClick();

	int findBookmarkAtFrame(int frame);

	void redrawBookmarksSectionCaption();
	void redrawBookmarksList(bool eraseBG = false);
	void redrawChangedBookmarks(int frame);
	void redrawBookmark(int bookmarkNumber);
	void redrawBookmarksListRow(int rowIndex);

	void handleMouseMove(int newX, int newY);
	int findItemUnderMouse();

	int getSelectedSlot();

	// saved vars
	std::vector<BOOKMARK> bookmarksArray;

	// not saved vars
	int editMode;
	bool mustCheckItemUnderMouse;
	bool mouseOverBranchesBitmap, mouseOverBookmarksList;
	int itemUnderMouse;
	int bookmarkLeftclicked, bookmarkRightclicked, columnClicked;
	int listTopMargin;
	int listRowLeft;
	int listRowHeight;

protected:
	void resizeEvent(QResizeEvent *event);
	void paintEvent(QPaintEvent *event);
	void mousePressEvent(QMouseEvent * event);
	void mouseReleaseEvent(QMouseEvent * event);
	void mouseMoveEvent(QMouseEvent * event);
	void focusOutEvent(QFocusEvent *event);
	void leaveEvent(QEvent *event);
	bool event(QEvent *event);

	int    calcColumn( int px );
	QPoint convPixToCursor( QPoint p );

private:
	void set(int slot);
	void jump(int slot);
	void deploy(int slot);
	void calcFontData(void);

	// not saved vars
	std::vector<int> commands;
	int selectedSlot;
	int mouseX, mouseY;
	clock_t nextFlashUpdateTime;

	// GUI stuff
	QFont       font;
	QScrollBar *hbar;
	QScrollBar *vbar;
	QTimer     *imageTimer;

	QPoint      imagePos;

	int imageItem;
	int viewWidth;
	int viewHeight;
	int viewLines;

	int pxCharWidth;
	int pxCharHeight;
	int pxCursorHeight;
	int pxLineXScroll;
	int pxLineWidth;
	int pxLineSpacing;
	int pxLineLead;
	int pxLineTextOfs;
	int pxWidthCol1;
	int pxWidthFrameCol;
	int pxWidthTimeCol;
	int pxStartCol1;
	int pxStartCol2;
	int pxStartCol3;

	bool redrawFlag;

	private slots:
		void showImage(void);

	signals:
		void imageIndexChanged(int);
};
