// Specification file for POPUP_DISPLAY class

#define SCR_BMP_PHASE_MAX 10
#define SCR_BMP_PHASE_ALPHA_MAX 8
#define SCR_BMP_DX 7

#define SCR_BMP_TOOLTIP_GAP 2

#define MARKER_NOTE_TOOLTIP_WIDTH 360
#define MARKER_NOTE_TOOLTIP_HEIGHT 20

#define DISPLAY_UPDATE_TICK 40		// update at 25FPS

class POPUP_DISPLAY
{
public:
	POPUP_DISPLAY();
	void init();
	void free();
	void reset();
	void update();

	void ChangeScreenshotBitmap();
	void RedrawScreenshotBitmap();
	void ChangeTooltipText();

	void ParentWindowMoved();

	int screenshot_currently_shown;
	HWND hwndScrBmp, scr_bmp_pic, hwndMarkerNoteTooltip, marker_note_tooltip;
	
private:
	int scr_bmp_x;
	int scr_bmp_y;
	int scr_bmp_phase;
	int next_update_time;

	int tooltip_x;
	int tooltip_y;
	
	WNDCLASSEX wincl1, wincl2;
	BLENDFUNCTION blend;
	LPBITMAPINFO scr_bmi;
	HBITMAP scr_bmp;
	uint8* scr_ptr;

};
