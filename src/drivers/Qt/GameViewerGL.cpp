// GameViewer.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "Qt/gl_win.h"
#include "Qt/GameViewerGL.h"

extern unsigned int gui_draw_area_width;
extern unsigned int gui_draw_area_height;

gameViewGL_t::gameViewGL_t(QWidget *parent)
	: QOpenGLWidget( parent )
{
	view_width  = 0;
	view_height = 0;
	gltexture   = 0;

	//QSurfaceFormat  fmt = QSurfaceFormat::defaultFormat();

	//fmt.setRedBufferSize(8);
	//fmt.setGreenBufferSize(8);
	//fmt.setBlueBufferSize(8);
	//fmt.setAlphaBufferSize(8);
	//fmt.setDepthBufferSize( 24 );
	//setTextureFormat(GL_RGBA8);

	////printf("R: %i \n", fmt.redBufferSize() );
	////printf("G: %i \n", fmt.greenBufferSize() );
	////printf("B: %i \n", fmt.blueBufferSize() );
	////printf("A: %i \n", fmt.alphaBufferSize() );
	////printf("SW: %i \n", fmt.swapBehavior() );

	//setFormat( fmt );
}

gameViewGL_t::~gameViewGL_t(void)
{
	// Make sure the context is current and then explicitly
    // destroy all underlying OpenGL resources.
    makeCurrent();

	 // Free GL texture
	 if (gltexture) 
	 {
	 	printf("Destroying GLX Texture\n");
	 	glDeleteTextures(1, &gltexture);
	 	gltexture=0;
	 }

	 doneCurrent();
}

void gameViewGL_t::initializeGL(void)
{
	int ipolate = 0;

	 initializeOpenGLFunctions();
    // Set up the rendering context, load shaders and other resources, etc.:
    //QOpenGLFunctions *gl = QOpenGLContext::currentContext()->functions();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	 printf("GL Init!\n");

	 //glEnable(GL_TEXTURE_2D);
	 glEnable(GL_TEXTURE_RECTANGLE);

	 if ( gltexture )
	 {
	 	printf("GL Texture already exists\n");
	 }
	 else
	 {
    	glGenTextures(1, &gltexture);
	 }
	 printf("Linear Interpolation on GL Texture: %s \n", ipolate ? "Enabled" : "Disabled");

	 //glBindTexture(GL_TEXTURE_2D, gltexture);

	 //glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,ipolate?GL_LINEAR:GL_NEAREST);
	 //glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,ipolate?GL_LINEAR:GL_NEAREST);
	 //glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	 //glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);

	 glBindTexture(GL_TEXTURE_RECTANGLE, gltexture);

	 glTexParameteri(GL_TEXTURE_RECTANGLE,GL_TEXTURE_MAG_FILTER,ipolate?GL_LINEAR:GL_NEAREST);
	 glTexParameteri(GL_TEXTURE_RECTANGLE,GL_TEXTURE_MIN_FILTER,ipolate?GL_LINEAR:GL_NEAREST);
	 glTexParameteri(GL_TEXTURE_RECTANGLE,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	 glTexParameteri(GL_TEXTURE_RECTANGLE,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
}

void gameViewGL_t::resizeGL(int w, int h)
{
	printf("GL Resize: %i x %i \n", w, h );
	glViewport(0, 0, w, h);

	view_width  = w;
	view_height = h;

	gui_draw_area_width = w;
	gui_draw_area_height = h;
}

void gameViewGL_t::paintGL(void)
{
	int l=0, r=gl_shm->ncol;
	int t=0, b=gl_shm->nrow;

	float xscale = (float)view_width  / (float)gl_shm->ncol;
	float yscale = (float)view_height / (float)gl_shm->nrow;

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
	//glViewport( 0, 0, screen_width, screen_height);

	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	//glOrtho( -1.0,  1.0,  -1.0,  1.0,  -1.0,  1.0);
	glOrtho( 0.0,  rw,  0.0,  rh,  -1.0,  1.0);

	glDisable(GL_DEPTH_TEST);
	glClearColor( 0.0, 0.0f, 0.0f, 0.0f);	// Background color to black.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//glEnable(GL_LINE_SMOOTH);
	//glEnable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_RECTANGLE);
	//glBindTexture(GL_TEXTURE_2D, gltexture);
	glBindTexture(GL_TEXTURE_RECTANGLE, gltexture);

	//print_pixbuf();
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 256, 0,
	//				GL_RGBA, GL_UNSIGNED_BYTE, gl_shm->pixbuf );
	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA8, 256, 256, 0,
					GL_RGBA, GL_UNSIGNED_BYTE, gl_shm->pixbuf );

	glBegin(GL_QUADS);
	//glTexCoord2f(1.0f*l/256, 1.0f*b/256); // Bottom left of picture.
	//glVertex2f(-1.0f, -1.0f);	// Bottom left of target.
	glTexCoord2f( l, b); // Bottom left of picture.
	glVertex2f( 0.0, 0.0f);	// Bottom left of target.

	//glTexCoord2f(1.0f*r/256, 1.0f*b/256);// Bottom right of picture.
	glTexCoord2f(r, b);// Bottom right of picture.
	glVertex2f( rw, 0.0f);	// Bottom right of target.

	//glTexCoord2f(1.0f*r/256, 1.0f*t/256); // Top right of our picture.
	glTexCoord2f(r, t); // Top right of our picture.
	glVertex2f( rw,  rh);	// Top right of target.

	//glTexCoord2f(1.0f*l/256, 1.0f*t/256);  // Top left of our picture.
	glTexCoord2f(l, t);  // Top left of our picture.
	glVertex2f( 0.0f,  rh);	// Top left of target.
	glEnd();

	//glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_RECTANGLE);

	//glColor4f( 1.0, 1.0, 1.0, 1.0 );
	//glLineWidth(5.0);
	//glBegin(GL_LINES);
	//glVertex2f(-1.0f, -1.0f);	// Bottom left of target.
	//glVertex2f( 1.0f,  1.0f);	// Top right of target.
	//glEnd();

	//context()->swapBuffers( context()->surface() );
	//if ( double_buffer_ena )
	//{
	//	glXSwapBuffers( dpy, win );
	//}
	//else
	//{
	//	glFlush();
	//}
	//glFlush();

	//float x1, y1, x2, y2;

	//angle += (2.0 * 3.14 * 0.01); 
	// 
   //glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	//x1 = cos( angle );
	//y1 = sin( angle );

	//x2 = -x1;
	//y2 = -y1;

	//glColor4f( 1.0, 1.0, 1.0, 1.0 );
	//glLineWidth(5.0);

	//glBegin(GL_LINES);
	//    glVertex2f( x1, y1 );
	//    glVertex2f( x2, y2 );
	//glEnd();

	 //printf("Paint GL!\n");
}
