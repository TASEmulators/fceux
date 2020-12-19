// GameViewer.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <QApplication>
#include <QScreen>
#include <QMouseEvent>

#include "Qt/nes_shm.h"
#include "Qt/fceuWrapper.h"
#include "Qt/ConsoleViewerGL.h"

extern unsigned int gui_draw_area_width;
extern unsigned int gui_draw_area_height;

ConsoleViewGL_t::ConsoleViewGL_t(QWidget *parent)
	: QOpenGLWidget( parent )
{
	view_width  = 0;
	view_height = 0;
	gltexture   = 0;
	devPixRatio = 1.0f;
	linearFilter = false;
	sqrPixels    = true;
	autoScaleEna = true;
	xscale = 2.0;
	yscale = 2.0;
	sx = 0; sy = 0;
	rw = 256;
	rh = 240;
	mouseButtonMask = 0;

	setMinimumWidth( GL_NES_WIDTH );
	setMinimumHeight( GL_NES_HEIGHT );

	QScreen *screen = QGuiApplication::primaryScreen();

	if ( screen != NULL )
	{
		devPixRatio = screen->devicePixelRatio();
		//printf("Ratio: %f \n", screen->devicePixelRatio() );
	}
	localBufSize = (4 * GL_NES_WIDTH) * (4 * GL_NES_HEIGHT) * sizeof(uint32_t);

	localBuf = (uint32_t*)malloc( localBufSize );

	if ( localBuf )
	{
		memset( localBuf, 0, localBufSize );
	}

	linearFilter = false;

	if ( g_config )
	{
		int opt;
		g_config->getOption("SDL.OpenGLip", &opt );
		
		linearFilter = (opt) ? true : false;
	}
}

ConsoleViewGL_t::~ConsoleViewGL_t(void)
{
	// Make sure the context is current and then explicitly
    // destroy all underlying OpenGL resources.
    makeCurrent();

	 // Free GL texture
	 if (gltexture) 
	 {
	 	//printf("Destroying GL Texture\n");
	 	glDeleteTextures(1, &gltexture);
	 	gltexture=0;
	 }

	 doneCurrent();

	if ( localBuf )
	{
		free( localBuf ); localBuf = NULL;
	}
}

int ConsoleViewGL_t::init( void )
{
	return 0;
}

void ConsoleViewGL_t::buildTextures(void)
{
	int w, h;
	 glEnable(GL_TEXTURE_RECTANGLE);

	 if ( gltexture )
	 {
	 	glDeleteTextures(1, &gltexture);
	 	gltexture=0;
	 }

	glGenTextures(1, &gltexture);
	//printf("Linear Interpolation on GL Texture: %s \n", linearFilter ? "Enabled" : "Disabled");

	glBindTexture( GL_TEXTURE_RECTANGLE, gltexture);

	glTexParameteri( GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, linearFilter ? GL_LINEAR : GL_NEAREST );
	glTexParameteri( GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, linearFilter ? GL_LINEAR : GL_NEAREST );
	glTexParameteri( GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	w = nes_shm->video.ncol;
	h = nes_shm->video.nrow;

	glTexImage2D( GL_TEXTURE_RECTANGLE, 0, 
			GL_RGBA8, w, h, 0,
					GL_BGRA, GL_UNSIGNED_BYTE, 0 );

}

void ConsoleViewGL_t::initializeGL(void)
{

	 initializeOpenGLFunctions();
    // Set up the rendering context, load shaders and other resources, etc.:
    //QOpenGLFunctions *gl = QOpenGLContext::currentContext()->functions();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	 //printf("GL Init!\n");

	 buildTextures();
}

void ConsoleViewGL_t::resizeGL(int w, int h)
{
	w = (int)( devPixRatio * w );
	h = (int)( devPixRatio * h );
	//printf("GL Resize: %i x %i \n", w, h );
	glViewport(0, 0, w, h);

	view_width  = w;
	view_height = h;

	gui_draw_area_width = w;
	gui_draw_area_height = h;

	buildTextures();
}

void ConsoleViewGL_t::setLinearFilterEnable( bool ena )
{
   if ( linearFilter != ena )
   {
      linearFilter = ena;

	   buildTextures();
   }
}

void ConsoleViewGL_t::setScaleXY( double xs, double ys )
{
	float xyRatio   = (float)nes_shm->video.xyRatio;

	xscale = xs;
	yscale = ys;

	if ( sqrPixels )
	{
		if ( (xscale*xyRatio) < yscale )
		{
			yscale = (xscale*xyRatio);
		}
		else 
		{
			xscale = (yscale/xyRatio);
		}
	}
}

void ConsoleViewGL_t::transfer2LocalBuffer(void)
{
	int i=0, hq = 0;
	int numPixels = nes_shm->video.ncol * nes_shm->video.nrow;
	int cpSize = numPixels * 4;
 	uint8_t *src, *dest;

	if ( cpSize > localBufSize )
	{
		cpSize = localBufSize;
	}
	src  = (uint8_t*)nes_shm->pixbuf;
	dest = (uint8_t*)localBuf;

	hq = (nes_shm->video.preScaler == 1) || (nes_shm->video.preScaler == 4); // hq2x and hq3x

	if ( hq )
	{
		for (i=0; i<numPixels; i++)
		{
			dest[3] = 0xFF;
			dest[1] = src[1];
			dest[2] = src[2];
			dest[0] = src[0];

			src += 4; dest += 4;
		}
	}
	else
	{
		memcpy( localBuf, nes_shm->pixbuf, cpSize );
	}
}

void ConsoleViewGL_t::mousePressEvent(QMouseEvent * event)
{
	//printf("Mouse Button Press: (%i,%i) %x  %x\n", 
	//		event->pos().x(), event->pos().y(), event->button(), event->buttons() );

	mouseButtonMask = event->buttons();
}

void ConsoleViewGL_t::mouseReleaseEvent(QMouseEvent * event)
{
	//printf("Mouse Button Release: (%i,%i) %x  %x\n", 
	//		event->pos().x(), event->pos().y(), event->button(), event->buttons() );

	mouseButtonMask = event->buttons();
}

bool ConsoleViewGL_t::getMouseButtonState( unsigned int btn )
{
	return (mouseButtonMask & btn) ? true : false;
}

void  ConsoleViewGL_t::getNormalizedCursorPos( double &x, double &y )
{
	QPoint cursor;

	cursor = QCursor::pos();

	//printf("Global Cursor (%i,%i) \n", cursor.x(), cursor.y() );

	cursor = mapFromGlobal( cursor );

	//printf("Window Cursor (%i,%i) \n", cursor.x(), cursor.y() );

	x = (double)(cursor.x() - sx) / (double)rw;
	y = (double)(cursor.y() - sy) / (double)rh;

	if ( x < 0.0 )
	{
		x = 0.0;
	}
	else if ( x > 1.0 )
	{
		x = 1.0;
	}
	if ( y < 0.0 )
	{
		y = 0.0;
	}
	else if ( y > 1.0 )
	{
		y = 1.0;
	}
	//printf("Normalized Cursor (%f,%f) \n", x, y );
}

void ConsoleViewGL_t::paintGL(void)
{
	int texture_width  = nes_shm->video.ncol;
	int texture_height = nes_shm->video.nrow;
	int l=0, r=texture_width;
	int t=0, b=texture_height;

	float xyRatio   = (float)nes_shm->video.xyRatio;
	float xscaleTmp = (float)(view_width)  / (float)(texture_width);
	float yscaleTmp = (float)(view_height) / (float)(texture_height);

	if ( sqrPixels )
	{
		if ( (xscaleTmp*xyRatio) < yscaleTmp )
		{
			yscaleTmp = (xscaleTmp*xyRatio);
		}
		else 
		{
			xscaleTmp = (yscaleTmp/xyRatio);
		}
	}

	if ( autoScaleEna )
	{
		xscale = xscaleTmp;
		yscale = yscaleTmp;
	}
	else
	{
		if ( xscaleTmp > xscale )
		{
			xscaleTmp = xscale;
		}
		if ( yscaleTmp > yscale )
		{
			yscaleTmp = yscale;
		}
	}
	rw=(int)((r-l)*xscaleTmp);
	rh=(int)((b-t)*yscaleTmp);
	sx=(view_width-rw)/2;   
	sy=(view_height-rh)/2;

	glViewport(sx, sy, rw, rh);

	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glOrtho( 0.0,  rw,  0.0,  rh,  -1.0,  1.0);

	glDisable(GL_DEPTH_TEST);
	glClearColor( 0.0, 0.0f, 0.0f, 0.0f);	// Background color to black.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_RECTANGLE);
	glBindTexture(GL_TEXTURE_RECTANGLE, gltexture);

	glTexSubImage2D(GL_TEXTURE_RECTANGLE, 0,
		  	0, 0, texture_width, texture_height,
				GL_BGRA, GL_UNSIGNED_BYTE, localBuf );

	glBegin(GL_QUADS);
	glTexCoord2f( l, b); // Bottom left of picture.
	glVertex2f( 0.0, 0.0f);	// Bottom left of target.

	glTexCoord2f(r, b);// Bottom right of picture.
	glVertex2f( rw, 0.0f);	// Bottom right of target.

	glTexCoord2f(r, t); // Top right of our picture.
	glVertex2f( rw,  rh);	// Top right of target.

	glTexCoord2f(l, t);  // Top left of our picture.
	glVertex2f( 0.0f,  rh);	// Top left of target.
	glEnd();

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_RECTANGLE);

	 //printf("Paint GL!\n");
}
