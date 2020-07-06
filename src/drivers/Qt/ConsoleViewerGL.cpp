// GameViewer.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <QApplication>
#include <QScreen>

#include "Qt/nes_shm.h"
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

    QScreen *screen = QGuiApplication::primaryScreen();

    if ( screen != NULL )
    {
		devPixRatio = screen->devicePixelRatio();
    	//printf("Ratio: %f \n", screen->devicePixelRatio() );
	}
	localBufSize = GL_NES_WIDTH * GL_NES_HEIGHT * sizeof(uint32_t);

	localBuf = (uint32_t*)malloc( localBufSize );

	if ( localBuf )
	{
		memset( localBuf, 0, localBufSize );
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

	glTexImage2D( GL_TEXTURE_RECTANGLE, 0, 
			GL_RGBA8, GL_NES_WIDTH, GL_NES_HEIGHT, 0,
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

void ConsoleViewGL_t::transfer2LocalBuffer(void)
{
	memcpy( localBuf, nes_shm->pixbuf, localBufSize );
}

void ConsoleViewGL_t::paintGL(void)
{
	int texture_width  = nes_shm->ncol;
	int texture_height = nes_shm->nrow;
	int l=0, r=texture_width;
	int t=0, b=texture_height;

	float xscale = (float)view_width  / (float)texture_width;
	float yscale = (float)view_height / (float)texture_height;

	if (xscale < yscale )
	{
		yscale = xscale;
	}
	else 
	{
		xscale = yscale;
	}
	int rw=(int)((r-l)*xscale);
	int rh=(int)((b-t)*yscale);
	int sx=(view_width-rw)/2;   
	int sy=(view_height-rh)/2;

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
		  	0, 0, GL_NES_WIDTH, GL_NES_HEIGHT,
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
