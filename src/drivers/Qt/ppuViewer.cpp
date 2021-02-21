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
// ppuViewer.cpp
//
#include <stdio.h>
#include <stdint.h>

#include <QDir>
#include <QMenu>
#include <QAction>
#include <QMenuBar>
#include <QPainter>
#include <QInputDialog>
#include <QColorDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QTreeWidget>

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
#include "Qt/HexEditor.h"
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
void setPPUSelPatternTile( int table, int x, int y )
{
	if ( ppuViewWindow == NULL )
	{
		return;
	}
	if ( table )
	{
		table = 1;
	}
	else
	{
		table = 0;
	}
	ppuViewWindow->patternView[ table ]->setTileCoord( x, y );
	ppuViewWindow->patternView[ table ]->updateSelTileLabel();
}
//----------------------------------------------------
ppuViewerDialog_t::ppuViewerDialog_t(QWidget *parent)
	: QDialog( parent, Qt::Window )
{
	QMenuBar    *menuBar;
	QVBoxLayout *mainLayout, *vbox;
	QVBoxLayout *patternVbox[2];
	QHBoxLayout *hbox;
	QGridLayout *grid;
	QActionGroup *group;
	QMenu *fileMenu, *viewMenu, *colorMenu, *optMenu, *subMenu;
	QAction *act;
	char stmp[64];
	int useNativeMenuBar;

	ppuViewWindow = this;

	menuBar = new QMenuBar(this);

	// This is needed for menu bar to show up on MacOS
	g_config->getOption( "SDL.UseNativeMenuBar", &useNativeMenuBar );

	menuBar->setNativeMenuBar( useNativeMenuBar ? true : false );

	setWindowTitle( tr("PPU Viewer") );

	mainLayout = new QVBoxLayout();

	mainLayout->setMenuBar( menuBar );

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

	cycleCount  = 0;
	PPUViewSkip = 100;
	
	FCEUD_UpdatePPUView( -1, 1 );

	//-----------------------------------------------------------------------
	// Menu 
	//-----------------------------------------------------------------------
	// File
	fileMenu = menuBar->addMenu(tr("&File"));

	// File -> Close
	act = new QAction(tr("&Close"), this);
	act->setShortcut(QKeySequence::Close);
	act->setStatusTip(tr("Close Window"));
	connect(act, SIGNAL(triggered()), this, SLOT(closeWindow(void)) );
	
	fileMenu->addAction(act);

	// View1
	viewMenu = menuBar->addMenu(tr("View&1"));

	// View1 -> Toggle Grid
	act = new QAction(tr("Toggle &Grid"), this);
	//act->setShortcut(QKeySequence::Open);
	//act->setCheckable(true);
	//act->setChecked( patternView[0]->getDrawTileGrid() );
	act->setStatusTip(tr("Toggle Grid"));
	connect( act, SIGNAL(triggered()), patternView[0], SLOT(toggleTileGridLines()) );
	
	viewMenu->addAction(act);

	// View1 -> Colors
	colorMenu = viewMenu->addMenu(tr("&Colors"));

	// View1 -> Colors -> Tile Selector
	act = new QAction(tr("Tile &Selector"), this);
	//act->setShortcut(QKeySequence::Open);
	act->setStatusTip(tr("Tile Selector"));
	connect(act, SIGNAL(triggered()), patternView[0], SLOT(setTileSelectorColor()) );
	
	colorMenu->addAction(act);

	// View1 -> Colors -> Tile Grid
	act = new QAction(tr("Tile &Grid"), this);
	//act->setShortcut(QKeySequence::Open);
	act->setStatusTip(tr("Tile Grid"));
	connect(act, SIGNAL(triggered()), patternView[0], SLOT(setTileGridColor()) );
	
	colorMenu->addAction(act);

	// View2
	viewMenu = menuBar->addMenu(tr("View&2"));

	// View2 -> Toggle Grid
	act = new QAction(tr("Toggle &Grid"), this);
	//act->setShortcut(QKeySequence::Open);
	//act->setCheckable(true);
	//act->setChecked( patternView[1]->getDrawTileGrid() );
	act->setStatusTip(tr("Toggle Grid"));
	connect( act, SIGNAL(triggered()), patternView[1], SLOT(toggleTileGridLines()) );
	
	viewMenu->addAction(act);

	// View2 -> Colors
	colorMenu = viewMenu->addMenu(tr("&Colors"));

	// View2 -> Colors -> Tile Selector
	act = new QAction(tr("Tile &Selector"), this);
	//act->setShortcut(QKeySequence::Open);
	act->setStatusTip(tr("Tile Selector"));
	connect(act, SIGNAL(triggered()), patternView[1], SLOT(setTileSelectorColor()) );
	
	colorMenu->addAction(act);

	// View2 -> Colors -> Tile Grid
	act = new QAction(tr("Tile &Grid"), this);
	//act->setShortcut(QKeySequence::Open);
	act->setStatusTip(tr("Tile Grid"));
	connect(act, SIGNAL(triggered()), patternView[1], SLOT(setTileGridColor()) );
	
	colorMenu->addAction(act);

	// Options
	optMenu = menuBar->addMenu(tr("&Options"));

	// Options -> Focus
	subMenu = optMenu->addMenu(tr("&Focus Policy"));
	group   = new QActionGroup(this);
	group->setExclusive(true);

	act = new QAction(tr("&Click"), this);
	act->setCheckable(true);
	act->setChecked(true);
	group->addAction(act);
	subMenu->addAction(act);
	connect(act, SIGNAL(triggered()), this, SLOT(setClickFocus(void)) );

	act = new QAction(tr("&Hover"), this);
	act->setCheckable(true);
	act->setChecked(false);
	group->addAction(act);
	subMenu->addAction(act);
	connect(act, SIGNAL(triggered()), this, SLOT(setHoverFocus(void)) );

	//-----------------------------------------------------------------------
	// End Menu 
	//-----------------------------------------------------------------------

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
	cycleCount = (cycleCount + 1) % 4;

	if ( redrawWindow || (cycleCount == 0) )
	{
		this->update();
		redrawWindow = false;
	}
	patternView[0]->updateCycleCounter();
	patternView[1]->updateCycleCounter();
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
void ppuViewerDialog_t::setClickFocus(void)
{
	patternView[0]->setHoverFocus(false);
	patternView[1]->setHoverFocus(false);
}
//----------------------------------------------------
void ppuViewerDialog_t::setHoverFocus(void)
{
	patternView[0]->setHoverFocus(true);
	patternView[1]->setHoverFocus(true);
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
	cycleCount   = 0;
	drawTileGrid = true;
	hover2Focus  = false;

	selTileColor.setRgb(255,255,255);
	gridColor.setRgb(128,128,128);
	selTile.setX(-1);
	selTile.setY(-1);
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
void ppuPatternView_t::setHoverFocus( bool h )
{
	hover2Focus = h;
}
//----------------------------------------------------
void ppuPatternView_t::setTileCoord( int x, int y )
{
	selTile.setX(x);
	selTile.setY(y);
	cycleCount = 0;
}
//----------------------------------------------------
ppuPatternView_t::~ppuPatternView_t(void)
{

}
//----------------------------------------------------
void ppuPatternView_t::openColorPicker( QColor *c )
{
	int ret;
	QColorDialog dialog( this );

	dialog.setCurrentColor( *c );
	dialog.setOption( QColorDialog::DontUseNativeDialog, true );
	ret = dialog.exec();

	if ( ret == QDialog::Accepted )
	{
		//QString colorText;
		//colorText = dialog.selectedColor().name();
		//printf("FG Color string '%s'\n", colorText.toStdString().c_str() );
		//g_config->setOption("SDL.HexEditFgColor", colorText.toStdString().c_str() );
		*c = dialog.selectedColor();
	}
}
//----------------------------------------------------
void ppuPatternView_t::setTileSelectorColor(void)
{
	openColorPicker( &selTileColor );
}
//----------------------------------------------------
void ppuPatternView_t::setTileGridColor(void)
{
	openColorPicker( &gridColor );
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

		event->accept();
	}
	else if ( event->key() == Qt::Key_G )
	{
		drawTileGrid = !drawTileGrid;

		event->accept();
	}
	else if ( event->key() == Qt::Key_E )
	{
		openTileEditor();

		event->accept();
	}
	else if ( event->key() == Qt::Key_P )
	{
		pindex[ patternIndex ] = (pindex[ patternIndex ] + 1) % 9;
	
	     	PPUViewSkip = 100;
	
	     	FCEUD_UpdatePPUView( -1, 0 );

		event->accept();
	}
	else if ( event->key() == Qt::Key_F5 )
	{
		// Load Tile Viewport
		PPUViewSkip = 100;

		FCEUD_UpdatePPUView( -1, 1 );

		event->accept();
	}
	else if ( event->key() == Qt::Key_Up )
	{
		int x, y;

		y = selTile.y();
		x = selTile.x();

		if ( PPUView_sprite16Mode[ patternIndex ] )
		{
			if ( (x % 2) == 0 )
			{
				y -= 2;
				x++;
			}
			else
			{
				x--;
			}
		}
		else
		{
			y--;
		}
		if ( y < 0 )
		{
			y += 16;
		}
		selTile.setX(x);
		selTile.setY(y);

		cycleCount = 0;

		updateSelTileLabel();

		event->accept();
	}
	else if ( event->key() == Qt::Key_Down )
	{
		int x,y;

		y = selTile.y();
		x = selTile.x();

		if ( PPUView_sprite16Mode[ patternIndex ] )
		{
			if ( (x % 2) )
			{
				y += 2;
				x--;
			}
			else
			{
				x++;
			}
		}
		else
		{
			y++;
		}
		if ( y >= 16 )
		{
			y = 0;
		}
		selTile.setX(x);
		selTile.setY(y);

		cycleCount = 0;

		updateSelTileLabel();

		event->accept();
	}
	else if ( event->key() == Qt::Key_Left )
	{
		int x,y;

		x = selTile.x();
		y = selTile.y();

		if ( PPUView_sprite16Mode[ patternIndex ] )
		{
			x -= 2;

			if ( x < 0 )
			{
				if ( y % 2 )
				{
					y--;
				}
				else
				{
					y++;
				}
				x += 16;
			}
		}
		else
		{
			x--;
		}
		if ( x < 0 )
		{
			x = 15;
		}
		selTile.setX(x);
		selTile.setY(y);

		cycleCount = 0;

		updateSelTileLabel();

		event->accept();
	}
	else if ( event->key() == Qt::Key_Right )
	{
		int x,y;

		x = selTile.x();
		y = selTile.y();

		if ( PPUView_sprite16Mode[ patternIndex ] )
		{
			x += 2;

			if ( x >= 16 )
			{
				if ( y % 2 )
				{
					y--;
				}
				else
				{
					y++;
				}
				if ( x % 2 )
				{
					x = 1;
				}
				else
				{
					x = 0;
				}
			}
		}
		else
		{
			x++;
		}
		if ( x >= 16 )
		{
			x = 0;
		}
		selTile.setX(x);
		selTile.setY(y);

		cycleCount = 0;

		updateSelTileLabel();

		event->accept();
	}

}
//----------------------------------------------------
void ppuPatternView_t::updateSelTileLabel(void)
{
	char stmp[32];
	if ( (selTile.y() >= 0) && (selTile.x() >= 0) )
	{
		sprintf( stmp, "Tile: $%X%X", selTile.y(), selTile.x() );
	}
	else
	{
		strcpy( stmp, "Tile:");
	}
	tileLabel->setText( tr(stmp) );
}
//----------------------------------------------------
void ppuPatternView_t::mouseMoveEvent(QMouseEvent *event)
{
	if ( mode == 0 )
	{
		QPoint tile = convPixToTile( event->pos() );

		if ( (tile.x() < 16) && (tile.y() < 16) )
		{
		        if ( hover2Focus )
			{
				selTile = tile;

				cycleCount = 0;

				updateSelTileLabel();
			}
		}
	}
}
//----------------------------------------------------------------------------
void ppuPatternView_t::mousePressEvent(QMouseEvent * event)
{
	QPoint tile = convPixToTile( event->pos() );

	if ( event->button() == Qt::LeftButton )
	{
		if ( (tile.x() < 16) && (tile.y() < 16) )
		{
			selTile = tile;

			cycleCount = 0;

			updateSelTileLabel();
		}
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

	act = new QAction(tr("Open Tile &Editor"), &menu);
	act->setShortcut( QKeySequence(tr("E")));
	connect( act, SIGNAL(triggered(void)), this, SLOT(openTileEditor(void)) );
	menu.addAction( act );

	if ( mode )
	{
		sprintf( stmp, "Exit Tile &View: %X%X", selTile.y(), selTile.x() );
		
		act = new QAction(tr(stmp), &menu);
		act->setShortcut( QKeySequence(tr("Z")));
		connect( act, SIGNAL(triggered(void)), this, SLOT(exitTileMode(void)) );
		menu.addAction( act );
	}
	else
	{
		sprintf( stmp, "&View Tile: %X%X", selTile.y(), selTile.x() );
		
		act = new QAction(tr(stmp), &menu);
		act->setShortcut( QKeySequence(tr("Z")));
		connect( act, SIGNAL(triggered(void)), this, SLOT(showTileMode(void)) );
		menu.addAction( act );
	}

	act = new QAction(tr("Draw Tile &Grid Lines"), &menu);
	act->setCheckable(true);
	act->setChecked(drawTileGrid);
	act->setShortcut( QKeySequence(tr("G")));
	connect( act, SIGNAL(triggered(void)), this, SLOT(toggleTileGridLines(void)) );
	menu.addAction( act );

	act = new QAction(tr("Next &Palette"), &menu);
	act->setShortcut( QKeySequence(tr("P")));
	connect( act, SIGNAL(triggered(void)), this, SLOT(cycleNextPalette(void)) );
	menu.addAction( act );

	subMenu = menu.addMenu(tr("Palette &Select"));
	group   = new QActionGroup(&menu);

	group->setExclusive(true);

	for (int i=0; i<9; i++)
	{
	   char stmp[8];

	   sprintf( stmp, "&%i", i+1 );

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
void ppuPatternView_t::openTileEditor(void)
{
	ppuTileEditor_t *tileEditor;

	tileEditor = new ppuTileEditor_t( patternIndex, this );

	tileEditor->setTile( &selTile );

	tileEditor->show();
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
void ppuPatternView_t::updateCycleCounter(void)
{
	cycleCount = (cycleCount + 1) % 30;
}
//----------------------------------------------------
void ppuPatternView_t::paintEvent(QPaintEvent *event)
{
	int i,j,x,y,w,h,xx,yy,ii,jj,rr;
	QPainter painter(this);
	QPen pen;
	char showSelector;

	viewWidth  = event->rect().width();
	viewHeight = event->rect().height();

	pen = painter.pen();

	//printf("PPU PatternView %ix%i \n", viewWidth, viewHeight );

	pen.setWidth( 1 );
	pen.setColor( gridColor );
	painter.setPen( pen );

	w = viewWidth / 128;
  	h = viewHeight / 128;

	pattern->w = w;
	pattern->h = h;

	xx = 0; yy = 0;

	showSelector = (cycleCount < 20);

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
				yy = (j*8)*h;

				pattern->tile[jj][ii].x = xx;
				pattern->tile[jj][ii].y = yy;

				for (x=0; x < 8; x++)
				{
					yy = (j*8)*h;

					for (y=0; y < 8; y++)
					{
						painter.fillRect( xx, yy, w, h, pattern->tile[jj][ii].pixel[y][x].color );
						yy += h;
					}
					xx += w;
				}
			}
		}

		if ( drawTileGrid )
		{
			xx = 0; y = 128*h;

			for (i=0; i<16; i++) //Columns
			{
				painter.drawLine( xx, 0 , xx, y ); xx += (8*w);
			}

			yy = 0; x = 128*w;

			for (j=0; j<16; j++) //Rows
			{
				painter.drawLine( 0, yy , x, yy ); yy += (8*h);
			}
		}

		if ( showSelector )
		{
			xx = pattern->tile[ selTile.y() ][ selTile.x() ].x;
			yy = pattern->tile[ selTile.y() ][ selTile.x() ].y;

			pen.setWidth( 3 );
			pen.setColor( QColor(  0,  0,  0) );
			painter.setPen( pen );

			painter.drawRect( xx, yy, w*8, h*8 );

			pen.setWidth( 1 );
			pen.setColor( selTileColor );
			painter.setPen( pen );

			painter.drawRect( xx, yy, w*8, h*8 );
		}
	}
	else
	{
		for (i=0; i<16; i++) //Columns
		{
			for (j=0; j<16; j++) //Rows
			{
				xx = (i*8)*w;
				yy = (j*8)*h;

				pattern->tile[j][i].x = xx;
				pattern->tile[j][i].y = yy;

				for (x=0; x < 8; x++)
				{
					yy = (j*8)*h;

					for (y=0; y < 8; y++)
					{
						painter.fillRect( xx, yy, w, h, pattern->tile[j][i].pixel[y][x].color );
						yy += h;
					}
					xx += w;
				}
			}
		}

		if ( drawTileGrid )
		{
			xx = 0; y = 128*h;

			for (i=0; i<16; i++) //Columns
			{
				painter.drawLine( xx, 0 , xx, y ); xx += (8*w);
			}

			yy = 0; x = 128*w;

			for (j=0; j<16; j++) //Rows
			{
				painter.drawLine( 0, yy , x, yy ); yy += (8*h);
			}
		}

		if ( showSelector )
		{
			xx = pattern->tile[ selTile.y() ][ selTile.x() ].x;
			yy = pattern->tile[ selTile.y() ][ selTile.x() ].y;

			pen.setWidth( 3 );
			pen.setColor( QColor(  0,  0,  0) );
			painter.setPen( pen );

			painter.drawRect( xx, yy, w*8, h*8 );

			pen.setWidth( 1 );
			pen.setColor( selTileColor );
			painter.setPen( pen );

			painter.drawRect( xx, yy, w*8, h*8 );
		}
	}
}
//----------------------------------------------------
static int getPPU( unsigned int i )
{
	i &= 0x3FFF;
	if (i < 0x2000)return VPage[(i) >> 10][(i)];
	//NSF PPU Viewer crash here (UGETAB) (Also disabled by 'MaxSize = 0x2000')
	if (GameInfo->type == GIT_NSF)
		return 0;
	else
	{
		if (i < 0x3F00)
			return vnapage[(i >> 10) & 0x3][i & 0x3FF];
		return READPAL_MOTHEROFALL(i & 0x1F);
	}
	return 0;
}
//----------------------------------------------------
static void PalettePoke(uint32 addr, uint8 data)
{
	data = data & 0x3F;
	addr = addr & 0x1F;
	if ((addr & 3) == 0)
	{
		addr = (addr & 0xC) >> 2;
		if (addr == 0)
		{
			PALRAM[0x00] = PALRAM[0x04] = PALRAM[0x08] = PALRAM[0x0C] = data;
		}
		else
		{
			UPALRAM[addr-1] = data;
		}
	}
	else
	{
		PALRAM[addr] = data;
	}
}
//----------------------------------------------------
static int writeMemPPU( unsigned int addr, int value )
{
	addr &= 0x3FFF;
	if (addr < 0x2000)
	{
		VPage[addr >> 10][addr] = value; //todo: detect if this is vrom and turn it red if so
	}
	if ((addr >= 0x2000) && (addr < 0x3F00))
	{
		vnapage[(addr >> 10) & 0x3][addr & 0x3FF] = value; //todo: this causes 0x3000-0x3f00 to mirror 0x2000-0x2f00, is this correct?
	}
	if ((addr >= 0x3F00) && (addr < 0x3FFF))
	{
		PalettePoke(addr, value);
	}
	return 0;
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

	if (palo == NULL)
	{
		return;
	}

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

					pattern->tile[i][j].pixel[y][x].val = p;

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
//----------------------------------------------------
// PPU Tile Editor
//----------------------------------------------------
//----------------------------------------------------
ppuTileEditor_t::ppuTileEditor_t(int patternIndex, QWidget *parent)
	: QDialog( parent, Qt::Window )
{
	QVBoxLayout *mainLayout;
	QHBoxLayout *hbox;
	QMenuBar *menuBar;
	QMenu *helpMenu;
	QAction *act;
	int useNativeMenuBar;

	this->setFocusPolicy(Qt::StrongFocus);

	menuBar = new QMenuBar(this);

	// This is needed for menu bar to show up on MacOS
	g_config->getOption( "SDL.UseNativeMenuBar", &useNativeMenuBar );

	menuBar->setNativeMenuBar( useNativeMenuBar ? true : false );

	//-----------------------------------------------------------------------
	// Menu 
	//-----------------------------------------------------------------------
	// Help
	helpMenu = menuBar->addMenu(tr("Help"));

	// Help -> Key Assignments
	act = new QAction(tr("Keys"), this);
	//act->setShortcut(QKeySequence::Open);
	act->setStatusTip(tr("View Key Descriptions"));
	connect(act, SIGNAL(triggered()), this, SLOT(showKeyAssignments(void)) );
	
	helpMenu->addAction(act);

	//-----------------------------------------------------------------------
	// End Menu 
	//-----------------------------------------------------------------------

	tileAddr = 0;
	palIdx = pindex[ patternIndex ];

	//ppuViewWindow = this;

	setWindowTitle( tr("PPU Tile Editor") );

	mainLayout = new QVBoxLayout();

	mainLayout->setMenuBar( menuBar );

	setLayout( mainLayout );

	//vbox = new QVBoxLayout();
	//hbox = new QHBoxLayout();

	tileView = new ppuTileView_t( patternIndex, this );

	if ( patternIndex )
	{
		tileView->setPattern( &pattern1 );
	}
	else
	{
		tileView->setPattern( &pattern0 );
	}
	tileView->setPaletteNES( palIdx );

	colorPicker = new ppuTileEditColorPicker_t();

	colorPicker->setPaletteNES( palIdx );

	tileIdxLbl = new QLabel();

	palSelBox = new QComboBox();
	palSelBox->addItem( tr("Tile 0"), 0 );
	palSelBox->addItem( tr("Tile 1"), 1 );
	palSelBox->addItem( tr("Tile 2"), 2 );
	palSelBox->addItem( tr("Tile 3"), 3 );
	palSelBox->addItem( tr("Sprite 0"), 4 );
	palSelBox->addItem( tr("Sprite 1"), 5 );
	palSelBox->addItem( tr("Sprite 2"), 6 );
	palSelBox->addItem( tr("Sprite 3"), 7 );
	palSelBox->addItem( tr("GrayScale"), 8 );

	palSelBox->setCurrentIndex( palIdx );

	connect(palSelBox, SIGNAL(currentIndexChanged(int)), this, SLOT(paletteChanged(int)) );

	mainLayout->addWidget( tileIdxLbl, 1 );
	mainLayout->addWidget( tileView, 100  );
	mainLayout->addWidget( colorPicker, 10  );

	hbox = new QHBoxLayout();
	hbox->addWidget( new QLabel( tr("Palette:") ), 1 );
	hbox->addWidget( palSelBox, 10 );

	mainLayout->addLayout( hbox, 1 );

	updateTimer  = new QTimer( this );

	connect( updateTimer, &QTimer::timeout, this, &ppuTileEditor_t::periodicUpdate );

	updateTimer->start( 100 ); // 10hz
}
//----------------------------------------------------
ppuTileEditor_t::~ppuTileEditor_t(void)
{
	updateTimer->stop();

	printf("PPU Tile Editor Window Deleted\n");
}
//----------------------------------------------------
void ppuTileEditor_t::closeEvent(QCloseEvent *event)
{
	printf("PPU Tile Editor Close Window Event\n");
	done(0);
	deleteLater();
	event->accept();
}
//----------------------------------------------------
void ppuTileEditor_t::closeWindow(void)
{
	printf("Close Window\n");
	done(0);
	deleteLater();
}
//----------------------------------------------------
void ppuTileEditor_t::periodicUpdate(void)
{

	tileView->update();
	colorPicker->update();
}
//----------------------------------------------------
void ppuTileEditor_t::paletteChanged(int index)
{
	palIdx = index;

	tileView->setPaletteNES( palIdx );
	colorPicker->setPaletteNES( palIdx );
	//palSelBox->setCurrentIndex( palIdx );
}
//----------------------------------------------------
void ppuTileEditor_t::setTile( QPoint *t )
{
	if ( (t->x() < 16) && (t->y() < 16) )
	{
		int addr;
		char stmp[64];

		addr = tileView->getPatternIndex() ? 0x1000 : 0x0000;
		addr = addr + ( t->y() * 0x0100 );
		addr = addr + ( t->x() * 0x0010 );

		sprintf( stmp, "Tile Index: $%X%X   Address: $%04X", t->y(), t->x(), addr );
		tileIdxLbl->setText( tr(stmp) );

		tileView->setTile( t );
		tileAddr = addr;
	}
}
//----------------------------------------------------
void ppuTileEditor_t::setCellValue( int y, int x, int colorIndex )
{
	int a;
	unsigned char chr0, chr1, mask, val;

	chr0 = (colorIndex & 0x01) ? 1 : 0;
	chr1 = (colorIndex & 0x02) ? 1 : 0;

	a = tileAddr + y;

	mask = (0x01 << (7-x));

	val = getPPU( a );

	if ( chr0 )
	{
		val = val |  mask;
	}
	else
	{
		val = val & ~mask;
	}
	writeMemPPU( a, val );

	val = getPPU( a+8 );

	if ( chr1 )
	{
		val = val |  mask;
	}
	else
	{
		val = val & ~mask;
	}
	writeMemPPU( a+8, val );

	hexEditorRequestUpdateAll();
}
//----------------------------------------------------
void ppuTileEditor_t::showKeyAssignments(void)
{
	int i;
	QDialog *dialog;
	QVBoxLayout *mainLayout;
	QTreeWidget *tree;
	QTreeWidgetItem *item;
	const char *txt[] = 
	{ 
		"Up"   , "Move Selected Cell Up",
		"Down" , "Move Selected Cell Down",
		"Left" , "Move Selected Cell Left",
		"Right", "Move Selected Cell Right",
		"1"    , "Set Selected Cell to Color #1",
		"2"    , "Set Selected Cell to Color #2",
		"3"    , "Set Selected Cell to Color #3",
		"4"    , "Set Selected Cell to Color #4",
		"P"    , "Cycle to Next Tile Palette",
		"ESC"  , "Close Window",
		NULL
	};

	dialog = new QDialog(this);
	dialog->setWindowTitle("Tile Editor Key Descriptions");
	dialog->resize( 512, 512 );

	tree = new QTreeWidget();

	tree->setColumnCount(2);

	item = new QTreeWidgetItem();
	item->setText( 0, QString::fromStdString( "Key" ) );
	item->setText( 1, QString::fromStdString( "Description" ) );
	item->setTextAlignment( 0, Qt::AlignLeft);
	item->setTextAlignment( 1, Qt::AlignLeft);

	tree->setHeaderItem( item );

	tree->header()->setSectionResizeMode( QHeaderView::ResizeToContents );

	i=0;
	while ( txt[i] != NULL )
	{

		item = new QTreeWidgetItem();

		item->setText( 0, tr(txt[i]) ); i++;
		item->setText( 1, tr(txt[i]) ); i++;

		item->setTextAlignment( 0, Qt::AlignLeft);
		item->setTextAlignment( 1, Qt::AlignLeft);

		tree->addTopLevelItem( item );
	}
	mainLayout = new QVBoxLayout();

	mainLayout->addWidget( tree );

	dialog->setLayout( mainLayout );

	dialog->show();
}
//----------------------------------------------------
void ppuTileEditor_t::keyPressEvent(QKeyEvent *event)
{
	//printf("Tile Editor Key Press: 0x%x \n", event->key() );

	if ( (event->key() >= Qt::Key_1) && (event->key() <= Qt::Key_4) )
	{
		QPoint cell;
		int selColor = event->key() - Qt::Key_1;
		
		colorPicker->setColor( selColor );

		cell = tileView->getSelPix();

		setCellValue( cell.y(), cell.x(), selColor );

		PPUViewSkip = 100;
		FCEUD_UpdatePPUView( -1, 1 );

		event->accept();
	}
	else if ( event->key() == Qt::Key_P )
	{
		palIdx = (palIdx + 1) % 9;
		
		tileView->setPaletteNES( palIdx );
		colorPicker->setPaletteNES( palIdx );
		palSelBox->setCurrentIndex( palIdx );

		event->accept();
	}
	else if ( event->key() == Qt::Key_Up )
	{
		tileView->moveSelBoxUpDown(-1);

		event->accept();
	}
	else if ( event->key() == Qt::Key_Down )
	{
		tileView->moveSelBoxUpDown(1);

		event->accept();
	}
	else if ( event->key() == Qt::Key_Left )
	{
		tileView->moveSelBoxLeftRight(-1);

		event->accept();
	}
	else if ( event->key() == Qt::Key_Right )
	{
		tileView->moveSelBoxLeftRight(1);

		event->accept();
	}
	else if ( event->key() == Qt::Key_Escape )
	{
		closeWindow();

		event->accept();
	}

}
//----------------------------------------------------
//----------------------------------------------------
ppuTileView_t::ppuTileView_t( int patternIndexID, QWidget *parent)
	: QWidget(parent)
{
	this->setFocusPolicy(Qt::StrongFocus);
	this->setMouseTracking(true);
	patternIndex = patternIndexID;
	paletteIndex = 0;
	setMinimumWidth( 256 );
	setMinimumHeight( 256 );
	viewWidth = 256;
	viewHeight = 256;
	tileLabel = NULL;
	drawTileGrid = true;
}
//----------------------------------------------------
void ppuTileView_t::setPattern( ppuPatternTable_t *p )
{
	pattern = p;
}
//----------------------------------------------------
void ppuTileView_t::setPaletteNES( int palIndex )
{
	paletteIndex = palIndex << 2;
}
//----------------------------------------------------
void ppuTileView_t::setTile( QPoint *t )
{
	selTile = *t;
}
//----------------------------------------------------
void ppuTileView_t::setTileLabel( QLabel *l )
{
	tileLabel = l;
}
//----------------------------------------------------
void ppuTileView_t::moveSelBoxUpDown( int i )
{
	int y;

	y = selPix.y();

	y = y + (i%8);

	if ( y < 0 )
	{
		y = y + 8;
	}
	else if ( y >= 8 )
	{
		y = y - 8;
	}

	selPix.setY(y);

}
//----------------------------------------------------
void ppuTileView_t::moveSelBoxLeftRight( int i )
{
	int x;

	x = selPix.x();

	x = x + (i%8);

	if ( x < 0 )
	{
		x = x + 8;
	}
	else if ( x >= 8 )
	{
		x = x - 8;
	}

	selPix.setX(x);

}
//----------------------------------------------------
void ppuTileView_t::setSelCell( QPoint &p )
{
	selPix = p;
}
//----------------------------------------------------
ppuTileView_t::~ppuTileView_t(void)
{

}
//----------------------------------------------------
QPoint ppuTileView_t::convPixToCell( QPoint p )
{
	QPoint t(0,0);
	int x,y,w,h,i,j;

	x = p.x(); y = p.y();

	w = boxWidth;
	h = boxHeight;

	i = x / w;
	j = y / h;

	//printf("(x,y) = (%i,%i) w=%i h=%i  $%X%X \n", x, y, w, h, j, i );

	t.setX(i);
	t.setY(j);

	return t;
}
//----------------------------------------------------
void ppuTileView_t::resizeEvent(QResizeEvent *event)
{
	viewWidth  = event->size().width();
	viewHeight = event->size().height();

	boxWidth  = viewWidth / 8;
  	boxHeight = viewHeight / 8;
}
//----------------------------------------------------
void ppuTileView_t::keyPressEvent(QKeyEvent *event)
{
	//printf("Tile View Key Press: 0x%x \n", event->key() );

	//event->accept();
	event->ignore();
}
//----------------------------------------------------
void ppuTileView_t::mouseMoveEvent(QMouseEvent *event)
{
//	QPoint cell = convPixToCell( event->pos() );
}
//----------------------------------------------------------------------------
void ppuTileView_t::mousePressEvent(QMouseEvent * event)
{
	QPoint cell = convPixToCell( event->pos() );

	if ( event->button() == Qt::LeftButton )
	{
		// Set Cell
		setSelCell( cell );
	}
}
//----------------------------------------------------
void ppuTileView_t::contextMenuEvent(QContextMenuEvent *event)
{
//	QAction *act;
//	QMenu menu(this);
//   QMenu *subMenu;
//	QActionGroup *group;
//	QAction *paletteAct[9];
//   char stmp[64];
//
//   if ( mode )
//   {
//      sprintf( stmp, "Exit Tile View: %X%X", selTile.y(), selTile.x() );
//
//      act = new QAction(tr(stmp), &menu);
//      act->setShortcut( QKeySequence(tr("Z")));
//	   connect( act, SIGNAL(triggered(void)), this, SLOT(exitTileMode(void)) );
//      menu.addAction( act );
//
//      act = new QAction(tr("Draw Tile Grid Lines"), &menu);
//      act->setCheckable(true);
//      act->setChecked(drawTileGrid);
//      act->setShortcut( QKeySequence(tr("G")));
//	   connect( act, SIGNAL(triggered(void)), this, SLOT(toggleTileGridLines(void)) );
//      menu.addAction( act );
//   }
//   else
//   {
//      sprintf( stmp, "View Tile: %X%X", selTile.y(), selTile.x() );
//
//      act = new QAction(tr(stmp), &menu);
//      act->setShortcut( QKeySequence(tr("Z")));
//	   connect( act, SIGNAL(triggered(void)), this, SLOT(showTileMode(void)) );
//      menu.addAction( act );
//   }
//
//	act = new QAction(tr("Open Tile Editor"), &menu);
//   act->setShortcut( QKeySequence(tr("O")));
//	connect( act, SIGNAL(triggered(void)), this, SLOT(openTileEditor(void)) );
//   menu.addAction( act );
//
//   act = new QAction(tr("Next Palette"), &menu);
//   act->setShortcut( QKeySequence(tr("P")));
//	connect( act, SIGNAL(triggered(void)), this, SLOT(cycleNextPalette(void)) );
//   menu.addAction( act );
//
//	subMenu = menu.addMenu(tr("Palette Select"));
//	group   = new QActionGroup(&menu);
//
//	group->setExclusive(true);
//
//	for (int i=0; i<9; i++)
//	{
//	   char stmp[8];
//
//	   sprintf( stmp, "%i", i+1 );
//
//	   paletteAct[i] = new QAction(tr(stmp), &menu);
//	   paletteAct[i]->setCheckable(true);
//
//	   group->addAction(paletteAct[i]);
//		subMenu->addAction(paletteAct[i]);
//      
//	   paletteAct[i]->setChecked( pindex[ patternIndex ] == i );
//	}
//
//   connect( paletteAct[0], SIGNAL(triggered(void)), this, SLOT(selPalette0(void)) );
//   connect( paletteAct[1], SIGNAL(triggered(void)), this, SLOT(selPalette1(void)) );
//   connect( paletteAct[2], SIGNAL(triggered(void)), this, SLOT(selPalette2(void)) );
//   connect( paletteAct[3], SIGNAL(triggered(void)), this, SLOT(selPalette3(void)) );
//   connect( paletteAct[4], SIGNAL(triggered(void)), this, SLOT(selPalette4(void)) );
//   connect( paletteAct[5], SIGNAL(triggered(void)), this, SLOT(selPalette5(void)) );
//   connect( paletteAct[6], SIGNAL(triggered(void)), this, SLOT(selPalette6(void)) );
//   connect( paletteAct[7], SIGNAL(triggered(void)), this, SLOT(selPalette7(void)) );
//   connect( paletteAct[8], SIGNAL(triggered(void)), this, SLOT(selPalette8(void)) );
//
//   menu.exec(event->globalPos());
}
//----------------------------------------------------
void ppuTileView_t::paintEvent(QPaintEvent *event)
{
	int x,y,w,h,xx,yy,ii,jj;
	QPainter painter(this);
	QColor   color[4];
	QPen     pen;

	pen = painter.pen();

	viewWidth  = event->rect().width();
	viewHeight = event->rect().height();

	//printf("PPU TileView %ix%i \n", viewWidth, viewHeight );
	
	for (x=0; x < 4; x++)
	{
		color[x].setBlue( palo[palcache[paletteIndex|x]].b );
		color[x].setGreen( palo[palcache[paletteIndex|x]].g );
		color[x].setRed( palo[palcache[paletteIndex|x]].r );
	}

	w = viewWidth / 8;
  	h = viewHeight / 8;

	boxWidth = w;
	boxHeight = h;

	xx = 0; yy = 0;

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
			painter.fillRect( xx, yy, w, h, color[ pattern->tile[jj][ii].pixel[y][x].val & 0x03 ] );
			yy += h;
		}
		xx += w;
	}
	
	if ( drawTileGrid )
	{
		pen.setWidth( 1 );
		pen.setColor( QColor(128,128,128) );
		painter.setPen( pen );

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

	x = selPix.x() * w;
	y = selPix.y() * h;

	pen.setWidth( 6 );
	pen.setColor( QColor(  0,  0,  0) );
	painter.setPen( pen );

	painter.drawRect( x, y, w, h );

	pen.setWidth( 2 );
	pen.setColor( QColor(255,  0,  0) );
	painter.setPen( pen );

	painter.drawRect( x, y, w, h );
}
//----------------------------------------------------
ppuTileEditColorPicker_t::ppuTileEditColorPicker_t(QWidget *parent)
	: QWidget(parent)
{
	int boxPixSize = 64;
	this->setFocusPolicy(Qt::StrongFocus);
	this->setMouseTracking(true);

	font.setFamily("Courier New");
	font.setStyle( QFont::StyleNormal );
	font.setStyleHint( QFont::Monospace );
	font.setPixelSize( boxPixSize / 3 );
	QFontMetrics fm(font);

	setMinimumWidth( boxPixSize * NUM_COLORS );
	setMinimumHeight( boxPixSize );

	viewWidth  = boxPixSize * NUM_COLORS;
	viewHeight = boxPixSize;

	boxWidth  = viewWidth / NUM_COLORS;
	boxHeight = viewHeight;

	selColor = 0;
	paletteIndex = 0;
	//paletteIY = 0;
	//paletteIX = 0;

	#if QT_VERSION > QT_VERSION_CHECK(5, 11, 0)
	pxCharWidth = fm.horizontalAdvance(QLatin1Char('2'));
	#else
	pxCharWidth = fm.width(QLatin1Char('2'));
	#endif
	pxCharHeight = fm.height();
	//pxCharHeight = fm.lineSpacing();
}
//----------------------------------------------------
ppuTileEditColorPicker_t::~ppuTileEditColorPicker_t(void)
{

}
//----------------------------------------------------
QPoint ppuTileEditColorPicker_t::convPixToTile( QPoint p )
{
	QPoint t(0,0);

	t.setX( p.x() / boxWidth );
	t.setY( p.y() / boxHeight );

	return t;
}
//----------------------------------------------------
void  ppuTileEditColorPicker_t::setColor( int colorIndex )
{
	//printf("Setting color to: %i \n", colorIndex );
	selColor = colorIndex;
}
//----------------------------------------------------
void  ppuTileEditColorPicker_t::setPaletteNES( int palIndex )
{
	paletteIndex = palIndex << 2;
}
//----------------------------------------------------
void ppuTileEditColorPicker_t::resizeEvent(QResizeEvent *event)
{
	viewWidth  = event->size().width();
	viewHeight = event->size().height();

	boxWidth  = viewWidth / NUM_COLORS;
	boxHeight = viewHeight;
}
//----------------------------------------------------
void ppuTileEditColorPicker_t::mouseMoveEvent(QMouseEvent *event)
{
	//QPoint tile = convPixToTile( event->pos() );

	//if ( (tile.x() < NUM_COLORS) && (tile.y() < PALETTEHEIGHT) )
	//{
	//	char stmp[64];
	//	int ix = (tile.y()<<4)|tile.x();

	//	sprintf( stmp, "Palette: $%02X", palcache[ix]);

	//	frame->setTitle( tr(stmp) );
	//}
}
//----------------------------------------------------------------------------
void ppuTileEditColorPicker_t::mousePressEvent(QMouseEvent * event)
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
void ppuTileEditColorPicker_t::paintEvent(QPaintEvent *event)
{
	int x,y,w,h,xx,yy;
	QPainter painter(this);
	viewWidth  = event->rect().width();
	viewHeight = event->rect().height();
	QColor color[NUM_COLORS];
	QPen pen;

	painter.setFont(font);

	pen = painter.pen();

	//printf("PPU PatternView %ix%i \n", viewWidth, viewHeight );

	w = boxWidth;
  	h = boxHeight;

	yy = 0;
	xx = 0;

	y=0;
	for (x=0; x < NUM_COLORS; x++)
	{
		color[x].setBlue( palo[palcache[paletteIndex|x]].b );
		color[x].setGreen( palo[palcache[paletteIndex|x]].g );
		color[x].setRed( palo[palcache[paletteIndex|x]].r );

		painter.fillRect( xx, yy, w, h, color[x] );
		xx += w;
	}

	y = h;
	for (int i=0; i<=NUM_COLORS; i++)
	{
		x = i*w; 
		painter.drawLine( x, 0 , x, y );
	}
	
	x = NUM_COLORS*w; 
	for (int i=0; i<=1; i++)
	{
		y = i*h;
		painter.drawLine( 0, y, x, y );
	}

	pen.setWidth( 6 );
	painter.setPen( pen );

	x = selColor * w;
	painter.drawRect( x+3, 3, w-6, h-6 );

	pen.setWidth( 2 );
	pen.setColor( QColor(255,255,255) );
	painter.setPen( pen );
	painter.drawRect( x+3, 3, w-6, h-6 );

	y = (pxCharHeight) + (h - pxCharHeight) / 2;

	for (int i=0; i<NUM_COLORS; i++)
	{
		char c[2];

		x = (i * w) + (w - pxCharWidth)/2;

		c[0] = '1' + i;
		c[1] =  0;

		if ( qGray( color[i].red(), color[i].green(), color[i].blue() ) > 128 )
	        {
			painter.setPen( QColor(  0,  0,  0) );
		}
		else
		{
			painter.setPen( QColor(255,255,255) );
	        }
		painter.drawText( x, y, tr(c) );
	}
}
//----------------------------------------------------
