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
	use_sw_pix_remap = 0;
	remap_pixBuf = NULL;
	remap_pixPtr = NULL;
	devPixRatio  = 1.0f;

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

	 if ( remap_pixBuf )
	 {
		::free( remap_pixBuf ); remap_pixBuf = NULL;
	 }
	 if ( remap_pixPtr )
	 {
		::free( remap_pixPtr ); remap_pixPtr = NULL;
	 }
}

int gameViewGL_t::init( double devPixRatioInput )
{
	devPixRatio = devPixRatioInput;
	return 0;
}

void gameViewGL_t::buildTextures(void)
{
	int ipolate = 0;

	//glEnable(GL_TEXTURE_2D);
	 glEnable(GL_TEXTURE_RECTANGLE);

	 if ( gltexture )
	 {
	 	glDeleteTextures(1, &gltexture);
	 	gltexture=0;
	 }

   glGenTextures(1, &gltexture);
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

	 if ( remap_pixBuf )
	 {
		::free( remap_pixBuf ); remap_pixBuf = NULL;
	 }
	 if ( remap_pixPtr )
	 {
		::free( remap_pixPtr ); remap_pixPtr = NULL;
	 }

	 if ( use_sw_pix_remap )
	 {
	 	texture_width  = view_width;
	 	texture_height = view_height;
	 }
	 else
	 {
	 	texture_width  = GL_NES_WIDTH;
	 	texture_height = GL_NES_HEIGHT;
	 }
	 if ( texture_width < GL_NES_WIDTH )
	 {
	 	 texture_width = GL_NES_WIDTH;
	 }
	 if ( texture_height < GL_NES_HEIGHT )
	 {
	 	 texture_height = GL_NES_HEIGHT;
	 }

	 if ( use_sw_pix_remap )
	 {
		remap_pixBuf = (uint32_t*)malloc( texture_width * texture_height * sizeof(uint32_t) );

		if ( remap_pixBuf )
		{
			memset( remap_pixBuf, 0, texture_width * texture_height * sizeof(uint32_t) );
		}

		remap_pixPtr = (uint32_t**)malloc( texture_width * texture_height * sizeof(uintptr_t) );

		if ( remap_pixPtr )
		{
			memset( remap_pixPtr, 0, texture_width * texture_height * sizeof(uintptr_t) );
		}
		calcPixRemap();
	 }

	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, 
			GL_RGBA8, texture_width, texture_height, 0,
					GL_BGRA, GL_UNSIGNED_BYTE, 0 );

}

void gameViewGL_t::initializeGL(void)
{

	 initializeOpenGLFunctions();
    // Set up the rendering context, load shaders and other resources, etc.:
    //QOpenGLFunctions *gl = QOpenGLContext::currentContext()->functions();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	 printf("GL Init!\n");

	 buildTextures();
}

void gameViewGL_t::resizeGL(int w, int h)
{
	w = (int)( devPixRatio * w );
	h = (int)( devPixRatio * h );
	printf("GL Resize: %i x %i \n", w, h );
	glViewport(0, 0, w, h);

	view_width  = w;
	view_height = h;

	gui_draw_area_width = w;
	gui_draw_area_height = h;

	buildTextures();
}

void gameViewGL_t::paintGL(void)
{
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
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_RECTANGLE);
	//glBindTexture(GL_TEXTURE_2D, gltexture);
	glBindTexture(GL_TEXTURE_RECTANGLE, gltexture);

	//print_pixbuf();
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 256, 0,
	//				GL_RGBA, GL_UNSIGNED_BYTE, gl_shm->pixbuf );
	//glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA8, 256, 256, 0,
	//				GL_BGRA, GL_UNSIGNED_BYTE, gl_shm->pixbuf );
	if ( use_sw_pix_remap )
	{
		doRemap();

		glTexSubImage2D(GL_TEXTURE_RECTANGLE, 0,
			  	0, 0, texture_width, texture_height,
					GL_BGRA, GL_UNSIGNED_BYTE, remap_pixBuf );
	}
	else
	{
		glTexSubImage2D(GL_TEXTURE_RECTANGLE, 0,
			  	0, 0, texture_width, texture_height,
					GL_BGRA, GL_UNSIGNED_BYTE, gl_shm->pixbuf );
	}

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

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_RECTANGLE);

	 //printf("Paint GL!\n");
}

void gameViewGL_t::doRemap(void)
{
	int x,y,i;

	i=0;
	for (x=0; x<texture_width; x++)
	{
		for (y=0; y<texture_height; y++)
		{
			if ( remap_pixPtr[i] == NULL )
			{
				remap_pixBuf[i] = 0x00000000;
			}
			else
			{
				remap_pixBuf[i] = *remap_pixPtr[i];
			}
			i++;
		}
	}

}

void gameViewGL_t::calcPixRemap(void)
{
	int w, h, s;
	int i, j, x, y;
	int   sw, sh, rx, ry, gw, gh;
	int llx, lly, urx, ury;
	float sx, sy, nw, nh;

	w = view_width;
	h = view_height;

	s = w * h * 4;

	gw = gl_shm->ncol;
	gh = gl_shm->nrow;

	sx = (float)w / (float)gw;
	sy = (float)h / (float)gh;

	if (sx < sy )
	{
		sy = sx;
	}
	else 
	{
		sx = sy;
	}

	sw = (int) ( (float)gw * sx );
	sh = (int) ( (float)gh * sy );

	llx = (w - sw) / 2;
	lly = (h - sh) / 2;
	urx = llx + sw;
	ury = lly + sh;

	i=0;
	for (y=0; y<h; y++)
	{
		if ( (y < lly) || (y > ury) )
		{
			for (x=0; x<w; x++)
			{
				remap_pixPtr[i] = 0; i++;
			}
		}
		else
		{
			for (x=0; x<w; x++)
			{
				if ( (x < llx) || (x > urx) )
				{
					remap_pixPtr[i] = 0; i++;
				}
				else
				{
					nw = (float)(x - llx) / (float)sw;
					nh = (float)(y - lly) / (float)sh;

					rx = (int)((float)gw * nw);
					ry = (int)((float)gh * nh);

					if ( rx < 0 )
					{
						rx = 0;
					}
					else if ( rx >= GL_NES_WIDTH )
					{
						rx = GL_NES_WIDTH-1;
					}
					if ( ry < 0 )
					{
						ry = 0;
					}
					else if ( ry >= GL_NES_HEIGHT )
					{
						ry = GL_NES_HEIGHT-1;
					}

					j = (ry * GL_NES_WIDTH) + rx;

					remap_pixPtr[i] = &gl_shm->pixbuf[j]; i++;

					//printf("Remap: (%i,%i)=%i   (%i,%i)=%i \n", x,y,i, rx,ry,j );
				}
			}
		}
	}
	//numRendLines = gh;
}
