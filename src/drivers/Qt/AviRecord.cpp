#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "Qt/AviRecord.h"
#include "Qt/avi/gwavi.h"
#include "Qt/nes_shm.h"

static gwavi_t  *gwavi = NULL;
static bool      recordEnable = false;
//**************************************************************************************

static void convertRgb_32_to_24( const unsigned char *src, unsigned char *dest, int w, int h, int nPix )
{
	int i=0, j=0, x, y;

	// Uncompressed RGB needs to be flipped vertically
	y = h-1;

	while ( y >= 0 )
	{
		x = 0;
		i = y*w*4;

		while ( x < w )
		{
			dest[j] = src[i]; i++; j++;
			dest[j] = src[i]; i++; j++;
			dest[j] = src[i]; i++; j++;
			i++; 
			x++;
		}
		y--;
	}
}
//**************************************************************************************
int aviRecordOpenFile( const char *filepath, int format, int width, int height )
{
	char fourcc[8];
	gwavi_audio_t  audioConfig;

	if ( gwavi != NULL )
	{
		delete gwavi; gwavi = NULL;
	}
	audioConfig.channels = 1;
	audioConfig.bits     = 16;
	audioConfig.samples_per_second = 48000;

	memset( fourcc, 0, sizeof(fourcc) );

	gwavi = new gwavi_t();

	if ( gwavi->open( "/tmp/test.avi", nes_shm->video.ncol, nes_shm->video.nrow, fourcc, 60, &audioConfig ) )
	{
		printf("Error: Failed to open AVI file.\n");
		recordEnable = false;
		return -1;
	}

	recordEnable = true;
	return 0;
}
//**************************************************************************************
int aviRecordAddFrame( void )
{
	if ( !recordEnable )
	{
		return -1;
	}

	if ( gwavi == NULL )
	{
		return -1;
	}
	int numPixels, bufferSize;

	numPixels  = nes_shm->video.ncol * nes_shm->video.nrow;
	bufferSize = numPixels * sizeof(uint32_t);

	{
		unsigned char rgb24[bufferSize];
		convertRgb_32_to_24( (const unsigned char*)nes_shm->pixbuf, rgb24,
				nes_shm->video.ncol, nes_shm->video.nrow, numPixels );
		gwavi->add_frame( rgb24, numPixels*3 );
	}

	return 0;
}
//**************************************************************************************
int aviRecordAddAudioFrame( int32_t *buf, int numSamples )
{
	if ( !recordEnable )
	{
		return -1;
	}

	if ( gwavi == NULL )
	{
		return -1;
	}
	int16_t lclBuf[numSamples];

	for (int i=0; i<numSamples; i++)
	{
		lclBuf[i] = buf[i];
	}
	gwavi->add_audio( (unsigned char *)lclBuf, numSamples*2);
}
//**************************************************************************************
int aviRecordClose(void)
{
	recordEnable = false;

	if ( gwavi != NULL )
	{
		gwavi->close();

		delete gwavi; gwavi = NULL;
	}

	return 0;
}
//**************************************************************************************
bool aviRecordRunning(void)
{
	return recordEnable;
}
//**************************************************************************************
