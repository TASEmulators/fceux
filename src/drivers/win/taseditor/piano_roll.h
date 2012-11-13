// Specification file for PIANO_ROLL class

#define PIANO_ROLL_ID_LEN 11

#define CDDS_SUBITEMPREPAINT       (CDDS_SUBITEM | CDDS_ITEMPREPAINT)
#define CDDS_SUBITEMPOSTPAINT      (CDDS_SUBITEM | CDDS_ITEMPOSTPAINT)
#define CDDS_SUBITEMPREERASE       (CDDS_SUBITEM | CDDS_ITEMPREERASE)
#define CDDS_SUBITEMPOSTERASE      (CDDS_SUBITEM | CDDS_ITEMPOSTERASE)

#define MAX_NUM_JOYPADS 4			// = max(joysticks_per_frame[])
#define NUM_JOYPAD_BUTTONS 8

#define HEADER_LIGHT_MAX 10
#define HEADER_LIGHT_HOLD 5
#define HEADER_LIGHT_MOUSEOVER_SEL 3
#define HEADER_LIGHT_MOUSEOVER 0
#define HEADER_LIGHT_UPDATE_TICK 40	// 25FPS
#define HEADER_DX_FIX 4

#define PIANO_ROLL_SCROLLING_BOOST 2
#define PLAYBACK_WHEEL_BOOST 2

#define MARKER_DRAG_BOX_ALPHA 180
#define MARKER_DRAG_COUNTDOWN_MAX 14
#define MARKER_DRAG_ALPHA_PER_TICK 13
#define MARKER_DRAG_MAX_SPEED 72
#define MARKER_DRAG_GRAVITY 2

#define DRAG_SCROLLING_BORDER_SIZE 10		// in pixels

#define DOUBLETAP_COUNT 3			// 1:quick press, 2 - quick release, 3 - quick press

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

enum DRAG_MODES
{
	DRAG_MODE_NONE,
	DRAG_MODE_OBSERVE,
	DRAG_MODE_PLAYBACK,
	DRAG_MODE_MARKER,
	DRAG_MODE_SET,
	DRAG_MODE_UNSET,
	DRAG_MODE_SELECTION,
	DRAG_MODE_DESELECTION,
};

// when there's too many button columns, there's need for 2nd Frame# column at the end
#define NUM_COLUMNS_NEED_2ND_FRAMENUM COLUMN_JOYPAD4_R

#define DIGITS_IN_FRAMENUM 7

#define BOOKMARKS_WITH_BLUE_ARROW 20
#define BOOKMARKS_WITH_GREEN_ARROW 40
#define BLUE_ARROW_IMAGE_ID 60
#define GREEN_ARROW_IMAGE_ID 61
#define GREEN_BLUE_ARROW_IMAGE_ID 62

#define COLUMN_ICONS_WIDTH 17
#define COLUMN_FRAMENUM_WIDTH 75
#define COLUMN_BUTTON_WIDTH 21

// listview colors
#define NORMAL_TEXT_COLOR 0x0
#define NORMAL_BACKGROUND_COLOR 0xFFFFFF

#define NORMAL_FRAMENUM_COLOR 0xFFFFFF
#define NORMAL_INPUT_COLOR1 0xE9E9E9
#define NORMAL_INPUT_COLOR2 0xE2E2E2

#define GREENZONE_FRAMENUM_COLOR 0xDDFFDD
#define GREENZONE_INPUT_COLOR1 0xC8F7C4
#define GREENZONE_INPUT_COLOR2 0xADE7AD

#define PALE_GREENZONE_FRAMENUM_COLOR 0xE4FFE4
#define PALE_GREENZONE_INPUT_COLOR1 0xD3F9D2
#define PALE_GREENZONE_INPUT_COLOR2 0xBAEBBA

#define VERY_PALE_GREENZONE_FRAMENUM_COLOR 0xF9FFF9
#define VERY_PALE_GREENZONE_INPUT_COLOR1 0xE0FBE0
#define VERY_PALE_GREENZONE_INPUT_COLOR2 0xD2F2D2

#define LAG_FRAMENUM_COLOR 0xDDDCFF
#define LAG_INPUT_COLOR1 0xD2D0F0
#define LAG_INPUT_COLOR2 0xC9C6E8

#define PALE_LAG_FRAMENUM_COLOR 0xE3E3FF
#define PALE_LAG_INPUT_COLOR1 0xDADAF4
#define PALE_LAG_INPUT_COLOR2 0xCFCEEA

#define VERY_PALE_LAG_FRAMENUM_COLOR 0xE9E9FF 
#define VERY_PALE_LAG_INPUT_COLOR1 0xE5E5F7
#define VERY_PALE_LAG_INPUT_COLOR2 0xE0E0F1

#define CUR_FRAMENUM_COLOR 0xFCEDCF
#define CUR_INPUT_COLOR1 0xF7E7B5
#define CUR_INPUT_COLOR2 0xE5DBA5

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

class PIANO_ROLL
{
public:
	PIANO_ROLL();
	void init();
	void free();
	void reset();
	void update();

	void save(EMUFILE *os, bool really_save = true);
	bool load(EMUFILE *is, unsigned int offset);

	void RedrawList();
	void RedrawRow(int index);
	void RedrawHeader();

	void UpdateItemCount();
	bool CheckItemVisible(int frame);

	void FollowPlayback();
	void FollowPlaybackIfNeeded();
	void FollowUndo();
	void FollowSelection();
	void FollowPauseframe();
	void FollowMarker(int marker_id);
	void EnsureVisible(int row_index);

	void ColumnSet(int column, bool alt_pressed);

	void SetHeaderColumnLight(int column, int level);

	void StartDraggingPlaybackCursor();
	void StartDraggingMarker(int mouse_x, int mouse_y, int row_index, int column_index);
	void StartSelectingDrag(int start_frame);
	void StartDeselectingDrag(int start_frame);

	void GetDispInfo(NMLVDISPINFO* nmlvDispInfo);
	LONG CustomDraw(NMLVCUSTOMDRAW* msg);
	LONG HeaderCustomDraw(NMLVCUSTOMDRAW* msg);

	void RightClick(LVHITTESTINFO& info);

	bool CheckIfTheresAnyIconAtFrame(int frame);
	void CrossGaps(int zDelta);

	int header_item_under_mouse;
	HWND hwndList, hwndHeader;
	TRACKMOUSEEVENT tme;

	int list_row_top, list_row_height, list_header_height;

	bool must_check_item_under_mouse;
	int row_under_mouse, real_row_under_mouse, column_under_mouse;

	unsigned int drag_mode;
	bool rbutton_drag_mode;
	bool can_drag_when_seeking;
	int marker_drag_box_dx, marker_drag_box_dy;
	int marker_drag_box_x, marker_drag_box_y;
	int marker_drag_countdown;
	int marker_drag_framenum;
	int drawing_last_x, drawing_last_y;
	int drawing_start_time;
	int drag_selection_starting_frame;
	int drag_selection_ending_frame;

	bool shift_held, ctrl_held, alt_held;
	int shift_timer, ctrl_timer;
	int shift_count, ctrl_count;

	HWND hwndMarkerDragBox, hwndMarkerDragBoxText;
	// GDI stuff
	HFONT hMainListFont, hMainListSelectFont, hMarkersFont, hMarkersEditFont, hTaseditorAboutFont;
	HBRUSH bg_brush, marker_drag_box_brush, marker_drag_box_brush_bind;

private:
	void CenterListAt(int frame);

	void DragPlaybackCursor();
	void FinishDrag();

	std::vector<uint8> header_colors;
	int num_columns;
	int next_header_update_time;

	bool must_redraw_list;

	HMENU hrmenu;

	// GDI stuff
	HIMAGELIST himglist;

	WNDCLASSEX wincl;
	BLENDFUNCTION blend;

};
