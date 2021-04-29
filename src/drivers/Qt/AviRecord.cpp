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
int aviRecordOpenFile( const char *filepath, int format, int width, int height )
{
	if ( gwavi != NULL )
	{
		delete gwavi; gwavi = NULL;
	}

	gwavi = new gwavi_t();

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

	//gwavi->

	return 0;
}
//**************************************************************************************
int aviRecordClose(void)
{
	recordEnable = false;

	if ( gwavi != NULL )
	{
		delete gwavi; gwavi = NULL;
	}

	return 0;
}
//**************************************************************************************
