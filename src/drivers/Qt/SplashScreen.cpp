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
// SplashScreen.cpp
#include "Qt/SplashScreen.h"

fceuSplashScreen::fceuSplashScreen(void)
	: QSplashScreen( QPixmap(":/fceux1.png") )
{

	alpha = 255;

	showMessage("Initializing GUI...");
	updateTimer = new QTimer(this);

	connect(updateTimer, &QTimer::timeout, this, &fceuSplashScreen::periodicUpdate);
}

fceuSplashScreen::~fceuSplashScreen(void)
{
	//printf("SplashScreen Detroyed\n");
	updateTimer->stop();
}

void fceuSplashScreen::closeEvent(QCloseEvent *event)
{
	//printf("Splash CloseEvent\n");

	if ( alpha > 0 )
	{
		if ( !updateTimer->isActive() )
		{
			updateTimer->start(33);
		}
		event->ignore();
	}
	else
	{
		updateTimer->stop();
		QSplashScreen::closeEvent(event);
	}
}

void fceuSplashScreen::periodicUpdate(void)
{
	if ( alpha > 0 )
	{
		alpha -= 10;

		if ( alpha < 0 )
		{
			alpha = 0;
		}
		setWindowOpacity( (double)alpha / 255.0 );
		update();
	}
	else
	{
		close();
		deleteLater();
		updateTimer->stop();
	}
}
