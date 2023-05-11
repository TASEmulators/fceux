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
// GameViewer.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
//#include <unistd.h>

#include "../../profiler.h"
#include "Qt/nes_shm.h"
#include "Qt/throttle.h"
#include "Qt/fceuWrapper.h"
#include "Qt/ConsoleViewerQWidget.h"
#include "Qt/ConsoleUtilities.h"
#include "Qt/ConsoleWindow.h"

extern unsigned int gui_draw_area_width;
extern unsigned int gui_draw_area_height;

ConsoleViewQWidget_t::ConsoleViewQWidget_t(QWidget *parent)
	: QWidget( parent )
{
	consoleWin_t *win = qobject_cast <consoleWin_t*>(parent);

	printf("Initializing QPainter Video Driver\n");

	QPalette pal = palette();

	pal.setColor(QPalette::Window, Qt::black);
	setAutoFillBackground(true);
	setPalette(pal);

	bgColor = nullptr;

	if ( win )
	{
		bgColor = win->getVideoBgColorPtr();
		bgColor->setRgb( 0, 0, 0 );
	}

	setMinimumWidth( 256 );
	setMinimumHeight( 224 );
	setFocusPolicy(Qt::StrongFocus);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	view_width  = GL_NES_WIDTH;
	view_height = GL_NES_HEIGHT;

	sx = sy = 0;
	rw = view_width;
	rh = view_height;
	sdlRendW = 0;
	sdlRendH = 0;
	xscale = 2.0;
	yscale = 2.0;

	devPixRatio = 1.0f;
	aspectRatio = 1.0f;
	aspectX     = 1.0f;
	aspectY     = 1.0f;

	vsyncEnabled = false;
	mouseButtonMask = 0;

	localBufSize = (4 * GL_NES_WIDTH) * (4 * GL_NES_HEIGHT) * sizeof(uint32_t);

	localBuf = (uint32_t*)malloc( localBufSize );

	if ( localBuf )
	{
		memset32( localBuf, alphaMask, localBufSize );
	}

	forceAspect  = true;
	autoScaleEna = true;
	linearFilter = false;

	if ( g_config )
	{
		int opt;
		g_config->getOption("SDL.OpenGLip", &opt );
		
		linearFilter = (opt) ? true : false;

		g_config->getOption ("SDL.AutoScale", &opt);

		autoScaleEna = (opt) ? true : false;

		g_config->getOption("SDL.XScale", &xscale);
		g_config->getOption("SDL.YScale", &yscale);

		g_config->getOption ("SDL.ForceAspect", &forceAspect);

		if ( bgColor )
		{
			fceuLoadConfigColor( "SDL.VideoBgColor", bgColor );
		}
		g_config->getOption ("SDL.VideoVsync", &vsyncEnabled);
	}
}

ConsoleViewQWidget_t::~ConsoleViewQWidget_t(void)
{
	//printf("Destroying QPainter Viewport\n");

	if ( localBuf )
	{
		free( localBuf ); localBuf = nullptr;
	}
	cleanup();
}

void ConsoleViewQWidget_t::setBgColor( QColor &c )
{
	if ( bgColor )
	{
		*bgColor = c;
	}
}

void ConsoleViewQWidget_t::setVsyncEnable( bool ena )
{
	if ( vsyncEnabled != ena )
	{
		vsyncEnabled = ena;

		reset();
	}
}

void ConsoleViewQWidget_t::setLinearFilterEnable( bool ena )
{
   if ( ena != linearFilter )
   {
      linearFilter = ena;

	   reset();
   }
}

void ConsoleViewQWidget_t::setScaleXY( double xs, double ys )
{
	xscale = xs;
	yscale = ys;

	if ( forceAspect )
	{
		if ( xscale < yscale )
		{
			yscale = xscale;
		}
		else 
		{
			xscale = yscale;
		}
	}
}

void ConsoleViewQWidget_t::setAspectXY( double x, double y )
{
	aspectX = x;
	aspectY = y;

	aspectRatio = aspectY / aspectX;
}

void ConsoleViewQWidget_t::getAspectXY( double &x, double &y )
{
	x = aspectX;
	y = aspectY;
}

double ConsoleViewQWidget_t::getAspectRatio(void)
{
	return aspectRatio;
}

void ConsoleViewQWidget_t::transfer2LocalBuffer(void)
{
	int i=0, hq = 0, bufIdx;
	int numPixels = nes_shm->video.ncol * nes_shm->video.nrow;
	unsigned int cpSize = numPixels * 4;
 	uint8_t *src, *dest;

	bufIdx = nes_shm->pixBufIdx-1;

	if ( bufIdx < 0 )
	{
		bufIdx = NES_VIDEO_BUFLEN-1;
	}
	if ( cpSize > localBufSize )
	{
		cpSize = localBufSize;
	}
	src  = (uint8_t*)nes_shm->pixbuf[bufIdx];
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
		//memcpy( localBuf, src, cpSize );
		copyPixels32( dest, src, cpSize, alphaMask);
	}
}

int ConsoleViewQWidget_t::init(void)
{
	return 0;
}

void ConsoleViewQWidget_t::cleanup(void)
{

}

void ConsoleViewQWidget_t::reset(void)
{
	cleanup();
	if ( init() == 0 )
  	{
	
	}
  	else 
	{
		cleanup();
	}
}

void ConsoleViewQWidget_t::setCursor(const QCursor &c)
{
	QWidget::setCursor(c);
}

void ConsoleViewQWidget_t::setCursor( Qt::CursorShape s )
{
	QWidget::setCursor(s);
}

void ConsoleViewQWidget_t::showEvent(QShowEvent *event)
{
	//printf("SDL Show: %i x %i \n", width(), height() );

	//view_width  = width();
	//view_height = height();

	//gui_draw_area_width = view_width;
	//gui_draw_area_height = view_height;

	//reset();
}

void ConsoleViewQWidget_t::resizeEvent(QResizeEvent *event)
{
	QSize s;

	s = event->size();
	view_width  = s.width();
	view_height = s.height();
	printf("QWidget Resize: %i x %i \n", view_width, view_height);

	gui_draw_area_width = view_width;
	gui_draw_area_height = view_height;

	reset();
}

void ConsoleViewQWidget_t::mousePressEvent(QMouseEvent * event)
{
	//printf("Mouse Button Press: (%i,%i) %x  %x\n", 
	//		event->pos().x(), event->pos().y(), event->button(), event->buttons() );

	mouseButtonMask = event->buttons();
}

void ConsoleViewQWidget_t::mouseReleaseEvent(QMouseEvent * event)
{
	//printf("Mouse Button Release: (%i,%i) %x  %x\n", 
	//		event->pos().x(), event->pos().y(), event->button(), event->buttons() );

	mouseButtonMask = event->buttons();
}

bool ConsoleViewQWidget_t::getMouseButtonState( unsigned int btn )
{
	bool isPressed = false;

	if ( mouseButtonMask & btn )
	{
		isPressed = true;
	}
	else
	{	// Check SDL mouse state just in case SDL is intercepting 
		// mouse events from window system causing Qt not to see them.
		int x, y;
        	uint32_t b;
		b = SDL_GetMouseState( &x, &y);
		
		if ( btn & Qt::LeftButton )
		{
			if ( b & SDL_BUTTON(SDL_BUTTON_LEFT) )
			{
				isPressed = true;
			}
		}

		if ( btn & Qt::RightButton )
		{
			if ( b & SDL_BUTTON(SDL_BUTTON_RIGHT) )
			{
				isPressed = true;
			}
		}

		if ( btn & Qt::MiddleButton )
		{
			if ( b & SDL_BUTTON(SDL_BUTTON_MIDDLE) )
			{
				isPressed = true;
			}
		}
	}
	return isPressed;
}

void  ConsoleViewQWidget_t::getNormalizedCursorPos( double &x, double &y )
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

void ConsoleViewQWidget_t::paintEvent(QPaintEvent *event)
{
	QPainter painter(this);
	int nesWidth  = GL_NES_WIDTH;
	int nesHeight = GL_NES_HEIGHT;
	float ixScale = 1.0;
	float iyScale = 1.0;

	if ( nes_shm != nullptr )
	{
		nesWidth  = nes_shm->video.ncol;
		nesHeight = nes_shm->video.nrow;
		ixScale   = (float)nes_shm->video.xscale;
		iyScale   = (float)nes_shm->video.yscale;
	}
	//printf(" %i x %i \n", nesWidth, nesHeight );
	float xscaleTmp = (float)view_width  / (float)nesWidth;
	float yscaleTmp = (float)view_height / (float)nesHeight;

	xscaleTmp *= ixScale;
	yscaleTmp *= iyScale;

	if ( forceAspect )
	{
		if ( xscaleTmp < yscaleTmp )
		{
			yscaleTmp = xscaleTmp;
		}
		else 
		{
			xscaleTmp = yscaleTmp;
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

	rw=(int)(nesWidth*xscaleTmp/ixScale);
	rh=(int)(nesHeight*yscaleTmp/iyScale);

	if ( forceAspect )
	{
		int iw, ih, ax, ay;

		ax = (int)(aspectX+0.50);
		ay = (int)(aspectY+0.50);

		iw = rw * ay;
		ih = rh * ax;
		
		if ( iw > ih )
		{
			rh = (rw * ay) / ax;
		}
		else
		{
			rw = (rh * ax) / ay;
		}

		if ( rw > view_width )
		{
			rw = view_width;
			rh = (rw * ay) / ax;
		}

		if ( rh > view_height )
		{
			rh = view_height;
			rw = (rh * ax) / ay;
		}
	}

	if ( rw > view_width ) rw = view_width;
	if ( rh > view_height) rh = view_height;

	sx=(view_width-rw)/2;   
	sy=(view_height-rh)/2;

	if ( bgColor )
	{
		painter.fillRect( 0, 0, view_width, view_height, *bgColor );
	}
	else
	{
		painter.fillRect( 0, 0, view_width, view_height, Qt::black );
	}
	painter.setRenderHint( QPainter::SmoothPixmapTransform, linearFilter );

	int rowPitch = nesWidth * sizeof(uint32_t);

	QImage tmpImage( (const uchar*)localBuf, nesWidth, nesHeight, rowPitch, QImage::Format_ARGB32);

	//SDL_Rect source = {0, 0, nesWidth, nesHeight };
	QRect dest( sx, sy, rw, rh );

	painter.drawImage( dest, tmpImage );

	videoBufferSwapMark();

	nes_shm->render_count++;
}
