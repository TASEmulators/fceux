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
#include <string>

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
#ifdef _USE_X265
#include "x265.h"
#endif
#ifdef _USE_LIBAV
#ifdef __cplusplus
extern "C"
{
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
//#include "libavresample/avresample.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}
#endif
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

//static const int RGB2YUV_SHIFT = 15; // highest value where [RGB][YUV] fit in signed short

//static const int RY = 8414;  //  ((int)(( 65.738/256.0)*(1<<RGB2YUV_SHIFT)+0.5));
//static const int RV = 14392; //  ((int)((112.439/256.0)*(1<<RGB2YUV_SHIFT)+0.5));
//static const int RU = -4856; //  ((int)((-37.945/256.0)*(1<<RGB2YUV_SHIFT)+0.5));
//
//static const int GY = 16519; //  ((int)((129.057/256.0)*(1<<RGB2YUV_SHIFT)+0.5));
//static const int GV = -12051;//  ((int)((-94.154/256.0)*(1<<RGB2YUV_SHIFT)+0.5));
//static const int GU = -9534; //  ((int)((-74.494/256.0)*(1<<RGB2YUV_SHIFT)+0.5));
//
//static const int BY = 3208;  //  ((int)(( 25.064/256.0)*(1<<RGB2YUV_SHIFT)+0.5));
//static const int BV = -2339; //  ((int)((-18.285/256.0)*(1<<RGB2YUV_SHIFT)+0.5));
//static const int BU = 14392; //  ((int)((112.439/256.0)*(1<<RGB2YUV_SHIFT)+0.5));

#define  RGB2YUV_SHIFT  15  // highest value where [RGB][YUV] fit in signed short

//#define BY ((int)( 0.098 * (1 << RGB2YUV_SHIFT) + 0.5))
//#define BV ((int)(-0.071 * (1 << RGB2YUV_SHIFT) + 0.5))
//#define BU ((int)( 0.439 * (1 << RGB2YUV_SHIFT) + 0.5))
//#define GY ((int)( 0.504 * (1 << RGB2YUV_SHIFT) + 0.5))
//#define GV ((int)(-0.368 * (1 << RGB2YUV_SHIFT) + 0.5))
//#define GU ((int)(-0.291 * (1 << RGB2YUV_SHIFT) + 0.5))
//#define RY ((int)( 0.257 * (1 << RGB2YUV_SHIFT) + 0.5))
//#define RV ((int)( 0.439 * (1 << RGB2YUV_SHIFT) + 0.5))
//#define RU ((int)(-0.148 * (1 << RGB2YUV_SHIFT) + 0.5))

//#define BY ((int)( 0.114 * (1 << RGB2YUV_SHIFT) + 0.5))
//#define BV ((int)(-0.100 * (1 << RGB2YUV_SHIFT) + 0.5))
//#define BU ((int)( 0.436 * (1 << RGB2YUV_SHIFT) + 0.5))
//#define GY ((int)( 0.587 * (1 << RGB2YUV_SHIFT) + 0.5))
//#define GV ((int)(-0.515 * (1 << RGB2YUV_SHIFT) + 0.5))
//#define GU ((int)(-0.289 * (1 << RGB2YUV_SHIFT) + 0.5))
//#define RY ((int)( 0.299 * (1 << RGB2YUV_SHIFT) + 0.5))
//#define RV ((int)( 0.615 * (1 << RGB2YUV_SHIFT) + 0.5))
//#define RU ((int)(-0.148 * (1 << RGB2YUV_SHIFT) + 0.5))

//#define BY ((int)( 0.11400 * (1 << RGB2YUV_SHIFT) + 0.5))
//#define BV ((int)(-0.08131 * (1 << RGB2YUV_SHIFT) + 0.5))
//#define BU ((int)( 0.50000 * (1 << RGB2YUV_SHIFT) + 0.5))
//#define GY ((int)( 0.58700 * (1 << RGB2YUV_SHIFT) + 0.5))
//#define GV ((int)(-0.41869 * (1 << RGB2YUV_SHIFT) + 0.5))
//#define GU ((int)(-0.33126 * (1 << RGB2YUV_SHIFT) + 0.5))
//#define RY ((int)( 0.29900 * (1 << RGB2YUV_SHIFT) + 0.5))
//#define RV ((int)( 0.50000 * (1 << RGB2YUV_SHIFT) + 0.5))
//#define RU ((int)(-0.16874 * (1 << RGB2YUV_SHIFT) + 0.5))

#define BY ( (int)(0.114 * 219 / 255 * (1 << RGB2YUV_SHIFT) + 0.5))
#define BV (-(int)(0.081 * 224 / 255 * (1 << RGB2YUV_SHIFT) + 0.5))
#define BU ( (int)(0.500 * 224 / 255 * (1 << RGB2YUV_SHIFT) + 0.5))
#define GY ( (int)(0.587 * 219 / 255 * (1 << RGB2YUV_SHIFT) + 0.5))
#define GV (-(int)(0.419 * 224 / 255 * (1 << RGB2YUV_SHIFT) + 0.5))
#define GU (-(int)(0.331 * 224 / 255 * (1 << RGB2YUV_SHIFT) + 0.5))
#define RY ( (int)(0.299 * 219 / 255 * (1 << RGB2YUV_SHIFT) + 0.5))
#define RV ( (int)(0.500 * 224 / 255 * (1 << RGB2YUV_SHIFT) + 0.5))
#define RU (-(int)(0.169 * 224 / 255 * (1 << RGB2YUV_SHIFT) + 0.5))

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
    int Y, U, V;

    /*fprintf(stderr, "npixels=%u, width=%u, height=%u, ypos=%u,upos=%u,vpos=%u",
        npixels,width,height, ypos,upos,vpos);*/

    /* This function is based on code from x264 svn version 711 */
    /* TODO: Apply MMX optimization for 24-bit pixels */
    
    for(unsigned y=0; y<height; y += 2)
    {
        for(unsigned x=0; x<width; x += 2)
        {
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
                 Y = Y_ADD + ((RY * rgb[0][n]
                              + GY * rgb[1][n]
                              + BY * rgb[2][n]
                               ) >> RGB2YUV_SHIFT);  // y

		 dest[destpos[n]] = Y;
            }
            
            U = (U_ADD + ((RU * c[0] + GU * c[1] + BU * c[2]) >> (RGB2YUV_SHIFT+2)) );
            V = (V_ADD + ((RV * c[0] + GV * c[1] + BV * c[2]) >> (RGB2YUV_SHIFT+2)) ); 

	    //printf("%i,%i = rgb( %i, %i, %i ) = yuv( %i, %i, %i )\n", x,y,
	    //    	    rgb[0][3], rgb[1][3], rgb[2][3], Y, U, V );
            dest[upos++] = U;
            dest[vpos++] = V;
          }
            
            ypos += 2;
        }
        pos += stride;
        ypos += width;
    }
    
    /*fprintf(stderr, ",yr=%u,ur=%u,vr=%u\n",
        ypos,upos,vpos);*/
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
	unsigned int flags = 0;

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
		if ( pic_out.b_keyframe )
		{
			flags |= gwavi_t::IF_KEYFRAME;
		}
		gwavi->add_frame( nal->p_payload, i_frame_size, flags );
	}
	return i_frame_size;
}

static int close(void)
{
	int i_frame_size;
	unsigned int flags = 0;

	/* Flush delayed frames */
	while( x264_encoder_delayed_frames( hdl ) )
	{
	    i_frame_size = x264_encoder_encode( hdl, &nal, &i_nal, NULL, &pic_out );

	    if ( i_frame_size < 0 )
	    {
	        break;
	    }
	    else if ( i_frame_size )
	    {
		flags = 0;

		if ( pic_out.b_keyframe )
		{
			flags |= gwavi_t::IF_KEYFRAME;
		}
		gwavi->add_frame( nal->p_payload, i_frame_size, flags );
	    }
	}
	
	x264_encoder_close( hdl );
	x264_picture_clean( &pic );

	return 0;
}

}; // End X264 namespace
#endif
//**************************************************************************************
#ifdef _USE_X265

namespace X265
{
static x265_param *param = NULL;
static x265_picture *pic = NULL;
static x265_picture pic_out;
static x265_encoder *hdl = NULL;
static x265_nal *nal = NULL;
static unsigned int i_nal = 0;

static int init( int width, int height )
{
	double fps;
	unsigned int usec;

	fps = getBaseFrameRate();

	usec = (unsigned int)((1000000.0 / fps)+0.50);
	
	param = x265_param_alloc();

	x265_param_default( param);

	/* Get default params for preset/tuning */
	//if( x265_param_default_preset( param, "medium", NULL ) < 0 )
	//{
	//    goto x265_init_fail;
	//}
	
	/* Configure non-default params */
	param->internalCsp = X265_CSP_I420;
	param->sourceWidth  = width;
	param->sourceHeight = height;
	param->bRepeatHeaders = 1;
	param->fpsNum   = 1000000;
	param->fpsDenom = usec;
	
	/* Apply profile restrictions. */
	//if( x265_param_apply_profile( param, "high" ) < 0 )
	//{
	//    goto x265_init_fail;
	//}
	
	if( (pic = x265_picture_alloc()) == NULL )
	{
	    goto x265_init_fail;
	}
	x265_picture_init( param, pic );

	hdl = x265_encoder_open( param );
	if ( hdl == NULL )
	{
		goto x265_init_fail;
	}

	return 0;

x265_init_fail:
	return -1;
}

static int encode_frame( unsigned char *inBuf, int width, int height )
{
	int luma_size = width * height;
	int chroma_size = luma_size / 4;
	int ret = 0;
	int ofs;
	unsigned int flags = 0, totalPayload = 0;

	ofs = 0;
	pic->planes[0] = &inBuf[ofs]; ofs += luma_size;
	pic->planes[1] = &inBuf[ofs]; ofs += chroma_size;
	pic->planes[2] = &inBuf[ofs]; ofs += chroma_size;
	pic->stride[0] = width;
	pic->stride[1] = width/2;
	pic->stride[2] = width/2;

	ret = x265_encoder_encode( hdl, &nal, &i_nal, pic, &pic_out );

	if ( ret <= 0 )
	{
		return -1;
	}
	else if ( i_nal > 0 )
	{
		flags = 0;
		totalPayload = 0;

		if ( IS_X265_TYPE_I(pic_out.sliceType) )
		{
			flags |= gwavi_t::IF_KEYFRAME;
		}

		for (unsigned int i=0; i<i_nal; i++)
		{
			totalPayload += nal[i].sizeBytes;
		}
		gwavi->add_frame( nal[0].payload, totalPayload, flags );
	}
	return ret;
}

static int close(void)
{
	int ret;
	unsigned int flags = 0, totalPayload = 0;

	/* Flush delayed frames */
	while( hdl != NULL )
	{
	    ret = x265_encoder_encode( hdl, &nal, &i_nal, NULL, &pic_out );

	    if ( ret <= 0 )
	    {
	        break;
	    }
	    else if ( i_nal > 0 )
	    {
		totalPayload = 0;
		flags = 0;

		if ( IS_X265_TYPE_I(pic_out.sliceType) )
		{
			flags |= gwavi_t::IF_KEYFRAME;
		}
		for (unsigned int i=0; i<i_nal; i++)
		{
			totalPayload += nal[i].sizeBytes;
		}
		gwavi->add_frame( nal[0].payload, totalPayload, flags );
	    }
	}
	
	x265_encoder_close( hdl );
	x265_picture_free( pic ); 
	x265_param_free( param );

	return 0;
}

}; // End X265 namespace
#endif
//**************************************************************************************
// Windows VFW Interface
#ifdef WIN32
namespace VFW
{
static bool cmpSet = false;
static COMPVARS  cmpvars;
//static DWORD dwFlags = 0;
static BITMAPINFOHEADER   bmapIn;
static LPBITMAPINFOHEADER bmapOut = NULL;
static DWORD frameNum = 0;
static DWORD dwQuality = 0;
static DWORD icErrCount = 0;
static DWORD flagsOut = 0;
static LPVOID outBuf = NULL;

static int chooseConfig(int width, int height)
{
	bool ret;
	char fccHandler[8];
	std::string fccHandlerString;

	if ( cmpSet )
	{
		ICCompressorFree( &cmpvars );
		cmpSet = false;
	}
	memset( fccHandler, 0, sizeof(fccHandler));
	memset( &cmpvars, 0, sizeof(COMPVARS));
	cmpvars.cbSize = sizeof(COMPVARS);

	g_config->getOption("SDL.AviVfwFccHandler", &fccHandlerString);

	if ( fccHandlerString.size() > 0 )
	{
		strcpy( fccHandler, fccHandlerString.c_str() );
		memcpy( &cmpvars.fccHandler, fccHandler, 4 );
		cmpvars.dwFlags = ICMF_COMPVARS_VALID;
	}

	ret = ICCompressorChoose( HWND(consoleWindow->winId()), ICMF_CHOOSE_ALLCOMPRESSORS,
			0, NULL, &cmpvars, 0);

	memcpy( fccHandler, &cmpvars.fccHandler, 4 );
	fccHandler[4] = 0;

	printf("FCC:%08X  %c%c%c%c \n", cmpvars.fccHandler,
	    (cmpvars.fccHandler & 0x000000FF) ,
	    (cmpvars.fccHandler & 0x0000FF00) >>  8,
	    (cmpvars.fccHandler & 0x00FF0000) >> 16,
	    (cmpvars.fccHandler & 0xFF000000) >> 24 );

	if ( ret )
	{
		g_config->setOption("SDL.AviVfwFccHandler", fccHandler);
		cmpSet = true;
	}
	return (cmpSet == false) ? -1 : 0;
}

static int init( int width, int height )
{
	void *h;
	ICINFO icInfo;
	DWORD dwFormatSize, dwCompressBufferSize;
	bool qualitySupported = false;

	memset( &bmapIn, 0, sizeof(bmapIn));
	bmapIn.biSize = sizeof(BITMAPINFOHEADER);
	bmapIn.biWidth = width;
	bmapIn.biHeight = height;
	bmapIn.biPlanes = 1;
	bmapIn.biBitCount = 24;
	bmapIn.biCompression = BI_RGB;
	bmapIn.biSizeImage = width * height * 3;

	dwFormatSize = ICCompressGetFormatSize( cmpvars.hic, &bmapIn );

	if ( ICGetInfo( cmpvars.hic, &icInfo, sizeof(icInfo) ) )
	{
		printf("Name : %ls\n" , icInfo.szName );
		printf("Flags: 0x%08X", icInfo.dwFlags );

		if ( icInfo.dwFlags & VIDCF_CRUNCH )
		{
			printf("  VIDCF_CRUNCH  ");
		}

		if ( icInfo.dwFlags & VIDCF_TEMPORAL )
		{
			printf("  VIDCF_TEMPORAL  ");
		}

		if ( icInfo.dwFlags & VIDCF_TEMPORAL )
		{
			printf("  VIDCF_TEMPORAL  ");
		}

		if ( icInfo.dwFlags & VIDCF_QUALITY )
		{
			printf("  VIDCF_QUALITY  ");
			qualitySupported = true;
		}

		if ( icInfo.dwFlags & VIDCF_FASTTEMPORALC )
		{
			printf("  VIDCF_FASTTEMPORALC  ");
		}

		if ( icInfo.dwFlags & VIDCF_FASTTEMPORALD )
		{
			printf("  VIDCF_FASTTEMPORALD  ");
		}
		printf("\n");

	}

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
	memset( outBuf, 0, dwCompressBufferSize);

	//dwQuality = ICGetDefaultQuality( cmpvars.hic ); 
	if ( qualitySupported )
	{
		dwQuality = cmpvars.lQ; 
	}
	else
	{
		dwQuality = 0;
	}

	//printf("Quality Setting: %i\n", dwQuality );

	if ( ICCompressBegin( cmpvars.hic, &bmapIn, bmapOut ) != ICERR_OK )
	{
		printf("Error: ICCompressBegin\n");
		icErrCount = 9999;
		return -1;
	}
	
	frameNum   = 0;
	flagsOut   = 0;
	icErrCount = 0;

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
	DWORD reserved = 0;
	int bytesWritten = 0;

	if ( icErrCount > 10 )
	{
		return -1;
	}

	ret = ICCompress( 
		cmpvars.hic, 
		0, 
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
		//printf("Compressing Frame:%i   Size:%i  Flags:%08X\n",
		//		frameNum, bmapOut->biSizeImage, flagsOut );
		bytesWritten = bmapOut->biSizeImage;
		gwavi->add_frame( (unsigned char*)outBuf, bytesWritten, flagsOut );
	}
	else
	{
		printf("Compression Error Frame:%i\n", frameNum);
		icErrCount++;
	}

	return bytesWritten;
}

} // End namespace VFW
#endif
//**************************************************************************************
// LIBAV Interface
#ifdef _USE_LIBAV
namespace LIBAV
{
static  AVFormatContext *oc = NULL;

struct OutputStream
{
	AVStream  *st;
	AVCodecContext *enc;
	AVFrame *frame;
	AVFrame *tmp_frame;
	struct SwsContext *sws_ctx;
	struct SwrContext *swr_ctx;
	int64_t next_pts;
	int      bytesPerSample;
	int      frameSize;
	std::string selEnc;

	OutputStream(void)
	{
		st  = NULL;
		enc = NULL;
		frame = tmp_frame = NULL;
		sws_ctx = NULL;
		swr_ctx = NULL;
		bytesPerSample = 0;
		frameSize = 0;
		next_pts = 0;
	}

	void close(void)
	{
		if ( enc != NULL )
		{
			avcodec_free_context(&enc); enc = NULL;
		}
		if ( frame != NULL )
		{
			av_frame_free(&frame); frame = NULL;
		}
		if ( tmp_frame != NULL )
		{
			av_frame_free(&tmp_frame); tmp_frame = NULL;
		}
		if ( swr_ctx != NULL )
		{
			swr_free(&swr_ctx); swr_ctx = NULL;
		}
		bytesPerSample = 0;
		next_pts = 0;
	}
};
static  OutputStream  video_st;
static  OutputStream  audio_st;

static AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
{
	AVFrame *picture;
	int ret;
	picture = av_frame_alloc();
	if (!picture)
	{
		return NULL;
	}
	picture->format = pix_fmt;
	picture->width  = width;
	picture->height = height;
	picture->pts    = 0;
	//picture->sample_aspect_ratio = (AVRational){ 4, 3 };

	/* allocate the buffers for the frame data */
	ret = av_frame_get_buffer(picture, 0);
	if (ret < 0)
	{
		fprintf(stderr, "Could not allocate frame data.\n");
		return NULL;
	}

	return picture;
}

static int initVideoStream( const char *codec_name, OutputStream *ost )
{
	int ret;
	const AVCodec *codec;
	AVCodecContext *c;
	double fps;
	int fps1000;
	unsigned int usec;

	fps = getBaseFrameRate();

	usec = (unsigned int)((1.0e6 / fps)+0.50);

	fps1000 = (int)(fps * 1000.0);

	/* find the video encoder */
	codec = avcodec_find_encoder_by_name(codec_name);

	if (codec == NULL)
	{
		fprintf(stderr, "codec not found\n");
		return -1;
	}
	printf("CODEC: %s\n", codec->name );

	ost->st = avformat_new_stream(oc, NULL);

	if (ost->st == NULL)
	{
		fprintf(stderr, "Could not alloc stream\n");
		return -1;
	}

	c = avcodec_alloc_context3(codec);

	if (c == NULL)
	{
		fprintf(stderr, "Could not alloc an encoding context\n");
		return -1;
	}
	ost->enc = c;

	av_opt_show2( (void*)c, NULL, AV_OPT_FLAG_VIDEO_PARAM, 0 );

	/* Put sample parameters. */
	c->bit_rate = 400000;
	/* Resolution must be a multiple of two. */
	c->width    = nes_shm->video.ncol;
	c->height   = nes_shm->video.nrow;

	/* timebase: This is the fundamental unit of time (in seconds) in terms
	 * of which frame timestamps are represented. For fixed-fps content,
	 * timebase should be 1/framerate and timestamp increments should be
	 * identical to 1. */
	//ost->st->time_base = (AVRational){ 1000, fps1000 };
	if ( codec->id == AV_CODEC_ID_MPEG4 )
	{   // MPEG4 max num/den is 65535 each
		ost->st->time_base.num = 1000;
		ost->st->time_base.den = fps1000;
	}
	else
	{
		ost->st->time_base.num =    usec;
		ost->st->time_base.den = 1000000u;
	}
	c->time_base       = ost->st->time_base;
	c->gop_size      = 12; /* emit one intra frame every twelve frames at most */
	c->pix_fmt       = AV_PIX_FMT_YUV420P; // Every video encoder seems to accept this

	//c->sample_aspect_ratio =  (AVRational){ 4, 3 };
	//printf("compression_level:%i\n", c->compression_level);
	//printf("TAG:0x%08X\n", c->codec_tag);

	if ( codec->pix_fmts )
	{
		// Auto select least lossy format to comvert to.
		c->pix_fmt = avcodec_find_best_pix_fmt_of_list( codec->pix_fmts, AV_PIX_FMT_BGRA, 0, NULL);

		int i=0, formatOk=0;
		while (codec->pix_fmts[i] != -1)
		{
			printf("Codec PIX_FMT: %i\n", codec->pix_fmts[i]);
			if ( codec->pix_fmts[i] == c->pix_fmt )
			{
				printf("CODEC Supports PIX_FMT:%i\n", c->pix_fmt );
				formatOk = 1;
			}
			i++;
		}
		if ( !formatOk )
		{
			printf("CODEC Does Not Support PIX_FMT:%i  Changing to:%i\n", c->pix_fmt, codec->pix_fmts[0] );
			c->pix_fmt = codec->pix_fmts[0];
		}
	}

	//printf("PIX_FMT:%i\n", c->pix_fmt );

	if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO)
	{
		/* just for testing, we also add B-frames */
		c->max_b_frames = 2;
	}
	if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO)
	{
		/* Needed to avoid using macroblocks in which some coeffs overflow.
		 * This does not happen with normal video, it just happens here as
		 * the motion of the chroma plane does not match the luma plane. */
		c->mb_decision = 2;
	}
	/* Some formats want stream headers to be separate. */
	if (oc->oformat->flags & AVFMT_GLOBALHEADER)
	{
		c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

	/* open the codec */
	if (avcodec_open2(c, NULL, NULL) < 0)
	{
		fprintf(stderr, "could not open codec\n");
		return -1;
	}

	/* Allocate the encoded raw picture. */
	ost->frame = alloc_picture(c->pix_fmt, c->width, c->height);

	if (!ost->frame)
	{
		fprintf(stderr, "Could not allocate picture\n");
		return -1;
	}

	/* If the output format is not YUV420P, then a temporary YUV420P
	 * picture is needed too. It is then converted to the required
	 * output format. */
	ost->tmp_frame = alloc_picture(AV_PIX_FMT_BGRA, c->width, c->height);

	if (ost->tmp_frame == NULL)
	{
		fprintf(stderr, "Could not allocate temporary picture\n");
		return -1;
	}
	
	ost->sws_ctx = sws_getContext(c->width, c->height,
					AV_PIX_FMT_BGRA,
					c->width, c->height,
					c->pix_fmt,
					SWS_BICUBIC, NULL, NULL, NULL);

	/* copy the stream parameters to the muxer */
	ret = avcodec_parameters_from_context(ost->st->codecpar, c);

	if (ret < 0)
	{
		fprintf(stderr, "Could not copy the stream parameters\n");
		return -1;
	}
	return 0;
}

static AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt,
                                  uint64_t channel_layout,
                                  int sample_rate, int nb_samples)
{
	AVFrame *frame = av_frame_alloc();
	int ret;
	if (!frame)
	{
		fprintf(stderr, "Error allocating an audio frame\n");
		return NULL;
	}
	frame->format = sample_fmt;
	frame->channel_layout = channel_layout;
	frame->sample_rate = sample_rate;
	frame->nb_samples = nb_samples;

	if (nb_samples)
	{
		ret = av_frame_get_buffer(frame, 0);
		if (ret < 0)
		{
			fprintf(stderr, "Error allocating an audio buffer\n");
			return NULL;
		}
	}
	return frame;
}

static int initAudioStream( const char *codec_name, OutputStream *ost )
{
	int ret, nb_samples;
	const AVCodec *codec;
	AVCodecContext *c;

	/* find the audio encoder */
	codec = avcodec_find_encoder_by_name(codec_name);

	if (codec == NULL)
	{
		fprintf(stderr, "codec not found: '%s'\n", codec_name);
		return -1;
	}

	ost->st = avformat_new_stream(oc, NULL);

	if (ost->st == NULL)
	{
		fprintf(stderr, "Could not alloc stream\n");
		return -1;
	}

	c = avcodec_alloc_context3(codec);

	if (c == NULL)
	{
		fprintf(stderr, "Could not alloc an encoding context\n");
		return -1;
	}
	ost->enc = c;

	/* put sample parameters */
	c->sample_fmt     = codec->sample_fmts           ? codec->sample_fmts[0]           : AV_SAMPLE_FMT_S16;
	c->sample_rate    = codec->supported_samplerates ? codec->supported_samplerates[0] : audioSampleRate;
	c->channel_layout = codec->channel_layouts       ? codec->channel_layouts[0]       : AV_CH_LAYOUT_STEREO;
	c->channels       = av_get_channel_layout_nb_channels(c->channel_layout);
	c->bit_rate       = 64000;
	//ost->st->time_base = (AVRational){ 1, c->sample_rate };
	ost->st->time_base.num = 1;
	ost->st->time_base.den = c->sample_rate;
	// some formats want stream headers to be separate
	if (oc->oformat->flags & AVFMT_GLOBALHEADER)
	{
		c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}
	/* initialize sample format conversion;
	 * to simplify the code, we always pass the data through lavr, even
	 * if the encoder supports the generated format directly -- the price is
	 * some extra data copying;
	 */
	ost->swr_ctx = swr_alloc();
	if (!ost->swr_ctx)
	{
		fprintf(stderr, "Error allocating the resampling context\n");
		return -1;
	}
	av_opt_set_sample_fmt(ost->swr_ctx, "in_sample_fmt",      AV_SAMPLE_FMT_S16,   0);
	av_opt_set_int(ost->swr_ctx, "in_sample_rate",     audioSampleRate,     0);
	av_opt_set_int(ost->swr_ctx, "in_channel_layout",  AV_CH_LAYOUT_MONO,   0);
	av_opt_set_sample_fmt(ost->swr_ctx, "out_sample_fmt",     c->sample_fmt,       0);
	av_opt_set_int(ost->swr_ctx, "out_sample_rate",    c->sample_rate,      0);
	av_opt_set_int(ost->swr_ctx, "out_channel_layout", c->channel_layout,   0);

	ret = swr_init(ost->swr_ctx);
	if (ret < 0)
	{
		fprintf(stderr, "Error opening the resampling context\n");
		return -1;
	}

	/* open it */
	if (avcodec_open2(c, NULL, NULL) < 0)
	{
		fprintf(stderr, "could not open codec\n");
		return -1;
	}

	if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
	{
		nb_samples = 10000;
	}
	else
	{
		nb_samples = c->frame_size;
	}

	ost->frameSize = nb_samples;
	ost->bytesPerSample  = av_get_bytes_per_sample( c->sample_fmt );

	ost->frame     = alloc_audio_frame(c->sample_fmt, c->channel_layout, c->sample_rate, nb_samples);
	ost->tmp_frame = alloc_audio_frame(c->sample_fmt, c->channel_layout, c->sample_rate, nb_samples);
	//ost->tmp_frame = alloc_audio_frame(AV_SAMPLE_FMT_S16, AV_CH_LAYOUT_MONO, audioSampleRate, nb_samples);

	printf("Audio: FMT:%i  ChanLayout:%li  Rate:%i  FrameSize:%i  bytesPerSample:%i \n",
			c->sample_fmt, c->channel_layout, c->sample_rate, nb_samples, ost->bytesPerSample );

	/* copy the stream parameters to the muxer */
	ret = avcodec_parameters_from_context(ost->st->codecpar, c);
	if (ret < 0)
	{
		fprintf(stderr, "Could not copy the stream parameters\n");
		return -1;
	}
	ost->frame->nb_samples = 0;

	return 0;
}

static void print_Codecs(void)
{
	void *it = NULL;
	const AVCodec *c;

	c = av_codec_iterate( &it );

	while ( c != NULL )
	{
		if ( av_codec_is_encoder(c) )
		{
			if ( c->type == AVMEDIA_TYPE_VIDEO )
			{
				printf("Video Encoder: %i  %s   %s\n", c->id, c->name, c->long_name);
			}
			else if ( c->type == AVMEDIA_TYPE_AUDIO )
			{
				printf("Audio Encoder: %i  %s   %s\n", c->id, c->name, c->long_name);
			}
		}

		c = av_codec_iterate( &it );
	}
}

static int setCodecFromConfig(void)
{
	std::string s;
	const AVCodec *c;
#if  LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT( 59, 0, 0 )
	const AVOutputFormat *fmt;
#else
	AVOutputFormat *fmt;
#endif

	fmt = av_guess_format("avi", NULL, NULL);

	g_config->getOption("SDL.AviFFmpegVideoCodec", &s);	

	if ( s.size() > 0 )
	{
		c = avcodec_find_encoder_by_name(s.c_str());

		if ( c )
		{
			video_st.selEnc = c->name;	
		}
	}

	g_config->getOption("SDL.AviFFmpegAudioCodec", &s);	

	if ( s.size() > 0 )
	{
		c = avcodec_find_encoder_by_name(s.c_str());

		if ( c )
		{
			audio_st.selEnc = c->name;	
		}
	}

	if ( fmt )
	{
		if ( video_st.selEnc.size() == 0 )
		{
			c = avcodec_find_encoder( fmt->video_codec );

			if ( c )
			{
				video_st.selEnc = c->name;
			}
		}
		if ( audio_st.selEnc.size() == 0 )
		{
			c = avcodec_find_encoder( fmt->audio_codec );

			if ( c )
			{
				audio_st.selEnc = c->name;
			}
		}
	}
	return 0;
}

static int initMedia( const char *filename )
{

#if  LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT( 59, 0, 0 )
	const AVOutputFormat *fmt;
#else
	AVOutputFormat *fmt;
#endif
	/* Initialize libavcodec, and register all codecs and formats. */
	//av_register_all();

	print_Codecs();

	/* Autodetect the output format from the name. default is MPEG. */
	fmt = av_guess_format(NULL, filename, NULL);

	if (fmt == NULL)
	{
		printf("Could not deduce output format from file extension: using AVI.\n");
		fmt = av_guess_format("avi", NULL, NULL);
	}
	if (fmt == NULL)
	{
		fprintf(stderr, "Could not find suitable output format\n");
		return -1;
	}

	/* Allocate the output media context. */
	oc = avformat_alloc_context();
	if (oc == NULL)
	{
		fprintf(stderr, "Memory error\n");
		return -1;
	}
	oc->oformat = fmt;

	setCodecFromConfig();

	if ( initVideoStream( video_st.selEnc.c_str(), &video_st ) )
	{
		printf("Video Stream Init Failed\n");
		return -1;
	}
	if ( initAudioStream( audio_st.selEnc.c_str(), &audio_st ) )
	{
		printf("Audio Stream Init Failed\n");
		return -1;
	}

	av_dump_format(oc, 0, filename, 1);

	/* open the output file, if needed */
	if ( !(fmt->flags & AVFMT_NOFILE))
	{
		if (avio_open( &oc->pb, filename, AVIO_FLAG_WRITE) < 0)
		{
			fprintf(stderr, "Could not open '%s'\n", filename);
			return -1;
		}
		else
		{
			printf("Opened: %s\n", filename);
		}
	}

	/* Write the stream header, if any. */
	if ( avformat_write_header(oc, NULL) )
	{
		printf("Error: Failed to write avformat header\n");
		return -1;
	}

	return 0;
}

static int init( int width, int height )
{
	return 0;
}

static int write_audio_frame( AVFrame *frame )
{
	int ret;
	OutputStream *ost = &audio_st;

	if ( frame )
	{
		frame->pts        = ost->next_pts;
		ost->next_pts    += frame->nb_samples;
	}
	//printf("Encoding Audio: %i\n", frame->nb_samples );

	ret = avcodec_send_frame(ost->enc, frame);

	if (ret < 0)
	{
		fprintf(stderr, "Error submitting a frame for encoding\n");
		return -1;
	}
	while (ret >= 0)
	{
		AVPacket pkt = { 0 }; // data and size must be 0;
		av_init_packet(&pkt);
		ret = avcodec_receive_packet(ost->enc, &pkt);
		if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
		{
			fprintf(stderr, "Error encoding a video frame\n");
			return -1;
		}
		else if (ret >= 0)
		{
			av_packet_rescale_ts(&pkt, ost->enc->time_base, ost->st->time_base);
			pkt.stream_index = ost->st->index;
			/* Write the compressed frame to the media file. */
			ret = av_interleaved_write_frame(oc, &pkt);
			if (ret < 0)
			{
				fprintf(stderr, "Error while writing video frame\n");
				return -1;
			}
		}
	}
	return 0;
}

static int encode_audio_frame( int16_t *audioOut, int numSamples)
{
	int i,ret;
	OutputStream *ost = &audio_st;
	const uint8_t *inData[AV_NUM_DATA_POINTERS];
	
	for (i=0; i<AV_NUM_DATA_POINTERS; i++)
	{
		inData[i] = 0;
	}

	if ( audioOut )
	{
		inData[0] = (const uint8_t*)audioOut;

		//printf("Audio NumSamplesIn: %i\n", numSamples);
	}

	ret = av_frame_make_writable(ost->tmp_frame);

	if (ret < 0)
	{
	    return -1;
	}
	
	if ( audioOut )
	{
		ret = swr_convert( ost->swr_ctx, ost->tmp_frame->data, ost->tmp_frame->linesize[0], inData, numSamples );
	}
	else
	{
		ret = swr_convert( ost->swr_ctx, ost->tmp_frame->data, ost->tmp_frame->linesize[0], NULL, 0 );
	}

	if (ret < 0)
	{
		fprintf(stderr, "Error feeding audio data to the resampler\n");
		return -1;
	}
	if (ret > 0)
	{
		int spaceAvail, samplesLeft, copySize, srcOffset = 0, frameSize;

		frameSize = ost->frameSize;

		samplesLeft = ost->tmp_frame->nb_samples = ret;

		spaceAvail  = frameSize - ost->frame->nb_samples;

		while ( samplesLeft > 0 )
		{
			if ( spaceAvail >= samplesLeft )
			{
				copySize = samplesLeft;
			}
			else
			{
				copySize = spaceAvail;
			}
			//printf("Audio: Space:%i  Data:%i  Copy:%i\n", spaceAvail, samplesLeft, copySize );

			ret = av_frame_make_writable(ost->frame);

			if (ret < 0)
			{
				fprintf(stderr, "Error audio av_frame_make_writable\n");
				return -1;
			}

			ret = av_samples_copy( ost->frame->data, ost->tmp_frame->data,
					ost->frame->nb_samples, srcOffset, copySize, 
						ost->frame->channels, ost->enc->sample_fmt );

			if ( ret < 0 )
			{
				return -1;
			}
			ost->frame->nb_samples += copySize;
			srcOffset              += copySize;
			samplesLeft            -= copySize;

			if ( ost->frame->nb_samples >= frameSize )
			{
				if ( write_audio_frame( ost->frame ) )
				{
					return -1;
				}
				ost->frame->nb_samples = 0;
			}
			spaceAvail = frameSize - ost->frame->nb_samples;
		}
	}

	if ( audioOut == NULL )
	{
		if ( ost->frame->nb_samples > 0 )
		{
			if ( write_audio_frame( ost->frame ) )
			{
				return -1;
			}
			ost->frame->nb_samples = 0;
		}
		if ( write_audio_frame( NULL ) )
		{
			return -1;
		}
	}
	return 0;
}

static int encode_video_frame( unsigned char *inBuf )
{
	int ret, y, ofs, inLineSize;
	OutputStream *ost = &video_st;
	AVCodecContext *c = video_st.enc;
	unsigned char *outBuf;

	ret = av_frame_make_writable( video_st.frame );

	if ( ret < 0 )
	{
		return -1;
	}

	ofs = 0;

	inLineSize = c->width * 4;

	outBuf = ost->tmp_frame->data[0];

	for (y=0; y < c->height; y++)
	{
		memcpy( outBuf, &inBuf[ofs], inLineSize ); ofs += inLineSize;

		outBuf += ost->tmp_frame->linesize[0];
	}

	sws_scale(ost->sws_ctx, (const uint8_t * const *) ost->tmp_frame->data,
			ost->tmp_frame->linesize, 0, c->height, ost->frame->data,
			ost->frame->linesize);

	video_st.frame->pts = video_st.next_pts++;
	
	/* encode the image */
	ret = avcodec_send_frame(c, video_st.frame);
	if (ret < 0)
	{
		fprintf(stderr, "Error submitting a frame for encoding\n");
		return -1;
	}

	while (ret >= 0)
	{
		AVPacket pkt = { 0 };

		av_init_packet(&pkt);

		ret = avcodec_receive_packet(c, &pkt);

		if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
		{
			fprintf(stderr, "Error encoding a video frame\n");
			return -1;
		}
		else if (ret >= 0)
		{
			av_packet_rescale_ts(&pkt, c->time_base, video_st.st->time_base);
			pkt.stream_index = video_st.st->index;
			/* Write the compressed frame to the media file. */
			ret = av_interleaved_write_frame(oc, &pkt);
			if (ret < 0)
			{
				fprintf(stderr, "Error while writing video frame\n");
				return -1;
			}
		}
	}

	return ret == AVERROR_EOF;
}

static int close(void)
{
	encode_audio_frame( NULL, 0 );

	/* Write the trailer, if any. The trailer must be written before you
	 * close the CodecContexts open when you wrote the header; otherwise
	 * av_write_trailer() may try to use memory that was freed on
	 * av_codec_close(). */
	av_write_trailer(oc);

	video_st.close();
	audio_st.close();

	/* Close the output file. */
	avio_close(oc->pb);

	/* free the stream */
	avformat_free_context(oc);

	oc = NULL;

	return 0;
}

} // End namespace LIBAV
#endif
//**************************************************************************************
int aviRecordOpenFile( const char *filepath )
{
	char fourcc[8];
	gwavi_audio_t  audioConfig;
	double fps;
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

#ifdef WIN32
	if ( videoFormat == AVI_VFW )
	{
		if ( VFW::chooseConfig( nes_shm->video.ncol, nes_shm->video.nrow ) )
		{
			return -1;
		}
	}
#endif

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
	#ifdef _USE_X265
	else if ( videoFormat == AVI_X265 )
	{
		strcpy( fourcc, "H265");
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

#ifdef _USE_LIBAV
	if ( videoFormat == AVI_LIBAV )
	{
		if ( LIBAV::initMedia( fileName ) )
		{
			printf("Error: Failed to open AVI file.\n");
			recordEnable = false;
			return -1;
		}
	}
	else
#endif
	{
		gwavi = new gwavi_t();

		if ( gwavi->open( fileName, nes_shm->video.ncol, nes_shm->video.nrow, fourcc, fps, &audioConfig ) )
		{
			printf("Error: Failed to open AVI file.\n");
			recordEnable = false;
			return -1;
		}
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

	//if ( gwavi == NULL )
	//{
	//	return -1;
	//}
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

	//if ( gwavi == NULL )
	//{
	//	return -1;
	//}
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
int aviDebugOpenFile( const char *filepath )
{
	gwavi_t inAvi;

	if ( inAvi.openIn( filepath ) )
	{
		printf("Failed to open AVI File: '%s'\n", filepath);
		return -1;
	}

	inAvi.printHeaders();

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
			#ifdef _USE_X265
			case AVI_X265:
				s.assign("X265 (H.265)");
			break;
			#endif
			#ifdef _USE_LIBAV
			case AVI_LIBAV:
				s.assign("libav (ffmpeg)");
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
#ifdef _USE_X265
	if ( localVideoFormat == AVI_X265)
	{
		X265::init( width, height );
	}
#endif
#ifdef _USE_LIBAV
	if ( localVideoFormat == AVI_LIBAV)
	{
		LIBAV::init( width, height );
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
			#ifdef _USE_X265
			else if ( localVideoFormat == AVI_X265)
			{
				Convert_4byte_To_I420Frame<4>(videoOut,rgb24,numPixels,width);
				X265::encode_frame( rgb24, width, height );
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
			#ifdef _USE_LIBAV
			else if ( localVideoFormat == AVI_LIBAV)
			{
				//Convert_4byte_To_I420Frame<4>(videoOut,rgb24,numPixels,width);
				//convertRgb_32_to_24( (const unsigned char*)videoOut, rgb24,
				//		width, height, numPixels, true );
				//LIBAV::encode_video_frame( rgb24 );
				LIBAV::encode_video_frame( (unsigned char*)videoOut );
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
					#ifdef _USE_LIBAV
					if ( localVideoFormat == AVI_LIBAV)
					{
						LIBAV::encode_audio_frame( audioOut, numSamples );
					}
					else
					#endif
					{
						gwavi->add_audio( (unsigned char *)audioOut, numSamples*2);
					}

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
#ifdef _USE_X265
	if ( localVideoFormat == AVI_X265)
	{
		X265::close();
	}
#endif
#ifdef _USE_LIBAV
	if ( localVideoFormat == AVI_LIBAV)
	{
		LIBAV::close();
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
//**************************************************************************************
//*****************************  Options Pages *****************************************
//**************************************************************************************
//**************************************************************************************
#ifdef _USE_LIBAV
LibavOptionsPage::LibavOptionsPage(QWidget *parent)
	: QWidget(parent)
{
	QLabel *lbl;
	QVBoxLayout *vbox, *vbox1;
	QHBoxLayout *hbox;

	LIBAV::setCodecFromConfig();

	vbox1 = new QVBoxLayout();

	videoGbox = new QGroupBox( tr("Video:") );
	audioGbox = new QGroupBox( tr("Audio:") );

	audioGbox->setCheckable(true);

	videoEncSel = new QComboBox();
	audioEncSel = new QComboBox();

	vbox1->addWidget( videoGbox );
	vbox1->addWidget( audioGbox );

	vbox = new QVBoxLayout();
	videoGbox->setLayout(vbox);

	hbox = new QHBoxLayout();
	vbox->addLayout(hbox);
	lbl  = new QLabel( tr("Encoder:") );
	hbox->addWidget( lbl, 1 );
	hbox->addWidget( videoEncSel, 5 );

	vbox = new QVBoxLayout();
	audioGbox->setLayout(vbox);

	hbox  = new QHBoxLayout();
	vbox->addLayout(hbox);
	lbl  = new QLabel( tr("Encoder:") );
	hbox->addWidget( lbl, 1 );
	hbox->addWidget( audioEncSel, 5 );

	initCodecLists();

	setLayout(vbox1);

	connect(videoEncSel, SIGNAL(currentIndexChanged(int)), this, SLOT(videoCodecChanged(int)));
	connect(audioEncSel, SIGNAL(currentIndexChanged(int)), this, SLOT(audioCodecChanged(int)));
}
//-----------------------------------------------------
LibavOptionsPage::~LibavOptionsPage(void)
{

}
//-----------------------------------------------------
void LibavOptionsPage::initPixelFormatSelect(const char *codec_name)
{
	const AVCodec *c;
	const AVPixFmtDescriptor *desc;

	c = avcodec_find_encoder_by_name( codec_name );

	if ( c == NULL )
	{
		return;
	}
	if ( c->pix_fmts )
	{
		int i=0, formatOk=0;
		while (c->pix_fmts[i] != -1)
		{
			desc = av_pix_fmt_desc_get( c->pix_fmts[i] );

			if ( desc )
			{
				printf("Codec PIX_FMT: %i: %s 0x%04X\t-  %s\n", c->pix_fmts[i],
						desc->name, av_get_pix_fmt_loss(c->pix_fmts[i], AV_PIX_FMT_BGRA, 0), desc->alias);
			}
			i++;
		}
		//if ( !formatOk )
		//{
		//	printf("CODEC Does Not Support PIX_FMT:%i  Changing to:%i\n", c->pix_fmt, codec->pix_fmts[0] );
		//	c->pix_fmt = codec->pix_fmts[0];
		//}
	}
	else
	{
		desc = av_pix_fmt_desc_next( NULL );

		while ( desc != NULL )
		{
			AVPixelFormat pf;

			pf = av_pix_fmt_desc_get_id(desc);

			printf("Codec PIX_FMT: %i: %s  0x%04X\t-  %s\n", 
					pf, desc->name, av_get_pix_fmt_loss(pf, AV_PIX_FMT_BGRA, 0), desc->alias);

			desc = av_pix_fmt_desc_next( desc );
		}
	}

}
//-----------------------------------------------------
void LibavOptionsPage::initCodecLists(void)
{
	void *it = NULL;
	const AVCodec *c;
	const AVOutputFormat *ofmt;
	int compatible;

	ofmt = av_guess_format("avi", NULL, NULL);

	c = av_codec_iterate( &it );

	while ( c != NULL )
	{
		if ( av_codec_is_encoder(c) )
		{
			if ( c->type == AVMEDIA_TYPE_VIDEO )
			{
				compatible = avformat_query_codec( ofmt, c->id, FF_COMPLIANCE_NORMAL );
				printf("Video Encoder: %i  %s   %s\t:%i\n", c->id, c->name, c->long_name, compatible);
				if ( compatible )
				{
					videoEncSel->addItem( tr(c->name), c->id );

					if ( strcmp( LIBAV::video_st.selEnc.c_str(), c->name ) == 0 )
					{
						videoEncSel->setCurrentIndex( videoEncSel->count() - 1 );
					}
				}
			}
			else if ( c->type == AVMEDIA_TYPE_AUDIO )
			{
				compatible = avformat_query_codec( ofmt, c->id, FF_COMPLIANCE_NORMAL );
				printf("Audio Encoder: %i  %s   %s\t:%i\n", c->id, c->name, c->long_name, compatible);
				if ( compatible )
				{
					audioEncSel->addItem( tr(c->name), c->id );

					if ( strcmp( LIBAV::audio_st.selEnc.c_str(), c->name ) == 0 )
					{
						audioEncSel->setCurrentIndex( audioEncSel->count() - 1 );
					}
				}
			}
		}

		c = av_codec_iterate( &it );
	}

	initPixelFormatSelect( videoEncSel->currentText().toStdString().c_str() );
}
//-----------------------------------------------------
void LibavOptionsPage::videoCodecChanged(int idx)
{
	const AVCodec *c;

	LIBAV::video_st.selEnc = videoEncSel->currentText().toStdString().c_str();

	c = avcodec_find_encoder_by_name( LIBAV::video_st.selEnc.c_str() );

	if ( c )
	{
		g_config->setOption("SDL.AviFFmpegVideoCodec", c->name);	
		initPixelFormatSelect( c->name );
	}
}
//-----------------------------------------------------
void LibavOptionsPage::audioCodecChanged(int idx)
{
	const AVCodec *c;

	LIBAV::audio_st.selEnc = audioEncSel->currentText().toStdString().c_str();

	c = avcodec_find_encoder_by_name( LIBAV::audio_st.selEnc.c_str() );

	if ( c )
	{
		g_config->setOption("SDL.AviFFmpegAudioCodec", c->name);	
	}
}
//-----------------------------------------------------
#endif
//**************************************************************************************
