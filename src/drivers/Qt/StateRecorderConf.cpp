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

#include "Qt/fceuWrapper.h"
#include "Qt/StateRecorderConf.h"

#include "../../fceu.h"
#include "../../state.h"
#include "../../emufile.h"

//----------------------------------------------------------------------------
StateRecorderDialog_t::StateRecorderDialog_t(QWidget *parent)
	: QDialog(parent)
{
	QVBoxLayout *mainLayout;
	QHBoxLayout *hbox, *hbox1;
	QGroupBox *frame, *frame1;
	QGridLayout *grid, *memStatsGrid;
	QSettings settings;

	setWindowTitle("State Recorder Config");

	mainLayout = new QVBoxLayout();
	      grid = new QGridLayout();

	recorderEnable  = new QCheckBox(tr("Auto Start Recorder at ROM Load"));
	snapMinutes     = new QSpinBox();
	snapSeconds     = new QSpinBox();
	historyDuration = new QSpinBox();

	recorderEnable->setChecked( FCEU_StateRecorderIsEnabled() );

	connect( recorderEnable, SIGNAL(stateChanged(int)), this, SLOT(enableChanged(int)) );

	snapSeconds->setMinimum(0);
	snapSeconds->setMaximum(60);
	snapSeconds->setValue(3);
	snapMinutes->setMinimum(0);
	snapMinutes->setMaximum(60);
	snapMinutes->setValue(0);
	historyDuration->setMinimum(1);
	historyDuration->setMaximum(180);
	historyDuration->setValue(15);

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

	frame1 = new QGroupBox(tr("Time Between Snapshots:"));
	hbox1  = new QHBoxLayout();
	frame1->setLayout(hbox1);
	grid->addWidget( frame1, 2, 0 );

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

	frame = new QGroupBox( tr("Memory Usage:") );
	memStatsGrid = new QGridLayout();

	numSnapsLbl      = new QLineEdit();
	snapMemSizeLbl   = new QLineEdit();
	totalMemUsageLbl = new QLineEdit();

	     numSnapsLbl->setReadOnly(true);
	  snapMemSizeLbl->setReadOnly(true);
	totalMemUsageLbl->setReadOnly(true);

	grid->addWidget(frame, 1, 2, 2, 2);
	frame->setLayout(memStatsGrid);
	memStatsGrid->addWidget( new QLabel( tr("Number of\nSnapshots:") ), 0, 0 );
	memStatsGrid->addWidget( numSnapsLbl, 0, 1 );

	memStatsGrid->addWidget( new QLabel( tr("Snapshot Size:") ), 1, 0 );
	memStatsGrid->addWidget( snapMemSizeLbl, 1, 1 );

	memStatsGrid->addWidget( new QLabel( tr("Total Size:") ), 2, 0 );
	memStatsGrid->addWidget( totalMemUsageLbl, 2, 1 );

	mainLayout->addLayout(grid);

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
	done(0);
	deleteLater();
	event->accept();
}
//----------------------------------------------------------------------------
void StateRecorderDialog_t::closeWindow(void)
{
	QSettings settings;
	settings.setValue("stateRecorderWindow/geometry", saveGeometry());
	done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
void StateRecorderDialog_t::applyChanges(void)
{

}
//----------------------------------------------------------------------------
void StateRecorderDialog_t::enableChanged(int val)
{
	bool ena = val ? true : false;

	FCEU_StateRecorderSetEnabled( ena );

	g_config->setOption("SDL.StateRecorderEnable", ena);
	g_config->save();
}
//----------------------------------------------------------------------------
void StateRecorderDialog_t::spinBoxValueChanged(int newValue)
{
	recalcMemoryUsage();
}
//----------------------------------------------------------------------------
void StateRecorderDialog_t::recalcMemoryUsage(void)
{
	char stmp[64];

	float fhistMin = static_cast<float>( historyDuration->value() );
	float fsnapMin =   static_cast<float>( snapMinutes->value() ) +
		        ( static_cast<float>( snapSeconds->value() ) / 60.0f );

	float fnumSnaps = fhistMin / fsnapMin;
	float fsnapSize = 10.0f * 1024.0f;
	float ftotalSize;
	constexpr float oneKiloByte = 1024.0f;
	constexpr float oneMegaByte = 1024.0f * 1024.0f;

	int inumSnaps = static_cast<int>( fnumSnaps + 0.5f );

	sprintf( stmp, "%i", inumSnaps );
	
	numSnapsLbl->setText( tr(stmp) );

	if (GameInfo)
	{
		EMUFILE_MEMORY em;
		int compressionLevel = 0;

		FCEUSS_SaveMS( &em, compressionLevel );

		fsnapSize = static_cast<float>( em.size() ) / 1024.0f;
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
}
//----------------------------------------------------------------------------
