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
// PaletteEditor.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <SDL.h>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QCloseEvent>
#include <QFileDialog>
#include <QColorDialog>

#include "../../types.h"
#include "../../fceu.h"
#include "../../cart.h"
#include "../../ppu.h"
#include "../../debug.h"
#include "../../palette.h"

#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/fceuWrapper.h"
#include "Qt/PaletteEditor.h"

//----------------------------------------------------------------------------
PaletteEditorDialog_t::PaletteEditorDialog_t(QWidget *parent)
	: QDialog( parent )
{
	QVBoxLayout *mainLayout;
	QMenuBar *menuBar;
	QMenu *fileMenu, *memMenu;
	QAction *act;
	int useNativeMenuBar;

	setWindowTitle("Palette Editor");
	//resize( 512, 512 );

	menuBar = new QMenuBar(this);

	// This is needed for menu bar to show up on MacOS
	g_config->getOption( "SDL.UseNativeMenuBar", &useNativeMenuBar );

	menuBar->setNativeMenuBar( useNativeMenuBar ? true : false );

	//-----------------------------------------------------------------------
	// Menu 
	//-----------------------------------------------------------------------
	// File
	fileMenu = menuBar->addMenu(tr("File"));

	// File -> Open
	act = new QAction(tr("Open"), this);
	act->setShortcut(QKeySequence::Open);
	act->setStatusTip(tr("Open Palette From File"));
	connect(act, SIGNAL(triggered()), this, SLOT(openPaletteFileDialog(void)) );
	
	fileMenu->addAction(act);

	// File -> Save
	act = new QAction(tr("Save"), this);
	act->setShortcut( QKeySequence::Save );
	act->setStatusTip(tr("Save Palette To File"));
	connect(act, SIGNAL(triggered()), this, SLOT(savePaletteFileDialog(void)) );
	
	fileMenu->addAction(act);

	// Memory
	memMenu = menuBar->addMenu(tr("Memory"));

	// Emulator -> Write To
	act = new QAction(tr("Write To"), this);
	act->setShortcut( QKeySequence(tr("F5")));
	act->setStatusTip(tr("Write to Active Color Palette"));
	connect(act, SIGNAL(triggered()), this, SLOT(setActivePalette(void)) );
	
	memMenu->addAction(act);

	//-----------------------------------------------------------------------
	// End Menu 
	//-----------------------------------------------------------------------

	mainLayout = new QVBoxLayout();

	mainLayout->setMenuBar( menuBar );

	setLayout( mainLayout );

	palView = new nesPaletteView(this);

	mainLayout->addWidget( palView );

	palView->loadActivePalette();
}
//----------------------------------------------------------------------------
PaletteEditorDialog_t::~PaletteEditorDialog_t(void)
{
	printf("Destroy Palette Editor Config Window\n");
}
//----------------------------------------------------------------------------
void PaletteEditorDialog_t::closeEvent(QCloseEvent *event)
{
	//printf("Palette Editor Close Window Event\n");
	done(0);
	deleteLater();
	event->accept();
}
//----------------------------------------------------------------------------
void PaletteEditorDialog_t::closeWindow(void)
{
	//printf("Close Window\n");
	done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
void PaletteEditorDialog_t::setActivePalette(void)
{
	palView->setActivePalette();
}
//----------------------------------------------------------------------------
void PaletteEditorDialog_t::openPaletteFileDialog(void)
{
	int ret, useNativeFileDialogVal;
	QString filename;
	QFileDialog  dialog(this, tr("Open Palette From File") );

	const QStringList filters(
	{ 
		"Palette Files (*.pal *.Pal)",
		"Any files (*)"
         });

	dialog.setFileMode(QFileDialog::ExistingFile);

	dialog.setNameFilters( filters );

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::AllDirs | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Open") );

	dialog.setDirectory( tr("/usr/share/fceux/palettes") );

	// Check config option to use native file dialog or not
	g_config->getOption ("SDL.UseNativeFileDialog", &useNativeFileDialogVal);

	dialog.setOption(QFileDialog::DontUseNativeDialog, !useNativeFileDialogVal);

	dialog.show();
	ret = dialog.exec();

	if ( ret )
	{
		QStringList fileList;
		fileList = dialog.selectedFiles();

		if ( fileList.size() > 0 )
		{
			filename = fileList[0];
		}
	}

	if ( filename.isNull() )
	{
	   return;
	}
	qDebug() << "selected file path : " << filename.toUtf8();

	palView->loadFromFile( filename.toStdString().c_str() );

   return;
}
//----------------------------------------------------------------------------
void PaletteEditorDialog_t::savePaletteFileDialog(void)
{
	int ret, useNativeFileDialogVal;
	QString filename;
	QFileDialog  dialog(this, tr("Save Palette To File") );
	const char *home;

	dialog.setFileMode(QFileDialog::AnyFile);

	dialog.setNameFilter(tr("Pal Files (*.pal *.PAL) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::AllDirs | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Save") );
	dialog.setDefaultSuffix( tr(".pal") );

	home = ::getenv("HOME");

	if ( home )
	{
		dialog.setDirectory( tr(home) );
	}

	// Check config option to use native file dialog or not
	g_config->getOption ("SDL.UseNativeFileDialog", &useNativeFileDialogVal);

	dialog.setOption(QFileDialog::DontUseNativeDialog, !useNativeFileDialogVal);

	dialog.show();
	ret = dialog.exec();

	if ( ret )
	{
		QStringList fileList;
		fileList = dialog.selectedFiles();

		if ( fileList.size() > 0 )
		{
			filename = fileList[0];
		}
	}

	if ( filename.isNull() )
	{
	   return;
	}
	qDebug() << "selected file path : " << filename.toUtf8();

	palView->saveToFile( filename.toStdString().c_str() );
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
nesPaletteView::nesPaletteView( QWidget *parent)
	: QWidget(parent)
{
	this->setFocusPolicy(Qt::StrongFocus);
	this->setMouseTracking(true);

	font.setFamily("Courier New");
	font.setStyle( QFont::StyleNormal );
	font.setStyleHint( QFont::Monospace );
	//font.setPixelSize( boxPixSize / 3 );
	QFontMetrics fm(font);

	#if QT_VERSION > QT_VERSION_CHECK(5, 11, 0)
	pxCharWidth = fm.horizontalAdvance(QLatin1Char('2'));
	#else
	pxCharWidth = fm.width(QLatin1Char('2'));
	#endif
	pxCharHeight = fm.height();

	boxWidth  = pxCharWidth  * 4;
  	boxHeight = pxCharHeight * 2;

	viewWidth = boxWidth * 16;
	viewHeight = boxHeight * 4;
	setMinimumWidth( viewWidth );
	setMinimumHeight( viewHeight );
}
//----------------------------------------------------------------------------
nesPaletteView::~nesPaletteView(void)
{

}
//----------------------------------------------------------------------------
void nesPaletteView::loadActivePalette(void)
{
	if ( palo == NULL )
	{
		return;
	}

	for (int p=0; p<NUM_COLORS; p++)
	{
		color[p].setBlue( palo[p].b );
		color[p].setGreen( palo[p].g );
		color[p].setRed( palo[p].r );
	}
}
//----------------------------------------------------------------------------
void nesPaletteView::setActivePalette(void)
{
	int i;
	uint8_t pal[256];

	if ( palo == NULL )
	{
		return;
	}

	i=0;

	for (int p=0; p<NUM_COLORS; p++)
	{
		palo[p].b = color[p].blue();
		palo[p].g = color[p].green();
		palo[p].r = color[p].red();

		pal[i] = palo[p].r; i++;
		pal[i] = palo[p].g; i++;
		pal[i] = palo[p].b; i++;

		//FCEUD_SetPalette( p, palo[p].r, palo[p].g, palo[p].b );

		//printf("%i  %i,%i,%i \n", p, palo[p].r, palo[p].g, palo[p].b );
	}

	fceuWrapperLock();
	FCEUI_SetUserPalette( pal, NUM_COLORS );
	fceuWrapperUnLock();
}
//----------------------------------------------------------------------------
int  nesPaletteView::loadFromFile( const char *filepath )
{
	FILE *fp;
	int   i, j, numBytes;
	unsigned char buf[512];

	fp = ::fopen( filepath, "rb");

	if ( fp == NULL )
	{
		return -1;
	}

	numBytes = ::fread( buf, 1, 3 * 0x40, fp );

	j=0;

	if ( numBytes >= (3*0x40) )
	{

		for (i=0; i<NUM_COLORS; i++)
		{
			color[i].setRed( buf[j] ); j++;
			color[i].setGreen( buf[j] ); j++;
			color[i].setBlue( buf[j] ); j++;
		}
	}

	::fclose(fp);

	return 0;
}

//----------------------------------------------------------------------------
int  nesPaletteView::saveToFile( const char *filepath )
{
	FILE *fp;
	int   i=0, ret = 0, numBytes;
	unsigned char buf[256];

	fp = ::fopen( filepath, "wb");

	if ( fp == NULL )
	{
		return -1;
	}

	i = 0;

	for (int p=0; p<NUM_COLORS; p++)
	{
		buf[i] = color[p].red();  i++;
		buf[i] = color[p].green(); i++;
		buf[i] = color[p].blue(); i++;
	}
	numBytes = ::fwrite( buf, 1, 3 * NUM_COLORS, fp );

	if ( numBytes != (3*NUM_COLORS) )
	{
		printf("Error Failed to Save Palette\n");
		ret = -1;
	}

	::fclose(fp);

	return ret;
}
//----------------------------------------------------------------------------
void nesPaletteView::resizeEvent(QResizeEvent *event)
{
	//viewWidth  = event->size().width();
	//viewHeight = event->size().height();

	//boxWidth  = viewWidth / 16;
  	//boxHeight = viewHeight / 4;
}
//----------------------------------------------------
void nesPaletteView::keyPressEvent(QKeyEvent *event)
{
	//printf("NES Palette View Key Press: 0x%x \n", event->key() );

	if ( event->key() == Qt::Key_E )
	{
		openColorPicker();

		event->accept();
	}

	event->ignore();
}
//----------------------------------------------------
void nesPaletteView::mouseMoveEvent(QMouseEvent *event)
{
	QPoint cell = convPixToCell( event->pos() );

	//printf("Cell %X%X\n", cell.y(), cell.x() ); 

	if ( (cell.x() >= 0) && (cell.x() < 16) &&
	       (cell.y() >= 0) && (cell.y() < 4) )
	{
		selCell = cell;
	}
}
//----------------------------------------------------------------------------
void nesPaletteView::mousePressEvent(QMouseEvent * event)
{
	//QPoint cell = convPixToCell( event->pos() );

	if ( event->button() == Qt::LeftButton )
	{
		// Set Cell
		//setSelCell( cell );
	}
}
//----------------------------------------------------------------------------
void nesPaletteView::contextMenuEvent(QContextMenuEvent *event)
{
	QAction *act;
	QMenu menu(this);
	//QMenu *subMenu;
	//QActionGroup *group;
	char stmp[64];

	sprintf( stmp, "Edit Color %X%X", selCell.y(), selCell.x() );
	act = new QAction(tr(stmp), &menu);
	act->setShortcut( QKeySequence(tr("E")));
	connect( act, SIGNAL(triggered(void)), this, SLOT(editSelColor(void)) );
	menu.addAction( act );

	menu.exec(event->globalPos());
}
//----------------------------------------------------------------------------
void nesPaletteView::editSelColor(void)
{
	openColorPicker();
}
//----------------------------------------------------------------------------
QPoint nesPaletteView::convPixToCell( QPoint p )
{
	int x,y;
	QPoint c;

	x = p.x();
	y = p.y();

	c.setX( x / boxWidth );
	c.setY( y / boxHeight);

	return c;
}
//----------------------------------------------------------------------------
void nesPaletteView::openColorPicker(void)
{
	int idx;
	QColor *c;

	idx = selCell.y()*16 + selCell.x();

	c = &color[idx];

	nesColorPickerDialog_t *dialog = new nesColorPickerDialog_t(idx, c, this);

	dialog->show();
}
//----------------------------------------------------------------------------
static int conv2hex( int i )
{
	int h = 0;
	if ( i >= 10 )
	{
		h = 'A' + i - 10;
	}
	else
	{
		h = '0' + i;
	}
	return h;
}
//----------------------------------------------------------------------------
void nesPaletteView::paintEvent(QPaintEvent *event)
{
	int x,y,w,h,xx,yy,ii,i,j;
	QPainter painter(this);
	QPen     pen;
	char     c[4];
	QColor   white(255,255,255), black(0,0,0);

	pen = painter.pen();

	viewWidth  = event->rect().width();
	viewHeight = event->rect().height();

	//printf("PPU nesPaletteView %ix%i \n", viewWidth, viewHeight );
	
	w = viewWidth / 16;
  	h = viewHeight / 4;

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

	i = w / 4;
	j = h / 4;

	ii=0;
	
	// Draw Tile Pixels as rectangles
	for (y=0; y < 4; y++)
	{
		xx = 0;
	
		for (x=0; x < 16; x++)
		{
			c[0] = conv2hex( (ii & 0xF0) >> 4 );
			c[1] = conv2hex(  ii & 0x0F);
			c[2] =  0;

			painter.fillRect( xx, yy, w, h, color[ ii ] ); 

			if ( qGray( color[ii].red(), color[ii].green(), color[ii].blue() ) > 128 )
	        	{
				painter.setPen( black );
			}
			else
			{
				painter.setPen( white );
	        	}
			painter.drawText( xx+i, yy+h-j, tr(c) );

			xx += w; ii++;
		}
		yy += h;
	}
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
nesColorPickerDialog_t::nesColorPickerDialog_t( int palIndex, QColor *c, QWidget *parent )
	: QDialog( parent )
{
	QVBoxLayout *mainLayout;
	QHBoxLayout *hbox;
	QPushButton *okButton;
	QPushButton *cancelButton;
	QPushButton *resetButton;
	QStyle *style;
	char stmp[128];

	style = this->style();

	sprintf( stmp, "Pick Palette Color $%02X", palIndex );

	setWindowTitle( stmp );

	palIdx = palIndex;
	colorPtr = c;
	origColor = *c;

	mainLayout = new QVBoxLayout();

	setLayout( mainLayout );

	colorDialog = new QColorDialog(this);

	mainLayout->addWidget( colorDialog );

	colorDialog->setCurrentColor( *c );
	colorDialog->setWindowFlags(Qt::Widget);
	colorDialog->setOption( QColorDialog::DontUseNativeDialog, true );
	colorDialog->setOption( QColorDialog::NoButtons, true );
	
	connect( colorDialog, SIGNAL(colorSelected(const QColor &))      , this, SLOT(colorChanged( const QColor &)) );
	connect( colorDialog, SIGNAL(currentColorChanged(const QColor &)), this, SLOT(colorChanged( const QColor &)) );

	connect( colorDialog, SIGNAL(accepted(void)), this, SLOT(colorAccepted(void)) );
	connect( colorDialog, SIGNAL(rejected(void)), this, SLOT(colorRejected(void)) );

	hbox = new QHBoxLayout();
	mainLayout->addLayout( hbox );

	okButton     = new QPushButton( tr("OK") );
	cancelButton = new QPushButton( tr("Cancel") );
	resetButton  = new QPushButton( tr("Reset") );

	okButton->setIcon( style->standardIcon( QStyle::SP_DialogApplyButton ) );
	cancelButton->setIcon( style->standardIcon( QStyle::SP_DialogCancelButton ) );
	resetButton->setIcon( style->standardIcon( QStyle::SP_DialogResetButton ) );

	hbox->addWidget( resetButton, 1  );
	hbox->addStretch( 10 );
	hbox->addWidget( okButton, 1     );
	hbox->addWidget( cancelButton, 1 );

	connect( okButton    , SIGNAL(clicked(void)), this, SLOT(colorAccepted(void)) );
	connect( cancelButton, SIGNAL(clicked(void)), this, SLOT(colorRejected(void)) );
	connect( resetButton , SIGNAL(clicked(void)), this, SLOT(resetColor(void)) );
}
//----------------------------------------------------------------------------
nesColorPickerDialog_t::~nesColorPickerDialog_t(void)
{
	//printf("nesColorPicker Destroyed\n");
}
//----------------------------------------------------------------------------
void nesColorPickerDialog_t::closeEvent(QCloseEvent *event)
{
	//printf("nesColorPicker Close Window Event\n");
	done(0);
	deleteLater();
	event->accept();
}
//----------------------------------------------------------------------------
void nesColorPickerDialog_t::closeWindow(void)
{
	//printf("Close Window\n");
	done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
void nesColorPickerDialog_t::colorChanged( const QColor &color )
{
	//printf("Color Changed: R:%i  G%i  B%i \n", color.red(), color.green(), color.blue() );

	*colorPtr = color;

	( (nesPaletteView*)parent())->setActivePalette();
}
//----------------------------------------------------------------------------
void nesColorPickerDialog_t::colorAccepted(void)
{
	//printf("nesColorPicker Accepted\n");
	deleteLater();
}
//----------------------------------------------------------------------------
void nesColorPickerDialog_t::colorRejected(void)
{
	//printf("nesColorPicker Rejected\n");

	// Reset to original color
	*colorPtr = origColor;

	( (nesPaletteView*)parent())->setActivePalette();

	deleteLater();
}
//----------------------------------------------------------------------------
void nesColorPickerDialog_t::resetColor(void)
{
	// Reset to original color
	*colorPtr = origColor;

	colorDialog->setCurrentColor( origColor );

	( (nesPaletteView*)parent())->setActivePalette();
}
//----------------------------------------------------------------------------
