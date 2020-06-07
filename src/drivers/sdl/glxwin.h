// glxwin.cpp
//

#ifndef __GLXWIN_H__
#define __GLXWIN_H__

#include <stdint.h>

#include <semaphore.h>

int  spawn_glxwin( int flags );

#define  GLX_NES_WIDTH   256
#define  GLX_NES_HEIGHT  256

struct  glxwin_shm_t
{
	int   pid;
	int   run;
	uint32_t  render_count;
	uint32_t  blit_count;
	sem_t     sem;

	// Pass Key Events back to GTK Gui
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
};

extern glxwin_shm_t *glx_shm;

#endif
