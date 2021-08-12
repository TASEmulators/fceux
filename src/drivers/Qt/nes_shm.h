// nes_shm.h
//

#ifndef __NES_SHM_H__
#define __NES_SHM_H__

#include <stdint.h>

#define  GL_WIN_PIXEL_LINEAR_FILTER  0x0001
#define  GL_WIN_DOUBLE_BUFFER        0x0002

#define  GL_NES_WIDTH   256
#define  GL_NES_HEIGHT  240
#define  NES_AUDIO_BUFLEN   480000

struct  nes_shm_t
{
	int   pid;
	int   run;
	uint32_t  render_count;
	uint32_t  blit_count;

	struct 
	{
		int   ncol;
		int   nrow;
		int   pitch;
		int   xscale;
		int   yscale;
		int   xyRatio;
		int   preScaler;
	} video;

	char  runEmulator;
	char  blitUpdated;

	uint32_t  pixbuf[1048576]; // 1024 x 1024
	uint32_t  avibuf[1048576]; // 1024 x 1024

	void clear_pixbuf(void)
	{
		memset( pixbuf, 0, sizeof(pixbuf) );
		memset( avibuf, 0, sizeof(avibuf) );
	}

	struct sndBuf_t
	{
		int  head;
		int  tail;
		int16_t  data[NES_AUDIO_BUFLEN];
		unsigned int starveCounter;
	} sndBuf;

	void  push_sound_sample( int16_t sample )
	{
		sndBuf.data[ sndBuf.head ] = sample;
		sndBuf.head = (sndBuf.head + 1) % NES_AUDIO_BUFLEN;
	}
};

extern nes_shm_t *nes_shm;

nes_shm_t *open_nes_shm(void);

void close_nes_shm(void);

#endif
