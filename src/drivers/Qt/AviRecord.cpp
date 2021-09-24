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
#include <stdarg.h>
#include <string.h>
#include <string>

#ifdef WIN32
#include <windows.h>
#include <vfw.h>
#endif

#include <QObject>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>

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

#define  AV_LOG_FILE_NAME  "fceuxAV.log"

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
static int       aviDriver = 0;
static int       videoFormat = AVI_RGB24;
static int       audioSampleRate = 48000;
static FILE     *avLogFp = NULL;
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
	int      pixelFormat;
	int      sampleFormat;
	int      sampleRate;
	int      chanLayout;
	bool     isAudio;
	bool     writeError;
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
		writeError = false;
		isAudio = false;
		pixelFormat = -1;
		sampleFormat = -1;
		sampleRate = -1;
		chanLayout = -1;
	}

	void close(void)
	{
		if ( writeError )
		{
			char msg[512];
			sprintf( msg, "%s Stream Write Errors Detected.\nOutput may be incomplete or corrupt.\nSee log file '%s' for details\n",
				       isAudio ? "Audio" : "Video", AV_LOG_FILE_NAME);
			FCEUD_PrintError(msg);
		}
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
		if ( sws_ctx != NULL )
		{
			sws_freeContext(sws_ctx); sws_ctx = NULL;
		}
		if ( swr_ctx != NULL )
		{
			swr_free(&swr_ctx); swr_ctx = NULL;
		}
		st = NULL;
		writeError = false;
		bytesPerSample = 0;
		next_pts = 0;
	}
};
static  OutputStream  video_st;
static  OutputStream  audio_st;

static void log_callback( void *avcl, int level, const char *fmt, va_list vl)
{
	if ( avLogFp != NULL )
	{
		va_list vl2;

		va_copy( vl2, vl );

		vfprintf( avLogFp, fmt, vl2 );
	}

	av_log_default_callback( avcl, level, fmt, vl );

	//FCEUD_Message( stmp );
}

int loadCodecConfig( int type, const char *codec_name, AVCodecContext *ctx)
{
	int i,j;
	char filename[512];
	char line[512];
	char section[256], id[256], val[256];
	void *obj, *child;
	FILE *fp;
	const char *baseDir = FCEUI_GetBaseDirectory();

	sprintf( filename, "%s/avi/%s.conf", baseDir, codec_name );

	fp = fopen( filename, "r");

	if ( fp == NULL )
	{
		printf("Error: Failed to open file '%s' for reading\n", filename );
		return -1;
	}
	section[0] = 0;

	obj = ctx;

	while ( fgets( line, sizeof(line), fp ) != 0 )
	{
		i=0;

		while ( line[i] != 0 )
		{
			if ( line[i] == '#' )
			{
				line[i] = 0; break;
			}
			i++;
		}
		i=0;

		while ( isspace(line[i]) ) i++;

		if ( line[i] == '[' )
		{
			i++;
			while ( isspace(line[i]) ) i++;

			j=0;
			while ( (line[i] != 0) && (line[i] != ']') )
			{
				section[j] = line[i]; i++; j++;
			}
			section[j] = 0; j--;
			
			while ( (j >= 0) && isspace(section[j]) )
			{
				section[j] = 0; j--;
			}
			//printf("Section: '%s'\n", section);

			continue;
		}

		j=0;
		while ( (line[i] != 0) && (line[i] != '=') )
		{
			id[j] = line[i]; i++; j++;
		}
		id[j] = 0; j--;
		
		while ( (j >= 0) && isspace(id[j]) )
		{
			id[j] = 0; j--;
		}
		//printf("ID: '%s'\n", id);

		if (id[0] == 0)
		{
			continue;
		}

		if (line[i] != '=')
		{
			continue;
		}
		i++;

		while ( isspace(line[i]) ) i++;

		j=0;
		while ( (line[i] != 0) )
		{
			val[j] = line[i]; i++; j++;
		}
		val[j] = 0; j--;
		
		while ( (j >= 0) && isspace(val[j]) )
		{
			val[j] = 0; j--;
		}
		//printf("VAL: '%s'\n", val);

		if ( section[0] == 0 )
		{
			continue;
		}
		//if ( val[0] == 0 )
		//{
		//	continue;
		//}
		printf("'%s.%s' = '%s'\n", section, id, val);

		obj = ctx;
		child = NULL;

		while ( obj != NULL )
		{
			const char *groupName = (*static_cast<AVClass**>(obj))->class_name;

			if ( strcmp( groupName, section ) == 0 )
			{
				break;
			}
			obj = child = av_opt_child_next( ctx, child );
		}

		if ( obj != NULL )
		{
			if ( av_opt_set( obj, id, val, 0 ) < 0 )
			{
				printf("Error: Failed to set option %s.%s = %s\n", section, id, val );
			}
		}
	}
	fclose(fp);

	return 0;
}

int saveCodecConfig( int type, const char *codec_name, AVCodecContext *ctx)
{
	void *obj, *child = NULL;
	FILE *fp;
	uint8_t *str;
	char filename[512];
	const AVOption *opt;
	bool useOpt;
	const char *baseDir = FCEUI_GetBaseDirectory();

	sprintf( filename, "%s/avi/%s.conf", baseDir, codec_name );

	fp = fopen( filename, "w");

	if ( fp == NULL )
	{
		printf("Error: Failed to open file '%s' for writing\n", filename );
		return -1;
	}

	obj = ctx;

	while ( obj != NULL )
	{
		const char *groupName = (*static_cast<AVClass**>(obj))->class_name;

		fprintf( fp, "\n[ %s ]\n", groupName );

		//printf("OBJ Class: %s\n", groupName);

		opt = av_opt_next( obj, NULL );

		while ( opt != NULL )
		{
			useOpt = (opt->name != NULL) &&
					(opt->type != AV_OPT_TYPE_BINARY) &&
					(opt->type != AV_OPT_TYPE_DICT);

			if ( type )
			{
				useOpt = useOpt &&
						(opt->flags & AV_OPT_FLAG_ENCODING_PARAM) &&
						(opt->flags & AV_OPT_FLAG_AUDIO_PARAM);
			}
			else
			{
				useOpt = useOpt &&
						(opt->flags & AV_OPT_FLAG_ENCODING_PARAM) &&
						(opt->flags & AV_OPT_FLAG_VIDEO_PARAM);
			}

			if ( useOpt )
			{
				str = NULL;

				av_opt_get( obj, opt->name, 0, &str);

				if ( str )
				{
					if ( av_opt_is_set_to_default( obj, opt ) == 0 )
					{
						fprintf( fp, "%s=%s   # %s\n", opt->name, str, 
								opt->help ? opt->help : "" );
					}
					av_free(str); str = NULL;
				}
			}
			opt = av_opt_next( obj, opt );
		}
		obj = child = av_opt_child_next( ctx, child );
	}
	fclose(fp);

	return 0;
}

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
		fprintf( avLogFp, "Video codec not found: %s\n", codec_name);
		return -1;
	}
	//printf("CODEC: %s\n", codec->name );

	ost->st = avformat_new_stream(oc, NULL);

	if (ost->st == NULL)
	{
		fprintf( avLogFp, "Error: Could not alloc video stream\n");
		return -1;
	}

	c = avcodec_alloc_context3(codec);

	if (c == NULL)
	{
		fprintf( avLogFp, "Error: Could not alloc an video encoding context\n");
		return -1;
	}
	/* Put sample parameters. */
	c->bit_rate = 400000;
	c->gop_size = 12; /* emit one intra frame every twelve frames at most */

	loadCodecConfig( 0, codec_name, c );

	ost->enc = c;

	//av_opt_show2( (void*)c, NULL, AV_OPT_FLAG_VIDEO_PARAM, 0 );

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
	//c->pix_fmt       = AV_PIX_FMT_YUV420P; // Every video encoder seems to accept this
	c->pix_fmt       = (AVPixelFormat)ost->pixelFormat;

	//c->sample_aspect_ratio =  (AVRational){ 4, 3 };
	//printf("compression_level:%i\n", c->compression_level);
	//printf("TAG:0x%08X\n", c->codec_tag);

	if ( codec->pix_fmts )
	{
		if ( ost->pixelFormat == -1 )
		{
			// Auto select least lossy format to comvert to.
			c->pix_fmt = avcodec_find_best_pix_fmt_of_list( codec->pix_fmts, AV_PIX_FMT_BGRA, 0, NULL);
		}

		int i=0, formatOk=0;
		while (codec->pix_fmts[i] != -1)
		{
			//printf("Codec PIX_FMT: %i\n", codec->pix_fmts[i]);
			if ( codec->pix_fmts[i] == c->pix_fmt )
			{
				printf("CODEC Supports PIX_FMT:%i\n", c->pix_fmt );
				formatOk = 1;
			}
			i++;
		}
		if ( !formatOk )
		{
			printf("CODEC Does Not Support PIX_FMT:%i\n", c->pix_fmt);

			c->pix_fmt = avcodec_find_best_pix_fmt_of_list( codec->pix_fmts, AV_PIX_FMT_BGRA, 0, NULL);

			printf("Changing to:%i\n", c->pix_fmt);
		}
	}
	else
	{
		if ( ost->pixelFormat == -1 )
		{
			c->pix_fmt = AV_PIX_FMT_YUV420P; // Every video encoder seems to accept this
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
		fprintf( avLogFp, "Error: Could not open codec: %s\n", codec_name);
		return -1;
	}

	/* Allocate the encoded raw picture. */
	ost->frame = alloc_picture(c->pix_fmt, c->width, c->height);

	if (!ost->frame)
	{
		fprintf( avLogFp, "Error: Could not allocate picture\n");
		return -1;
	}

	/* If the output format is not YUV420P, then a temporary YUV420P
	 * picture is needed too. It is then converted to the required
	 * output format. */
	ost->tmp_frame = alloc_picture(AV_PIX_FMT_BGRA, c->width, c->height);

	if (ost->tmp_frame == NULL)
	{
		fprintf( avLogFp, "Error: Could not allocate temporary picture\n");
		return -1;
	}
	
	ost->sws_ctx = sws_getContext(c->width, c->height,
					AV_PIX_FMT_BGRA,
					c->width, c->height,
					c->pix_fmt,
					SWS_BICUBIC, NULL, NULL, NULL);

	if ( ost->sws_ctx == NULL )
	{
		fprintf( avLogFp, "Error: Video sws_getContext Failed. Video conversion not possible\n");
		return -1;
	}

	/* copy the stream parameters to the muxer */
	ret = avcodec_parameters_from_context(ost->st->codecpar, c);

	if (ret < 0)
	{
		fprintf( avLogFp, "Error: Video avcodec_parameters_from_context Failed. Could not copy the stream parameters\n");
		return -1;
	}
	ost->writeError = false;

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

	ost->isAudio = true;

	/* find the audio encoder */
	codec = avcodec_find_encoder_by_name(codec_name);

	if (codec == NULL)
	{
		fprintf( avLogFp, "Audio codec not found: '%s'\n", codec_name);
		return -1;
	}

	ost->st = avformat_new_stream(oc, NULL);

	if (ost->st == NULL)
	{
		fprintf( avLogFp, "Could not alloc audio stream\n");
		return -1;
	}

	c = avcodec_alloc_context3(codec);

	if (c == NULL)
	{
		fprintf( avLogFp, "Could not alloc an audio encoding context\n");
		return -1;
	}
	loadCodecConfig( 1, codec_name, c );

	ost->enc = c;

	/* put sample parameters */

	// Sample Format Selection
	if ( ost->sampleFormat > 0 )
	{
		c->sample_fmt  = (AVSampleFormat)ost->sampleFormat;

		if ( codec->sample_fmts )
		{
			int i=0, formatOk=0;
			while ( codec->sample_fmts[i] != -1 )
			{
				if ( c->sample_fmt == codec->sample_fmts[i] )
				{
					formatOk = true; break;
				}
				i++;
			}
			if ( !formatOk )
			{
				c->sample_fmt = codec->sample_fmts  ? codec->sample_fmts[0] : AV_SAMPLE_FMT_S16;
			}
		}
	}
	else
	{
		c->sample_fmt = codec->sample_fmts ? codec->sample_fmts[0] : AV_SAMPLE_FMT_S16;
	}

	// Sample Rate Selection
	if ( ost->sampleRate > 0 )
	{
		c->sample_rate = ost->sampleRate;

		if ( codec->supported_samplerates )
		{
			int i=0, formatOk=0;
			while ( codec->supported_samplerates[i] != 0 )
			{
				if ( c->sample_rate == codec->supported_samplerates[i] )
				{
					formatOk = true; break;
				}
				i++;
			}
			if ( !formatOk )
			{
				c->sample_rate    = codec->supported_samplerates ? codec->supported_samplerates[0] : audioSampleRate;
			}
		}
	}
	else
	{
		c->sample_rate    = codec->supported_samplerates ? codec->supported_samplerates[0] : audioSampleRate;
	}

	// Channel Layout Selection
	if ( ost->chanLayout > 0 )
	{
		c->channel_layout = ost->chanLayout;

		if ( codec->channel_layouts )
		{
			int i=0, formatOk=0;
			while ( codec->channel_layouts[i] != 0 )
			{
				if ( c->channel_layout == codec->channel_layouts[i] )
				{
					formatOk = true; break;
				}
				i++;
			}
			if ( !formatOk )
			{
				c->channel_layout = codec->channel_layouts       ? codec->channel_layouts[0]       : AV_CH_LAYOUT_STEREO;
			}
		}
	}
	else
	{
		c->channel_layout = codec->channel_layouts       ? codec->channel_layouts[0]       : AV_CH_LAYOUT_STEREO;
	}
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
		fprintf( avLogFp, "Error allocating the audio resampling context\n");
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
		fprintf( avLogFp, "Error: Could not init the audio resampling context\n");
		return -1;
	}

	/* open it */
	if (avcodec_open2(c, NULL, NULL) < 0)
	{
		fprintf( avLogFp, "Error: Could not open codec: %s\n", codec_name);
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

	//printf("Audio: FMT:%i  ChanLayout:%li  Rate:%i  FrameSize:%i  bytesPerSample:%i \n",
	//		c->sample_fmt, c->channel_layout, c->sample_rate, nb_samples, ost->bytesPerSample );

	/* copy the stream parameters to the muxer */
	ret = avcodec_parameters_from_context(ost->st->codecpar, c);
	if (ret < 0)
	{
		fprintf( avLogFp, "Error: Audio avcodec_parameters_from_context Failed. Could not copy the stream parameters\n");
		return -1;
	}
	ost->frame->nb_samples = 0;
	ost->writeError = false;

	return 0;
}

//static void print_Codecs(void)
//{
//	void *it = NULL;
//	const AVCodec *c;
//
//	c = av_codec_iterate( &it );
//
//	while ( c != NULL )
//	{
//		if ( av_codec_is_encoder(c) )
//		{
//			if ( c->type == AVMEDIA_TYPE_VIDEO )
//			{
//				printf("Video Encoder: %i  %s   %s\n", c->id, c->name, c->long_name);
//			}
//			else if ( c->type == AVMEDIA_TYPE_AUDIO )
//			{
//				printf("Audio Encoder: %i  %s   %s\n", c->id, c->name, c->long_name);
//			}
//		}
//
//		c = av_codec_iterate( &it );
//	}
//}

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

	g_config->getOption("SDL.AviFFmpegVideoPixFmt"    , &video_st.pixelFormat);	
	g_config->getOption("SDL.AviFFmpegAudioSmpFmt"    , &audio_st.sampleFormat);	
	g_config->getOption("SDL.AviFFmpegAudioSmpRate"   , &audio_st.sampleRate);	
	g_config->getOption("SDL.AviFFmpegAudioChanLayout", &audio_st.chanLayout);	

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

	av_log_set_callback( log_callback );

	//print_Codecs();

	/* Autodetect the output format from the name. default is MPEG. */
	fmt = av_guess_format(NULL, filename, NULL);

	if (fmt == NULL)
	{
		fprintf( avLogFp, "Could not deduce output format from file extension: using AVI.\n");
		fmt = av_guess_format("avi", NULL, NULL);
	}
	if (fmt == NULL)
	{
		fprintf( avLogFp, "Error: Could not find suitable output format\n");
		goto LIBAV_INIT_MEDIA_ERROR_EXIT;
	}

	/* Allocate the output media context. */
	oc = avformat_alloc_context();
	if (oc == NULL)
	{
		fprintf( avLogFp, "Memory error: avformat_alloc_context failed\n");
		goto LIBAV_INIT_MEDIA_ERROR_EXIT;
	}
	oc->oformat = fmt;

	setCodecFromConfig();

	if ( initVideoStream( video_st.selEnc.c_str(), &video_st ) )
	{
		fprintf( avLogFp, "Video Stream Init Failed\n");
		goto LIBAV_INIT_MEDIA_ERROR_EXIT;
	}
	if ( recordAudio )
	{
		if ( initAudioStream( audio_st.selEnc.c_str(), &audio_st ) )
		{
			fprintf( avLogFp, "Audio Stream Init Failed\n");
			goto LIBAV_INIT_MEDIA_ERROR_EXIT;
		}
	}

	av_dump_format(oc, 0, filename, 1);

	/* open the output file, if needed */
	if ( !(fmt->flags & AVFMT_NOFILE))
	{
		if (avio_open( &oc->pb, filename, AVIO_FLAG_WRITE) < 0)
		{
			fprintf( avLogFp, "Error: Could not open file: '%s'\n", filename);
			goto LIBAV_INIT_MEDIA_ERROR_EXIT;
		}
		else
		{
			fprintf( avLogFp, "Opened file for writing: %s\n", filename);
		}
	}

	/* Write the stream header, if any. */
	if ( avformat_write_header(oc, NULL) )
	{
		fprintf( avLogFp, "Error: avformat_write_header Failed: Could not write avformat header\n");
		goto LIBAV_INIT_MEDIA_ERROR_EXIT;
	}

	return 0;

LIBAV_INIT_MEDIA_ERROR_EXIT:

	video_st.close();
	audio_st.close();

	/* Close the output file. */

	/* free the stream */
	if ( oc )
	{
		avio_close(oc->pb);

		avformat_free_context(oc); oc = NULL;
	}
	return -1;
}

static int init( int width, int height )
{
	return 0;
}

static int write_audio_frame( AVFrame *frame )
{
	int ret;
	OutputStream *ost = &audio_st;

	if ( ost->writeError )
	{
		return -1;
	}
	if ( frame )
	{
		frame->pts        = ost->next_pts;
		ost->next_pts    += frame->nb_samples;
	}
	//printf("Encoding Audio: %i\n", frame->nb_samples );

	ret = avcodec_send_frame(ost->enc, frame);

	if (ret < 0)
	{
		fprintf( avLogFp, "Error submitting audio frame for encoding\n");
		ost->writeError = true;
		return -1;
	}
	while (ret >= 0)
	{
		AVPacket pkt = { 0 }; // data and size must be 0;
		av_init_packet(&pkt);
		ret = avcodec_receive_packet(ost->enc, &pkt);
		if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
		{
			fprintf( avLogFp, "Error encoding audio frame\n");
			ost->writeError = true;
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
				fprintf( avLogFp, "Error while writing audio frame\n");
				ost->writeError = true;
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

	if ( ost->st == NULL )
	{
		return -1;
	}
	if ( ost->writeError )
	{
		return -1;
	}
	
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

	if ( ost->writeError )
	{
		return -1;
	}
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
		fprintf( avLogFp, "Error submitting a video frame for encoding\n");
		ost->writeError = true;
		return -1;
	}

	while (ret >= 0)
	{
		AVPacket pkt = { 0 };

		av_init_packet(&pkt);

		ret = avcodec_receive_packet(c, &pkt);

		if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
		{
			fprintf( avLogFp, "Error encoding a video frame\n");
			ost->writeError = true;
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
				fprintf( avLogFp, "Error while writing video frame\n");
				ost->writeError = true;
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
int aviRecordInit(void)
{
	g_config->getOption("SDL.AviDriver", &aviDriver);
	g_config->getOption("SDL.AviVideoFormat", &videoFormat);
	g_config->getOption("SDL.AviRecordAudio", &recordAudio);
	g_config->getOption("SDL.Sound.Rate", &audioSampleRate);

#ifdef _USE_LIBAV
	// LIBAV has its own internal video format configs,
	// it does not use this videoFormat symbol.
	if ( videoFormat == AVI_LIBAV )
	{
		aviSetSelVideoFormat( AVI_RGB24 );
	}
	LIBAV::setCodecFromConfig();
#endif
	return 0;
}
//**************************************************************************************
int aviRecordLogClose(void)
{
	if ( avLogFp != NULL )
	{
		if ( avLogFp != stdout )
		{
			fclose(avLogFp);
		}
		avLogFp = NULL;
	}
	return 0;
}
//**************************************************************************************
int aviRecordLogOpen(void)
{
	if ( avLogFp != NULL )
	{
		aviRecordLogClose();	
	}
	if ( avLogFp == NULL )
	{
		avLogFp = fopen( AV_LOG_FILE_NAME, "w");

		if ( avLogFp == NULL )
		{
			char msg[512];
			sprintf( msg, "Error: Failed to open AV Recording log file for writing: %s\n", AV_LOG_FILE_NAME);
			FCEUD_PrintError(msg);
			avLogFp = stdout;
		}
	}
	return avLogFp == NULL;
}
//**************************************************************************************
int aviRecordOpenFile( const char *filepath )
{
	char fourcc[8];
	gwavi_audio_t  audioConfig;
	double fps;
	char fileName[1024];

	if ( aviRecordLogOpen() )
	{
		// Log Error
		return -1;
	}
	g_config->getOption("SDL.AviRecordAudio", &recordAudio);

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

	g_config->getOption("SDL.AviVideoFormat", &videoFormat);

#ifdef WIN32
	if ( (aviDriver == AVI_DRIVER_LIBGWAVI) && (videoFormat == AVI_VFW) )
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
		//printf("Set VFW FourCC: %s\n", fourcc);
	}
	#endif 

#ifdef _USE_LIBAV
	if ( aviDriver == AVI_DRIVER_LIBAV )
	{
		if ( LIBAV::initMedia( fileName ) )
		{
			char msg[512];
			fprintf( avLogFp, "Error: Failed to open AVI file.\n");
			recordEnable = false;
			sprintf( msg, "Error: AV Recording Initialization Failed.\nSee %s for details...\n", AV_LOG_FILE_NAME);
			FCEUD_PrintError(msg);
			return -1;
		}
	}
	else
#endif
	{
		gwavi = new gwavi_t();

		if ( gwavi->open( fileName, nes_shm->video.ncol, nes_shm->video.nrow, fourcc, fps, &audioConfig ) )
		{
			char msg[512];
			fprintf( avLogFp, "Error: Failed to open AVI file.\n");
			recordEnable = false;
			sprintf( msg, "Error: AV Recording Initialization Failed.\nSee %s for details...\n", AV_LOG_FILE_NAME);
			FCEUD_PrintError(msg);
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

	g_config->setOption("SDL.AviRecordAudio", val);
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
int aviGetSelDriver(void)
{
	return aviDriver;
}
//**************************************************************************************
void aviSetSelDriver(int idx)
{
	aviDriver = idx;
	//printf("AVI Driver Changed:%i\n", aviDriver );

	g_config->setOption("SDL.AviDriver", aviDriver);
}
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
int FCEUD_AviGetDriverList( std::vector <std::string> &formatList )
{
	std::string s;

	for (int i=0; i<AVI_NUM_DRIVERS; i++)
	{
		switch ( i )
		{
			default:
				s.assign("Unknown");
			break;
			case AVI_DRIVER_LIBGWAVI:
				s.assign("libgwavi");
			break;
			#ifdef _USE_LIBAV
			case AVI_DRIVER_LIBAV:
				s.assign("libav (ffmpeg)");
			break;
			#endif
		}
		formatList.push_back(s);
	}
	return AVI_NUM_DRIVERS;
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
	char localRecordAudio = 0;
	int  avgAudioPerFrame, localVideoFormat;

	fprintf( avLogFp, "AVI Record Disk Thread Start\n");

	setPriority( QThread::HighestPriority );

	fps = getBaseFrameRate();

	avgAudioPerFrame = ( audioSampleRate / fps) + 1;

	fprintf( avLogFp, "Avg Audio Sample Rate per Frame: %i \n", avgAudioPerFrame );

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
#ifdef _USE_LIBAV
	if ( aviDriver == AVI_DRIVER_LIBAV )
	{
		localVideoFormat = AVI_LIBAV;
	}
	else
#endif
	{
		localVideoFormat = videoFormat;
	}
	localRecordAudio = recordAudio;

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

			if ( writeAudio && localRecordAudio )
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

	fprintf( avLogFp, "AVI Record Disk Thread Exit\n");
	emit finished();

	if ( avLogFp != NULL )
	{
		if ( avLogFp != stdout )
		{
			fclose(avLogFp);
		}
		avLogFp = NULL;
	}
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
	//QHBoxLayout *hbox;
	QGridLayout *grid;
	QPushButton *videoConfBtn, *audioConfBtn;

	g_config->getOption("SDL.AviRecordAudio", &recordAudio);

	LIBAV::setCodecFromConfig();

	vbox1 = new QVBoxLayout();

	videoGbox = new QGroupBox( tr("Video:") );
	audioGbox = new QGroupBox( tr("Audio:") );

	audioGbox->setCheckable(false);
	//audioGbox->setChecked( aviGetAudioEnable() );

	videoEncSel     = new QComboBox();
	audioEncSel     = new QComboBox();
	videoPixfmt     = new QComboBox();
	audioSamplefmt  = new QComboBox();
	audioSampleRate = new QComboBox();
	audioChanLayout = new QComboBox();

	vbox1->addWidget( videoGbox );
	vbox1->addWidget( audioGbox );

	vbox = new QVBoxLayout();
	videoGbox->setLayout(vbox);

	grid = new QGridLayout();
	vbox->addLayout(grid);
	lbl  = new QLabel( tr("Encoder:") );
	grid->addWidget( lbl, 0, 0);
	grid->addWidget( videoEncSel, 0, 1);
	lbl  = new QLabel( tr("Pixel Format:") );
	grid->addWidget( lbl, 1, 0);
	grid->addWidget( videoPixfmt, 1, 1);
	videoConfBtn = new QPushButton( tr("Options...") );
	grid->addWidget( videoConfBtn, 2, 1);

	vbox = new QVBoxLayout();
	audioGbox->setLayout(vbox);

	grid = new QGridLayout();
	vbox->addLayout(grid);
	lbl  = new QLabel( tr("Encoder:") );
	grid->addWidget( lbl, 0, 0);
	grid->addWidget( audioEncSel, 0, 1 );
	lbl  = new QLabel( tr("Sample Format:") );
	grid->addWidget( lbl, 1, 0);
	grid->addWidget( audioSamplefmt, 1, 1);
	lbl  = new QLabel( tr("Sample Rate:") );
	grid->addWidget( lbl, 2, 0);
	grid->addWidget( audioSampleRate, 2, 1);
	lbl  = new QLabel( tr("Channel Layout:") );
	grid->addWidget( lbl, 3, 0);
	grid->addWidget( audioChanLayout, 3, 1);
	audioConfBtn = new QPushButton( tr("Options...") );
	grid->addWidget( audioConfBtn, 4, 1);

	initCodecLists();

	setLayout(vbox1);

	connect(videoEncSel, SIGNAL(currentIndexChanged(int)), this, SLOT(videoCodecChanged(int)));
	connect(audioEncSel, SIGNAL(currentIndexChanged(int)), this, SLOT(audioCodecChanged(int)));

	connect(videoPixfmt    , SIGNAL(currentIndexChanged(int)), this, SLOT(videoPixelFormatChanged(int)));
	connect(audioSamplefmt , SIGNAL(currentIndexChanged(int)), this, SLOT(audioSampleFormatChanged(int)));
	connect(audioSampleRate, SIGNAL(currentIndexChanged(int)), this, SLOT(audioSampleRateChanged(int)));
	connect(audioChanLayout, SIGNAL(currentIndexChanged(int)), this, SLOT(audioChannelLayoutChanged(int)));

	connect(videoConfBtn, SIGNAL(clicked(void)), this, SLOT(openVideoCodecOptions(void)));
	connect(audioConfBtn, SIGNAL(clicked(void)), this, SLOT(openAudioCodecOptions(void)));

	//connect(audioGbox, SIGNAL(clicked(bool)), this, SLOT(includeAudioChanged(bool)));
	
	updateTimer = new QTimer(this);

	connect( updateTimer, &QTimer::timeout, this, &LibavOptionsPage::periodicUpdate );

	updateTimer->start(200);
}
//-----------------------------------------------------
LibavOptionsPage::~LibavOptionsPage(void)
{
	updateTimer->stop();
}
//-----------------------------------------------------
void LibavOptionsPage::periodicUpdate(void)
{
	audioGbox->setEnabled( recordAudio );
}
//-----------------------------------------------------
void LibavOptionsPage::initPixelFormatSelect(const char *codec_name)
{
	const AVCodec *c;
	const AVPixFmtDescriptor *desc;
	bool formatOk = false;

	c = avcodec_find_encoder_by_name( codec_name );

	videoPixfmt->clear();
	videoPixfmt->addItem( tr("Auto"), -1);

	if ( c == NULL )
	{
		return;
	}
	if ( c->pix_fmts )
	{
		int i=0; //, formatOk=0;
		while (c->pix_fmts[i] != -1)
		{
			desc = av_pix_fmt_desc_get( c->pix_fmts[i] );

			if ( desc )
			{
				//printf("Codec PIX_FMT: %i: %s 0x%04X\t-  %s\n", c->pix_fmts[i],
				//		desc->name, av_get_pix_fmt_loss(c->pix_fmts[i], AV_PIX_FMT_BGRA, 0), desc->alias);

				videoPixfmt->addItem( tr(desc->name), c->pix_fmts[i]);

				if ( LIBAV::video_st.pixelFormat == c->pix_fmts[i] )
				{
					videoPixfmt->setCurrentIndex( videoPixfmt->count() - 1 );
					formatOk = true;
				}
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
		// List More Common Raw Video Formats Only
		desc = av_pix_fmt_desc_next( NULL );

		while ( desc != NULL )
		{
			AVPixelFormat pf;

			pf = av_pix_fmt_desc_get_id(desc);

			//printf("Codec PIX_FMT: %i: %s  0x%04X\t-  %s\n", 
			//		pf, desc->name, av_get_pix_fmt_loss(pf, AV_PIX_FMT_BGRA, 0), desc->alias);

			switch ( pf )
			{
				default:

				break;
				case  AV_PIX_FMT_YUV420P:   ///< planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
				case  AV_PIX_FMT_YUYV422:   ///< packed YUV 4:2:2, 16bpp, Y0 Cb Y1 Cr
				case  AV_PIX_FMT_RGB24:     ///< packed RGB 8:8:8, 24bpp, RGBRGB...
				case  AV_PIX_FMT_BGR24:     ///< packed RGB 8:8:8, 24bpp, BGRBGR...
				case  AV_PIX_FMT_YUV422P:   ///< planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)
				case  AV_PIX_FMT_YUV444P:   ///< planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples)
				case  AV_PIX_FMT_YUV410P:   ///< planar YUV 4:1:0,  9bpp, (1 Cr & Cb sample per 4x4 Y samples)
				case  AV_PIX_FMT_YUV411P:   ///< planar YUV 4:1:1, 12bpp, (1 Cr & Cb sample per 4x1 Y samples)
				case  AV_PIX_FMT_PAL8:      ///< 8 bits with AV_PIX_FMT_RGB32 palette
				case  AV_PIX_FMT_YUVJ420P:  ///< planar YUV 4:2:0, 12bpp, full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV420P and setting color_range
				case  AV_PIX_FMT_YUVJ422P:  ///< planar YUV 4:2:2, 16bpp, full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV422P and setting color_range
				case  AV_PIX_FMT_YUVJ444P:  ///< planar YUV 4:4:4, 24bpp, full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV444P and setting color_range
				case  AV_PIX_FMT_UYVY422:   ///< packed YUV 4:2:2, 16bpp, Cb Y0 Cr Y1
				case  AV_PIX_FMT_UYYVYY411: ///< packed YUV 4:1:1, 12bpp, Cb Y0 Y1 Cr Y2 Y3
				case  AV_PIX_FMT_NV12:      ///< planar YUV 4:2:0, 12bpp, 1 plane for Y and 1 plane for the UV components, which are interleaved (first byte U and the following byte V)
				case  AV_PIX_FMT_NV21:      ///< as above, but U and V bytes are swapped
				case  AV_PIX_FMT_ARGB:      ///< packed ARGB 8:8:8:8, 32bpp, ARGBARGB...
				case  AV_PIX_FMT_RGBA:      ///< packed RGBA 8:8:8:8, 32bpp, RGBARGBA...
				case  AV_PIX_FMT_ABGR:      ///< packed ABGR 8:8:8:8, 32bpp, ABGRABGR...
				case  AV_PIX_FMT_BGRA:      ///< packed BGRA 8:8:8:8, 32bpp, BGRABGRA...
					videoPixfmt->addItem( tr(desc->name), pf);

					if ( LIBAV::video_st.pixelFormat == pf )
					{
						videoPixfmt->setCurrentIndex( videoPixfmt->count() - 1 );
						formatOk = true;
					}
				break;
			}

			desc = av_pix_fmt_desc_next( desc );
		}
	}

	if ( !formatOk )
	{
		LIBAV::video_st.pixelFormat = -1;
	}

}
//-----------------------------------------------------
void LibavOptionsPage::initSampleFormatSelect( const char *codec_name )
{

	const AVCodec *c;
	bool formatOk = false;

	c = avcodec_find_encoder_by_name( codec_name );

	audioSamplefmt->clear();
	audioSamplefmt->addItem( tr("Auto"), -1);

	if ( c == NULL )
	{
		return;
	}
	if ( c->sample_fmts )
	{
		int i=0;
		const char *fmtName;

		while ( c->sample_fmts[i] != -1 )
		{
			fmtName = av_get_sample_fmt_name( c->sample_fmts[i] );

			if ( fmtName )
			{
				audioSamplefmt->addItem( tr(fmtName), c->sample_fmts[i] );

				if ( LIBAV::audio_st.sampleFormat == c->sample_fmts[i] )
				{
					audioSamplefmt->setCurrentIndex( audioSamplefmt->count() - 1 );
					formatOk = true;
				}
			}
			i++;
		}
	}
	if ( !formatOk )
	{
		LIBAV::audio_st.sampleFormat = -1;
	}
}
//-----------------------------------------------------
void LibavOptionsPage::initSampleRateSelect( const char *codec_name )
{

	const AVCodec *c;
	bool formatOk = false;

	c = avcodec_find_encoder_by_name( codec_name );

	audioSampleRate->clear();
	audioSampleRate->addItem( tr("Auto"), -1);

	if ( c == NULL )
	{
		return;
	}
	if ( c->supported_samplerates )
	{
		int i=0;
		char rateName[64];

		while ( c->supported_samplerates[i] != 0 )
		{
			sprintf( rateName, "%i", c->supported_samplerates[i] );

			audioSampleRate->addItem( tr(rateName), c->supported_samplerates[i] );

			if ( LIBAV::audio_st.sampleRate == c->supported_samplerates[i] )
			{
				audioSampleRate->setCurrentIndex( audioSampleRate->count() - 1 );
				formatOk = true;
			}
			i++;
		}
	}
	if ( !formatOk )
	{
		LIBAV::audio_st.sampleRate = -1;
	}
}
//-----------------------------------------------------
void LibavOptionsPage::initChannelLayoutSelect( const char *codec_name )
{

	const AVCodec *c;
	bool formatOk = false;

	c = avcodec_find_encoder_by_name( codec_name );

	audioChanLayout->clear();
	audioChanLayout->addItem( tr("Auto"), -1);

	if ( c == NULL )
	{
		return;
	}
	if ( c->channel_layouts )
	{
		int i=0;
		char layoutDesc[256];

		while ( c->channel_layouts[i] != 0 )
		{
			av_get_channel_layout_string( layoutDesc, sizeof(layoutDesc), -1, c->channel_layouts[i] );

			audioChanLayout->addItem( tr(layoutDesc), (unsigned long long)c->channel_layouts[i] );

			if ( LIBAV::audio_st.chanLayout == c->channel_layouts[i] )
			{
				audioChanLayout->setCurrentIndex( audioChanLayout->count() - 1 );
				formatOk = true;
			}
			i++;
		}
	}
	if ( !formatOk )
	{
		LIBAV::audio_st.chanLayout = -1;
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
				//printf("Video Encoder: %i  %s   %s\t:%i\n", c->id, c->name, c->long_name, compatible);
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
				//printf("Audio Encoder: %i  %s   %s\t:%i\n", c->id, c->name, c->long_name, compatible);
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
	initSampleFormatSelect( audioEncSel->currentText().toStdString().c_str() );
	initSampleRateSelect( audioEncSel->currentText().toStdString().c_str() );
	initChannelLayoutSelect( audioEncSel->currentText().toStdString().c_str() );
}
//-----------------------------------------------------
void LibavOptionsPage::includeAudioChanged(bool checked)
{
	aviSetAudioEnable( checked );
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

		initSampleFormatSelect( c->name );
		initSampleRateSelect( c->name );
		initChannelLayoutSelect( c->name );
	}
}
//-----------------------------------------------------
void LibavOptionsPage::videoPixelFormatChanged(int idx)
{
	LIBAV::video_st.pixelFormat = videoPixfmt->itemData(idx).toInt();

	printf("Selected Pixel Format: %i\n", LIBAV::video_st.pixelFormat );
	
	g_config->setOption("SDL.AviFFmpegVideoPixFmt", LIBAV::video_st.pixelFormat);	
}
//-----------------------------------------------------
void LibavOptionsPage::audioSampleFormatChanged(int idx)
{
	LIBAV::audio_st.sampleFormat = audioSamplefmt->itemData(idx).toInt();

	printf("Selected Sample Format: %i\n", LIBAV::audio_st.sampleFormat );
	
	g_config->setOption("SDL.AviFFmpegAudioSmpFmt", LIBAV::audio_st.sampleFormat);	
}
//-----------------------------------------------------
void LibavOptionsPage::audioSampleRateChanged(int idx)
{
	LIBAV::audio_st.sampleRate = audioSampleRate->itemData(idx).toInt();

	printf("Selected Sample Rate: %i\n", LIBAV::audio_st.sampleRate );
	
	g_config->setOption("SDL.AviFFmpegAudioSmpRate", LIBAV::audio_st.sampleRate);	
}
//-----------------------------------------------------
void LibavOptionsPage::audioChannelLayoutChanged(int idx)
{
	LIBAV::audio_st.chanLayout = audioChanLayout->itemData(idx).toInt();

	printf("Selected Channel Layout: 0x%X\n", LIBAV::audio_st.chanLayout );
	
	g_config->setOption("SDL.AviFFmpegAudioChanLayout", LIBAV::audio_st.chanLayout);	
}
//----------------------------------------------------------------------------
void LibavOptionsPage::openVideoCodecOptions(void)
{
	LibavEncOptWin *win = new LibavEncOptWin(0,this);

	win->show();
}
//----------------------------------------------------------------------------
void LibavOptionsPage::openAudioCodecOptions(void)
{
	LibavEncOptWin *win = new LibavEncOptWin(1,this);

	win->show();
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LibavEncOptItem::LibavEncOptItem(QTreeWidgetItem *parent)
	: QTreeWidgetItem(parent)
{
	opt = NULL;
}
//----------------------------------------------------------------------------
LibavEncOptItem::~LibavEncOptItem(void)
{

}
//----------------------------------------------------------------------------
void LibavEncOptItem::setValueText(void)
{
	char stmp[256];
	uint8_t *s;

	stmp[0] = 0;

	s = NULL;

	if ( av_opt_get( obj, opt->name, 0, &s ) >= 0 )
	{
		if ( s != NULL )
		{
			strcpy( stmp, (char*)s );
		}
	}
	if ( s != NULL )
	{
		av_free(s); s = NULL;
	}

	if ( units.size() > 0 )
	{
		switch ( opt->type )
		{
			case AV_OPT_TYPE_FLAGS:
			{
				int64_t i,j;

				j=0;
				if ( av_opt_get_int( obj, opt->name, 0, &i ) >= 0 )
				{
					for (size_t x=0; x<units.size(); x++)
					{
						if ( units[x]->default_val.i64 & i )
						{
							char stmp2[128];
							sprintf( stmp2, "%s", units[x]->name );
							if (j>0)
							{
								strcat( stmp, ",");
							}
							else
							{
								strcat( stmp, " (");
							}
							strcat( stmp, stmp2 );
							j++;
						}
					}
					if ( j > 0 )
					{
						strcat( stmp, ")");
					}
				}
			}
			break;
			case AV_OPT_TYPE_INT:
			case AV_OPT_TYPE_INT64:
			case AV_OPT_TYPE_UINT64:
			{
				int64_t i;

				if ( av_opt_get_int( obj, opt->name, 0, &i ) >= 0 )
				{
					for (size_t x=0; x<units.size(); x++)
					{
						if ( units[x]->default_val.i64 == i )
						{
							char stmp2[128];
							sprintf( stmp2, " (%s)", units[x]->name );
							strcat( stmp, stmp2 );
							break;
						}
					}
				}
			}
			break;
			default:
				// Nothing to add
			break;
		}
	}
	setText(1, QString::fromStdString(stmp));
}
//----------------------------------------------------------------------------
LibavEncOptWin::LibavEncOptWin(int type, QWidget *parent)
	: QDialog(parent)
{
	QVBoxLayout *mainLayout;
	QHBoxLayout *hbox;
	QPushButton *closeButton, *resetDefaults;
	QTreeWidgetItem *itemHdr = NULL;
	QTreeWidgetItem *groupItem = NULL;
	LibavEncOptItem *item = NULL;
	const AVCodec *codec;
	const AVOption *opt;
	bool useOpt, newOpt;
	char title[128];
	void *obj = NULL, *ctx_child = NULL;

	this->type = type;

	if ( type )
	{
		codec_name = LIBAV::audio_st.selEnc.c_str();
		sprintf( title, "%s Audio Encoder Configuration", codec_name );
	}
	else
	{
		codec_name = LIBAV::video_st.selEnc.c_str();
		sprintf( title, "%s Video Encoder Configuration", codec_name );
	}
	setWindowTitle( title );
	resize(512, 512);

	/* find the video encoder */
	codec = avcodec_find_encoder_by_name(codec_name);

	ctx = NULL;
	if (codec != NULL)
	{
		//printf("CODEC: %s\n", codec->name );

		ctx = avcodec_alloc_context3(codec);

		LIBAV::loadCodecConfig( type, codec_name, ctx );

		//av_opt_show2( (void*)ctx, NULL, AV_OPT_FLAG_VIDEO_PARAM, 0 );

		//ctx_child = av_opt_child_next( ctx, ctx_child );

		//while ( ctx_child != NULL )
		//{
		//	av_opt_show2( ctx_child, NULL, AV_OPT_FLAG_VIDEO_PARAM, 0 );

		//	ctx_child = av_opt_child_next( ctx, ctx_child );
		//}
	}
	obj = ctx;

	mainLayout = new QVBoxLayout();

	tree = new QTreeWidget(this);

	tree->setColumnCount(3);
	tree->setSelectionMode( QAbstractItemView::SingleSelection );
	tree->setAlternatingRowColors(true);

	itemHdr = new QTreeWidgetItem();
	itemHdr->setText(0, QString::fromStdString("Option"));
	itemHdr->setText(1, QString::fromStdString("Value"));
	itemHdr->setText(2, QString::fromStdString("Desc"));
	itemHdr->setTextAlignment(0, Qt::AlignLeft);
	itemHdr->setTextAlignment(1, Qt::AlignLeft);
	itemHdr->setTextAlignment(2, Qt::AlignLeft);

	tree->setHeaderItem(itemHdr);

	tree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

	//printf("CTX Class: %s\n", ctx->av_class->class_name );
	obj = ctx;
	ctx_child = NULL;

	while ( obj != NULL )
	{
		const char *groupName = (*static_cast<AVClass**>(obj))->class_name;

		//printf("OBJ Class: %s\n", groupName);

		groupItem = new QTreeWidgetItem();
		tree->addTopLevelItem(groupItem);

		groupItem->setText(0, QString::fromStdString(groupName));

		opt = av_opt_next( obj, NULL );

		while ( opt != NULL )
		{
			useOpt = (opt->name != NULL) &&
					(opt->type != AV_OPT_TYPE_BINARY) &&
					(opt->type != AV_OPT_TYPE_DICT);

			if ( type )
			{
				useOpt = useOpt &&
						(opt->flags & AV_OPT_FLAG_ENCODING_PARAM) &&
						(opt->flags & AV_OPT_FLAG_AUDIO_PARAM);
			}
			else
			{
				useOpt = useOpt &&
						(opt->flags & AV_OPT_FLAG_ENCODING_PARAM) &&
						(opt->flags & AV_OPT_FLAG_VIDEO_PARAM);
			}
			newOpt = true;

			if ( item )
			{
				if ( opt->unit && item->opt->unit )
				{
					if ( strcmp( opt->unit, item->opt->unit ) == 0 )
					{
						newOpt = false;
					}
				}
			}

			if ( useOpt )
			{
				if ( newOpt )
				{
					item = new LibavEncOptItem();

					item->obj = obj;
					item->opt = opt;
					item->setText(0, QString::fromStdString(opt->name));
					//item->setText(1, QString::fromStdString("Value"));
					if ( opt->help )
					{
						item->setText(2, QString::fromStdString(opt->help));
					}

					item->setValueText();
					item->setTextAlignment(0, Qt::AlignLeft);
					item->setTextAlignment(1, Qt::AlignLeft);
					item->setTextAlignment(2, Qt::AlignLeft);

					//tree->addTopLevelItem(item);
					groupItem->addChild(item);
				}
				else
				{
					if ( item )
					{
						item->units.push_back(opt);
						item->setValueText();
					}
				}
				//printf("Option: %s - %i - %s - %s\n", opt->name, opt->type, opt->unit, opt->help);

				//if ( opt->type == AV_OPT_TYPE_FLAGS )
				//{
				//	printf("   Value: %llx \n", (unsigned long long)opt->default_val.i64 );
				//}
				//else if ( opt->type == AV_OPT_TYPE_CONST )
				//{
				//	printf("   Value: %llx \n", (unsigned long long)opt->default_val.i64 );
				//}
			}
			opt = av_opt_next( obj, opt );
		}
		obj = ctx_child = av_opt_child_next( ctx, ctx_child );
	}

	//connect( tree, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(hotKeyDoubleClicked(QTreeWidgetItem*,int) ) );
	connect( tree, SIGNAL(itemActivated(QTreeWidgetItem*,int)), this, SLOT(itemChangeActivated(QTreeWidgetItem*,int) ) );

	mainLayout->addWidget(tree);

	resetDefaults = new QPushButton( tr("Restore Defaults") );
	resetDefaults->setIcon(style()->standardIcon(QStyle::SP_DialogResetButton));
	connect(resetDefaults, SIGNAL(clicked(void)), this, SLOT(resetDefaultsCB(void)));

	closeButton = new QPushButton( tr("Close") );
	closeButton->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
	connect(closeButton, SIGNAL(clicked(void)), this, SLOT(closeWindow(void)));

	hbox = new QHBoxLayout();
	hbox->addWidget( resetDefaults, 1 );
	hbox->addStretch(5);
	hbox->addWidget( closeButton, 1 );
	mainLayout->addLayout( hbox );

	setLayout(mainLayout);
}
//----------------------------------------------------------------------------
LibavEncOptWin::~LibavEncOptWin(void)
{
	printf("Destroy Encoder Options Config Window\n");

	LIBAV::saveCodecConfig(type, codec_name, ctx);
}
//----------------------------------------------------------------------------
void LibavEncOptWin::closeEvent(QCloseEvent *event)
{
	printf("Encoder Options Close Window Event\n");
	done(0);
	deleteLater();
	event->accept();
}
//----------------------------------------------------------------------------
void LibavEncOptWin::closeWindow(void)
{
	//printf("Close Window\n");
	done(0);
	deleteLater();
}
//-----------------------------------------------------
void LibavEncOptWin::resetDefaultsCB(void)
{
	void *obj = NULL;

	av_opt_set_defaults(ctx);

	obj = av_opt_child_next( ctx, obj );

	while (obj != NULL)
	{
		av_opt_set_defaults(obj);

		obj = av_opt_child_next( ctx, obj );
	}
	updateItems();
}
//-----------------------------------------------------
void LibavEncOptWin::updateItems(void)
{
	QTreeWidgetItem *groupItem;
	LibavEncOptItem *item = NULL;

	for (int i=0; i<tree->topLevelItemCount(); i++)
	{
		groupItem = tree->topLevelItem(i);

		for (int j=0; j<groupItem->childCount(); j++)
		{
			item = static_cast<LibavEncOptItem*>(groupItem->child(j));
			item->setValueText();
		}
	}
	tree->viewport()->update();
}
//-----------------------------------------------------
void LibavEncOptWin::itemChangeActivated( QTreeWidgetItem *itemBase, int col)
{
	LibavEncOptItem *item = NULL;
	LibavEncOptInputWin *win;

	if ( itemBase->childCount() > 0 )
	{
		return;
	}

	item = static_cast<LibavEncOptItem*>(itemBase);

	if ( item->opt == NULL )
	{
		return;
	}
	win = new LibavEncOptInputWin( item, this );

	win->show();

	connect( win, SIGNAL(finished(int)), this, SLOT(editWindowFinished(int)) );
}
//-----------------------------------------------------
void LibavEncOptWin::editWindowFinished(int result)
{
	updateItems();
}
//-----------------------------------------------------
//-----------------------------------------------------
//-----------------------------------------------------
//----------------------------------------------------------------------------
LibavEncOptInputWin::LibavEncOptInputWin( LibavEncOptItem *itemIn, QWidget *parent )
	: QDialog(parent)
{
	const AVOption *opt;
	QVBoxLayout *mainLayout;
	QHBoxLayout *hbox;
	QGridLayout *grid;
	QLabel *lbl;
	char stmp[128];
	void *obj;

	item = itemIn;
	opt  = item->opt;
	obj  = item->obj;

	setWindowTitle("Set Value");

	mainLayout = new QVBoxLayout();
	grid       = new QGridLayout();

	setLayout( mainLayout );
	mainLayout->addLayout(grid);
	grid->addWidget( new QLabel( tr("Name:")   ), 0, 0 );
	grid->addWidget( new QLabel( tr(opt->name) ), 0, 1 );

	if ( opt->help )
	{
		lbl = new QLabel( tr(opt->help) );
		lbl->setWordWrap(true);

		grid->addWidget( new QLabel( tr("Desc:")   ), 1, 0 );
		grid->addWidget( lbl, 1, 1 );
	}
	combo = NULL;
	intEntry = NULL;
	floatEntry = NULL;
	numEntry = NULL;
	denEntry = NULL;
	strEntry = NULL;

	switch ( opt->type )
	{
		case AV_OPT_TYPE_INT:
		case AV_OPT_TYPE_INT64:
		case AV_OPT_TYPE_UINT64:
		{
			int64_t val;

			av_opt_get_int( obj, opt->name, 0, &val );

			grid->addWidget( new QLabel( tr("Range:")   ), 2, 0 );

			sprintf( stmp, "[ %.0f, %.0f ]", opt->min, opt->max );

			grid->addWidget( new QLabel( tr(stmp)   ), 2, 1 );

			grid->addWidget( new QLabel( tr("Default:")   ), 3, 0 );

			sprintf( stmp, "%lli", (long long)opt->default_val.i64 );

			grid->addWidget( new QLabel( tr(stmp)   ), 3, 1 );

			grid->addWidget( new QLabel( tr("Value:")   ), 4, 0 );

			if ( item->units.size() > 0 )
			{
				combo = new QComboBox();

				grid->addWidget( combo, 4, 1 );

				for (size_t i=0; i<item->units.size(); i++)
				{
					if ( item->units[i]->help )
					{
						sprintf( stmp, "%3lli:  %s  -  %s", (long long)item->units[i]->default_val.i64,
							item->units[i]->name, item->units[i]->help );
					}
					else
					{
						sprintf( stmp, "%3lli:  %s", (long long)item->units[i]->default_val.i64,
							item->units[i]->name );
					}

					combo->addItem( tr(stmp), (const long long)item->units[i]->default_val.i64 );

					if ( val == item->units[i]->default_val.i64 )
					{
						combo->setCurrentIndex( combo->count() - 1 );
					}
				}
			}
			else
			{
				intEntry = new QSpinBox();
				intEntry->setValue( val );
				intEntry->setRange( (int)(opt->min), (int)(opt->max) );

				grid->addWidget( intEntry, 4, 1 );
			}
		}
		break;
		case AV_OPT_TYPE_FLOAT:
		case AV_OPT_TYPE_DOUBLE:
		{
			double val;

			av_opt_get_double( obj, opt->name, 0, &val );

			grid->addWidget( new QLabel( tr("Range:")   ), 2, 0 );

			sprintf( stmp, "[ %e, %e ]", opt->min, opt->max );

			grid->addWidget( new QLabel( tr(stmp)   ), 2, 1 );

			grid->addWidget( new QLabel( tr("Default:")   ), 3, 0 );

			sprintf( stmp, "%f", opt->default_val.dbl );

			grid->addWidget( new QLabel( tr(stmp)   ), 3, 1 );

			grid->addWidget( new QLabel( tr("Value:")   ), 4, 0 );

			floatEntry = new QDoubleSpinBox();
			floatEntry->setValue( val );
			floatEntry->setRange( opt->min, opt->max );

			grid->addWidget( floatEntry, 4, 1 );
		}
		break;
		case AV_OPT_TYPE_STRING:
		{
			uint8_t *val = NULL;

			av_opt_get( obj, opt->name, 0, &val );

			grid->addWidget( new QLabel( tr("Default:")   ), 2, 0 );

			stmp[0] = 0;

			if ( opt->default_val.str )
			{
				sprintf( stmp, "%s", opt->default_val.str );
			}
			grid->addWidget( new QLabel( tr(stmp)   ), 2, 1 );

			grid->addWidget( new QLabel( tr("Value:")   ), 3, 0 );

			strEntry = new QLineEdit();

			if ( val )
			{
				strEntry->setText( tr( (const char*)val ) );
				av_free(val); val = NULL;
			}
			grid->addWidget( strEntry, 3, 1 );
		}
		break;
		case AV_OPT_TYPE_RATIONAL:
		{
			AVRational val;

			av_opt_get_q( obj, opt->name, 0, &val );

			grid->addWidget( new QLabel( tr("Default:")   ), 2, 0 );

			sprintf( stmp, "%i/%i", opt->default_val.q.num, opt->default_val.q.den );

			grid->addWidget( new QLabel( tr(stmp)   ), 2, 1 );

			grid->addWidget( new QLabel( tr("Numerator:")   ), 3, 0 );

			numEntry = new QSpinBox();
			numEntry->setValue( val.num );
			numEntry->setRange( 0, 0x7FFFFFFF );

			grid->addWidget( numEntry, 3, 1 );

			grid->addWidget( new QLabel( tr("Denominator:")   ), 4, 0 );

			denEntry = new QSpinBox();
			denEntry->setValue( val.den );
			denEntry->setRange( 1, 0x7FFFFFFF );

			grid->addWidget( denEntry, 4, 1 );
		}
		break;
		case AV_OPT_TYPE_BOOL:
		{
			int64_t val;
			QCheckBox *c;

			av_opt_get_int( obj, opt->name, 0, &val );

			grid->addWidget( new QLabel( tr("Default:")   ), 2, 0 );

			sprintf( stmp, "%s", opt->default_val.i64 ? "true" : "false" );

			grid->addWidget( new QLabel( tr(stmp)   ), 2, 1 );

			grid->addWidget( new QLabel( tr("Value:")   ), 3, 0 );

			c = new QCheckBox( tr("Checked=true") );

			c->setChecked( val != 0 );

			chkBox.push_back(c);

			grid->addWidget( c, 3, 1 );
		}
		break;
		case AV_OPT_TYPE_FLAGS:
		{
			int64_t val;
			QCheckBox *c;

			av_opt_get_int( obj, opt->name, 0, &val );

			grid->addWidget( new QLabel( tr("Default:")   ), 2, 0 );

			sprintf( stmp, "0x%08llX", (unsigned long long)opt->default_val.i64 );

			grid->addWidget( new QLabel( tr(stmp)   ), 2, 1 );

			if ( item->units.size() > 0 )
			{
				int row, col;

				for (size_t i=0; i<item->units.size(); i++)
				{
					sprintf( stmp, "%s", item->units[i]->name );

					c = new QCheckBox( tr(stmp) );

					if ( item->units[i]->help )
					{
						c->setToolTip( tr(item->units[i]->help) );
					}

					c->setChecked( (val & item->units[i]->default_val.i64) ? true : false );

					chkBox.push_back(c);
				}

				row = col = 0;
				for (size_t i=0; i<chkBox.size(); i++)
				{
					grid->addWidget( chkBox[i], row+3, col );
					
					row++;
					if ( row >= 8 )
					{
						row = 0;
						col++;
					}
				}
			}
		}
		break;
		default:

		break;
	}

	    okButton  = new QPushButton( tr("Apply") );
	cancelButton  = new QPushButton( tr("Cancel") );
	resetDefaults = new QPushButton( tr("Reset") );

	     okButton->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
	 cancelButton->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));
	resetDefaults->setIcon(style()->standardIcon(QStyle::SP_DialogResetButton));

	hbox = new QHBoxLayout();
	hbox->addWidget( resetDefaults, 1 );
	hbox->addWidget(  cancelButton, 1 );
	hbox->addStretch(5);
	hbox->addWidget(      okButton, 1 );
	mainLayout->addLayout(hbox);

	connect(      okButton, SIGNAL(clicked(void)), this, SLOT(applyChanges(void))    );
	connect(  cancelButton, SIGNAL(clicked(void)), this, SLOT(closeWindow(void))     );
	connect( resetDefaults, SIGNAL(clicked(void)), this, SLOT(resetDefaultsCB(void)) );
}
//-----------------------------------------------------
LibavEncOptInputWin::~LibavEncOptInputWin(void)
{

}
//----------------------------------------------------------------------------
void LibavEncOptInputWin::closeEvent(QCloseEvent *event)
{
	printf("Encoder Options Close Window Event\n");
	done(0);
	deleteLater();
	event->accept();
}
//----------------------------------------------------------------------------
void LibavEncOptInputWin::closeWindow(void)
{
	//printf("Close Window\n");
	done(0);
	deleteLater();
}
//-----------------------------------------------------
void LibavEncOptInputWin::applyChanges(void)
{
	switch ( item->opt->type )
	{
		case AV_OPT_TYPE_INT:
		case AV_OPT_TYPE_INT64:
		case AV_OPT_TYPE_UINT64:
		{
			if ( intEntry )
			{
				av_opt_set_int( item->obj, item->opt->name, intEntry->value(), 0 );
			}

			if ( combo )
			{
				av_opt_set_int( item->obj, item->opt->name, 
						combo->currentData().toInt(), 0 );
			}
		}
		break;
		case AV_OPT_TYPE_FLOAT:
		case AV_OPT_TYPE_DOUBLE:
		{
			if ( floatEntry )
			{
				av_opt_set_double( item->obj, item->opt->name, floatEntry->value(), 0 );
			}
		}
		break;
		case AV_OPT_TYPE_STRING:
		{
			if ( strEntry )
			{
				av_opt_set( item->obj, item->opt->name, strEntry->text().toStdString().c_str(), 0 );
			}
		}
		break;
		case AV_OPT_TYPE_RATIONAL:
		{
			AVRational q;

			q.num = 0;
			q.den = 1;

			if ( numEntry )
			{
				q.num = numEntry->value();
			}

			if ( denEntry )
			{
				q.den = denEntry->value();
			}

			av_opt_set_q( item->obj, item->opt->name, q, 0 );
		}
		break;
		case AV_OPT_TYPE_BOOL:
		{
			if ( chkBox.size() > 0 )
			{
				av_opt_set_int( item->obj, item->opt->name, chkBox[0]->isChecked(), 0 );
			}
		}
		break;
		case AV_OPT_TYPE_FLAGS:
		{
			if ( chkBox.size() > 0 )
			{
				int64_t i64=0;

				for (size_t i=0; i<chkBox.size(); i++)
				{
					if ( chkBox[i]->isChecked() )
					{
						i64 |= item->units[i]->default_val.i64;
					}
				}
				av_opt_set_int( item->obj, item->opt->name, i64, 0 );
			}
		}
		break;
		default:
		break;
	}

	done(0);
	deleteLater();
}
//-----------------------------------------------------
void LibavEncOptInputWin::resetDefaultsCB(void)
{
	switch ( item->opt->type )
	{
		case AV_OPT_TYPE_INT:
		case AV_OPT_TYPE_INT64:
		case AV_OPT_TYPE_UINT64:
		{
			if ( intEntry )
			{
				intEntry->setValue( item->opt->default_val.i64 );
			}

			if ( combo )
			{
				for (int i=0; i<combo->count(); i++)
				{
					if ( combo->itemData(i).toInt() == item->opt->default_val.i64 )
					{
						combo->setCurrentIndex(i);
					}
				}
			}
		}
		break;
		case AV_OPT_TYPE_FLOAT:
		case AV_OPT_TYPE_DOUBLE:
		{
			if ( floatEntry )
			{
				floatEntry->setValue( item->opt->default_val.dbl );
			}
		}
		break;
		case AV_OPT_TYPE_STRING:
		{
			if ( strEntry )
			{
				if ( item->opt->default_val.str )
				{
					strEntry->setText( tr(item->opt->default_val.str) );
				}
				else
				{
					strEntry->clear();
				}
			}
		}
		break;
		case AV_OPT_TYPE_RATIONAL:
		{
			if ( numEntry )
			{
				numEntry->setValue( item->opt->default_val.q.num );
			}

			if ( denEntry )
			{
				denEntry->setValue( item->opt->default_val.q.den );
			}
		}
		break;
		case AV_OPT_TYPE_BOOL:
		{
			if ( chkBox.size() > 0 )
			{
				chkBox[0]->setChecked( item->opt->default_val.i64 != 0 );
			}
		}
		break;
		case AV_OPT_TYPE_FLAGS:
		{
			if ( chkBox.size() > 0 )
			{
				for (size_t i=0; i<chkBox.size(); i++)
				{
					chkBox[i]->setChecked( (item->opt->default_val.i64 & item->units[i]->default_val.i64) ? true : false );
				}
			}
		}
		break;
		default:

		break;
	}
}
//-----------------------------------------------------
#endif
//**************************************************************************************
LibgwaviOptionsPage::LibgwaviOptionsPage(QWidget *parent)
	: QWidget(parent)
{
	QLabel *lbl;
	QVBoxLayout *vbox, *vbox1;
	//QHBoxLayout *hbox;
	QGridLayout *grid;
	QPushButton *videoConfBtn, *audioConfBtn;

	g_config->getOption("SDL.AviRecordAudio", &recordAudio);

	vbox1 = new QVBoxLayout();

	videoGbox = new QGroupBox( tr("Video:") );
	audioGbox = new QGroupBox( tr("Audio:") );

	audioGbox->setCheckable(false);
	//audioGbox->setChecked( aviGetAudioEnable() );

	videoEncSel     = new QComboBox();
	audioEncSel     = new QComboBox();
	videoPixfmt     = new QComboBox();
	audioSamplefmt  = new QComboBox();
	audioSampleRate = new QComboBox();
	audioChanLayout = new QComboBox();

	vbox1->addWidget( videoGbox );
	vbox1->addWidget( audioGbox );

	vbox = new QVBoxLayout();
	videoGbox->setLayout(vbox);

	grid = new QGridLayout();
	vbox->addLayout(grid);
	lbl  = new QLabel( tr("Encoder:") );
	grid->addWidget( lbl, 0, 0);
	grid->addWidget( videoEncSel, 0, 1);
	lbl  = new QLabel( tr("Pixel Format:") );
	grid->addWidget( lbl, 1, 0);
	grid->addWidget( videoPixfmt, 1, 1);
	videoConfBtn = new QPushButton( tr("Options...") );
	grid->addWidget( videoConfBtn, 2, 1);

	vbox = new QVBoxLayout();
	audioGbox->setLayout(vbox);

	grid = new QGridLayout();
	vbox->addLayout(grid);
	lbl  = new QLabel( tr("Encoder:") );
	grid->addWidget( lbl, 0, 0);
	grid->addWidget( audioEncSel, 0, 1 );
	lbl  = new QLabel( tr("Sample Format:") );
	grid->addWidget( lbl, 1, 0);
	grid->addWidget( audioSamplefmt, 1, 1);
	lbl  = new QLabel( tr("Sample Rate:") );
	grid->addWidget( lbl, 2, 0);
	grid->addWidget( audioSampleRate, 2, 1);
	lbl  = new QLabel( tr("Channel Layout:") );
	grid->addWidget( lbl, 3, 0);
	grid->addWidget( audioChanLayout, 3, 1);
	audioConfBtn = new QPushButton( tr("Options...") );
	grid->addWidget( audioConfBtn, 4, 1);

	initCodecLists();

	setLayout(vbox1);

	connect(videoEncSel, SIGNAL(currentIndexChanged(int)), this, SLOT(videoCodecChanged(int)));
	connect(audioEncSel, SIGNAL(currentIndexChanged(int)), this, SLOT(audioCodecChanged(int)));

	connect(videoPixfmt    , SIGNAL(currentIndexChanged(int)), this, SLOT(videoPixelFormatChanged(int)));
	connect(audioSamplefmt , SIGNAL(currentIndexChanged(int)), this, SLOT(audioSampleFormatChanged(int)));
	connect(audioSampleRate, SIGNAL(currentIndexChanged(int)), this, SLOT(audioSampleRateChanged(int)));
	connect(audioChanLayout, SIGNAL(currentIndexChanged(int)), this, SLOT(audioChannelLayoutChanged(int)));

	connect(videoConfBtn, SIGNAL(clicked(void)), this, SLOT(openVideoCodecOptions(void)));
	connect(audioConfBtn, SIGNAL(clicked(void)), this, SLOT(openAudioCodecOptions(void)));

	updateTimer = new QTimer(this);

	connect( updateTimer, &QTimer::timeout, this, &LibgwaviOptionsPage::periodicUpdate );

	updateTimer->start(200);
}
//-----------------------------------------------------
LibgwaviOptionsPage::~LibgwaviOptionsPage(void)
{
	updateTimer->stop();
}
//-----------------------------------------------------
void LibgwaviOptionsPage::periodicUpdate(void)
{
	audioGbox->setEnabled( recordAudio );
}
//-----------------------------------------------------
void LibgwaviOptionsPage::initCodecLists(void)
{
	int videoEncoder = aviGetSelVideoFormat();

	videoEncSel->addItem( tr("RGB24 (Uncompressed)"), AVI_RGB24 );
	videoEncSel->addItem( tr("I420  (YUV 4:2:0)")   , AVI_I420  );
	#ifdef _USE_X264
	videoEncSel->addItem( tr("X264  (H.264)")   , AVI_X264  );
	#endif
	#ifdef _USE_X265
	videoEncSel->addItem( tr("X265  (H.265)")   , AVI_X265  );
	#endif
	#ifdef WIN32
	videoEncSel->addItem( tr("VfW (Video for Windows)"), AVI_VFW);
	#endif

	for (int i=0; i<videoEncSel->count(); i++)
	{
		if ( videoEncoder == videoEncSel->itemData(i).toInt() )
		{
			videoEncSel->setCurrentIndex(i); break;
		}
	}
	audioEncSel->addItem( tr("Raw PCM (Uncompressed)"), 0 );

	int audioEncoder = audioEncSel->currentData().toInt();

	initPixelFormatSelect(videoFormat);
	initSampleFormatSelect(audioEncoder);
	initSampleRateSelect(audioEncoder);
	initChannelLayoutSelect(audioEncoder);
}
//-----------------------------------------------------
void LibgwaviOptionsPage::initPixelFormatSelect( int encoder )
{
	videoPixfmt->clear();
	videoPixfmt->addItem( tr("Auto"), -1 );

	switch ( encoder )
	{
		default:
		case AVI_I420:
			videoPixfmt->addItem( tr("YUV 420"), AVI_I420 );
		break;
#ifdef _USE_X264
		case AVI_X264:
			videoPixfmt->addItem( tr("YUV 420"), AVI_I420 );
		break;
#endif
#ifdef _USE_X265
		case AVI_X265:
			videoPixfmt->addItem( tr("YUV 420"), AVI_I420 );
		break;
#endif
#ifdef WIN32
		case AVI_VFW:
#endif
		case AVI_RGB24:
			videoPixfmt->addItem( tr("RGB24"), AVI_RGB24 );
		break;
	}
}
//-----------------------------------------------------
void LibgwaviOptionsPage::initSampleFormatSelect( int encoder )
{
	audioSamplefmt->clear();
	audioSamplefmt->addItem( tr("Auto"), -1 );
	audioSamplefmt->addItem( tr("S16 - Signed 16 Bit") ,  0 );
}
//-----------------------------------------------------
void LibgwaviOptionsPage::initSampleRateSelect( int encoder )
{
	audioSampleRate->clear();
	audioSampleRate->addItem( tr("Auto"), -1 );
}
//-----------------------------------------------------
void LibgwaviOptionsPage::initChannelLayoutSelect( int encoder )
{
	audioChanLayout->clear();
	audioChanLayout->addItem( tr("Auto"), -1 );
	audioChanLayout->addItem( tr("Mono"),  0 );
}
//-----------------------------------------------------
void LibgwaviOptionsPage::videoCodecChanged(int idx)
{
	aviSetSelVideoFormat( videoEncSel->currentData().toInt() );

	initPixelFormatSelect(videoFormat);
}
//-----------------------------------------------------
void LibgwaviOptionsPage::audioCodecChanged(int idx)
{
	int audioEncoder = audioEncSel->currentData().toInt();

	initSampleFormatSelect(audioEncoder);
	initSampleRateSelect(audioEncoder);
	initChannelLayoutSelect(audioEncoder);
}
//-----------------------------------------------------
void LibgwaviOptionsPage::openVideoCodecOptions(void)
{

}
//-----------------------------------------------------
void LibgwaviOptionsPage::openAudioCodecOptions(void)
{

}
//-----------------------------------------------------
void LibgwaviOptionsPage::videoPixelFormatChanged(int idx)
{

}
//-----------------------------------------------------
void LibgwaviOptionsPage::audioSampleFormatChanged(int idx)
{

}
//-----------------------------------------------------
void LibgwaviOptionsPage::audioSampleRateChanged(int idx)
{

}
//-----------------------------------------------------
void LibgwaviOptionsPage::audioChannelLayoutChanged(int idx)
{

}
//-----------------------------------------------------
//**************************************************************************************
