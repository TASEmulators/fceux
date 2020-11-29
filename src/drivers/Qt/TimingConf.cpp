// TimingConf.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <SDL.h>
#include <QHeaderView>
#include <QCloseEvent>

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
static bool hasNicePermissions( int val )
{
	int usrID;
	bool usrRoot;

	usrID = geteuid();

	usrRoot = (usrID == 0);

	if ( usrRoot )
	{
		return true;
	}
#ifdef __linux__
	struct rlimit r;

	if ( getrlimit( RLIMIT_NICE, &r ) == 0 )
	{
		int ncur = 20 - r.rlim_cur;

		if ( val >= ncur )
		{
			return true;
		}
		//printf("RLim Cur: %lu \n", r.rlim_cur );
		//printf("RLim Max: %lu \n", r.rlim_max );
	}
#endif

	return false;
}
//----------------------------------------------------------------------------
TimingConfDialog_t::TimingConfDialog_t(QWidget *parent)
	: QDialog( parent )
{
	int opt;
	QVBoxLayout *mainLayout;
	QHBoxLayout *hbox;
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
	emuSchedNiceLabel  = new QLabel( tr("Priority (Nice)") );

	emuSchedPolicyBox->addItem( tr("SCHED_OTHER") , SCHED_OTHER );
	emuSchedPolicyBox->addItem( tr("SCHED_FIFO")  , SCHED_FIFO  );
	emuSchedPolicyBox->addItem( tr("SCHED_RR")    , SCHED_RR    );

	grid->addWidget( new QLabel( tr("Policy") ), 0, 0 );
	grid->addWidget( emuSchedPolicyBox, 0, 1 );
	grid->addWidget( emuSchedPrioLabel, 1, 0 );
	grid->addWidget( emuSchedPrioSlider, 1, 1 );
	grid->addWidget( emuSchedNiceLabel, 2, 0 );
	grid->addWidget( emuSchedNiceSlider, 2, 1 );

	mainLayout->addWidget( emuPrioCtlEna );
	mainLayout->addWidget( emuPrioBox );

	grid       = new QGridLayout();
	guiPrioBox->setLayout( grid );

	guiSchedPolicyBox  = new QComboBox();
	guiSchedPrioSlider = new QSlider( Qt::Horizontal );
	guiSchedNiceSlider = new QSlider( Qt::Horizontal );
	guiSchedPrioLabel  = new QLabel( tr("Priority (RT)") );
	guiSchedNiceLabel  = new QLabel( tr("Priority (Nice)") );

	guiSchedPolicyBox->addItem( tr("SCHED_OTHER") , SCHED_OTHER );
	guiSchedPolicyBox->addItem( tr("SCHED_FIFO")  , SCHED_FIFO  );
	guiSchedPolicyBox->addItem( tr("SCHED_RR")    , SCHED_RR    );

	grid->addWidget( new QLabel( tr("Policy") ), 0, 0 );
	grid->addWidget( guiSchedPolicyBox, 0, 1 );
	grid->addWidget( guiSchedPrioLabel, 1, 0 );
	grid->addWidget( guiSchedPrioSlider, 1, 1 );
	grid->addWidget( guiSchedNiceLabel, 2, 0 );
	grid->addWidget( guiSchedNiceSlider, 2, 1 );

	mainLayout->addWidget( guiPrioBox );

	hbox = new QHBoxLayout();
	timingDevSelBox = new QComboBox();
	timingDevSelBox->addItem( tr("NanoSleep") , 0 );

#ifdef __linux__
	timingDevSelBox->addItem( tr("Timer FD")  , 1 );
#endif
	hbox->addWidget( new QLabel( tr("Timing Mechanism:") ) );
	hbox->addWidget( timingDevSelBox );
	mainLayout->addLayout( hbox );

	setLayout( mainLayout );

	g_config->getOption( "SDL.SetSchedParam", &opt );
	
	emuPrioCtlEna->setChecked( opt );

	updatePolicyBox();
	updateSliderLimits();
	updateSliderValues();
	updateTimingMech();

	connect( emuSchedPolicyBox   , SIGNAL(activated(int))   , this, SLOT(emuSchedPolicyChange(int))   );
	connect( emuSchedNiceSlider  , SIGNAL(valueChanged(int)), this, SLOT(emuSchedNiceChange(int))     );
	connect( emuSchedPrioSlider  , SIGNAL(valueChanged(int)), this, SLOT(emuSchedPrioChange(int))     );
	connect( guiSchedPolicyBox   , SIGNAL(activated(int))   , this, SLOT(guiSchedPolicyChange(int))   );
	connect( guiSchedNiceSlider  , SIGNAL(valueChanged(int)), this, SLOT(guiSchedNiceChange(int))     );
	connect( guiSchedPrioSlider  , SIGNAL(valueChanged(int)), this, SLOT(guiSchedPrioChange(int))     );
	connect( emuPrioCtlEna       , SIGNAL(stateChanged(int)), this, SLOT(emuSchedCtlChange(int))      );
	connect( timingDevSelBox     , SIGNAL(activated(int))   , this, SLOT(emuTimingMechChange(int))    );
}
//----------------------------------------------------------------------------
TimingConfDialog_t::~TimingConfDialog_t(void)
{
	printf("Destroy Timing Config Window\n");
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
void TimingConfDialog_t::emuSchedCtlChange( int state )
{
	g_config->setOption( "SDL.SetSchedParam", (state != Qt::Unchecked) );
}
//----------------------------------------------------------------------------
void TimingConfDialog_t::saveValues(void)
{
	int policy, prio, nice;

	if ( consoleWindow == NULL )
	{
		return;
	}
	nice = consoleWindow->emulatorThread->getNicePriority();

	consoleWindow->emulatorThread->getSchedParam( policy, prio );

	g_config->setOption( "SDL.EmuSchedPolicy", policy );
	g_config->setOption( "SDL.EmuSchedPrioRt", prio   );
	g_config->setOption( "SDL.EmuSchedNice"  , nice   );

	//printf("EMU Sched: %i  %i  %i\n", policy, prio, nice );

	nice = consoleWindow->getNicePriority();

	consoleWindow->getSchedParam( policy, prio );

	g_config->setOption( "SDL.GuiSchedPolicy", policy );
	g_config->setOption( "SDL.GuiSchedPrioRt", prio   );
	g_config->setOption( "SDL.GuiSchedNice"  , nice   );

	//printf("GUI Sched: %i  %i  %i\n", policy, prio, nice );

	g_config->save();
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
	bool hasNicePerms;

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
	hasNicePerms = hasNicePermissions( consoleWindow->emulatorThread->getNicePriority() );

	emuSchedNiceLabel->setEnabled( hasNicePerms );
	emuSchedNiceSlider->setEnabled( hasNicePerms );

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
	hasNicePerms = hasNicePermissions( consoleWindow->getNicePriority() );

	guiSchedNiceLabel->setEnabled( hasNicePerms );
	guiSchedNiceSlider->setEnabled( hasNicePerms );

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
void TimingConfDialog_t::emuTimingMechChange( int index )
{
	int mode;

	if ( consoleWindow == NULL )
	{
		return;
	}
	fceuWrapperLock();

	mode = timingDevSelBox->itemData( index ).toInt();

	setTimingMode( mode );

	RefreshThrottleFPS();

	g_config->setOption("SDL.EmuTimingMech", mode);

	fceuWrapperUnLock();
}
//----------------------------------------------------------------------------
void TimingConfDialog_t::updateTimingMech(void)
{
	int mode = getTimingMode();

	for (int j=0; j<timingDevSelBox->count(); j++)
	{
		if ( timingDevSelBox->itemData(j).toInt() == mode )
		{
			timingDevSelBox->setCurrentIndex( j );
		}
	}
}
//----------------------------------------------------------------------------
