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
#include "Qt/AviRecord.h"
#include "Qt/fceuWrapper.h"
#include "Qt/MovieOptions.h"

//----------------------------------------------------------------------------
MovieOptionsDialog_t::MovieOptionsDialog_t(QWidget *parent)
	: QDialog(parent, Qt::Window)
{
	QLabel *lbl;
	QGroupBox   *gbox;
	QHBoxLayout *mainLayout;
	QHBoxLayout *hbox;
	QVBoxLayout *vbox1, *vbox2;
	QPushButton *closeButton;
	std::vector <std::string> aviDriverList;

	setWindowTitle("Movie Options");

	mainLayout = new QHBoxLayout();
	vbox1      = new QVBoxLayout();
	vbox2      = new QVBoxLayout();

	mainLayout->addLayout(vbox1);

	readOnlyReplay = new QCheckBox(tr("Always Suggest Read-Only Replay"));
	pauseAfterPlay = new QCheckBox(tr("Pause After Playback"));
	closeAfterPlay = new QCheckBox(tr("Close After Playback"));
	bindSaveStates = new QCheckBox(tr("Bind Save-States to Movies"));
	dpySubTitles = new QCheckBox(tr("Display Movie Subtitles"));
	putSubTitlesAvi = new QCheckBox(tr("Put Movie Subtitles in AVI"));
	autoBackUp = new QCheckBox(tr("Automatically Backup Movies"));
	loadFullStates = new QCheckBox(tr("Load Full Save-State Movies:"));

	lbl = new QLabel(tr("Loading states in record mode will not immediately truncate movie, next frame input will. (VBA-rr and SNES9x style)"));
	lbl->setWordWrap(true);

	vbox1->addWidget(readOnlyReplay);
	vbox1->addWidget(pauseAfterPlay);
	vbox1->addWidget(closeAfterPlay);
	vbox1->addWidget(bindSaveStates);
	vbox1->addWidget(dpySubTitles);
	vbox1->addWidget(putSubTitlesAvi);
	vbox1->addWidget(autoBackUp);
	vbox1->addWidget(loadFullStates);
	vbox1->addWidget(lbl);

	readOnlyReplay->setChecked(suggestReadOnlyReplay);
	pauseAfterPlay->setChecked(pauseAfterPlayback);
	closeAfterPlay->setChecked(closeFinishedMovie);
	bindSaveStates->setChecked(bindSavestate);
	dpySubTitles->setChecked(movieSubtitles);
	putSubTitlesAvi->setChecked(subtitlesOnAVI);
	autoBackUp->setChecked(autoMovieBackup);
	loadFullStates->setChecked(fullSaveStateLoads);

	closeButton = new QPushButton( tr("Close") );
	closeButton->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
	connect(closeButton, SIGNAL(clicked(void)), this, SLOT(closeWindow(void)));

	hbox = new QHBoxLayout();
	hbox->addStretch(5);
	hbox->addWidget( closeButton, 1 );
	vbox1->addLayout( hbox );

	FCEUD_AviGetFormatOpts( aviDriverList );

	gbox = new QGroupBox( tr("AVI Recording Options") );
	gbox->setLayout(vbox2);
	mainLayout->addWidget(gbox);
	hbox = new QHBoxLayout();
	aviBackend = new QComboBox();
	aviPageStack = new QStackedWidget();
	lbl = new QLabel(tr("AVI Backend Driver:"));
	hbox->addWidget( lbl );
	hbox->addWidget( aviBackend );
	vbox2->addLayout( hbox );
	vbox2->addWidget( aviPageStack );

	for (size_t i=0; i<aviDriverList.size(); i++)
	{
		aviBackend->addItem(tr(aviDriverList[i].c_str()), (unsigned int)i);

		switch (i)
		{
			#ifdef _USE_LIBAV
			case AVI_LIBAV:
			{
				aviPageStack->addWidget( new LibavOptionsPage() );
			}
			break;
			#endif
			default:
				aviPageStack->addWidget( new QWidget() );
			break;
		}

		if ( i == aviGetSelVideoFormat() )
		{
			aviBackend->setCurrentIndex(i);
			aviPageStack->setCurrentIndex(i);
		}
	}
	setLayout(mainLayout);

	connect(readOnlyReplay, SIGNAL(stateChanged(int)), this, SLOT(readOnlyReplayChanged(int)));
	connect(pauseAfterPlay, SIGNAL(stateChanged(int)), this, SLOT(pauseAfterPlayChanged(int)));
	connect(closeAfterPlay, SIGNAL(stateChanged(int)), this, SLOT(closeAfterPlayChanged(int)));
	connect(bindSaveStates, SIGNAL(stateChanged(int)), this, SLOT(bindSaveStatesChanged(int)));
	connect(dpySubTitles, SIGNAL(stateChanged(int)), this, SLOT(dpySubTitlesChanged(int)));
	connect(putSubTitlesAvi, SIGNAL(stateChanged(int)), this, SLOT(putSubTitlesAviChanged(int)));
	connect(autoBackUp, SIGNAL(stateChanged(int)), this, SLOT(autoBackUpChanged(int)));
	connect(loadFullStates, SIGNAL(stateChanged(int)), this, SLOT(loadFullStatesChanged(int)));

	connect(aviBackend, SIGNAL(currentIndexChanged(int)), this, SLOT(aviBackendChanged(int)));

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
void MovieOptionsDialog_t::readOnlyReplayChanged(int state)
{
	suggestReadOnlyReplay = (state != Qt::Unchecked);
}
//----------------------------------------------------------------------------
void MovieOptionsDialog_t::pauseAfterPlayChanged(int state)
{
	pauseAfterPlayback = (state != Qt::Unchecked);
}
//----------------------------------------------------------------------------
void MovieOptionsDialog_t::closeAfterPlayChanged(int state)
{
	closeFinishedMovie = (state != Qt::Unchecked);
}
//----------------------------------------------------------------------------
void MovieOptionsDialog_t::bindSaveStatesChanged(int state)
{
	bindSavestate = (state != Qt::Unchecked);
}
//----------------------------------------------------------------------------
void MovieOptionsDialog_t::dpySubTitlesChanged(int state)
{
	movieSubtitles = (state != Qt::Unchecked);

	g_config->setOption("SDL.SubtitleDisplay", movieSubtitles);
}
//----------------------------------------------------------------------------
void MovieOptionsDialog_t::putSubTitlesAviChanged(int state)
{
	subtitlesOnAVI = (state != Qt::Unchecked);
}
//----------------------------------------------------------------------------
void MovieOptionsDialog_t::autoBackUpChanged(int state)
{
	autoMovieBackup = (state != Qt::Unchecked);
}
//----------------------------------------------------------------------------
void MovieOptionsDialog_t::loadFullStatesChanged(int state)
{
	fullSaveStateLoads = (state != Qt::Unchecked);
}
//----------------------------------------------------------------------------
void MovieOptionsDialog_t::aviBackendChanged(int idx)
{
	aviPageStack->setCurrentIndex(idx);

	aviSetSelVideoFormat(idx);
}
//----------------------------------------------------------------------------
