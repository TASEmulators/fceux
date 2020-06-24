// glxwin.cpp
//

#ifndef __GLXWIN_H__
#define __GLXWIN_H__

#include <stdint.h>

#define  GL_WIN_PIXEL_LINEAR_FILTER  0x0001
#define  GL_WIN_DOUBLE_BUFFER        0x0002

#define  GL_NES_WIDTH   256
#define  GL_NES_HEIGHT  256

struct  gl_win_shm_t
{
	int   pid;
	int   run;
	uint32_t  render_count;
	uint32_t  blit_count;

	int   ncol;
	int   nrow;
	int   pitch;

	// Pass Key Events back to QT Gui
	struct 
	{
		int head;
		int tail;

		struct {
			int type;
			int keycode;
			int state;
		} data[64];

	} keyEventBuf;

	// Gui Command Event Queue
	struct 
	{
		int head;
		int tail;

		struct {
			 int id;

			 union {
			 	int   i32[4];
				float f32[4];
			 } param;
		} cmd[64];
	} guiEvent;

	uint32_t  pixbuf[65536]; // 256 x 256

	void clear_pixbuf(void)
	{
		memset( pixbuf, 0, sizeof(pixbuf) );
	}
};

extern gl_win_shm_t *gl_shm;

gl_win_shm_t *open_video_shm(void);

#endif
