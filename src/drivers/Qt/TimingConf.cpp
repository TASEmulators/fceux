// TimingConf.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <SDL.h>
#include <QHeaderView>
#include <QCloseEvent>

#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/fceuWrapper.h"
#include "Qt/ConsoleWindow.h"
#include "Qt/TimingConf.h"

//----------------------------------------------------------------------------
TimingConfDialog_t::TimingConfDialog_t(QWidget *parent)
	: QDialog( parent )
{
	QVBoxLayout *mainLayout, *vbox;
	//QHBoxLayout *hbox;
	QGridLayout *grid;
	QGroupBox   *emuPrioBox, *guiPrioBox;

	setWindowTitle("Timing Configuration");

	mainLayout = new QVBoxLayout();

	emuPrioCtlEna = new QCheckBox( tr("Set Scheduling Parameters at Startup") );

	emuPrioBox = new QGroupBox( tr("EMU Thread Scheduling Parameters") );
	guiPrioBox = new QGroupBox( tr("GUI Thread Scheduling Parameters") );
	grid       = new QGridLayout();
	emuPrioBox->setLayout( grid );

	emuSchedPolicyBox  = new QComboBox();
	emuSchedPrioSlider = new QSlider( Qt::Horizontal );
	emuSchedNiceSlider = new QSlider( Qt::Horizontal );
	emuSchedPrioLabel  = new QLabel( tr("Priority (RT)") );

	emuSchedPolicyBox->addItem( tr("SCHED_OTHER") , SCHED_OTHER );
	emuSchedPolicyBox->addItem( tr("SCHED_FIFO")  , SCHED_FIFO  );
	emuSchedPolicyBox->addItem( tr("SCHED_RR")    , SCHED_RR    );

	grid->addWidget( new QLabel( tr("Policy") ), 0, 0 );
	grid->addWidget( emuSchedPolicyBox, 0, 1 );
	grid->addWidget( emuSchedPrioLabel, 1, 0 );
	grid->addWidget( emuSchedPrioSlider, 1, 1 );
	grid->addWidget( new QLabel( tr("Priority (Nice)") ), 2, 0 );
	grid->addWidget( emuSchedNiceSlider, 2, 1 );

	mainLayout->addWidget( emuPrioCtlEna );
	mainLayout->addWidget( emuPrioBox );

	grid       = new QGridLayout();
	guiPrioBox->setLayout( grid );

	guiSchedPolicyBox  = new QComboBox();
	guiSchedPrioSlider = new QSlider( Qt::Horizontal );
	guiSchedNiceSlider = new QSlider( Qt::Horizontal );
	guiSchedPrioLabel  = new QLabel( tr("Priority (RT)") );

	guiSchedPolicyBox->addItem( tr("SCHED_OTHER") , SCHED_OTHER );
	guiSchedPolicyBox->addItem( tr("SCHED_FIFO")  , SCHED_FIFO  );
	guiSchedPolicyBox->addItem( tr("SCHED_RR")    , SCHED_RR    );

	grid->addWidget( new QLabel( tr("Policy") ), 0, 0 );
	grid->addWidget( guiSchedPolicyBox, 0, 1 );
	grid->addWidget( guiSchedPrioLabel, 1, 0 );
	grid->addWidget( guiSchedPrioSlider, 1, 1 );
	grid->addWidget( new QLabel( tr("Priority (Nice)") ), 2, 0 );
	grid->addWidget( guiSchedNiceSlider, 2, 1 );

	mainLayout->addWidget( guiPrioBox );

	setLayout( mainLayout );

	updatePolicyBox();
	updateSliderLimits();
	updateSliderValues();

	connect( emuSchedPolicyBox   , SIGNAL(activated(int))   , this, SLOT(emuSchedPolicyChange(int))   );
	connect( emuSchedNiceSlider  , SIGNAL(valueChanged(int)), this, SLOT(emuSchedNiceChange(int))     );
	connect( emuSchedPrioSlider  , SIGNAL(valueChanged(int)), this, SLOT(emuSchedPrioChange(int))     );
	connect( guiSchedPolicyBox   , SIGNAL(activated(int))   , this, SLOT(guiSchedPolicyChange(int))   );
	connect( guiSchedNiceSlider  , SIGNAL(valueChanged(int)), this, SLOT(guiSchedNiceChange(int))     );
	connect( guiSchedPrioSlider  , SIGNAL(valueChanged(int)), this, SLOT(guiSchedPrioChange(int))     );
}
//----------------------------------------------------------------------------
TimingConfDialog_t::~TimingConfDialog_t(void)
{
	printf("Destroy Timing Config Window\n");
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
void TimingConfDialog_t::emuSchedNiceChange(int val)
{
	if ( consoleWindow == NULL )
	{
		return;
	}
	fceuWrapperLock();
	if ( consoleWindow->emulatorThread->setNicePriority( -val ) )
	{
		char msg[1024];

		sprintf( msg, "Error: system call setPriority Failed\nReason: %s\n", strerror(errno) );
#ifdef __linux__
		strcat( msg, "Ensure that your system has the proper resource permissions set in the file:\n\n");
		strcat( msg, "        /etc/security/limits.conf \n\n");
		strcat( msg, "Adding the following lines to that file and rebooting will usually fix the issue:\n\n");
		strcat( msg, "*  -  priority   99 \n");
		strcat( msg, "*  -  rtprio     99 \n");
		strcat( msg, "*  -  nice      -20 \n");
#endif
		printf("%s\n", msg );
		consoleWindow->QueueErrorMsgWindow( msg );
		updateSliderValues();
	}
	fceuWrapperUnLock();
}
//----------------------------------------------------------------------------
void TimingConfDialog_t::emuSchedPrioChange(int val)
{
	int policy, prio;

	if ( consoleWindow == NULL )
	{
		return;
	}
	fceuWrapperLock();
	consoleWindow->emulatorThread->getSchedParam( policy, prio );

	if ( consoleWindow->emulatorThread->setSchedParam( policy, val ) )
	{
		char msg[1024];

		sprintf( msg, "Error: system call pthread_setschedparam Failed\nReason: %s\n", strerror(errno) );
#ifdef __linux__
		strcat( msg, "Ensure that your system has the proper resource permissions set in the file:\n\n");
		strcat( msg, "        /etc/security/limits.conf \n\n");
		strcat( msg, "Adding the following lines to that file and rebooting will usually fix the issue:\n\n");
		strcat( msg, "*  -  priority   99 \n");
		strcat( msg, "*  -  rtprio     99 \n");
		strcat( msg, "*  -  nice      -20 \n");
#endif
		printf("%s\n", msg );
		consoleWindow->QueueErrorMsgWindow( msg );
		updateSliderValues();
	}
	fceuWrapperUnLock();
}
//----------------------------------------------------------------------------
void TimingConfDialog_t::emuSchedPolicyChange( int index )
{
	int policy, prio;

	if ( consoleWindow == NULL )
	{
		return;
	}
	fceuWrapperLock();
	consoleWindow->emulatorThread->getSchedParam( policy, prio );

	policy = emuSchedPolicyBox->itemData( index ).toInt();

	if ( consoleWindow->emulatorThread->setSchedParam( policy, prio ) )
	{
		char msg[1024];

		sprintf( msg, "Error: system call pthread_setschedparam Failed\nReason: %s\n", strerror(errno) );
#ifdef __linux__
		strcat( msg, "Ensure that your system has the proper resource permissions set in the file:\n\n");
		strcat( msg, "        /etc/security/limits.conf \n\n");
		strcat( msg, "Adding the following lines to that file and rebooting will usually fix the issue:\n\n");
		strcat( msg, "*  -  priority   99 \n");
		strcat( msg, "*  -  rtprio     99 \n");
		strcat( msg, "*  -  nice      -20 \n");
#endif
		printf("%s\n", msg );
		consoleWindow->QueueErrorMsgWindow( msg );
	}

	updatePolicyBox();
	updateSliderLimits();
	updateSliderValues();
	fceuWrapperUnLock();
}
//----------------------------------------------------------------------------
void TimingConfDialog_t::guiSchedNiceChange(int val)
{
	if ( consoleWindow == NULL )
	{
		return;
	}
	fceuWrapperLock();
	if ( consoleWindow->setNicePriority( -val ) )
	{
		char msg[1024];

		sprintf( msg, "Error: system call setPriority Failed\nReason: %s\n", strerror(errno) );
#ifdef __linux__
		strcat( msg, "Ensure that your system has the proper resource permissions set in the file:\n\n");
		strcat( msg, "        /etc/security/limits.conf \n\n");
		strcat( msg, "Adding the following lines to that file and rebooting will usually fix the issue:\n\n");
		strcat( msg, "*  -  priority   99 \n");
		strcat( msg, "*  -  rtprio     99 \n");
		strcat( msg, "*  -  nice      -20 \n");
#endif
		printf("%s\n", msg );
		consoleWindow->QueueErrorMsgWindow( msg );
		updateSliderValues();
	}
	fceuWrapperUnLock();
}
//----------------------------------------------------------------------------
void TimingConfDialog_t::guiSchedPrioChange(int val)
{
	int policy, prio;

	if ( consoleWindow == NULL )
	{
		return;
	}
	fceuWrapperLock();
	consoleWindow->getSchedParam( policy, prio );

	if ( consoleWindow->setSchedParam( policy, val ) )
	{
		char msg[1024];

		sprintf( msg, "Error: system call pthread_setschedparam Failed\nReason: %s\n", strerror(errno) );
#ifdef __linux__
		strcat( msg, "Ensure that your system has the proper resource permissions set in the file:\n\n");
		strcat( msg, "        /etc/security/limits.conf \n\n");
		strcat( msg, "Adding the following lines to that file and rebooting will usually fix the issue:\n\n");
		strcat( msg, "*  -  priority   99 \n");
		strcat( msg, "*  -  rtprio     99 \n");
		strcat( msg, "*  -  nice      -20 \n");
#endif
		printf("%s\n", msg );
		consoleWindow->QueueErrorMsgWindow( msg );
		updateSliderValues();
	}
	fceuWrapperUnLock();
}
//----------------------------------------------------------------------------
void TimingConfDialog_t::guiSchedPolicyChange( int index )
{
	int policy, prio;

	if ( consoleWindow == NULL )
	{
		return;
	}
	fceuWrapperLock();
	consoleWindow->getSchedParam( policy, prio );

	policy = guiSchedPolicyBox->itemData( index ).toInt();

	if ( consoleWindow->setSchedParam( policy, prio ) )
	{
		char msg[1024];

		sprintf( msg, "Error: system call pthread_setschedparam Failed\nReason: %s\n", strerror(errno) );
#ifdef __linux__
		strcat( msg, "Ensure that your system has the proper resource permissions set in the file:\n\n");
		strcat( msg, "        /etc/security/limits.conf \n\n");
		strcat( msg, "Adding the following lines to that file and rebooting will usually fix the issue:\n\n");
		strcat( msg, "*  -  priority   99 \n");
		strcat( msg, "*  -  rtprio     99 \n");
		strcat( msg, "*  -  nice      -20 \n");
#endif
		printf("%s\n", msg );
		consoleWindow->QueueErrorMsgWindow( msg );
	}

	updatePolicyBox();
	updateSliderLimits();
	updateSliderValues();
	fceuWrapperUnLock();
}
//----------------------------------------------------------------------------
void TimingConfDialog_t::updatePolicyBox(void)
{
	int policy, prio;

	if ( consoleWindow == NULL )
	{
		return;
	}
	consoleWindow->emulatorThread->getSchedParam( policy, prio );

	for (int j=0; j<emuSchedPolicyBox->count(); j++)
	{
		if ( emuSchedPolicyBox->itemData(j).toInt() == policy )
		{
			//printf("Found Policy %i  %i\n",  j , policy );
			emuSchedPolicyBox->setCurrentIndex( j );
		}
	}

	consoleWindow->getSchedParam( policy, prio );

	for (int j=0; j<guiSchedPolicyBox->count(); j++)
	{
		if ( guiSchedPolicyBox->itemData(j).toInt() == policy )
		{
			//printf("Found Policy %i  %i\n",  j , policy );
			guiSchedPolicyBox->setCurrentIndex( j );
		}
	}

}
//----------------------------------------------------------------------------
void TimingConfDialog_t::updateSliderValues(void)
{
	int policy, prio;

	if ( consoleWindow == NULL )
	{
		return;
	}
	consoleWindow->emulatorThread->getSchedParam( policy, prio );

	emuSchedNiceSlider->setValue( -consoleWindow->emulatorThread->getNicePriority() );
	emuSchedPrioSlider->setValue(  prio );

	if ( (policy == SCHED_RR) || (policy == SCHED_FIFO) )
	{
		emuSchedPrioLabel->setEnabled(true);
		emuSchedPrioSlider->setEnabled(true);
	}
	else
	{
		emuSchedPrioLabel->setEnabled(false);
		emuSchedPrioSlider->setEnabled(false);
	}

	consoleWindow->getSchedParam( policy, prio );

	guiSchedNiceSlider->setValue( -consoleWindow->getNicePriority() );
	guiSchedPrioSlider->setValue(  prio );

	if ( (policy == SCHED_RR) || (policy == SCHED_FIFO) )
	{
		guiSchedPrioLabel->setEnabled(true);
		guiSchedPrioSlider->setEnabled(true);
	}
	else
	{
		guiSchedPrioLabel->setEnabled(false);
		guiSchedPrioSlider->setEnabled(false);
	}
}
//----------------------------------------------------------------------------
void TimingConfDialog_t::updateSliderLimits(void)
{
	if ( consoleWindow == NULL )
	{
		return;
	}

	emuSchedNiceSlider->setMinimum( -20 );
	emuSchedNiceSlider->setMaximum(  20 );
	emuSchedPrioSlider->setMinimum( consoleWindow->emulatorThread->getMinSchedPriority() );
	emuSchedPrioSlider->setMaximum( consoleWindow->emulatorThread->getMaxSchedPriority() );

	guiSchedNiceSlider->setMinimum( -20 );
	guiSchedNiceSlider->setMaximum(  20 );
	guiSchedPrioSlider->setMinimum( consoleWindow->getMinSchedPriority() );
	guiSchedPrioSlider->setMaximum( consoleWindow->getMaxSchedPriority() );

}
//----------------------------------------------------------------------------
