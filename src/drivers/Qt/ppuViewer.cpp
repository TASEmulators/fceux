// ppuViewer.cpp
//
#include <stdio.h>
#include <stdint.h>

#include <QDir>
#include <QPainter>
#include <QInputDialog>
#include <QMessageBox>

#include "../../types.h"
#include "../../fceu.h"
#include "../../cart.h"
#include "../../ppu.h"
#include "../../debug.h"
#include "../../palette.h"

#include "Qt/ppuViewer.h"
#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/fceuWrapper.h"

#define PATTERNWIDTH          128
#define PATTERNHEIGHT         128
#define PATTERNBITWIDTH       PATTERNWIDTH*3

#define PALETTEWIDTH          16
#define PALETTEHEIGHT         2
#define PALETTEBITWIDTH       PALETTEWIDTH*3

static ppuViewerDialog_t *ppuViewWindow = NULL;
static int PPUViewScanline = 0;
//static int PPUViewSkip = 0;
//static int PPUViewRefresh = 0;
static bool PPUView_maskUnusedGraphics = true;
static bool PPUView_invertTheMask = false;
static int PPUView_sprite16Mode = 0;
static int pindex0 = 0, pindex1 = 0;
static QColor ppuv_palette[PALETTEHEIGHT][PALETTEWIDTH];
static uint8_t pallast[32+3] = { 0 }; // palette cache for change comparison
static uint8_t palcache[36] = { 0 }; //palette cache for drawing
static uint8_t chrcache0[0x1000] = {0}, chrcache1[0x1000] = {0}, logcache0[0x1000] = {0}, logcache1[0x1000] = {0}; //cache CHR, fixes a refresh problem when right-clicking
//pattern table bitmap arrays

struct patternTable_t
{
	struct 
	{
		struct 
		{
			QColor color;
		} pixel[16][8];
	} tile[16][16];
};
static patternTable_t pattern0;
static patternTable_t pattern1;
//----------------------------------------------------
int openPPUViewWindow( QWidget *parent )
{
	if ( ppuViewWindow != NULL )
	{
		return -1;
	}
	ppuViewWindow = new ppuViewerDialog_t(parent);

	ppuViewWindow->show();

	return 0;
}
//----------------------------------------------------
ppuViewerDialog_t::ppuViewerDialog_t(QWidget *parent)
	: QDialog( parent )
{
	QVBoxLayout *mainLayout;

	ppuViewWindow = this;

   setWindowTitle( tr("PPU Viewer") );

	mainLayout = new QVBoxLayout();

	setLayout( mainLayout );

	patternView = new ppuPatternView_t(this);
	paletteView = new ppuPalatteView_t(this);

	mainLayout->addWidget( patternView, 10 );
	mainLayout->addWidget( paletteView,  1 );

	FCEUD_UpdatePPUView( -1, 1 );
}

//----------------------------------------------------
ppuViewerDialog_t::~ppuViewerDialog_t(void)
{
	ppuViewWindow = NULL;

	printf("PPU Viewer Window Deleted\n");
}
//----------------------------------------------------
void ppuViewerDialog_t::closeEvent(QCloseEvent *event)
{
   printf("PPU Viewer Close Window Event\n");
   done(0);
	deleteLater();
   event->accept();
}
//----------------------------------------------------
void ppuViewerDialog_t::closeWindow(void)
{
   printf("Close Window\n");
   done(0);
	deleteLater();
}
//----------------------------------------------------
ppuPatternView_t::ppuPatternView_t(QWidget *parent)
	: QWidget(parent)
{
	setMinimumWidth( 512 );
	setMinimumHeight( 256 );

}
//----------------------------------------------------
ppuPatternView_t::~ppuPatternView_t(void)
{

}
//----------------------------------------------------
void ppuPatternView_t::paintEvent(QPaintEvent *event)
{
	int i,j,x,y,w,h,w2,xx,yy;
	QPainter painter(this);
	int viewWidth  = event->rect().width();
	int viewHeight = event->rect().height();

	//printf("PPU PatternView %ix%i \n", viewWidth, viewHeight );

	w = viewWidth / 256;
  	h = viewHeight / 128;
	w2= viewWidth / 2;

	xx = 0; yy = 0;

	for (i=0; i<16; i++) //Columns
	{
		for (j=0; j<16; j++) //Rows
		{
			xx = (i*8)*w;

			for (x=0; x < 8; x++)
			{
				yy = (j*8)*h;

				for (y=0; y < 8; y++)
				{
					painter.fillRect( xx, yy, w, h, pattern0.tile[j][i].pixel[y][x].color );
					yy += h;
				}
				xx += w;
			}
		}
	}

	xx = w2; yy = 0;

	for (i=0; i<16; i++) //Columns
	{
		for (j=0; j<16; j++) //Rows
		{
			xx = w2 + (i*8)*w;

			for (x=0; x < 8; x++)
			{
				yy = (j*8)*h;

				for (y=0; y < 8; y++)
				{
					painter.fillRect( xx, yy, w, h, pattern1.tile[j][i].pixel[y][x].color );
					yy += h;
				}
				xx += w;
			}
		}
	}
}
//----------------------------------------------------
static void DrawPatternTable( patternTable_t *pattern, uint8_t *table, uint8_t *log, uint8_t pal)
{
	int i,j,k,x,y,index=0;
	int p=0,tmp;
	uint8_t chr0,chr1,logs,shift;

	pal <<= 2;
	for (i = 0; i < (16 >> PPUView_sprite16Mode); i++)		//Columns
	{
		for (j = 0; j < 16; j++)	//Rows
		{
			//index =  (i<<4)|(j<<8);
			//printf("Tile: %X%X  index:%04X   %04X\n", j,i,index, (i<<4)|(j<<8));
			//-----------------------------------------------
			for (k = 0; k < (PPUView_sprite16Mode + 1); k++) 
			{
				for (y = 0; y < 8; y++)
				{
					chr0 = table[index];
					chr1 = table[index + 8];
					logs = log[index] & log[index + 8];
					tmp = 7;
					//shift=(PPUView_maskUnusedGraphics && debug_loggingCD && (((logs & 3) != 0) == PPUView_invertTheMask))?3:0;
					shift=0;
					for (x = 0; x < 8; x++)
					{
						p  =  (chr0 >> tmp) & 1;
						p |= ((chr1 >> tmp) & 1) << 1;
						p = palcache[p | pal];
						tmp--;
						pattern->tile[i][j].pixel[y][x].color.setBlue( palo[p].b >> shift );
						pattern->tile[i][j].pixel[y][x].color.setGreen( palo[p].g >> shift );
						pattern->tile[i][j].pixel[y][x].color.setRed( palo[p].r >> shift );

						//printf("Tile: %X%X Pixel: (%i,%i)  P:%i RGB: (%i,%i,%i)\n", j, i, x, y, p,
						//	  pattern->tile[j][i].pixel[y][x].color.red(), 
						//	  pattern->tile[j][i].pixel[y][x].color.green(), 
						//	  pattern->tile[j][i].pixel[y][x].color.blue() );
					}
					index++;
					//pbitmap += (PATTERNBITWIDTH-24);
				}
				index+=8;
			}
			//pbitmap -= ((PATTERNBITWIDTH<<(3+PPUView_sprite16Mode))-24);
			//------------------------------------------------
		}
		//pbitmap += (PATTERNBITWIDTH*((8<<PPUView_sprite16Mode)-1));
	}
}
//----------------------------------------------------
void FCEUD_UpdatePPUView(int scanline, int refreshchr)
{
	if ( ppuViewWindow == NULL )
	{
		return;
	}
	if ( (scanline != -1) && (scanline != PPUViewScanline) )
	{
		return;
	}
	int x,y,i;


	if (refreshchr)
	{
		for (i = 0, x=0x1000; i < 0x1000; i++, x++)
		{
			chrcache0[i] = VPage[i>>10][i];
			chrcache1[i] = VPage[x>>10][x];
			if (debug_loggingCD) 
			{
				if (cdloggerVideoDataSize)
				{
					int addr;
					addr = &VPage[i >> 10][i] - CHRptr[0];
					if ((addr >= 0) && (addr < (int)cdloggerVideoDataSize))
						logcache0[i] = cdloggervdata[addr];
					addr = &VPage[x >> 10][x] - CHRptr[0];
					if ((addr >= 0) && (addr < (int)cdloggerVideoDataSize))
						logcache1[i] = cdloggervdata[addr];
				}
			  	else
			  	{
					logcache0[i] = cdloggervdata[i];
					logcache1[i] = cdloggervdata[x];
				}
			}
		}
	}

	//if (PPUViewSkip < PPUViewRefresh) 
	//{
	//	return;
	//}
	
	// update palette only if required
	if ((memcmp(pallast, PALRAM, 32) != 0) || (memcmp(pallast+32, UPALRAM, 3) != 0))
	{
		//printf("Updated PPU View Palette\n");
		memcpy(pallast, PALRAM, 32);
		memcpy(pallast+32, UPALRAM, 3);

		// cache palette content
		memcpy(palcache,PALRAM,32);
		palcache[0x10] = palcache[0x00];
		palcache[0x04] = palcache[0x14] = UPALRAM[0];
		palcache[0x08] = palcache[0x18] = UPALRAM[1];
		palcache[0x0C] = palcache[0x1C] = UPALRAM[2];

		//draw palettes
		for (y = 0; y < PALETTEHEIGHT; y++)
		{
			for (x = 0; x < PALETTEWIDTH; x++)
			{
				i = (y*PALETTEWIDTH) + x;

				ppuv_palette[y][x].setBlue( palo[palcache[i]].b );
				ppuv_palette[y][x].setGreen( palo[palcache[i]].g );
				ppuv_palette[y][x].setRed( palo[palcache[i]].r );
			}
		}
	}

	DrawPatternTable( &pattern0,chrcache0,logcache0,pindex0);
	DrawPatternTable( &pattern1,chrcache1,logcache1,pindex1);

	if ( ppuViewWindow )
	{
		ppuViewWindow->update();
	}
}
//----------------------------------------------------
ppuPalatteView_t::ppuPalatteView_t(QWidget *parent)
	: QWidget(parent)
{
	setMinimumWidth( 32 * PALETTEHEIGHT );
	setMinimumHeight( 32 * PALETTEHEIGHT );

}
//----------------------------------------------------
ppuPalatteView_t::~ppuPalatteView_t(void)
{

}
//----------------------------------------------------
void ppuPalatteView_t::paintEvent(QPaintEvent *event)
{
	int x,y,w,h,xx,yy;
	QPainter painter(this);
	int viewWidth  = event->rect().width();
	int viewHeight = event->rect().height();

	//printf("PPU PatternView %ix%i \n", viewWidth, viewHeight );

	w = viewWidth / PALETTEWIDTH;
  	h = viewHeight / PALETTEHEIGHT;

	yy = 0;
	for (y=0; y < PALETTEHEIGHT; y++)
	{
		xx = 0;

		for (x=0; x < PALETTEWIDTH; x++)
		{
		
			//painter.setPen( pattern0.tile[j][i].pixel[y][x].color );
			painter.fillRect( xx, yy, w, h, ppuv_palette[y][x] );
			xx += w;
		}
		yy += h;
	}
	painter.drawLine( 0, viewHeight / 2, viewWidth, viewHeight / 2 );
}
//----------------------------------------------------
