// CodeDataLogger.cpp
//
#include <QDir>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>

#include "../../types.h"
#include "../../fceu.h"
#include "../../cart.h"
#include "../../x6502.h"
#include "../../debug.h"
#include "../../ppu.h"
#include "../../ines.h"
#include "../../nsf.h"

#include "Qt/ConsoleUtilities.h"
#include "Qt/TraceLogger.h"
#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/fceuWrapper.h"

//----------------------------------------------------
TraceLoggerDialog_t::TraceLoggerDialog_t(QWidget *parent)
	: QDialog( parent )
{
	QVBoxLayout *mainLayout;
	QHBoxLayout *hbox;
	QGridLayout *grid;
	QGroupBox *frame;
	QLabel    *lbl;

   setWindowTitle( tr("Trace Logger") );

	mainLayout   = new QVBoxLayout();
	grid         = new QGridLayout();

	mainLayout->addLayout( grid  );

	lbl = new QLabel( tr("Lines") );
	logLastCbox  = new QCheckBox( tr("Log Last") );
	logMaxLinesComboBox = new QComboBox();

	logFileCbox  = new QCheckBox( tr("Log to File") );
	selLogFileButton = new QPushButton( tr("Browse...") );
	startStopButton = new QPushButton( tr("Start Logging") );
	autoUpdateCbox = new QCheckBox( tr("Automatically update this window while logging") );

	hbox = new QHBoxLayout();
	hbox->addWidget( logLastCbox );
	hbox->addWidget( logMaxLinesComboBox );
	hbox->addWidget( lbl );

	grid->addLayout( hbox, 0, 0, Qt::AlignLeft );
	grid->addWidget( startStopButton, 0, 1, Qt::AlignCenter );

	hbox = new QHBoxLayout();
	hbox->addWidget( logFileCbox );
	hbox->addWidget( selLogFileButton );

	grid->addLayout( hbox, 1, 0, Qt::AlignLeft );
	grid->addWidget( autoUpdateCbox, 1, 1, Qt::AlignCenter );

	grid  = new QGridLayout();
	frame = new QGroupBox(tr("Log Options"));
	frame->setLayout( grid );

	logRegCbox            = new QCheckBox( tr("Log State of Registers") );
	logFrameCbox          = new QCheckBox( tr("Log Frames Count") );
	logEmuMsgCbox         = new QCheckBox( tr("Log Emulator Messages") );
	symTraceEnaCbox       = new QCheckBox( tr("Symbolic Trace") );
	logProcStatFlagCbox   = new QCheckBox( tr("Log Processor Status Flags") );
	logCyclesCountCbox    = new QCheckBox( tr("Log Cycles Count") );
	logBreakpointCbox     = new QCheckBox( tr("Log Breakpoint Hits") );
	useStackPointerCbox   = new QCheckBox( tr("Use Stack Pointer for Code Tabbing (Nesting Visualization)") );
	toLeftDisassemblyCbox = new QCheckBox( tr("To the Left from Disassembly") );
	logInstrCountCbox     = new QCheckBox( tr("Log Instructions Count") );
	logBankNumCbox        = new QCheckBox( tr("Log Bank Number") );

	grid->addWidget( logRegCbox     , 0, 0, Qt::AlignLeft );
	grid->addWidget( logFrameCbox   , 1, 0, Qt::AlignLeft );
	grid->addWidget( logEmuMsgCbox  , 2, 0, Qt::AlignLeft );
	grid->addWidget( symTraceEnaCbox, 3, 0, Qt::AlignLeft );
	grid->addWidget( logProcStatFlagCbox, 0, 1, Qt::AlignLeft );
	grid->addWidget( logCyclesCountCbox , 1, 1, Qt::AlignLeft );
	grid->addWidget( logBreakpointCbox  , 2, 1, Qt::AlignLeft );
	grid->addWidget( useStackPointerCbox, 3, 1, 1, 2, Qt::AlignLeft );
	grid->addWidget( toLeftDisassemblyCbox, 0, 2, Qt::AlignLeft );
	grid->addWidget( logInstrCountCbox    , 1, 2, Qt::AlignLeft );
	grid->addWidget( logBankNumCbox       , 2, 2, Qt::AlignLeft );

	mainLayout->addWidget( frame );

	grid  = new QGridLayout();
	frame = new QGroupBox(tr("Extra Log Options that work with the Code/Data Logger"));
	frame->setLayout( grid );

	logNewMapCodeCbox = new QCheckBox( tr("Only Log Newly Mapped Code") );
	logNewMapDataCbox = new QCheckBox( tr("Only Log that Accesses Newly Mapped Data") );

	grid->addWidget( logNewMapCodeCbox, 0, 0, Qt::AlignLeft );
	grid->addWidget( logNewMapDataCbox, 0, 1, Qt::AlignLeft );

	mainLayout->addWidget( frame );

	setLayout( mainLayout );

}
//----------------------------------------------------
TraceLoggerDialog_t::~TraceLoggerDialog_t(void)
{
	printf("Trace Logger Window Deleted\n");
}
//----------------------------------------------------
void TraceLoggerDialog_t::closeEvent(QCloseEvent *event)
{
   printf("Trace Logger Close Window Event\n");
   done(0);
	deleteLater();
   event->accept();
}
//----------------------------------------------------
void TraceLoggerDialog_t::closeWindow(void)
{
   printf("Trace Logger Close Window\n");
   done(0);
	deleteLater();
}
//----------------------------------------------------
