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
// FamilyKeyboard.cpp
#include <QPainter>
#include <QFontMetrics>

#include "Qt/config.h"
#include "Qt/fceuWrapper.h"
#include "Qt/FamilyKeyboard.h"

static const char *keyNames[] =
{
    "F1",
    "F2",
    "F3",
    "F4",
    "F5",
    "F6",
    "F7",
    "F8",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "0",
    "-",
    "^",
    "\\",
    "STP",
    "ESC",
    "Q",
    "W",
    "E",
    "R",
    "T",
    "Y",
    "U",
    "I",
    "O",
    "P",
    "@",
    "[",
    "RETURN",
    "CTR",
    "A",
    "S",
    "D",
    "F",
    "G",
    "H",
    "J",
    "K",
    "L",
    ";",
    ":",
    "]",
    "KANA",
    "SHIFT",
    "Z",
    "X",
    "C",
    "V",
    "B",
    "N",
    "M",
    ",",
    ".",
    "/",
    "_",
    "SHIFT",
    "GRPH",
    "SPACE",
    "CLR",
    "INS",
    "DEL",
    "UP",
    "LEFT",
    "RIGHT",
    "DOWN",
    NULL
};
//*********************************************************************************
FamilyKeyboardWidget::FamilyKeyboardWidget( QWidget *parent )
{
	QFont font;
	std::string fontString;

	g_config->getOption("SDL.FamilyKeyboardFont", &fontString);

	if ( fontString.size() > 0 )
	{
		font.fromString( QString::fromStdString( fontString ) );
	}
	else
	{
		font.setFamily("Courier New");
		font.setStyle( QFont::StyleNormal );
		font.setStyleHint( QFont::Monospace );
	}
	setFont( font );
		
	setMinimumSize( 512, 256 );

	calcFontData();

}
//*********************************************************************************
FamilyKeyboardWidget::~FamilyKeyboardWidget(void)
{
	
}
//*********************************************************************************
void FamilyKeyboardWidget::calcFontData(void)
{
	QWidget::setFont(font());
	QFontMetrics metrics(font());
#if QT_VERSION > QT_VERSION_CHECK(5, 11, 0)
	pxCharWidth = metrics.horizontalAdvance(QLatin1Char('2'));
#else
	pxCharWidth = metrics.width(QLatin1Char('2'));
#endif
	pxCharHeight   = metrics.capHeight();
	//pxLineSpacing  = metrics.lineSpacing() * 1.25;
	//pxCursorHeight = metrics.height();
	
	pxBtnGridX = (3 * pxCharWidth) + 2;
	pxBtnGridY = (2 * pxCharHeight);

	if ( pxBtnGridX % 2 )
	{
		pxBtnGridX++;
	}
	if ( pxBtnGridY % 2 )
	{
		pxBtnGridY++;
	}
	setMinimumWidth( pxBtnGridX * 25 );
	setMinimumHeight( pxBtnGridY * 8 );
}
//*********************************************************************************
void FamilyKeyboardWidget::paintEvent(QPaintEvent *event)
{
	int i, j, x, y, w, h, xs, ys;
	QPainter painter(this);

	// Row 1
	x = pxBtnGridX / 2;
	y = pxBtnGridY / 2;

	w = pxBtnGridX * 2;
	h = pxBtnGridY;

	xs = w / 4;
	ys = h / 4;

	for (i=0; i<8; i++)
	{
		j = i;

		key[j].rect = QRect( x, y, w, h );

		painter.drawText( key[j].rect, Qt::AlignCenter, tr(keyNames[j]) );
		painter.drawRect( key[j].rect );
		
		x += (w + xs);
	}

	// Row 2
	x  = pxBtnGridX;
	y += pxBtnGridY + ys;

	w = pxBtnGridX;

	xs = w / 4;

	for (i=8; i<22; i++)
	{
		j = i;

		key[j].rect = QRect( x, y, w, h );

		painter.drawText( key[j].rect, Qt::AlignCenter, tr(keyNames[j]) );
		painter.drawRect( key[j].rect );
		
		x += (w + xs);
	}
	
	// Row 3
	x  = pxBtnGridX / 2;
	y += pxBtnGridY + ys;

	w = pxBtnGridX;

	xs = w / 4;

	for (i=22; i<35; i++)
	{
		j = i;

		key[j].rect = QRect( x, y, w, h );

		painter.drawText( key[j].rect, Qt::AlignCenter, tr(keyNames[j]) );
		painter.drawRect( key[j].rect );
		
		x += (w + xs);
	}

	x += (xs);

	j = 35;
	key[j].rect = QRect( x, y, w*3, h );
	painter.drawText( key[j].rect, Qt::AlignCenter, tr(keyNames[j]) );
	painter.drawRect( key[j].rect );

	// Row 4
	x  = pxBtnGridX;
	y += pxBtnGridY + ys;

	w = pxBtnGridX;

	xs = w / 4;
	x -= xs;

	for (i=36; i<49; i++)
	{
		j = i;

		key[j].rect = QRect( x, y, w, h );

		painter.drawText( key[j].rect, Qt::AlignCenter, tr(keyNames[j]) );
		painter.drawRect( key[j].rect );
		
		x += (w + xs);
	}

	j = 49;
	key[j].rect = QRect( x, y, w*2, h );
	painter.drawText( key[j].rect, Qt::AlignCenter, tr(keyNames[j]) );
	painter.drawRect( key[j].rect );

	// Row 5
	x  = pxBtnGridX / 2;
	y += pxBtnGridY + ys;

	w = pxBtnGridX;

	xs = w / 4;
	//x -= xs;

	j = 50;
	key[j].rect = QRect( x, y, w*2, h );
	painter.drawText( key[j].rect, Qt::AlignCenter, tr(keyNames[j]) );
	painter.drawRect( key[j].rect );

	x += ((w*2) + xs);

	for (i=51; i<62; i++)
	{
		j = i;

		key[j].rect = QRect( x, y, w, h );

		painter.drawText( key[j].rect, Qt::AlignCenter, tr(keyNames[j]) );
		painter.drawRect( key[j].rect );
		
		x += (w + xs);
	}

	j = 62;
	key[j].rect = QRect( x, y, w*2, h );
	painter.drawText( key[j].rect, Qt::AlignCenter, tr(keyNames[j]) );
	painter.drawRect( key[j].rect );

	// Row 6
	x  = (pxBtnGridX * 5) / 2;
	y += pxBtnGridY + ys;

	w = pxBtnGridX;

	xs = w / 4;
	x += (2*xs);

	j = 63;
	key[j].rect = QRect( x, y, w*2, h );
	painter.drawText( key[j].rect, Qt::AlignCenter, tr(keyNames[j]) );
	painter.drawRect( key[j].rect );

	x += ((w*2) + xs);

	j = 64;
	key[j].rect = QRect( x, y, (w*8) + (xs*7), h );
	painter.drawText( key[j].rect, Qt::AlignCenter, tr(keyNames[j]) );
	painter.drawRect( key[j].rect );

	// Row 7
	xs =  pxBtnGridX / 4;
	x  = (pxBtnGridX+xs) * 17;
	y  = (pxBtnGridY * 3) / 2;

	w = pxBtnGridX;

	xs = w / 4;
	//x += (2*xs);
	y += (2*ys);

	for (i=65; i<68; i++)
	{
		j = i;

		key[j].rect = QRect( x, y, w, h );

		painter.drawText( key[j].rect, Qt::AlignCenter, tr(keyNames[j]) );
		painter.drawRect( key[j].rect );
		
		x += (w + xs);
	}

	// Arrow Keys
	x  = (pxBtnGridX+xs) * 17 + (pxBtnGridX)/2 + xs;
	y += pxBtnGridY + ys;

	j = 68;
	key[j].rect = QRect( x, y, w*2, h );
	painter.drawText( key[j].rect, Qt::AlignCenter, tr(keyNames[j]) );
	painter.drawRect( key[j].rect );

	x  = (pxBtnGridX+xs) * 17 - (pxBtnGridX)/2 + (xs/2);
	y += pxBtnGridY + ys;

	j = 69;
	key[j].rect = QRect( x, y, w*2, h );
	painter.drawText( key[j].rect, Qt::AlignCenter, tr(keyNames[j]) );
	painter.drawRect( key[j].rect );

	x += ((w*2) + xs);

	j = 70;
	key[j].rect = QRect( x, y, w*2, h );
	painter.drawText( key[j].rect, Qt::AlignCenter, tr(keyNames[j]) );
	painter.drawRect( key[j].rect );

	x  = (pxBtnGridX+xs) * 17 + (pxBtnGridX)/2 + xs;
	y += pxBtnGridY + ys;

	j = 71;
	key[j].rect = QRect( x, y, w*2, h );
	painter.drawText( key[j].rect, Qt::AlignCenter, tr(keyNames[j]) );
	painter.drawRect( key[j].rect );
}
//*********************************************************************************
