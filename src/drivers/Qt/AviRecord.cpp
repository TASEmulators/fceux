#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "driver.h"
#include "common/os_utils.h"

#ifdef _USE_X264
#include "x264.h"
#endif

#include "Qt/AviRecord.h"
#include "Qt/avi/gwavi.h"
#include "Qt/nes_shm.h"
#include "Qt/ConsoleUtilities.h"
#include "Qt/fceuWrapper.h"

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
static int       videoFormat = 2;
static int       audioSampleRate = 48000;
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
/* For RGB2YUV: */

static const int RGB2YUV_SHIFT = 15; /* highest value where [RGB][YUV] fit in signed short */

static const int RY = 8414;  //  ((int)(( 65.738/256.0)*(1<<RGB2YUV_SHIFT)+0.5));
static const int RV = 14392; //  ((int)((112.439/256.0)*(1<<RGB2YUV_SHIFT)+0.5));
static const int RU = -4856; //  ((int)((-37.945/256.0)*(1<<RGB2YUV_SHIFT)+0.5));

static const int GY = 16519; //  ((int)((129.057/256.0)*(1<<RGB2YUV_SHIFT)+0.5));
static const int GV = -12051;//  ((int)((-94.154/256.0)*(1<<RGB2YUV_SHIFT)+0.5));
static const int GU = -9534; //  ((int)((-74.494/256.0)*(1<<RGB2YUV_SHIFT)+0.5));

static const int BY = 3208;  //  ((int)(( 25.064/256.0)*(1<<RGB2YUV_SHIFT)+0.5));
static const int BV = -2339; //  ((int)((-18.285/256.0)*(1<<RGB2YUV_SHIFT)+0.5));
static const int BU = 14392; //  ((int)((112.439/256.0)*(1<<RGB2YUV_SHIFT)+0.5));

static const int Y_ADD = 16;
static const int U_ADD = 128;
static const int V_ADD = 128;

template<int PixStride>
void Convert_4byte_To_I420Frame(const void* data, unsigned char* dest, unsigned npixels, unsigned width)
{
    const unsigned char* src = (const unsigned char*) data;
    unsigned height = npixels / width;
    
    unsigned pos = 0;
    unsigned ypos = 0;
    unsigned vpos = npixels;
    unsigned upos = vpos + npixels / 4;
    unsigned stride = width*PixStride;

    /*fprintf(stderr, "npixels=%u, width=%u, height=%u, ypos=%u,upos=%u,vpos=%u",
        npixels,width,height, ypos,upos,vpos);*/

    /* This function is based on code from x264 svn version 711 */
    /* TODO: Apply MMX optimization for 24-bit pixels */
    
    for(unsigned y=0; y<height; y += 2)
    {
        for(unsigned x=0; x<width; x += 2)
        {
        //#ifdef __MMX__
        //  if(PixStride == 4)
        //  {
        //    c64_MMX p0_1; p0_1.Get(&src[pos]);        // two 32-bit pixels (4*8)
        //    c64_MMX p2_3; p2_3.Get(&src[pos+stride]); // two 32-bit pixels

        //    pos += PixStride*2;
        //    
        //    Convert_I420_MMX_Common(p0_1, p2_3,
        //        dest+ypos,
        //        dest+ypos+width,
        //        dest+upos++,
        //        dest+vpos++);
        //  }
        //  else
        //#endif
          {
            int c[3], rgb[3][4];
            
            /* luma */
            for(int n=0; n<3; ++n) c[n]  = rgb[n][0] = src[pos + n];
            for(int n=0; n<3; ++n) c[n] += rgb[n][1] = src[pos + n + stride];
            pos += PixStride;
            
            for(int n=0; n<3; ++n) c[n] += rgb[n][2] = src[pos + n];
            for(int n=0; n<3; ++n) c[n] += rgb[n][3] = src[pos + n + stride];
            pos += PixStride;

            unsigned destpos[4] = { ypos, ypos+width, ypos+1, ypos+width+1 };
            for(int n=0; n<4; ++n)
            {
                dest[destpos[n]]
                    = Y_ADD + ((RY * rgb[0][n]
                              + GY * rgb[1][n]
                              + BY * rgb[2][n]
                               ) >> RGB2YUV_SHIFT);  // y
            }
            
            dest[upos++] = (U_ADD + ((RU * c[0] + GU * c[1] + BU * c[2]) >> (RGB2YUV_SHIFT+2)) );
            dest[vpos++] = (V_ADD + ((RV * c[0] + GV * c[1] + BV * c[2]) >> (RGB2YUV_SHIFT+2)) ); 
          }
            
            ypos += 2;
        }
        pos += stride;
        ypos += width;
    }
    
    /*fprintf(stderr, ",yr=%u,ur=%u,vr=%u\n",
        ypos,upos,vpos);*/
    
    //#ifdef __MMX__
    // MMX_clear();
    //#endif
}
//**************************************************************************************
#ifdef _USE_X264

namespace X264
{
static x264_param_t param;
static x264_picture_t pic;
static x264_picture_t pic_out;
static x264_t *hdl = NULL;
static x264_nal_t *nal = NULL;
static int i_nal = 0;
static int i_frame = 0;

static int init( int width, int height )
{
	/* Get default params for preset/tuning */
	if( x264_param_default_preset( &param, "medium", NULL ) < 0 )
	{
	    goto x264_init_fail;
	}
	
	/* Configure non-default params */
	param.i_bitdepth = 8;
	param.i_csp = X264_CSP_I420;
	param.i_width  = width;
	param.i_height = height;
	param.b_vfr_input = 0;
	param.b_repeat_headers = 1;
	param.b_annexb = 1;
	
	/* Apply profile restrictions. */
	if( x264_param_apply_profile( &param, "high" ) < 0 )
	{
	    goto x264_init_fail;
	}
	
	if( x264_picture_alloc( &pic, param.i_csp, param.i_width, param.i_height ) < 0 )
	{
	    goto x264_init_fail;
	}

	hdl = x264_encoder_open( &param );
	if ( hdl == NULL )
	{
		goto x264_init_fail;
	}
	i_frame = 0;

	return 0;

x264_init_fail:
	return -1;
}

static int encode_frame( unsigned char *inBuf, int width, int height )
{
	int luma_size = width * height;
	int chroma_size = luma_size / 4;
	int i_frame_size = 0;
	int ofs;

	ofs = 0;
	memcpy( pic.img.plane[0], &inBuf[ofs], luma_size   ); ofs += luma_size;
	memcpy( pic.img.plane[1], &inBuf[ofs], chroma_size ); ofs += chroma_size;
	memcpy( pic.img.plane[2], &inBuf[ofs], chroma_size ); ofs += chroma_size;

	pic.i_pts = i_frame++;

	i_frame_size = x264_encoder_encode( hdl, &nal, &i_nal, &pic, &pic_out );

	if ( i_frame_size < 0 )
	{
		return -1;
	}
	else if ( i_frame_size )
	{
		gwavi->add_frame( nal->p_payload, i_frame_size );
	}
	return i_frame_size;
}

static int close(void)
{
	int i_frame_size;

	/* Flush delayed frames */
	while( x264_encoder_delayed_frames( hdl ) )
	{
	    i_frame_size = x264_encoder_encode( hdl, &nal, &i_nal, NULL, &pic_out );

	    if ( i_frame_size < 0 )
	    {
	        break;
	    }
	    else if( i_frame_size )
	    {
		gwavi->add_frame( nal->p_payload, i_frame_size );
	    }
	}
	
	x264_encoder_close( hdl );
	x264_picture_clean( &pic );

	return 0;
}

}; // End X264 namespace
#endif
//**************************************************************************************
int aviRecordOpenFile( const char *filepath )
{
	char fourcc[8];
	gwavi_audio_t  audioConfig;
	unsigned int fps;
	char fileName[1024];


	if ( filepath != NULL )
	{
		strcpy( fileName, filepath );
	}
	else
	{
		const char *romFile;

		romFile = getRomFile();

		if ( romFile )
		{
			char base[512];
			const char *baseDir = FCEUI_GetBaseDirectory();

			getFileBaseName( romFile, base );

			if ( baseDir )
			{
				strcpy( fileName, baseDir );
				strcat( fileName, "/avi/" );
			}
			else
			{
				fileName[0] = 0;
			}
			strcat( fileName, base );
			strcat( fileName, ".avi");
			//printf("AVI Filepath:'%s'\n", fileName );
		}
		else
		{
			return -1;
		}
	}

	if ( gwavi != NULL )
	{
		delete gwavi; gwavi = NULL;
	}
	fps = FCEUI_GetDesiredFPS() >> 24;

	g_config->getOption("SDL.Sound.Rate", &audioSampleRate);

	audioConfig.channels = 1;
	audioConfig.bits     = 16;
	audioConfig.samples_per_second = audioSampleRate;

	memset( fourcc, 0, sizeof(fourcc) );


	if ( videoFormat == 1 )
	{
		strcpy( fourcc, "I420");
	}
	else if ( videoFormat == 2 )
	{
		strcpy( fourcc, "X264");
	}

	gwavi = new gwavi_t();

	if ( gwavi->open( fileName, nes_shm->video.ncol, nes_shm->video.nrow, fourcc, fps, &audioConfig ) )
	{
		printf("Error: Failed to open AVI file.\n");
		recordEnable = false;
		return -1;
	}

	vbufSize    = 1024 * 1024 * 60;
	rawVideoBuf = (uint32_t*)malloc( vbufSize * sizeof(uint32_t) );

	abufSize    = 48000;
	rawAudioBuf = (int16_t*)malloc( abufSize * sizeof(uint16_t) );

	vbufHead = 0;
	vbufTail = 0;
	abufHead = 0;
	abufTail = 0;

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
	if ( FCEUI_EmulationPaused() )
	{
		return 0;
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
	int fps = 60, numSamples = 0;
	unsigned char *rgb24;
	int16_t audioOut[48000];
	uint32_t videoOut[1048576];
	char writeAudio = 1;
	int  avgAudioPerFrame, localVideoFormat;

	printf("AVI Record Disk Start\n");

	setPriority( QThread::HighestPriority );

	fps = FCEUI_GetDesiredFPS() >> 24;

	avgAudioPerFrame = (audioSampleRate / fps) + 1;

	printf("Avg Audio Rate per Frame: %i \n", avgAudioPerFrame );

	width     = nes_shm->video.ncol;
	height    = nes_shm->video.nrow;
	numPixels = width * height;

	rgb24 = (unsigned char *)malloc( numPixels * sizeof(uint32_t) );

	if ( rgb24 )
	{
		memset( rgb24, 0, numPixels * sizeof(uint32_t) );
	}
	else
	{
		// Error allocating buffer.
		return;
	}
	localVideoFormat = videoFormat;

#ifdef _USE_X264
	if ( localVideoFormat == 2)
	{
		X264::init( width, height );
	}
#endif

	while ( !isInterruptionRequested() )
	{
		
		while ( (numPixelsReady < numPixels) && (vbufTail != vbufHead) )
		{
			videoOut[ numPixelsReady ] = rawVideoBuf[ vbufTail ]; numPixelsReady++;
			
			vbufTail = (vbufTail + 1) % vbufSize;
		}

		if ( numPixelsReady >= numPixels )
		{
			//printf("Adding Frame:%i\n", frameCount++);

			writeAudio = 1;

			if ( localVideoFormat == 1)
			{
				Convert_4byte_To_I420Frame<4>(videoOut,rgb24,numPixels,width);
				gwavi->add_frame( rgb24, (numPixels*3)/2 );
			}
			#ifdef _USE_X264
			else if ( localVideoFormat == 2)
			{
				Convert_4byte_To_I420Frame<4>(videoOut,rgb24,numPixels,width);
				X264::encode_frame( rgb24, width, height );
			}
			#endif
			else
			{
				convertRgb_32_to_24( (const unsigned char*)videoOut, rgb24,
						width, height, numPixels );
				gwavi->add_frame( rgb24, numPixels*3 );
			}

			numPixelsReady = 0;

			if ( writeAudio )
			{
				numSamples = 0;

				while ( abufHead != abufTail )
				{
					audioOut[ numSamples ] = rawAudioBuf[ abufTail ]; numSamples++;

					abufTail = (abufTail + 1) % abufSize;

					if ( numSamples > avgAudioPerFrame )
					{
						break;
					}
				}

				if ( numSamples > 0 )
				{
					//printf("NUM Audio Samples: %i \n", numSamples );
					gwavi->add_audio( (unsigned char *)audioOut, numSamples*2);

					numSamples = 0;
				}
			}
		}
		else
		{
			msleep(1);
		}
	}

	free(rgb24);

#ifdef _USE_X264
	if ( localVideoFormat == 2)
	{
		X264::close();
	}
#endif
	aviRecordClose();

	printf("AVI Record Disk Exit\n");
	emit finished();
}
//----------------------------------------------------
//**************************************************************************************
