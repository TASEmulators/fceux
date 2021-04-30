#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "driver.h"
#include "common/os_utils.h"

#include "Qt/AviRecord.h"
#include "Qt/avi/gwavi.h"
#include "Qt/nes_shm.h"
#include "Qt/ConsoleUtilities.h"

static gwavi_t  *gwavi = NULL;
static bool      recordEnable = false;
static int       vbufHead = 0;
static int       vbufTail = 0;
static int       vbufSize = 0;
static int       abufHead = 0;
static int       abufTail = 0;
static int       abufSize = 0;
static uint32_t *rawVideoBuf = NULL;
static int16_t  *rawAudioBuf = NULL;
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
	unsigned int fps;

	if ( gwavi != NULL )
	{
		delete gwavi; gwavi = NULL;
	}
	fps = FCEUI_GetDesiredFPS() >> 24;

	audioConfig.channels = 1;
	audioConfig.bits     = 16;
	audioConfig.samples_per_second = 48000;

	memset( fourcc, 0, sizeof(fourcc) );

	gwavi = new gwavi_t();

	if ( gwavi->open( "/tmp/test.avi", nes_shm->video.ncol, nes_shm->video.nrow, fourcc, fps, &audioConfig ) )
	{
		printf("Error: Failed to open AVI file.\n");
		recordEnable = false;
		return -1;
	}

	vbufSize    = 1024 * 1024 * 60;
	rawVideoBuf = (uint32_t*)malloc( vbufSize * sizeof(uint32_t) );

	abufSize    = 48000;
	rawAudioBuf = (int16_t*)malloc( abufSize * sizeof(uint16_t) );

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
	int i, head, numPixels, availSize;

	numPixels  = nes_shm->video.ncol * nes_shm->video.nrow;

	availSize = (vbufTail - vbufHead);
	if ( availSize <= 0 )
	{
		availSize += vbufSize;
	}

	while ( numPixels > availSize )
	{
		//printf("Video Unavail %i \n", availSize );
		msleep(1);

		availSize = (vbufTail - vbufHead);
		if ( availSize <= 0 )
		{
			availSize += vbufSize;
		}
	}

	i = 0; head = vbufHead;

	while ( i < numPixels )
	{
		rawVideoBuf[ head ] = nes_shm->pixbuf[i]; i++;

		head = (head + 1) % vbufSize;
	}
	vbufHead = head;

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

	for (int i=0; i<numSamples; i++)
	{
		rawAudioBuf[ abufHead ] = buf[i];

		abufHead = (abufHead + 1) % abufSize;
	}

	return 0;
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

	if ( rawVideoBuf != NULL )
	{
		free(rawVideoBuf); rawVideoBuf = NULL;
	}
	if ( rawAudioBuf != NULL )
	{
		free(rawAudioBuf); rawAudioBuf = NULL;
	}
	vbufTail = abufTail = 0;
	vbufSize = abufSize = 0;

	return 0;
}
//**************************************************************************************
bool aviRecordRunning(void)
{
	return recordEnable;
}
//**************************************************************************************
// AVI Recorder Disk Thread
//**************************************************************************************
//----------------------------------------------------
AviRecordDiskThread_t::AviRecordDiskThread_t( QObject *parent )
	: QThread(parent)
{
}
//----------------------------------------------------
AviRecordDiskThread_t::~AviRecordDiskThread_t(void)
{

}
//----------------------------------------------------
void AviRecordDiskThread_t::run(void)
{
	int numPixels, width, height, numPixelsReady = 0;
	int numSamples = 0;
	unsigned char *rgb24;
	int16_t audioOut[48000];
	uint32_t videoOut[1048576];

	printf("AVI Record Disk Start\n");

	setPriority( QThread::HighestPriority );

	//avgAudioPerFrame = 48000 / 60;

	width     = nes_shm->video.ncol;
	height    = nes_shm->video.nrow;
	numPixels = width * height;

	rgb24 = (unsigned char *)malloc( numPixels * sizeof(uint32_t) );

	while ( !isInterruptionRequested() )
	{
		
		while ( (numPixelsReady < numPixels) && (vbufTail != vbufHead) )
		{
			videoOut[ numPixelsReady ] = rawVideoBuf[ vbufTail ]; numPixelsReady++;
			
			vbufTail = (vbufTail + 1) % vbufSize;
		}

		if ( numPixelsReady >= numPixels )
		{
			convertRgb_32_to_24( (const unsigned char*)videoOut, rgb24,
					width, height, numPixels );
			gwavi->add_frame( rgb24, numPixels*3 );

			numPixelsReady = 0;

			numSamples = 0;

			while ( abufHead != abufTail )
			{
				audioOut[ numSamples ] = rawAudioBuf[ abufTail ]; numSamples++;

				abufTail = (abufTail + 1) % abufSize;

				//if ( numSamples > avgAudioPerFrame )
				//{
				//	break;
				//}
			}

			if ( numSamples > 0 )
			{
				//printf("NUM Audio Samples: %i \n", numSamples );
				gwavi->add_audio( (unsigned char *)audioOut, numSamples*2);

				numSamples = 0;
			}
		}
		else
		{
			msleep(1);
		}
	}

	free(rgb24);

	aviRecordClose();

	printf("AVI Record Disk Exit\n");
	emit finished();
}
//----------------------------------------------------
//**************************************************************************************
