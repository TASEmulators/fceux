//Specification file for TASEDIT_LIST class
#define LIST_ID_LEN 5

#define CDDS_SUBITEMPREPAINT       (CDDS_SUBITEM | CDDS_ITEMPREPAINT)
#define CDDS_SUBITEMPOSTPAINT      (CDDS_SUBITEM | CDDS_ITEMPOSTPAINT)
#define CDDS_SUBITEMPREERASE       (CDDS_SUBITEM | CDDS_ITEMPREERASE)
#define CDDS_SUBITEMPOSTERASE      (CDDS_SUBITEM | CDDS_ITEMPOSTERASE)

#define NUM_JOYPADS 4
#define NUM_JOYPAD_BUTTONS 8

#define HEADER_LIGHT_MAX 10
#define HEADER_LIGHT_HOLD 4
#define HEADER_LIGHT_UPDATE_TICK 40	// 25FPS

#define MAX_NUM_COLUMNS 35
#define COLUMN_ICONS 0
#define COLUMN_FRAMENUM	 1
#define COLUMN_JOYPAD1_A 2
#define COLUMN_JOYPAD1_B 3
#define COLUMN_JOYPAD1_S 4
#define COLUMN_JOYPAD1_T 5
#define COLUMN_JOYPAD1_U 6
#define COLUMN_JOYPAD1_D 7
#define COLUMN_JOYPAD1_L 8
#define COLUMN_JOYPAD1_R 9
#define COLUMN_JOYPAD2_A 10
#define COLUMN_JOYPAD2_B 11
#define COLUMN_JOYPAD2_S 12
#define COLUMN_JOYPAD2_T 13
#define COLUMN_JOYPAD2_U 14
#define COLUMN_JOYPAD2_D 15
#define COLUMN_JOYPAD2_L 16
#define COLUMN_JOYPAD2_R 17
#define COLUMN_JOYPAD3_A 18
#define COLUMN_JOYPAD3_B 19
#define COLUMN_JOYPAD3_S 20
#define COLUMN_JOYPAD3_T 21
#define COLUMN_JOYPAD3_U 22
#define COLUMN_JOYPAD3_D 23
#define COLUMN_JOYPAD3_L 24
#define COLUMN_JOYPAD3_R 25
#define COLUMN_JOYPAD4_A 26
#define COLUMN_JOYPAD4_B 27
#define COLUMN_JOYPAD4_S 28
#define COLUMN_JOYPAD4_T 29
#define COLUMN_JOYPAD4_U 30
#define COLUMN_JOYPAD4_D 31
#define COLUMN_JOYPAD4_L 32
#define COLUMN_JOYPAD4_R 33
#define COLUMN_FRAMENUM2 34

#define DIGITS_IN_FRAMENUM 7
#define ARROW_IMAGE_ID 20

// listview colors
#define NORMAL_TEXT_COLOR 0x0

#define NORMAL_FRAMENUM_COLOR 0xFFFFFF
#define NORMAL_INPUT_COLOR1 0xEDEDED
#define NORMAL_INPUT_COLOR2 0xE2E2E2

#define GREENZONE_FRAMENUM_COLOR 0xDDFFDD
#define GREENZONE_INPUT_COLOR1 0xC8F7C4
#define GREENZONE_INPUT_COLOR2 0xADE7AD

#define PALE_GREENZONE_FRAMENUM_COLOR 0xE4FFE4
#define PALE_GREENZONE_INPUT_COLOR1 0xD3F9D2
#define PALE_GREENZONE_INPUT_COLOR2 0xBAEBBA

#define LAG_FRAMENUM_COLOR 0xDDDCFF
#define LAG_INPUT_COLOR1 0xD2D0F0
#define LAG_INPUT_COLOR2 0xC9C6E8

#define PALE_LAG_FRAMENUM_COLOR 0xE3E3FF
#define PALE_LAG_INPUT_COLOR1 0xDADAF4
#define PALE_LAG_INPUT_COLOR2 0xCFCEEA

#define CUR_FRAMENUM_COLOR 0xFCF1CE
#define CUR_INPUT_COLOR1 0xF8EBB6
#define CUR_INPUT_COLOR2 0xE6DDA5

#define UNDOHINT_FRAMENUM_COLOR 0xF9DDE6
#define UNDOHINT_INPUT_COLOR1 0xF7D2E1
#define UNDOHINT_INPUT_COLOR2 0xE9BED1

#define MARKED_FRAMENUM_COLOR 0xC0FCFF
#define CUR_MARKED_FRAMENUM_COLOR 0xDEF7F3
#define MARKED_UNDOHINT_FRAMENUM_COLOR 0xE1E7EC

class TASEDIT_LIST
{
public:
	TASEDIT_LIST();
	void init();
	void free();
	void reset();
	void update();

	void save(EMUFILE *os, bool really_save = true);
	bool load(EMUFILE *is);

	void AddFourscore();
	void RemoveFourscore();

	void RedrawList();
	void RedrawRow(int index);
	void RedrawHeader();

	bool CheckItemVisible(int frame);

	void FollowPlayback();
	void FollowPlaybackIfNeeded();
	void FollowUndo();
	void FollowSelection();
	void FollowPauseframe();

	void SetHeaderColumnLight(int column, int level);

	void GetDispInfo(NMLVDISPINFO* nmlvDispInfo);
	LONG CustomDraw(NMLVCUSTOMDRAW* msg);
	LONG HeaderCustomDraw(NMLVCUSTOMDRAW* msg);

	HWND hwndList, hwndHeader;

	// GDI stuff
	HIMAGELIST himglist;
	HFONT hMainListFont, hMainListSelectFont, hMarkersFont, hMarkersEditFont;

private:
	std::vector<uint8> header_colors;
	int num_columns;
	int next_header_update_time;

};
