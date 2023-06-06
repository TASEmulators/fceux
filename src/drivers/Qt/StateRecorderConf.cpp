/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2022 thor2016
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
// StateRecorderConf.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <QCloseEvent>
#include <QGridLayout>
#include <QSettings>
#include <QMessageBox>

#include "Qt/throttle.h"
#include "Qt/fceuWrapper.h"
#include "Qt/StateRecorderConf.h"

#include "../../fceu.h"
#include "../../state.h"
#include "../../driver.h"
#include "../../emufile.h"

//----------------------------------------------------------------------------
StateRecorderDialog_t::StateRecorderDialog_t(QWidget *parent)
	: QDialog(parent)
{
	QVBoxLayout *mainLayout, *vbox1;
	QHBoxLayout *hbox, *hbox1;
	QGroupBox *frame, *frame1;
	QGridLayout *grid, *memStatsGrid, *sysStatusGrid;
	QSettings settings;
	int opt;

	setWindowTitle("State Recorder Config");

	mainLayout = new QVBoxLayout();
	      grid = new QGridLayout();

	recorderEnable  = new QCheckBox(tr("Auto Start Recorder at ROM Load"));
	snapFrames      = new QSpinBox();
	snapMinutes     = new QSpinBox();
	snapSeconds     = new QSpinBox();
	historyDuration = new QSpinBox();
	snapFrameSelBtn = new QRadioButton( tr("By Frames") );
	snapTimeSelBtn  = new QRadioButton( tr("By Time") );

	recorderEnable->setChecked( FCEU_StateRecorderIsEnabled() );

	connect( recorderEnable, SIGNAL(stateChanged(int)), this, SLOT(enableChanged(int)) );

	snapFrames->setMinimum(1);
	snapFrames->setMaximum(10000);
	snapSeconds->setMinimum(0);
	snapSeconds->setMaximum(60);
	snapMinutes->setMinimum(0);
	snapMinutes->setMaximum(60);
	historyDuration->setMinimum(1);
	historyDuration->setMaximum(180);

	opt = 10;
	g_config->getOption("SDL.StateRecorderFramesBetweenSnaps", &opt);
	snapFrames->setValue(opt);

	opt = 15;
	g_config->getOption("SDL.StateRecorderHistoryDurationMin", &opt );
	historyDuration->setValue(opt);

	opt = 0;
	g_config->getOption("SDL.StateRecorderTimeBetweenSnapsMin", &opt);
	snapMinutes->setValue(opt);

	opt = 3;
	g_config->getOption("SDL.StateRecorderTimeBetweenSnapsSec", &opt);
	snapSeconds->setValue(opt);

	opt = 0;
	g_config->getOption("SDL.StateRecorderTimingMode", &opt);

	snapFrameSelBtn->setChecked(opt == 0);
	 snapTimeSelBtn->setChecked(opt == 1);
	 snapUseTime = opt ? true : false;

	connect(     snapFrames , SIGNAL(valueChanged(int)), this, SLOT(spinBoxValueChanged(int)) );
	connect(     snapSeconds, SIGNAL(valueChanged(int)), this, SLOT(spinBoxValueChanged(int)) );
	connect(     snapMinutes, SIGNAL(valueChanged(int)), this, SLOT(spinBoxValueChanged(int)) );
	connect( historyDuration, SIGNAL(valueChanged(int)), this, SLOT(spinBoxValueChanged(int)) );

	frame = new QGroupBox(tr("Retain History For:"));
	hbox  = new QHBoxLayout();

	hbox->addWidget( historyDuration);
	hbox->addWidget( new QLabel( tr("Minutes") ) );

	frame->setLayout(hbox);

	grid->addWidget( recorderEnable, 0, 0 );
	grid->addWidget( frame         , 1, 0 );

	frame = new QGroupBox(tr("Compression Level:"));
	hbox  = new QHBoxLayout();

	cmprLvlCbox = new QComboBox();
	cmprLvlCbox->addItem( tr("0 - None"), 0 );
	cmprLvlCbox->addItem( tr("1"), 1 );
	cmprLvlCbox->addItem( tr("2"), 2 );
	cmprLvlCbox->addItem( tr("3"), 3 );
	cmprLvlCbox->addItem( tr("4"), 4 );
	cmprLvlCbox->addItem( tr("5"), 5 );
	cmprLvlCbox->addItem( tr("6"), 6 );
	cmprLvlCbox->addItem( tr("7"), 7 );
	cmprLvlCbox->addItem( tr("8"), 8 );
	cmprLvlCbox->addItem( tr("9 - Max"), 9 );

	opt = 0;
	g_config->getOption("SDL.StateRecorderCompressionLevel", &opt);
	cmprLvlCbox->setCurrentIndex(opt);

	connect( cmprLvlCbox, SIGNAL(currentIndexChanged(int)), this, SLOT(compressionLevelChanged(int)) );

	hbox->addWidget(cmprLvlCbox);

	frame->setLayout(hbox);
	grid->addWidget( frame, 1, 1 );

	frame1 = new QGroupBox(tr("Snapshot Timing Setting:"));
	hbox1  = new QHBoxLayout();
	frame1->setLayout(hbox1);
	grid->addWidget( frame1, 2, 0, 1, 2 );

	g_config->getOption("SDL.StateRecorderTimingMode", &opt);

	hbox1->addWidget( snapFrameSelBtn );
	hbox1->addWidget( snapTimeSelBtn  );

	snapFramesGroup = new QGroupBox(tr("Frames Between Snapshots:"));
	hbox1  = new QHBoxLayout();
	snapFramesGroup->setLayout(hbox1);
	snapFramesGroup->setEnabled(snapFrameSelBtn->isChecked());
	grid->addWidget( snapFramesGroup, 3, 0, 1, 2 );

	hbox1->addWidget( snapFrames);
	hbox1->addWidget( new QLabel( tr("Range (1 - 10000)") ) );

	snapTimeGroup = new QGroupBox(tr("Time Between Snapshots:"));
	hbox1  = new QHBoxLayout();
	snapTimeGroup->setLayout(hbox1);
	snapTimeGroup->setEnabled(snapTimeSelBtn->isChecked());
	grid->addWidget( snapTimeGroup, 4, 0, 1, 2 );

	frame  = new QGroupBox();
	hbox   = new QHBoxLayout();

	hbox1->addWidget(frame);
	hbox->addWidget( snapMinutes);
	hbox->addWidget( new QLabel( tr("Minutes") ) );

	frame->setLayout(hbox);

	frame = new QGroupBox();
	hbox  = new QHBoxLayout();

	hbox1->addWidget(frame);
	hbox->addWidget( snapSeconds);
	hbox->addWidget( new QLabel( tr("Seconds") ) );

	frame->setLayout(hbox);

	frame1 = new QGroupBox(tr("Pause on State Load:"));
	vbox1  = new QVBoxLayout();
	frame1->setLayout(vbox1);

	g_config->getOption("SDL.StateRecorderPauseOnLoad", &opt);

	pauseOnLoadCbox = new QComboBox();
	pauseOnLoadCbox->addItem( tr("No"), StateRecorderConfigData::NO_PAUSE );
	pauseOnLoadCbox->addItem( tr("Temporary"), StateRecorderConfigData::TEMPORARY_PAUSE );
	pauseOnLoadCbox->addItem( tr("Full"), StateRecorderConfigData::FULL_PAUSE );

	pauseOnLoadCbox->setCurrentIndex( opt );

	connect( pauseOnLoadCbox, SIGNAL(currentIndexChanged(int)), this, SLOT(pauseOnLoadChanged(int)) );

	g_config->getOption("SDL.StateRecorderPauseDuration", &opt);

	pauseDuration = new QSpinBox();
	pauseDuration->setMinimum(0);
	pauseDuration->setMaximum(60);
	pauseDuration->setValue(opt);

	vbox1->addWidget(pauseOnLoadCbox);

	frame = new QGroupBox( tr("Duration:") );
	hbox  = new QHBoxLayout();

	vbox1->addWidget(frame);
	hbox->addWidget( pauseDuration);
	hbox->addWidget( new QLabel( tr("Seconds") ) );

	frame->setLayout(hbox);

	grid->addWidget(frame1, 4, 3, 2, 1);

	frame = new QGroupBox( tr("Memory Usage:") );
	memStatsGrid = new QGridLayout();

	numSnapsLbl      = new QLineEdit();
	snapMemSizeLbl   = new QLineEdit();
	totalMemUsageLbl = new QLineEdit();
	saveTimeLbl      = new QLineEdit();

	     numSnapsLbl->setReadOnly(true);
	  snapMemSizeLbl->setReadOnly(true);
	totalMemUsageLbl->setReadOnly(true);
	     saveTimeLbl->setReadOnly(true);

	grid->addWidget(frame, 1, 3, 2, 2);
	frame->setLayout(memStatsGrid);
	memStatsGrid->addWidget( new QLabel( tr("Number of\nSnapshots:") ), 0, 0 );
	memStatsGrid->addWidget( numSnapsLbl, 0, 1 );

	memStatsGrid->addWidget( new QLabel( tr("Snapshot Size:") ), 1, 0 );
	memStatsGrid->addWidget( snapMemSizeLbl, 1, 1 );

	memStatsGrid->addWidget( new QLabel( tr("Total Size:") ), 2, 0 );
	memStatsGrid->addWidget( totalMemUsageLbl, 2, 1 );

	frame = new QGroupBox( tr("CPU Usage:") );
	hbox  = new QHBoxLayout();
	frame->setLayout(hbox);

	hbox->addWidget(new QLabel(tr("Snapshot\nSave Time:")), 2);
	hbox->addWidget(saveTimeLbl, 2);

	grid->addWidget(frame, 3, 3, 1, 1);

	mainLayout->addLayout(grid);

	frame1 = new QGroupBox(tr("Recorder Status"));
	sysStatusGrid = new QGridLayout();
	frame1->setLayout(sysStatusGrid);

	frame = new QGroupBox();
	hbox  = new QHBoxLayout();

	sysStatusGrid->addWidget( frame, 0, 0, 1, 2);
	frame->setLayout(hbox);

	recStatusLbl = new QLineEdit();
	recStatusLbl->setReadOnly(true);
	startStopButton = new QPushButton( tr("Start") );
	hbox->addWidget( new QLabel(tr("State:")), 1 );
	hbox->addWidget( recStatusLbl, 2 );
	hbox->addWidget( startStopButton, 1 );

	updateStartStopBuffon();
	updateRecorderStatusLabel();

	connect(startStopButton, SIGNAL(clicked(void)), this, SLOT(startStopClicked(void)));
	connect(snapFrameSelBtn, SIGNAL(clicked(void)), this, SLOT(snapFrameModeClicked(void)));
	connect(snapTimeSelBtn , SIGNAL(clicked(void)), this, SLOT(snapTimeModeClicked(void)));

	frame = new QGroupBox();
	hbox  = new QHBoxLayout();

	sysStatusGrid->addWidget( frame, 0, 2, 1, 1);
	frame->setLayout(hbox);

	recBufSizeLbl = new QLineEdit();
	recBufSizeLbl->setReadOnly(true);
	hbox->addWidget( new QLabel(tr("Buffer Size:")), 1 );
	hbox->addWidget( recBufSizeLbl, 1 );

	frame = new QGroupBox( tr("Buffer Use:") );
	hbox  = new QHBoxLayout();

	sysStatusGrid->addWidget( frame, 1, 0, 1, 3);
	frame->setLayout(hbox);

	bufUsage = new QProgressBar();
	bufUsage->setToolTip( tr("% use of history record buffer.") );
	bufUsage->setOrientation( Qt::Horizontal );
	bufUsage->setMinimum(   0 );
	bufUsage->setMaximum( 100 );
	bufUsage->setValue( 0 );

	hbox->addWidget(bufUsage);

	updateBufferSizeStatus();

	mainLayout->addWidget(frame1);

	hbox  = new QHBoxLayout();
	mainLayout->addLayout(hbox);

	closeButton = new QPushButton( tr("Close") );
	applyButton = new QPushButton( tr("Apply") );

	closeButton->setIcon( style()->standardIcon( QStyle::SP_DialogCloseButton ) );
	applyButton->setIcon( style()->standardIcon( QStyle::SP_DialogApplyButton ) );

	connect(closeButton, SIGNAL(clicked(void)), this, SLOT(closeWindow(void)));
	connect(applyButton, SIGNAL(clicked(void)), this, SLOT(applyChanges(void)));

	hbox->addWidget(applyButton, 1);
	hbox->addStretch(8);
	hbox->addWidget(closeButton, 1);

	setLayout(mainLayout);

	restoreGeometry(settings.value("stateRecorderWindow/geometry").toByteArray());

	recalcMemoryUsage();

	pauseOnLoadChanged( pauseOnLoadCbox->currentIndex() );

	updateTimer = new QTimer(this);

	connect(updateTimer, &QTimer::timeout, this, &StateRecorderDialog_t::updatePeriodic);

	updateTimer->start(1000); // 1hz
}
//----------------------------------------------------------------------------
StateRecorderDialog_t::~StateRecorderDialog_t(void)
{
	//printf("Destroy State Recorder Config Window\n");
}
//----------------------------------------------------------------------------
void StateRecorderDialog_t::closeEvent(QCloseEvent *event)
{
	QSettings settings;
	settings.setValue("stateRecorderWindow/geometry", saveGeometry());

	if (dataSavedCheck())
	{
		done(0);
		deleteLater();
		event->accept();
	}
}
//----------------------------------------------------------------------------
void StateRecorderDialog_t::closeWindow(void)
{
	QSettings settings;
	settings.setValue("stateRecorderWindow/geometry", saveGeometry());

	if (dataSavedCheck())
	{
		done(0);
		deleteLater();
	}
}
//----------------------------------------------------------------------------
void StateRecorderDialog_t::packConfig( StateRecorderConfigData &config )
{
	config.timingMode = snapTimeSelBtn->isChecked() ? 
		StateRecorderConfigData::TIME : StateRecorderConfigData::FRAMES;

	config.framesBetweenSnaps = snapFrames->value();
	config.historyDurationMinutes = static_cast<float>( historyDuration->value() );
	config.timeBetweenSnapsMinutes = static_cast<float>( snapMinutes->value() ) +
		                          ( static_cast<float>( snapSeconds->value() ) / 60.0f );
	config.compressionLevel = cmprLvlCbox->currentData().toInt();
	config.loadPauseTimeSeconds = pauseDuration->value();
	config.pauseOnLoad = static_cast<StateRecorderConfigData::PauseType>( pauseOnLoadCbox->currentData().toInt() );
}
//----------------------------------------------------------------------------
bool StateRecorderDialog_t::dataSavedCheck(void)
{
	bool okToClose = true;
	const StateRecorderConfigData &curConfig = FCEU_StateRecorderGetConfigData();

	StateRecorderConfigData selConfig;

	packConfig( selConfig );

	if ( selConfig.compare( curConfig ) == false )
	{
		QMessageBox msgBox(QMessageBox::Question, tr("State Recorder"),
				tr("Setting selections have not yet been saved.\nDo you wish to save/apply the new settings?"),
				QMessageBox::No | QMessageBox::Yes, this);

		msgBox.setDefaultButton( QMessageBox::Yes );

		int ret = msgBox.exec();

		if ( ret == QMessageBox::Yes )
		{
			applyChanges();
		}
	}
	return okToClose;
}
//----------------------------------------------------------------------------
void StateRecorderDialog_t::applyChanges(void)
{
	StateRecorderConfigData config;

	packConfig( config );

	FCEU_WRAPPER_LOCK();
	FCEU_StateRecorderSetEnabled( recorderEnable->isChecked() );
	FCEU_StateRecorderSetConfigData( config );
	if (FCEU_StateRecorderRunning())
	{
		QMessageBox msgBox(QMessageBox::Question, tr("State Recorder"),
				tr("New settings will not take effect until state recorder is restarted. Do you wish to restart?"),
				QMessageBox::No | QMessageBox::Yes, this);

		msgBox.setDefaultButton( QMessageBox::Yes );

		int ret = msgBox.exec();

		if ( ret == QMessageBox::Yes )
		{
			FCEU_StateRecorderStop();
			FCEU_StateRecorderStart();
			updateStatusDisplay();
		}
	}
	FCEU_WRAPPER_UNLOCK();

	g_config->setOption("SDL.StateRecorderHistoryDurationMin", historyDuration->value() );
	g_config->setOption("SDL.StateRecorderTimingMode", snapTimeSelBtn->isChecked() ? 1 : 0);
	g_config->setOption("SDL.StateRecorderFramesBetweenSnaps", snapFrames->value() );
	g_config->setOption("SDL.StateRecorderTimeBetweenSnapsMin", snapMinutes->value() );
	g_config->setOption("SDL.StateRecorderTimeBetweenSnapsSec", snapSeconds->value() );
	g_config->setOption("SDL.StateRecorderCompressionLevel", config.compressionLevel);
	g_config->setOption("SDL.StateRecorderPauseOnLoad", config.pauseOnLoad);
	g_config->setOption("SDL.StateRecorderPauseDuration", config.loadPauseTimeSeconds);
	g_config->setOption("SDL.StateRecorderEnable", recorderEnable->isChecked() );
	g_config->save();
}
//----------------------------------------------------------------------------
void StateRecorderDialog_t::startStopClicked(void)
{
	FCEU_WRAPPER_LOCK();
	bool isRunning = FCEU_StateRecorderRunning();

	if (isRunning)
	{
		FCEU_StateRecorderStop();
	}
	else
	{
		FCEU_StateRecorderStart();
	}
	updateStatusDisplay();

	FCEU_WRAPPER_UNLOCK();
}
//----------------------------------------------------------------------------
void StateRecorderDialog_t::snapFrameModeClicked(void)
{
	snapUseTime = false;
	snapTimeGroup->setEnabled(false);
	snapFramesGroup->setEnabled(true);

	recalcMemoryUsage();
}
//----------------------------------------------------------------------------
void StateRecorderDialog_t::snapTimeModeClicked(void)
{
	snapUseTime = true;
	snapFramesGroup->setEnabled(false);
	snapTimeGroup->setEnabled(true);

	recalcMemoryUsage();
}
//----------------------------------------------------------------------------
void StateRecorderDialog_t::updateStartStopBuffon(void)
{
	bool isRunning = FCEU_StateRecorderRunning();

	if (isRunning)
	{
		startStopButton->setText( tr("Stop") );
		startStopButton->setIcon( style()->standardIcon( QStyle::SP_MediaStop ) );
	}
	else
	{
		startStopButton->setText( tr("Start") );
		startStopButton->setIcon( QIcon(":icons/media-record.png") );
	}
}
//----------------------------------------------------------------------------
void StateRecorderDialog_t::updateBufferSizeStatus(void)
{
	char stmp[64];

	int numSnapsSaved = FCEU_StateRecorderGetNumSnapsSaved();
	int maxSnaps      = FCEU_StateRecorderGetMaxSnaps();

	snprintf( stmp, sizeof(stmp), "%i", maxSnaps );

	recBufSizeLbl->setText( tr(stmp) );

	if (maxSnaps > 0)
	{
		bufUsage->setMaximum( maxSnaps );
	}
	bufUsage->setValue( numSnapsSaved );
}
//----------------------------------------------------------------------------
void StateRecorderDialog_t::updateRecorderStatusLabel(void)
{
	bool isRunning = FCEU_StateRecorderRunning();

	if (isRunning)
	{
		recStatusLbl->setText( tr("Recording") );
	}
	else
	{
		recStatusLbl->setText( tr("Off") );
	}
}
//----------------------------------------------------------------------------
void StateRecorderDialog_t::updateStatusDisplay(void)
{
	updateStartStopBuffon();
	updateRecorderStatusLabel();
	updateBufferSizeStatus();
}
//----------------------------------------------------------------------------
void StateRecorderDialog_t::updatePeriodic(void)
{
	FCEU_WRAPPER_LOCK();
	updateStatusDisplay();
	FCEU_WRAPPER_UNLOCK();
}
//----------------------------------------------------------------------------
void StateRecorderDialog_t::enableChanged(int val)
{
	bool ena = val ? true : false;

	FCEU_WRAPPER_LOCK();
	FCEU_StateRecorderSetEnabled( ena );
	FCEU_WRAPPER_UNLOCK();

	g_config->setOption("SDL.StateRecorderEnable", ena );
	g_config->save();
}
//----------------------------------------------------------------------------
void StateRecorderDialog_t::spinBoxValueChanged(int newValue)
{
	recalcMemoryUsage();
}
//----------------------------------------------------------------------------
void StateRecorderDialog_t::compressionLevelChanged(int newValue)
{
	recalcMemoryUsage();
}
//----------------------------------------------------------------------------
void StateRecorderDialog_t::pauseOnLoadChanged(int index)
{
	StateRecorderConfigData::PauseType pauseOnLoad;

	pauseOnLoad = static_cast<StateRecorderConfigData::PauseType>( pauseOnLoadCbox->currentData().toInt() );

	pauseDuration->setEnabled( pauseOnLoad == StateRecorderConfigData::TEMPORARY_PAUSE );
}
//----------------------------------------------------------------------------
void StateRecorderDialog_t::recalcMemoryUsage(void)
{
	char stmp[64];

	float fsnapMin = 1.0;
	float fhistMin = static_cast<float>( historyDuration->value() );

	if (snapUseTime)
	{
		fsnapMin =   static_cast<float>( snapMinutes->value() ) +
		        ( static_cast<float>( snapSeconds->value() ) / 60.0f );
	}
	else
	{
		int32_t fps = FCEUI_GetDesiredFPS(); // Do >> 24 to get in Hz
		double hz = ( ((double)fps) / 16777216.0 );

		fsnapMin  = static_cast<double>(snapFrames->value()) / (hz * 60.0);
	}

	float fnumSnaps = fhistMin / fsnapMin;
	float fsnapSize = 10.0f * 1024.0f;
	float ftotalSize;
	constexpr float oneKiloByte = 1024.0f;
	constexpr float oneMegaByte = 1024.0f * 1024.0f;

	int inumSnaps = static_cast<int>( fnumSnaps + 0.5f );

	sprintf( stmp, "%i", inumSnaps );
	
	numSnapsLbl->setText( tr(stmp) );

	saveTimeMs = 0.0;

	if (GameInfo)
	{
		constexpr int numIterations = 10;
		double ts_start, ts_end;

		FCEU_WRAPPER_LOCK();

		EMUFILE_MEMORY em;
		int compressionLevel = cmprLvlCbox->currentData().toInt();

		ts_start = getHighPrecTimeStamp();

		// Perform State Save multiple times to get a good average
		// on what the compression delays will be.
		for (int i=0; i<numIterations; i++)
		{
			em.set_len(0);
			FCEUSS_SaveMS( &em, compressionLevel );
		}
		ts_end   = getHighPrecTimeStamp();

		saveTimeMs = (ts_end - ts_start) * 1000.0 / static_cast<double>(numIterations);

		fsnapSize = static_cast<float>( em.size() );

		FCEU_WRAPPER_UNLOCK();
	}

	if (fsnapSize >= oneKiloByte)
	{
		sprintf( stmp, "%.02f kB", fsnapSize / oneKiloByte );
	}
	else
	{
		sprintf( stmp, "%.0f B", fsnapSize );
	}

	snapMemSizeLbl->setText( tr(stmp) );

	ftotalSize = fsnapSize * static_cast<float>(inumSnaps);

	if (ftotalSize >= oneMegaByte)
	{
		sprintf( stmp, "%.02f MB", ftotalSize / oneMegaByte );
	}
	else if (ftotalSize >= oneKiloByte)
	{
		sprintf( stmp, "%.02f kB", ftotalSize / oneKiloByte );
	}
	else
	{
		sprintf( stmp, "%.0f B", ftotalSize );
	}

	totalMemUsageLbl->setText( tr(stmp) );

	sprintf( stmp, "%.02f ms", saveTimeMs);
	saveTimeLbl->setText( tr(stmp) );
}
//----------------------------------------------------------------------------
