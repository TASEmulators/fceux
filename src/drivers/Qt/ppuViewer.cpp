// ppuViewer.cpp
//
#include <stdio.h>
#include <stdint.h>

#include <QDir>
#include <QMenu>
#include <QAction>
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
static int PPUViewSkip = 0;
static int PPUViewRefresh = 1;
static bool PPUView_maskUnusedGraphics = true;
static bool PPUView_invertTheMask = false;
static int PPUView_sprite16Mode[2] = { 0, 0 };
static int pindex[2] = { 0, 0 };
static QColor ppuv_palette[PALETTEHEIGHT][PALETTEWIDTH];
static uint8_t pallast[32+3] = { 0 }; // palette cache for change comparison
static uint8_t palcache[36] = { 0 }; //palette cache for drawing
static uint8_t chrcache0[0x1000] = {0}, chrcache1[0x1000] = {0}, logcache0[0x1000] = {0}, logcache1[0x1000] = {0}; //cache CHR, fixes a refresh problem when right-clicking
static bool	redrawWindow = true;

static void initPPUViewer(void);
static ppuPatternTable_t pattern0;
static ppuPatternTable_t pattern1;
//----------------------------------------------------
int openPPUViewWindow( QWidget *parent )
{
	if ( ppuViewWindow != NULL )
	{
		return -1;
	}
	initPPUViewer();

	ppuViewWindow = new ppuViewerDialog_t(parent);

	ppuViewWindow->show();

	return 0;
}
//----------------------------------------------------
ppuViewerDialog_t::ppuViewerDialog_t(QWidget *parent)
	: QDialog( parent, Qt::Window )
{
	QVBoxLayout *mainLayout, *vbox;
	QVBoxLayout *patternVbox[2];
	QHBoxLayout *hbox;
	QGridLayout *grid;
	char stmp[64];

	ppuViewWindow = this;

   setWindowTitle( tr("PPU Viewer") );

	mainLayout = new QVBoxLayout();

	setLayout( mainLayout );

	vbox              = new QVBoxLayout();
	hbox              = new QHBoxLayout();
	grid              = new QGridLayout;
	patternVbox[0]    = new QVBoxLayout();
	patternVbox[1]    = new QVBoxLayout();
	patternFrame[0]   = new QGroupBox( tr("Pattern Table 0") );
	patternFrame[1]   = new QGroupBox( tr("Pattern Table 1") );
	patternView[0]    = new ppuPatternView_t( 0, this);
	patternView[1]    = new ppuPatternView_t( 1, this);
	sprite8x16Cbox[0] = new QCheckBox( tr("Sprites 8x16 Mode") );
	sprite8x16Cbox[1] = new QCheckBox( tr("Sprites 8x16 Mode") );
	tileLabel[0]      = new QLabel( tr("Tile:") );
	tileLabel[1]      = new QLabel( tr("Tile:") );

	sprite8x16Cbox[0]->setChecked( PPUView_sprite16Mode[0] );
	sprite8x16Cbox[1]->setChecked( PPUView_sprite16Mode[1] );

	patternVbox[0]->addWidget( patternView[0], 100 );
	patternVbox[0]->addWidget( tileLabel[0], 1 );
	patternVbox[0]->addWidget( sprite8x16Cbox[0], 1 );
	patternVbox[1]->addWidget( patternView[1], 100 );
	patternVbox[1]->addWidget( tileLabel[1], 1 );
	patternVbox[1]->addWidget( sprite8x16Cbox[1], 1 );

	patternFrame[0]->setLayout( patternVbox[0] );
	patternFrame[1]->setLayout( patternVbox[1] );

	hbox->addWidget( patternFrame[0] );
	hbox->addWidget( patternFrame[1] );

	mainLayout->addLayout( hbox, 10 );
	mainLayout->addLayout( grid,  1 );

	maskUnusedCbox = new QCheckBox( tr("Mask unused Graphics (Code/Data Logger)") );
	invertMaskCbox = new QCheckBox( tr("Invert the Mask (Code/Data Logger)") );

	maskUnusedCbox->setChecked( PPUView_maskUnusedGraphics );
	invertMaskCbox->setChecked( PPUView_invertTheMask );

	connect( sprite8x16Cbox[0], SIGNAL(stateChanged(int)), this, SLOT(sprite8x16Changed0(int)));
	connect( sprite8x16Cbox[1], SIGNAL(stateChanged(int)), this, SLOT(sprite8x16Changed1(int)));

	hbox           = new QHBoxLayout();
	refreshSlider  = new QSlider( Qt::Horizontal );
	hbox->addWidget( new QLabel( tr("Refresh: More") ) );
	hbox->addWidget( refreshSlider );
	hbox->addWidget( new QLabel( tr("Less") ) );

	grid->addWidget( maskUnusedCbox, 0, 0, Qt::AlignLeft );
	grid->addWidget( invertMaskCbox, 1, 0, Qt::AlignLeft );
	grid->addLayout( hbox, 0, 1, Qt::AlignRight );

	hbox         = new QHBoxLayout();
	scanLineEdit = new QLineEdit();
	hbox->addWidget( new QLabel( tr("Display on Scanline:") ) );
	hbox->addWidget( scanLineEdit );
	grid->addLayout( hbox, 1, 1, Qt::AlignRight );

	vbox         = new QVBoxLayout();
	paletteFrame = new QGroupBox( tr("Palettes:") );
	paletteView  = new ppuPalatteView_t(this);

	vbox->addWidget( paletteView, 1 );
	paletteFrame->setLayout( vbox );

	mainLayout->addWidget( paletteFrame,  1 );

	patternView[0]->setPattern( &pattern0 );
	patternView[1]->setPattern( &pattern1 );
	patternView[0]->setTileLabel( tileLabel[0] );
	patternView[1]->setTileLabel( tileLabel[1] );
	paletteView->setTileLabel( paletteFrame );

	scanLineEdit->setMaxLength( 3 );
	scanLineEdit->setInputMask( ">900;" );
	sprintf( stmp, "%i", PPUViewScanline );
	scanLineEdit->setText( tr(stmp) );

	connect( scanLineEdit, SIGNAL(textEdited(const QString &)), this, SLOT(scanLineChanged(const QString &)));

	refreshSlider->setMinimum( 0);
	refreshSlider->setMaximum(25);
	refreshSlider->setValue(PPUViewRefresh);

	connect( refreshSlider, SIGNAL(valueChanged(int)), this, SLOT(refreshSliderChanged(int)));

	FCEUD_UpdatePPUView( -1, 1 );

	updateTimer  = new QTimer( this );

   connect( updateTimer, &QTimer::timeout, this, &ppuViewerDialog_t::periodicUpdate );

	updateTimer->start( 33 ); // 30hz
}

//----------------------------------------------------
ppuViewerDialog_t::~ppuViewerDialog_t(void)
{
	updateTimer->stop();
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
void ppuViewerDialog_t::periodicUpdate(void)
{
	if ( redrawWindow )
	{
		this->update();
		redrawWindow = false;
	}
}
//----------------------------------------------------
void ppuViewerDialog_t::scanLineChanged( const QString &txt )
{
	std::string s;

	s = txt.toStdString();

	if ( s.size() > 0 )
	{
		PPUViewScanline = strtoul( s.c_str(), NULL, 10 );
	}
	//printf("ScanLine: '%s'  %i\n", s.c_str(), PPUViewScanline );
}
//----------------------------------------------------
void ppuViewerDialog_t::sprite8x16Changed0(int state)
{
	PPUView_sprite16Mode[0] = (state == Qt::Unchecked) ? 0 : 1;
}
//----------------------------------------------------
void ppuViewerDialog_t::sprite8x16Changed1(int state)
{
	PPUView_sprite16Mode[1] = (state == Qt::Unchecked) ? 0 : 1;
}
//----------------------------------------------------
void ppuViewerDialog_t::refreshSliderChanged(int value)
{
	PPUViewRefresh = value;
}
//----------------------------------------------------
ppuPatternView_t::ppuPatternView_t( int patternIndexID, QWidget *parent)
	: QWidget(parent)
{
	this->setFocusPolicy(Qt::StrongFocus);
	this->setMouseTracking(true);
	patternIndex = patternIndexID;
	setMinimumWidth( 256 );
	setMinimumHeight( 256 );
	viewWidth = 256;
	viewHeight = 256;
	tileLabel = NULL;
   mode = 0;
   drawTileGrid = true;
}
//----------------------------------------------------
void ppuPatternView_t::setPattern( ppuPatternTable_t *p )
{
	pattern = p;
}
//----------------------------------------------------
void ppuPatternView_t::setTileLabel( QLabel *l )
{
	tileLabel = l;
}
//----------------------------------------------------
ppuPatternView_t::~ppuPatternView_t(void)
{

}
//----------------------------------------------------
QPoint ppuPatternView_t::convPixToTile( QPoint p )
{
	QPoint t(0,0);
	int x,y,w,h,i,j,ii,jj,rr;

	x = p.x(); y = p.y();

	w = pattern->w;
	h = pattern->h;

	i = x / (w*8);
	j = y / (h*8);

	if ( PPUView_sprite16Mode[ patternIndex ] )
	{
		rr = (j%2);
		jj =  j;

		if ( rr )
		{
			jj--;
		}

		ii = (i*2)+rr;

		if ( ii >= 16 )
		{
			ii = ii % 16;
			jj++;
		}
	}
	else
	{
		ii = i; jj = j;
	}
	//printf("(x,y) = (%i,%i) w=%i h=%i  $%X%X \n", x, y, w, h, jj, ii );

	t.setX(ii);
	t.setY(jj);

	return t;
}
//----------------------------------------------------
void ppuPatternView_t::resizeEvent(QResizeEvent *event)
{
	viewWidth  = event->size().width();
	viewHeight = event->size().height();

	pattern->w = viewWidth / 128;
  	pattern->h = viewHeight / 128;
}
//----------------------------------------------------
void ppuPatternView_t::keyPressEvent(QKeyEvent *event)
{
	//printf("Pattern View Key Press: 0x%x \n", event->key() );

   if ( event->key() == Qt::Key_Z )
   {
      mode = !mode;
   }
   else if ( event->key() == Qt::Key_G )
   {
      if ( mode )
      {
         drawTileGrid = !drawTileGrid;
      }
   }
   else if ( event->key() == Qt::Key_P )
   {
      pindex[ patternIndex ] = (pindex[ patternIndex ] + 1) % 9;

		PPUViewSkip = 100;

		FCEUD_UpdatePPUView( -1, 0 );
   }

}
//----------------------------------------------------
void ppuPatternView_t::mouseMoveEvent(QMouseEvent *event)
{
   if ( mode == 0 )
   {
	   QPoint tile = convPixToTile( event->pos() );

	   if ( (tile.x() < 16) && (tile.y() < 16) )
	   {
	   	char stmp[64];
	   	sprintf( stmp, "Tile: $%X%X", tile.y(), tile.x() );
	   	tileLabel->setText( tr(stmp) );

         selTile = tile;
	   }
	}
}
//----------------------------------------------------------------------------
void ppuPatternView_t::mousePressEvent(QMouseEvent * event)
{
	//QPoint tile = convPixToTile( event->pos() );

	if ( event->button() == Qt::LeftButton )
	{
		// Load Tile Viewport
		PPUViewSkip = 100;

		FCEUD_UpdatePPUView( -1, 0 );
	}
}
//----------------------------------------------------
void ppuPatternView_t::contextMenuEvent(QContextMenuEvent *event)
{
	QAction *act;
	QMenu menu(this);
   QMenu *subMenu;
	QActionGroup *group;
	QAction *paletteAct[9];
   char stmp[64];

   if ( mode )
   {
      sprintf( stmp, "Exit Tile View: %X%X", selTile.y(), selTile.x() );

      act = new QAction(tr(stmp), &menu);
      act->setShortcut( QKeySequence(tr("Z")));
	   connect( act, SIGNAL(triggered(void)), this, SLOT(exitTileMode(void)) );
      menu.addAction( act );

      act = new QAction(tr("Draw Tile Grid Lines"), &menu);
      act->setCheckable(true);
      act->setChecked(drawTileGrid);
      act->setShortcut( QKeySequence(tr("G")));
	   connect( act, SIGNAL(triggered(void)), this, SLOT(toggleTileGridLines(void)) );
      menu.addAction( act );
   }
   else
   {
      sprintf( stmp, "View Tile: %X%X", selTile.y(), selTile.x() );

      act = new QAction(tr(stmp), &menu);
      act->setShortcut( QKeySequence(tr("Z")));
	   connect( act, SIGNAL(triggered(void)), this, SLOT(showTileMode(void)) );
      menu.addAction( act );
   }

   act = new QAction(tr("Next Palette"), &menu);
   act->setShortcut( QKeySequence(tr("P")));
	connect( act, SIGNAL(triggered(void)), this, SLOT(cycleNextPalette(void)) );
   menu.addAction( act );

	subMenu = menu.addMenu(tr("Palette Select"));
	group   = new QActionGroup(&menu);

	group->setExclusive(true);

	for (int i=0; i<9; i++)
	{
	   char stmp[8];

	   sprintf( stmp, "%i", i+1 );

	   paletteAct[i] = new QAction(tr(stmp), &menu);
	   paletteAct[i]->setCheckable(true);

	   group->addAction(paletteAct[i]);
		subMenu->addAction(paletteAct[i]);
      
	   paletteAct[i]->setChecked( pindex[ patternIndex ] == i );
	}

   connect( paletteAct[0], SIGNAL(triggered(void)), this, SLOT(selPalette0(void)) );
   connect( paletteAct[1], SIGNAL(triggered(void)), this, SLOT(selPalette1(void)) );
   connect( paletteAct[2], SIGNAL(triggered(void)), this, SLOT(selPalette2(void)) );
   connect( paletteAct[3], SIGNAL(triggered(void)), this, SLOT(selPalette3(void)) );
   connect( paletteAct[4], SIGNAL(triggered(void)), this, SLOT(selPalette4(void)) );
   connect( paletteAct[5], SIGNAL(triggered(void)), this, SLOT(selPalette5(void)) );
   connect( paletteAct[6], SIGNAL(triggered(void)), this, SLOT(selPalette6(void)) );
   connect( paletteAct[7], SIGNAL(triggered(void)), this, SLOT(selPalette7(void)) );
   connect( paletteAct[8], SIGNAL(triggered(void)), this, SLOT(selPalette8(void)) );

   menu.exec(event->globalPos());
}
//----------------------------------------------------
void ppuPatternView_t::toggleTileGridLines(void)
{
   drawTileGrid = !drawTileGrid;
}
//----------------------------------------------------
void ppuPatternView_t::showTileMode(void)
{
   mode = 1;
}
//----------------------------------------------------
void ppuPatternView_t::exitTileMode(void)
{
   mode = 0;
}
//----------------------------------------------------
void ppuPatternView_t::cycleNextPalette(void)
{
   pindex[ patternIndex ] = (pindex[ patternIndex ] + 1) % 9;

	PPUViewSkip = 100;

	FCEUD_UpdatePPUView( -1, 0 );
}
//----------------------------------------------------
void ppuPatternView_t::selPalette0(void)
{
   pindex[ patternIndex ] = 0;
}
//----------------------------------------------------
void ppuPatternView_t::selPalette1(void)
{
   pindex[ patternIndex ] = 1;
}
//----------------------------------------------------
void ppuPatternView_t::selPalette2(void)
{
   pindex[ patternIndex ] = 2;
}
//----------------------------------------------------
void ppuPatternView_t::selPalette3(void)
{
   pindex[ patternIndex ] = 3;
}
//----------------------------------------------------
void ppuPatternView_t::selPalette4(void)
{
   pindex[ patternIndex ] = 4;
}
//----------------------------------------------------
void ppuPatternView_t::selPalette5(void)
{
   pindex[ patternIndex ] = 5;
}
//----------------------------------------------------
void ppuPatternView_t::selPalette6(void)
{
   pindex[ patternIndex ] = 6;
}
//----------------------------------------------------
void ppuPatternView_t::selPalette7(void)
{
   pindex[ patternIndex ] = 7;
}
//----------------------------------------------------
void ppuPatternView_t::selPalette8(void)
{
   pindex[ patternIndex ] = 8;
}
//----------------------------------------------------
void ppuPatternView_t::paintEvent(QPaintEvent *event)
{
	int i,j,x,y,w,h,xx,yy,ii,jj,rr;
	QPainter painter(this);
	viewWidth  = event->rect().width();
	viewHeight = event->rect().height();

	//printf("PPU PatternView %ix%i \n", viewWidth, viewHeight );

	w = viewWidth / 128;
  	h = viewHeight / 128;

	pattern->w = w;
	pattern->h = h;

	xx = 0; yy = 0;

   if ( mode == 1 )
   {
	   w = viewWidth / 8;
  	   h = viewHeight / 8;

      if ( w < h )
      {
         h = w;
      }
      else
      {
         w = h;
      }

      ii = selTile.x();
      jj = selTile.y();

      // Draw Tile Pixels as rectangles
      for (x=0; x < 8; x++)
		{
			yy = 0;

			for (y=0; y < 8; y++)
			{
				painter.fillRect( xx, yy, w, h, pattern->tile[jj][ii].pixel[y][x].color );
				yy += h;
			}
			xx += w;
		}

      if ( drawTileGrid )
      {
         // Draw Tile Pixel grid lines
         xx = 0; y = 8*h;

         for (x=0; x<9; x++)
         {
		      painter.drawLine( xx, 0 , xx, y ); xx += w;
         }
         yy = 0; x = 8*w;

         for (y=0; y<9; y++)
         {
		      painter.drawLine( 0, yy , x, yy ); yy += h;
         }
      }
   }
   else if ( PPUView_sprite16Mode[ patternIndex ] )
	{
		for (i=0; i<16; i++) //Columns
		{
			for (j=0; j<16; j++) //Rows
			{
				rr = (j%2);
				jj =  j;

				if ( rr )
				{
					jj--;
				}

				ii = (i*2)+rr;

				if ( ii >= 16 )
				{
					ii = ii % 16;
					jj++;
				}

				xx = (i*8)*w;

				for (x=0; x < 8; x++)
				{
					yy = (j*8)*h;

					for (y=0; y < 8; y++)
					{
						pattern->tile[jj][ii].x = xx;
						pattern->tile[jj][ii].y = yy;
						painter.fillRect( xx, yy, w, h, pattern->tile[jj][ii].pixel[y][x].color );
						yy += h;
					}
					xx += w;
				}
			}
		}
	}
	else
	{
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
						pattern->tile[j][i].x = xx;
						pattern->tile[j][i].y = yy;
						painter.fillRect( xx, yy, w, h, pattern->tile[j][i].pixel[y][x].color );
						yy += h;
					}
					xx += w;
				}
			}
		}
	}
}
//----------------------------------------------------
static void initPPUViewer(void)
{
	memset( pallast  , 0, sizeof(pallast)   );
	memset( palcache , 0, sizeof(palcache)  );
	memset( chrcache0, 0, sizeof(chrcache0) );
	memset( chrcache1, 0, sizeof(chrcache1) );
	memset( logcache0, 0, sizeof(logcache0) );
	memset( logcache1, 0, sizeof(logcache1) );

	// forced palette (e.g. for debugging CHR when palettes are all-black)
	palcache[(8*4)+0] = 0x0F;
	palcache[(8*4)+1] = 0x00;
	palcache[(8*4)+2] = 0x10;
	palcache[(8*4)+3] = 0x20;

	pindex[0] = 0;
	pindex[1] = 0;

}
//----------------------------------------------------
static void DrawPatternTable( ppuPatternTable_t *pattern, uint8_t *table, uint8_t *log, uint8_t pal)
{
	int i,j,x,y,index=0;
	int p=0,tmp;
	uint8_t chr0,chr1,logs,shift;

	pal <<= 2;
	for (i = 0; i < 16; i++)		//Columns
	{
		for (j = 0; j < 16; j++)	//Rows
		{
			//printf("Tile: %X%X  index:%04X   %04X\n", j,i,index, (i<<4)|(j<<8));
			//-----------------------------------------------
			for (y = 0; y < 8; y++)
			{
				chr0 = table[index];
				chr1 = table[index + 8];
				logs = log[index] & log[index + 8];
				tmp = 7;
				shift=(PPUView_maskUnusedGraphics && debug_loggingCD && (((logs & 3) != 0) == PPUView_invertTheMask))?3:0;
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
			}
			index+=8;
			//------------------------------------------------
		}
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
		int i10, x10;
		for (i = 0, x=0x1000; i < 0x1000; i++, x++)
		{
			i10 = i>>10;
			x10 = x>>10;

			if ( VPage[i10] == NULL )
			{
				continue;
			}
			chrcache0[i] = VPage[i10][i];
			chrcache1[i] = VPage[x10][x];

			if (debug_loggingCD) 
			{
				if (cdloggerVideoDataSize)
				{
					int addr;
					addr = &VPage[i10][i] - CHRptr[0];
					if ((addr >= 0) && (addr < (int)cdloggerVideoDataSize))
						logcache0[i] = cdloggervdata[addr];
					addr = &VPage[x10][x] - CHRptr[0];
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

	if (PPUViewSkip < PPUViewRefresh) 
	{
		PPUViewSkip++;
		return;
	}
	PPUViewSkip = 0;
	
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

	DrawPatternTable( &pattern0,chrcache0,logcache0,pindex[0]);
	DrawPatternTable( &pattern1,chrcache1,logcache1,pindex[1]);

	redrawWindow = true;
}
//----------------------------------------------------
ppuPalatteView_t::ppuPalatteView_t(QWidget *parent)
	: QWidget(parent)
{
	this->setFocusPolicy(Qt::StrongFocus);
	this->setMouseTracking(true);

	setMinimumWidth( 32 * PALETTEWIDTH );
	setMinimumHeight( 32 * PALETTEHEIGHT );

	viewWidth  = 32 * PALETTEWIDTH;
	viewHeight = 32 * PALETTEHEIGHT;

	boxWidth  = viewWidth / PALETTEWIDTH;
   boxHeight = viewHeight / PALETTEHEIGHT;

	frame = NULL;
}
//----------------------------------------------------
ppuPalatteView_t::~ppuPalatteView_t(void)
{

}
//----------------------------------------------------
void ppuPalatteView_t::setTileLabel( QGroupBox *l )
{
	frame = l;
}
//----------------------------------------------------
QPoint ppuPalatteView_t::convPixToTile( QPoint p )
{
	QPoint t(0,0);

	t.setX( p.x() / boxWidth );
	t.setY( p.y() / boxHeight );

	return t;
}
//----------------------------------------------------
void ppuPalatteView_t::resizeEvent(QResizeEvent *event)
{
	viewWidth  = event->size().width();
	viewHeight = event->size().height();

	boxWidth  = viewWidth / PALETTEWIDTH;
   boxHeight = viewHeight / PALETTEHEIGHT;
}
//----------------------------------------------------
void ppuPalatteView_t::mouseMoveEvent(QMouseEvent *event)
{
	QPoint tile = convPixToTile( event->pos() );

	if ( (tile.x() < PALETTEWIDTH) && (tile.y() < PALETTEHEIGHT) )
	{
		char stmp[64];
		int ix = (tile.y()<<4)|tile.x();

		sprintf( stmp, "Palette: $%02X", palcache[ix]);

		frame->setTitle( tr(stmp) );
	}
}
//----------------------------------------------------------------------------
void ppuPalatteView_t::mousePressEvent(QMouseEvent * event)
{
	//QPoint tile = convPixToTile( event->pos() );

	//if ( event->button() == Qt::LeftButton )
	//{
	//}
	//else if ( event->button() == Qt::RightButton )
	//{
	//}
}
//----------------------------------------------------
void ppuPalatteView_t::paintEvent(QPaintEvent *event)
{
	int x,y,w,h,xx,yy;
	QPainter painter(this);
	viewWidth  = event->rect().width();
	viewHeight = event->rect().height();

	//printf("PPU PatternView %ix%i \n", viewWidth, viewHeight );

	w = viewWidth / PALETTEWIDTH;
  	h = viewHeight / PALETTEHEIGHT;

	yy = 0;
	for (y=0; y < PALETTEHEIGHT; y++)
	{
		xx = 0;

		for (x=0; x < PALETTEWIDTH; x++)
		{
			painter.fillRect( xx, yy, w, h, ppuv_palette[y][x] );
			xx += w;
		}
		yy += h;
	}

	y = PALETTEHEIGHT*h;
	for (int i=0; i<=PALETTEWIDTH; i++)
	{
		x = i*w; 
		painter.drawLine( x, 0 , x, y );
	}
	
	x = PALETTEWIDTH*w; 
	for (int i=0; i<=PALETTEHEIGHT; i++)
	{
		y = i*h;
		painter.drawLine( 0, y, x, y );
	}
}
//----------------------------------------------------
