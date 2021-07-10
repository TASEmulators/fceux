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
// ColorMenu.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <QHBoxLayout>
#include <QPushButton>
#include <QImage>
#include <QPixmap>
#include <QStyle>

#include "Qt/ColorMenu.h"
//----------------------------------------------------------------------------
ColorMenuItem::ColorMenuItem( QString txt, QWidget *parent)
	: QAction( txt, parent)
{
	title = txt;
	picker = NULL;
	colorPtr = &lastColor;

	setImageColor( Qt::red );

	connect( this, SIGNAL(triggered(void)), this, SLOT(openColorPicker(void)) );
}
//----------------------------------------------------------------------------
ColorMenuItem::~ColorMenuItem(void)
{
	printf("Destroy Color Menu Item\n");
}
//----------------------------------------------------------------------------
void ColorMenuItem::connectColor( QColor *c )
{
	if ( c != NULL )
	{
		colorPtr = c;
	}
	else
	{
		colorPtr = &lastColor;
	}
	setImageColor( *colorPtr );
}
//----------------------------------------------------------------------------
void ColorMenuItem::setImageColor( QColor c )
{
	int i, x, y, w = 32, h = 32, bw = 4;
	unsigned char pixelData[ 32 * 32 * 4 ];
	QPixmap  pixmap;
	QColor   b;

	lastColor = c;

	b = parentWidget()->palette().color(QPalette::WindowText);

	i=0;

	for (y=0; y<h; y++)
	{
		for (x=0; x<w; x++)
		{
			if ( (y < bw) || (y > h-bw) || (x < bw) || (x > w-bw) )
			{
				pixelData[i] = b.red(); i++;
				pixelData[i] = b.green(); i++;
				pixelData[i] = b.blue(); i++;
				pixelData[i] = 0xFF; i++;
			}
			else
			{
				pixelData[i] = c.red(); i++;
				pixelData[i] = c.green(); i++;
				pixelData[i] = c.blue(); i++;
				pixelData[i] = 0xFF; i++;
			}
		}
	}

	QImage img( pixelData, 32, 32, 32*4, QImage::Format_RGBA8888 );

	pixmap.convertFromImage( img );

	setIcon( QIcon(pixmap) );
}
//----------------------------------------------------------------------------
void ColorMenuItem::pickerClosed(int ret)
{
	picker = NULL;

	setImageColor( *colorPtr );
	//printf("Picker Closed: %i\n", ret );
}
//----------------------------------------------------------------------------
void ColorMenuItem::openColorPicker(void)
{
	//printf("Open Color Picker\n");
	if ( picker == NULL )
	{
		picker = new ColorMenuPickerDialog_t( colorPtr, parentWidget() );

		picker->show();

		connect( picker, SIGNAL(finished(int)), this, SLOT(pickerClosed(int)) );
	}
	else
	{

	}
}
//----------------------------------------------------------------------------
//------ Color Menu Picker
//----------------------------------------------------------------------------
ColorMenuPickerDialog_t::ColorMenuPickerDialog_t( QColor *c, QWidget *parent )
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

	sprintf( stmp, "Pick Color");

	setWindowTitle( stmp );

	colorPtr = c;
	origColor = *c;

	mainLayout = new QVBoxLayout();

	setLayout( mainLayout );

	colorDialog = new QColorDialog(this);

	mainLayout->addWidget( colorDialog );

	colorDialog->setWindowFlags(Qt::Widget);
	colorDialog->setOption( QColorDialog::DontUseNativeDialog, true );
	colorDialog->setOption( QColorDialog::NoButtons, true );
	colorDialog->setCurrentColor( *c );
	
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
ColorMenuPickerDialog_t::~ColorMenuPickerDialog_t(void)
{
	//printf("nesColorPicker Destroyed\n");
}
//----------------------------------------------------------------------------
void ColorMenuPickerDialog_t::closeEvent(QCloseEvent *event)
{
	//printf("nesColorPicker Close Window Event\n");
	done(0);
	deleteLater();
	event->accept();
}
//----------------------------------------------------------------------------
void ColorMenuPickerDialog_t::closeWindow(void)
{
	//printf("Close Window\n");
	done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
void ColorMenuPickerDialog_t::colorChanged( const QColor &color )
{
	//printf("Color Changed: R:%i  G%i  B%i \n", color.red(), color.green(), color.blue() );

	*colorPtr = color;
}
//----------------------------------------------------------------------------
void ColorMenuPickerDialog_t::colorAccepted(void)
{
	//printf("nesColorPicker Accepted: %zi\n", colorChangeHistory.size() );
	done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
void ColorMenuPickerDialog_t::colorRejected(void)
{
	//printf("nesColorPicker Rejected\n");

	// Reset to original color
	*colorPtr = origColor;

	done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
void ColorMenuPickerDialog_t::resetColor(void)
{
	// Reset to original color
	*colorPtr = origColor;

	colorDialog->setCurrentColor( origColor );
}
//----------------------------------------------------------------------------
