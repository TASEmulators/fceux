/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020 mjbudd77
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
//
// AviRecord.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifdef WIN32
#include <windows.h>
#include <vfw.h>
#endif

#include <QObject>
#include <QMessageBox>

#include "driver.h"
#include "common/os_utils.h"

#ifdef _USE_X264
#include "x264.h"
#endif

#include "Qt/AviRecord.h"
#include "Qt/avi/gwavi.h"
#include "Qt/nes_shm.h"
#include "Qt/throttle.h"
#include "Qt/ConsoleWindow.h"
#include "Qt/ConsoleUtilities.h"
#include "Qt/fceuWrapper.h"

static gwavi_t  *gwavi = NULL;
static bool      recordEnable = false;
static bool      recordAudio  = true;
static int       vbufHead = 0;
static int       vbufTail = 0;
static int       vbufSize = 0;
static int       abufHead = 0;
static int       abufTail = 0;
static int       abufSize = 0;
static uint32_t *rawVideoBuf = NULL;
static int16_t  *rawAudioBuf = NULL;
static int       videoFormat = AVI_RGB24;
static int       audioSampleRate = 48000;
//**************************************************************************************

static void convertRgb_32_to_24( const unsigned char *src, unsigned char *dest, int w, int h, int nPix, bool verticalFlip )
{
	int i=0, j=0, x, y;

	if ( verticalFlip )
	{
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
	else
	{
		int size;

		size = nPix*4;
		i=0;
		while ( i < size )
		{
				dest[j] = src[i]; i++; j++;
				dest[j] = src[i]; i++; j++;
				dest[j] = src[i]; i++; j++;
				i++; 
		}
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
// Windows VFW Interface
#ifdef WIN32
namespace VFW
{
static bool cmpSet = false;
static COMPVARS  cmpvars;
static DWORD dwFlags = 0;
static BITMAPINFOHEADER   bmapIn;
static LPBITMAPINFOHEADER bmapOut = NULL;
static DWORD frameNum = 0;
static DWORD dwQuality = 0;
static LPVOID outBuf = NULL;

static int chooseConfig(int width, int height)
{
	bool ret;

	if ( cmpSet )
	{
		ICCompressorFree( &cmpvars );
		cmpSet = false;
	}
	memset( &cmpvars, 0, sizeof(COMPVARS));
	cmpvars.cbSize = sizeof(COMPVARS);
	ret = ICCompressorChoose( HWND(consoleWindow->winId()), ICMF_CHOOSE_ALLCOMPRESSORS,
			0, NULL, &cmpvars, 0);

	printf("FCC:%08X  %c%c%c%c \n", cmpvars.fccHandler,
	    (cmpvars.fccHandler & 0x000000FF) ,
	    (cmpvars.fccHandler & 0x0000FF00) >>  8,
	    (cmpvars.fccHandler & 0x00FF0000) >> 16,
	    (cmpvars.fccHandler & 0xFF000000) >> 24 );

	if ( ret )
	{
		cmpSet = true;
	}
	return (cmpSet == false) ? -1 : 0;
}

static int init( int width, int height )
{
	void *h;
	DWORD dwFormatSize, dwCompressBufferSize;

	memset( &bmapIn, 0, sizeof(bmapIn));
	bmapIn.biSize = sizeof(BITMAPINFOHEADER);
	bmapIn.biWidth = width;
	bmapIn.biHeight = height;
	bmapIn.biPlanes = 1;
	bmapIn.biBitCount = 24;
	bmapIn.biCompression = BI_RGB;
	bmapIn.biSizeImage = width * height * 3;

	dwFormatSize = ICCompressGetFormatSize( cmpvars.hic, &bmapIn );

	//printf("Format Size:%i  %zi\n", dwFormatSize, sizeof(BITMAPINFOHEADER));

	h = GlobalAlloc(GHND, dwFormatSize); 
	bmapOut = (LPBITMAPINFOHEADER)GlobalLock(h); 
	memset( bmapOut, 0, sizeof(bmapOut));
	bmapOut->biSize = sizeof(BITMAPINFOHEADER);
	ICCompressGetFormat( cmpvars.hic, &bmapIn, bmapOut);
	
	// Find the worst-case buffer size. 
	dwCompressBufferSize = ICCompressGetSize( cmpvars.hic, &bmapIn, bmapOut); 
 
	//printf("Worst Case Compress Buffer Size: %i\n", dwCompressBufferSize );

	// Allocate a buffer and get lpOutput to point to it. 
	h = GlobalAlloc(GHND, dwCompressBufferSize); 
	outBuf = (LPVOID)GlobalLock(h);

	//dwQuality = ICGetDefaultQuality( cmpvars.hic ); 
	dwQuality = cmpvars.lQ; 

	//printf("Quality Setting: %i\n", dwQuality );

	ICCompressBegin( cmpvars.hic, &bmapIn, bmapOut );
	
	return 0;
}

static int close(void)
{
	ICCompressEnd( cmpvars.hic);

	GlobalFree(bmapOut);
	GlobalFree(outBuf);

	if ( cmpSet )
	{
		ICCompressorFree( &cmpvars );
		cmpSet = false;
	}
	return 0;
}

static int encode_frame( unsigned char *inBuf, int width, int height )
{
	DWORD ret;
	DWORD flagsOut = 0, reserved = 0;
	int bytesWritten = 0;

	ret = ICCompress( 
		cmpvars.hic, 
		dwFlags, 
		bmapOut,
		outBuf,
		&bmapIn, 
		inBuf,
		&reserved,
		&flagsOut,
		frameNum++,
		0,
		dwQuality,
		NULL, NULL );

	if ( ret == ICERR_OK )
	{
		//printf("Compressing Frame:%i   Size:%i\n", frameNum, bmapOut->biSizeImage);
		bytesWritten = bmapOut->biSizeImage;
		gwavi->add_frame( (unsigned char*)outBuf, bytesWritten );
	}

	return bytesWritten;
}

} // End namespace VFW
#endif
//**************************************************************************************
int aviRecordOpenFile( const char *filepath )
{
	char fourcc[8];
	gwavi_audio_t  audioConfig;
	double fps;
	char fileName[1024];

#ifdef WIN32
	if ( videoFormat == AVI_VFW )
	{
		if ( VFW::chooseConfig( nes_shm->video.ncol, nes_shm->video.nrow ) )
		{
			return -1;
		}
	}
#endif

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

	if ( fileName[0] != 0 )
	{
		QFile file(fileName);

		if ( file.exists() )
		{
			int ret;
			std::string msg;

			msg = "Pre-existing AVI file will be overwritten:\n\n" +
				std::string(fileName) +	"\n\nReplace file?";

			ret = QMessageBox::warning( consoleWindow, QObject::tr("Overwrite Warning"),
					QString::fromStdString(msg), QMessageBox::Yes | QMessageBox::No );

			if ( ret == QMessageBox::No )
			{
				return -1;
			}
		}
	}

	if ( gwavi != NULL )
	{
		delete gwavi; gwavi = NULL;
	}
	fps = getBaseFrameRate();

	g_config->getOption("SDL.Sound.Rate", &audioSampleRate);

	audioConfig.channels = 1;
	audioConfig.bits     = 16;
	audioConfig.samples_per_second = audioSampleRate;

	memset( fourcc, 0, sizeof(fourcc) );

	if ( videoFormat == AVI_I420 )
	{
		strcpy( fourcc, "I420");
	}
	#ifdef _USE_X264
	else if ( videoFormat == AVI_X264 )
	{
		strcpy( fourcc, "X264");
	}
	#endif 
	#ifdef WIN32
	else if ( videoFormat == AVI_VFW )
	{
		memcpy( fourcc, &VFW::cmpvars.fccHandler, 4);
		for (int i=0; i<4; i++)
		{
			fourcc[i] = toupper(fourcc[i]);
		}
		printf("Set VFW FourCC: %s\n", fourcc);
	}
	#endif 

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
		rawVideoBuf[ head ] = nes_shm->avibuf[i]; i++;

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
	if ( !recordAudio )
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
bool aviGetAudioEnable(void)
{
	return recordAudio;
}
//**************************************************************************************
void aviSetAudioEnable(bool val)
{
	recordAudio = val;
}
//**************************************************************************************
bool aviRecordRunning(void)
{
	return recordEnable;
}
bool FCEUI_AviIsRecording(void)
{
	return recordEnable;
}
//**************************************************************************************
void FCEUD_AviRecordTo(void)
{
	return;
}
//**************************************************************************************
void FCEUD_AviStop(void)
{
	return;
}
//**************************************************************************************
// // This function is implemented in sdl-video.cpp
//void FCEUI_AviVideoUpdate(const unsigned char* buffer)
//{
//	return;
//}
//**************************************************************************************
int aviGetSelVideoFormat(void)
{
	return videoFormat;
}
//**************************************************************************************
void aviSetSelVideoFormat(int idx)
{
	videoFormat = idx;
	//printf("AVI Video Format Changed:%i\n", videoFormat );

	g_config->setOption("SDL.AviVideoFormat", videoFormat);
}
//**************************************************************************************
int FCEUD_AviGetFormatOpts( std::vector <std::string> &formatList )
{
	std::string s;

	for (int i=0; i<AVI_NUM_ENC; i++)
	{
		switch ( i )
		{
			default:
				s.assign("Unknown");
			break;
			case AVI_RGB24:
				s.assign("RGB24 (Uncompressed)");
			break;
			case AVI_I420:
				s.assign("I420 (YUV 4:2:0)");
			break;
			#ifdef _USE_X264
			case AVI_X264:
				s.assign("X264 (H.264)");
			break;
			#endif
			#ifdef WIN32
			case AVI_VFW:
				s.assign("VfW (Video for Windows)");
			break;
			#endif
		}
		formatList.push_back(s);
	}
	return AVI_NUM_ENC;
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
	double fps = 60.0;
	unsigned char *rgb24;
	int16_t *audioOut;
	uint32_t *videoOut;
	char writeAudio = 1;
	int  avgAudioPerFrame, localVideoFormat;

	printf("AVI Record Disk Start\n");

	setPriority( QThread::HighestPriority );

	fps = getBaseFrameRate();

	avgAudioPerFrame = ( audioSampleRate / fps) + 1;

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
	if ( localVideoFormat == AVI_X264)
	{
		X264::init( width, height );
	}
#endif
#ifdef WIN32
	if ( localVideoFormat == AVI_VFW)
	{
		VFW::init( width, height );
	}
#endif

	audioOut = (int16_t *)malloc(48000);
	videoOut = (uint32_t*)malloc(1048576);

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

			if ( localVideoFormat == AVI_I420)
			{
				Convert_4byte_To_I420Frame<4>(videoOut,rgb24,numPixels,width);
				gwavi->add_frame( rgb24, (numPixels*3)/2 );
			}
			#ifdef _USE_X264
			else if ( localVideoFormat == AVI_X264)
			{
				Convert_4byte_To_I420Frame<4>(videoOut,rgb24,numPixels,width);
				X264::encode_frame( rgb24, width, height );
			}
			#endif
			#ifdef WIN32
			else if ( localVideoFormat == AVI_VFW)
			{
				convertRgb_32_to_24( (const unsigned char*)videoOut, rgb24,
						width, height, numPixels, true );
				writeAudio = VFW::encode_frame( rgb24, width, height ) > 0;
			}
			#endif
			else
			{
				convertRgb_32_to_24( (const unsigned char*)videoOut, rgb24,
						width, height, numPixels, true );
				gwavi->add_frame( rgb24, numPixels*3 );
			}

			numPixelsReady = 0;

			if ( writeAudio && recordAudio )
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
	if ( localVideoFormat == AVI_X264)
	{
		X264::close();
	}
#endif
#ifdef WIN32
	if ( localVideoFormat == AVI_VFW)
	{
		VFW::close();
	}
#endif
	aviRecordClose();

	free(audioOut);
	free(videoOut);

	printf("AVI Record Disk Exit\n");
	emit finished();
}
//----------------------------------------------------
//**************************************************************************************
