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
// TimingConf.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#ifndef WIN32
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include <SDL.h>
#include <QHeaderView>
#include <QCloseEvent>

#include "fceu.h"
#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/throttle.h"
#include "Qt/fceuWrapper.h"
#include "Qt/ConsoleWindow.h"
#include "Qt/TimingConf.h"

//----------------------------------------------------------------------------
static bool hasNicePermissions(int val)
{
#ifndef WIN32
	int usrID;
	bool usrRoot;

	usrID = geteuid();

	usrRoot = (usrID == 0);

	if (usrRoot)
	{
		return true;
	}
#ifdef __linux__
	struct rlimit r;

	if (getrlimit(RLIMIT_NICE, &r) == 0)
	{
		int ncur = 20 - r.rlim_cur;

		if (val >= ncur)
		{
			return true;
		}
		//printf("RLim Cur: %lu \n", r.rlim_cur );
		//printf("RLim Max: %lu \n", r.rlim_max );
	}
#endif

#endif
	return false;
}
//----------------------------------------------------------------------------
TimingConfDialog_t::TimingConfDialog_t(QWidget *parent)
	: QDialog(parent)
{
	int opt;
	QVBoxLayout *mainLayout, *vbox;
	QHBoxLayout *hbox;
	QGridLayout *grid;
	QPushButton *closeButton;
	QGroupBox *emuPrioBox, *guiPrioBox;

	setWindowTitle("Timing Configuration");

	mainLayout = new QVBoxLayout();

	emuPrioCtlEna = new QCheckBox(tr("Set Scheduling Parameters at Startup"));

	emuPrioBox = new QGroupBox(tr("EMU Thread Scheduling Parameters"));
	guiPrioBox = new QGroupBox(tr("GUI Thread Scheduling Parameters"));
	grid = new QGridLayout();
	emuPrioBox->setLayout(grid);

	mainLayout->addWidget(emuPrioCtlEna);
	mainLayout->addWidget(emuPrioBox);
	mainLayout->addWidget(guiPrioBox);

#ifdef WIN32
	emuSchedPrioBox = new QComboBox();
	guiSchedPrioBox = new QComboBox();

	grid->addWidget(emuSchedPrioBox, 0, 0);

	grid = new QGridLayout();
	guiPrioBox->setLayout(grid);

	grid->addWidget(guiSchedPrioBox, 0, 0);

	emuSchedPrioBox->addItem(tr("Idle"), QThread::IdlePriority);
	emuSchedPrioBox->addItem(tr("Lowest"), QThread::LowestPriority);
	emuSchedPrioBox->addItem(tr("Low"), QThread::LowPriority);
	emuSchedPrioBox->addItem(tr("Normal"), QThread::NormalPriority);
	emuSchedPrioBox->addItem(tr("High"), QThread::HighPriority);
	emuSchedPrioBox->addItem(tr("Highest"), QThread::HighestPriority);
	emuSchedPrioBox->addItem(tr("Time Critical"), QThread::TimeCriticalPriority);
	emuSchedPrioBox->addItem(tr("Inherit"), QThread::InheritPriority);

	guiSchedPrioBox->addItem(tr("Idle"), QThread::IdlePriority);
	guiSchedPrioBox->addItem(tr("Lowest"), QThread::LowestPriority);
	guiSchedPrioBox->addItem(tr("Low"), QThread::LowPriority);
	guiSchedPrioBox->addItem(tr("Normal"), QThread::NormalPriority);
	guiSchedPrioBox->addItem(tr("High"), QThread::HighPriority);
	guiSchedPrioBox->addItem(tr("Highest"), QThread::HighestPriority);
	guiSchedPrioBox->addItem(tr("Time Critical"), QThread::TimeCriticalPriority);
	guiSchedPrioBox->addItem(tr("Inherit"), QThread::InheritPriority);
#else
	emuSchedPolicyBox = new QComboBox();
	emuSchedPrioSlider = new QSlider(Qt::Horizontal);
	emuSchedNiceSlider = new QSlider(Qt::Horizontal);
	emuSchedPrioLabel = new QLabel(tr("Priority (RT)"));
	emuSchedNiceLabel = new QLabel(tr("Priority (Nice)"));

	emuSchedPolicyBox->addItem(tr("SCHED_OTHER"), SCHED_OTHER);
	emuSchedPolicyBox->addItem(tr("SCHED_FIFO"), SCHED_FIFO);
	emuSchedPolicyBox->addItem(tr("SCHED_RR"), SCHED_RR);

	grid->addWidget(new QLabel(tr("Policy")), 0, 0);
	grid->addWidget(emuSchedPolicyBox, 0, 1);
	grid->addWidget(emuSchedPrioLabel, 1, 0);
	grid->addWidget(emuSchedPrioSlider, 1, 1);
	grid->addWidget(emuSchedNiceLabel, 2, 0);
	grid->addWidget(emuSchedNiceSlider, 2, 1);

	grid = new QGridLayout();
	guiPrioBox->setLayout(grid);

	guiSchedPolicyBox = new QComboBox();
	guiSchedPrioSlider = new QSlider(Qt::Horizontal);
	guiSchedNiceSlider = new QSlider(Qt::Horizontal);
	guiSchedPrioLabel = new QLabel(tr("Priority (RT)"));
	guiSchedNiceLabel = new QLabel(tr("Priority (Nice)"));

	guiSchedPolicyBox->addItem(tr("SCHED_OTHER"), SCHED_OTHER);
	guiSchedPolicyBox->addItem(tr("SCHED_FIFO"), SCHED_FIFO);
	guiSchedPolicyBox->addItem(tr("SCHED_RR"), SCHED_RR);

	grid->addWidget(new QLabel(tr("Policy")), 0, 0);
	grid->addWidget(guiSchedPolicyBox, 0, 1);
	grid->addWidget(guiSchedPrioLabel, 1, 0);
	grid->addWidget(guiSchedPrioSlider, 1, 1);
	grid->addWidget(guiSchedNiceLabel, 2, 0);
	grid->addWidget(guiSchedNiceSlider, 2, 1);
#endif

	hbox = new QHBoxLayout();
	timingDevSelBox = new QComboBox();
#ifdef WIN32
	timingDevSelBox->addItem(tr("SDL_Delay"), 0);
#else
	timingDevSelBox->addItem(tr("NanoSleep"), 0);
#endif

#ifdef __linux__
	timingDevSelBox->addItem(tr("Timer FD"), 1);
#endif
	hbox->addWidget(new QLabel(tr("Timing Mechanism:")));
	hbox->addWidget(timingDevSelBox);
	mainLayout->addLayout(hbox);

	vbox = new QVBoxLayout();
	grid = new QGridLayout();
	ppuOverClockBox = new QGroupBox( tr("Overclocking (Old PPU Only)") );
	ppuOverClockBox->setCheckable(true);
	ppuOverClockBox->setChecked(overclock_enabled);
	ppuOverClockBox->setEnabled(!newppu);
	ppuOverClockBox->setLayout(vbox);
	mainLayout->addWidget( ppuOverClockBox );

	postRenderBox      = new QSpinBox();
	vblankScanlinesBox = new QSpinBox();
	no7bitSamples      = new QCheckBox( tr("Don't Overclock 7-bit Samples") );

	postRenderBox->setRange(0, 999);
	vblankScanlinesBox->setRange(0, 999);

	postRenderBox->setValue( postrenderscanlines );
	vblankScanlinesBox->setValue( vblankscanlines );
	no7bitSamples->setChecked( skip_7bit_overclocking );

	vbox->addLayout( grid );
	grid->addWidget( new QLabel( tr("Post-render") ), 0, 0 );
	grid->addWidget( new QLabel( tr("Vblank Scanlines") ), 1, 0 );
	grid->addWidget( postRenderBox, 0, 1 );
	grid->addWidget( vblankScanlinesBox, 1, 1 );
	vbox->addWidget( no7bitSamples );

	closeButton = new QPushButton( tr("Close") );
	closeButton->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
	connect(closeButton, SIGNAL(clicked(void)), this, SLOT(closeWindow(void)));

	hbox = new QHBoxLayout();
	hbox->addStretch(5);
	hbox->addWidget( closeButton, 1 );
	mainLayout->addLayout( hbox );

	setLayout(mainLayout);

	g_config->getOption("SDL.SetSchedParam", &opt);

	emuPrioCtlEna->setChecked(opt);

	updatePolicyBox();
	updateSliderLimits();
	updateSliderValues();
	updateTimingMech();
	updateOverclocking();

#ifdef WIN32
	connect(emuSchedPrioBox, SIGNAL(activated(int)), this, SLOT(emuSchedPrioChange(int)));
	connect(guiSchedPrioBox, SIGNAL(activated(int)), this, SLOT(guiSchedPrioChange(int)));
#else
	connect(emuSchedPolicyBox, SIGNAL(activated(int)), this, SLOT(emuSchedPolicyChange(int)));
	connect(emuSchedNiceSlider, SIGNAL(valueChanged(int)), this, SLOT(emuSchedNiceChange(int)));
	connect(emuSchedPrioSlider, SIGNAL(valueChanged(int)), this, SLOT(emuSchedPrioChange(int)));
	connect(guiSchedPolicyBox, SIGNAL(activated(int)), this, SLOT(guiSchedPolicyChange(int)));
	connect(guiSchedNiceSlider, SIGNAL(valueChanged(int)), this, SLOT(guiSchedNiceChange(int)));
	connect(guiSchedPrioSlider, SIGNAL(valueChanged(int)), this, SLOT(guiSchedPrioChange(int)));
#endif
	connect(emuPrioCtlEna, SIGNAL(stateChanged(int)), this, SLOT(emuSchedCtlChange(int)));
	connect(timingDevSelBox, SIGNAL(activated(int)), this, SLOT(emuTimingMechChange(int)));

	connect( ppuOverClockBox   , SIGNAL(toggled(bool))    , this, SLOT(overclockingToggled(bool)));
	connect( postRenderBox     , SIGNAL(valueChanged(int)), this, SLOT(postRenderChanged(int)));
	connect( vblankScanlinesBox, SIGNAL(valueChanged(int)), this, SLOT(vblankScanlinesChanged(int)));
	connect( no7bitSamples     , SIGNAL(stateChanged(int)), this, SLOT(no7bitChanged(int)));

	updateTimer  = new QTimer( this );

	connect( updateTimer, &QTimer::timeout, this, &TimingConfDialog_t::periodicUpdate );

	updateTimer->start( 500 ); // 2Hz
}
//----------------------------------------------------------------------------
TimingConfDialog_t::~TimingConfDialog_t(void)
{
	printf("Destroy Timing Config Window\n");
	updateTimer->stop();
	saveValues();
}
//----------------------------------------------------------------------------
void TimingConfDialog_t::closeEvent(QCloseEvent *event)
{
	printf("Timing Close Window Event\n");
	done(0);
	deleteLater();
	event->accept();
}
//----------------------------------------------------------------------------
void TimingConfDialog_t::closeWindow(void)
{
	//printf("Close Window\n");
	done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
void TimingConfDialog_t::periodicUpdate(void)
{
	updateOverclocking();

}
//----------------------------------------------------------------------------
void TimingConfDialog_t::emuSchedCtlChange(int state)
{
	g_config->setOption("SDL.SetSchedParam", (state != Qt::Unchecked));
}
//----------------------------------------------------------------------------
void TimingConfDialog_t::saveValues(void)
{
#ifndef WIN32
	int policy, prio, nice;

	if (consoleWindow == NULL)
	{
		return;
	}
	nice = consoleWindow->emulatorThread->getNicePriority();

	consoleWindow->emulatorThread->getSchedParam(policy, prio);

	g_config->setOption("SDL.EmuSchedPolicy", policy);
	g_config->setOption("SDL.EmuSchedPrioRt", prio);
	g_config->setOption("SDL.EmuSchedNice", nice);

	//printf("EMU Sched: %i  %i  %i\n", policy, prio, nice );

	nice = consoleWindow->getNicePriority();

	consoleWindow->getSchedParam(policy, prio);

	g_config->setOption("SDL.GuiSchedPolicy", policy);
	g_config->setOption("SDL.GuiSchedPrioRt", prio);
	g_config->setOption("SDL.GuiSchedNice", nice);

	//printf("GUI Sched: %i  %i  %i\n", policy, prio, nice );

	g_config->save();
#endif
}
//----------------------------------------------------------------------------
void TimingConfDialog_t::emuSchedNiceChange(int val)
{
#ifndef WIN32
	if (consoleWindow == NULL)
	{
		return;
	}
	fceuWrapperLock();
	if (consoleWindow->emulatorThread->setNicePriority(-val))
	{
		char msg[1024];

		sprintf(msg, "Error: system call setPriority Failed\nReason: %s\n", strerror(errno));
#ifdef __linux__
		strcat(msg, "Ensure that your system has the proper resource permissions set in the file:\n\n");
		strcat(msg, "        /etc/security/limits.conf \n\n");
		strcat(msg, "Adding the following lines to that file and rebooting will usually fix the issue:\n\n");
		strcat(msg, "*  -  priority   99 \n");
		strcat(msg, "*  -  rtprio     99 \n");
		strcat(msg, "*  -  nice      -20 \n");
#endif
		printf("%s\n", msg);
		consoleWindow->QueueErrorMsgWindow(msg);
		updateSliderValues();
	}
	fceuWrapperUnLock();
#endif
}
//----------------------------------------------------------------------------
void TimingConfDialog_t::emuSchedPrioChange(int val)
{
	if (consoleWindow == NULL)
	{
		return;
	}
#ifdef WIN32
	printf("Setting EMU Thread to %i\n", val);
	fceuWrapperLock();
	consoleWindow->emulatorThread->setPriority((QThread::Priority)val);
	fceuWrapperUnLock();
#else
	int policy, prio;

	fceuWrapperLock();
	consoleWindow->emulatorThread->getSchedParam(policy, prio);

	if (consoleWindow->emulatorThread->setSchedParam(policy, val))
	{
		char msg[1024];

		sprintf(msg, "Error: system call pthread_setschedparam Failed\nReason: %s\n", strerror(errno));
#ifdef __linux__
		strcat(msg, "Ensure that your system has the proper resource permissions set in the file:\n\n");
		strcat(msg, "        /etc/security/limits.conf \n\n");
		strcat(msg, "Adding the following lines to that file and rebooting will usually fix the issue:\n\n");
		strcat(msg, "*  -  priority   99 \n");
		strcat(msg, "*  -  rtprio     99 \n");
		strcat(msg, "*  -  nice      -20 \n");
#endif
		printf("%s\n", msg);
		consoleWindow->QueueErrorMsgWindow(msg);
		updateSliderValues();
	}
	fceuWrapperUnLock();
#endif
}
//----------------------------------------------------------------------------
void TimingConfDialog_t::emuSchedPolicyChange(int index)
{
#ifndef WIN32
	int policy, prio;

	if (consoleWindow == NULL)
	{
		return;
	}
	fceuWrapperLock();
	consoleWindow->emulatorThread->getSchedParam(policy, prio);

	policy = emuSchedPolicyBox->itemData(index).toInt();

	if (consoleWindow->emulatorThread->setSchedParam(policy, prio))
	{
		char msg[1024];

		sprintf(msg, "Error: system call pthread_setschedparam Failed\nReason: %s\n", strerror(errno));
#ifdef __linux__
		strcat(msg, "Ensure that your system has the proper resource permissions set in the file:\n\n");
		strcat(msg, "        /etc/security/limits.conf \n\n");
		strcat(msg, "Adding the following lines to that file and rebooting will usually fix the issue:\n\n");
		strcat(msg, "*  -  priority   99 \n");
		strcat(msg, "*  -  rtprio     99 \n");
		strcat(msg, "*  -  nice      -20 \n");
#endif
		printf("%s\n", msg);
		consoleWindow->QueueErrorMsgWindow(msg);
	}

	updatePolicyBox();
	updateSliderLimits();
	updateSliderValues();
	fceuWrapperUnLock();
#endif
}
//----------------------------------------------------------------------------
void TimingConfDialog_t::guiSchedNiceChange(int val)
{
#ifndef WIN32
	if (consoleWindow == NULL)
	{
		return;
	}
	fceuWrapperLock();
	if (consoleWindow->setNicePriority(-val))
	{
		char msg[1024];

		sprintf(msg, "Error: system call setPriority Failed\nReason: %s\n", strerror(errno));
#ifdef __linux__
		strcat(msg, "Ensure that your system has the proper resource permissions set in the file:\n\n");
		strcat(msg, "        /etc/security/limits.conf \n\n");
		strcat(msg, "Adding the following lines to that file and rebooting will usually fix the issue:\n\n");
		strcat(msg, "*  -  priority   99 \n");
		strcat(msg, "*  -  rtprio     99 \n");
		strcat(msg, "*  -  nice      -20 \n");
#endif
		printf("%s\n", msg);
		consoleWindow->QueueErrorMsgWindow(msg);
		updateSliderValues();
	}
	fceuWrapperUnLock();
#endif
}
//----------------------------------------------------------------------------
void TimingConfDialog_t::guiSchedPrioChange(int val)
{
#ifdef WIN32
	printf("Setting GUI Thread to %i\n", val);
	QThread::currentThread()->setPriority((QThread::Priority)val);
#else
	int policy, prio;

	if (consoleWindow == NULL)
	{
		return;
	}
	fceuWrapperLock();
	consoleWindow->getSchedParam(policy, prio);

	if (consoleWindow->setSchedParam(policy, val))
	{
		char msg[1024];

		sprintf(msg, "Error: system call pthread_setschedparam Failed\nReason: %s\n", strerror(errno));
#ifdef __linux__
		strcat(msg, "Ensure that your system has the proper resource permissions set in the file:\n\n");
		strcat(msg, "        /etc/security/limits.conf \n\n");
		strcat(msg, "Adding the following lines to that file and rebooting will usually fix the issue:\n\n");
		strcat(msg, "*  -  priority   99 \n");
		strcat(msg, "*  -  rtprio     99 \n");
		strcat(msg, "*  -  nice      -20 \n");
#endif
		printf("%s\n", msg);
		consoleWindow->QueueErrorMsgWindow(msg);
		updateSliderValues();
	}
	fceuWrapperUnLock();
#endif
}
//----------------------------------------------------------------------------
void TimingConfDialog_t::guiSchedPolicyChange(int index)
{
#ifndef WIN32
	int policy, prio;

	if (consoleWindow == NULL)
	{
		return;
	}
	fceuWrapperLock();
	consoleWindow->getSchedParam(policy, prio);

	policy = guiSchedPolicyBox->itemData(index).toInt();

	if (consoleWindow->setSchedParam(policy, prio))
	{
		char msg[1024];

		sprintf(msg, "Error: system call pthread_setschedparam Failed\nReason: %s\n", strerror(errno));
#ifdef __linux__
		strcat(msg, "Ensure that your system has the proper resource permissions set in the file:\n\n");
		strcat(msg, "        /etc/security/limits.conf \n\n");
		strcat(msg, "Adding the following lines to that file and rebooting will usually fix the issue:\n\n");
		strcat(msg, "*  -  priority   99 \n");
		strcat(msg, "*  -  rtprio     99 \n");
		strcat(msg, "*  -  nice      -20 \n");
#endif
		printf("%s\n", msg);
		consoleWindow->QueueErrorMsgWindow(msg);
	}

	updatePolicyBox();
	updateSliderLimits();
	updateSliderValues();
	fceuWrapperUnLock();
#endif
}
//----------------------------------------------------------------------------
void TimingConfDialog_t::updatePolicyBox(void)
{
	if (consoleWindow == NULL)
	{
		return;
	}
#ifdef WIN32
	int prio;

	prio = consoleWindow->emulatorThread->priority();

	printf("EMU Priority %i\n", prio);
	for (int j = 0; j < emuSchedPrioBox->count(); j++)
	{
		if (emuSchedPrioBox->itemData(j).toInt() == prio)
		{
			printf("EMU Found Priority %i  %i\n", j, prio);
			emuSchedPrioBox->setCurrentIndex(j);
		}
	}

	prio = QThread::currentThread()->priority();

	for (int j = 0; j < guiSchedPrioBox->count(); j++)
	{
		if (guiSchedPrioBox->itemData(j).toInt() == prio)
		{
			printf("GUI Found Priority %i  %i\n", j, prio);
			guiSchedPrioBox->setCurrentIndex(j);
		}
	}
#else
	int policy, prio;

	consoleWindow->emulatorThread->getSchedParam(policy, prio);

	for (int j = 0; j < emuSchedPolicyBox->count(); j++)
	{
		if (emuSchedPolicyBox->itemData(j).toInt() == policy)
		{
			//printf("Found Policy %i  %i\n",  j , policy );
			emuSchedPolicyBox->setCurrentIndex(j);
		}
	}

	consoleWindow->getSchedParam(policy, prio);

	for (int j = 0; j < guiSchedPolicyBox->count(); j++)
	{
		if (guiSchedPolicyBox->itemData(j).toInt() == policy)
		{
			//printf("Found Policy %i  %i\n",  j , policy );
			guiSchedPolicyBox->setCurrentIndex(j);
		}
	}
#endif
}
//----------------------------------------------------------------------------
void TimingConfDialog_t::updateSliderValues(void)
{
#ifndef WIN32
	int policy, prio;
	bool hasNicePerms;

	if (consoleWindow == NULL)
	{
		return;
	}
	consoleWindow->emulatorThread->getSchedParam(policy, prio);

	emuSchedNiceSlider->setValue(-consoleWindow->emulatorThread->getNicePriority());
	emuSchedPrioSlider->setValue(prio);

	if ((policy == SCHED_RR) || (policy == SCHED_FIFO))
	{
		emuSchedPrioLabel->setEnabled(true);
		emuSchedPrioSlider->setEnabled(true);
	}
	else
	{
		emuSchedPrioLabel->setEnabled(false);
		emuSchedPrioSlider->setEnabled(false);
	}
	hasNicePerms = hasNicePermissions(consoleWindow->emulatorThread->getNicePriority());

	emuSchedNiceLabel->setEnabled(hasNicePerms);
	emuSchedNiceSlider->setEnabled(hasNicePerms);

	consoleWindow->getSchedParam(policy, prio);

	guiSchedNiceSlider->setValue(-consoleWindow->getNicePriority());
	guiSchedPrioSlider->setValue(prio);

	if ((policy == SCHED_RR) || (policy == SCHED_FIFO))
	{
		guiSchedPrioLabel->setEnabled(true);
		guiSchedPrioSlider->setEnabled(true);
	}
	else
	{
		guiSchedPrioLabel->setEnabled(false);
		guiSchedPrioSlider->setEnabled(false);
	}
	hasNicePerms = hasNicePermissions(consoleWindow->getNicePriority());

	guiSchedNiceLabel->setEnabled(hasNicePerms);
	guiSchedNiceSlider->setEnabled(hasNicePerms);
#endif
}
//----------------------------------------------------------------------------
void TimingConfDialog_t::updateSliderLimits(void)
{
	if (consoleWindow == NULL)
	{
		return;
	}

#ifndef WIN32
	emuSchedNiceSlider->setMinimum(-20);
	emuSchedNiceSlider->setMaximum(20);
	emuSchedPrioSlider->setMinimum(consoleWindow->emulatorThread->getMinSchedPriority());
	emuSchedPrioSlider->setMaximum(consoleWindow->emulatorThread->getMaxSchedPriority());

	guiSchedNiceSlider->setMinimum(-20);
	guiSchedNiceSlider->setMaximum(20);
	guiSchedPrioSlider->setMinimum(consoleWindow->getMinSchedPriority());
	guiSchedPrioSlider->setMaximum(consoleWindow->getMaxSchedPriority());
#endif
}
//----------------------------------------------------------------------------
void TimingConfDialog_t::emuTimingMechChange(int index)
{
	int mode;

	if (consoleWindow == NULL)
	{
		return;
	}
	fceuWrapperLock();

	mode = timingDevSelBox->itemData(index).toInt();

	setTimingMode(mode);

	RefreshThrottleFPS();

	g_config->setOption("SDL.EmuTimingMech", mode);

	fceuWrapperUnLock();
}
//----------------------------------------------------------------------------
void TimingConfDialog_t::updateTimingMech(void)
{
	int mode = getTimingMode();

	for (int j = 0; j < timingDevSelBox->count(); j++)
	{
		if (timingDevSelBox->itemData(j).toInt() == mode)
		{
			timingDevSelBox->setCurrentIndex(j);
		}
	}
}
//----------------------------------------------------------------------------
void TimingConfDialog_t::overclockingToggled(bool on)
{
	fceuWrapperLock();
	if ( !newppu )
	{
		overclock_enabled = on;
	}
	fceuWrapperUnLock();
}
//----------------------------------------------------------------------------
void TimingConfDialog_t::postRenderChanged(int value)
{
	fceuWrapperLock();
	postrenderscanlines = value;
	fceuWrapperUnLock();
	//printf("Post Render %i\n", postrenderscanlines );
}
//----------------------------------------------------------------------------
void TimingConfDialog_t::vblankScanlinesChanged(int value)
{
	fceuWrapperLock();
	vblankscanlines = value;
	fceuWrapperUnLock();
	//printf("Vblank Scanlines %i\n", vblankscanlines );
}
//----------------------------------------------------------------------------
void TimingConfDialog_t::no7bitChanged(int value)
{
	fceuWrapperLock();
	skip_7bit_overclocking = (value != Qt::Unchecked);
	fceuWrapperUnLock();
	//printf("Skip 7-bit: %i\n", skip_7bit_overclocking );
}
//----------------------------------------------------------------------------
void TimingConfDialog_t::updateOverclocking(void)
{
	ppuOverClockBox->setEnabled( !newppu );
	ppuOverClockBox->setChecked( overclock_enabled );
}
//----------------------------------------------------------------------------
