// Specification file for Branches class
#pragma once
#include <stdint.h>
#include <time.h>
#include <vector>

#include <QFont>
#include <QTimer>
#include <QWidget>

#define BRANCHES_ANIMATION_TICK (CLOCKS_PER_SEC / 25)	// animate at 25FPS
#define BRANCHES_TRANSITION_MAX 12
#define CURSOR_MIN_DISTANCE 1.0
#define CURSOR_MAX_DISTANCE 256.0
#define CURSOR_MIN_SPEED 1.0

// floating empty branches
#define MAX_FLOATING_PHASE 4

// branches bitmap
#define BRANCHES_BITMAP_WIDTH 170
#define BRANCHES_BITMAP_HEIGHT 145
#define BRANCHES_ANIMATION_FRAMES 12
#define BRANCHES_BITMAP_FRAMENUM_X 2
#define BRANCHES_BITMAP_FRAMENUM_Y 1
#define BRANCHES_BITMAP_TIME_X 2
#define BRANCHES_BITMAP_TIME_Y BRANCHES_BITMAP_HEIGHT - 17
#define BRANCHES_TEXT_SHADOW_COLOR 0xFF, 0xFF, 0xFF
#define BRANCHES_TEXT_COLOR 0x00, 0x00, 0x7F
// constants for drawing branches tree
#define BRANCHES_CANVAS_WIDTH 140
#define BRANCHES_CANVAS_HEIGHT 130
#define BRANCHES_CLOUD_X 14
#define BRANCHES_CLOUD_Y 72
#define BRANCHES_GRID_MIN_WIDTH 14
//#define BRANCHES_GRID_MAX_WIDTH 30
#define BRANCHES_GRID_MAX_WIDTH 40
#define MIN_CLOUD_LINE_LENGTH 19
#define MIN_CLOUD_X 12
#define BASE_HORIZONTAL_SHIFT 10
#define BRANCHES_GRID_MIN_HALFHEIGHT 8
#define BRANCHES_GRID_MAX_HALFHEIGHT 32
//#define BRANCHES_GRID_MAX_HALFHEIGHT 12
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
#define BRANCHES_CORNER_BASE_SHIFT 3
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

class BRANCHES : public QWidget
{
	Q_OBJECT

public:
	BRANCHES(QWidget *parent = 0);
	~BRANCHES(void);
	void init();
	void free();
	void reset();
	void resetVars();
	void update();

	void setFont( QFont &font );
	void save(EMUFILE *os);
	bool load(EMUFILE *is);

	int getParentOf(int child);
	int getCurrentBranch();
	bool areThereChangesSinceCurrentBranch();

	bool isSafeToShowBranchesData();

	void redrawBranchesBitmap();
	//void paintBranchesBitmap(HDC hdc);

	void handleBookmarkSet(int slot);
	void handleBookmarkDeploy(int slot);
	void handleHistoryJump(int newCurrentBranch, bool newChangesSinceCurrentBranch);
	void setChangesMadeSinceBranch();

	void invalidateRelationsOfBranchSlot(int slot);
	int findFullTimelineForBranch(int branchNumber);

	int findItemUnderMouse(int mouseX, int mouseY);

	// not saved vars
	bool mustRedrawBranchesBitmap;
	bool mustRecalculateBranchesTree;
	int branchRightclicked;

protected:
	void paintEvent(QPaintEvent *event);
	void resizeEvent(QResizeEvent *event);
	void mousePressEvent(QMouseEvent * event);
	void mouseReleaseEvent(QMouseEvent * event);
	void mouseMoveEvent(QMouseEvent * event);
	void mouseDoubleClickEvent(QMouseEvent * event);
	void focusOutEvent(QFocusEvent *event);
	void leaveEvent(QEvent *event);
	bool event(QEvent *event);

private:
	void calcFontData(void);
	void setCurrentPosTimestamp();

	void recalculateParents();
	void recalculateBranchesTree();
	void recursiveAddHeight(int branchNumber, int amount);
	void recursiveSetYPos(int parent, int parentY);

	int getFirstDifferenceBetween(int firstBranch, int secondBranch);

	// saved vars
	std::vector<int> parents;
	int currentBranch;
	bool changesSinceCurrentBranch;
	char cloudTimestamp[TIMESTAMP_LENGTH];
	char currentPosTimestamp[TIMESTAMP_LENGTH];
	std::vector<std::vector<int>> cachedFirstDifferences;
	std::vector<int8_t> cachedTimelines;		// stores id of the last branch on the timeline of every Branch. Sometimes it's the id of the Branch itself, but sometimes it's an id of its child/grandchild that shares the same Input

	// not saved vars
	int transitionPhase;
	int currentAnimationFrame;
	clock_t nextAnimationTime;
	int playbackCursorX, playbackCursorY;
	double cornersCursorX, cornersCursorY;
	std::vector<int> branchX;				// in pixels
	std::vector<int> branchY;
	std::vector<int> branchPreviousX;
	std::vector<int> branchPreviousY;
	std::vector<int> branchCurrentX;
	std::vector<int> branchCurrentY;
	int cloudX, cloudPreviousX, cloudCurrentX;
	int fireballSize;
	int lastItemUnderMouse;

	QFont font;
	QPoint  imagePos;
	QTimer *imageTimer;
	QRect   viewRect;

	int  imageItem;
	int  viewWidth;
	int  viewHeight;
	int  pxCharWidth;
	int  pxCharHeight;
	int  pxLineSpacing;
	int  pxBoxWidth;
	int  pxBoxHeight;
	int  pxSelWidth;
	int  pxSelHeight;
	int  pxMinGridWidth;
	int  pxMaxGridWidth;

	// temps
	std::vector<int> gridX;				// measured in grid units, not in pixels
	std::vector<int> gridY;
	std::vector<int> gridHeight;
	std::vector<std::vector<uint8_t>> children;

	private slots:
		void showImage(void);

	signals:
		void imageIndexChanged(int);
};
