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
// ConsoleDebugger.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <list>

#include <SDL.h>
#include <QMenu>
#include <QAction>
#include <QPainter>
#include <QSpinBox>
#include <QHeaderView>
#include <QCloseEvent>
#include <QGridLayout>
#include <QRadioButton>
#include <QInputDialog>
#include <QFontDialog>
#include <QMessageBox>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QGuiApplication>
#include <QSettings>
#include <QToolTip>
#include <QWindow>
#include <QScreen>
#include <QMimeData>
#include <QDrag>

#include "../../types.h"
#include "../../fceu.h"
#include "../../cheat.h"
#include "../../debug.h"
#include "../../driver.h"
#include "../../version.h"
#include "../../video.h"
#include "../../movie.h"
#include "../../palette.h"
#include "../../fds.h"
#include "../../cart.h"
#include "../../ines.h"
#include "../../asm.h"
#include "../../ppu.h"
#include "../../x6502.h"
#include "common/os_utils.h"
#include "common/configSys.h"

#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/config.h"
#include "Qt/nes_shm.h"
#include "Qt/fceuWrapper.h"
#include "Qt/HexEditor.h"
#include "Qt/ConsoleDebugger.h"
#include "Qt/ConsoleUtilities.h"
#include "Qt/ColorMenu.h"

// Where are these defined?
extern int vblankScanLines;
extern int vblankPixel;

debuggerBookmarkManager_t dbgBmMgr;
static ConsoleDebugger* dbgWin = NULL;

static void DeleteBreak(int sel);
static bool waitingAtBp = false;
static bool bpDebugEnable = true;
static int  lastBpIdx   = 0;
static bool breakOnCycleOneShot = 0;
static bool breakOnInstrOneShot = 0;
static int  breakOnCycleMode    = 1;
static int  breakOnInstrMode    = 1;
static unsigned long long int  breakOnCycleRelVal = 0;
static unsigned long long int  breakOnInstrRelVal = 0;
//----------------------------------------------------------------------------
ConsoleDebugger::ConsoleDebugger(QWidget *parent)
	: QDialog( parent, Qt::Window )
{
	QMenuBar    *menuBar;
	QSettings settings;
	std::string fontString;

	g_config->getOption("SDL.DebuggerCpuStatusFont", &fontString);

	if ( fontString.size() > 0 )
	{
		cpuFont.fromString( QString::fromStdString( fontString ) );
	}
	else
	{
		cpuFont.setFamily("Courier New");
		cpuFont.setStyle( QFont::StyleNormal );
		cpuFont.setStyleHint( QFont::Monospace );
	}

	font.setFamily("Courier New");
	font.setStyle( QFont::StyleNormal );
	font.setStyleHint( QFont::Monospace );
	QFontMetrics fm(font);

	setWindowTitle("6502 Debugger");

	menuBar = buildMenuBar();
	toolBar = buildToolBar();

	mainLayoutv = new QVBoxLayout();
	mainLayouth = new QSplitter( Qt::Horizontal );
	mainLayouth->setOpaqueResize(true);

	mainLayoutv->setMenuBar( menuBar );
	mainLayoutv->addWidget( toolBar );
	mainLayoutv->addWidget( mainLayouth );


	for (int i=0; i<2; i++)
	{
		vsplitter[i] = new QSplitter( Qt::Vertical );

		for (int j=0; j<4; j++)
		{
			char stmp[64];

			tabView[i][j] = new DebuggerTabWidget(i,j);

			sprintf( stmp, "debuggerTabView%i%i\n", i+1, j+1 );

			tabView[i][j]->setObjectName( tr(stmp) );

			vsplitter[i]->addWidget( tabView[i][j] );
		}
	}

	buildAsmViewDisplay();
	buildCpuListDisplay();
	buildPpuListDisplay();
	buildBpListDisplay();
	buildBmListDisplay();

	mainLayouth->addWidget(  asmViewContainerWidget );
	mainLayouth->addWidget( vsplitter[0] );
	mainLayouth->addWidget( vsplitter[1] );

	setLayout( mainLayoutv );

	loadDisplayViews();

	windowUpdateReq   = true;

	dbgWin = this;

	periodicTimer  = new QTimer( this );

	connect( periodicTimer, &QTimer::timeout, this, &ConsoleDebugger::updatePeriodic );
	connect( hbar, SIGNAL(valueChanged(int)), this, SLOT(hbarChanged(int)) );
	connect( vbar, SIGNAL(valueChanged(int)), this, SLOT(vbarChanged(int)) );

	bpListUpdate( false );

	periodicTimer->start( 100 ); // 10hz

	// If this is the first debug window to open, load breakpoints fresh
	{ 
		int autoLoadDebug;

		g_config->getOption( "SDL.AutoLoadDebugFiles", &autoLoadDebug );

		if ( autoLoadDebug )
		{
			loadGameDebugBreakpoints();
		}
	}

	//restoreGeometry(settings.value("debugger/geometry").toByteArray());

	setCpuStatusFont( cpuFont );

	   opcodeColorAct->connectColor( &asmView->opcodeColor );
	  addressColorAct->connectColor( &asmView->addressColor );
	immediateColorAct->connectColor( &asmView->immediateColor );
	    labelColorAct->connectColor( &asmView->labelColor  );
	  commentColorAct->connectColor( &asmView->commentColor);
	       pcColorAct->connectColor( &asmView->pcBgColor);

	connect( this, SIGNAL(rejected(void)), this, SLOT(deleteLater(void)));
}
//----------------------------------------------------------------------------
ConsoleDebugger::~ConsoleDebugger(void)
{
	std::list <ConsoleDebugger*>::iterator it;

	//printf("Destroy Debugger Window\n");
	periodicTimer->stop();

	saveDisplayViews();

	if ( dbgWin == this )
	{
		dbgWin = NULL;
	}

	if ( dbgWin == NULL )
	{
		saveGameDebugBreakpoints();
		debuggerClearAllBreakpoints();
		debuggerClearAllBookmarks();

		if ( waitingAtBp )
		{
			FCEUI_SetEmulationPaused(0);
		}

		break_on_cycles        = false;
		break_on_instructions  = false;
		break_on_unlogged_code = false;
		break_on_unlogged_data = false;
		FCEUI_Debugger().badopbreak = false;
	}
}
//----------------------------------------------------------------------------
void ConsoleDebugger::closeEvent(QCloseEvent *event)
{
	//printf("Debugger Close Window Event\n");
	saveDisplayViews();
	done(0);
	deleteLater();
	event->accept();
}
//----------------------------------------------------------------------------
void ConsoleDebugger::closeWindow(void)
{
	saveDisplayViews();
	//printf("Close Window\n");
	done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
QMenuBar *ConsoleDebugger::buildMenuBar(void)
{
	QMenu       *fileMenu, *viewMenu, *debugMenu,
		    *optMenu, *symMenu, *subMenu, *visMenu;
	QActionGroup *actGroup;
	QAction     *act;
	int opt, useNativeMenuBar=0;

	QMenuBar *menuBar = new QMenuBar(this);

	// This is needed for menu bar to show up on MacOS
	g_config->getOption( "SDL.UseNativeMenuBar", &useNativeMenuBar );

	menuBar->setNativeMenuBar( useNativeMenuBar ? true : false );

	//-----------------------------------------------------------------------
	// Menu Start
	//-----------------------------------------------------------------------
	// File
	fileMenu = menuBar->addMenu(tr("&File"));

	// File -> Close
	act = new QAction(tr("&Close"), this);
	act->setShortcut(QKeySequence::Close);
	act->setStatusTip(tr("Close Window"));
	connect(act, SIGNAL(triggered()), this, SLOT(closeWindow(void)) );

	fileMenu->addAction(act);

	// View
	viewMenu = menuBar->addMenu(tr("&View"));

	// View -> Go to Address
	act = new QAction(tr("&Go to Address"), this);
	act->setShortcut( QKeySequence(tr("Ctrl+A") ));
	act->setStatusTip(tr("&Go to Address"));
	//act->setIcon( QIcon(":icons/find.png") );
	act->setIcon( QIcon(":icons/JumpTarget.png") );
	connect(act, SIGNAL(triggered()), this, SLOT(openGotoAddrDialog(void)) );

	viewMenu->addAction(act);

	// View -> Go to PC
	act = new QAction(tr("Go to &PC"), this);
	act->setShortcut( QKeySequence(tr("Ctrl+G") ));
	act->setStatusTip(tr("Go to &PC"));
	//act->setIcon( QIcon(":icons/JumpTarget.png") );
	connect(act, SIGNAL(triggered()), this, SLOT(seekPCCB(void)) );

	viewMenu->addAction(act);

	// View -> Change PC
	act = new QAction(tr("&Change PC"), this);
	act->setShortcut( QKeySequence(tr("Ctrl+Shift+G") ));
	act->setStatusTip(tr("&Change PC"));
	//act->setIcon( QIcon(":icons/JumpTarget.png") );
	connect(act, SIGNAL(triggered()), this, SLOT(openChangePcDialog(void)) );

	viewMenu->addAction(act);

	viewMenu->addSeparator();

	// View -> Navigate Back
	act = new QAction(tr("Navigate &Back"), this);
	act->setShortcut( QKeySequence(tr("Ctrl+Left") ));
	act->setStatusTip(tr("Navigate Back"));
	//act->setIcon( QIcon(":icons/JumpTarget.png") );
	connect(act, SIGNAL(triggered()), this, SLOT(navHistBackCB(void)) );

	viewMenu->addAction(act);

	// View -> Navigate Forward
	act = new QAction(tr("Navigate &Forward"), this);
	act->setShortcut( QKeySequence(tr("Ctrl+Right") ));
	act->setStatusTip(tr("Navigate Forward"));
	//act->setIcon( QIcon(":icons/JumpTarget.png") );
	connect(act, SIGNAL(triggered()), this, SLOT(navHistForwardCB(void)) );

	viewMenu->addAction(act);

	viewMenu->addSeparator();

	// View -> Layout
	visMenu = viewMenu->addMenu( tr("&Layout Presets") );

	g_config->getOption( "SDL.DebuggerLayoutOpt", &opt );

	actGroup = new QActionGroup(this);
	actGroup->setExclusive(true);

	// View -> Layout -> Compact
	act = new QAction(tr("&Compact"), this);
	//act->setStatusTip(tr("Compact"));
	connect( act, &QAction::triggered, [this]{ setLayoutOption(1); } );
	actGroup->addAction(act);
	visMenu->addAction(act);

	// View -> Layout -> Compact Split
	act = new QAction(tr("Compact &Split"), this);
	//act->setStatusTip(tr("1 Tabbed Vertical Column with 2 Sections"));
	connect( act, &QAction::triggered, [this]{ setLayoutOption(2); } );
	actGroup->addAction(act);
	visMenu->addAction(act);

	// View -> Layout -> Wide
	act = new QAction(tr("&Wide"), this);
	//act->setStatusTip(tr("2 Tabbed Vertical Columns with 3 Sections"));
	connect( act, &QAction::triggered, [this]{ setLayoutOption(3); } );
	actGroup->addAction(act);
	visMenu->addAction(act);

	// View -> Layout -> Wide Quad
	act = new QAction(tr("Wide &Quad"), this);
	//act->setStatusTip(tr("2 Tabbed Vertical Columns with 4 Sections"));
	connect( act, &QAction::triggered, [this]{ setLayoutOption(4); } );
	actGroup->addAction(act);
	visMenu->addAction(act);

	// View -> Font Selection
	subMenu  = viewMenu->addMenu(tr("&Font Selection"));

	// Options -> Font Selection -> Assembly View
	act = new QAction(tr("&Assembly View"), this);
	//act->setShortcut(QKeySequence( tr("F7") ) );
	act->setStatusTip(tr("Set Assembly View Font"));
	connect( act, SIGNAL(triggered(void)), this, SLOT(changeAsmFontCB(void)) );

	subMenu->addAction(act);

	// View -> Font Selection -> Stack View
	act = new QAction(tr("&Stack View"), this);
	//act->setShortcut(QKeySequence( tr("F7") ) );
	act->setStatusTip(tr("Set Stack View Font"));
	connect( act, SIGNAL(triggered(void)), this, SLOT(changeStackFontCB(void)) );

	subMenu->addAction(act);

	// View -> Font Selection -> CPU Data View
	act = new QAction(tr("&CPU Data View"), this);
	//act->setShortcut(QKeySequence( tr("F7") ) );
	act->setStatusTip(tr("Set CPU View Font"));
	connect( act, SIGNAL(triggered(void)), this, SLOT(changeCpuFontCB(void)) );

	subMenu->addAction(act);

	// View -> Color Selection
	subMenu  = viewMenu->addMenu(tr("&Color Selection"));

	// View -> Color Selection -> Opcodes
	opcodeColorAct = new ColorMenuItem( tr("&Opcodes"), "SDL.AsmSyntaxColorOpcode", this);

	subMenu->addAction(opcodeColorAct);

	// View -> Color Selection -> Address Values
	addressColorAct = new ColorMenuItem( tr("&Address Values"), "SDL.AsmSyntaxColorAddress", this);

	subMenu->addAction(addressColorAct);

	// View -> Color Selection -> Immediate Values
	immediateColorAct = new ColorMenuItem( tr("&Immediate Values"), "SDL.AsmSyntaxColorImmediate", this);

	subMenu->addAction(immediateColorAct);

	// View -> Color Selection -> Labels
	labelColorAct = new ColorMenuItem( tr("&Labels"), "SDL.AsmSyntaxColorLabel", this);

	subMenu->addAction(labelColorAct);

	// View -> Color Selection -> Comments
	commentColorAct = new ColorMenuItem( tr("&Comments"), "SDL.AsmSyntaxColorComment", this);

	subMenu->addAction(commentColorAct);

	subMenu->addSeparator();

	// View -> Color Selection -> (PC) Active Statement
	pcColorAct = new ColorMenuItem( tr("(&PC) Active Statement BG"), "SDL.AsmSyntaxColorPC", this);

	subMenu->addAction(pcColorAct);

	viewMenu->addSeparator();

	// View -> PC Position
	subMenu  = viewMenu->addMenu(tr("&PC Line Positioning"));
	actGroup = new QActionGroup(this);

	actGroup->setExclusive(true);

	g_config->getOption( "SDL.DebuggerPCPlacement", &opt );

	// View -> PC Position -> Top Line
	act = new QAction(tr("&Top Line"), this);
	act->setStatusTip(tr("Top Line"));
	act->setCheckable(true);
	act->setChecked( opt == 0 );
	connect( act, SIGNAL(triggered()), this, SLOT(pcSetPlaceTop(void)) );
	actGroup->addAction(act);
	subMenu->addAction(act);

	// View -> PC Position -> Upper Mid-Line
	act = new QAction(tr("&Upper Mid-Line"), this);
	act->setStatusTip(tr("Upper Mid-Line"));
	act->setCheckable(true);
	act->setChecked( opt == 1 );
	connect( act, SIGNAL(triggered()), this, SLOT(pcSetPlaceUpperMid(void)) );
	actGroup->addAction(act);
	subMenu->addAction(act);

	// View -> PC Position -> Center Line
	act = new QAction(tr("&Center Line"), this);
	act->setStatusTip(tr("Center Line"));
	act->setCheckable(true);
	act->setChecked( opt == 2 );
	connect( act, SIGNAL(triggered()), this, SLOT(pcSetPlaceCenter(void)) );
	actGroup->addAction(act);
	subMenu->addAction(act);

	// View -> PC Position -> Lower Mid-Line
	act = new QAction(tr("&Lower Mid-Line"), this);
	act->setStatusTip(tr("Lower Mid-Line"));
	act->setCheckable(true);
	act->setChecked( opt == 3 );
	connect( act, SIGNAL(triggered()), this, SLOT(pcSetPlaceLowerMid(void)) );
	actGroup->addAction(act);
	subMenu->addAction(act);

	// View -> PC Position -> Bottom
	act = new QAction(tr("&Bottom Line"), this);
	act->setStatusTip(tr("Bottom Line"));
	act->setCheckable(true);
	act->setChecked( opt == 4 );
	connect( act, SIGNAL(triggered()), this, SLOT(pcSetPlaceBottom(void)) );
	actGroup->addAction(act);
	subMenu->addAction(act);

	// View -> PC Position -> Custom Line 
	act = new QAction(tr("Custom Line &Offset"), this);
	act->setStatusTip(tr("Custom Line Offset"));
	act->setChecked( opt == 5 );
	act->setCheckable(true);
	connect( act, SIGNAL(triggered()), this, SLOT(pcSetPlaceCustom(void)) );
	actGroup->addAction(act);
	subMenu->addAction(act);

	// View -> Show Byte Codes
	g_config->getOption( "SDL.AsmShowByteCodes", &opt );

	act = new QAction(tr("Show &Byte Codes"), this);
	//act->setShortcut(QKeySequence( tr("F7") ) );
	act->setStatusTip(tr("Show &Byte Codes"));
	act->setCheckable(true);
	act->setChecked(opt);
	connect( act, SIGNAL(triggered(bool)), this, SLOT(displayByteCodesCB(bool)) );

	viewMenu->addAction(act);

	// View -> Display ROM Offsets
	g_config->getOption( "SDL.AsmShowRomOffsets", &opt );

	act = new QAction(tr("Show ROM &Offsets"), this);
	//act->setShortcut(QKeySequence( tr("F7") ) );
	act->setStatusTip(tr("Show ROM &Offsets"));
	act->setCheckable(true);
	act->setChecked(opt);
	connect( act, SIGNAL(triggered(bool)), this, SLOT(displayROMoffsetCB(bool)) );

	viewMenu->addAction(act);

	// Debug
	debugMenu = menuBar->addMenu(tr("&Debug"));

	// Debug -> Run
	act = new QAction(tr("&Run"), this);
	act->setShortcut(QKeySequence( tr("F5") ) );
	act->setStatusTip(tr("Run"));
	//act->setIcon( style()->standardIcon( QStyle::SP_MediaPlay ) );
	act->setIcon( QIcon(":icons/debug-run.png") );
	connect( act, SIGNAL(triggered()), this, SLOT(debugRunCB(void)) );

	debugMenu->addAction(act);

	// Debug -> Step Into
	act = new QAction(tr("Step &Into"), this);
	act->setShortcut(QKeySequence( tr("F11") ) );
	act->setStatusTip(tr("Step Into"));
	act->setIcon( QIcon(":icons/StepInto.png") );
	connect( act, SIGNAL(triggered()), this, SLOT(debugStepIntoCB(void)) );

	debugMenu->addAction(act);

	// Debug -> Step Out
	act = new QAction(tr("&Step Out"), this);
	act->setShortcut(QKeySequence( tr("Shift+F11") ) );
	act->setStatusTip(tr("Step Out"));
	act->setIcon( QIcon(":icons/StepOut.png") );
	connect( act, SIGNAL(triggered()), this, SLOT(debugStepOutCB(void)) );

	debugMenu->addAction(act);

	// Debug -> Step Over
	act = new QAction(tr("Step &Over"), this);
	act->setShortcut(QKeySequence( tr("F10") ) );
	act->setStatusTip(tr("Step Over"));
	act->setIcon( QIcon(":icons/StepOver.png") );
	connect( act, SIGNAL(triggered()), this, SLOT(debugStepOverCB(void)) );

	debugMenu->addAction(act);

	// Debug -> Run to Selected Line
	act = new QAction(tr("Run to S&elected Line"), this);
	act->setShortcut(QKeySequence( tr("F1") ) );
	act->setStatusTip(tr("Run to Selected Line"));
	act->setIcon( QIcon(":icons/arrow-cursor.png") );
	connect( act, SIGNAL(triggered()), this, SLOT(debugRunToCursorCB(void)) );

	debugMenu->addAction(act);

	// Debug -> Run Line
	act = new QAction(tr("Run &Line"), this);
	act->setShortcut(QKeySequence( tr("F6") ) );
	act->setStatusTip(tr("Run Line"));
	act->setIcon( QIcon(":icons/RunPpuScanline.png") );
	connect( act, SIGNAL(triggered()), this, SLOT(debugRunLineCB(void)) );

	debugMenu->addAction(act);

	// Debug -> Run 128 Lines
	act = new QAction(tr("Run &128 Lines"), this);
	act->setShortcut(QKeySequence( tr("F7") ) );
	act->setStatusTip(tr("Run 128 Lines"));
	act->setIcon( QIcon(":icons/RunPpuFrame.png") );
	connect( act, SIGNAL(triggered()), this, SLOT(debugRunLine128CB(void)) );

	debugMenu->addAction(act);

	debugMenu->addSeparator();

	subMenu = debugMenu->addMenu(tr("&Break On..."));

	// Debug -> Break on -> Bad Opcodes
	g_config->getOption("SDL.DebuggerBreakOnBadOpcodes", &FCEUI_Debugger().badopbreak );

	act = new QAction(tr("Bad &Opcodes"), this);
	//act->setShortcut(QKeySequence( tr("F7") ) );
	act->setStatusTip(tr("Bad Opcodes"));
	act->setCheckable(true);
	act->setChecked( FCEUI_Debugger().badopbreak );
	connect( act, SIGNAL(triggered(bool)), this, SLOT(breakOnBadOpcodeCB(bool)) );

	subMenu->addAction(act);

	// Debug -> Break on -> Unlogged Code
	g_config->getOption("SDL.DebuggerBreakOnUnloggedCode", &break_on_unlogged_code );

	act = new QAction(tr("Unlogged &Code"), this);
	//act->setShortcut(QKeySequence( tr("F7") ) );
	act->setStatusTip(tr("Unlogged Code"));
	act->setCheckable(true);
	act->setChecked( break_on_unlogged_code );
	connect( act, SIGNAL(triggered(bool)), this, SLOT(breakOnNewCodeCB(bool)) );

	subMenu->addAction(act);

	// Debug -> Break on -> Unlogged Data
	g_config->getOption("SDL.DebuggerBreakOnUnloggedData", &break_on_unlogged_data );

	act = new QAction(tr("Unlogged &Data"), this);
	//act->setShortcut(QKeySequence( tr("F7") ) );
	act->setStatusTip(tr("Unlogged Data"));
	act->setCheckable(true);
	act->setChecked( break_on_unlogged_data );
	connect( act, SIGNAL(triggered(bool)), this, SLOT(breakOnNewDataCB(bool)) );

	subMenu->addAction(act);

	// Debug -> Break on -> Cycle Count Exceeded
	brkOnCycleExcAct = act = new QAction(tr("C&ycle Count Exceeded"), this);
	//act->setShortcut(QKeySequence( tr("F7") ) );
	act->setStatusTip(tr("CPU Cycle Count Exceeded"));
	act->setCheckable(true);
	act->setChecked( break_on_cycles );
	connect( act, SIGNAL(triggered(bool)), this, SLOT(breakOnCyclesCB(bool)) );

	subMenu->addAction(act);

	// Debug -> Break on -> Instruction Count Exceeded
	brkOnInstrExcAct = act = new QAction(tr("&Instruction Count Exceeded"), this);
	//act->setShortcut(QKeySequence( tr("F7") ) );
	act->setStatusTip(tr("CPU Instruction Count Exceeded"));
	act->setCheckable(true);
	act->setChecked( break_on_instructions );
	connect( act, SIGNAL(triggered(bool)), this, SLOT(breakOnInstructionsCB(bool)) );

	subMenu->addAction(act);

	debugMenu->addSeparator();

	// Debug -> Reset Counters
	act = new QAction(tr("Reset &Counters"), this);
	//act->setShortcut(QKeySequence( tr("F7") ) );
	act->setStatusTip(tr("Reset Counters"));
	act->setCheckable(false);
	act->setIcon( style()->standardIcon( QStyle::SP_BrowserReload ) );
	connect( act, SIGNAL(triggered(void)), this, SLOT(resetCountersCB(void)) );

	debugMenu->addAction(act);

	// Options
	optMenu = menuBar->addMenu(tr("&Options"));

	// Options -> Open Debugger on ROM Load
	g_config->getOption( "SDL.AutoOpenDebugger", &opt );

	act = new QAction(tr("&Open Debugger on ROM Load"), this);
	//act->setShortcut(QKeySequence( tr("F7") ) );
	act->setStatusTip(tr("&Reload"));
	act->setCheckable(true);
	act->setChecked( opt ? true : false );
	connect( act, SIGNAL(triggered(bool)), this, SLOT(autoOpenDebugCB(bool)) );

	optMenu->addAction(act);

	// Options -> Load .DEB
	g_config->getOption( "SDL.AutoLoadDebugFiles", &opt );

	act = new QAction(tr("&Load .DEB on ROM Load"), this);
	//act->setShortcut(QKeySequence( tr("F7") ) );
	act->setStatusTip(tr("&Load .DEB on ROM Load"));
	act->setCheckable(true);
	act->setChecked( opt ? true : false );
	connect( act, SIGNAL(triggered(bool)), this, SLOT(debFileAutoLoadCB(bool)) );

	optMenu->addAction(act);

	optMenu->addSeparator();

	// Symbols
	symMenu = menuBar->addMenu(tr("&Symbols"));

	// Symbols -> Reload
	act = new QAction(tr("&Reload"), this);
	//act->setShortcut(QKeySequence( tr("F7") ) );
	act->setStatusTip(tr("&Reload"));
	//act->setCheckable(true);
	//act->setChecked( break_on_unlogged_data );
	connect( act, SIGNAL(triggered(void)), this, SLOT(reloadSymbolsCB(void)) );

	symMenu->addAction(act);

	symMenu->addSeparator();

	// Symbols -> Symbolic Debug
	g_config->getOption( "SDL.DebuggerShowSymNames", &opt );

	act = new QAction(tr("&Symbolic Debug"), this);
	//act->setShortcut(QKeySequence( tr("F7") ) );
	act->setStatusTip(tr("&Symbolic Debug"));
	act->setCheckable(true);
	act->setChecked(opt);
	connect( act, SIGNAL(triggered(bool)), this, SLOT(symbolDebugEnableCB(bool)) );

	symMenu->addAction(act);

	// Symbols -> Register Names
	g_config->getOption( "SDL.DebuggerShowRegNames", &opt );

	act = new QAction(tr("&Register Names"), this);
	//act->setShortcut(QKeySequence( tr("F7") ) );
	act->setStatusTip(tr("&Register Names"));
	act->setCheckable(true);
	act->setChecked(opt);
	connect( act, SIGNAL(triggered(bool)), this, SLOT(registerNameEnableCB(bool)) );

	symMenu->addAction(act);

	//-----------------------------------------------------------------------
	// Menu End
	//-----------------------------------------------------------------------
	
	return menuBar;
}
//----------------------------------------------------------------------------
QToolBar *ConsoleDebugger::buildToolBar(void)
{
	QAction     *act;

	QToolBar *toolBar = new QToolBar(this);

	//-----------------------------------------------------------------------
	// Tool Bar Setup Start
	//-----------------------------------------------------------------------
	
	// File -> Go to Address
	act = new QAction(tr("&Go to Address"), this);
	//act->setShortcut( QKeySequence(tr("Ctrl+A") ));
	act->setStatusTip(tr("&Go to Address"));
	//act->setIcon( QIcon(":icons/find.png") );
	act->setIcon( QIcon(":icons/JumpTarget.png") );
	connect(act, SIGNAL(triggered()), this, SLOT(openGotoAddrDialog(void)) );

	toolBar->addAction(act);

	toolBar->addSeparator();

	// Debug -> Run
	act = new QAction(tr("&Run (F5)"), this);
	//act->setShortcut(QKeySequence( tr("F5") ) );
	act->setStatusTip(tr("Run"));
	//act->setIcon( style()->standardIcon( QStyle::SP_MediaPlay ) );
	act->setIcon( QIcon(":icons/debug-run.png") );
	connect( act, SIGNAL(triggered()), this, SLOT(debugRunCB(void)) );

	toolBar->addAction(act);

	// Debug -> Step Into
	act = new QAction(tr("Step &Into (F11)"), this);
	//act->setShortcut(QKeySequence( tr("F11") ) );
	act->setStatusTip(tr("Step Into"));
	act->setIcon( QIcon(":icons/StepInto.png") );
	connect( act, SIGNAL(triggered()), this, SLOT(debugStepIntoCB(void)) );

	toolBar->addAction(act);

	// Debug -> Step Out
	act = new QAction(tr("&Step Out (Shift+F11)"), this);
	//act->setShortcut(QKeySequence( tr("Shift+F11") ) );
	act->setStatusTip(tr("Step Out"));
	act->setIcon( QIcon(":icons/StepOut.png") );
	connect( act, SIGNAL(triggered()), this, SLOT(debugStepOutCB(void)) );

	toolBar->addAction(act);

	// Debug -> Step Over
	act = new QAction(tr("Step &Over (F10)"), this);
	//act->setShortcut(QKeySequence( tr("F10") ) );
	act->setStatusTip(tr("Step Over"));
	act->setIcon( QIcon(":icons/StepOver.png") );
	connect( act, SIGNAL(triggered()), this, SLOT(debugStepOverCB(void)) );

	toolBar->addAction(act);

	toolBar->addSeparator();

	// Debug -> Run Line
	act = new QAction(tr("Run &Line (F6)"), this);
	//act->setShortcut(QKeySequence( tr("F6") ) );
	act->setStatusTip(tr("Run Line"));
	act->setIcon( QIcon(":icons/RunPpuScanline.png") );
	connect( act, SIGNAL(triggered()), this, SLOT(debugRunLineCB(void)) );

	toolBar->addAction(act);

	// Debug -> Run 128 Lines
	act = new QAction(tr("Run &128 Lines (F7)"), this);
	//act->setShortcut(QKeySequence( tr("F7") ) );
	act->setStatusTip(tr("Run 128 Lines"));
	act->setIcon( QIcon(":icons/RunPpuFrame.png") );
	connect( act, SIGNAL(triggered()), this, SLOT(debugRunLine128CB(void)) );

	toolBar->addAction(act);

	toolBar->addSeparator();

	// Debug -> Reset Counters
	act = new QAction(tr("Reset &Counters"), this);
	//act->setShortcut(QKeySequence( tr("F7") ) );
	act->setStatusTip(tr("Reset Counters"));
	act->setIcon( style()->standardIcon( QStyle::SP_BrowserReload ) );
	connect( act, SIGNAL(triggered(void)), this, SLOT(resetCountersCB(void)) );

	toolBar->addAction(act);

	toolBar->addSeparator();

	//-----------------------------------------------------------------------
	// Tool Bar Setup End
	//-----------------------------------------------------------------------
	
	return toolBar;
}
//----------------------------------------------------------------------------
void ConsoleDebugger::buildAsmViewDisplay(void)
{
	QGridLayout *grid;

	grid       = new QGridLayout();
	asmView    = new QAsmView(this);
	vbar       = new QScrollBar( Qt::Vertical, this );
	hbar       = new QScrollBar( Qt::Horizontal, this );
	asmLineSelLbl = new QLabel( tr("Line Select") );
	emuStatLbl    = new QLabel( tr("Emulator is Running") );

	asmLineSelLbl->setWordWrap( true );

	asmView->setScrollBars( hbar, vbar );

	grid->addWidget( asmView, 0, 0 );
	grid->addWidget( vbar   , 0, 1 );
	grid->addWidget( hbar   , 1, 0 );

	 asmDpyVbox   = new QVBoxLayout();

	hbar->setMinimum(0);
	hbar->setMaximum(100);
	vbar->setMinimum(0);
	vbar->setMaximum( 0x10000 );

	asmDpyVbox->addLayout( grid, 100 );
	asmDpyVbox->addWidget( asmLineSelLbl, 1 );
	asmDpyVbox->addWidget( emuStatLbl   , 1 );
	
	asmViewContainerWidget = new QWidget();
	asmViewContainerWidget->setLayout( asmDpyVbox );

}
//----------------------------------------------------------------------------
void ConsoleDebugger::buildCpuListDisplay(void)
{
	QVBoxLayout *vbox, *vbox1;
	QHBoxLayout *hbox, *hbox1;
	QGridLayout *grid;
	QLabel      *lbl;
	QFont        cpuFont;
	std::string  fontString;
	int          fontCharWidth;

	g_config->getOption("SDL.DebuggerCpuStatusFont", &fontString);

	if ( fontString.size() > 0 )
	{
		cpuFont.fromString( QString::fromStdString( fontString ) );
	}
	else
	{
		cpuFont.setFamily("Courier New");
		cpuFont.setStyle( QFont::StyleNormal );
		cpuFont.setStyleHint( QFont::Monospace );
	}

	QFontMetrics fm(cpuFont);

	fontCharWidth = fm.averageCharWidth();

	vbox    = new QVBoxLayout();
	vbox1   = new QVBoxLayout();
	hbox1   = new QHBoxLayout();

	cpuFrame = new QFrame();
	grid     = new QGridLayout();

	cpuFrame->setObjectName( tr("debuggerStatusCPU") );
	cpuFrame->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Maximum );

	hbox1->addLayout( vbox, 1 );
	vbox->addLayout( grid  );
	vbox1->addLayout( hbox1, 1 );
	vbox1->addStretch( 10 );
	cpuFrame->setLayout( vbox1 );

	hbox = new QHBoxLayout();
	lbl  = new QLabel( tr("PC:") );
	lbl->setToolTip( tr("Program Counter Register") );
	pcEntry = new QLineEdit();
	pcEntry->setFont( cpuFont );
	pcEntry->setMaxLength( 4 );
	pcEntry->setInputMask( ">HHHH;0" );
	pcEntry->setAlignment(Qt::AlignCenter);
	pcEntry->setMinimumWidth( 6 * fontCharWidth );
	pcEntry->setMaximumWidth( 6 * fontCharWidth );
	pcEntry->setToolTip( tr("Program Counter Register Hex Value") );
	hbox->addWidget( lbl );
	hbox->addWidget( pcEntry, 1, Qt::AlignLeft );
	grid->addLayout( hbox, 0, 0, Qt::AlignLeft );

	//button = new QPushButton( tr("Seek PC") );
	//grid->addWidget( button, 4, 1, Qt::AlignLeft );
	//connect( button, SIGNAL(clicked(void)), this, SLOT(seekPCCB(void)) );

	hbox = new QHBoxLayout();
	lbl  = new QLabel( tr("A:") );
	lbl->setToolTip( tr("Accumulator Register") );
	regAEntry = new QLineEdit();
	regAEntry->setFont( cpuFont );
	regAEntry->setMaxLength( 2 );
	regAEntry->setInputMask( ">HH;0" );
	regAEntry->setAlignment(Qt::AlignCenter);
	regAEntry->setMinimumWidth( 4 * fontCharWidth );
	regAEntry->setMaximumWidth( 4 * fontCharWidth );
	regAEntry->setToolTip( tr("Accumulator Register Hex Value") );
	hbox->addWidget( lbl );
	hbox->addWidget( regAEntry, 1, Qt::AlignLeft );
	grid->addLayout( hbox, 0, 1 );

	hbox = new QHBoxLayout();
	lbl  = new QLabel( tr("X:") );
	lbl->setToolTip( tr("X Index Register") );
	regXEntry = new QLineEdit();
	regXEntry->setFont( cpuFont );
	regXEntry->setMaxLength( 2 );
	regXEntry->setInputMask( ">HH;0" );
	regXEntry->setAlignment(Qt::AlignCenter);
	regXEntry->setMinimumWidth( 4 * fontCharWidth );
	regXEntry->setMaximumWidth( 4 * fontCharWidth );
	regXEntry->setToolTip( tr("X Index Register Hex Value") );
	hbox->addWidget( lbl );
	hbox->addWidget( regXEntry, 1, Qt::AlignLeft );
	grid->addLayout( hbox, 0, 2 );

	hbox = new QHBoxLayout();
	lbl  = new QLabel( tr("Y:") );
	lbl->setToolTip( tr("Y Index Register") );
	regYEntry = new QLineEdit();
	regYEntry->setFont( cpuFont );
	regYEntry->setMaxLength( 2 );
	regYEntry->setInputMask( ">HH;0" );
	regYEntry->setAlignment(Qt::AlignCenter);
	regYEntry->setMinimumWidth( 4 * fontCharWidth );
	regYEntry->setMaximumWidth( 4 * fontCharWidth );
	regYEntry->setToolTip( tr("Y Index Register Hex Value") );
	hbox->addWidget( lbl );
	hbox->addWidget( regYEntry, 1, Qt::AlignLeft );
	grid->addLayout( hbox, 0, 3 );

	QHBoxLayout *regPHbox = new QHBoxLayout();
	lbl  = new QLabel( tr("P:") );
	lbl->setToolTip( tr("Status Register") );
	regPEntry = new QLineEdit();
	regPEntry->setFont( cpuFont );
	regPEntry->setMaxLength( 2 );
	regPEntry->setInputMask( ">HH;0" );
	regPEntry->setAlignment(Qt::AlignCenter);
	regPEntry->setMinimumWidth( 4 * fontCharWidth );
	regPEntry->setMaximumWidth( 4 * fontCharWidth );
	regPEntry->setToolTip( tr("Status Register Hex Value") );
	regPHbox->addWidget( lbl );
	regPHbox->addWidget( regPEntry, 1, Qt::AlignLeft );
	//grid->addLayout( regPHbox, 0, 4 );

	cpuCyclesLbl1 = new QLabel( tr("CPU Cycles:") );
	//cpuCyclesLbl2 = new QLabel( tr("(+0):") );
	cpuCyclesVal  = new QLineEdit( tr("(+0):") );
	cpuInstrsLbl1 = new QLabel( tr("Instructions:") );
	//cpuInstrsLbl2 = new QLabel( tr("(+0):") );
	cpuInstrsVal  = new QLineEdit( tr("(+0):") );
	//brkCpuCycExd  = new QCheckBox( tr("Break when Exceed") );
	//brkInstrsExd  = new QCheckBox( tr("Break when Exceed") );
	//cpuCycExdVal  = new QLineEdit( tr("0") );
	//instrExdVal   = new QLineEdit( tr("0") );

	cpuCyclesVal->setFont(cpuFont);
	cpuCyclesVal->setReadOnly(true);
	cpuCyclesVal->setMinimumWidth( 24 * fontCharWidth );
	cpuInstrsVal->setFont(cpuFont);
	cpuInstrsVal->setReadOnly(true);
	cpuInstrsVal->setMinimumWidth( 24 * fontCharWidth );

	//hbox = new QHBoxLayout();
	//hbox->addWidget( cpuCyclesLbl1, 1 );
	//hbox->addWidget( cpuCyclesVal , 4 );
	//grid->addLayout( hbox, 1, 0, 1, 4 );
	//hbox = new QHBoxLayout();
	//hbox->addWidget( cpuInstrsLbl1, 1 );
	//hbox->addWidget( cpuInstrsVal , 4 );
	//grid->addLayout( hbox, 2, 0, 1, 4 );

	grid->addWidget( cpuCyclesLbl1, 1, 0, 1, 1 );
	grid->addWidget( cpuCyclesVal , 1, 1, 1, 3 );
	grid->addWidget( cpuInstrsLbl1, 2, 0, 1, 1 );
	grid->addWidget( cpuInstrsVal , 2, 1, 1, 3 );
	
	stackFrame = new QGroupBox(tr("Stack $0100"));
	stackText  = new DebuggerStackDisplay(this);
	hbox       = new QHBoxLayout();
	hbox->addWidget( stackText );
	//vbox2->addWidget( stackFrame );
	hbox1->addWidget( stackFrame, 10 );
	stackFrame->setLayout( hbox );
	//stackText->setFont(font); // Stack font is now set in constructor
	stackText->setReadOnly(true);
	stackText->setWordWrapMode( QTextOption::NoWrap );
	//stackText->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	//stackText->setMaximumWidth( 16 * fontCharWidth );
	stackFrame->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Maximum );

	sfFrame = new QGroupBox(tr("Status Flags"));
	grid    = new QGridLayout();
	sfFrame->setLayout( grid );
	sfFrame->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Maximum );

	N_chkbox = new QCheckBox( tr("N") );
	V_chkbox = new QCheckBox( tr("V") );
	U_chkbox = new QCheckBox( tr("U") );
	B_chkbox = new QCheckBox( tr("B") );
	D_chkbox = new QCheckBox( tr("D") );
	I_chkbox = new QCheckBox( tr("I") );
	Z_chkbox = new QCheckBox( tr("Z") );
	C_chkbox = new QCheckBox( tr("C") );

	N_chkbox->setToolTip( tr("Negative" ) );
	V_chkbox->setToolTip( tr("Overflow" ) );
	U_chkbox->setToolTip( tr("Unused"   ) );
	B_chkbox->setToolTip( tr("Break"    ) );
	D_chkbox->setToolTip( tr("Decimal"  ) );
	I_chkbox->setToolTip( tr("Interrupt") );
	Z_chkbox->setToolTip( tr("Zero"     ) );
	C_chkbox->setToolTip( tr("Carry"    ) );

	grid->addLayout( regPHbox, 0, 0, 2, 1);
	grid->addWidget( N_chkbox, 0, 1, Qt::AlignLeft );
	grid->addWidget( V_chkbox, 0, 2, Qt::AlignLeft );
	grid->addWidget( U_chkbox, 0, 3, Qt::AlignLeft );
	grid->addWidget( B_chkbox, 0, 4, Qt::AlignLeft );
	grid->addWidget( D_chkbox, 1, 1, Qt::AlignLeft );
	grid->addWidget( I_chkbox, 1, 2, Qt::AlignLeft );
	grid->addWidget( Z_chkbox, 1, 3, Qt::AlignLeft );
	grid->addWidget( C_chkbox, 1, 4, Qt::AlignLeft );

	vbox->addWidget( sfFrame);
}
//----------------------------------------------------------------------------
void ConsoleDebugger::buildPpuListDisplay(void)
{
	QFontMetrics fm(cpuFont);
	QVBoxLayout  *vbox1;
	//QHBoxLayout *hbox;
	QGridLayout *grid;
	QLabel      *ppuCtrlLbl, *ppuMaskLbl, *ppuStatLbl, *ppuAddrLbl,
		    *oamAddrLbl, *ppuLineLbl, *ppuPixLbl;
	int fontCharWidth;

	fontCharWidth = fm.averageCharWidth();

	ppuStatContainerWidget = new QWidget(this);
	ppuStatContainerWidget->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Maximum );

	ppuFrame    = new QFrame();
	grid        = new QGridLayout();

	ppuDataGrid = grid;
	ppuFrame->setObjectName( tr("debuggerStatusPPU") );

	ppuCtrlLbl = new QLabel( tr("PPUCTRL:") );
	ppuMaskLbl = new QLabel( tr("PPUMASK:") );
	ppuStatLbl = new QLabel( tr("PPUSTAT:") );
	ppuAddrLbl = new QLabel( tr("PPUADDR:") );
	oamAddrLbl = new QLabel( tr("OAMADDR:") );
	ppuLineLbl = new QLabel( tr("Scanline:") );
	 ppuPixLbl = new QLabel( tr("Pixel:") );

	ppuCtrlLbl->setToolTip( tr("PPU Control Register, Address $2000") );
	ppuMaskLbl->setToolTip( tr("PPU Mask Register, Address $2001") );
	ppuStatLbl->setToolTip( tr("PPU Status Register, Address $2002") );
	oamAddrLbl->setToolTip( tr("OAM Address Register, Address $2003") );
	ppuAddrLbl->setToolTip( tr("PPU Address Register, Address $2006") );
	ppuLineLbl->setToolTip( tr("PPU Current Scanline being processed") );
	 ppuPixLbl->setToolTip( tr("PPU Current Pixel being processed") );

	ppuCtrlReg     = new ppuCtrlRegDpy();
	ppuMaskReg     = new ppuCtrlRegDpy();
	ppuStatReg     = new ppuCtrlRegDpy();
	ppuAddrDsp     = new QLineEdit();
	oamAddrDsp     = new QLineEdit();
	ppuScanLineDsp = new QLineEdit();
	ppuPixelDsp    = new QLineEdit();
	ppuScrollX     = new QLineEdit();
	ppuScrollY     = new QLineEdit();

	ppuAddrDsp->setReadOnly(true);
	oamAddrDsp->setReadOnly(true);
	ppuScanLineDsp->setReadOnly(true);
	ppuPixelDsp->setReadOnly(true);
	ppuScrollX->setReadOnly(true);
	ppuScrollY->setReadOnly(true);

	ppuCtrlReg->setFont( cpuFont );
	ppuMaskReg->setFont( cpuFont );
	ppuStatReg->setFont( cpuFont );
	ppuAddrDsp->setFont( cpuFont );
	oamAddrDsp->setFont( cpuFont );
	ppuScanLineDsp->setFont( cpuFont );
	ppuPixelDsp->setFont( cpuFont );
	ppuScrollX->setFont( cpuFont );
	ppuScrollY->setFont( cpuFont );

	ppuCtrlReg->setMinimumWidth( 4 * fontCharWidth );
	ppuMaskReg->setMinimumWidth( 4 * fontCharWidth );
	ppuStatReg->setMinimumWidth( 4 * fontCharWidth );
	ppuAddrDsp->setMinimumWidth( 7 * fontCharWidth );
	oamAddrDsp->setMinimumWidth( 4 * fontCharWidth );
	ppuScanLineDsp->setMinimumWidth( 4 * fontCharWidth );
	ppuPixelDsp->setMinimumWidth( 4 * fontCharWidth );
	ppuScrollX->setMinimumWidth( 4 * fontCharWidth );
	ppuScrollY->setMinimumWidth( 4 * fontCharWidth );

	ppuCtrlReg->setMaximumWidth( 4 * fontCharWidth );
	ppuMaskReg->setMaximumWidth( 4 * fontCharWidth );
	ppuStatReg->setMaximumWidth( 4 * fontCharWidth );
	ppuAddrDsp->setMaximumWidth( 7 * fontCharWidth );
	oamAddrDsp->setMaximumWidth( 4 * fontCharWidth );
	ppuScanLineDsp->setMaximumWidth( 4 * fontCharWidth );
	ppuPixelDsp->setMaximumWidth( 4 * fontCharWidth );
	ppuScrollX->setMaximumWidth( 8 * fontCharWidth );
	ppuScrollY->setMaximumWidth( 8 * fontCharWidth );

	grid->setColumnMinimumWidth(1, 8 * fontCharWidth);
	grid->setColumnMinimumWidth(3, 8 * fontCharWidth);

	grid->addWidget( ppuCtrlLbl, 0, 0 );
	grid->addWidget( ppuCtrlReg, 0, 1 );

	grid->addWidget( ppuMaskLbl, 0, 2 );
	grid->addWidget( ppuMaskReg, 0, 3 );

	grid->addWidget( ppuStatLbl, 0, 4 );
	grid->addWidget( ppuStatReg, 0, 5 );

	grid->addWidget( oamAddrLbl, 1, 0 );
	grid->addWidget( oamAddrDsp, 1, 1 );

	grid->addWidget( ppuAddrLbl, 1, 2 );
	grid->addWidget( ppuAddrDsp, 1, 3 );

	grid->addWidget( ppuLineLbl, 2, 0 );
	grid->addWidget( ppuScanLineDsp, 2, 1 );

	grid->addWidget( ppuPixLbl, 2, 2 );
	grid->addWidget( ppuPixelDsp, 2, 3 );

	grid->addWidget( new QLabel( tr("X Scroll:") ), 3, 0 );
	grid->addWidget( ppuScrollX, 3, 1 );

	grid->addWidget( new QLabel( tr("Y Scroll:") ), 3, 2 );
	grid->addWidget( ppuScrollY, 3, 3 );

	ppuStatContainerWidget->setLayout( grid  );

	vbox1 = new QVBoxLayout();
	vbox1->addWidget( ppuStatContainerWidget, 1 );
	vbox1->addStretch( 10 );

	ppuFrame->setLayout( vbox1  );
}
//----------------------------------------------------------------------------
void ConsoleDebugger::buildBpListDisplay(void)
{
	QVBoxLayout *vbox;
	QHBoxLayout *hbox;
	QPushButton *button;
	QTreeWidgetItem *item;
	QFontMetrics fm(font);

	bpFrame = new QFrame();
	vbox    = new QVBoxLayout();
	bpTree  = new QTreeWidget();

	bpFrame->setObjectName( tr("debuggerBreakpointList") );
	bpTree->setColumnCount(2);
	bpTree->setSelectionMode( QAbstractItemView::SingleSelection );
	bpTree->setMinimumHeight( 3 * fm.lineSpacing() );
	bpTree->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Ignored );

	item = new QTreeWidgetItem();
	item->setFont( 0, font );
	item->setFont( 1, font );
	item->setFont( 2, font );
	item->setFont( 3, font );
	item->setText( 0, QString::fromStdString( "Addr" ) );
	item->setText( 1, QString::fromStdString( "Flags" ) );
	item->setText( 2, QString::fromStdString( "Cond" ) );
	item->setText( 3, QString::fromStdString( "Desc" ) );
	item->setTextAlignment( 0, Qt::AlignCenter);
	item->setTextAlignment( 1, Qt::AlignCenter);
	item->setTextAlignment( 2, Qt::AlignCenter);
	item->setTextAlignment( 3, Qt::AlignCenter);

	bpTree->setHeaderItem( item );

	bpTree->header()->setSectionResizeMode( QHeaderView::ResizeToContents );

	connect( bpTree, SIGNAL(itemClicked(QTreeWidgetItem*, int)),
			   this, SLOT(bpItemClicked( QTreeWidgetItem*, int)) );

	hbox   = new QHBoxLayout();
	bpAddBtn = button = new QPushButton( tr("Add") );
	hbox->addWidget( button );
	connect( button, SIGNAL(clicked(void)), this, SLOT(add_BP_CB(void)) );

	bpEditBtn = button = new QPushButton( tr("Edit") );
	hbox->addWidget( button );
	connect( button, SIGNAL(clicked(void)), this, SLOT(edit_BP_CB(void)) );

	bpDelBtn = button = new QPushButton( tr("Delete") );
	hbox->addWidget( button );
	connect( button, SIGNAL(clicked(void)), this, SLOT(delete_BP_CB(void)) );

	vbox->addWidget( bpTree );
	vbox->addLayout( hbox   );
	bpTreeContainerWidget = new QWidget(this);
	bpTreeContainerWidget->setLayout( vbox );
	vbox    = new QVBoxLayout();
	vbox->addWidget( bpTreeContainerWidget );
	bpFrame->setLayout( vbox );
}
//----------------------------------------------------------------------------
void ConsoleDebugger::buildBmListDisplay(void)
{
	QVBoxLayout *vbox;
	QHBoxLayout *hbox;
	QPushButton *button;
	QTreeWidgetItem *item;
	QFontMetrics fm(font);

	bmTreeContainerWidget = new QWidget(this);

	hbox      = new QHBoxLayout();
	vbox      = new QVBoxLayout();
	bmFrame   = new QFrame();
	bmTree    = new QTreeWidget();
	selBmAddr = new QLineEdit();
	selBmAddrVal = 0;

	connect( selBmAddr, SIGNAL(textChanged(const QString &)), this, SLOT(selBmAddrChanged(const QString &)));

	bmFrame->setObjectName( tr("debuggerBookmarkList") );
	bmTree->setColumnCount(2);
	bmTree->setMinimumHeight( 3 * fm.lineSpacing() );
	bmTree->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Ignored );

	item = new QTreeWidgetItem();
	item->setFont( 0, font );
	item->setFont( 1, font );
	item->setText( 0, QString::fromStdString( "Addr" ) );
	item->setText( 1, QString::fromStdString( "Name" ) );
	item->setTextAlignment( 0, Qt::AlignCenter);
	item->setTextAlignment( 1, Qt::AlignCenter);

	bmTree->setHeaderItem( item );

	//bmTree->header()->setSectionResizeMode( QHeaderView::ResizeToContents );

	connect( bmTree, SIGNAL(itemClicked(QTreeWidgetItem*, int)),
			   this, SLOT(bmItemClicked( QTreeWidgetItem*, int)) );

	connect( bmTree, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)),
			   this, SLOT(bmItemDoubleClicked( QTreeWidgetItem*, int)) );

	vbox->addWidget( selBmAddr, 1 );

	button    = new QPushButton( tr("Add") );
	vbox->addWidget( button, 1 );
	connect( button, SIGNAL(clicked(void)), this, SLOT(add_BM_CB(void)) );

	button    = new QPushButton( tr("Delete") );
	vbox->addWidget( button, 1 );
	connect( button, SIGNAL(clicked(void)), this, SLOT(delete_BM_CB(void)) );

	button    = new QPushButton( tr("Name") );
	vbox->addWidget( button, 1 );
	connect( button, SIGNAL(clicked(void)), this, SLOT(edit_BM_CB(void)) );

	vbox->addStretch( 10 );

	hbox->addWidget( bmTree, 10 );
	hbox->addLayout( vbox  ,  1 );

	bmTreeContainerWidget->setLayout( hbox );

	bmTreeContainerWidget->setVisible(true);

	vbox = new QVBoxLayout();
	vbox->addWidget( bmTreeContainerWidget );

	bmFrame->setLayout( vbox );
}
//----------------------------------------------------------------------------
void ConsoleDebugger::loadDisplayViews(void)
{
	char key[128];
	QSettings  settings;
	bool prevStateSaved;

	prevStateSaved = settings.value("debugger/prevStateSaved", false).toBool();

	if ( !prevStateSaved )
	{
		setLayoutOption(2);
	}

	for (int i=0; i<2; i++)
	{
		for (int j=0; j<4; j++)
		{
			QString tabListVal;
			sprintf( key, "debugger/tabView%i%i", i+1, j+1 );
			tabListVal = settings.value(key).toString();

			QStringList tabList = tabListVal.split(',');
			for (int k=0; k<tabList.size(); k++)
			{
				if ( tabList[k].size() > 0 )
				{
					//printf("   %i: %s\n", k, tabList[k].toStdString().c_str() );

					if ( tabList[k].compare( cpuFrame->objectName() ) == 0 )
					{
						tabView[i][j]->addTab( cpuFrame, tr("CPU") );
					}
					else if ( tabList[k].compare( ppuFrame->objectName() ) == 0 )
					{
						tabView[i][j]->addTab( ppuFrame, tr("PPU") );
					}
					else if ( tabList[k].compare( bpFrame->objectName() ) == 0 )
					{
						tabView[i][j]->addTab( bpFrame, tr("Breakpoints") );
					}
					else if ( tabList[k].compare( bmFrame->objectName() ) == 0 )
					{
						tabView[i][j]->addTab( bmFrame, tr("Bookmarks") );
					}
				}
			}
		}
	}

	if ( cpuFrame->parent() == nullptr )
	{
		tabView[0][0]->addTab( cpuFrame, tr("CPU") );
	}
	if ( ppuFrame->parent() == nullptr )
	{
		tabView[0][0]->addTab( ppuFrame, tr("PPU") );
	}
	if ( bpFrame->parent() == nullptr )
	{
		tabView[0][0]->addTab( bpFrame, tr("Breakpoints") );
	}
	if ( bmFrame->parent() == nullptr )
	{
		tabView[0][0]->addTab( bmFrame, tr("Bookmarks") );
	}

	// Restore Window Geometry
	restoreGeometry(settings.value("debugger/geometry").toByteArray());

	// Restore Horizontal Panel State
	mainLayouth->restoreState( settings.value("debugger/hPanelState").toByteArray() );

	// Save Vertical Panel State
	for (int i=0; i<2; i++)
	{
		sprintf( key, "debugger/vPanelState%i", i+1);
		vsplitter[i]->restoreState( settings.value(key).toByteArray() );
	}

	updateTabVisibility();
}
//----------------------------------------------------------------------------
void ConsoleDebugger::saveDisplayViews(void)
{
	char key[128];
	QSettings  settings;

	// Save Tab Placement
	for (int i=0; i<2; i++)
	{
		for (int j=0; j<4; j++)
		{
			QString tabListVal;
			sprintf( key, "debugger/tabView%i%i", i+1, j+1 );

			for (int k=0; k<tabView[i][j]->count(); k++)
			{
				QWidget *w = tabView[i][j]->widget(k);

				//printf("(%i,%i,%i)   %s\n", i, j, k, w->objectName().toStdString().c_str() );

				tabListVal += w->objectName() + ",";
			}

			//printf("(%i,%i) %s\n", i, j, tabListVal.toStdString().c_str() );
			settings.setValue( key, tabListVal );
		}
	}

	// Save Horizontal Panel State
	settings.setValue("debugger/hPanelState", mainLayouth->saveState());

	// Save Vertical Panel State
	for (int i=0; i<2; i++)
	{
		sprintf( key, "debugger/vPanelState%i", i+1);
		settings.setValue( key, vsplitter[i]->saveState());
	}

	// Save Window Geometry
	settings.setValue("debugger/geometry", saveGeometry());

	// Set Window 
	settings.setValue("debugger/prevStateSaved", true);
}
//----------------------------------------------------------------------------
void ConsoleDebugger::updateTabVisibility(void)
{
	for (int i=0; i<2; i++)
	{
		for (int j=0; j<4; j++)
		{
			if ( tabView[i][j]->count() > 0 )
			{
				if ( !tabView[i][j]->isVisible() )
				{
					QList<int> s = vsplitter[i]->sizes();

					s[j] = tabView[i][j]->minimumSizeHint().height();
					
					vsplitter[i]->setSizes(s);
				}
				tabView[i][j]->setVisible( true );
			}
			else
			{
				tabView[i][j]->setVisible( false );
			}
		}
	}
}
//----------------------------------------------------------------------------
void ConsoleDebugger::moveTab( QWidget *w, int row, int column)
{
	int idx = -1;
	DebuggerTabWidget *p = NULL;

	for (int i=0; i<2; i++)
	{
		for (int j=0; j<4; j++)
		{
			idx = tabView[i][j]->indexOf(w);

			if ( idx >= 0 )
			{
				p = tabView[i][j]; break;
			}
		}
		if (p) break;
	}

	if ( p )
	{
		QString txt = p->tabBar()->tabText( idx );
		p->removeTab( idx );
		tabView[column][row]->addTab(w, txt);
		//printf("Move Widget %p to (%i,%i) %s\n", w, row, column, txt.toStdString().c_str() ); 
	}
	updateTabVisibility();
}
//----------------------------------------------------------------------------
void ConsoleDebugger::setCpuStatusFont( const QFont &font )
{
	QFontMetrics fm(font);
	int fontCharWidth;

	fontCharWidth = fm.averageCharWidth();

	pcEntry->setFont( font );
	pcEntry->setMinimumWidth( 6 * fontCharWidth );
	pcEntry->setMaximumWidth( 6 * fontCharWidth );

	regAEntry->setFont( font );
	regAEntry->setMinimumWidth( 4 * fontCharWidth );
	regAEntry->setMaximumWidth( 4 * fontCharWidth );

	regXEntry->setFont( font );
	regXEntry->setMinimumWidth( 4 * fontCharWidth );
	regXEntry->setMaximumWidth( 4 * fontCharWidth );

	regYEntry->setFont( font );
	regYEntry->setMinimumWidth( 4 * fontCharWidth );
	regYEntry->setMaximumWidth( 4 * fontCharWidth );

	regPEntry->setFont( font );
	regPEntry->setMinimumWidth( 4 * fontCharWidth );
	regPEntry->setMaximumWidth( 4 * fontCharWidth );

	cpuCyclesVal->setFont(font);
	cpuCyclesVal->setMinimumWidth( 24 * fontCharWidth );
	cpuInstrsVal->setFont(font);
	cpuInstrsVal->setMinimumWidth( 24 * fontCharWidth );
}
//----------------------------------------------------------------------------
void ConsoleDebugger::setPpuStatusFont( const QFont &font )
{
	QFontMetrics fm(font);
	int fontCharWidth;

	fontCharWidth = fm.averageCharWidth();

	ppuCtrlReg->setFont( font );
	ppuMaskReg->setFont( font );
	ppuStatReg->setFont( font );
	ppuAddrDsp->setFont( font );
	oamAddrDsp->setFont( font );
	ppuScanLineDsp->setFont( font );
	ppuPixelDsp->setFont( font );
	ppuScrollX->setFont( font );
	ppuScrollY->setFont( font );

	ppuCtrlReg->setMinimumWidth( 4 * fontCharWidth );
	ppuMaskReg->setMinimumWidth( 4 * fontCharWidth );
	ppuStatReg->setMinimumWidth( 4 * fontCharWidth );
	ppuAddrDsp->setMinimumWidth( 7 * fontCharWidth );
	oamAddrDsp->setMinimumWidth( 4 * fontCharWidth );
	ppuScanLineDsp->setMinimumWidth( 4 * fontCharWidth );
	ppuPixelDsp->setMinimumWidth( 4 * fontCharWidth );
	ppuScrollX->setMinimumWidth( 4 * fontCharWidth );
	ppuScrollY->setMinimumWidth( 4 * fontCharWidth );

	ppuCtrlReg->setMaximumWidth( 4 * fontCharWidth );
	ppuMaskReg->setMaximumWidth( 4 * fontCharWidth );
	ppuStatReg->setMaximumWidth( 4 * fontCharWidth );
	ppuAddrDsp->setMaximumWidth( 7 * fontCharWidth );
	oamAddrDsp->setMaximumWidth( 4 * fontCharWidth );
	ppuScanLineDsp->setMaximumWidth( 4 * fontCharWidth );
	ppuPixelDsp->setMaximumWidth( 4 * fontCharWidth );
	ppuScrollX->setMaximumWidth( 8 * fontCharWidth );
	ppuScrollY->setMaximumWidth( 8 * fontCharWidth );

	ppuDataGrid->setColumnMinimumWidth(1, 8 * fontCharWidth);
	ppuDataGrid->setColumnMinimumWidth(3, 8 * fontCharWidth);

}
//----------------------------------------------------------------------------
void 	ConsoleDebugger::bpItemClicked( QTreeWidgetItem *item, int column)
{
	int row = bpTree->indexOfTopLevelItem(item);

	//printf("Row: %i Column: %i \n", row, column );

	if ( column == 0 )
	{
		if ( (row >= 0) && (row < numWPs) )
		{	
			int isChecked = item->checkState( column ) != Qt::Unchecked;

			if ( isChecked )
			{
				watchpoint[row].flags |=  WP_E;
			}
			else
			{
				watchpoint[row].flags &= ~WP_E;
			}
		}
	}
}
//----------------------------------------------------------------------------
void 	ConsoleDebugger::bmItemClicked( QTreeWidgetItem *item, int column)
{
	//int row = bmTree->indexOfTopLevelItem(item);

	//printf("Row: %i Column: %i \n", row, column );

}
//----------------------------------------------------------------------------
void 	ConsoleDebugger::bmItemDoubleClicked( QTreeWidgetItem *item, int column)
{
	int addr, line;
	//int row = bmTree->indexOfTopLevelItem(item);

	//printf("Row: %i Column: %i \n", row, column );

	addr = strtol( item->text(0).toStdString().c_str(), NULL, 16 );

	line = asmView->getAsmLineFromAddr( addr );

	asmView->setLine( line );
}
//----------------------------------------------------------------------------
void ConsoleDebugger::selBmAddrChanged(const QString &txt)
{
	selBmAddrVal = strtol( txt.toStdString().c_str(), NULL, 16 );

	//printf("selBmAddrVal = %04X\n", selBmAddrVal );
}
//----------------------------------------------------------------------------
void ConsoleDebugger::openBpEditWindow( int editIdx, watchpointinfo *wp, bool forceAccept )
{
	int ret;
	QDialog dialog(this);
	QHBoxLayout *hbox;
	QVBoxLayout *mainLayout, *vbox;
	QLabel *lbl;
	QLineEdit *addr1, *addr2, *cond, *name;
	QCheckBox *forbidChkBox, *rbp, *wbp, *xbp, *ebp;
	QGridLayout *grid;
	QFrame *frame;
	QGroupBox *gbox;
	QPushButton *okButton, *cancelButton;
	QRadioButton *cpu_radio, *ppu_radio, *oam_radio, *rom_radio;

	if ( editIdx >= 0 )
	{
		dialog.setWindowTitle( tr("Edit Breakpoint") );
	}
	else
	{
		dialog.setWindowTitle( tr("Add Breakpoint") );
	}

	hbox       = new QHBoxLayout();
	mainLayout = new QVBoxLayout();

	mainLayout->addLayout( hbox );

	lbl   = new QLabel( tr("Address") );
	addr1 = new QLineEdit();

	hbox->addWidget( lbl );
	hbox->addWidget( addr1 );

	lbl   = new QLabel( tr("-") );
	addr2 = new QLineEdit();
	hbox->addWidget( lbl );
	hbox->addWidget( addr2 );

	forbidChkBox = new QCheckBox( tr("Forbid") );
	hbox->addWidget( forbidChkBox );

	frame = new QFrame();
	vbox  = new QVBoxLayout();
	hbox  = new QHBoxLayout();
	gbox  = new QGroupBox();

	rbp = new QCheckBox( tr("Read") );
	wbp = new QCheckBox( tr("Write") );
	xbp = new QCheckBox( tr("Execute") );
	ebp = new QCheckBox( tr("Enable") );

	gbox->setTitle( tr("Memory") );
	mainLayout->addWidget( frame );
	frame->setLayout( vbox );
	frame->setFrameShape( QFrame::Box );
	vbox->addLayout( hbox );
	vbox->addWidget( gbox );

	hbox->addWidget( rbp );
	hbox->addWidget( wbp );
	hbox->addWidget( xbp );
	hbox->addWidget( ebp );
	
	hbox         = new QHBoxLayout();
	cpu_radio    = new QRadioButton( tr("CPU") );
	ppu_radio    = new QRadioButton( tr("PPU") );
	oam_radio    = new QRadioButton( tr("OAM") );
	rom_radio    = new QRadioButton( tr("ROM") );
	cpu_radio->setChecked(true);

	gbox->setLayout( hbox );
	hbox->addWidget( cpu_radio );
	hbox->addWidget( ppu_radio );
	hbox->addWidget( oam_radio );
	hbox->addWidget( rom_radio );

	grid  = new QGridLayout();

	mainLayout->addLayout( grid );
	lbl   = new QLabel( tr("Condition") );
	cond  = new QLineEdit();

	grid->addWidget(  lbl, 0, 0 );
	grid->addWidget( cond, 0, 1 );

	lbl   = new QLabel( tr("Name") );
	name  = new QLineEdit();

	grid->addWidget(  lbl, 1, 0 );
	grid->addWidget( name, 1, 1 );

	hbox         = new QHBoxLayout();
	okButton     = new QPushButton( tr("OK") );
	cancelButton = new QPushButton( tr("Cancel") );

	mainLayout->addLayout( hbox );
	hbox->addWidget( cancelButton );
	hbox->addWidget(     okButton );

	connect(     okButton, SIGNAL(clicked(void)), &dialog, SLOT(accept(void)) );
	connect( cancelButton, SIGNAL(clicked(void)), &dialog, SLOT(reject(void)) );

	    okButton->setIcon( style()->standardIcon( QStyle::SP_DialogOkButton ) );
	cancelButton->setIcon( style()->standardIcon( QStyle::SP_DialogCancelButton ) );

	okButton->setDefault(true);

	if ( wp != NULL )
	{
		char stmp[256];

		if ( wp->flags & BT_P )
		{
			ppu_radio->setChecked(true);
		}
		else if ( wp->flags & BT_S )
		{
			oam_radio->setChecked(true);
		}
		else if ( wp->flags & BT_R )
		{
			rom_radio->setChecked(true);
		}

		sprintf( stmp, "%04X", wp->address );

		addr1->setText( tr(stmp) );

		if ( wp->endaddress > 0 )
		{
			sprintf( stmp, "%04X", wp->endaddress );

			addr2->setText( tr(stmp) );
		}

		if ( wp->flags & WP_R )
		{
		   rbp->setChecked(true);
		}
		if ( wp->flags & WP_W )
		{
		   wbp->setChecked(true);
		}
		if ( wp->flags & WP_X )
		{
		   xbp->setChecked(true);
		}
		if ( wp->flags & WP_F )
		{
		   forbidChkBox->setChecked(true);
		}
		if ( wp->flags & WP_E )
		{
		   ebp->setChecked(true);
		}

		if ( wp->condText )
		{
			cond->setText( tr(wp->condText) );
		}
		else
		{
			if ( editIdx < 0 )
			{
				// If new breakpoint, default enable checkbox to true
				ebp->setChecked(true);

				// If new breakpoint, suggest condition if in ROM Mapping area of memory.
				if ( cpu_radio->isChecked() && (wp->address >= 0x8000) )
				{
					int romAddr = GetNesFileAddress(wp->address);

					if ( romAddr >= 0 )
					{
						wp->address = romAddr;
						sprintf( stmp, "%X", wp->address );
						addr1->setText( tr(stmp) );
						rom_radio->setChecked(true);
					}
					else
					{
						char str[64];
						sprintf(str, "K==#%02X", getBank(wp->address));
						cond->setText( tr(str) );
					}
				}
			}
		}

		if ( wp->desc )
		{
			name->setText( tr(wp->desc) );
		}
	}
	else
	{
		// If new breakpoint, default enable checkbox to true
		ebp->setChecked(true);
	}

	dialog.setLayout( mainLayout );

	if ( forceAccept )
	{
		ret = QDialog::Accepted;
	}
	else
	{
		ret = dialog.exec();
	}

	if ( ret == QDialog::Accepted )
	{
		int  start_addr = -1, end_addr = -1, type = 0, enable = 1, slot;
		std::string s;

		slot = (editIdx < 0) ? numWPs : editIdx;

		if ( cpu_radio->isChecked() )
		{
			type |= BT_C;
		}
		else if ( ppu_radio->isChecked() ) 
		{
			type |= BT_P;
		}
		else if ( oam_radio->isChecked() ) 
		{
			type |= BT_S;
		}
		else if ( rom_radio->isChecked() )
		{
			type |= BT_R;
		}

		s = addr1->text().toStdString();

		if ( s.size() > 0 )
		{
			start_addr = offsetStringToInt( type, s.c_str() );
		}

		s = addr2->text().toStdString();

		if ( s.size() > 0 )
		{
			end_addr = offsetStringToInt( type, s.c_str() );
		}

		if ( rbp->isChecked() )
		{
			type |= WP_R;
		}
		if ( wbp->isChecked() )
		{
			type |= WP_W;
		}
		if ( xbp->isChecked() )
		{
			type |= WP_X;
		}

		if ( forbidChkBox->isChecked() )
		{
			type |= WP_F;
		}

		enable = ebp->isChecked();

		if ( (start_addr >= 0) && (numWPs < 64) )
		{
			unsigned int retval;
			std::string nameString, condString;

			nameString = name->text().toStdString();
			condString = cond->text().toStdString();

			retval = NewBreak( nameString.c_str(), start_addr, end_addr, type, condString.c_str(), slot, enable);

			if ( (retval == 1) || (retval == 2) )
			{
				printf("Breakpoint Add Failed\n");
			}
			else
			{
				if (editIdx < 0)
				{
					numWPs++;
				}

				bpListUpdate( false );
			}
		}
	}
}
//----------------------------------------------------------------------------
void ConsoleDebugger::openDebugSymbolEditWindow( int addr )
{
	int ret, bank;
	debugSymbol_t *sym;
	SymbolEditWindow win(this);

	if ( addr < 0x8000 )
	{
	   bank = -1;
	}
	else
	{
	   bank = getBank( addr );
	}

  	sym = debugSymbolTable.getSymbolAtBankOffset( bank, addr );

	win.setAddr( addr );

	win.setSym( sym );

	ret = win.exec();

	if ( ret == QDialog::Accepted )
	{
		fceuWrapperLock();
		asmView->updateAssemblyView();
		fceuWrapperUnLock();
	}
}
//----------------------------------------------------------------------------
void ConsoleDebugger::bpListUpdate( bool reset )
{
	QTreeWidgetItem *item;
	char cond[128], desc[128], addrStr[32], flags[16], enable;

	if ( reset )
	{
		bpTree->clear();
	}

	for (int i=0; i<numWPs; i++)
	{
		if ( bpTree->topLevelItemCount() > i )
		{
			item = bpTree->topLevelItem(i);
		}
		else
		{
			item = NULL;
		}

		if ( item == NULL )
		{
			item = new QTreeWidgetItem();

			bpTree->addTopLevelItem( item );
		}

		//item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsUserCheckable );
		item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemNeverHasChildren );

		if ( watchpoint[i].endaddress > 0 )
		{
			sprintf( addrStr, "$%04X-%04X:", watchpoint[i].address, watchpoint[i].endaddress );
		}
		else
		{
			sprintf( addrStr, "$%04X:", watchpoint[i].address );
		}

		flags[0] = (watchpoint[i].flags & WP_E) ? 'E' : '-';

		if ( watchpoint[i].flags & BT_P )
		{
			flags[1] = 'P';
		}
		else if ( watchpoint[i].flags & BT_S )
		{
			flags[1] = 'S';
		}
		else if ( watchpoint[i].flags & BT_R )
		{
			flags[1] = 'R';
		}
		else
		{
			flags[1] = 'C';
		}

		flags[2] = (watchpoint[i].flags & WP_R) ? 'R' : '-';
		flags[3] = (watchpoint[i].flags & WP_W) ? 'W' : '-';
		flags[4] = (watchpoint[i].flags & WP_X) ? 'X' : '-';
		flags[5] = (watchpoint[i].flags & WP_F) ? 'F' : '-';
		flags[6] = 0;

		enable = (watchpoint[i].flags & WP_E) ? 1 : 0;

		cond[0] = 0;
		desc[0] = 0;

		if (watchpoint[i].desc )
		{
			strcat( desc, watchpoint[i].desc);
		}

		if (watchpoint[i].condText )
		{
			strcat( cond, " (");
			strcat( cond, watchpoint[i].condText);
			strcat( cond, ") ");
		}

		item->setCheckState( 0, enable ? Qt::Checked : Qt::Unchecked );

		item->setFont( 0, font );
		item->setFont( 1, font );
		item->setFont( 2, font );
		item->setFont( 3, font );

		item->setText( 0, tr(addrStr));
		item->setText( 1, tr(flags)  );
		item->setText( 2, tr(cond)   );
		item->setText( 3, tr(desc)   );

		item->setTextAlignment( 0, Qt::AlignLeft);
		item->setTextAlignment( 1, Qt::AlignLeft);
		item->setTextAlignment( 2, Qt::AlignLeft);
		item->setTextAlignment( 3, Qt::AlignLeft);
	}

	bpTree->viewport()->update();
}
//----------------------------------------------------------------------------
void ConsoleDebugger::add_BM_CB(void)
{
	dbgBmMgr.addBookmark( selBmAddrVal );

	bmListUpdate(false);
}
//----------------------------------------------------------------------------
void ConsoleDebugger::edit_BM_CB(void)
{
	int addr;
	std::string s;
	QTreeWidgetItem *item;

	item = bmTree->currentItem();

	if ( item == NULL )
	{
		printf( "No Item Selected\n");
		return;
	}
	s = item->text(0).toStdString();

	addr = strtol( s.c_str(), NULL, 16 );

	edit_BM_name( addr );
	
}
//----------------------------------------------------------------------------
void ConsoleDebugger::delete_BM_CB(void)
{
	int addr;
	std::string s;
	QTreeWidgetItem *item;

	item = bmTree->currentItem();

	if ( item == NULL )
	{
		printf( "No Item Selected\n");
		return;
	}
	s = item->text(0).toStdString();

	addr = strtol( s.c_str(), NULL, 16 );

	dbgBmMgr.deleteBookmark( addr );

	bmListUpdate(true);

	saveGameDebugBreakpoints(true);
}
//----------------------------------------------------------------------------
void ConsoleDebugger::edit_BM_name( int addr )
{
	int ret;
	debuggerBookmark_t *bm;
	QInputDialog dialog(this);
	char stmp[128];

	bm = dbgBmMgr.getAddr( addr );

	sprintf( stmp, "Specify Bookmark Name for %04X", addr );

	dialog.setWindowTitle( tr("Edit Bookmark") );
	dialog.setLabelText( tr(stmp) );
	dialog.setOkButtonText( tr("Edit") );

	if ( bm != NULL )
	{
		dialog.setTextValue( tr(bm->name.c_str()) );
	}

	ret = dialog.exec();

	if ( QDialog::Accepted == ret )
	{
	     	bm->name = dialog.textValue().toStdString();
	     	bmListUpdate(false);
	}
}
//----------------------------------------------------------------------------
void ConsoleDebugger::bmListUpdate( bool reset )
{
	int i=0;
	QTreeWidgetItem *item;
	debuggerBookmark_t *bm;
	char addrStr[32];

	if ( reset )
	{
		bmTree->clear();
	}

	bm = dbgBmMgr.begin();

	while ( bm != NULL )
	{
		if ( bmTree->topLevelItemCount() > i )
		{
			item = bmTree->topLevelItem(i);
		}
		else
		{
			item = NULL;
		}

		if ( item == NULL )
		{
			item = new QTreeWidgetItem();

			bmTree->addTopLevelItem( item );
		}

		sprintf( addrStr, "%04X", bm->addr );

		//item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsUserCheckable );
		item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemNeverHasChildren );

		item->setFont( 0, font );
		item->setFont( 1, font );

		item->setText( 0, tr(addrStr));
		item->setText( 1, tr(bm->name.c_str()) );

		item->setTextAlignment( 0, Qt::AlignLeft);
		item->setTextAlignment( 1, Qt::AlignLeft);

		bm = dbgBmMgr.next(); i++;
	}

	bmTree->viewport()->update();
}
//----------------------------------------------------------------------------
void ConsoleDebugger::resizeToMinimumSizeHint(void) 
{
	resize( minimumSizeHint() );
}
//----------------------------------------------------------------------------
void ConsoleDebugger::add_BP_CB(void)
{
	openBpEditWindow(-1);

	asmView->determineLineBreakpoints();
}
//----------------------------------------------------------------------------
void ConsoleDebugger::edit_BP_CB(void)
{
	QTreeWidgetItem *item;

	item = bpTree->currentItem();

	if ( item == NULL )
	{
		printf( "No Item Selected\n");
		return;
	}

	int row = bpTree->indexOfTopLevelItem(item);

	openBpEditWindow( row, &watchpoint[row] );

	asmView->determineLineBreakpoints();
}
//----------------------------------------------------------------------------
static void DeleteBreak(int sel)
{
	if(sel<0) return;
	if(sel>=numWPs) return;

	fceuWrapperLock();

	if (watchpoint[sel].cond)
	{
		freeTree(watchpoint[sel].cond);
	}
	if (watchpoint[sel].condText)
	{
		free(watchpoint[sel].condText);
	}
	if (watchpoint[sel].desc)
	{
		free(watchpoint[sel].desc);
	}
	// move all BP items up in the list
	for (int i = sel; i < numWPs; i++) 
	{
		watchpoint[i].address = watchpoint[i+1].address;
		watchpoint[i].endaddress = watchpoint[i+1].endaddress;
		watchpoint[i].flags = watchpoint[i+1].flags;
// ################################## Start of SP CODE ###########################
		watchpoint[i].cond = watchpoint[i+1].cond;
		watchpoint[i].condText = watchpoint[i+1].condText;
		watchpoint[i].desc = watchpoint[i+1].desc;
// ################################## End of SP CODE ###########################
	}
	// erase last BP item
	watchpoint[numWPs].address = 0;
	watchpoint[numWPs].endaddress = 0;
	watchpoint[numWPs].flags = 0;
	watchpoint[numWPs].cond = 0;
	watchpoint[numWPs].condText = 0;
	watchpoint[numWPs].desc = 0;
	numWPs--;

	fceuWrapperUnLock();
}
//----------------------------------------------------------------------------
void debuggerClearAllBookmarks(void)
{
	dbgBmMgr.clear();
}
//----------------------------------------------------------------------------
void debuggerClearAllBreakpoints(void)
{
	int i;

	fceuWrapperLock();

	for (i=0; i<numWPs; i++)
	{
	   if (watchpoint[i].cond)
	   {
	   	freeTree(watchpoint[i].cond);
	   }
	   if (watchpoint[i].condText)
	   {
	   	free(watchpoint[i].condText);
	   }
	   if (watchpoint[i].desc)
	   {
	   	free(watchpoint[i].desc);
	   }

		watchpoint[i].address = 0;
	   watchpoint[i].endaddress = 0;
	   watchpoint[i].flags = 0;
	   watchpoint[i].cond = 0;
	   watchpoint[i].condText = 0;
	   watchpoint[i].desc = 0;
	}
	numWPs = 0;

	fceuWrapperUnLock();
}
//----------------------------------------------------------------------------
void ConsoleDebugger::delete_BP_CB(void)
{
	QTreeWidgetItem *item;

	item = bpTree->currentItem();

	if ( item == NULL )
	{
		printf( "No Item Selected\n");
		return;
	}

	int row = bpTree->indexOfTopLevelItem(item);

	DeleteBreak( row );
	delete item;
	//delete bpTree->takeTopLevelItem(row);
	//bpListUpdate( true );
	
	saveGameDebugBreakpoints(true);
	asmView->determineLineBreakpoints();
}
//----------------------------------------------------------------------------
void ConsoleDebugger::breakOnBadOpcodeCB(bool value)
{
	//printf("Value:%i\n", value);
	FCEUI_Debugger().badopbreak = value;

	g_config->setOption("SDL.DebuggerBreakOnBadOpcodes", value );
}
//----------------------------------------------------------------------------
void ConsoleDebugger::breakOnNewCodeCB(bool value)
{
	//printf("Code Value:%i\n", value);
	break_on_unlogged_code = value;

	g_config->setOption("SDL.DebuggerBreakOnUnloggedCode", value );
}
//----------------------------------------------------------------------------
void ConsoleDebugger::breakOnNewDataCB(bool value)
{
	//printf("Data Value:%i\n", value);
	break_on_unlogged_data = value;

	g_config->setOption("SDL.DebuggerBreakOnUnloggedData", value );
}
//----------------------------------------------------------------------------
void ConsoleDebugger::breakOnCyclesCB( bool value )
{
	std::string s;

	if ( value )
	{
		DebugBreakOnDialog *win = new DebugBreakOnDialog(0, this);
		win->exec();
	}
	else
	{
		break_on_cycles = value;
	}

	//s = cpuCycExdVal->text().toStdString();

   //printf("'%s'\n", txt );

	if ( s.size() > 0 )
	{
		break_cycles_limit = strtoul( s.c_str(), NULL, 10 );
	}
}
//----------------------------------------------------------------------------
void ConsoleDebugger::cpuCycleThresChanged(const QString &txt)
{
	std::string s;

	s = txt.toStdString();

	//printf("Cycles: '%s'\n", s.c_str() );

	if ( s.size() > 0 )
	{
		break_cycles_limit = strtoul( s.c_str(), NULL, 10 );
	}
}
//----------------------------------------------------------------------------
void ConsoleDebugger::breakOnInstructionsCB( bool value )
{
	std::string s;

	if ( value )
	{
		DebugBreakOnDialog *win = new DebugBreakOnDialog(1, this);
		win->exec();
	}
	else
	{
		break_on_instructions = value;
	}

	//s = instrExdVal->text().toStdString();

   //printf("'%s'\n", txt );

	if ( s.size() > 0 )
	{
		break_instructions_limit = strtoul( s.c_str(), NULL, 10 );
	}
}
//----------------------------------------------------------------------------
void ConsoleDebugger::instructionsThresChanged(const QString &txt)
{
	std::string s;

	s = txt.toStdString();

	//printf("Instructions: '%s'\n", s.c_str() );

	if ( s.size() > 0 )
	{
		break_instructions_limit = strtoul( s.c_str(), NULL, 10 );
	}
}
//----------------------------------------------------------------------------
void ConsoleDebugger::displayByteCodesCB( bool value )
{
	g_config->setOption( "SDL.AsmShowByteCodes", value );

	asmView->setDisplayByteCodes(value);
}
//----------------------------------------------------------------------------
void ConsoleDebugger::displayROMoffsetCB( bool value )
{
	g_config->setOption( "SDL.AsmShowRomOffsets", value );

	asmView->setDisplayROMoffsets(value);
}
//----------------------------------------------------------------------------
void ConsoleDebugger::symbolDebugEnableCB( bool value )
{
	g_config->setOption( "SDL.DebuggerShowSymNames", value );

	asmView->setSymbolDebugEnable(value);
}
//----------------------------------------------------------------------------
void ConsoleDebugger::registerNameEnableCB( bool value )
{
	g_config->setOption( "SDL.DebuggerShowRegNames", value );

	asmView->setRegisterNameEnable(value);
}
//----------------------------------------------------------------------------
void ConsoleDebugger::autoOpenDebugCB( bool value )
{
	g_config->setOption( "SDL.AutoOpenDebugger", value);
}
//----------------------------------------------------------------------------
void ConsoleDebugger::debFileAutoLoadCB( bool value )
{
	int autoLoadDebug = value;

	g_config->setOption("SDL.AutoLoadDebugFiles", autoLoadDebug);

	if ( autoLoadDebug && (numWPs == 0) )
	{
		loadGameDebugBreakpoints();
	}
}
//----------------------------------------------------------------------------
void ConsoleDebugger::changeAsmFontCB(void)
{
	bool ok = false;

	QFont selFont = QFontDialog::getFont( &ok, asmView->QWidget::font(), this, tr("Select Font"), QFontDialog::MonospacedFonts );

	if ( ok )
	{
		asmView->setFont( selFont );
		asmView->updateAssemblyView();

		//printf("Font Changed to: '%s'\n", font.toString().toStdString().c_str() );

		g_config->setOption("SDL.DebuggerAsmFont", selFont.toString().toStdString().c_str() );
	}
}
//----------------------------------------------------------------------------
void ConsoleDebugger::changeStackFontCB(void)
{
	bool ok = false;

	QFont selFont = QFontDialog::getFont( &ok, stackText->QWidget::font(), this, tr("Select Font"), QFontDialog::MonospacedFonts );

	if ( ok )
	{
		stackText->setFont( selFont );

		//printf("Font Changed to: '%s'\n", font.toString().toStdString().c_str() );

		g_config->setOption("SDL.DebuggerStackFont", selFont.toString().toStdString().c_str() );
	}
}
//----------------------------------------------------------------------------
void ConsoleDebugger::changeCpuFontCB(void)
{
	bool ok = false;

	QFont selFont = QFontDialog::getFont( &ok, pcEntry->QWidget::font(), this, tr("Select Font"), QFontDialog::MonospacedFonts );

	if ( ok )
	{
		cpuFont = selFont;
		setCpuStatusFont( selFont );
		setPpuStatusFont( selFont );

		//printf("Font Changed to: '%s'\n", font.toString().toStdString().c_str() );

		g_config->setOption("SDL.DebuggerCpuStatusFont", selFont.toString().toStdString().c_str() );
	}
}
//----------------------------------------------------------------------------
void ConsoleDebugger::reloadSymbolsCB(void)
{
	fceuWrapperLock();
	debugSymbolTable.loadGameSymbols();

	asmView->updateAssemblyView();
	fceuWrapperUnLock();
}
//----------------------------------------------------------------------------
void ConsoleDebugger::pcSetPlaceTop(void)
{
	asmView->setPC_placement( 0 );
}
//----------------------------------------------------------------------------
void ConsoleDebugger::pcSetPlaceUpperMid(void)
{
	asmView->setPC_placement( 1 );
}
//----------------------------------------------------------------------------
void ConsoleDebugger::pcSetPlaceCenter(void)
{
	asmView->setPC_placement( 2 );
}
//----------------------------------------------------------------------------
void ConsoleDebugger::pcSetPlaceLowerMid(void)
{
	asmView->setPC_placement( 3 );
}
//----------------------------------------------------------------------------
void ConsoleDebugger::pcSetPlaceBottom(void)
{
	asmView->setPC_placement( 4 );
}
//----------------------------------------------------------------------------
void ConsoleDebugger::pcSetPlaceCustom(void)
{
	int ret, ofs;
	QInputDialog dialog(this);

	g_config->getOption("SDL.DebuggerPCLineOffset" , &ofs );

	dialog.setWindowTitle( tr("PC Line Offset") );
	dialog.setLabelText( tr("Enter a line offset from 0 to 100.") );
	dialog.setOkButtonText( tr("Ok") );
	dialog.setInputMode( QInputDialog::IntInput );
	dialog.setIntRange( 0, 100 );
	dialog.setIntValue( ofs );
	
	ret = dialog.exec();
	
	if ( QDialog::Accepted == ret )
	{
	   ofs = dialog.intValue();
	
	     	asmView->setPC_placement( 5, ofs );
	}

}
//----------------------------------------------------------------------------
void ConsoleDebugger::debugRunCB(void)
{
	if (FCEUI_EmulationPaused()) 
	{
		setRegsFromEntry();
		FCEUI_ToggleEmulationPause();
		//DebuggerWasUpdated = false done in above function;
	}
}
//----------------------------------------------------------------------------
void ConsoleDebugger::debugStepIntoCB(void)
{
	if (FCEUI_EmulationPaused())
	{
		setRegsFromEntry();
	}
	FCEUI_Debugger().step = true;
	FCEUI_SetEmulationPaused(0);
}
//----------------------------------------------------------------------------
void ConsoleDebugger::debugStepOutCB(void)
{
	if (FCEUI_EmulationPaused() > 0) 
	{
		DebuggerState &dbgstate = FCEUI_Debugger();
		setRegsFromEntry();
		if (dbgstate.stepout)
		{
			int ret;
			QMessageBox msgBox(QMessageBox::Question, tr("Step Out Already Active"),
					tr("Step Out is currently in process. Cancel it and setup a new Step Out watch?"),
					QMessageBox::No | QMessageBox::Yes, this);

			ret = msgBox.exec();

			if ( ret != QMessageBox::Yes )
			{
				//printf("Step out cancelled\n");
				return;
			}
			//printf("Step out reset\n");
		}
		if (GetMem(X.PC) == 0x20)
		{
			dbgstate.jsrcount = 1;
		}
		else 
		{
			dbgstate.jsrcount = 0;
		}
		dbgstate.stepout = 1;
		FCEUI_SetEmulationPaused(0);
	}
}
//----------------------------------------------------------------------------
void ConsoleDebugger::debugStepOverCB(void)
{
	if (FCEUI_EmulationPaused()) 
	{
		setRegsFromEntry();
		int tmp=X.PC;
		uint8 opcode = GetMem(X.PC);
		bool jsr = opcode==0x20;
		bool call = jsr;
		#ifdef BRK_3BYTE_HACK
		//with this hack, treat BRK similar to JSR
		if(opcode == 0x00)
		{
			call = true;
		}
		#endif
		if (call) 
		{
			if (watchpoint[64].flags)
			{
				printf("Step Over is currently in process.\n");
				return;
			}
			watchpoint[64].address = (tmp+3);
			watchpoint[64].flags = WP_E|WP_X;
		}
		else 
		{
			FCEUI_Debugger().step = true;
		}
		FCEUI_SetEmulationPaused(0);
	}
}
//----------------------------------------------------------------------------
void ConsoleDebugger::debugRunToCursorCB(void)
{
	asmView->setBreakpointAtSelectedLine();
}
//----------------------------------------------------------------------------
void ConsoleDebugger::debugRunLineCB(void)
{
	if (FCEUI_EmulationPaused())
	{
		setRegsFromEntry();
	}
	uint64 ts=timestampbase;
	ts+=timestamp;
	ts+=341/3;
	//if (scanline == 240) vblankScanLines++;
	//else vblankScanLines = 0;
	FCEUI_Debugger().runline = true;
	FCEUI_Debugger().runline_end_time=ts;
	FCEUI_SetEmulationPaused(0);
}
//----------------------------------------------------------------------------
void ConsoleDebugger::debugRunLine128CB(void)
{
	if (FCEUI_EmulationPaused())
	{
		setRegsFromEntry();
	}
	FCEUI_Debugger().runline = true;
	{
		uint64 ts=timestampbase;
		ts+=timestamp;
		ts+=128*341/3;
		FCEUI_Debugger().runline_end_time=ts;
		//if (scanline+128 >= 240 && scanline+128 <= 257) vblankScanLines = (scanline+128)-240;
		//else vblankScanLines = 0;
	}
	FCEUI_SetEmulationPaused(0);
}
//----------------------------------------------------------------------------
//void ConsoleDebugger::seekToCB (void)
//{
//	std::string s;
//
//	s = seekEntry->displayText().toStdString();
//
//	//printf("Seek To: '%s'\n", s.c_str() );
//
//	if ( s.size() > 0 )
//	{
//		long int addr, line;
//
//		addr = strtol( s.c_str(), NULL, 16 );
//		
//		line = asmView->getAsmLineFromAddr(addr);
//
//		asmView->setLine( line );
//		vbar->setValue( line );
//	}
//}
//----------------------------------------------------------------------------
void ConsoleDebugger::navHistBackCB (void)
{
	asmView->navHistBack();
}
//----------------------------------------------------------------------------
void ConsoleDebugger::navHistForwardCB (void)
{
	asmView->navHistForward();
}
//----------------------------------------------------------------------------
void ConsoleDebugger::setLayoutOption( int opt )
{
	for (int i=0; i<2; i++)
	{
		for (int j=0; j<4; j++)
		{
			for (int k=0; k<tabView[i][j]->count(); k++)
			{
				tabView[i][j]->removeTab(k);
			}
		}
	}

	switch ( opt )
	{
		default:
		case 1:
		{
			tabView[0][0]->addTab( cpuFrame, tr("CPU") );
			tabView[0][0]->addTab( ppuFrame, tr("PPU") );
			tabView[0][0]->addTab(  bpFrame, tr("Breakpoints") );
			tabView[0][0]->addTab(  bmFrame, tr("BookMarks") );
			tabView[0][0]->setVisible(true);
		}
		break;
		case 2:
		{
			tabView[0][0]->addTab( cpuFrame, tr("CPU") );
			tabView[0][0]->addTab( ppuFrame, tr("PPU") );
			tabView[0][1]->addTab(  bpFrame, tr("Breakpoints") );
			tabView[0][1]->addTab(  bmFrame, tr("BookMarks") );

			tabView[0][0]->setVisible(true);
			tabView[0][1]->setVisible(true);

			QList <int> s = vsplitter[0]->sizes();

			s[0] = tabView[0][0]->minimumSizeHint().height();
			s[1] = vsplitter[0]->height() - s[0];
			s[2] = 0;
			s[3] = 0;

			vsplitter[0]->setSizes(s);
		}
		break;
		case 3:
		{
			tabView[0][0]->addTab( cpuFrame, tr("CPU") );
			tabView[0][1]->addTab( ppuFrame, tr("PPU") );
			tabView[1][0]->addTab(  bpFrame, tr("Breakpoints") );
			tabView[1][0]->addTab(  bmFrame, tr("BookMarks") );

			tabView[0][0]->setVisible(true);
			tabView[0][1]->setVisible(true);
			tabView[1][0]->setVisible(true);

			QList <int> s = vsplitter[0]->sizes();

			s[0] = vsplitter[0]->height() / 2;
			s[1] = s[0];
			s[2] = 0;
			s[3] = 0;

			vsplitter[0]->setSizes(s);

			s = mainLayouth->sizes();

			if ( s[2] == 0 )
			{
				s[0] = s[1] = s[2] = mainLayouth->width() / 3;
				mainLayouth->setSizes(s);
			}
		}
		break;
		case 4:
		{
			tabView[0][0]->addTab( cpuFrame, tr("CPU") );
			tabView[0][1]->addTab( ppuFrame, tr("PPU") );
			tabView[1][0]->addTab(  bpFrame, tr("Breakpoints") );
			tabView[1][1]->addTab(  bmFrame, tr("BookMarks") );

			tabView[0][0]->setVisible(true);
			tabView[0][1]->setVisible(true);
			tabView[1][0]->setVisible(true);
			tabView[1][1]->setVisible(true);

			QList <int> s = vsplitter[0]->sizes();

			s[0] = vsplitter[0]->height() / 2;
			s[1] = s[0];
			s[2] = 0;
			s[3] = 0;

			vsplitter[0]->setSizes(s);
			vsplitter[1]->setSizes(s);

			s = mainLayouth->sizes();

			if ( s[2] == 0 )
			{
				s[0] = s[1] = s[2] = mainLayouth->width() / 3;
				mainLayouth->setSizes(s);
			}
		}
		break;
	}

	updateTabVisibility();
}
//----------------------------------------------------------------------------
void ConsoleDebugger::seekPCCB (void)
{
	if (FCEUI_EmulationPaused())
	{
		setRegsFromEntry();
		//updateAllDebugWindows();
	}
	windowUpdateReq = true;
	//asmView->scrollToPC();
}
//----------------------------------------------------------------------------
void ConsoleDebugger::openChangePcDialog(void)
{
	int ret;
	QDialog dialog(this);
	QLabel *lbl;
	QSpinBox *sbox;
	QVBoxLayout *vbox;
	QHBoxLayout *hbox;
	QPushButton *okButton, *cancelButton;

	vbox = new QVBoxLayout();
	hbox = new QHBoxLayout();
	lbl  = new QLabel( tr("Specify Address [ 0x0000 -> 0xFFFF ]") );

	okButton     = new QPushButton( tr("Go") );
	cancelButton = new QPushButton( tr("Cancel") );

	okButton->setIcon( style()->standardIcon( QStyle::SP_DialogApplyButton ) );
	cancelButton->setIcon( style()->standardIcon( QStyle::SP_DialogCancelButton ) );

	connect(     okButton, SIGNAL(clicked(void)), &dialog, SLOT(accept(void)) );
	connect( cancelButton, SIGNAL(clicked(void)), &dialog, SLOT(reject(void)) );

	sbox = new QSpinBox();
	sbox->setRange(0x0000, 0xFFFF);
	sbox->setDisplayIntegerBase(16);
	sbox->setValue( X.PC );

	QFont font = sbox->font();
	font.setCapitalization(QFont::AllUppercase);
	sbox->setFont(font);

	hbox->addWidget( cancelButton );
	hbox->addWidget(     okButton );

	vbox->addWidget( lbl  );
	vbox->addWidget( sbox );
	vbox->addLayout( hbox );

	dialog.setLayout( vbox );

	dialog.setWindowTitle( tr("Change Program Counter") );

	okButton->setDefault(true);

	ret = dialog.exec();

	if ( QDialog::Accepted == ret )
	{
		X.PC = sbox->value();
	
		windowUpdateReq = true;
	}
}
//----------------------------------------------------------------------------
void ConsoleDebugger::openGotoAddrDialog(void)
{
	int ret;
	QDialog dialog(this);
	QLabel *lbl;
	QSpinBox *sbox;
	QVBoxLayout *vbox;
	QHBoxLayout *hbox;
	QPushButton *okButton, *cancelButton;

	vbox = new QVBoxLayout();
	hbox = new QHBoxLayout();
	lbl  = new QLabel( tr("Specify Address [ 0x0000 -> 0xFFFF ]") );

	okButton     = new QPushButton( tr("Go") );
	cancelButton = new QPushButton( tr("Cancel") );

	okButton->setIcon( style()->standardIcon( QStyle::SP_DialogApplyButton ) );
	cancelButton->setIcon( style()->standardIcon( QStyle::SP_DialogCancelButton ) );

	connect(     okButton, SIGNAL(clicked(void)), &dialog, SLOT(accept(void)) );
	connect( cancelButton, SIGNAL(clicked(void)), &dialog, SLOT(reject(void)) );

	sbox = new QSpinBox();
	sbox->setRange(0x0000, 0xFFFF);
	sbox->setDisplayIntegerBase(16);
	sbox->setValue( X.PC );

	QFont font = sbox->font();
	font.setCapitalization(QFont::AllUppercase);
	sbox->setFont(font);

	hbox->addWidget( cancelButton );
	hbox->addWidget(     okButton );

	vbox->addWidget( lbl  );
	vbox->addWidget( sbox );
	vbox->addLayout( hbox );

	dialog.setLayout( vbox );

	dialog.setWindowTitle( tr("Goto Address") );

	okButton->setDefault(true);

	ret = dialog.exec();

	if ( QDialog::Accepted == ret )
	{
		int addr;
	
		addr = sbox->value();
	
		asmView->gotoAddr(addr);
	}
}
//----------------------------------------------------------------------------
void ConsoleDebugger::resetCountersCB (void)
{
	ResetDebugStatisticsCounters();

	updateRegisterView();
}
//----------------------------------------------------------------------------
void ConsoleDebugger::asmViewCtxMenuRunToCursor(void)
{
	fceuWrapperLock();
	watchpoint[64].address = asmView->getCtxMenuAddr();
	watchpoint[64].flags   = WP_E|WP_X;

	FCEUI_SetEmulationPaused(0);
	fceuWrapperUnLock();
}
//----------------------------------------------------------------------------
void ConsoleDebugger::asmViewCtxMenuGoTo(void)
{
	asmView->gotoAddr( asmView->getCtxMenuAddr() );
}
//----------------------------------------------------------------------------
void ConsoleDebugger::asmViewCtxMenuAddBP(void)
{
	int bpNum;
	watchpointinfo wp;

	wp.address = asmView->getCtxMenuAddr();
	wp.endaddress = 0;
	wp.flags   = WP_X | WP_E;
	wp.condText = 0;
	wp.desc = NULL;

	if ( asmView->getCtxMenuAddrType() )
	{
		wp.flags |= BT_R;
	}

	bpNum = asmView->isBreakpointAtAddr( wp.address, GetNesFileAddress(wp.address) );

	if ( bpNum >= 0 )
	{
		openBpEditWindow( bpNum, &watchpoint[bpNum] );
	}
	else
	{
		openBpEditWindow( -1, &wp );
	}

	asmView->determineLineBreakpoints();
}
//----------------------------------------------------------------------------
void ConsoleDebugger::asmViewCtxMenuAddBM(void)
{
	int addr = asmView->getAsmAddrFromLine( asmView->getCtxMenuLine() );

	dbgBmMgr.addBookmark( addr );

	edit_BM_name( addr );

	bmListUpdate(false);
}
//----------------------------------------------------------------------------
void ConsoleDebugger::asmViewCtxMenuOpenHexEdit(void)
{
	int romAddr = -1;
	int addr = asmView->getCtxMenuAddr();

	if ( asmView->getCtxMenuAddrType() )
	{
		romAddr = addr;
	}
	else
	{
		if (addr >= 0x8000)
		{
			romAddr  = GetNesFileAddress(addr);
		}
	}

	if ( romAddr >= 0 )
	{
		hexEditorOpenFromDebugger( QHexEdit::MODE_NES_ROM, romAddr );
	}
	else
	{
		hexEditorOpenFromDebugger( QHexEdit::MODE_NES_RAM, addr );
	}
}
//----------------------------------------------------------------------------
void ConsoleDebugger::setBookmarkSelectedAddress( int addr )
{
	char stmp[32];

	sprintf( stmp, "%04X", addr );

	selBmAddr->setText( tr(stmp) );

	selBmAddrVal = addr;
}
//----------------------------------------------------------------------------
void ConsoleDebugger::asmViewCtxMenuAddSym(void)
{
	int addr = asmView->getAsmAddrFromLine( asmView->getCtxMenuLine() );

	openDebugSymbolEditWindow( addr );
}
//----------------------------------------------------------------------------
void QAsmView::setPC_placement( int mode, int ofs )
{
	pcLinePlacement = mode;

	if ( mode == 5 )
	{
		pcLineOffset = ofs;
	}

	g_config->setOption("SDL.DebuggerPCPlacement"  , pcLinePlacement);
	g_config->setOption("SDL.DebuggerPCLineOffset" , pcLineOffset   );
	g_config->save();
}
//----------------------------------------------------------------------------
void QAsmView::toggleBreakpoint(int line)
{
	if ( line < asmEntry.size() )
	{
		int bpNum = isBreakpointAtLine(line);

		if ( bpNum >= 0 )
		{
			DeleteBreak( bpNum );
		}
		else
		{
			watchpointinfo wp;

			wp.address = asmEntry[line]->addr;
			wp.endaddress = 0;
			wp.flags   = WP_X | WP_E;
			wp.condText = 0;
			wp.desc = NULL;

			dbgWin->openBpEditWindow( -1, &wp, true );
		}
		asmEntry[line]->bpNum = isBreakpointAtLine(line);
	}
}
//----------------------------------------------------------------------------
int QAsmView::isBreakpointAtAddr( int cpuAddr, int romAddr )
{
	for (int i=0; i<numWPs; i++)
	{
		if ( (watchpoint[i].flags & WP_X) == 0 )
		{
			continue;
		}
		if ( (watchpoint[i].flags & (BT_P|BT_S) ) != 0 )
		{
			continue;
		}
		if ( watchpoint[i].flags & BT_R )
		{
			if ( watchpoint[i].endaddress )
			{
				if ( (romAddr >= watchpoint[i].address) && 
					romAddr < watchpoint[i].endaddress )
				{
					return i;
				}
			}
			else
			{
				if (romAddr == watchpoint[i].address)
				{
					return i;
				}
			}
		}
		else
		{
			if ( watchpoint[i].endaddress )
			{
				if ( (cpuAddr >= watchpoint[i].address) && 
					cpuAddr < watchpoint[i].endaddress )
				{
					return i;
				}
			}
			else
			{
				if (cpuAddr == watchpoint[i].address)
				{
					return i;
				}
			}
		}
	}
	return -1;
}
//----------------------------------------------------------------------------
int QAsmView::isBreakpointAtLine( int l )
{
	if ( l < asmEntry.size() )
	{
		if ( asmEntry[l]->type == dbg_asm_entry_t::ASM_TEXT )
		{
			return isBreakpointAtAddr( asmEntry[l]->addr, asmEntry[l]->rom );
		}
	}
	return -1;
}
//----------------------------------------------------------------------------
void QAsmView::determineLineBreakpoints(void)
{
	for (size_t i=0; i<asmEntry.size(); i++)
	{
		asmEntry[i]->bpNum = isBreakpointAtLine(i);
	}
}
//----------------------------------------------------------------------------
void QAsmView::setBreakpointAtSelectedLine(void)
{
	int addr = -1;

	if ( (selAddrLine >= 0) && (selAddrLine < asmEntry.size()) )
	{
		if ( selAddrValue == asmEntry[ selAddrLine ]->addr )
		{
			addr = selAddrValue;
		}
	}

	if ( addr >= 0 )
	{
		fceuWrapperLock();
		watchpoint[64].address = addr;
		watchpoint[64].flags = WP_E|WP_X;
		
		FCEUI_SetEmulationPaused(0);
		fceuWrapperUnLock();
	}
}
//----------------------------------------------------------------------------
int  QAsmView::getAsmAddrFromLine(int line)
{
	if ( (line >= 0) && (line < asmEntry.size()) )
	{
		return asmEntry[line]->addr;
	}
	return -1;
}
//----------------------------------------------------------------------------
int  QAsmView::getAsmLineFromAddr(int addr)
{
	int  line = -1;
	int  incr, nextLine;
	int  run = 1;

	if ( asmEntry.size() <= 0 )
	{
		return -1;
	}
	incr = asmEntry.size() / 2;

	if ( addr < asmEntry[0]->addr )
	{
		return 0;
	}
	else if ( addr > asmEntry[ asmEntry.size() - 1 ]->addr )
	{
		return asmEntry.size() - 1;
	}

	if ( incr < 1 ) incr = 1;

	nextLine = line = incr;

	// algorithm to efficiently find line from address. Starts in middle of list and 
	// keeps dividing the list in 2 until it arrives at an answer.
	while ( run )
	{
		//printf("incr:%i   line:%i  addr:%04X   delta:%i\n", incr, line, asmEntry[line]->addr, addr - asmEntry[line]->addr);

		if ( incr == 1 )
		{
			if ( asmEntry[line]->addr < addr )
			{
				nextLine = line + 1;
				if ( asmEntry[nextLine]->addr > addr )
				{
					break;
				}
				line = nextLine;
			}
			else if ( asmEntry[line]->addr > addr )
			{
				nextLine = line - 1;
				if ( asmEntry[nextLine]->addr < addr )
				{
					break;
				}
				line = nextLine;
			}
			else 
			{
				nextLine = line - 1;

				if ( nextLine >= 0 )
				{
					while ( asmEntry[nextLine]->addr == asmEntry[line]->addr )
					{
						line = nextLine;
						nextLine--;
						if ( nextLine < 0 )
						{
							break;
						}
					}
				}
				run = 0; break;
			}
		} 
		else
		{
			incr = incr / 2; 
			if ( incr < 1 ) incr = 1;

			if ( asmEntry[line]->addr < addr )
			{
				nextLine = line + incr;
			}
			else if ( asmEntry[line]->addr > addr )
			{
				nextLine = line - incr;
			}
			else
			{
				nextLine = line - 1;

				if ( nextLine >= 0 )
				{
					while ( asmEntry[nextLine]->addr == asmEntry[line]->addr )
					{
						line = nextLine;
						nextLine--;
						if ( nextLine < 0 )
						{
							break;
						}
					}
				}
				run = 0; break;
			}
			line = nextLine;
		}
	}

	// Don't stop on an symbol name or comment line, search for next assembly line
	while ( (line < asmEntry.size()) && (asmEntry[line]->type != dbg_asm_entry_t::ASM_TEXT) )
	{
		line++;
	}

	//for (size_t i=0; i<asmEntry.size(); i++)
	//{
	//	if ( asmEntry[i]->addr >= addr )
	//	{
	//      line = i; break;
	//	}
	//}

	return line;
}
//----------------------------------------------------------------------------
// This function is for "smart" scrolling...
// it attempts to scroll up one line by a whole instruction
static int InstructionUp(int from)
{
	int i = std::min(16, from), j;

	while (i > 0)
	{
		j = i;
		while (j > 0)
		{
			if (GetMem(from - j) == 0x00)
				break;	// BRK usually signifies data
			if (opsize[GetMem(from - j)] == 0)
				break;	// invalid instruction!
			if (opsize[GetMem(from - j)] > j)
				break;	// instruction is too long!
			if (opsize[GetMem(from - j)] == j)
				return (from - j);	// instruction is just right! :D
			j -= opsize[GetMem(from - j)];
		}
		i--;
	}

	// if we get here, no suitable instruction was found
	if ((from >= 2) && (GetMem(from - 2) == 0x00))
		return (from - 2);	// if a BRK instruction is possible, use that
	if (from)
		return (from - 1);	// else, scroll up one byte
	return 0;	// of course, if we can't scroll up, just return 0!
}
//static int InstructionDown(int from)
//{
//	int tmp = opsize[GetMem(from)];
//	if ((tmp))
//		return from + tmp;
//	else
//		return from + 1;		// this is data or undefined instruction
//}
//----------------------------------------------------------------------------
void  QAsmView::updateAssemblyView(void)
{
	int starting_address, start_address_lp, addr, size;
	int instruction_addr, asmFlags = 0;
	std::string line;
	char chr[64];
	uint8 opcode[3];
	char asmTxt[256];
	dbg_asm_entry_t *a, *d;
	char pc_found = 0;

	start_address_lp = starting_address = X.PC;

	for (int i=0; i < 0xFFFF; i++)
	{
		//printf("%i: Start Address: 0x%04X \n", i, start_address_lp );

		starting_address = InstructionUp( start_address_lp );

		if ( starting_address == start_address_lp )
		{
			break;
		}
		if ( starting_address < 0 )
		{
			starting_address = start_address_lp;
			break;
		}	
		start_address_lp = starting_address;
	}

	maxLineLen = 0;

	asmClear();

	addr  = starting_address;
	asmPC = NULL;

	if ( symbolicDebugEnable )
	{
		asmFlags |= ASM_DEBUG_SYMS | ASM_DEBUG_REPLACE;

		if ( registerNameEnable )
		{
			asmFlags |= ASM_DEBUG_REGS;
		}
	}

	for (int i=0; i < 0xFFFF; i++)
	{
		line.clear();

		// PC pointer
		if (addr > 0xFFFF) break;

		a = new dbg_asm_entry_t;

		if (cdloggerdataSize)
		{
			uint8_t cdl_data;
			instruction_addr = GetNesFileAddress(addr) - 16;
			if ( (instruction_addr >= 0) && (instruction_addr < cdloggerdataSize) )
			{
				cdl_data = cdloggerdata[instruction_addr] & 3;
				if (cdl_data == 3)
				{
					line.append("cd ");	// both Code and Data
				}
				else if (cdl_data == 2)
				{
					line.append(" d ");	// Data
				}
				else if (cdl_data == 1)
				{
					line.append("c  ");	// Code
				}
				else
				{
					line.append("   ");	// not logged
				}
			}
			else
			{
				line.append("   ");	// cannot be logged
			}
		}

		instruction_addr = addr;

		if ( !pc_found )
		{
			if (addr > X.PC)
			{
				asmPC = a;
				line.append(">");
				pc_found = 1;
			}
			else if (addr == X.PC)
			{
				asmPC = a;
				line.append(">");
				pc_found = 1;
			} 
			else
			{
				line.append(" ");
			}
		}
		else 
		{
			line.append(" ");
		}
		a->addr = addr;

		if (addr >= 0x8000)
		{
			a->bank = getBank(addr);
			a->rom  = GetNesFileAddress(addr);

			if (displayROMoffsets && (a->rom != -1) )
			{
				sprintf(chr, " %06X: ", a->rom);
			} 
			else
			{
				sprintf(chr, "%02X:%04X: ", a->bank, addr);
			}
		} 
		else
		{
			sprintf(chr, "  :%04X: ", addr);
		}
		line.append(chr);

		a->size = size = opsize[GetMem(addr)];

		if (size == 0)
		{
			sprintf(chr, "%02X        UNDEFINED", GetMem(addr++));
			line.append(chr);
		}
		else
		{
			if ((addr + size) > 0xFFFF)
			{
				while (addr < 0xFFFF)
				{
					sprintf(chr, "%02X        OVERFLOW\n", GetMem(addr++));
					line.append(chr);
				}
				delete a;
				break;
			}
			for (int j = 0; j < size; j++)
			{
				sprintf(chr, "%02X ", opcode[j] = GetMem(addr++));
				if ( showByteCodes ) line.append(chr);
			}
			while (size < 3)
			{
				if ( showByteCodes ) line.append("   ");  //pad output to align ASM
				size++;
			}

			DisassembleWithDebug(addr, opcode, asmFlags, asmTxt, &a->sym);

			line.append( asmTxt );
		}
		for (int j=0; j<size; j++)
		{
			a->opcode[j] = opcode[j];
		}

		// special case: an RTS opcode
		if (GetMem(instruction_addr) == 0x60)
		{
			line.append(" -------------------------");
		}

		if ( symbolicDebugEnable )
		{
			debugSymbol_t *dbgSym;

			dbgSym = debugSymbolTable.getSymbolAtBankOffset( a->bank, a->addr );

			if ( dbgSym != NULL )
			{
				int i,j;
				const char *c;
				char stmp[256];
				//printf("Debug symbol Found at $%04X \n", dbgSym->ofs );

				if ( dbgSym->name.size() > 0 )
				{
					d = new dbg_asm_entry_t();

					*d = *a;
					d->type = dbg_asm_entry_t::SYMBOL_NAME;
					d->text.assign( "   " + dbgSym->name );
					d->text.append( ":");
					d->line = asmEntry.size();
					
					asmEntry.push_back(d);
				}

				i=0; j=0;
				c = dbgSym->comment.c_str();

				while ( c[i] != 0 )
				{
					if ( c[i] == '\n' )
					{
						if ( j > 0 )
						{
							stmp[j] = 0;

							d = new dbg_asm_entry_t();

							*d = *a;
							d->type = dbg_asm_entry_t::SYMBOL_COMMENT;
							d->text.assign( stmp );
							d->line = asmEntry.size();
							
							asmEntry.push_back(d);
						}
						i++; j=0;
					}
					else
					{
						if ( j == 0 )
						{
							while ( j < 3 )
							{
								stmp[j] = ' '; j++;
							}
							stmp[j] = ';'; j++;
							stmp[j] = ' '; j++;
						}
						stmp[j] = c[i]; j++; i++;
					}
				}
				stmp[j] = 0;

				if ( j > 0 )
				{
					d = new dbg_asm_entry_t();

					*d = *a;
					d->type = dbg_asm_entry_t::SYMBOL_COMMENT;
					d->text.assign( stmp );
					d->line = asmEntry.size();
				
					asmEntry.push_back(d);
				}
			}
		}

		a->text.assign( line );

		a->line = asmEntry.size();

		if ( maxLineLen < line.size() )
		{
			maxLineLen = line.size();
		}

		line.append("\n");

		asmEntry.push_back(a);
	}

	pxLineWidth = (maxLineLen+1) * pxCharWidth;

	if ( viewWidth >= pxLineWidth )
	{
		hbar->hide();
	}
	else
	{
		hbar->setPageStep( viewWidth );
		hbar->setMaximum( pxLineWidth - viewWidth );
		hbar->show();
	}
	//setMaximumWidth( pxLineWidth );

	vbar->setPageStep( (3*viewLines)/4 );
	vbar->setMaximum( asmEntry.size() );

	determineLineBreakpoints();
}
//----------------------------------------------------------------------------
void ConsoleDebugger::setRegsFromEntry(void)
{
	std::string s;
	long int i;

	s = pcEntry->displayText().toStdString();

	if ( s.size() > 0 )
	{
		i = strtol( s.c_str(), NULL, 16 );
	}
	else
	{
		i = 0;
	}
	X.PC = i;
	//printf("Set PC: '%s'  %04X\n", s.c_str(), X.PC );

	s = regAEntry->displayText().toStdString();

	if ( s.size() > 0 )
	{
		i = strtol( s.c_str(), NULL, 16 );
	}
	else
	{
		i = 0;
	}
	X.A  = i;
	//printf("Set A: '%s'  %02X\n", s.c_str(), X.A );

	s = regXEntry->displayText().toStdString();

	if ( s.size() > 0 )
	{
		i = strtol( s.c_str(), NULL, 16 );
	}
	else
	{
		i = 0;
	}
	X.X  = i;
	//printf("Set X: '%s'  %02X\n", s.c_str(), X.X );

	s = regYEntry->displayText().toStdString();

	if ( s.size() > 0 )
	{
		i = strtol( s.c_str(), NULL, 16 );
	}
	else
	{
		i = 0;
	}
	X.Y  = i;
	//printf("Set Y: '%s'  %02X\n", s.c_str(), X.Y );

	i=0;
	if ( N_chkbox->isChecked() )
	{
		i |= N_FLAG;
	}
	if ( V_chkbox->isChecked() )
	{
		i |= V_FLAG;
	}
	if ( U_chkbox->isChecked() )
	{
		i |= U_FLAG;
	}
	if ( B_chkbox->isChecked() )
	{
		i |= B_FLAG;
	}
	if ( D_chkbox->isChecked() )
	{
		i |= D_FLAG;
	}
	if ( I_chkbox->isChecked() )
	{
		i |= I_FLAG;
	}
	if ( Z_chkbox->isChecked() )
	{
		i |= Z_FLAG;
	}
	if ( C_chkbox->isChecked() )
	{
		i |= C_FLAG;
	}
	X.P = i;

}
//----------------------------------------------------------------------------
void  ConsoleDebugger::updateRegisterView(void)
{
	int stackPtr;
	char stmp[64];
	char str[32], str2[32];

	sprintf( stmp, "%04X", X.PC );

	pcEntry->setText( tr(stmp) );

	sprintf( stmp, "%02X", X.A );

	regAEntry->setText( tr(stmp) );

	sprintf( stmp, "%02X", X.X );

	regXEntry->setText( tr(stmp) );

	sprintf( stmp, "%02X", X.Y );

	regYEntry->setText( tr(stmp) );

	sprintf( stmp, "%02X", X.P );

	regPEntry->setText( tr(stmp) );

	N_chkbox->setChecked( (X.P & N_FLAG) ? true : false );
	V_chkbox->setChecked( (X.P & V_FLAG) ? true : false );
	U_chkbox->setChecked( (X.P & U_FLAG) ? true : false );
	B_chkbox->setChecked( (X.P & B_FLAG) ? true : false );
	D_chkbox->setChecked( (X.P & D_FLAG) ? true : false );
	I_chkbox->setChecked( (X.P & I_FLAG) ? true : false );
	Z_chkbox->setChecked( (X.P & Z_FLAG) ? true : false );
	C_chkbox->setChecked( (X.P & C_FLAG) ? true : false );

	stackPtr = X.S | 0x0100;

	sprintf( stmp, "Stack: $%04X", stackPtr );
	stackFrame->setTitle( tr(stmp) );
	stackText->updateText();

	// update counters
	int64 counter_value1 = timestampbase + (uint64)timestamp - total_cycles_base;
	int64 counter_value2 = timestampbase + (uint64)timestamp - delta_cycles_base;

	if (counter_value1 < 0)	// sanity check
	{
		ResetDebugStatisticsCounters();
		counter_value1 = 0;
	}
	if (counter_value2 < 0)	// sanity check
	{
		ResetDebugStatisticsCounters();
		counter_value2 = 0;
	}
	sprintf(stmp, "%10llu  (+%llu)", counter_value1, counter_value2);
	cpuCyclesVal->setText( tr(stmp) );

	sprintf(stmp, "%10llu  (+%llu)", total_instructions, delta_instructions);
	cpuInstrsVal->setText( tr(stmp) );

	// PPU Labels
	sprintf(stmp, "$%02X", PPU[0] );
	ppuCtrlReg->setText( tr(stmp) );

	sprintf(stmp, "$%02X", PPU[1] );
	ppuMaskReg->setText( tr(stmp) );

	sprintf(stmp, "$%02X", PPU[2] );
	ppuStatReg->setText( tr(stmp) );

	sprintf(stmp, "$%04X", (int)FCEUPPU_PeekAddress());
	ppuAddrDsp->setText( tr(stmp) );

	sprintf(stmp, "$%02X", PPU[3] );
	oamAddrDsp->setText( tr(stmp) );

	extern int linestartts;
	#define GETLASTPIXEL    (PAL?((timestamp*48-linestartts)/15) : ((timestamp*48-linestartts)/16) )
	
	int ppupixel = GETLASTPIXEL;

	if (ppupixel>341)	//maximum number of pixels per scanline
		ppupixel = 0;	//Currently pixel display is borked until Run 128 lines is clicked, this keeps garbage from displaying

	// If not in the 0-239 pixel range, make special cases for display
	if (scanline == 240 && vblankScanLines < (PAL?72:22))
	{
		if (!vblankScanLines)
		{
			// Idle scanline (240)
			sprintf(str, "%d", scanline);	// was "Idle %d"
		} else if (scanline + vblankScanLines == (PAL?311:261))
		{
			// Pre-render
			sprintf(str, "-1");	// was "Prerender -1"
		} else
		{
			// Vblank lines (241-260/310)
			sprintf(str, "%d", scanline + vblankScanLines);	// was "Vblank %d"
		}
		sprintf(str2, "%d", vblankPixel);
	} else
	{
		// Scanlines 0 - 239
		sprintf(str, "%d", scanline);
		sprintf(str2, "%d", ppupixel);
	}

	if(newppu)
	{
		sprintf(str ,"%d",newppu_get_scanline());
		sprintf(str2,"%d",newppu_get_dot());
	}

	sprintf( stmp, "%s", str );
	ppuScanLineDsp->setText( tr(stmp) );

	sprintf( stmp, "%s", str2 );
	ppuPixelDsp->setText( tr(stmp) );

	int ppuScrollPosX, ppuScrollPosY;
	ppu_getScroll( ppuScrollPosX, ppuScrollPosY);
	sprintf( stmp, "%i", ppuScrollPosX );
	ppuScrollX->setText( tr(stmp) );
	sprintf( stmp, "%i", ppuScrollPosY );
	ppuScrollY->setText( tr(stmp) );
}
//----------------------------------------------------------------------------
void ConsoleDebugger::updateWindowData(void)
{
	asmView->updateAssemblyView();
	
	asmView->scrollToPC();

	updateRegisterView();

	windowUpdateReq = false;
}
//----------------------------------------------------------------------------
void ConsoleDebugger::queueUpdate(void)
{
	windowUpdateReq = true;
}
//----------------------------------------------------------------------------
void ConsoleDebugger::updatePeriodic(void)
{
	bool buttonEnable;
	QTreeWidgetItem *item;

	//printf("Update Periodic\n");

	if ( windowUpdateReq )
	{
		fceuWrapperLock();
		updateWindowData();
		fceuWrapperUnLock();
	}
	asmView->update();

	if ( FCEUI_EmulationPaused() && !FCEUI_EmulationFrameStepped())
	{
		if ( waitingAtBp )
		{
			switch ( lastBpIdx )
			{
				case BREAK_TYPE_STEP:
					emuStatLbl->setText( tr(" Emulator Paused on Step") );
				break;
				case BREAK_TYPE_BADOP:
					emuStatLbl->setText( tr(" Emulator Paused on Bad Opcode") );
				break;
				case BREAK_TYPE_CYCLES_EXCEED:
					emuStatLbl->setText( tr(" Emulator Paused on Cycle Count Exceedance") );
				break;
				case BREAK_TYPE_INSTRUCTIONS_EXCEED:
					emuStatLbl->setText( tr(" Emulator Paused on Instruction Count Exceedance") );
				break;
				case BREAK_TYPE_LUA:
					emuStatLbl->setText( tr(" Emulator Paused on Lua Breakpoint") );
				break;
				case BREAK_TYPE_UNLOGGED_CODE:
					emuStatLbl->setText( tr(" Emulator Paused on Unlogged Code") );
				break;
				case BREAK_TYPE_UNLOGGED_DATA:
					emuStatLbl->setText( tr(" Emulator Paused on Unlogged Data") );
				break;
				default:
					if ( lastBpIdx >= 0 )
					{
						char stmp[128];
						sprintf( stmp, " Emulator Stopped / Paused at Breakpoint: %i", lastBpIdx );
						emuStatLbl->setText( tr(stmp) );
					}
					else
					{
						emuStatLbl->setText( tr(" Emulator Stopped / Paused at Breakpoint") );
					}
				break;
			}
		}
		else
		{
			emuStatLbl->setText( tr(" Emulator Stopped / Paused") );
		}
		emuStatLbl->setStyleSheet("background-color: red; color: white;");
	}
	else
	{
		emuStatLbl->setText( tr(" Emulator is Running") );
		emuStatLbl->setStyleSheet("background-color: green; color: white;");
	}

	if ( waitingAtBp && (lastBpIdx == BREAK_TYPE_CYCLES_EXCEED) )
	{
		cpuCyclesLbl1->setStyleSheet("background-color: blue; color: white;");
	}
	else
	{
		cpuCyclesLbl1->setStyleSheet(NULL);
	}
	brkOnCycleExcAct->setChecked( break_on_cycles );

	if ( waitingAtBp && (lastBpIdx == BREAK_TYPE_INSTRUCTIONS_EXCEED) )
	{
		cpuInstrsLbl1->setStyleSheet("background-color: blue; color: white;");
	}
	else
	{
		cpuInstrsLbl1->setStyleSheet(NULL);
	}
	brkOnInstrExcAct->setChecked( break_on_instructions );

	if ( bpTree->topLevelItemCount() != numWPs )
	{
		//printf("Breakpoint Tree Update\n");
		bpListUpdate( true );
	}

	if ( bmTree->topLevelItemCount() != dbgBmMgr.size() )
	{
		//printf("Bookmark Tree Update\n");
		bmListUpdate( true );
	}

	item = bpTree->currentItem();

	if ( item == NULL )
	{
		buttonEnable = false;
	}
	else
	{
		buttonEnable = true;
	}
	bpAddBtn->setEnabled(true);
	bpEditBtn->setEnabled(buttonEnable);
	bpDelBtn->setEnabled(buttonEnable);
}
//----------------------------------------------------------------------------
void ConsoleDebugger::breakPointNotify( int bpNum )
{
	if ( bpNum >= 0 )
	{
		// highlight bp_num item list

		if ( bpTree->topLevelItemCount() > 0 )
		{
			QTreeWidgetItem * item;

			item = bpTree->currentItem();

			if ( item != NULL )
			{
				item->setSelected(false);
			}

			item = bpTree->topLevelItem( bpNum );

			if ( item != NULL )
			{
				item->setSelected(true);
				bpTree->setCurrentItem( item );
			}
			bpTree->viewport()->update();
		}
	}
	else
	{
		if (bpNum == BREAK_TYPE_CYCLES_EXCEED)
		{
			// Label Coloring done in periodic update
		}
		else if (bpNum == BREAK_TYPE_INSTRUCTIONS_EXCEED)
		{
			// Label Coloring done in periodic update
		}
	}

	windowUpdateReq = true;
}
//----------------------------------------------------------------------------
void ConsoleDebugger::hbarChanged(int value)
{
	//printf("HBar Changed: %i\n", value);
	asmView->setXScroll( value );
	asmView->update();
}
//----------------------------------------------------------------------------
void ConsoleDebugger::vbarChanged(int value)
{
	//printf("VBar Changed: %i\n", value);
	asmView->setLine( value );
	asmView->update();
}
//----------------------------------------------------------------------------
void bpDebugSetEnable(bool val)
{
	bpDebugEnable = val;
}
//----------------------------------------------------------------------------
void FCEUD_DebugBreakpoint( int bpNum )
{
	std::list <ConsoleDebugger*>::iterator it;

	if ( !nes_shm->runEmulator || !bpDebugEnable )
	{
		return;
	}
	lastBpIdx   = bpNum;
	waitingAtBp = true;

	if (bpNum == BREAK_TYPE_CYCLES_EXCEED)
	{
		if ( breakOnCycleOneShot )
		{
			break_on_cycles = false;
		}
		else
		{
			if ( breakOnCycleMode )
			{
				long long int totalCount = timestampbase + (uint64)timestamp - total_cycles_base;

				if (totalCount < 0)	// sanity check
				{
					ResetDebugStatisticsCounters();
					totalCount = 0;
				}
				break_cycles_limit = totalCount + breakOnCycleRelVal;
				//printf("Increasing Cycles Limit by: %llu  to: %llu\n", breakOnCycleRelVal, break_cycles_limit );
			}
		}
	}
	else if (bpNum == BREAK_TYPE_INSTRUCTIONS_EXCEED)
	{
		if ( breakOnInstrOneShot )
		{
			break_on_instructions = false;
		}
		else
		{
			if ( breakOnInstrMode )
			{
				break_instructions_limit = total_instructions + breakOnInstrRelVal;
			}
		}
	}

	printf("Breakpoint Hit: %i \n", bpNum );

	fceuWrapperUnLock();

	if ( dbgWin )
	{
		dbgWin->breakPointNotify( bpNum );
	}

	while ( nes_shm->runEmulator && bpDebugEnable &&
			FCEUI_EmulationPaused() && !FCEUI_EmulationFrameStepped())
	{
		// HACK: break when Frame Advance is pressed
		extern bool frameAdvanceRequested;
		extern int frameAdvance_Delay_count, frameAdvance_Delay;

		if (frameAdvanceRequested)
		{
			if ( (frameAdvance_Delay_count == 0) || (frameAdvance_Delay_count >= frameAdvance_Delay) )
			{
				FCEUI_SetEmulationPaused(EMULATIONPAUSED_FA);
			}
			if (frameAdvance_Delay_count < frameAdvance_Delay)
			{
				frameAdvance_Delay_count++;
			}
		}
		msleep(16);
	}
	// since we unfreezed emulation, reset delta_cycles counter
	ResetDebugStatisticsDeltaCounters();

	fceuWrapperLock();

	waitingAtBp = false;
}
//----------------------------------------------------------------------------
bool debuggerWindowIsOpen(void)
{
	return dbgWin != NULL;
}
//----------------------------------------------------------------------------
void debuggerWindowSetFocus(bool val)
{
	if ( dbgWin )
	{
		dbgWin->activateWindow();
		dbgWin->raise();
		dbgWin->setFocus();
	}
}
//----------------------------------------------------------------------------
bool debuggerWaitingAtBreakpoint(void)
{
	return waitingAtBp;
}
//----------------------------------------------------------------------------
void updateAllDebuggerWindows( void )
{
	std::list <ConsoleDebugger*>::iterator it;

	if ( dbgWin )
	{
		dbgWin->queueUpdate();
	}
}
//----------------------------------------------------------------------------
static int getGameDebugBreakpointFileName(char *filepath)
{
	int i,j;
	const char *romFile;

	romFile = getRomFile();

	if ( romFile == NULL )
	{
		return -1;
	}
	i=0; j = -1;
	while ( romFile[i] != 0 )
	{

		if ( romFile[i] == '|' )
		{
			filepath[i] = '.';
		}
		else
		{
			if ( romFile[i] == '/' )
			{
				j = -1;
			}
			else if ( romFile[i] == '.' )
			{
				j = i;
			}
			filepath[i] = romFile[i];
		}
		i++;
	}
	if ( j >= 0 )
	{
		filepath[j] = 0; i=j;
	}

	filepath[i] = '.'; i++;
	filepath[i] = 'd'; i++;
	filepath[i] = 'b'; i++;
	filepath[i] = 'g'; i++;
	filepath[i] =  0;

	return 0;
}
//----------------------------------------------------------------------------
void saveGameDebugBreakpoints( bool force )
{
	int i;
	FILE *fp;
	char stmp[512];
	char flags[8];
	debuggerBookmark_t *bm;

	// If no breakpoints are loaded, skip saving
	if ( !force && (numWPs == 0) && (dbgBmMgr.size() == 0) )
	{
		return;
	}
	if ( getGameDebugBreakpointFileName( stmp ) )
	{
		printf("Error: Failed to get save file name for debug\n");
		return;
	}

	printf("Debug Save File: '%s' \n", stmp );

	fp = fopen( stmp, "w");

	if ( fp == NULL )
	{
		printf("Error: Failed to open file '%s' for writing\n", stmp );
		return;
	}

	for (i=0; i<numWPs; i++)
	{
		flags[0] = (watchpoint[i].flags & WP_E) ? 'E' : '-';

		if ( watchpoint[i].flags & BT_P )
		{
			flags[1] = 'P';
		}
		else if ( watchpoint[i].flags & BT_S )
		{
			flags[1] = 'S';
		}
		else if ( watchpoint[i].flags & BT_R )
		{
			flags[1] = 'R';
		}
		else
		{
			flags[1] = 'C';
		}

		flags[2] = (watchpoint[i].flags & WP_R) ? 'R' : '-';
		flags[3] = (watchpoint[i].flags & WP_W) ? 'W' : '-';
		flags[4] = (watchpoint[i].flags & WP_X) ? 'X' : '-';
		flags[5] = (watchpoint[i].flags & WP_F) ? 'F' : '-';
		flags[6] = 0;

		fprintf( fp, "BreakPoint: startAddr=%08X  endAddr=%08X  flags=%s  condition=\"%s\"  desc=\"%s\" \n",
			  	watchpoint[i].address, watchpoint[i].endaddress, flags,
			  		(watchpoint[i].condText != NULL) ? watchpoint[i].condText : "",
			  		(watchpoint[i].desc != NULL) ? watchpoint[i].desc : "");
	}

	bm = dbgBmMgr.begin();

	while ( bm != NULL )
	{
		fprintf( fp, "Bookmark: addr=%04X  desc=\"%s\" \n", bm->addr, bm->name.c_str() );

		bm = dbgBmMgr.next();
	}

	fclose(fp);

	return;
}
//----------------------------------------------------------------------------
static int getKeyValuePair( int i, const char *stmp, char *id, char *data )
{
   int j=0;
   char literal=0;

   id[0] = 0; data[0] = 0;

   if ( stmp[i] == 0 ) return i;

	while ( isspace(stmp[i]) ) i++;

	j=0;
	while ( isalnum(stmp[i]) )
	{
		id[j] = stmp[i]; j++; i++;
	}
	id[j] = 0;

	if ( j == 0 )
	{
	  	return i;
	}
	if ( stmp[i] != '=' )
	{
	  	return i;
	}
	i++; j=0;
	if ( stmp[i] == '\"' )
	{
		literal = 0;
		i++;
		while ( stmp[i] != 0 )
		{
			if ( literal )
			{
				data[j] = stmp[i]; i++; j++;
				literal = 0;
			}
			else
			{
				if ( stmp[i] == '\\' )
				{
					literal = 1; i++;
				}
				else if ( stmp[i] == '\"' )
				{
					i++; break;
				}
				else
				{
					data[j] = stmp[i]; j++; i++;
				}
			}
		}
		data[j] = 0;
	}
	else
	{
		j=0;
		while ( !isspace(stmp[i]) )
		{
			data[j] = stmp[i]; j++; i++;
		}
		data[j] = 0;
	}
   return i;
}
//----------------------------------------------------------------------------
void loadGameDebugBreakpoints(void)
{
	int i,j;
	FILE *fp;
	char stmp[512];
	char id[64], data[128];

	// If no debug windows are open, skip loading breakpoints
	if ( dbgWin == NULL )
	{
		printf("No Debug Windows Open: Skipping loading of breakpoint data\n");
		return;
	}
	if ( getGameDebugBreakpointFileName( stmp ) )
	{
		printf("Error: Failed to get load file name for debug\n");
		return;
	}

	//printf("Debug Load File: '%s' \n", stmp );

	fp = fopen( stmp, "r");

	if ( fp == NULL )
	{
		printf("Error: Failed to open file '%s' for writing\n", stmp );
		return;
	}

	while ( fgets( stmp, sizeof(stmp), fp ) != 0 )
	{
		i=0; j=0;

		while ( isspace(stmp[i]) ) i++;

		while ( isalnum(stmp[i]) )
		{
			id[j] = stmp[i]; j++; i++;
		}
		id[j] = 0;

		while ( isspace(stmp[i]) ) i++;

		if ( stmp[i] != ':' )
		{
			continue;
		}
		i++;
		while ( isspace(stmp[i]) ) i++;

		if ( strcmp( id, "BreakPoint" ) == 0 )
		{
			int retval;
			int start_addr = -1, end_addr = -1, type = 0, enable = 0;
			char cond[256], desc[256];

			cond[0] = 0; desc[0] = 0;

			while ( stmp[i] != 0 )
			{
				i = getKeyValuePair( i, stmp, id, data );

				//printf("ID:'%s'  DATA:'%s' \n", id, data );

				if ( strcmp( id, "startAddr" ) == 0 )
				{
					start_addr = strtol( data, NULL, 16 );
				}
				else if ( strcmp( id, "endAddr" ) == 0 )
				{
					end_addr = strtol( data, NULL, 16 );
				}
				else if ( strcmp( id, "flags" ) == 0 )
				{
					type = 0;
					enable = (data[0] == 'E');

					if ( data[1] == 'P' )
					{
						type |= BT_P;
					}
					else if ( data[1] == 'S' )
					{
						type |= BT_S;
					}
					else if ( data[1] == 'R' )
					{
						type |= BT_R;
					}
					else 
					{
						type |= BT_C;
					}

					type |= (data[2] == 'R') ? WP_R : 0;
					type |= (data[3] == 'W') ? WP_W : 0;
					type |= (data[4] == 'X') ? WP_X : 0;
					type |= (data[5] == 'F') ? WP_F : 0;
				}
				else if ( strcmp( id, "condition" ) == 0 )
				{
					strcpy( cond, data );
				}
				else if ( strcmp( id, "desc" ) == 0 )
				{
					strcpy( desc, data );
				}
			}

			if ( (start_addr >= 0) && (numWPs < 64) )
			{
				retval = NewBreak( desc, start_addr, end_addr, type, cond, numWPs, enable);

				if ( (retval == 1) || (retval == 2) )
				{
					printf("Breakpoint Add Failed\n");
				}
				else
				{
					numWPs++;
				}
			}
		}
		else if ( strcmp( id, "Bookmark" ) == 0 )
		{
			int addr = -1;
			char desc[256];

			desc[0] = 0;

			while ( stmp[i] != 0 )
			{
				i = getKeyValuePair( i, stmp, id, data );

				//printf("ID:'%s'  DATA:'%s' \n", id, data );

				if ( strcmp( id, "addr" ) == 0 )
				{
					addr = strtol( data, NULL, 16 );
				}
				else if ( strcmp( id, "desc" ) == 0 )
				{
					strcpy( desc, data );
				}
			}

			if ( addr >= 0 )
			{
				if ( dbgBmMgr.addBookmark( addr, desc ) )
				{
					printf("Error:Failed to add debug bookmark: $%04X  '%s' \n", addr, desc );
				}
			}
		}
	}
	fclose(fp);

	return;
}
//----------------------------------------------------------------------------
QAsmView::QAsmView(QWidget *parent)
	: QWidget( parent )
{
	QPalette pal;
	QColor fg("black"), bg("white");
	QColor c;
	std::string fontString;

	viewWidth  = 512;
	viewHeight = 512;

	useDarkTheme = false;

	pcBgColor.setRgb( 255, 255, 0 );
	opcodeColor.setRgb( 46, 139, 87 );
	labelColor.setRgb( 165,  42, 42 );
	commentColor.setRgb( 0, 0, 255 );
	addressColor.setRgb( 106, 90, 205 );
	immediateColor.setRgb( 255, 1, 255 );

	fceuLoadConfigColor( "SDL.AsmSyntaxColorOpcode"   , &opcodeColor );
	fceuLoadConfigColor( "SDL.AsmSyntaxColorAddress"  , &addressColor );
	fceuLoadConfigColor( "SDL.AsmSyntaxColorImmediate", &immediateColor );
	fceuLoadConfigColor( "SDL.AsmSyntaxColorLabel"    , &labelColor );
	fceuLoadConfigColor( "SDL.AsmSyntaxColorComment"  , &commentColor );
	fceuLoadConfigColor( "SDL.AsmSyntaxColorPC"       , &pcBgColor );

	g_config->getOption("SDL.DebuggerAsmFont", &fontString);

	if ( fontString.size() > 0 )
	{
		font.fromString( QString::fromStdString( fontString ) );
	}
	else
	{
		font.setFamily("Courier New");
		font.setStyle( QFont::StyleNormal );
		font.setStyleHint( QFont::Monospace );
	}

	pal = this->palette();

	//c = pal.color(QPalette::Base);
	//printf("Base: R:%i  G:%i  B:%i \n", c.red(), c.green(), c.blue() );

	//c = pal.color(QPalette::Window);
	//printf("BackGround: R:%i  G:%i  B:%i \n", c.red(), c.green(), c.blue() );

	// Figure out if we are using a light or dark theme by checking the 
	// default window text grayscale color. If more white, then we will
	// use white text on black background, else we do the opposite.
	c = pal.color(QPalette::WindowText);

	if ( qGray( c.red(), c.green(), c.blue() ) > 128 )
	{
		useDarkTheme = true;
	}
	//printf("WindowText: R:%i  G:%i  B:%i \n", c.red(), c.green(), c.blue() );

	if ( useDarkTheme )
	{
		pal.setColor(QPalette::Base      , fg );
		pal.setColor(QPalette::Window    , fg );
		pal.setColor(QPalette::WindowText, bg );
	}
	else 
	{
		pal.setColor(QPalette::Base      , bg );
		pal.setColor(QPalette::Window    , bg );
		pal.setColor(QPalette::WindowText, fg );
	}

	//this->parent = (ConsoleDebugger*)parent;
	this->parent = qobject_cast <ConsoleDebugger*>( parent );
	this->setPalette(pal);
	this->setMouseTracking(true);

	isPopUp = false;
	showByteCodes = false;
	displayROMoffsets = false;
	symbolicDebugEnable = true;
	registerNameEnable = true;

	g_config->getOption( "SDL.AsmShowByteCodes" , &showByteCodes );
	g_config->getOption( "SDL.AsmShowRomOffsets", &displayROMoffsets );
	g_config->getOption( "SDL.DebuggerShowSymNames", &symbolicDebugEnable );
	g_config->getOption( "SDL.DebuggerShowRegNames", &registerNameEnable );

	calcFontData();

	vbar = NULL;
	hbar = NULL;
	asmPC = NULL;
	maxLineLen = 0;
	pxLineWidth = 0;
	lineOffset = 0;
	maxLineOffset = 0;
	ctxMenuAddr = -1;
	ctxMenuAddrType = 0;
	ctxMenuLine = 0;
	cursorPosX = 0;
	cursorPosY = 0;

	mouseLeftBtnDown  = false;
	txtHlgtAnchorLine = -1;
	txtHlgtAnchorChar = -1;
	txtHlgtStartChar  = -1;
	txtHlgtStartLine  = -1;
	txtHlgtEndChar    = -1;
	txtHlgtEndLine    = -1;

	pcLinePlacement = 0;
	pcLineOffset    = 0;

	g_config->getOption( "SDL.DebuggerPCPlacement" , &pcLinePlacement );
	g_config->getOption( "SDL.DebuggerPCLineOffset", &pcLineOffset    );

	selAddrLine  = -1;
	selAddrChar  =  0;
	selAddrWidth =  0;
	selAddrValue = -1;
	selAddrType  =  0;
	memset( selAddrText, 0, sizeof(selAddrText) );

	cursorLineAddr    = -1;
	wheelPixelCounter =  0;

	//setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Expanding );
	setFocusPolicy(Qt::StrongFocus);

	clipboard = QGuiApplication::clipboard();

	//printf("clipboard->supportsSelection() : '%i' \n", clipboard->supportsSelection() );
	//printf("clipboard->supportsFindBuffer(): '%i' \n", clipboard->supportsFindBuffer() );

	calcLineOffsets();
}
//----------------------------------------------------------------------------
QAsmView::~QAsmView(void)
{
	asmClear();
}
//----------------------------------------------------------------------------
void QAsmView::setIsPopUp(bool val)
{
	isPopUp = val;
}
//----------------------------------------------------------------------------
void QAsmView::asmClear(void)
{
	for (size_t i=0; i<asmEntry.size(); i++)
	{
		delete asmEntry[i];
	}
	asmEntry.clear();
}
//----------------------------------------------------------------------------
void QAsmView::calcLineOffsets(void)
{
	pcLocLinePos    = 4;
	byteCodeLinePos = pcLocLinePos + 8; // 12;
	opcodeLinePos   = byteCodeLinePos + (showByteCodes ? 10 : 1); //22;
	operandLinePos  = opcodeLinePos + 3; // 25;
}
//----------------------------------------------------------------------------
void QAsmView::setLine(int lineNum)
{
	lineOffset = lineNum;
}
//----------------------------------------------------------------------------
void QAsmView::setXScroll(int value)
{
	if ( viewWidth >= pxLineWidth )
	{
		pxLineXScroll = 0;
	}
	else
	{
		pxLineXScroll = value;
	}
}
//----------------------------------------------------------------------------
void QAsmView::scrollToPC(void)
{
	if ( asmPC != NULL )
	{
		curNavLoc.addr = asmPC->addr;

		scrollToLine( asmPC->line );
	}
}
//----------------------------------------------------------------------------
void QAsmView::scrollToAddr( int addr )
{
	int line = getAsmLineFromAddr(addr);

	curNavLoc.addr = asmPC->addr;

	scrollToLine( line );
}
//----------------------------------------------------------------------------
void QAsmView::scrollToLine( int line )
{
	int ofs = 0;
	int maxOfs = (viewLines-3);

	if ( maxOfs < 0 )
	{
		maxOfs = 0;
	}

	switch ( pcLinePlacement )
	{
		default:
		case 0:
			ofs = 0;
		break;
		case 1:
			ofs = (viewLines / 4);
		break;
		case 2:
			ofs = (viewLines / 2);
		break;
		case 3:
			ofs = (viewLines*3) / 4;
		break;
		case 4:
			ofs =  maxOfs;
		break;
		case 5:
			ofs = pcLineOffset;

			if ( ofs < 0 )
			{
				ofs = 0;
			}
			else if ( ofs > maxOfs )
			{
				ofs = maxOfs;
			}
		break;
	}

	lineOffset = line - ofs;

	if ( lineOffset < 0 )
	{
		lineOffset = 0;
	}
	vbar->setValue( lineOffset );
}
//----------------------------------------------------------------------------
void QAsmView::gotoPC(void)
{
	if ( asmPC != NULL )
	{
		gotoLine( asmPC->line );
	}
}
//----------------------------------------------------------------------------
void QAsmView::gotoAddr( int addr )
{
	int line = getAsmLineFromAddr(addr);

	gotoLine( line );
}
//----------------------------------------------------------------------------
void QAsmView::gotoLine( int line )
{
	if ( (line >= 0) && (line < asmEntry.size()) )
	{
		if ( curNavLoc.addr != asmEntry[line]->addr )
		{	// Don't push back to back duplicates into the navigation history
			pushAddrHist();
		}
		curNavLoc.addr = asmEntry[line]->addr;

		scrollToLine( line );

		setSelAddrToLine(line);
	}
}
//----------------------------------------------------------------------------
void QAsmView::pushAddrHist(void)
{
	if ( navBckHist.size() > 0)
	{
		if ( navBckHist.back().addr == curNavLoc.addr )
		{
			//printf("Address is Same\n");
			return;
		}
	}
	navBckHist.push_back(curNavLoc);

	navFwdHist.clear();

	//printf("Push Address: $%04X   %zi\n", curNavLoc.addr, navBckHist.size() );
}
//----------------------------------------------------------------------------
void QAsmView::navHistBack(void)
{
	if ( navBckHist.size() > 0)
	{
		navFwdHist.push_back(curNavLoc);

		curNavLoc = navBckHist.back();

		navBckHist.pop_back();

		int line = getAsmLineFromAddr( curNavLoc.addr );

		scrollToLine( line );
		setSelAddrToLine( line );
	}
}
//----------------------------------------------------------------------------
void QAsmView::navHistForward(void)
{
	if ( navFwdHist.size() > 0 )
	{
		navBckHist.push_back(curNavLoc);

		curNavLoc = navFwdHist.back();

		navFwdHist.pop_back();

		int line = getAsmLineFromAddr( curNavLoc.addr );

		scrollToLine( line );
		setSelAddrToLine( line );
	}
}
//----------------------------------------------------------------------------
void QAsmView::setSelAddrToLine( int line )
{
	if ( (line >= 0) && (line < asmEntry.size()) )
	{
		int addr = asmEntry[line]->addr;
		selAddrLine  = line;
		selAddrChar  = pcLocLinePos + 3;
		selAddrWidth = 4;
		selAddrValue = addr;
		selAddrType  =  0;
		sprintf( selAddrText, "%04X", addr );


		if ( parent )
		{
			parent->setBookmarkSelectedAddress( addr );
		}

		//printf("Selected ADDR:  $%04X   '%s'  '%s'\n", addr, selAddrText, asmEntry[line]->text.c_str() );

		txtHlgtAnchorLine = -1;
		txtHlgtAnchorChar = -1;
		txtHlgtStartChar  = -1;
		txtHlgtStartLine  = -1;
		txtHlgtEndChar    = -1;
		txtHlgtEndLine    = -1;
	}
}
//----------------------------------------------------------------------------
void QAsmView::setDisplayByteCodes( bool value )
{
	if ( value != showByteCodes )
	{
		showByteCodes = value;

		calcLineOffsets();
		calcMinimumWidth();

		fceuWrapperLock();
		updateAssemblyView();
		fceuWrapperUnLock();
	}
}
//----------------------------------------------------------------------------
void QAsmView::setDisplayROMoffsets( bool value )
{
	if ( value != displayROMoffsets )
	{
		displayROMoffsets = value;

		fceuWrapperLock();
		updateAssemblyView();
		fceuWrapperUnLock();
	}
}
//----------------------------------------------------------------------------
void QAsmView::setSymbolDebugEnable( bool value )
{
	if ( value != symbolicDebugEnable )
	{
		symbolicDebugEnable = value;

		fceuWrapperLock();
		updateAssemblyView();
		fceuWrapperUnLock();
	}
}
//----------------------------------------------------------------------------
void QAsmView::setRegisterNameEnable( bool value )
{
	if ( value != registerNameEnable )
	{
		registerNameEnable = value;

		fceuWrapperLock();
		updateAssemblyView();
		fceuWrapperUnLock();
	}
}
//----------------------------------------------------------------------------
void QAsmView::setFont( const QFont &newFont )
{
	font = newFont;

	QWidget::setFont(newFont);

	calcFontData();
}
//----------------------------------------------------------------------------
void QAsmView::calcFontData(void)
{
	QWidget::setFont(font);
	QFontMetrics metrics(font);
#if QT_VERSION > QT_VERSION_CHECK(5, 11, 0)
	pxCharWidth = metrics.horizontalAdvance(QLatin1Char('2'));
#else
	pxCharWidth = metrics.width(QLatin1Char('2'));
#endif
	pxCharHeight   = metrics.capHeight();
	pxLineSpacing  = metrics.lineSpacing() * 1.25;
	pxLineLead     = pxLineSpacing - metrics.height();
	pxCursorHeight = metrics.height();

	//printf("W:%i  H:%i  LS:%i  \n", pxCharWidth, pxCharHeight, pxLineSpacing );

	viewLines   = (viewHeight / pxLineSpacing) + 1;

	calcMinimumWidth();
}
//----------------------------------------------------------------------------
void QAsmView::calcMinimumWidth(void)
{
	if ( showByteCodes )
	{
		setMinimumWidth( 50 * pxCharWidth );
	}
	else
	{
		setMinimumWidth( 41 * pxCharWidth );
	}
}
//----------------------------------------------------------------------------
void QAsmView::setScrollBars( QScrollBar *h, QScrollBar *v )
{
	hbar = h; vbar = v;
}
//----------------------------------------------------------------------------
bool QAsmView::event(QEvent *event)
{
	if (event->type() == QEvent::ToolTip)
	{
		int i,j, line, addr = 0;
		QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
		bool opcodeValid = false, showOpcodeDesc = false, showSymHexDecode = false;
		bool showAddrDesc = false, showOperandAddrDesc = false;
		char stmp[256];

		//printf("QEvent::ToolTip\n");

		QPoint c = convPixToCursor(helpEvent->pos());

		line = lineOffset + c.y();

		opcodeValid = (line < asmEntry.size()) && (asmEntry[line]->size > 0) &&
				(asmEntry[line]->type == dbg_asm_entry_t::ASM_TEXT);

		showOpcodeDesc = (c.x() >= opcodeLinePos) && (c.x() < operandLinePos) && opcodeValid;

		showAddrDesc = (c.x() >= pcLocLinePos) && (c.x() < byteCodeLinePos) && opcodeValid;

		if ( (c.x() > operandLinePos) && opcodeValid && (asmEntry[line]->sym.name.size() > 0) )
		{
			size_t subStrLoc = asmEntry[line]->text.find( asmEntry[line]->sym.name, operandLinePos );

			if ( (subStrLoc != std::string::npos) && (subStrLoc > operandLinePos) )
			{
				//printf("Line:%i asmEntry DB Sym: %zi  '%s'\n", line, subStrLoc, asmEntry[line]->sym.name.c_str() );
				int symTextStart = subStrLoc;
				int symTextEnd   = subStrLoc + asmEntry[line]->sym.name.size();

				if ( (c.x() >= symTextStart) && (c.x() < symTextEnd) )
				{
					showSymHexDecode = true;
				}
			}
		}

		if ( opcodeValid && (c.x() > operandLinePos) &&
				(c.x() < asmEntry[line]->text.size()) )
		{
			i = c.x();

			if ( isxdigit( asmEntry[line]->text[i] ) )
			{
				int addrClicked = 1;
				int addrTextLoc = i;

				while ( isxdigit( asmEntry[line]->text[i] ) )
				{
					addrTextLoc = i;
					i--;
				}
				if ( asmEntry[line]->text[i] == '$' || asmEntry[line]->text[i] == ':' )
				{
					i--;
				}
				else
				{
					addrClicked = 0;
				}
				if ( asmEntry[line]->text[i] == '#' )
				{
					addrClicked = 0;
				}
				if ( addrClicked )
				{
					j=0; i = addrTextLoc;
					
					while ( isxdigit( asmEntry[line]->text[i] ) )
					{
						stmp[j] = asmEntry[line]->text[i]; i++; j++;
					}
					stmp[j] = 0;

					//printf("Addr: '%s'\n", stmp );

					addr = strtol( stmp, NULL, 16 );

					showOperandAddrDesc = true;
				}
			}
		}

		if ( showOpcodeDesc )
		{
			QString qs = fceuGetOpcodeToolTip(asmEntry[line]->opcode, asmEntry[line]->size );

			//QToolTip::setFont(font);
			QToolTip::showText(helpEvent->globalPos(), qs, this );
		}
		else if ( showSymHexDecode )
		{
			sprintf( stmp, "$%04X", asmEntry[line]->sym.ofs );

			QToolTip::showText(helpEvent->globalPos(), tr(stmp), this );
		}
		else if ( showAddrDesc )
		{
			if ( asmEntry[line]->bank < 0 )
			{
				sprintf( stmp, "ADDR:\t$%04X", asmEntry[line]->addr );
			}
			else
			{
				sprintf( stmp, "ADDR:\t$%04X\nBANK:\t$%02X\nROM:\t$%06X", 
					asmEntry[line]->addr, asmEntry[line]->bank, asmEntry[line]->rom );
			}

			//static_cast<asmLookAheadPopup*>(fceuCustomToolTipShow( helpEvent, new asmLookAheadPopup(asmEntry[line]->addr, this) ));
			QToolTip::showText(helpEvent->globalPos(), tr(stmp), this );
			//QToolTip::hideText();
			//event->ignore();
		}
		else if ( showOperandAddrDesc )
		{
			int bank = -1, romOfs = -1;

			if (addr >= 0x8000)
			{
				bank   = getBank(addr);
				romOfs = GetNesFileAddress(addr);
			}

			if ( bank < 0 )
			{
				sprintf( stmp, "ADDR:\t$%04X", addr );
			}
			else
			{
				sprintf( stmp, "ADDR:\t$%04X\nBANK:\t$%02X\nROM:\t$%06X", 
					addr, bank, romOfs );
			}

			static_cast<asmLookAheadPopup*>(fceuCustomToolTipShow( helpEvent, new asmLookAheadPopup(addr, this) ));
			//QToolTip::showText(helpEvent->globalPos(), tr(stmp), this );
			QToolTip::hideText();
			event->ignore();
		}
		else
		{
			//printf("Tool Tip Hide\n");
			QToolTip::hideText();
			event->ignore();
		}
		return true;
	}
	//else if (event->type() == QEvent::Leave)
	//{
	//	printf("QEvent::Leave\n");
	//}
	return QWidget::event(event);
}
//----------------------------------------------------------------------------
void QAsmView::resizeEvent(QResizeEvent *event)
{
	viewWidth  = event->size().width();
	viewHeight = event->size().height();

	//printf("QAsmView Resize: %ix%i  $%04X\n", viewWidth, viewHeight );

	viewLines = (viewHeight / pxLineSpacing) + 1;

	maxLineOffset = asmEntry.size() - viewLines + 1;

	if ( maxLineOffset < 0 )
	{
		maxLineOffset = 0;
	}

	if ( viewWidth >= pxLineWidth )
	{
		pxLineXScroll = 0;
		hbar->hide();
	}
	else
	{
		hbar->setPageStep( viewWidth );
		hbar->setMaximum( pxLineWidth - viewWidth );
		hbar->show();
		pxLineXScroll = hbar->value();
	}
	vbar->setPageStep( (3*viewLines)/4 );
}
//----------------------------------------------------------------------------
void QAsmView::keyPressEvent(QKeyEvent *event)
{
	//printf("Debug ASM Window Key Press: 0x%x \n", event->key() );
	if (event->matches(QKeySequence::MoveToPreviousLine))
	{
		lineOffset--;

		if ( lineOffset < 0 )
		{
			lineOffset = 0;
		}
		vbar->setValue( lineOffset );
		event->accept();
	}
	else if (event->matches(QKeySequence::MoveToNextLine))
	{
		lineOffset++;

		if ( lineOffset > maxLineOffset )
		{
			lineOffset = maxLineOffset;
		}
		vbar->setValue( lineOffset );
		event->accept();
	}
	else if (event->matches(QKeySequence::MoveToNextPage))
	{
		lineOffset += ( (3 * viewLines) / 4);
	
		if ( lineOffset >= maxLineOffset )
		{
			lineOffset = maxLineOffset;
		}
		vbar->setValue( lineOffset );
		event->accept();
	}
	else if (event->matches(QKeySequence::MoveToPreviousPage))
	{
		lineOffset -= ( (3 * viewLines) / 4);
		
		if ( lineOffset < 0 )
		{
			lineOffset = 0;
		}
		vbar->setValue( lineOffset );
		event->accept();
	}
	else if ( selAddrValue >= 0 )
	{
		ctxMenuAddr = selAddrValue;
		ctxMenuAddrType = selAddrType;
		ctxMenuLine = selAddrLine;

		if ( event->key() == Qt::Key_B )
		{
			if ( parent )
			{
				parent->asmViewCtxMenuAddBP();
			}
			event->accept();
		}
		else if ( event->key() == Qt::Key_S )
		{
			if ( parent )
			{
				parent->asmViewCtxMenuAddSym();
			}
			event->accept();
		}
		else if ( event->key() == Qt::Key_M )
		{
			if ( parent )
			{
				parent->asmViewCtxMenuAddBM();
			}
			event->accept();
		}
		else if ( event->key() == Qt::Key_H )
		{
			if ( parent )
			{
				parent->asmViewCtxMenuOpenHexEdit();
			}
			event->accept();
		}
	}
}
//----------------------------------------------------------------------------
void QAsmView::keyReleaseEvent(QKeyEvent *event)
{
   //printf("Debug ASM Window Key Release: 0x%x \n", event->key() );
}
//----------------------------------------------------------------------------
QPoint QAsmView::convPixToCursor( QPoint p )
{
	QPoint c(0,0);

	if ( p.x() < 0 )
	{
		c.setX(0);
	}
	else
	{
		float x = (float)(p.x() + pxLineXScroll) / pxCharWidth;

		c.setX( (int)x );
	}

	if ( p.y() < 0 )
	{
		c.setY( 0 );
	}
	else 
	{
		float ly = ( (float)pxLineLead / (float)pxLineSpacing );
		float py = ( (float)p.y() ) /  (float)pxLineSpacing;
		float ry = fmod( py, 1.0 );

		if ( ry < ly )
		{
			c.setY( ((int)py) - 1 );
		}
		else
		{
			c.setY( (int)py );
		}
	}
	return c;
}
//----------------------------------------------------------------------------
bool QAsmView::textIsHighlighted(void)
{
	bool set = false;

	if ( txtHlgtStartLine == txtHlgtEndLine )
	{
		set = (txtHlgtStartChar != txtHlgtEndChar);
	}
	else
	{
		set = true;
	}
	return set;
}
//----------------------------------------------------------------------------
void QAsmView::setHighlightEndCoord( int x, int y )
{

	if ( txtHlgtAnchorLine < y )
	{
		txtHlgtStartLine = txtHlgtAnchorLine;
		txtHlgtStartChar = txtHlgtAnchorChar;
		txtHlgtEndLine   = y;
		txtHlgtEndChar   = x;
	}
	else if ( txtHlgtAnchorLine > y )
	{
		txtHlgtStartLine = y;
		txtHlgtStartChar = x;
		txtHlgtEndLine   = txtHlgtAnchorLine;
		txtHlgtEndChar   = txtHlgtAnchorChar;
	}
	else
	{
		txtHlgtStartLine = txtHlgtAnchorLine;
		txtHlgtEndLine   = txtHlgtAnchorLine;

		if ( txtHlgtAnchorChar < x )
		{
			txtHlgtStartChar = txtHlgtAnchorChar;
			txtHlgtEndChar   = x;
		}
		else if ( txtHlgtAnchorChar > x )
		{
			txtHlgtStartChar = x;
			txtHlgtEndChar   = txtHlgtAnchorChar;
		}
		else
		{
			txtHlgtStartChar = txtHlgtAnchorChar;
			txtHlgtEndChar   = txtHlgtAnchorChar;
		}
	}
	return;
}
//----------------------------------------------------------------------------
void QAsmView::mouseMoveEvent(QMouseEvent * event)
{
	int line;
	char txt[256];
	std::string s;

	QPoint c = convPixToCursor( event->pos() );

	line = lineOffset + c.y();

	if ( mouseLeftBtnDown )
	{
		//printf("Left Button Move: (%i,%i)\n", c.x(), c.y() );
		setHighlightEndCoord( c.x(), line );
	}

	//printf("c (%i,%i) : Line %i : %04X \n", c.x(), c.y(), line, asmEntry[line]->addr );

	if ( line < asmEntry.size() )
	{
		int addr;

		cursorLineAddr = addr = asmEntry[line]->addr;

		if (addr >= 0x8000)
		{
			int bank, romOfs;
			bank   = getBank(addr);
			romOfs = GetNesFileAddress(addr);

			sprintf( txt, "CPU Address: %02X:%04X", bank, addr);

			s.assign( txt );

			if (romOfs != -1)
			{
				const char *fileName;

				fileName = iNesShortFName();

				if ( fileName == NULL )
				{
					fileName = "...";
				}
				sprintf( txt, "\nOffset 0x%06X in File \"%s\" (NL file: %X)", romOfs, fileName, bank);

				s.append( txt );
			}
		}
		else
		{
			sprintf( txt, "CPU Address: %04X", addr);

			s.assign( txt );
		}
	}

	if ( parent != NULL )
	{
		parent->asmLineSelLbl->setText( tr(s.c_str()) );
	}
}
//----------------------------------------------------------------------------
void QAsmView::loadClipboard( const char *txt )
{
	clipboard->setText( tr(txt), QClipboard::Clipboard );

	if ( clipboard->supportsSelection() )
	{
		clipboard->setText( tr(txt), QClipboard::Selection );
	}
}
//----------------------------------------------------------------------------
void QAsmView::loadHighlightToClipboard(void)
{
	if ( !textIsHighlighted() )
	{
		return;
	}
	int l, row, nrow;
	std::string txt;

	nrow = (viewHeight / pxLineSpacing) + 1;

	if ( nrow < 1 ) nrow = 1;

	for (row=0; row < nrow; row++)
	{
		l = lineOffset + row;

		if ( (l >= txtHlgtStartLine) && (l <= txtHlgtEndLine) )
		{
			int hlgtXs, hlgtXe, hlgtXd;
			std::string s;
			bool addNewLine;

			if ( l == txtHlgtStartLine )
			{
				hlgtXs = txtHlgtStartChar;
			}
			else
			{
				hlgtXs = 0;
			}

			if ( l == txtHlgtEndLine )
			{
				hlgtXe = txtHlgtEndChar;
				addNewLine = false;
			}
			else
			{
				hlgtXe = (viewWidth / pxCharWidth) + 1;
				addNewLine = true;
			}
			hlgtXd = (hlgtXe - hlgtXs);

			if ( hlgtXs < asmEntry[l]->text.size() )
			{
				s = asmEntry[l]->text.substr( hlgtXs, hlgtXd );
			}
			txt.append(s);

			if ( addNewLine )
			{
				txt.append("\n");
			}
		}
	}

	//printf("Load Text to Clipboard:\n%s\n", txt.c_str() );

	loadClipboard( txt.c_str() );

}
//----------------------------------------------------------------------------
void QAsmView::mouseReleaseEvent(QMouseEvent * event)
{
	int line;
	QPoint c = convPixToCursor( event->pos() );

	line = lineOffset + c.y();

	if ( event->button() == Qt::LeftButton )
	{
		//printf("Left Button Release: (%i,%i)\n", c.x(), c.y() );

		if ( mouseLeftBtnDown )
		{
			mouseLeftBtnDown = false;
			setHighlightEndCoord( c.x(), line );

			loadHighlightToClipboard();
		}
	}
}
//----------------------------------------------------------------------------
void QAsmView::mouseDoubleClickEvent(QMouseEvent * event)
{
	//printf("Double Click\n");

	if ( (selAddrType == 0) && (selAddrValue >= 0) )
	{
		gotoAddr( selAddrValue );

		mouseLeftBtnDown = false;
		txtHlgtAnchorLine = -1;
		txtHlgtAnchorChar = -1;
		txtHlgtStartChar  = -1;
		txtHlgtStartLine  = -1;
		txtHlgtEndChar    = -1;
		txtHlgtEndLine    = -1;
	}
}
//----------------------------------------------------------------------------
void QAsmView::mousePressEvent(QMouseEvent * event)
{
	int line;
	QPoint c = convPixToCursor( event->pos() );

	line = lineOffset + c.y();

	//printf("Mouse Button Pressed: 0x%x (%i,%i)\n", event->button(), c.x(), c.y() );
	
	if ( event->button() == Qt::LeftButton )
	{
		//printf("Left Button Pressed: (%i,%i)\n", c.x(), c.y() );
		mouseLeftBtnDown = true;
		txtHlgtAnchorChar = c.x();
		txtHlgtAnchorLine = line;

		setHighlightEndCoord( c.x(), line );

		if ( (c.x() >= 2) && (c.x() <= 3) )
		{
			//printf("Toggle BP!\n");
			toggleBreakpoint(line);
		}

	}
	
	selAddrLine  = -1;
	selAddrChar  =  0;
	selAddrWidth =  0;
	selAddrValue = -1;
	selAddrType  =  0;
	selAddrText[0] = 0;

	if ( line < asmEntry.size() )
	{
		int i,j, addr = -1, addrTextLoc = -1, selChar;
		int symTextStart = -1, symTextEnd = -1;
		char addrClicked = 0;
		char stmp[64];

		selChar = c.x();

		if ( asmEntry[line]->type == dbg_asm_entry_t::ASM_TEXT )
		{
			if ( selChar < (int)asmEntry[line]->text.size() )
			{
				i = selChar;

				if ( asmEntry[line]->sym.name.size() > 0 )
				{
					size_t subStrLoc = asmEntry[line]->text.find( asmEntry[line]->sym.name, operandLinePos );

					if ( (subStrLoc != std::string::npos) && (subStrLoc > operandLinePos) )
					{
						//printf("Line:%i asmEntry DB Sym: %zi  '%s'\n", line, subStrLoc, asmEntry[line]->sym.name.c_str() );
						symTextStart = subStrLoc;
						symTextEnd   = subStrLoc + asmEntry[line]->sym.name.size();
					}
				}

				if ( (i >= symTextStart) && (i < symTextEnd) )
				{
					selAddrLine  = line;
					selAddrChar  = symTextStart;
					selAddrWidth = symTextEnd - symTextStart;
					selAddrValue = addr = asmEntry[line]->sym.ofs;
					selAddrType  = 0;

					if ( selAddrWidth >= (int)sizeof(selAddrText) )
					{
						selAddrWidth = sizeof(selAddrText)-1;
					}
					strncpy( selAddrText, asmEntry[line]->sym.name.c_str(), selAddrWidth );
					selAddrText[ selAddrWidth ] = 0;
				}
				else if ( isxdigit( asmEntry[line]->text[i] ) )
				{
					addrClicked = 1;
					addrTextLoc = i;

					while ( isxdigit( asmEntry[line]->text[i] ) )
					{
						addrTextLoc = i;
						i--;
					}
					if ( asmEntry[line]->text[i] == '$' || asmEntry[line]->text[i] == ':' || asmEntry[line]->text[i] == ' ' )
					{
						i--;
					}
					else
					{
						addrClicked = 0;
					}
					if ( asmEntry[line]->text[i] == '#' )
					{
						addrClicked = 0;
					}
					if ( addrClicked )
					{
						j=0; i = addrTextLoc;
						
						while ( isxdigit( asmEntry[line]->text[i] ) )
						{
							stmp[j] = asmEntry[line]->text[i]; i++; j++;
						}
						stmp[j] = 0;

						//printf("Addr: '%s'\n", stmp );

						addr = strtol( stmp, NULL, 16 );

						selAddrLine  = line;
						selAddrChar  = addrTextLoc;
						selAddrWidth = j;
						selAddrValue = addr;
						selAddrType  = displayROMoffsets && (addrTextLoc < byteCodeLinePos) && (asmEntry[line]->rom >= 0);
						strcpy( selAddrText, stmp );
					}
				}
			}
		}
		else if ( asmEntry[line]->type == dbg_asm_entry_t::SYMBOL_NAME )
		{
			selAddrLine  = line;
			selAddrChar  = 3;
			selAddrValue = addr = asmEntry[line]->addr;

			i=selAddrChar; j=0;
			while ( asmEntry[line]->text[i] != 0 )
			{
				if ( j >= (int)sizeof(selAddrText) )
				{
					j=sizeof(selAddrText)-1;
					break;
				}
				selAddrText[j] = asmEntry[line]->text[i]; i++; j++;
			}
			selAddrText[j] = 0;
			selAddrWidth = j-1;
			selAddrText[ selAddrWidth ] = 0;
		}

		if ( addr < 0 )
		{
			addr = asmEntry[line]->addr;
			selAddrType = 0;

			if ( displayROMoffsets )
			{
				if ( asmEntry[line]->rom >= 0 )
				{
					addr = asmEntry[line]->rom;
					selAddrType = 1;
				}
			}

			if ( selAddrType )
			{
				sprintf( selAddrText, "%06X", addr );
				selAddrWidth = 6;
				selAddrChar  = pcLocLinePos+1;
			}
			else
			{
				sprintf( selAddrText, "%04X", addr );
				selAddrWidth = 4;
				selAddrChar  = pcLocLinePos+3;
			}
			selAddrLine  = line;
			selAddrValue = addr;
		}
		//printf("Line: '%s'\n", asmEntry[line]->text.c_str() );

		if ( addr >= 0 )
		{
			if ( parent )
			{
				parent->setBookmarkSelectedAddress( addr );
			}
		}

		if ( selAddrText[0] != 0 )
		{
			loadClipboard( selAddrText );
		}
	}
}
//----------------------------------------------------------------------------
void QAsmView::wheelEvent(QWheelEvent *event)
{

	QPoint numPixels = event->pixelDelta();
	QPoint numDegrees = event->angleDelta();

	if (!numPixels.isNull()) 
	{
		wheelPixelCounter -= numPixels.y();
	   //printf("numPixels: (%i,%i) \n", numPixels.x(), numPixels.y() );
	} 
	else if (!numDegrees.isNull()) 
	{
		//QPoint numSteps = numDegrees / 15;
		//printf("numSteps: (%i,%i) \n", numSteps.x(), numSteps.y() );
		//printf("numDegrees: (%i,%i)  %i\n", numDegrees.x(), numDegrees.y(), pxLineSpacing );
		wheelPixelCounter -= (pxLineSpacing * numDegrees.y()) / (15*8);
	}
	//printf("Wheel Event: %i\n", wheelPixelCounter);

	if ( wheelPixelCounter >= pxLineSpacing )
	{
		lineOffset += (wheelPixelCounter / pxLineSpacing);

		if ( lineOffset > maxLineOffset )
		{
			lineOffset = maxLineOffset;
		}
		vbar->setValue( lineOffset );

		wheelPixelCounter = wheelPixelCounter % pxLineSpacing;
	}
	else if ( wheelPixelCounter <= -pxLineSpacing )
	{
		lineOffset += (wheelPixelCounter / pxLineSpacing);

		if ( lineOffset < 0 )
		{
			lineOffset = 0;
		}
		vbar->setValue( lineOffset );

		wheelPixelCounter = wheelPixelCounter % pxLineSpacing;
	}

	 event->accept();
}
//----------------------------------------------------------------------------
void QAsmView::contextMenuEvent(QContextMenuEvent *event)
{
	int line;
	QAction *act;
	QMenu menu(this);
	QPoint c = convPixToCursor( event->pos() );
	bool enableRunToCursor = false;
	char stmp[128];

	if ( parent == NULL )
	{
		return;
	}
	line = lineOffset + c.y();

	ctxMenuAddr = -1;

	if ( line < asmEntry.size() )
	{
		int addr, romAddr;

		if ( selAddrValue < 0 )
		{
			ctxMenuAddr = addr = asmEntry[line]->addr;
			ctxMenuAddrType = 0;
			ctxMenuLine = line;;

			enableRunToCursor = true;
		}
		else
		{
			ctxMenuAddr = addr = selAddrValue;
			ctxMenuAddrType = selAddrType;
			ctxMenuLine = selAddrLine;

			enableRunToCursor = (selAddrValue == asmEntry[line]->addr);
		}
		romAddr = GetNesFileAddress(addr);

		if ( enableRunToCursor )
		{
			act = new QAction(tr("Run To &Cursor"), &menu);
			menu.addAction(act);
			//act->setShortcut( QKeySequence(tr("Ctrl+F10")));
			connect( act, SIGNAL(triggered(void)), parent, SLOT(asmViewCtxMenuRunToCursor(void)) );
		}

		if ( ctxMenuAddrType == 0 )
		{
			sprintf( stmp, "Go to $%04X\tDouble+Click", ctxMenuAddr );
			act = new QAction(tr(stmp), &menu);
			menu.addAction(act);
			//act->setShortcut( QKeySequence(tr("Ctrl+F10")));
			connect( act, SIGNAL(triggered(void)), parent, SLOT(asmViewCtxMenuGoTo(void)) );
		}

		if ( isBreakpointAtAddr( addr, romAddr ) >= 0 )
		{
			act = new QAction(tr("Edit &Breakpoint"), &menu);
		}
		else
		{
			act = new QAction(tr("Add &Breakpoint"), &menu);
		}
		menu.addAction(act);
		act->setShortcut( QKeySequence(tr("B")));
		connect( act, SIGNAL(triggered(void)), parent, SLOT(asmViewCtxMenuAddBP(void)) );

		act = new QAction(tr("Add &Symbolic Debug Marker"), &menu);
	 	menu.addAction(act);
		act->setShortcut( QKeySequence(tr("S")));
		connect( act, SIGNAL(triggered(void)), parent, SLOT(asmViewCtxMenuAddSym(void)) );

		act = new QAction(tr("Add Book&mark"), &menu);
	 	menu.addAction(act);
		act->setShortcut( QKeySequence(tr("M")));
		connect( act, SIGNAL(triggered(void)), parent, SLOT(asmViewCtxMenuAddBM(void)) );
		
		act = new QAction(tr("Open &Hex Editor"), &menu);
	 	menu.addAction(act);
		act->setShortcut( QKeySequence(tr("H")));
		connect( act, SIGNAL(triggered(void)), parent, SLOT(asmViewCtxMenuOpenHexEdit(void)) );
		
		menu.exec(event->globalPos());
	}
}
//----------------------------------------------------------------------------
void QAsmView::drawPointerPC( QPainter *painter, int xl, int yl )
{
	int x, y, w, h, b, b2;
	QPoint p[3];

	b  = 2;
	b2 = 2*b;

	w =  pxCharWidth;

	h = (pxCharHeight / 4);

	if ( h < 1 )
	{
		h = 1;
	}

	x = xl + (pxCharWidth*2);
	y = yl -  pxCharHeight + (pxCharHeight - h - b2)/2;

	painter->fillRect( x, y, w, h+b2, Qt::black );

	x = xl + (pxCharWidth*3);
	y = yl;

	p[0] = QPoint( x, y );
	p[1] = QPoint( x, y-pxCharHeight );
	p[2] = QPoint( x+pxCharWidth-1, y-(pxCharHeight/2) );

	painter->setBrush(Qt::red);

	painter->drawPolygon( p, 3 );

	x = xl + (pxCharWidth*2);
	y = yl -  pxCharHeight + (pxCharHeight - h)/2;

	painter->fillRect( x+b, y, w, h, Qt::red );
}
//----------------------------------------------------------------------------
void QAsmView::drawText( QPainter *painter, int x, int y, const char *txt )
{
	int i=0;
	char c[2];

	c[0] = 0; c[1] = 0;

	while ( txt[i] != 0 )
	{
		c[0] = txt[i];
		painter->drawText( x, y, tr(c) );
		i++; x += pxCharWidth;
	}
}
//----------------------------------------------------------------------------
void QAsmView::drawAsmLine( QPainter *painter, int x, int y, const char *txt )
{
	int i=0;
	char c[2];

	c[0] = 0; c[1] = 0;

	// CD Log Area
	painter->setPen( this->palette().color(QPalette::WindowText));

	while ( (i<pcLocLinePos) && (txt[i] != 0) )
	{
		c[0] = txt[i];
		painter->drawText( x, y, tr(c) );
		i++; x += pxCharWidth;
	}

	// Address Area
	painter->setPen( this->palette().color(QPalette::WindowText));

	while ( (i<byteCodeLinePos) && (txt[i] != 0) )
	{
		c[0] = txt[i];
		painter->drawText( x, y, tr(c) );
		i++; x += pxCharWidth;
	}

	// Byte Code Area
	painter->setPen( this->palette().color(QPalette::WindowText));

	while ( (i<opcodeLinePos) && (txt[i] != 0) )
	{
		c[0] = txt[i];
		painter->drawText( x, y, tr(c) );
		i++; x += pxCharWidth;
	}

	// Opcode Area
	painter->setPen( opcodeColor );

	if ( txt[i] != ' ' ) // Skip over undefined opcodes
	{
		while ( (i<operandLinePos) && (txt[i] != 0) )
		{
			c[0] = txt[i];
			painter->drawText( x, y, tr(c) );
			i++; x += pxCharWidth;
		}
	}

	// Operand Area
	painter->setPen( this->palette().color(QPalette::WindowText));

	while ( (txt[i] != 0) )
	{
		if ( (txt[i] == '$') && isxdigit( txt[i+1] ) )
		{
			painter->setPen( addressColor );

			c[0] = txt[i];
			painter->drawText( x, y, tr(c) );
			i++; x += pxCharWidth;

			while ( isxdigit( txt[i] ) )
			{
				c[0] = txt[i];
				painter->drawText( x, y, tr(c) );
				i++; x += pxCharWidth;
			}
		}
		else if ( (txt[i] == '#') && (txt[i+1] == '$') && isxdigit( txt[i+2] ) )
		{
			painter->setPen( immediateColor );

			c[0] = txt[i];
			painter->drawText( x, y, tr(c) );
			i++; x += pxCharWidth;

			c[0] = txt[i];
			painter->drawText( x, y, tr(c) );
			i++; x += pxCharWidth;

			while ( isxdigit( txt[i] ) )
			{
				c[0] = txt[i];
				painter->drawText( x, y, tr(c) );
				i++; x += pxCharWidth;
			}
		}
		else
		{
			painter->setPen( this->palette().color(QPalette::WindowText));
			c[0] = txt[i];
			painter->drawText( x, y, tr(c) );
			i++; x += pxCharWidth;
		}
	}
}
//----------------------------------------------------------------------------
void QAsmView::drawLabelLine( QPainter *painter, int x, int y, const char *txt )
{
	int i=0;
	char c[2];

	c[0] = 0; c[1] = 0;

	// Label Text
	painter->setPen( this->palette().color(QPalette::WindowText));

	while ( (txt[i] != 0) )
	{
		if ( isalnum(txt[i]) || (txt[i] == '_') )
		{
			painter->setPen( labelColor );
		}
		else
		{
			painter->setPen( this->palette().color(QPalette::WindowText));
		}
		c[0] = txt[i];
		painter->drawText( x, y, tr(c) );
		i++; x += pxCharWidth;
	}
}
//----------------------------------------------------------------------------
void QAsmView::drawCommentLine( QPainter *painter, int x, int y, const char *txt )
{
	int i=0;
	char c[2];

	c[0] = 0; c[1] = 0;

	// Comment Text
	painter->setPen( commentColor );

	while ( (txt[i] != 0) )
	{
		c[0] = txt[i];
		painter->drawText( x, y, tr(c) );
		i++; x += pxCharWidth;
	}
}
//----------------------------------------------------------------------------
void QAsmView::paintEvent(QPaintEvent *event)
{
	int x,y,l, row, nrow, selAddr;
	int cd_boundary, asm_start_boundary;
	int pxCharWidth2, vOffset, vlineOffset;
	QPainter painter(this);
	QColor white("white"), black("black"), blue("blue");
	QColor hlgtFG("white"), hlgtBG("blue");
	bool forceDarkColor = false;
	bool txtHlgtSet = false;
	bool lineIsPC = false;
	QPen pen;

	//if ( isPopUp )
	//{
	//	printf("Is PopUp\n");
	//}
	painter.setFont(font);
	viewWidth  = event->rect().width();
	viewHeight = event->rect().height();

	nrow = (viewHeight / pxLineSpacing) + 1;

	if ( nrow < 1 ) nrow = 1;

	viewLines = nrow;

	if ( cursorPosY >= viewLines )
	{
		cursorPosY = viewLines-1;
	}
	maxLineOffset = asmEntry.size() - nrow + 1;

	if ( maxLineOffset < 0 )
	{
		maxLineOffset = 0;
	}

	if ( lineOffset < 0 )
	{
		lineOffset = 0;
	}
	if ( lineOffset > maxLineOffset )
	{
		lineOffset = maxLineOffset;
	}
	if ( parent )
	{
		selAddr = parent->getBookmarkSelectedAddress();
	}
	else
	{
		selAddr = selAddrValue;
	}

	pxCharWidth2 = (pxCharWidth/2);
	cd_boundary = (int)(2.5*pxCharWidth) - pxLineXScroll;
	asm_start_boundary = cd_boundary + (10*pxCharWidth);
	//asm_stop_boundary  = asm_start_boundary + (9*pxCharWidth);
	
	vlineOffset = -pxLineSpacing + (pxLineSpacing - pxCharHeight)/2;

	painter.fillRect( 0, 0, viewWidth, viewHeight, this->palette().color(QPalette::Window) );
	painter.fillRect( 0, 0, cd_boundary, viewHeight, this->palette().color(QPalette::Mid) );

	if ( showByteCodes )
	{
		painter.fillRect( asm_start_boundary, 0, (9*pxCharWidth), viewHeight, this->palette().color(QPalette::AlternateBase) );
	}

	y = pxLineSpacing;

	txtHlgtSet = textIsHighlighted();

	for (row=0; row < nrow; row++)
	{
		x = -pxLineXScroll;
		l = lineOffset + row;

		forceDarkColor = false;

		if ( asmPC != NULL )
		{
			if ( l == asmPC->line )
			{
				painter.fillRect( cd_boundary, y + vlineOffset, viewWidth, pxLineSpacing, pcBgColor );
				forceDarkColor = true;
				lineIsPC = true;
			}
			else
			{
				lineIsPC = false;
			}
		}
		else
		{
			lineIsPC = false;
		}

		if ( l < asmEntry.size() )
		{
			//if ( asmEntry[l]->type != dbg_asm_entry_t::ASM_TEXT )
			//{
			//	painter.fillRect( cd_boundary, y - pxLineSpacing + pxLineLead, viewWidth, pxLineSpacing, QColor("light blue") );
			//	forceDarkColor = true;
			//}

			if ( forceDarkColor )
			{
				painter.setPen( black );
			}
			else
			{
				painter.setPen( this->palette().color(QPalette::WindowText));
			}
			if ( asmEntry[l]->type == dbg_asm_entry_t::ASM_TEXT )
			{
				drawAsmLine( &painter, x, y, asmEntry[l]->text.c_str() );
			}
			else if ( asmEntry[l]->type == dbg_asm_entry_t::SYMBOL_NAME )
			{
				drawLabelLine( &painter, x, y, asmEntry[l]->text.c_str() );
			}
			else
			{
				drawCommentLine( &painter, x, y, asmEntry[l]->text.c_str() );
			}

			if ( (selAddrLine == l) )
			{	// Highlight ASM line for selected address.
				if ( !txtHlgtSet && (selAddr == selAddrValue) && 
				  	    (asmEntry[l]->text.size() >= (selAddrChar + selAddrWidth) ) && 
						    ( asmEntry[l]->text.compare( selAddrChar, selAddrWidth, selAddrText ) == 0 ) )
				{
					int ax;

					ax = x + selAddrChar*pxCharWidth;

					painter.fillRect( ax, y + vlineOffset, selAddrWidth*pxCharWidth, pxLineSpacing, blue );

					painter.setPen( white );

					drawText( &painter, ax, y, selAddrText );

					painter.setPen( this->palette().color(QPalette::WindowText));
				}
			}
		}
		y += pxLineSpacing;
	}

	y = pxLineSpacing;

	painter.setPen( hlgtFG );

	if ( txtHlgtSet )
	{
		for (row=0; row < nrow; row++)
		{
			x = -pxLineXScroll;
			l = lineOffset + row;

			if ( (l >= txtHlgtStartLine) && (l <= txtHlgtEndLine) )
			{
				int ax, hlgtXs, hlgtXe, hlgtXd;
				std::string s;

				if ( l == txtHlgtStartLine )
				{
					hlgtXs = txtHlgtStartChar;
				}
				else
				{
					hlgtXs = 0;
				}

				if ( l == txtHlgtEndLine )
				{
					hlgtXe = txtHlgtEndChar;
				}
				else
				{
					hlgtXe = (viewWidth / pxCharWidth) + 1;
				}
				hlgtXd = (hlgtXe - hlgtXs);

				if ( hlgtXs < asmEntry[l]->text.size() )
				{
					s = asmEntry[l]->text.substr( hlgtXs, hlgtXd );
				}

				ax = x + (hlgtXs * pxCharWidth);

				painter.fillRect( ax, y + vlineOffset, hlgtXd * pxCharWidth, pxLineSpacing, hlgtBG );

				drawText( &painter, ax, y, s.c_str() );
			}
			y += pxLineSpacing;
		}
	}
	pen = painter.pen();
	pen.setWidth(3);
	pen.setColor( this->palette().color(QPalette::WindowText));
	painter.setPen( pen );
	painter.drawLine( cd_boundary, 0, cd_boundary, viewHeight );

	y = pxLineSpacing;

	pen.setWidth(2);
	painter.setPen( pen );
	painter.setBrush(Qt::red);
	vOffset = pxLineLead + (pxLineSpacing - pxLineLead - pxCharWidth)/2;

	for (row=0; row < nrow; row++)
	{
		x = -pxLineXScroll;
		l = lineOffset + row;

		if ( l < asmEntry.size() )
		{
			if ( asmPC != NULL )
			{
				if ( l == asmPC->line )
				{
					lineIsPC = true;
				}
				else
				{
					lineIsPC = false;
				}
			}
			else
			{
				lineIsPC = false;
			}

			if ( lineIsPC )
			{
				drawPointerPC( &painter, x, y );
			}

			if ( asmEntry[l]->bpNum >= 0 )
			{
				painter.drawEllipse( cd_boundary - pxCharWidth2, y + vlineOffset + vOffset, pxCharWidth, pxCharWidth );
			}
		}

		y += pxLineSpacing;
	}
}
//----------------------------------------------------------------------------
// Bookmark Manager Methods
//----------------------------------------------------------------------------
debuggerBookmarkManager_t::debuggerBookmarkManager_t(void)
{
	internal_iter = bmMap.begin();

}
//----------------------------------------------------------------------------
debuggerBookmarkManager_t::~debuggerBookmarkManager_t(void)
{
	this->clear();
}
//----------------------------------------------------------------------------
void debuggerBookmarkManager_t::clear(void)
{
	std::map <int, debuggerBookmark_t*>::iterator it;

	for (it=bmMap.begin(); it!=bmMap.end(); it++)
	{
		delete it->second;
	}
	bmMap.clear();

	internal_iter = bmMap.begin();
}
//----------------------------------------------------------------------------
int debuggerBookmarkManager_t::addBookmark( int addr, const char *name )
{
	int retval = -1;
	debuggerBookmark_t *bm = NULL;
	std::map <int, debuggerBookmark_t*>::iterator it;

	it = bmMap.find( addr );

	if ( it == bmMap.end() )
	{
		bm = new debuggerBookmark_t();
		bm->addr = addr;

		if ( name != NULL )
		{
			bm->name.assign( name );
		}
		bmMap[ addr ] = bm;

		retval = 0;
	}

	return retval;
}
//----------------------------------------------------------------------------
int debuggerBookmarkManager_t::editBookmark( int addr, const char *name )
{
	int retval = -1;
	debuggerBookmark_t *bm = NULL;
	std::map <int, debuggerBookmark_t*>::iterator it;

	it = bmMap.find( addr );

	if ( it != bmMap.end() )
	{
		bm = it->second;

		if ( name != NULL )
		{
			bm->name.assign( name );
		}
		retval = 0;
	}

	return retval;
}
//----------------------------------------------------------------------------
int debuggerBookmarkManager_t::deleteBookmark( int addr )
{
	int retval = -1;
	std::map <int, debuggerBookmark_t*>::iterator it;

	it = bmMap.find( addr );

	if ( it != bmMap.end() )
	{
		bmMap.erase(it);

		retval = 0;
	}
	return retval;
}
//----------------------------------------------------------------------------
int debuggerBookmarkManager_t::size(void)
{
	return bmMap.size();
}
//----------------------------------------------------------------------------
debuggerBookmark_t *debuggerBookmarkManager_t::begin(void)
{
	internal_iter = bmMap.begin();

	if ( internal_iter == bmMap.end() )
	{
		return NULL;
	}
	return internal_iter->second;
}
//----------------------------------------------------------------------------
debuggerBookmark_t *debuggerBookmarkManager_t::next(void)
{
	if ( internal_iter == bmMap.end() )
	{
		return NULL;
	}
	internal_iter++;

	if ( internal_iter == bmMap.end() )
	{
		return NULL;
	}
	return internal_iter->second;
}
//----------------------------------------------------------------------------
debuggerBookmark_t *debuggerBookmarkManager_t::getAddr( int addr )
{
	std::map <int, debuggerBookmark_t*>::iterator it;

	it = bmMap.find( addr );

	if ( it != bmMap.end() )
	{
		return it->second;
	}
	return NULL;
}
//----------------------------------------------------------------------------
DebuggerStackDisplay::DebuggerStackDisplay(QWidget *parent)
   : QPlainTextEdit(parent)
{
	QFont font;
	std::string fontString;

	stackBytesPerLine = 4;
	showAddrs = true;

	recalcCharsPerLine();

	g_config->getOption("SDL.DebuggerStackFont", &fontString);

	if ( fontString.size() > 0 )
	{
		font.fromString( QString::fromStdString( fontString ) );
	}
	else
	{
		font.setFamily("Courier New");
		font.setStyle( QFont::StyleNormal );
		font.setStyleHint( QFont::Monospace );
	}
	setFont( font );

	QFontMetrics fm(font);

#if QT_VERSION > QT_VERSION_CHECK(5, 11, 0)
	pxCharWidth = fm.horizontalAdvance(QLatin1Char('2'));
#else
	pxCharWidth = fm.width(QLatin1Char('2'));
#endif
	pxLineSpacing = fm.lineSpacing();

	setMinimumWidth( pxCharWidth * charsPerLine );
	setMinimumHeight( pxLineSpacing *  5 );
	setMaximumHeight( pxLineSpacing * 20 );
}
//----------------------------------------------------------------------------
DebuggerStackDisplay::~DebuggerStackDisplay(void)
{

}
//----------------------------------------------------------------------------
void DebuggerStackDisplay::setFont( const QFont &font )
{
	QFontMetrics fm(font);

	QPlainTextEdit::setFont(font);

#if QT_VERSION > QT_VERSION_CHECK(5, 11, 0)
	pxCharWidth = fm.horizontalAdvance(QLatin1Char('2'));
#else
	pxCharWidth = fm.width(QLatin1Char('2'));
#endif
	pxLineSpacing = fm.lineSpacing();
	
	setMinimumWidth( pxCharWidth * charsPerLine );
	setMinimumHeight( pxLineSpacing * 5 );
	setMaximumHeight( pxLineSpacing * 20 );
}
//----------------------------------------------------------------------------
void DebuggerStackDisplay::recalcCharsPerLine(void)
{
	charsPerLine = (showAddrs ? 5 : 1) + (stackBytesPerLine*3);
}
//----------------------------------------------------------------------------
void DebuggerStackDisplay::keyPressEvent(QKeyEvent *event)
{
	//printf("Debug Stack Window Key Press: 0x%x \n", event->key() );

	if ( (event->key() >= Qt::Key_1) && ( (event->key() < Qt::Key_9) ) )
	{
		stackBytesPerLine = event->key() - Qt::Key_0;
		recalcCharsPerLine();
	
		updateText();
	}
	else if ( event->key() == Qt::Key_A )
	{
		showAddrs = !showAddrs;
		recalcCharsPerLine();
		updateText();
	}
}
//----------------------------------------------------------------------------
void DebuggerStackDisplay::contextMenuEvent(QContextMenuEvent *event)
{
	QAction *act;
	QMenu menu(this);
	QMenu *subMenu;
	QActionGroup *group;
	QAction *bytesPerLineAct[4];

	act = new QAction(tr("Show Addresses"), &menu);
	act->setCheckable(true);
	act->setChecked(showAddrs);
	connect( act, SIGNAL(triggered(void)), this, SLOT(toggleShowAddr(void)) );

	menu.addAction( act );

	subMenu = menu.addMenu(tr("Display Bytes Per Line"));
	group   = new QActionGroup(&menu);

	group->setExclusive(true);

	for (int i=0; i<4; i++)
	{
	   char stmp[8];

	   sprintf( stmp, "%i", i+1 );

	   bytesPerLineAct[i] = new QAction(tr(stmp), &menu);
	   bytesPerLineAct[i]->setCheckable(true);

	   group->addAction(bytesPerLineAct[i]);
		subMenu->addAction(bytesPerLineAct[i]);
      
	   bytesPerLineAct[i]->setChecked( stackBytesPerLine == (i+1) );
	}

	connect( bytesPerLineAct[0], SIGNAL(triggered(void)), this, SLOT(sel1BytePerLine(void)) );
	connect( bytesPerLineAct[1], SIGNAL(triggered(void)), this, SLOT(sel2BytesPerLine(void)) );
	connect( bytesPerLineAct[2], SIGNAL(triggered(void)), this, SLOT(sel3BytesPerLine(void)) );
	connect( bytesPerLineAct[3], SIGNAL(triggered(void)), this, SLOT(sel4BytesPerLine(void)) );

	menu.exec(event->globalPos());
}
//----------------------------------------------------------------------------
void DebuggerStackDisplay::toggleShowAddr(void)
{
	showAddrs = !showAddrs;
	recalcCharsPerLine();

	updateText();
}
//----------------------------------------------------------------------------
void DebuggerStackDisplay::sel1BytePerLine(void)
{
	stackBytesPerLine = 1;
	recalcCharsPerLine();
	updateText();
}
//----------------------------------------------------------------------------
void DebuggerStackDisplay::sel2BytesPerLine(void)
{
	stackBytesPerLine = 1;
	recalcCharsPerLine();
	updateText();
}
//----------------------------------------------------------------------------
void DebuggerStackDisplay::sel3BytesPerLine(void)
{
	stackBytesPerLine = 3;
	recalcCharsPerLine();
	updateText();
}
//----------------------------------------------------------------------------
void DebuggerStackDisplay::sel4BytesPerLine(void)
{
	stackBytesPerLine = 4;
	recalcCharsPerLine();
	updateText();
}
//----------------------------------------------------------------------------
void DebuggerStackDisplay::updateText(void)
{
   char stmp[128];
   int stackPtr = X.S | 0x0100;
   std::string stackLine;

	stackPtr++;

	if ( stackPtr <= 0x01FF )
	{
		if ( showAddrs || (stackBytesPerLine <= 1) )
		{
			sprintf( stmp, "%03X: %02X", stackPtr, GetMem(stackPtr) );
		}
		else
		{
			sprintf( stmp, "%02X", GetMem(stackPtr) );
		}

		stackLine.assign( stmp );

		for (int i = 1; i < 128; i++)
		{
			//tmp = ((tmp+1)|0x0100)&0x01FF;  //increment and fix pointer to $0100-$01FF range
			stackPtr++;
			if (stackPtr > 0x1FF)
				break;

			if ( stackBytesPerLine > 1 )
			{
				if ((i % stackBytesPerLine) == 0)
				{
					if ( showAddrs )
					{
						sprintf( stmp, "\n%03X: %02X", stackPtr, GetMem(stackPtr) );
					}
					else
					{
						sprintf( stmp, "\n%02X", GetMem(stackPtr) );
					}
				}
				else
				{
				  	sprintf( stmp, ",%02X", GetMem(stackPtr) );
				}
			}
			else
			{
			       	   sprintf( stmp, "\n%03X: %02X", stackPtr, GetMem(stackPtr) );
			}
			stackLine.append( stmp );

			//printf("Stack $%X: %s\n", stackPtr, stmp );
		}
	}

	setPlainText( tr(stackLine.c_str()) );

	setMinimumWidth( pxCharWidth * charsPerLine );
	setMinimumHeight( pxLineSpacing * 5 );
}
//----------------------------------------------------------------------------
//--  PPU Control Register Widget
//----------------------------------------------------------------------------
ppuCtrlRegDpy::ppuCtrlRegDpy( QWidget *parent )
	: QLineEdit( parent )
{
	popup = NULL;
	setReadOnly(true);
}
//----------------------------------------------------------------------------
ppuCtrlRegDpy::~ppuCtrlRegDpy( void )
{
	//if ( popup != NULL )
	//{
	//	popup->close();
	//}
}
//----------------------------------------------------------------------------
bool ppuCtrlRegDpy::event(QEvent *event)
{
	if (event->type() == QEvent::ToolTip)
	{
		QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
		//if ( popup == NULL )
		//{
		//	printf("Tool Tip Show\n");
		//}
		popup = static_cast<ppuRegPopup*>(fceuCustomToolTipShow( helpEvent, new ppuRegPopup(this) ));

		QToolTip::hideText();
		event->ignore();
		return true;
	}
	return QWidget::event(event);
}
//----------------------------------------------------------------------------
//--  PPU Register Tool Tip Popup
//----------------------------------------------------------------------------
asmLookAheadPopup::asmLookAheadPopup( int addr, QWidget *parent )
	: fceuCustomToolTip( parent )
{
	int line, bank = -1, romOfs = -1;
	int pxCharWidth;
	QVBoxLayout *vbox, *vbox1;
	QHBoxLayout *hbox;
	QGridLayout *grid;
	QScrollBar  *hbar, *vbar;
	QFrame      *asmFrame, *dataFrame;
	QLineEdit   *cpuAddr, *cpuVal, *romAddr;
	QLabel      *lbl;
	QFont        font;
	char stmp[128];

	fceuWrapperLock();

	vbox    = new QVBoxLayout();
	vbox1   = new QVBoxLayout();

	asmFrame   = new QFrame();
	dataFrame  = new QFrame();
	asmView    = new QAsmView(this);
	vbar       = new QScrollBar( Qt::Vertical, this );
	hbar       = new QScrollBar( Qt::Horizontal, this );

	font = asmView->getFont();
	QFontMetrics fm(font);
#if QT_VERSION > QT_VERSION_CHECK(5, 11, 0)
	pxCharWidth = fm.horizontalAdvance(QLatin1Char('2'));
#else
	pxCharWidth = fm.width(QLatin1Char('2'));
#endif
	setHideOnMouseMove(true);
	asmView->setIsPopUp(true);

	if (addr >= 0x8000)
	{
		bank   = getBank(addr);
		romOfs = GetNesFileAddress(addr);
	}
	dataFrame->setLayout( vbox );

	cpuAddr = new QLineEdit();
	cpuVal  = new QLineEdit();
	romAddr = NULL;

	if ( bank >= 0 )
	{
		romAddr = new QLineEdit();
		hbox = new QHBoxLayout();
		vbox->addLayout( hbox );

		sprintf( stmp, "%02X : $%04X", bank, addr );
		cpuAddr->setText( tr(stmp) );
		sprintf( stmp, "#$%02X", GetMem(addr) );
		cpuVal->setText( tr(stmp) );
		sprintf( stmp, "$%06X", romOfs );
		romAddr->setText( tr(stmp) );

		lbl = new QLabel( tr("CPU ADDR:") );
		lbl->setFont(font);
		hbox->addWidget( lbl, 1 );
		hbox->addWidget( cpuAddr, 1 );

		lbl = new QLabel( tr(" = ") );
		lbl->setFont(font);
		hbox->addWidget( lbl, 1 );
		hbox->addWidget( cpuVal, 1 );
		hbox->addStretch(5);

		hbox = new QHBoxLayout();
		vbox->addLayout( hbox );
		lbl = new QLabel( tr("ROM ADDR:") );
		lbl->setFont(font);
		hbox->addWidget( lbl, 1 );
		hbox->addWidget( romAddr, 1 );
		hbox->addStretch(5);
	}
	else
	{
		hbox = new QHBoxLayout();
		vbox->addLayout( hbox );

		sprintf( stmp, "$%04X", addr );
		cpuAddr->setText( tr(stmp) );
		sprintf( stmp, "#$%02X", GetMem(addr) );
		cpuVal->setText( tr(stmp) );

		lbl = new QLabel( tr("CPU ADDR:") );
		lbl->setFont(font);
		hbox->addWidget( lbl, 1 );
		hbox->addWidget( cpuAddr, 1 );

		lbl = new QLabel( tr(" = ") );
		lbl->setFont(font);
		hbox->addWidget( lbl, 1 );
		hbox->addWidget( cpuVal, 1 );
		hbox->addStretch(5);
	}

	cpuAddr->setFont(font);
	 cpuVal->setFont(font);
	cpuAddr->setAlignment( Qt::AlignCenter );
	 cpuVal->setAlignment( Qt::AlignCenter );

	cpuAddr->setMinimumWidth( pxCharWidth * (cpuAddr->text().size()+2) );
	 cpuVal->setMinimumWidth( pxCharWidth * ( cpuVal->text().size()+2) );

	cpuAddr->setMaximumWidth( cpuAddr->minimumWidth() );
	 cpuVal->setMaximumWidth(  cpuVal->minimumWidth() );

	if ( romAddr )
	{
		romAddr->setFont(font);
		romAddr->setAlignment( Qt::AlignCenter );
		romAddr->setMinimumWidth( pxCharWidth * (romAddr->text().size()+2) );
		romAddr->setMaximumWidth( romAddr->minimumWidth() );

		if ( romAddr->minimumWidth() < cpuAddr->minimumWidth() )
		{
			romAddr->setMinimumWidth( cpuAddr->minimumWidth() );
			romAddr->setMaximumWidth( cpuAddr->minimumWidth() );
		}
	}

	vbox    = new QVBoxLayout();
	asmFrame->setLayout( vbox );
	vbox1->addWidget( dataFrame, 1 );
	vbox1->addWidget( asmFrame, 100 );
	dataFrame->setFrameShape(QFrame::Box);
	asmFrame->setFrameShape(QFrame::Box);

	hbar->setMinimum(0);
	hbar->setMaximum(100);
	vbar->setMinimum(0);
	vbar->setMaximum( 0x10000 );

	asmView->setScrollBars( hbar, vbar );

	grid = new QGridLayout();
	grid->addWidget( asmView, 0, 0 );
	grid->addWidget( vbar   , 0, 1 );
	grid->addWidget( hbar   , 1, 0 );

	vbox->addLayout( grid );

	setLayout( vbox1 );

	resize(512, 512);

	asmView->updateAssemblyView();
	fceuWrapperUnLock();

	hbar->hide();
	vbar->hide();

	line = asmView->getAsmLineFromAddr(addr);
	asmView->gotoLine(line);

	//printf("PopUp: Addr: $%04X   %i\n", addr, line );
}
//----------------------------------------------------------------------------
asmLookAheadPopup::~asmLookAheadPopup( void )
{
	//printf("Popup Deleted\n");
}
//----------------------------------------------------------------------------
//--  PPU Register Tool Tip Popup
//----------------------------------------------------------------------------
ppuRegPopup::ppuRegPopup( QWidget *parent )
	: fceuCustomToolTip( parent )
{
	QVBoxLayout *vbox, *vbox1;
	QGridLayout *grid, *grid1;
	QGroupBox *ctlFrame;
	QFrame    *winFrame;
	QLabel    *bgAddrLbl, *sprAddrLbl;
	QLineEdit *ppuBgAddr;
	QLineEdit *ppuSprAddr;
	QCheckBox *bgEnabled_cbox;
	QCheckBox *sprites_cbox;
	QCheckBox *drawLeftBg_cbox;
	QCheckBox *drawLeftFg_cbox;
	QCheckBox *vwrite_cbox;
	QCheckBox *nmiBlank_cbox;
	QCheckBox *sprite8x16_cbox;
	QCheckBox *grayscale_cbox;
	QCheckBox *iRed_cbox;
	QCheckBox *iGrn_cbox;
	QCheckBox *iBlu_cbox;
	QCheckBox *vblank_cbox;
	QCheckBox *sprite0hit_cbox;
	QCheckBox *spriteOvrflw_cbox;
	char stmp[32];

	//QPalette pal = this->palette();
	//pal.setColor( QPalette::Window    , pal.color(QPalette::ToolTipBase) );
	//pal.setColor( QPalette::WindowText, pal.color(QPalette::ToolTipText) );
	//setPalette(pal);

	vbox1    = new QVBoxLayout();
	vbox     = new QVBoxLayout();
	winFrame = new QFrame();
	ctlFrame = new QGroupBox( tr("PPU Control / Mask / Status") );
	grid1    = new QGridLayout();
	grid     = new QGridLayout();

	winFrame->setFrameShape(QFrame::Box);

	vbox1->addWidget( winFrame );
	winFrame->setLayout( vbox );
	vbox->addWidget( ctlFrame );
	ctlFrame->setLayout( grid1 );

	 bgAddrLbl = new QLabel( tr("BG Addr") );
	sprAddrLbl = new QLabel( tr("Spr Addr") );

	ppuBgAddr   = new QLineEdit();
	ppuSprAddr  = new QLineEdit();

	grid->addWidget( bgAddrLbl, 0, 0 );
	grid->addWidget( sprAddrLbl, 1, 0 );
	grid->addWidget( ppuBgAddr, 0, 1 );
	grid->addWidget( ppuSprAddr, 1, 1 );

	grid1->addLayout( grid, 0, 0, 3, 1 );

	bgEnabled_cbox    = new QCheckBox( tr("BG Enabled") );
	sprites_cbox      = new QCheckBox( tr("Sprites Enabled") );
	drawLeftBg_cbox   = new QCheckBox( tr("Draw Left BG (8px)") );
	drawLeftFg_cbox   = new QCheckBox( tr("Draw Left Sprites (8px)") );
	vwrite_cbox       = new QCheckBox( tr("Vertical Write") );
	nmiBlank_cbox     = new QCheckBox( tr("NMI on vBlank") );
	sprite8x16_cbox   = new QCheckBox( tr("8x16 Sprites") );
	grayscale_cbox    = new QCheckBox( tr("Grayscale") );
	iRed_cbox         = new QCheckBox( tr("Intensify Red") );
	iGrn_cbox         = new QCheckBox( tr("Intensify Green") );
	iBlu_cbox         = new QCheckBox( tr("Intensify Blue") );
	vblank_cbox       = new QCheckBox( tr("V-Blank") );
	sprite0hit_cbox   = new QCheckBox( tr("Sprite 0 Hit") );
	spriteOvrflw_cbox = new QCheckBox( tr("Sprite Overflow") );

	sprintf( stmp, "$%04X", 0x2000 + (0x400*(PPU[0] & 0x03)));
	ppuBgAddr->setText( tr(stmp) );

	sprintf( stmp, "$%04X", (PPU[0] & 0x08) ? 0x1000 : 0x0000 );
	ppuSprAddr->setText( tr(stmp) );

	  nmiBlank_cbox->setChecked( PPU[0] & 0x80 );
	sprite8x16_cbox->setChecked( PPU[0] & 0x20 );

	 grayscale_cbox->setChecked( PPU[1] & 0x01 );
	drawLeftBg_cbox->setChecked( PPU[1] & 0x02 );
	drawLeftFg_cbox->setChecked( PPU[1] & 0x04 );
	 bgEnabled_cbox->setChecked( PPU[1] & 0x08 );
	   sprites_cbox->setChecked( PPU[1] & 0x10 );
	      iRed_cbox->setChecked( PPU[1] & 0x20 );
	      iGrn_cbox->setChecked( PPU[1] & 0x40 );
	      iBlu_cbox->setChecked( PPU[1] & 0x80 );

	      vblank_cbox->setChecked( PPU[2] & 0x80 );
	  sprite0hit_cbox->setChecked( PPU[2] & 0x40 );
	spriteOvrflw_cbox->setChecked( PPU[2] & 0x20 );

	grid1->addWidget( bgEnabled_cbox   , 3, 0 );
	grid1->addWidget( sprites_cbox     , 4, 0 );
	grid1->addWidget( drawLeftBg_cbox  , 5, 0 );
	grid1->addWidget( drawLeftFg_cbox  , 6, 0 );
	grid1->addWidget( sprite0hit_cbox  , 7, 0 );
	grid1->addWidget( spriteOvrflw_cbox, 8, 0 );

	grid1->addWidget( vwrite_cbox    , 0, 1 );
	grid1->addWidget( nmiBlank_cbox  , 1, 1 );
	grid1->addWidget( sprite8x16_cbox, 2, 1 );
	grid1->addWidget( grayscale_cbox , 3, 1 );
	grid1->addWidget( iRed_cbox      , 4, 1 );
	grid1->addWidget( iGrn_cbox      , 5, 1 );
	grid1->addWidget( iBlu_cbox      , 6, 1 );
	grid1->addWidget( vblank_cbox    , 7, 1 );

	setLayout( vbox1 );
}
//----------------------------------------------------------------------------
ppuRegPopup::~ppuRegPopup( void )
{
	//printf("Popup Deleted\n");
}
//----------------------------------------------------------------------------
//--- Debugger Tabbed Data Display
//----------------------------------------------------------------------------
DebuggerTabWidget::DebuggerTabWidget( int row, int col, QWidget *parent )
	: QTabWidget(parent)
{
	DebuggerTabBar *bar = new DebuggerTabBar(this);
	setTabBar( bar );
	bar->setAcceptDrops(true);
	bar->setChangeCurrentOnDrag(true);
	setMouseTracking(true);
	setAcceptDrops(true);

	_row = row;
	_col = col;

	setMovable(true);
	setUsesScrollButtons(true);

	connect( bar, &DebuggerTabBar::beginDragOut, this,[this,bar](int index)
	{
		if (!this->indexValid(index))
		{
		    return;
		}
		QWidget *drag_tab=this->widget(index);
		//Fixed tab will not be dragged out
		if (!drag_tab /*||fixedPage.contains(drag_tab)*/)
		{
		    return;
		}
		//Drag and drop the current page as a snapshot
		//The size adds the title bar and border
		QPixmap pixmap(drag_tab->size()+QSize(2,31));
		pixmap.fill(Qt::transparent);

		QPainter painter(&pixmap);

		if (painter.isActive())
		{
			//I want to make the title bar pasted on the content
			//But you can't get the image of the default title bar, just draw a rectangular box
			//If the external theme color is set, you need to change it
			QRect title_rect{0,0,pixmap.width(),30};
			painter.fillRect(title_rect,Qt::white);
			painter.drawText(title_rect,Qt::AlignLeft|Qt::AlignVCenter,"  "+drag_tab->windowTitle());
			painter.drawRect(pixmap.rect().adjusted(0,0,-1,-1));
		}
		painter.end();
		drag_tab->render(&pixmap,QPoint(1,30));
		
		QMimeData *mime=new QMimeData;
		QDrag *drag=new QDrag(bar);
		drag->setMimeData(mime);
		drag->setPixmap(pixmap);
		drag->setHotSpot(QPoint(10,0));
		
		//Drag is released after the mouse bounces up, at this time to judge whether it is dragged to the outside
		connect(drag,&QDrag::destroyed,this,[=]{
		    QPoint bar_point=bar->mapFromGlobal(QCursor::pos());
		                 //Out of range, drag out
		    if (!bar->contentsRect().contains(bar_point))
		    {
		        popPage(drag_tab);
		    }
		});
		
		drag->exec(Qt::MoveAction);
	});
}
//----------------------------------------------------------------------------
DebuggerTabWidget::~DebuggerTabWidget(void)
{

}
//----------------------------------------------------------------------------
bool DebuggerTabWidget::indexValid(int idx)
{
	return ( (idx >= 0) && (idx < count()) );
}
//----------------------------------------------------------------------------
void DebuggerTabWidget::dragEnterEvent(QDragEnterEvent *event)
{
	DebuggerTabBar *w = qobject_cast<DebuggerTabBar*>(event->source());
	//printf("Tab Widget Drag Enter Event: %p\n", w);

	if ( (w != NULL) && (event->dropAction() == Qt::MoveAction) )
	{
		//printf("Drag Action Accepted\n");
		event->acceptProposedAction();
	}
}

//----------------------------------------------------------------------------
void DebuggerTabWidget::dropEvent(QDropEvent *event)
{
	DebuggerTabBar *bar = qobject_cast<DebuggerTabBar*>(event->source());
	//printf("Tab Widget Drop Event: %p\n", bar);

	if ( (bar != NULL) && (event->dropAction() == Qt::MoveAction) )
	{
		int idx = bar->currentIndex();

		DebuggerTabWidget *p = qobject_cast<DebuggerTabWidget*>(bar->parent());
		if ( p == NULL )
		{
			return;
		}
		QWidget *w = p->widget(idx);

		if ( w )
		{
			QString txt = bar->tabText(idx);
			//printf("Removing Widget from Parent:%p  %p  %p  %i\n", bar, p, w, idx);
			p->removeTab( idx );
			addTab(w, txt);

			dbgWin->updateTabVisibility();
		}
	}
}
//----------------------------------------------------------------------------
void DebuggerTabWidget::mouseMoveEvent(QMouseEvent * e)
{
	//printf("TabWidget: (%i,%i) \n", e->pos().x(), e->pos().y() );;
}
//----------------------------------------------------------------------------
void DebuggerTabWidget::contextMenuEvent(QContextMenuEvent *event)
{
	buildContextMenu(event);
}
//----------------------------------------------------------------------------
void DebuggerTabWidget::buildContextMenu(QContextMenuEvent *event)
{
	int i,j;
	QAction *act;
	QActionGroup *group;
	QMenu menu(this);
	QMenu *moveMenu, *subMenu;

	//printf("TabWidget Context\n");

	moveMenu = menu.addMenu( tr("Move Tab To...") );

	group = new QActionGroup(moveMenu);
	group->setExclusive(true);

	for (j=0; j<4; j++)
	{
		const char *vertText;

		switch (j)
		{
			default:
			case 0:
				vertText = "Upper";
			break;
			case 1:
				vertText = "Mid-Upper";
			break;
			case 2:
				vertText = "Mid-Lower";
			break;
			case 3:
				vertText = "Lower";
			break;
		}
		subMenu = moveMenu->addMenu( tr(vertText) );

		for (i=0; i<2; i++)
		{
			const char *horzText = i ? "Right" : "Left";

			act = new QAction(tr(horzText), &menu);

			subMenu->addAction(act);
			group->addAction(act);

			QWidget *w = widget( currentIndex() );

			connect( act, &QAction::triggered, [w, j, i]{ dbgWin->ConsoleDebugger::moveTab( w, j, i ); } );
		}
	}

	menu.exec(event->globalPos());
}
//----------------------------------------------------------------------------
void DebuggerTabWidget::popPage(QWidget *page)
{
	printf("Pop Page: %p\n", page);
}
//----------------------------------------------------------------------------
//--- Debugger Tabbed Data Display
//----------------------------------------------------------------------------
DebuggerTabBar::DebuggerTabBar( QWidget *parent )
	: QTabBar(parent)
{
	setMouseTracking(true);
	setAcceptDrops(true);
	theDragPress = false;
	theDragOut = false;
}
//----------------------------------------------------------------------------
DebuggerTabBar::~DebuggerTabBar(void)
{
}
//----------------------------------------------------------------------------
void DebuggerTabBar::mouseMoveEvent( QMouseEvent *event)
{
	//printf("TabBar Mouse Move: (%i,%i) \n", event->pos().x(), event->pos().y() );;
	QTabBar::mouseMoveEvent(event);

	if ( theDragPress && event->buttons())
	{
		//Is it out of the scope of tabbar
		if ( !theDragOut && !contentsRect().contains(event->pos()) )
		{
			theDragOut=true;
			emit beginDragOut(this->currentIndex());
			
			//The release will not be triggered after QDrag.exec, manually trigger it yourself
			//But he still seems to animate after the mouse is up, to be resolved
			QMouseEvent *e=new QMouseEvent(QEvent::MouseButtonRelease,
			                                  this->mapFromGlobal(QCursor::pos()),
			                                   Qt::LeftButton,
			                                   Qt::LeftButton,
			                                   Qt::NoModifier);
			//mouseReleaseEvent(event);
			QApplication::postEvent(this,e);
		}
	}
}
//----------------------------------------------------------------------------
void DebuggerTabBar::mousePressEvent( QMouseEvent *event)
{
	//printf("TabBar Mouse Press: (%i,%i) \n", event->pos().x(), event->pos().y() );;
	QTabBar::mousePressEvent(event);

	if ( (event->button() == Qt::LeftButton) && (currentIndex() >= 0) )
	{
		//Save state
		theDragPress = true;
	}
}
//----------------------------------------------------------------------------
void DebuggerTabBar::mouseReleaseEvent( QMouseEvent *event)
{
	//printf("TabBar Mouse Release: (%i,%i) \n", event->pos().x(), event->pos().y() );;
	QTabBar::mouseReleaseEvent(event);
	theDragPress = false;
	theDragOut = false;
}
//----------------------------------------------------------------------------
void DebuggerTabBar::contextMenuEvent(QContextMenuEvent *event)
{
	int idx;

	idx = tabAt(event->pos());

	if ( idx < 0 )
	{
		idx = currentIndex();
	}

	if ( idx != currentIndex() )
	{
		setCurrentIndex(idx);
	}
	DebuggerTabWidget *p = qobject_cast<DebuggerTabWidget*>(parent());

	if ( p )
	{
		p->buildContextMenu(event);
	}
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//---  Break On Count Setup Dialog
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DebugBreakOnDialog::DebugBreakOnDialog(int type, QWidget *parent )
	: QDialog(parent)
{
	
	QVBoxLayout *mainLayout, *vbox;
	QHBoxLayout *hbox;
	QGroupBox   *gbox;
	QGridLayout *grid;
	fceuDecIntValidtor *validator;
	int  refMode;
	bool oneShotMode;
	char stmp[128];
	QPushButton *btn;
	
	fceuWrapperLock();

	prevPauseState = FCEUI_EmulationPaused();

	if (prevPauseState == 0)
	{
		FCEUI_ToggleEmulationPause();
	}
	fceuWrapperUnLock();

	currLbl = new QLabel();

	this->type = type;

	if ( type )
	{
		totalCount = total_instructions;
		deltaCount = delta_instructions;

		setWindowTitle( tr("Break on CPU Instruction Exceedance") );
		oneShotMode = breakOnInstrOneShot;
		refMode     = breakOnInstrMode;
		threshold   = break_instructions_limit;

		sprintf(stmp, "Current Instruction Count: %10llu  (+%llu)", totalCount, deltaCount);
		currLbl->setText( tr(stmp) );

	}
	else
	{
		totalCount = timestampbase + (uint64)timestamp - total_cycles_base;
		deltaCount = timestampbase + (uint64)timestamp - delta_cycles_base;

		if (totalCount < 0)	// sanity check
		{
			ResetDebugStatisticsCounters();
			totalCount = 0;
		}
		if (deltaCount < 0)	// sanity check
		{
			ResetDebugStatisticsCounters();
			deltaCount = 0;
		}
		setWindowTitle( tr("Break on CPU Cycle Exceedance") );
		oneShotMode = breakOnCycleOneShot;
		refMode     = breakOnCycleMode;
		threshold   = break_cycles_limit;

		sprintf(stmp, "Current Cycle Count: %10llu  (+%llu)", totalCount, deltaCount);
		currLbl->setText( tr(stmp) );
	}

	mainLayout = new QVBoxLayout();
	hbox       = new QHBoxLayout();

	mainLayout->addLayout(hbox);

	gbox       = new QGroupBox( tr("Mode") );
	vbox       = new QVBoxLayout();
	oneShotBtn = new QRadioButton( tr("One-Shot") );
	contBtn    = new QRadioButton( tr("Continuous") );

	oneShotBtn->setChecked(  oneShotMode );
	   contBtn->setChecked( !oneShotMode );

	hbox->addWidget(gbox);
	gbox->setLayout(vbox);
	vbox->addWidget( oneShotBtn );
	vbox->addWidget( contBtn    );

	gbox       = new QGroupBox( tr("Reference") );
	vbox       = new QVBoxLayout();
	absBtn     = new QRadioButton( tr("Absolute") );
	relBtn     = new QRadioButton( tr("Relative") );

	absBtn->setChecked( refMode == 0 );
	relBtn->setChecked( refMode == 1 );

	hbox->addWidget(gbox);
	gbox->setLayout(vbox);
	vbox->addWidget( absBtn );
	vbox->addWidget( relBtn );

	mainLayout->addWidget(currLbl);

	hbox       = new QHBoxLayout();
	mainLayout->addLayout(hbox);

	validator = new fceuDecIntValidtor( 0, 0x7FFFFFFFFFFFFFFFLL, this);

	countEntryBox = new QLineEdit();
	countEntryBox->setValidator( validator );
	countEntryBox->setAlignment(Qt::AlignCenter);

	hbox->addWidget( new QLabel( tr("Threshold:") ), 1 );
	hbox->addWidget( countEntryBox, 100 );

	grid = new QGridLayout();
	mainLayout->addLayout( grid );

	int b = 1, bb, bbb;
	for (int col=0; col<3; col++)
	{
		int c;

		bb = 1;

		if ( b < 1000 )
		{
			c = ' ';
		}
		else if ( b < 1000000 )
		{
			c = 'k';
		}
		else
		{
			c = 'm';
		}

		for (int row=0; row<3; row++)
		{
			bbb = b * bb;

			btn = new QPushButton();

			grid->addWidget( btn, row, 4-(col*2) );

			sprintf( stmp, "%+i%c", -bb, c);

			btn->setText( tr(stmp) );

			connect( btn, &QPushButton::clicked, [ this, bbb ] { incrThreshold( -bbb ); } );

			btn = new QPushButton();

			grid->addWidget( btn, row, 4-(col*2)+1 );

			sprintf( stmp, "%+i%c", bb, c);

			btn->setText( tr(stmp) );

			connect( btn, &QPushButton::clicked, [ this, bbb ] { incrThreshold( bbb ); } );

			bb = bb * 10;
		}
		b = b * 1000;
	}

	btn = new QPushButton( tr("Sync to Current") );
	grid->addWidget( btn, 3, 4, 1, 2 );
	connect( btn, SIGNAL(clicked(void)), this, SLOT(syncToCurrent(void)) );

	btn = new QPushButton( tr("-1g") );
	grid->addWidget( btn, 3, 0, 1, 1 );
	connect( btn, &QPushButton::clicked, [ this ] { incrThreshold( -1e9 ); } );

	btn = new QPushButton( tr("+1g") );
	grid->addWidget( btn, 3, 1, 1, 1 );
	connect( btn, &QPushButton::clicked, [ this ] { incrThreshold(  1e9 ); } );

	descLbl = new QLabel();
	descLbl->setWordWrap(true);
	mainLayout->addWidget( descLbl );

	hbox = new QHBoxLayout();
	mainLayout->addLayout(hbox);

	btn = new QPushButton( tr("Reset All") );
	hbox->addWidget( btn, 1 );
	btn->setIcon( style()->standardIcon( QStyle::SP_DialogResetButton ) );
	connect( btn, SIGNAL(clicked(void)), this, SLOT(resetCounters(void)) );

	btn = new QPushButton( tr("Reset Deltas") );
	hbox->addWidget( btn, 1 );
	btn->setIcon( style()->standardIcon( QStyle::SP_DialogResetButton ) );
	connect( btn, SIGNAL(clicked(void)), this, SLOT(resetDeltas(void)) );

	hbox->addStretch(10);

	btn = new QPushButton( tr("Cancel") );
	hbox->addWidget( btn, 1 );
	btn->setIcon( style()->standardIcon( QStyle::SP_DialogCancelButton ) );
	connect( btn, SIGNAL(clicked(void)), this, SLOT(reject(void)) );

	btn = new QPushButton( tr("Ok") );
	hbox->addWidget( btn, 1 );
	btn->setIcon( style()->standardIcon( QStyle::SP_DialogOkButton ) );
	connect( btn, SIGNAL(clicked(void)), this, SLOT(accept(void)) );

	setLayout( mainLayout );

	setThreshold( threshold );

	connect( countEntryBox, SIGNAL(textChanged(const QString &)), this, SLOT(setThreshold(const QString &)) );

	connect( absBtn, SIGNAL(clicked(bool)), this, SLOT(refModeChanged(bool)) );
	connect( relBtn, SIGNAL(clicked(bool)), this, SLOT(refModeChanged(bool)) );
	connect( this  , SIGNAL(finished(int)), this, SLOT(closeWindow(int)) );
}
//----------------------------------------------------------------------------
DebugBreakOnDialog::~DebugBreakOnDialog(void)
{
	FCEUI_SetEmulationPaused(prevPauseState);
}
//----------------------------------------------------------------------------
void DebugBreakOnDialog::closeEvent(QCloseEvent *event)
{
	//printf("Close Window Event\n");
	done(QDialog::Rejected);
	deleteLater();
	event->accept();
}
//----------------------------------------------------------------------------
void DebugBreakOnDialog::closeWindow(int ret)
{
	if ( ret == QDialog::Accepted )
	{
		if ( type )
		{
			breakOnInstrOneShot = oneShotBtn->isChecked();
			breakOnInstrMode    =     relBtn->isChecked();

			if ( absBtn->isChecked() )
			{
				break_instructions_limit = threshold;
			}
			else
			{
				breakOnInstrRelVal = threshold;

				break_instructions_limit = totalCount + (breakOnInstrRelVal - deltaCount);
			}
			break_on_instructions = true;
		}
		else
		{
			breakOnCycleOneShot = oneShotBtn->isChecked();
			breakOnCycleMode    =     relBtn->isChecked();

			if ( absBtn->isChecked() )
			{
				break_cycles_limit = threshold;
			}
			else
			{
				breakOnCycleRelVal = threshold;

				break_cycles_limit = totalCount + (breakOnCycleRelVal - deltaCount);
			}
			break_on_cycles = true;
		}
	}
	else
	{
		if ( type )
		{
			break_on_instructions = false;
		}
		else
		{
			break_on_cycles = false;
		}
	}
	//printf("Close Window\n");
	deleteLater();
}
//----------------------------------------------------------------------------
void DebugBreakOnDialog::updateCurrent(void)
{
	char stmp[128];

	if ( type )
	{
		totalCount = total_instructions;
		deltaCount = delta_instructions;

		sprintf(stmp, "Current Instruction Count: %10llu  (+%llu)", totalCount, deltaCount);
		currLbl->setText( tr(stmp) );

	}
	else
	{
		totalCount = timestampbase + (uint64)timestamp - total_cycles_base;
		deltaCount = timestampbase + (uint64)timestamp - delta_cycles_base;

		if (totalCount < 0)	// sanity check
		{
			ResetDebugStatisticsCounters();
			totalCount = 0;
		}
		if (deltaCount < 0)	// sanity check
		{
			ResetDebugStatisticsCounters();
			deltaCount = 0;
		}
		sprintf(stmp, "Current Cycle Count: %10llu  (+%llu)", totalCount, deltaCount);
		currLbl->setText( tr(stmp) );
	}

}
//----------------------------------------------------------------------------
void DebugBreakOnDialog::resetCounters(void)
{
	ResetDebugStatisticsCounters();

	if ( dbgWin )
	{
		dbgWin->updateRegisterView();
	}
	setThreshold(0);
	updateCurrent();
	updateLabel();
}
//----------------------------------------------------------------------------
void DebugBreakOnDialog::resetDeltas(void)
{
	ResetDebugStatisticsDeltaCounters();

	if ( dbgWin )
	{
		dbgWin->updateRegisterView();
	}
	updateCurrent();
	updateLabel();
}
//----------------------------------------------------------------------------
void DebugBreakOnDialog::syncToCurrent(void)
{
	if ( absBtn->isChecked() )
	{
		setThreshold( totalCount );
	}
	else
	{
		setThreshold( deltaCount );
	}
}
//----------------------------------------------------------------------------
void DebugBreakOnDialog::incrThreshold( int ival )
{
	long long int ll = threshold + ival;

	if ( ll < 0 ) ll = 0;

	setThreshold( ll );
}
//----------------------------------------------------------------------------
void DebugBreakOnDialog::setThreshold( unsigned long long int val )
{
	char stmp[64];

	threshold = val;

	sprintf( stmp, "%llu", threshold );

	countEntryBox->setText( tr(stmp) );

	updateLabel();
}
//----------------------------------------------------------------------------
void DebugBreakOnDialog::setThreshold( const QString &text )
{
	threshold = strtoull( text.toStdString().c_str(), NULL, 10 );

	updateLabel();
}
//----------------------------------------------------------------------------
void DebugBreakOnDialog::refModeChanged(bool val)
{
	updateLabel();
}
//----------------------------------------------------------------------------
void DebugBreakOnDialog::updateLabel(void)
{
	long long delta;
	char stmp[256];

	stmp[0] = 0;

	if ( type )
	{
		if ( absBtn->isChecked() )
		{
			delta = threshold - total_instructions;

			if ( delta > 0 )
			{
				sprintf( stmp, "Will break in %lli CPU Instruction%s", delta, (delta > 1) ? "s":"" );
			}
			else
			{
				sprintf( stmp, "Will break immediately, CPU instruction count already exceeds value by %lli.", -delta);
			}
		}
		else
		{
			delta = threshold - delta_instructions;

			if ( delta > 0 )
			{
				sprintf( stmp, "Will break in %lli CPU Instruction%s", delta, (delta > 1) ? "s":"" );
			}
			else
			{
				sprintf( stmp, "Will break immediately, CPU instruction count already exceeds value by %lli.", -delta);
			}
		}
	}
	else
	{
		if ( absBtn->isChecked() )
		{
			delta = threshold - totalCount;

			if ( delta > 0 )
			{
				sprintf( stmp, "Will break in %lli CPU cycle%s", delta, (delta > 1) ? "s":"" );
			}
			else
			{
				sprintf( stmp, "Will break immediately, CPU cycle count already exceeds value by %lli.", -delta);
			}
		}
		else
		{
			delta = threshold - deltaCount;

			if ( delta > 0 )
			{
				sprintf( stmp, "Will break in %lli CPU cycle%s", delta, (delta > 1) ? "s":"" );
			}
			else
			{
				sprintf( stmp, "Will break immediately, CPU cycle count already exceeds value %lli.", -delta);
			}
		}
	}
	descLbl->setText( tr(stmp) );

}
//----------------------------------------------------------------------------
