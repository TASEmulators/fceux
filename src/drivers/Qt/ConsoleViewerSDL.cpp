// GameViewer.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include "Qt/nes_shm.h"
#include "Qt/ConsoleViewerSDL.h"

extern unsigned int gui_draw_area_width;
extern unsigned int gui_draw_area_height;

ConsoleViewSDL_t::ConsoleViewSDL_t(QWidget *parent)
	: QWidget( parent )
{
	view_width  = GL_NES_WIDTH;
	view_height = GL_NES_HEIGHT;

	sx = sy = 0;
	rw = view_width;
	rh = view_height;
	sdlRendW = 0;
	sdlRendH = 0;

	devPixRatio = 1.0f;
	sdlWindow   = NULL;
	sdlRenderer = NULL;
	sdlTexture  = NULL;

	vsyncEnabled = false;
}

ConsoleViewSDL_t::~ConsoleViewSDL_t(void)
{

}

int ConsoleViewSDL_t::init(void)
{
	WId windowHandle;

	if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) 
	{
		printf("[SDL] Failed to initialize video subsystem.\n");
		return -1;
	}
	//else
	//{
	//	printf("Initialized SDL Video Subsystem\n");
	//}

	windowHandle = this->winId();

	//printf("Window Handle: %llu \n", windowHandle );

	//sleep(1);

	sdlWindow = SDL_CreateWindowFrom( (void*)windowHandle);
	if (sdlWindow == NULL) 
	{
		printf("[SDL] Failed to create window from handle.\n");
		return -1;
	}

	uint32_t baseFlags = vsyncEnabled ? SDL_RENDERER_PRESENTVSYNC : 0;

	sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, baseFlags | SDL_RENDERER_ACCELERATED);

	if (sdlRenderer == NULL) 
	{
		printf("[SDL] Failed to create accelerated renderer.\n");

		printf("[SDL] Attempting to create software renderer...\n");

		sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, baseFlags | SDL_RENDERER_SOFTWARE);

		if (sdlRenderer == NULL)
	  	{
			printf("[SDL] Failed to create software renderer.\n");
			return -1;
		}		
	}

	SDL_GetRendererOutputSize( sdlRenderer, &sdlRendW, &sdlRendH );

	printf("[SDL] Renderer Output Size: %i x %i \n", sdlRendW, sdlRendH );

	sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, GL_NES_WIDTH, GL_NES_HEIGHT);

	if (sdlTexture == NULL) 
	{
		printf("[SDL] Failed to create texture: %i x %i", GL_NES_WIDTH, GL_NES_HEIGHT );
		return -1;
	}

	return 0;
}

void ConsoleViewSDL_t::cleanup(void)
{
	if (sdlTexture) 
	{
		SDL_DestroyTexture(sdlTexture);
		sdlTexture = NULL;		
	}
	if (sdlRenderer) 
	{
		SDL_DestroyRenderer(sdlRenderer);
		sdlRenderer = NULL;
	}
}

void ConsoleViewSDL_t::reset(void)
{
	cleanup();
	if ( init() == 0 )
  	{
		//console->GetVideoRenderer()->RegisterRenderingDevice(this);
	}
  	else 
	{
		cleanup();
	}
}

void ConsoleViewSDL_t::resizeEvent(QResizeEvent *event)
{
	QSize s;

	s = event->size();
	view_width  = s.width();
	view_height = s.height();
	//printf("SDL Resize: %i x %i \n", view_width, view_height);

	reset();

	sdlViewport.x = sdlRendW - view_width;
	sdlViewport.y = sdlRendH - view_height;
	sdlViewport.w = view_width;
	sdlViewport.h = view_height;
}

void ConsoleViewSDL_t::paintEvent( QPaintEvent *event )
{
	int nesWidth  = GL_NES_WIDTH;
	int nesHeight = GL_NES_HEIGHT;

	if ( nes_shm != NULL )
	{
		nesWidth  = nes_shm->ncol;
		nesHeight = nes_shm->nrow;
	}
	//printf(" %i x %i \n", nesWidth, nesHeight );
	float xscale = (float)view_width  / (float)nesWidth;
	float yscale = (float)view_height / (float)nesHeight;

	if (xscale < yscale )
	{
		yscale = xscale;
	}
	else 
	{
		xscale = yscale;
	}

	rw=(int)(nesWidth*xscale);
	rh=(int)(nesHeight*yscale);
	sx=sdlViewport.x + (view_width-rw)/2;   
	sy=sdlViewport.y + (view_height-rh)/2;

	if ( (sdlRenderer == NULL) || (sdlTexture == NULL) )
  	{
		return;
	}

	SDL_SetRenderDrawColor( sdlRenderer, 0, 0, 0, 0 );

	SDL_RenderClear(sdlRenderer);

	uint8_t *textureBuffer;
	int rowPitch;
	SDL_LockTexture( sdlTexture, nullptr, (void**)&textureBuffer, &rowPitch);
	{
		memcpy( textureBuffer, nes_shm->pixbuf, GL_NES_HEIGHT*GL_NES_WIDTH*sizeof(uint32_t) );
	}
	SDL_UnlockTexture(sdlTexture);

	SDL_RenderSetViewport( sdlRenderer, &sdlViewport );

	SDL_Rect source = {0, 0, GL_NES_WIDTH, GL_NES_HEIGHT };
	SDL_Rect dest = { sx, sy, rw, rh };
	SDL_RenderCopy(sdlRenderer, sdlTexture, &source, &dest);

	SDL_RenderPresent(sdlRenderer);

}
