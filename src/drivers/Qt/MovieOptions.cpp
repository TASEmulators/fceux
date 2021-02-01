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
// MovieOptions.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <QHeaderView>
#include <QCloseEvent>

#include "../../fceu.h"
#include "../../movie.h"

#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/fceuWrapper.h"
#include "Qt/MovieOptions.h"

//----------------------------------------------------------------------------
MovieOptionsDialog_t::MovieOptionsDialog_t(QWidget *parent)
	: QDialog( parent )
{
	QLabel *lbl;
	QVBoxLayout *mainLayout;

	setWindowTitle("Movie Options");

	mainLayout = new QVBoxLayout();

	readOnlyReplay  = new QCheckBox( tr("Always Suggest Read-Only Replay") );
	pauseAfterPlay  = new QCheckBox( tr("Pause After Playback") );
	closeAfterPlay  = new QCheckBox( tr("Close After Playback") );
	bindSaveStates  = new QCheckBox( tr("Bind Save-States to Movies") );
	dpySubTitles    = new QCheckBox( tr("Display Movie Sub Titles") );
	putSubTitlesAvi = new QCheckBox( tr("Put Movie Sub Titles in AVI") );
	autoBackUp      = new QCheckBox( tr("Automatically Backup Movies") );
	loadFullStates  = new QCheckBox( tr("Load Full Save-State Movies:") );

	lbl = new QLabel( tr("Loading states in record mode will not immediately truncate movie, next frame input will. (VBA-rr and SNES9x style)") );
	lbl->setWordWrap(true);

	mainLayout->addWidget( readOnlyReplay  );
	mainLayout->addWidget( pauseAfterPlay  );
	mainLayout->addWidget( closeAfterPlay  );
	mainLayout->addWidget( bindSaveStates  );
	mainLayout->addWidget( dpySubTitles    );
	mainLayout->addWidget( putSubTitlesAvi );
	mainLayout->addWidget( autoBackUp      );
	mainLayout->addWidget( loadFullStates  );
	mainLayout->addWidget( lbl  );

	readOnlyReplay->setChecked( suggestReadOnlyReplay );
	pauseAfterPlay->setChecked( pauseAfterPlayback );
	closeAfterPlay->setChecked( closeFinishedMovie );
	bindSaveStates->setChecked( bindSavestate );
	dpySubTitles->setChecked( movieSubtitles );
	putSubTitlesAvi->setChecked( subtitlesOnAVI );
	autoBackUp->setChecked( autoMovieBackup );
	loadFullStates->setChecked( fullSaveStateLoads );

	setLayout( mainLayout );

	connect( readOnlyReplay , SIGNAL(stateChanged(int)), this, SLOT(readOnlyReplayChanged(int))  );
	connect( pauseAfterPlay , SIGNAL(stateChanged(int)), this, SLOT(pauseAfterPlayChanged(int))  );
	connect( closeAfterPlay , SIGNAL(stateChanged(int)), this, SLOT(closeAfterPlayChanged(int))  );
	connect( bindSaveStates , SIGNAL(stateChanged(int)), this, SLOT(bindSaveStatesChanged(int))  );
	connect( dpySubTitles   , SIGNAL(stateChanged(int)), this, SLOT(dpySubTitlesChanged(int))    );
	connect( putSubTitlesAvi, SIGNAL(stateChanged(int)), this, SLOT(putSubTitlesAviChanged(int)) );
	connect( autoBackUp     , SIGNAL(stateChanged(int)), this, SLOT(autoBackUpChanged(int))      );
	connect( loadFullStates , SIGNAL(stateChanged(int)), this, SLOT(loadFullStatesChanged(int))  );
}
//----------------------------------------------------------------------------
MovieOptionsDialog_t::~MovieOptionsDialog_t(void)
{
	printf("Destroy Movie Options Window\n");
}
//----------------------------------------------------------------------------
void MovieOptionsDialog_t::closeEvent(QCloseEvent *event)
{
   printf("Movie Options Close Window Event\n");
   done(0);
	deleteLater();
   event->accept();
}
//----------------------------------------------------------------------------
void MovieOptionsDialog_t::closeWindow(void)
{
   //printf("Close Window\n");
   done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
void  MovieOptionsDialog_t::readOnlyReplayChanged( int state )
{
	suggestReadOnlyReplay = (state != Qt::Unchecked);
}
//----------------------------------------------------------------------------
void  MovieOptionsDialog_t::pauseAfterPlayChanged( int state )
{
	pauseAfterPlayback = (state != Qt::Unchecked);
}
//----------------------------------------------------------------------------
void  MovieOptionsDialog_t::closeAfterPlayChanged( int state )
{
	closeFinishedMovie = (state != Qt::Unchecked);
}
//----------------------------------------------------------------------------
void  MovieOptionsDialog_t::bindSaveStatesChanged( int state )
{
	bindSavestate = (state != Qt::Unchecked);
}
//----------------------------------------------------------------------------
void  MovieOptionsDialog_t::dpySubTitlesChanged( int state )
{
	movieSubtitles = (state != Qt::Unchecked);

	g_config->setOption("SDL.SubtitleDisplay", movieSubtitles);
}
//----------------------------------------------------------------------------
void  MovieOptionsDialog_t::putSubTitlesAviChanged( int state )
{
	subtitlesOnAVI = (state != Qt::Unchecked);
}
//----------------------------------------------------------------------------
void  MovieOptionsDialog_t::autoBackUpChanged( int state )
{
	autoMovieBackup = (state != Qt::Unchecked);
}
//----------------------------------------------------------------------------
void  MovieOptionsDialog_t::loadFullStatesChanged( int state )
{
	fullSaveStateLoads = (state != Qt::Unchecked);
}
//----------------------------------------------------------------------------
