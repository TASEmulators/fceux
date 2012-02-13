//Specification file for TASEDITOR_LIST class
#define LIST_ID_LEN 5

#define CDDS_SUBITEMPREPAINT       (CDDS_SUBITEM | CDDS_ITEMPREPAINT)
#define CDDS_SUBITEMPOSTPAINT      (CDDS_SUBITEM | CDDS_ITEMPOSTPAINT)
#define CDDS_SUBITEMPREERASE       (CDDS_SUBITEM | CDDS_ITEMPREERASE)
#define CDDS_SUBITEMPOSTERASE      (CDDS_SUBITEM | CDDS_ITEMPOSTERASE)

#define MAX_NUM_JOYPADS 4			// = max(joysticks_per_frame[])
#define NUM_JOYPAD_BUTTONS 8

#define HEADER_LIGHT_MAX 10
#define HEADER_LIGHT_HOLD 4
#define HEADER_LIGHT_UPDATE_TICK 40	// 25FPS

enum
{
	COLUMN_ICONS,
	COLUMN_FRAMENUM,
	COLUMN_JOYPAD1_A,
	COLUMN_JOYPAD1_B,
	COLUMN_JOYPAD1_S,
	COLUMN_JOYPAD1_T,
	COLUMN_JOYPAD1_U,
	COLUMN_JOYPAD1_D,
	COLUMN_JOYPAD1_L,
	COLUMN_JOYPAD1_R,
	COLUMN_JOYPAD2_A,
	COLUMN_JOYPAD2_B,
	COLUMN_JOYPAD2_S,
	COLUMN_JOYPAD2_T,
	COLUMN_JOYPAD2_U,
	COLUMN_JOYPAD2_D,
	COLUMN_JOYPAD2_L,
	COLUMN_JOYPAD2_R,
	COLUMN_JOYPAD3_A,
	COLUMN_JOYPAD3_B,
	COLUMN_JOYPAD3_S,
	COLUMN_JOYPAD3_T,
	COLUMN_JOYPAD3_U,
	COLUMN_JOYPAD3_D,
	COLUMN_JOYPAD3_L,
	COLUMN_JOYPAD3_R,
	COLUMN_JOYPAD4_A,
	COLUMN_JOYPAD4_B,
	COLUMN_JOYPAD4_S,
	COLUMN_JOYPAD4_T,
	COLUMN_JOYPAD4_U,
	COLUMN_JOYPAD4_D,
	COLUMN_JOYPAD4_L,
	COLUMN_JOYPAD4_R,
	COLUMN_FRAMENUM2,

	TOTAL_COLUMNS
};

// when there's too many button columns, there's need for 2nd Frame# column at the end
#define NUM_COLUMNS_NEED_2ND_FRAMENUM COLUMN_JOYPAD4_R

#define DIGITS_IN_FRAMENUM 7
#define ARROW_IMAGE_ID 20
#define GREEN_ARROW_IMAGE_ID 21

#define COLUMN_ICONS_WIDTH 13
#define COLUMN_FRAMENUM_WIDTH 75
#define COLUMN_BUTTON_WIDTH 21

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

#define MARKED_FRAMENUM_COLOR 0xAEF0FF
#define CUR_MARKED_FRAMENUM_COLOR 0xCAEDEA
#define MARKED_UNDOHINT_FRAMENUM_COLOR 0xDDE5E9

#define BINDMARKED_FRAMENUM_COLOR 0xC9FFF7
#define CUR_BINDMARKED_FRAMENUM_COLOR 0xD5F2EC
#define BINDMARKED_UNDOHINT_FRAMENUM_COLOR 0xE1EBED

#define PLAYBACK_MARKER_COLOR 0xC9AF00

class TASEDITOR_LIST
{
public:
	TASEDITOR_LIST();
	void init();
	void free();
	void reset();
	void update();

	void save(EMUFILE *os, bool really_save = true);
	bool load(EMUFILE *is);

	void RedrawList();
	void RedrawRow(int index);
	void RedrawHeader();

	bool CheckItemVisible(int frame);

	void FollowPlayback();
	void FollowPlaybackIfNeeded();
	void FollowUndo();
	void FollowSelection();
	void FollowPauseframe();
	void FollowMarker(int marker_id);

	void SetHeaderColumnLight(int column, int level);

	void GetDispInfo(NMLVDISPINFO* nmlvDispInfo);
	LONG CustomDraw(NMLVCUSTOMDRAW* msg);
	LONG HeaderCustomDraw(NMLVCUSTOMDRAW* msg);

	void SingleClick(LPNMITEMACTIVATE info);
	void DoubleClick(LPNMITEMACTIVATE info);

	void ToggleJoypadBit(int column_index, int row_index, UINT KeyFlags);
	void ColumnSet(int column);
	void InputColumnSet(int column);
	void FrameColumnSet();


	HWND hwndList, hwndHeader;

	// GDI stuff
	HIMAGELIST himglist;
	HFONT hMainListFont, hMainListSelectFont, hMarkersFont, hMarkersEditFont;
	HBRUSH bg_brush;

private:
	void CenterListAt(int frame);

	std::vector<uint8> header_colors;
	int num_columns;
	int next_header_update_time;

};
