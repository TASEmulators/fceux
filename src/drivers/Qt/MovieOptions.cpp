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
#include "Qt/ConsoleUtilities.h"
#include "Qt/MovieOptions.h"

//----------------------------------------------------------------------------
MovieOptionsDialog_t::MovieOptionsDialog_t(QWidget *parent)
	: QDialog(fceuToolWindowParent(parent), Qt::Window)
{
	QLabel *lbl;
	QGroupBox   *gbox;
	QHBoxLayout *mainLayout;
	QHBoxLayout *hbox;
	QVBoxLayout *vbox1, *vbox2;
	QPushButton *closeButton;
	std::vector <std::string> aviDriverList;
	int aviDriver=0;

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
	loadFullStates = new QCheckBox(tr("Restore Full Movie from Save State"));
	aviEnableHUD = new QCheckBox(tr("Record Overlay (HUD) in AVI"));
	aviEnableMsg = new QCheckBox(tr("Record On-Screen Messages in AVI"));
	aviEnableAudio = new QCheckBox(tr("AVI Enable Audio Recording"));
	readOnlyReplay->setToolTip(tr("Preselect Read-Only mode in the Play Movie dialog."));
	pauseAfterPlay->setToolTip(tr("Pause emulation when movie playback ends."));
	closeAfterPlay->setToolTip(tr("Stop movie playback when it reaches the end."));
	bindSaveStates->setToolTip(tr("Include the movie name in save-state filenames so states are associated with that movie."));
	dpySubTitles->setToolTip(tr("Display subtitles stored in the movie."));
	putSubTitlesAvi->setToolTip(tr("Include movie subtitles in AVI video recordings."));
	autoBackUp->setToolTip(tr("Create a backup before overwriting a movie file."));
	loadFullStates->setToolTip(tr("In read/write mode, restore all movie input stored in the save state instead of truncating it at the saved frame."));
	aviEnableHUD->setToolTip(tr("Include the on-screen overlay in AVI video recordings."));
	aviEnableMsg->setToolTip(tr("Include emulator messages in AVI video recordings."));
	aviEnableAudio->setToolTip(tr("Include sound output in AVI recordings."));

	lbl = new QLabel(tr("With full movie loading enabled, recording the next frame may truncate future input, depending on the recording mode."));
	lbl->setWordWrap(true);

	vbox1->addWidget(readOnlyReplay);
	vbox1->addWidget(pauseAfterPlay);
	vbox1->addWidget(closeAfterPlay);
	vbox1->addWidget(bindSaveStates);
	vbox1->addWidget(dpySubTitles);
	vbox1->addWidget(putSubTitlesAvi);
	vbox1->addWidget(autoBackUp);
	vbox1->addWidget(loadFullStates);
	vbox1->addWidget(aviEnableHUD);
	vbox1->addWidget(aviEnableMsg);
	vbox1->addWidget(aviEnableAudio);
	vbox1->addWidget(lbl);

	readOnlyReplay->setChecked(suggestReadOnlyReplay);
	pauseAfterPlay->setChecked(pauseAfterPlayback);
	closeAfterPlay->setChecked(closeFinishedMovie);
	bindSaveStates->setChecked(bindSavestate);
	dpySubTitles->setChecked(movieSubtitles);
	putSubTitlesAvi->setChecked(subtitlesOnAVI);
	autoBackUp->setChecked(autoMovieBackup);
	loadFullStates->setChecked(fullSaveStateLoads);
	aviEnableHUD->setChecked(FCEUI_AviEnableHUDrecording());
	aviEnableMsg->setChecked(!FCEUI_AviDisableMovieMessages());
	aviEnableAudio->setChecked(aviGetAudioEnable());

	closeButton = new QPushButton( tr("Close") );
	closeButton->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
	connect(closeButton, SIGNAL(clicked(void)), this, SLOT(closeWindow(void)));

	hbox = new QHBoxLayout();
	hbox->addStretch(5);
	addWhatsThisButton(hbox, this);
	hbox->addWidget( closeButton, 1 );
	vbox1->addLayout( hbox );

	FCEUD_AviGetDriverList( aviDriverList );

	gbox = new QGroupBox( tr("AVI Recording Options") );
	gbox->setLayout(vbox2);
	mainLayout->addWidget(gbox);
	hbox = new QHBoxLayout();
	aviBackend = new QComboBox();
	aviPageStack = new QStackedWidget();
	lbl = new QLabel(tr("AVI Backend Driver:"));
	aviBackend->setToolTip(tr("Select the library used to encode AVI recordings. Codec options for the selected backend appear below."));
	aviBackend->setWhatsThis(tr("The AVI backend supplies the encoders used to write video and audio. Available encoders and their options depend on the selected backend and how FCEUX was built."));
	hbox->addWidget( lbl );
	hbox->addWidget( aviBackend );
	vbox2->addLayout( hbox );
	vbox2->addWidget( aviPageStack );

	g_config->getOption("SDL.AviDriver", &aviDriver );

	for (size_t i=0; i<aviDriverList.size(); i++)
	{
		aviBackend->addItem(tr(aviDriverList[i].c_str()), (unsigned int)i);

		switch (i)
		{
			#ifdef _USE_LIBAV
			case AVI_DRIVER_LIBAV:
			{
				aviPageStack->addWidget( new LibavOptionsPage() );
			}
			break;
			#endif
			case AVI_DRIVER_LIBGWAVI:
				aviPageStack->addWidget( new LibgwaviOptionsPage() );
			break;
			default:
				aviPageStack->addWidget( new QWidget() );
			break;
		}

		if ( i == static_cast<size_t>(aviDriver) )
		{
			aviBackend->setCurrentIndex(i);
			aviPageStack->setCurrentIndex(i);
		}
	}
	setLayout(mainLayout);

	connect(readOnlyReplay, SIGNAL(toggled(bool)), this, SLOT(readOnlyReplayChanged(bool)));
	connect(pauseAfterPlay, SIGNAL(toggled(bool)), this, SLOT(pauseAfterPlayChanged(bool)));
	connect(closeAfterPlay, SIGNAL(toggled(bool)), this, SLOT(closeAfterPlayChanged(bool)));
	connect(bindSaveStates, SIGNAL(toggled(bool)), this, SLOT(bindSaveStatesChanged(bool)));
	connect(dpySubTitles  , SIGNAL(toggled(bool)), this, SLOT(dpySubTitlesChanged(bool)));
	connect(putSubTitlesAvi, SIGNAL(toggled(bool)), this, SLOT(putSubTitlesAviChanged(bool)));
	connect(autoBackUp, SIGNAL(toggled(bool)), this, SLOT(autoBackUpChanged(bool)));
	connect(loadFullStates, SIGNAL(toggled(bool)), this, SLOT(loadFullStatesChanged(bool)));
	connect(aviEnableHUD  , SIGNAL(toggled(bool)), this, SLOT(setAviHudEnable(bool)));
	connect(aviEnableMsg  , SIGNAL(toggled(bool)), this, SLOT(setAviMsgEnable(bool)));
	connect(aviEnableAudio, SIGNAL(toggled(bool)), this, SLOT(setAviAudioEnable(bool)));

	connect(aviBackend, SIGNAL(currentIndexChanged(int)), this, SLOT(aviBackendChanged(int)));

}
//----------------------------------------------------------------------------
MovieOptionsDialog_t::~MovieOptionsDialog_t(void)
{
	//printf("Destroy Movie Options Window\n");
}
//----------------------------------------------------------------------------
void MovieOptionsDialog_t::closeEvent(QCloseEvent *event)
{
	//printf("Movie Options Close Window Event\n");
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
void MovieOptionsDialog_t::readOnlyReplayChanged(bool state)
{
	suggestReadOnlyReplay = (state);

	g_config->setOption("SDL.SuggestReadOnlyReplay"  , suggestReadOnlyReplay);
}
//----------------------------------------------------------------------------
void MovieOptionsDialog_t::pauseAfterPlayChanged(bool state)
{
	pauseAfterPlayback = (state);

	g_config->setOption("SDL.PauseAfterMoviePlayback", pauseAfterPlayback);
}
//----------------------------------------------------------------------------
void MovieOptionsDialog_t::closeAfterPlayChanged(bool state)
{
	closeFinishedMovie = (state);

	g_config->setOption("SDL.CloseFinishedMovie", closeFinishedMovie);
}
//----------------------------------------------------------------------------
void MovieOptionsDialog_t::bindSaveStatesChanged(bool state)
{
	bindSavestate = (state);

	g_config->setOption("SDL.MovieBindSavestate", bindSavestate);
}
//----------------------------------------------------------------------------
void MovieOptionsDialog_t::dpySubTitlesChanged(bool state)
{
	movieSubtitles = (state);

	g_config->setOption("SDL.SubtitleDisplay", movieSubtitles);
}
//----------------------------------------------------------------------------
void MovieOptionsDialog_t::putSubTitlesAviChanged(bool state)
{
	subtitlesOnAVI = (state);

	g_config->setOption("SDL.SubtitlesOnAVI", subtitlesOnAVI);
}
//----------------------------------------------------------------------------
void MovieOptionsDialog_t::setAviHudEnable(bool state)
{
	bool checked = (state);

	FCEUI_SetAviEnableHUDrecording( checked );

	g_config->setOption("SDL.RecordHUD", checked );
}
//----------------------------------------------------------------------------
void MovieOptionsDialog_t::setAviMsgEnable(bool state)
{
	bool checked = (state);

	FCEUI_SetAviDisableMovieMessages( !checked );

	g_config->setOption("SDL.MovieMsg", checked );
}
//----------------------------------------------------------------------------
void MovieOptionsDialog_t::setAviAudioEnable(bool state)
{
	bool checked = (state);

	aviSetAudioEnable( checked );
}
//----------------------------------------------------------------------------
void MovieOptionsDialog_t::autoBackUpChanged(bool state)
{
	autoMovieBackup = (state);

	g_config->setOption("SDL.AutoMovieBackup", autoMovieBackup);
}
//----------------------------------------------------------------------------
void MovieOptionsDialog_t::loadFullStatesChanged(bool state)
{
	fullSaveStateLoads = (state);

	g_config->setOption("SDL.MovieFullSaveStateLoads", fullSaveStateLoads);
}
//----------------------------------------------------------------------------
void MovieOptionsDialog_t::aviBackendChanged(int idx)
{
	aviPageStack->setCurrentIndex(idx);

	aviSetSelDriver(idx);
}
//----------------------------------------------------------------------------
