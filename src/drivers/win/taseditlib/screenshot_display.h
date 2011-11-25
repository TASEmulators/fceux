//Specification file for SCREENSHOT_DISPLAY class

#define SCR_BMP_PHASE_MAX 13
#define SCR_BMP_PHASE_ALPHA_MAX 10

#define DISPLAY_UPDATE_TICK 40		// update at 25FPS

class SCREENSHOT_DISPLAY
{
public:
	SCREENSHOT_DISPLAY();
	void init();
	void free();
	void reset();
	void update();

	void ChangeScreenshotBitmap();
	void RedrawScreenshotBitmap();

	void ParentWindowMoved();

	int screenshot_currently_shown;
	HWND hwndScrBmp, scr_bmp_pic;
	
private:
	int scr_bmp_x;
	int scr_bmp_y;
	int scr_bmp_phase;
	int next_update_time;
	
	WNDCLASSEX wincl;
	BLENDFUNCTION blend;
	LPBITMAPINFO scr_bmi;
	HBITMAP scr_bmp;
	uint8* scr_ptr;

};
