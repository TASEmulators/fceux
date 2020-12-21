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
#include <QHeaderView>
#include <QCloseEvent>
#include <QGridLayout>
#include <QRadioButton>
#include <QInputDialog>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QGuiApplication>

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
#include "common/configSys.h"

#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/config.h"
#include "Qt/nes_shm.h"
#include "Qt/fceuWrapper.h"
#include "Qt/HexEditor.h"
#include "Qt/ConsoleDebugger.h"
#include "Qt/ConsoleUtilities.h"

// Where are these defined?
extern int vblankScanLines;
extern int vblankPixel;

debuggerBookmarkManager_t dbgBmMgr;
static std::list <ConsoleDebugger*> dbgWinList;

static void DeleteBreak(int sel);
static bool waitingAtBp = false;
static int  lastBpIdx   = 0;
//----------------------------------------------------------------------------
ConsoleDebugger::ConsoleDebugger(QWidget *parent)
	: QDialog( parent, Qt::Window )
{
	QHBoxLayout *mainLayout;
	QVBoxLayout *vbox, *vbox1, *vbox2, *vbox3, *vbox4;
	QHBoxLayout *hbox, *hbox1, *hbox2, *hbox3;
	QGridLayout *grid;
	QPushButton *button;
	QFrame      *frame;
	QLabel      *lbl;
	QMenuBar    *menuBar;
	QMenu       *debugMenu, *optMenu, *subMenu;
	QActionGroup *actGroup;
	QAction     *act;
	float fontCharWidth;
	QTreeWidgetItem * item;
	int opt, useNativeMenuBar;
	fceuDecIntValidtor *validator;

	font.setFamily("Courier New");
	font.setStyle( QFont::StyleNormal );
	font.setStyleHint( QFont::Monospace );
	QFontMetrics fm(font);

	fontCharWidth = 1.00 * fm.averageCharWidth();

	setWindowTitle("6502 Debugger");

	//resize( 512, 512 );
	
	menuBar = new QMenuBar(this);

   // This is needed for menu bar to show up on MacOS
	g_config->getOption( "SDL.UseNativeMenuBar", &useNativeMenuBar );

	menuBar->setNativeMenuBar( useNativeMenuBar ? true : false );

	//-----------------------------------------------------------------------
	// Menu Start
	//-----------------------------------------------------------------------
	// Debug
   debugMenu = menuBar->addMenu(tr("Debug"));

	// Debug -> Run
	act = new QAction(tr("Run"), this);
   act->setShortcut(QKeySequence( tr("F5") ) );
   act->setStatusTip(tr("Run"));
   connect( act, SIGNAL(triggered()), this, SLOT(debugRunCB(void)) );

   debugMenu->addAction(act);

	// Debug -> Step Into
	act = new QAction(tr("Step Into"), this);
   act->setShortcut(QKeySequence( tr("F11") ) );
   act->setStatusTip(tr("Step Into"));
   connect( act, SIGNAL(triggered()), this, SLOT(debugStepIntoCB(void)) );

   debugMenu->addAction(act);

	// Debug -> Step Out
	act = new QAction(tr("Step Out"), this);
   act->setShortcut(QKeySequence( tr("Shift+F11") ) );
   act->setStatusTip(tr("Step Out"));
   connect( act, SIGNAL(triggered()), this, SLOT(debugStepOutCB(void)) );

   debugMenu->addAction(act);

	// Debug -> Step Over
	act = new QAction(tr("Step Over"), this);
   act->setShortcut(QKeySequence( tr("F10") ) );
   act->setStatusTip(tr("Step Over"));
   connect( act, SIGNAL(triggered()), this, SLOT(debugStepOverCB(void)) );

   debugMenu->addAction(act);

	// Debug -> Run to Selected Line
	act = new QAction(tr("Run to Selected Line"), this);
   act->setShortcut(QKeySequence( tr("F1") ) );
   act->setStatusTip(tr("Run to Selected Line"));
   connect( act, SIGNAL(triggered()), this, SLOT(debugRunToCursorCB(void)) );

   debugMenu->addAction(act);

	// Debug -> Run Line
	act = new QAction(tr("Run Line"), this);
   act->setShortcut(QKeySequence( tr("F6") ) );
   act->setStatusTip(tr("Run Line"));
   connect( act, SIGNAL(triggered()), this, SLOT(debugRunLineCB(void)) );

   debugMenu->addAction(act);

	// Debug -> Run 128 Lines
	act = new QAction(tr("Run 128 Lines"), this);
   act->setShortcut(QKeySequence( tr("F7") ) );
   act->setStatusTip(tr("Run 128 Lines"));
   connect( act, SIGNAL(triggered()), this, SLOT(debugRunLine128CB(void)) );

   debugMenu->addAction(act);

	// Options
   optMenu = menuBar->addMenu(tr("Options"));

	// Options -> PC Position
	subMenu  = optMenu->addMenu(tr("PC Line Positioning"));
	actGroup = new QActionGroup(this);

	actGroup->setExclusive(true);

	g_config->getOption( "SDL.DebuggerPCPlacement", &opt );

	// Options -> PC Position -> Top Line
	act = new QAction(tr("Top Line"), this);
   act->setStatusTip(tr("Top Line"));
	act->setCheckable(true);
	act->setChecked( opt == 0 );
   connect( act, SIGNAL(triggered()), this, SLOT(pcSetPlaceTop(void)) );
	actGroup->addAction(act);
	subMenu->addAction(act);

	// Options -> PC Position -> Upper Mid-Line
	act = new QAction(tr("Upper Mid-Line"), this);
   act->setStatusTip(tr("Upper Mid-Line"));
	act->setCheckable(true);
	act->setChecked( opt == 1 );
   connect( act, SIGNAL(triggered()), this, SLOT(pcSetPlaceUpperMid(void)) );
	actGroup->addAction(act);
	subMenu->addAction(act);

	// Options -> PC Position -> Center Line
	act = new QAction(tr("Center Line"), this);
   act->setStatusTip(tr("Center Line"));
	act->setCheckable(true);
	act->setChecked( opt == 2 );
   connect( act, SIGNAL(triggered()), this, SLOT(pcSetPlaceCenter(void)) );
	actGroup->addAction(act);
	subMenu->addAction(act);

	// Options -> PC Position -> Lower Mid-Line
	act = new QAction(tr("Lower Mid-Line"), this);
   act->setStatusTip(tr("Lower Mid-Line"));
	act->setCheckable(true);
	act->setChecked( opt == 3 );
   connect( act, SIGNAL(triggered()), this, SLOT(pcSetPlaceLowerMid(void)) );
	actGroup->addAction(act);
	subMenu->addAction(act);

	// Options -> PC Position -> Bottom
	act = new QAction(tr("Bottom Line"), this);
   act->setStatusTip(tr("Bottom Line"));
	act->setCheckable(true);
	act->setChecked( opt == 4 );
   connect( act, SIGNAL(triggered()), this, SLOT(pcSetPlaceBottom(void)) );
	actGroup->addAction(act);
	subMenu->addAction(act);

	// Options -> PC Position -> Custom Line 
	act = new QAction(tr("Custom Line Offset"), this);
   act->setStatusTip(tr("Custom Line Offset"));
	act->setChecked( opt == 5 );
	act->setCheckable(true);
   connect( act, SIGNAL(triggered()), this, SLOT(pcSetPlaceCustom(void)) );
	actGroup->addAction(act);
	subMenu->addAction(act);

	//-----------------------------------------------------------------------
	// Menu End
	//-----------------------------------------------------------------------
	mainLayout = new QHBoxLayout();

	mainLayout->setMenuBar( menuBar );

	vbox4      = new QVBoxLayout();
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

	vbox1   = new QVBoxLayout();
	vbox2   = new QVBoxLayout();
	hbox1   = new QHBoxLayout();

	hbar->setMinimum(0);
	hbar->setMaximum(100);
	vbar->setMinimum(0);
	vbar->setMaximum( 0x10000 );

	vbox4->addLayout( grid, 100 );
	vbox4->addWidget( asmLineSelLbl, 1 );
	vbox4->addWidget( emuStatLbl   , 1 );
	//asmText->setFont(font);
	//asmText->setReadOnly(true);
	//asmText->setOverwriteMode(true);
	//asmText->setMinimumWidth( 20 * fontCharWidth );
	//asmText->setLineWrapMode( QPlainTextEdit::NoWrap );

	mainLayout->addLayout( vbox4, 10 );
	mainLayout->addLayout( vbox1, 1 );

	grid    = new QGridLayout();

	vbox1->addLayout( hbox1 );
	hbox1->addLayout( vbox2 );
	vbox2->addLayout( grid  );

	button = new QPushButton( tr("Run") );
	grid->addWidget( button, 0, 0, Qt::AlignLeft );
   connect( button, SIGNAL(clicked(void)), this, SLOT(debugRunCB(void)) );

	button = new QPushButton( tr("Step Into") );
	grid->addWidget( button, 0, 1, Qt::AlignLeft );
   connect( button, SIGNAL(clicked(void)), this, SLOT(debugStepIntoCB(void)) );

	button = new QPushButton( tr("Step Out") );
	grid->addWidget( button, 1, 0, Qt::AlignLeft );
   connect( button, SIGNAL(clicked(void)), this, SLOT(debugStepOutCB(void)) );

	button = new QPushButton( tr("Step Over") );
	grid->addWidget( button, 1, 1, Qt::AlignLeft );
   connect( button, SIGNAL(clicked(void)), this, SLOT(debugStepOverCB(void)) );

	button = new QPushButton( tr("Run Line") );
	grid->addWidget( button, 2, 0, Qt::AlignLeft );
   connect( button, SIGNAL(clicked(void)), this, SLOT(debugRunLineCB(void)) );

	button = new QPushButton( tr("128 Lines") );
	grid->addWidget( button, 2, 1, Qt::AlignLeft );
   connect( button, SIGNAL(clicked(void)), this, SLOT(debugRunLine128CB(void)) );

	button = new QPushButton( tr("Seek To:") );
	grid->addWidget( button, 3, 0, Qt::AlignLeft );
   connect( button, SIGNAL(clicked(void)), this, SLOT(seekToCB(void)) );

	seekEntry = new QLineEdit();
	seekEntry->setFont( font );
	seekEntry->setText("0000");
	seekEntry->setMaxLength( 4 );
	seekEntry->setInputMask( ">HHHH;" );
	seekEntry->setAlignment(Qt::AlignCenter);
	seekEntry->setMaximumWidth( 6 * fontCharWidth );
	grid->addWidget( seekEntry, 3, 1, Qt::AlignLeft );

	hbox = new QHBoxLayout();
	lbl  = new QLabel( tr("PC:") );
	pcEntry = new QLineEdit();
	pcEntry->setFont( font );
	pcEntry->setMaxLength( 4 );
	pcEntry->setInputMask( ">HHHH;" );
	pcEntry->setAlignment(Qt::AlignCenter);
	pcEntry->setMaximumWidth( 6 * fontCharWidth );
	hbox->addWidget( lbl );
	hbox->addWidget( pcEntry, 1, Qt::AlignLeft );
	grid->addLayout( hbox, 4, 0, Qt::AlignLeft );

	button = new QPushButton( tr("Seek PC") );
	grid->addWidget( button, 4, 1, Qt::AlignLeft );
   connect( button, SIGNAL(clicked(void)), this, SLOT(seekPCCB(void)) );

	hbox = new QHBoxLayout();
	lbl  = new QLabel( tr("A:") );
	regAEntry = new QLineEdit();
	regAEntry->setFont( font );
	regAEntry->setMaxLength( 2 );
	regAEntry->setInputMask( ">HH;" );
	regAEntry->setAlignment(Qt::AlignCenter);
	regAEntry->setMaximumWidth( 4 * fontCharWidth );
	hbox->addWidget( lbl );
	hbox->addWidget( regAEntry, 1, Qt::AlignLeft );
	lbl  = new QLabel( tr("X:") );
	regXEntry = new QLineEdit();
	regXEntry->setFont( font );
	regXEntry->setMaxLength( 2 );
	regXEntry->setInputMask( ">HH;" );
	regXEntry->setAlignment(Qt::AlignCenter);
	regXEntry->setMaximumWidth( 4 * fontCharWidth );
	hbox->addWidget( lbl );
	hbox->addWidget( regXEntry, 1, Qt::AlignLeft );
	lbl  = new QLabel( tr("Y:") );
	regYEntry = new QLineEdit();
	regYEntry->setFont( font );
	regYEntry->setMaxLength( 2 );
	regYEntry->setInputMask( ">HH;" );
	regYEntry->setAlignment(Qt::AlignCenter);
	regYEntry->setMaximumWidth( 4 * fontCharWidth );
	hbox->addWidget( lbl );
	hbox->addWidget( regYEntry, 1, Qt::AlignLeft );
	vbox2->addLayout( hbox );

	stackFrame = new QGroupBox(tr("Stack $0100"));
	stackText  = new DebuggerStackDisplay(this);
	hbox       = new QHBoxLayout();
	hbox->addWidget( stackText );
	vbox2->addWidget( stackFrame );
	stackFrame->setLayout( hbox );
	stackText->setFont(font);
	stackText->setReadOnly(true);
	stackText->setWordWrapMode( QTextOption::NoWrap );
	//stackText->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	//stackText->setMaximumWidth( 16 * fontCharWidth );

	bpFrame = new QGroupBox(tr("Breakpoints"));
	vbox3   = new QVBoxLayout();
	vbox    = new QVBoxLayout();
	hbox    = new QHBoxLayout();
	bpTree  = new QTreeWidget();

	bpTree->setColumnCount(2);
	bpTree->setSelectionMode( QAbstractItemView::SingleSelection );

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

	hbox->addWidget( bpTree );

	hbox   = new QHBoxLayout();
	button = new QPushButton( tr("Add") );
	hbox->addWidget( button );
   connect( button, SIGNAL(clicked(void)), this, SLOT(add_BP_CB(void)) );

	button = new QPushButton( tr("Delete") );
	hbox->addWidget( button );
   connect( button, SIGNAL(clicked(void)), this, SLOT(delete_BP_CB(void)) );

	button = new QPushButton( tr("Edit") );
	hbox->addWidget( button );
   connect( button, SIGNAL(clicked(void)), this, SLOT(edit_BP_CB(void)) );

	brkBadOpsCbox = new QCheckBox( tr("Break on Bad Opcodes") );
	brkBadOpsCbox->setChecked( FCEUI_Debugger().badopbreak );
   connect( brkBadOpsCbox, SIGNAL(stateChanged(int)), this, SLOT(breakOnBadOpcodeCB(int)) );

	vbox->addWidget( bpTree );
	vbox->addLayout( hbox   );
	vbox->addWidget( brkBadOpsCbox );
	bpFrame->setLayout( vbox );

	sfFrame = new QGroupBox(tr("Status Flags"));
	grid    = new QGridLayout();
	sfFrame->setLayout( grid );

	N_chkbox = new QCheckBox( tr("N") );
	V_chkbox = new QCheckBox( tr("V") );
	U_chkbox = new QCheckBox( tr("U") );
	B_chkbox = new QCheckBox( tr("B") );
	D_chkbox = new QCheckBox( tr("D") );
	I_chkbox = new QCheckBox( tr("I") );
	Z_chkbox = new QCheckBox( tr("Z") );
	C_chkbox = new QCheckBox( tr("C") );

	grid->addWidget( N_chkbox, 0, 0, Qt::AlignCenter );
	grid->addWidget( V_chkbox, 0, 1, Qt::AlignCenter );
	grid->addWidget( U_chkbox, 0, 2, Qt::AlignCenter );
	grid->addWidget( B_chkbox, 0, 3, Qt::AlignCenter );
	grid->addWidget( D_chkbox, 1, 0, Qt::AlignCenter );
	grid->addWidget( I_chkbox, 1, 1, Qt::AlignCenter );
	grid->addWidget( Z_chkbox, 1, 2, Qt::AlignCenter );
	grid->addWidget( C_chkbox, 1, 3, Qt::AlignCenter );

	vbox3->addWidget( bpFrame);
	vbox3->addWidget( sfFrame);
	hbox1->addLayout( vbox3  );

	hbox2       = new QHBoxLayout();
	vbox        = new QVBoxLayout();
	frame       = new QFrame();
	ppuLbl      = new QLabel( tr("PPU:") );
	spriteLbl   = new QLabel( tr("Sprite:") );
	scanLineLbl = new QLabel( tr("Scanline:") );
	pixLbl      = new QLabel( tr("Pixel:") );
	vbox->addWidget( ppuLbl );
	vbox->addWidget( spriteLbl );
	vbox->addWidget( scanLineLbl );
	vbox->addWidget( pixLbl );
	vbox1->addLayout( hbox2 );
	hbox2->addWidget( frame );
	frame->setLayout( vbox  );
	frame->setFrameShape( QFrame::Box );

	vbox          = new QVBoxLayout();
	cpuCyclesLbl1 = new QLabel( tr("CPU Cycles:") );
	cpuCyclesLbl2 = new QLabel( tr("(+0):") );
	cpuInstrsLbl1 = new QLabel( tr("Instructions:") );
	cpuInstrsLbl2 = new QLabel( tr("(+0):") );
	brkCpuCycExd  = new QCheckBox( tr("Break when Exceed") );
	brkInstrsExd  = new QCheckBox( tr("Break when Exceed") );
	cpuCycExdVal  = new QLineEdit( tr("0") );
	instrExdVal   = new QLineEdit( tr("0") );
	hbox          = new QHBoxLayout();
	vbox->addLayout( hbox );
	hbox->addWidget( cpuCyclesLbl1 );
	hbox->addWidget( cpuCyclesLbl2 );
	hbox         = new QHBoxLayout();
	vbox->addLayout( hbox );
	hbox->addWidget( brkCpuCycExd );
	hbox->addWidget( cpuCycExdVal, 1, Qt::AlignLeft );

	hbox         = new QHBoxLayout();
	vbox->addLayout( hbox );
	hbox->addWidget( cpuInstrsLbl1 );
	hbox->addWidget( cpuInstrsLbl2 );
	hbox         = new QHBoxLayout();
	vbox->addLayout( hbox );
	hbox->addWidget( brkInstrsExd );
	hbox->addWidget( instrExdVal, 1, Qt::AlignLeft );
	hbox2->addLayout( vbox );

	validator = new fceuDecIntValidtor( 0, 0x3FFFFFFF, this );
	cpuCycExdVal->setFont( font );
	cpuCycExdVal->setMaxLength( 16 );
	cpuCycExdVal->setValidator( validator );
	cpuCycExdVal->setAlignment(Qt::AlignLeft);
	cpuCycExdVal->setMaximumWidth( 18 * fontCharWidth );
   cpuCycExdVal->setCursorPosition(0);
	connect( cpuCycExdVal, SIGNAL(textEdited(const QString &)), this, SLOT(cpuCycleThresChanged(const QString &)));

	validator = new fceuDecIntValidtor( 0, 0x3FFFFFFF, this );
	instrExdVal->setFont( font );
	instrExdVal->setMaxLength( 16 );
	instrExdVal->setValidator( validator );
	instrExdVal->setAlignment(Qt::AlignLeft);
	instrExdVal->setMaximumWidth( 18 * fontCharWidth );
   instrExdVal->setCursorPosition(0);
	connect( instrExdVal, SIGNAL(textEdited(const QString &)), this, SLOT(instructionsThresChanged(const QString &)));

	brkCpuCycExd->setChecked( break_on_cycles );
   connect( brkCpuCycExd, SIGNAL(stateChanged(int)), this, SLOT(breakOnCyclesCB(int)) );

	brkInstrsExd->setChecked( break_on_instructions );
   connect( brkInstrsExd, SIGNAL(stateChanged(int)), this, SLOT(breakOnInstructionsCB(int)) );

	hbox3     = new QHBoxLayout();
	hbox      = new QHBoxLayout();
	vbox      = new QVBoxLayout();
	bmFrame   = new QGroupBox( tr("Address Bookmarks") );
	bmTree    = new QTreeWidget();
	selBmAddr = new QLineEdit();
	selBmAddrVal = 0;

	connect( selBmAddr, SIGNAL(textChanged(const QString &)), this, SLOT(selBmAddrChanged(const QString &)));

	bmTree->setColumnCount(2);

	item = new QTreeWidgetItem();
	item->setFont( 0, font );
	item->setFont( 1, font );
	item->setText( 0, QString::fromStdString( "Addr" ) );
	item->setText( 1, QString::fromStdString( "Name" ) );
	item->setTextAlignment( 0, Qt::AlignCenter);
	item->setTextAlignment( 1, Qt::AlignCenter);

	bmTree->setHeaderItem( item );

	bmTree->header()->setSectionResizeMode( QHeaderView::ResizeToContents );

	connect( bmTree, SIGNAL(itemClicked(QTreeWidgetItem*, int)),
			   this, SLOT(bmItemClicked( QTreeWidgetItem*, int)) );

	connect( bmTree, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)),
			   this, SLOT(bmItemDoubleClicked( QTreeWidgetItem*, int)) );

	vbox->addWidget( selBmAddr );

	button    = new QPushButton( tr("Add") );
	vbox->addWidget( button );
   connect( button, SIGNAL(clicked(void)), this, SLOT(add_BM_CB(void)) );

	button    = new QPushButton( tr("Delete") );
	vbox->addWidget( button );
   connect( button, SIGNAL(clicked(void)), this, SLOT(delete_BM_CB(void)) );

	button    = new QPushButton( tr("Name") );
	vbox->addWidget( button );
   connect( button, SIGNAL(clicked(void)), this, SLOT(edit_BM_CB(void)) );

	hbox->addWidget( bmTree );
	hbox->addLayout( vbox   );
	bmFrame->setLayout( hbox );
	hbox3->addWidget( bmFrame );
	vbox1->addLayout( hbox3 );

	frame        = new QFrame();
	vbox         = new QVBoxLayout();
	button       = new QPushButton( tr("Reset Counters") );
   connect( button, SIGNAL(clicked(void)), this, SLOT(resetCountersCB(void)) );
	vbox->addWidget( button );
	vbox->addWidget( frame  );
	hbox3->addLayout( vbox  );

	vbox         = new QVBoxLayout();
	romOfsChkBox = new QCheckBox( tr("ROM Offsets") );
	symDbgChkBox = new QCheckBox( tr("Symbolic Debug") );
	regNamChkBox = new QCheckBox( tr("Register Names") );
	vbox->addWidget( romOfsChkBox );
	vbox->addWidget( symDbgChkBox );
	vbox->addWidget( regNamChkBox );

	symDbgChkBox->setChecked(true);
	regNamChkBox->setChecked(true);

   connect( romOfsChkBox, SIGNAL(stateChanged(int)), this, SLOT(displayROMoffsetCB(int)) );
   connect( symDbgChkBox, SIGNAL(stateChanged(int)), this, SLOT(symbolDebugEnableCB(int)) );
   connect( regNamChkBox, SIGNAL(stateChanged(int)), this, SLOT(registerNameEnableCB(int)) );


	button       = new QPushButton( tr("Reload Symbols") );
	vbox->addWidget( button );
   connect( button, SIGNAL(clicked(void)), this, SLOT(reloadSymbolsCB(void)) );

	button       = new QPushButton( tr("ROM Patcher") );
	vbox->addWidget( button );
	button->setEnabled(false); // TODO

	frame->setLayout( vbox );
	frame->setFrameShape( QFrame::Box );

	hbox      = new QHBoxLayout();
	vbox1->addLayout( hbox );

	button         = new QPushButton( tr("Default Window Size") );
	autoOpenChkBox = new QCheckBox( tr("Auto-Open") );
	debFileChkBox  = new QCheckBox( tr("DEB Files") );
	idaFontChkBox  = new QCheckBox( tr("IDA Font") );
	hbox->addWidget( button );
	hbox->addWidget( autoOpenChkBox );
	hbox->addWidget( debFileChkBox  );
	hbox->addWidget( idaFontChkBox  );

	g_config->getOption( "SDL.AutoOpenDebugger", &opt );
	autoOpenChkBox->setChecked( opt );

	g_config->getOption( "SDL.AutoLoadDebugFiles", &opt );
	debFileChkBox->setChecked( opt );

   connect( autoOpenChkBox, SIGNAL(stateChanged(int)), this, SLOT(autoOpenDebugCB(int)) );
   connect( debFileChkBox , SIGNAL(stateChanged(int)), this, SLOT(debFileAutoLoadCB(int)) );

	button->setEnabled(false); // TODO

	// IDA font is just a monospace font, we are forcing this anyway. It is just easier to read the assembly.
	// If a different font is desired, my thought is to open a QFontDialog and let the user pick a new font,
	// rather than use a checkbox that selects between two. But for the moment, I have more important things
	// to do.
	idaFontChkBox->setEnabled(false); 
	idaFontChkBox->setChecked(true); 

	setLayout( mainLayout );

	windowUpdateReq   = true;

	dbgWinList.push_back( this );

	periodicTimer  = new QTimer( this );

   connect( periodicTimer, &QTimer::timeout, this, &ConsoleDebugger::updatePeriodic );
	connect( hbar, SIGNAL(valueChanged(int)), this, SLOT(hbarChanged(int)) );
   connect( vbar, SIGNAL(valueChanged(int)), this, SLOT(vbarChanged(int)) );

	bpListUpdate( false );

	periodicTimer->start( 100 ); // 10hz

	// If this is the first debug window to open, load breakpoints fresh
	if ( dbgWinList.size() == 1 )
	{ 
		int autoLoadDebug;

		g_config->getOption( "SDL.AutoLoadDebugFiles", &autoLoadDebug );

		if ( autoLoadDebug )
		{
			loadGameDebugBreakpoints();
		}
	}
}
//----------------------------------------------------------------------------
ConsoleDebugger::~ConsoleDebugger(void)
{
	std::list <ConsoleDebugger*>::iterator it;

	printf("Destroy Debugger Window\n");
	periodicTimer->stop();

	for (it = dbgWinList.begin(); it != dbgWinList.end(); it++)
	{
		if ( (*it) == this )
		{
			dbgWinList.erase(it);
			printf("Removing Debugger Window\n");
			break;
		}
	}

	if ( dbgWinList.size() == 0 )
	{
		saveGameDebugBreakpoints();
		debuggerClearAllBreakpoints();
	}
}
//----------------------------------------------------------------------------
void ConsoleDebugger::closeEvent(QCloseEvent *event)
{
   printf("Debugger Close Window Event\n");
   done(0);
	deleteLater();
   event->accept();
}
//----------------------------------------------------------------------------
void ConsoleDebugger::closeWindow(void)
{
   //printf("Close Window\n");
   done(0);
	deleteLater();
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
void ConsoleDebugger::openBpEditWindow( int editIdx, watchpointinfo *wp )
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
	QRadioButton *cpu_radio, *ppu_radio, *sprite_radio;

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
	cpu_radio    = new QRadioButton( tr("CPU Mem") );
	ppu_radio    = new QRadioButton( tr("PPU Mem") );
	sprite_radio = new QRadioButton( tr("Sprite Mem") );
	cpu_radio->setChecked(true);

	gbox->setLayout( hbox );
	hbox->addWidget( cpu_radio );
	hbox->addWidget( ppu_radio );
	hbox->addWidget( sprite_radio );

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
			sprite_radio->setChecked(true);
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
				// If new breakpoint, suggest condition if in ROM Mapping area of memory.
				if ( wp->address >= 0x8000 )
				{
					char str[64];
					sprintf(str, "K==#%02X", getBank(wp->address));
					cond->setText( tr(str) );
				}
			}
		}

		if ( wp->desc )
		{
			name->setText( tr(wp->desc) );
		}
	}

	dialog.setLayout( mainLayout );

	ret = dialog.exec();

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
		else if ( sprite_radio->isChecked() ) 
		{
			type |= BT_S;
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

   dialog.show();
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
void ConsoleDebugger::add_BP_CB(void)
{
	openBpEditWindow(-1);
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
}
//----------------------------------------------------------------------------
void ConsoleDebugger::breakOnBadOpcodeCB(int value)
{
	//printf("Value:%i\n", value);
	FCEUI_Debugger().badopbreak = (value != Qt::Unchecked);
}
//----------------------------------------------------------------------------
void ConsoleDebugger::breakOnCyclesCB( int value )
{
	std::string s;

	break_on_cycles = (value != Qt::Unchecked);

	s = cpuCycExdVal->text().toStdString();

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
void ConsoleDebugger::breakOnInstructionsCB( int value )
{
	std::string s;

	break_on_instructions = (value != Qt::Unchecked);

	s = instrExdVal->text().toStdString();

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
void ConsoleDebugger::displayROMoffsetCB( int value )
{
	asmView->setDisplayROMoffsets(value != Qt::Unchecked);
}
//----------------------------------------------------------------------------
void ConsoleDebugger::symbolDebugEnableCB( int value )
{
	asmView->setSymbolDebugEnable(value != Qt::Unchecked);
}
//----------------------------------------------------------------------------
void ConsoleDebugger::registerNameEnableCB( int value )
{
	asmView->setRegisterNameEnable(value != Qt::Unchecked);
}
//----------------------------------------------------------------------------
void ConsoleDebugger::autoOpenDebugCB( int value )
{
	g_config->setOption( "SDL.AutoOpenDebugger", value != Qt::Unchecked);
}
//----------------------------------------------------------------------------
void ConsoleDebugger::debFileAutoLoadCB( int value )
{
	int autoLoadDebug = value != Qt::Unchecked;

	g_config->setOption("SDL.AutoLoadDebugFiles", autoLoadDebug);

	if ( autoLoadDebug && (numWPs == 0) )
	{
		loadGameDebugBreakpoints();
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

   dialog.show();
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
			printf("Step Out is currently in process.\n");
			return;
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
void ConsoleDebugger::seekToCB (void)
{
	std::string s;

	s = seekEntry->text().toStdString();

	//printf("Seek To: '%s'\n", s.c_str() );

	if ( s.size() > 0 )
	{
		long int addr, line;

		addr = strtol( s.c_str(), NULL, 16 );
		
		line = asmView->getAsmLineFromAddr(addr);

		asmView->setLine( line );
		vbar->setValue( line );
	}
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
void ConsoleDebugger::asmViewCtxMenuAddBP(void)
{
	watchpointinfo wp;

	wp.address = asmView->getCtxMenuAddr();
	wp.endaddress = 0;
	wp.flags   = WP_X | WP_E;
	wp.condText = 0;
	wp.desc = NULL;

	openBpEditWindow( -1, &wp );

}
//----------------------------------------------------------------------------
void ConsoleDebugger::asmViewCtxMenuAddBM(void)
{
	int addr = asmView->getCtxMenuAddr();

	dbgBmMgr.addBookmark( addr );

	edit_BM_name( addr );

	bmListUpdate(false);
}
//----------------------------------------------------------------------------
void ConsoleDebugger::asmViewCtxMenuOpenHexEdit(void)
{
	int romAddr = -1;
	int addr = asmView->getCtxMenuAddr();

	if (addr >= 0x8000)
	{
		romAddr  = GetNesFileAddress(addr);
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
	openDebugSymbolEditWindow( asmView->getCtxMenuAddr() );
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
				if ( asmEntry[line]->addr > nextLine )
				{
					break;
				}
				line = nextLine;
			}
			else if ( asmEntry[line]->addr > addr )
			{
				nextLine = line - 1;
				if ( asmEntry[line]->addr < nextLine )
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
		asmFlags |= ASM_DEBUG_SYMS;

		if ( registerNameEnable )
		{
			asmFlags |= ASM_DEBUG_REGS;
		}
	}
	//asmText->clear();

	for (int i=0; i < 0xFFFF; i++)
	{
		line.clear();

		// PC pointer
		if (addr > 0xFFFF) break;

		a = new dbg_asm_entry_t;

		instruction_addr = addr;

		if ( !pc_found )
		{
			if (addr > X.PC)
			{
				asmPC = a;
				line.assign(">");
				pc_found = 1;
			}
			else if (addr == X.PC)
			{
				asmPC = a;
				line.assign(">");
				pc_found = 1;
			} 
			else
			{
				line.assign(" ");
			}
		}
		else 
		{
			line.assign(" ");
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

		size = opsize[GetMem(addr)];
		if (size == 0)
		{
			sprintf(chr, "%02X        UNDEFINED", GetMem(addr++));
			line.append(chr);
		} else
		{
			if ((addr + size) > 0xFFFF)
			{
				while (addr < 0xFFFF)
				{
					sprintf(chr, "%02X        OVERFLOW\n", GetMem(addr++));
					line.append(chr);
				}
				break;
			}
			for (int j = 0; j < size; j++)
			{
				sprintf(chr, "%02X ", opcode[j] = GetMem(addr++));
				line.append(chr);
			}
			while (size < 3)
			{
				line.append("   ");  //pad output to align ASM
				size++;
			}

			DisassembleWithDebug(addr, opcode, asmFlags, asmTxt, &a->sym);

			line.append( asmTxt );
		}
		for (int j=0; j<size; j++)
		{
			a->opcode[j] = opcode[j];
		}
		a->size = size;

		// special case: an RTS opcode
		if (GetMem(instruction_addr) == 0x60)
		{
			line.append("-------------------------");
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
					d->text.assign( dbgSym->name );
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

	pxLineWidth = maxLineLen * pxCharWidth;

	setMinimumWidth( 50 * pxCharWidth );

	vbar->setMaximum( asmEntry.size() );
}
//----------------------------------------------------------------------------
void ConsoleDebugger::setRegsFromEntry(void)
{
	std::string s;
	long int i;

	s = pcEntry->text().toStdString();
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

	s = regAEntry->text().toStdString();
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

	s = regXEntry->text().toStdString();
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

	s = regYEntry->text().toStdString();
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
	int64 counter_value = timestampbase + (uint64)timestamp - total_cycles_base;
	if (counter_value < 0)	// sanity check
	{
		ResetDebugStatisticsCounters();
		counter_value = 0;
	}
	sprintf( stmp, "CPU Cycles: %llu", counter_value);

	cpuCyclesLbl1->setText( tr(stmp) );

	counter_value = timestampbase + (uint64)timestamp - delta_cycles_base;
	if (counter_value < 0)	// sanity check
	{
		ResetDebugStatisticsCounters();
		counter_value = 0;
	}
	sprintf(stmp, "(+%llu)", counter_value);

	cpuCyclesLbl2->setText( tr(stmp) );

	sprintf(stmp, "Instructions: %llu", total_instructions);
	cpuInstrsLbl1->setText( tr(stmp) );

	sprintf(stmp, "(+%llu)", delta_instructions);
	cpuInstrsLbl2->setText( tr(stmp) );

	// PPU Labels
	sprintf(stmp, "PPU: 0x%04X", (int)FCEUPPU_PeekAddress());
	ppuLbl->setText( tr(stmp) );

	sprintf(stmp, "Sprite: 0x%02X", PPU[3] );
	spriteLbl->setText( tr(stmp) );

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

	sprintf( stmp, "Scanline: %s", str );
	scanLineLbl->setText( tr(stmp) );

	sprintf( stmp, "Pixel: %s", str2 );
	pixLbl->setText( tr(stmp) );

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
		emuStatLbl->setText( tr(" Emulator Stopped / Paused") );
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

	if ( waitingAtBp && (lastBpIdx == BREAK_TYPE_INSTRUCTIONS_EXCEED) )
	{
		cpuInstrsLbl1->setStyleSheet("background-color: blue; color: white;");
	}
	else
	{
		cpuInstrsLbl1->setStyleSheet(NULL);
	}

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
}
//----------------------------------------------------------------------------
void ConsoleDebugger::vbarChanged(int value)
{
	//printf("VBar Changed: %i\n", value);
	asmView->setLine( value );
}
//----------------------------------------------------------------------------
void FCEUD_DebugBreakpoint( int bpNum )
{
	std::list <ConsoleDebugger*>::iterator it;

	if ( !nes_shm->runEmulator )
	{
		return;
	}
	lastBpIdx   = bpNum;
	waitingAtBp = true;

	printf("Breakpoint Hit: %i \n", bpNum );

	fceuWrapperUnLock();

	for (it=dbgWinList.begin(); it!=dbgWinList.end(); it++)
	{
		(*it)->breakPointNotify( bpNum );
	}

	while ( nes_shm->runEmulator && FCEUI_EmulationPaused() && !FCEUI_EmulationFrameStepped())
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
		usleep(16667);
	}
	// since we unfreezed emulation, reset delta_cycles counter
	ResetDebugStatisticsDeltaCounters();

	fceuWrapperLock();

	waitingAtBp = false;
}
//----------------------------------------------------------------------------
bool debuggerWindowIsOpen(void)
{
	return (dbgWinList.size() > 0);
}
//----------------------------------------------------------------------------
void updateAllDebuggerWindows( void )
{
	std::list <ConsoleDebugger*>::iterator it;

	for (it=dbgWinList.begin(); it!=dbgWinList.end(); it++)
	{
		(*it)->queueUpdate();
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
void saveGameDebugBreakpoints(void)
{
	int i;
	FILE *fp;
	char stmp[512];
	char flags[8];
   debuggerBookmark_t *bm;

	// If no breakpoints are loaded, skip saving
	if ( (numWPs == 0) && (dbgBmMgr.size() == 0) )
	{
		return;
	}
	if ( getGameDebugBreakpointFileName( stmp ) )
	{
		return;
	}

	//printf("Debug Save File: '%s' \n", stmp );

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
		else
		{
			flags[1] = 'C';
		}

		flags[2] = (watchpoint[i].flags & WP_R) ? 'R' : '-';
		flags[3] = (watchpoint[i].flags & WP_W) ? 'W' : '-';
		flags[4] = (watchpoint[i].flags & WP_X) ? 'X' : '-';
		flags[5] = (watchpoint[i].flags & WP_F) ? 'F' : '-';
		flags[6] = 0;

		fprintf( fp, "BreakPoint: startAddr=%04X  endAddr=%04X  flags=%s  condition=\"%s\"  desc=\"%s\" \n",
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
	if ( dbgWinList.size() == 0 )
	{
		printf("No Debug Windows Open: Skipping loading of breakpoint data\n");
		return;
	}
	if ( getGameDebugBreakpointFileName( stmp ) )
	{
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

	useDarkTheme = false;

	font.setFamily("Courier New");
	font.setStyle( QFont::StyleNormal );
	font.setStyleHint( QFont::Monospace );

	pal = this->palette();

	//c = pal.color(QPalette::Base);
	//printf("Base: R:%i  G:%i  B:%i \n", c.red(), c.green(), c.blue() );

	//c = pal.color(QPalette::Background);
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
		pal.setColor(QPalette::Background, fg );
		pal.setColor(QPalette::WindowText, bg );
	}
	else 
	{
		pal.setColor(QPalette::Base      , bg );
		pal.setColor(QPalette::Background, bg );
		pal.setColor(QPalette::WindowText, fg );
	}

	this->parent = (ConsoleDebugger*)parent;
	this->setPalette(pal);
	this->setMouseTracking(true);

	calcFontData();

	vbar = NULL;
	hbar = NULL;
	asmPC = NULL;
	displayROMoffsets = false;
	symbolicDebugEnable = true;
	registerNameEnable = true;
	maxLineLen = 0;
	pxLineWidth = 0;
	lineOffset = 0;
	maxLineOffset = 0;
	ctxMenuAddr = -1;

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
	memset( selAddrText, 0, sizeof(selAddrText) );

	cursorLineAddr    = -1;
	wheelPixelCounter =  0;

	//setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Expanding );
	setFocusPolicy(Qt::StrongFocus);

	clipboard = QGuiApplication::clipboard();

	//printf("clipboard->supportsSelection() : '%i' \n", clipboard->supportsSelection() );
	//printf("clipboard->supportsFindBuffer(): '%i' \n", clipboard->supportsFindBuffer() );

}
//----------------------------------------------------------------------------
QAsmView::~QAsmView(void)
{
	asmClear();
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
		pxLineXScroll = (int)(0.010f * (float)value * (float)(pxLineWidth - viewWidth) );
	}
}
//----------------------------------------------------------------------------
void QAsmView::scrollToPC(void)
{
	if ( asmPC != NULL )
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

		lineOffset = asmPC->line - ofs;

		if ( lineOffset < 0 )
		{
			lineOffset = 0;
		}
		vbar->setValue( lineOffset );
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
void QAsmView::calcFontData(void)
{
	 this->setFont(font);
    QFontMetrics metrics(font);
#if QT_VERSION > QT_VERSION_CHECK(5, 11, 0)
    pxCharWidth = metrics.horizontalAdvance(QLatin1Char('2'));
#else
    pxCharWidth = metrics.width(QLatin1Char('2'));
#endif
    pxCharHeight   = metrics.height();
	 pxLineSpacing  = metrics.lineSpacing() * 1.25;
    pxLineLead     = pxLineSpacing - pxCharHeight;
    pxCursorHeight = pxCharHeight;

	 viewLines   = (viewHeight / pxLineSpacing) + 1;
}
//----------------------------------------------------------------------------
void QAsmView::setScrollBars( QScrollBar *h, QScrollBar *v )
{
	hbar = h; vbar = v;
}
//----------------------------------------------------------------------------
void QAsmView::resizeEvent(QResizeEvent *event)
{
	viewWidth  = event->size().width();
	viewHeight = event->size().height();

	//printf("QAsmView Resize: %ix%i\n", viewWidth, viewHeight );

	viewLines = (viewHeight / pxLineSpacing) + 1;

	maxLineOffset = 0; // mb.numLines() - viewLines + 1;

	if ( viewWidth >= pxLineWidth )
	{
		pxLineXScroll = 0;
	}
	else
	{
		pxLineXScroll = (int)(0.010f * (float)hbar->value() * (float)(pxLineWidth - viewWidth) );
	}

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

		if ( event->key() == Qt::Key_B )
   	{
			parent->asmViewCtxMenuAddBP();
			event->accept();
   	}
		else if ( event->key() == Qt::Key_S )
   	{
			parent->asmViewCtxMenuAddSym();
			event->accept();
   	}
		else if ( event->key() == Qt::Key_M )
   	{
			parent->asmViewCtxMenuAddBM();
			event->accept();
   	}
		else if ( event->key() == Qt::Key_H )
   	{
			parent->asmViewCtxMenuOpenHexEdit();
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
		float x = (float)p.x() / pxCharWidth;

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
		mouseLeftBtnDown = false;
		setHighlightEndCoord( c.x(), line );

		loadHighlightToClipboard();
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
	}
	
	selAddrLine  = -1;
	selAddrChar  =  0;
	selAddrWidth =  0;
	selAddrValue = -1;
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
					size_t subStrLoc = asmEntry[line]->text.find( asmEntry[line]->sym.name, 22 );

					if ( (subStrLoc != std::string::npos) && (subStrLoc > 22) )
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

						selAddrLine  = line;
						selAddrChar  = addrTextLoc;
						selAddrWidth = j;
						selAddrValue = addr;
						strcpy( selAddrText, stmp );
					}
				}
			}
		}
		else if ( asmEntry[line]->type == dbg_asm_entry_t::SYMBOL_NAME )
		{
			selAddrLine  = line;
			selAddrChar  = 0;
			selAddrValue = addr = asmEntry[line]->addr;

			if ( asmEntry[line]->text.size() > 0 )
			{
				selAddrWidth = asmEntry[line]->text.size()-1;
			}
			else
			{
				selAddrWidth = 0;
			}
			if ( selAddrWidth >= (int)sizeof(selAddrText) )
			{
				selAddrWidth = sizeof(selAddrText)-1;
			}
			strncpy( selAddrText, asmEntry[line]->text.c_str(), selAddrWidth );
			selAddrText[ selAddrWidth ] = 0;
		}

		if ( addr < 0 )
		{
			addr = asmEntry[line]->addr;
			selAddrLine  = line;
			selAddrChar  = 4;
			selAddrWidth = 4;
			selAddrValue = addr;
			sprintf( selAddrText, "%04X", addr );
		}
		//printf("Line: '%s'\n", asmEntry[line]->text.c_str() );

		if ( addr >= 0 )
		{
			parent->setBookmarkSelectedAddress( addr );
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

	line = lineOffset + c.y();

	ctxMenuAddr = -1;

	if ( line < asmEntry.size() )
	{
		int addr;

		if ( selAddrValue < 0 )
		{
			ctxMenuAddr = addr = asmEntry[line]->addr;

			enableRunToCursor = true;
		}
		else
		{
			ctxMenuAddr = addr = selAddrValue;

			enableRunToCursor = (selAddrValue == asmEntry[line]->addr);
		}

		if ( enableRunToCursor )
		{
			act = new QAction(tr("Run To Cursor"), &menu);
			menu.addAction(act);
			//act->setShortcut( QKeySequence(tr("Ctrl+F10")));
			connect( act, SIGNAL(triggered(void)), parent, SLOT(asmViewCtxMenuRunToCursor(void)) );
		}

		act = new QAction(tr("Add Breakpoint"), &menu);
		menu.addAction(act);
		act->setShortcut( QKeySequence(tr("B")));
		connect( act, SIGNAL(triggered(void)), parent, SLOT(asmViewCtxMenuAddBP(void)) );

		act = new QAction(tr("Add Symbolic Debug Marker"), &menu);
	 	menu.addAction(act);
		act->setShortcut( QKeySequence(tr("S")));
		connect( act, SIGNAL(triggered(void)), parent, SLOT(asmViewCtxMenuAddSym(void)) );

		act = new QAction(tr("Add Bookmark"), &menu);
	 	menu.addAction(act);
		act->setShortcut( QKeySequence(tr("M")));
		connect( act, SIGNAL(triggered(void)), parent, SLOT(asmViewCtxMenuAddBM(void)) );
		
		act = new QAction(tr("Open Hex Editor"), &menu);
	 	menu.addAction(act);
		act->setShortcut( QKeySequence(tr("H")));
		connect( act, SIGNAL(triggered(void)), parent, SLOT(asmViewCtxMenuOpenHexEdit(void)) );
		
		menu.exec(event->globalPos());
	}
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
void QAsmView::paintEvent(QPaintEvent *event)
{
	int x,y,l, row, nrow, selAddr;
	QPainter painter(this);
	QColor white("white"), black("black"), blue("blue");
	QColor hlgtFG("white"), hlgtBG("blue");
	bool forceDarkColor = false;
	bool txtHlgtSet = false;

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
	selAddr = parent->getBookmarkSelectedAddress();

	painter.fillRect( 0, 0, viewWidth, viewHeight, this->palette().color(QPalette::Background) );

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
				painter.fillRect( 0, y - pxLineSpacing + pxLineLead, viewWidth, pxLineSpacing, QColor("pink") );
				forceDarkColor = true;
			}
		}

		if ( l < asmEntry.size() )
		{
			if ( asmEntry[l]->type != dbg_asm_entry_t::ASM_TEXT )
			{
				painter.fillRect( 0, y - pxLineSpacing + pxLineLead, viewWidth, pxLineSpacing, QColor("light blue") );
				forceDarkColor = true;
			}

			if ( forceDarkColor )
			{
				painter.setPen( black );
			}
			else
			{
				painter.setPen( this->palette().color(QPalette::WindowText));
			}
			drawText( &painter, x, y, asmEntry[l]->text.c_str() );

			if ( (selAddrLine == l) )
			{	// Highlight ASM line for selected address.
				if ( !txtHlgtSet && (selAddr == selAddrValue) && 
				  	    (asmEntry[l]->text.size() >= (selAddrChar + selAddrWidth) ) && 
						    ( asmEntry[l]->text.compare( selAddrChar, selAddrWidth, selAddrText ) == 0 ) )
				{
					int ax;

					ax = x + selAddrChar*pxCharWidth;

					painter.fillRect( ax, y - pxLineSpacing + pxLineLead, selAddrWidth*pxCharWidth, pxLineSpacing, blue );

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

				painter.fillRect( ax, y - pxLineSpacing + pxLineLead, hlgtXd * pxCharWidth, pxLineSpacing, hlgtBG );

				drawText( &painter, ax, y, s.c_str() );
			}
			y += pxLineSpacing;
		}
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
   stackBytesPerLine = 4;
   showAddrs = true;
}
//----------------------------------------------------------------------------
DebuggerStackDisplay::~DebuggerStackDisplay(void)
{

}
//----------------------------------------------------------------------------
void DebuggerStackDisplay::keyPressEvent(QKeyEvent *event)
{
	//printf("Debug Stack Window Key Press: 0x%x \n", event->key() );

   if ( (event->key() >= Qt::Key_1) && ( (event->key() < Qt::Key_9) ) )
   {
      stackBytesPerLine = event->key() - Qt::Key_0;
      updateText();
   }
   else if ( event->key() == Qt::Key_A )
   {
      showAddrs = !showAddrs;
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

   updateText();
}
//----------------------------------------------------------------------------
void DebuggerStackDisplay::sel1BytePerLine(void)
{
   stackBytesPerLine = 1;
   updateText();
}
//----------------------------------------------------------------------------
void DebuggerStackDisplay::sel2BytesPerLine(void)
{
   stackBytesPerLine = 2;
   updateText();
}
//----------------------------------------------------------------------------
void DebuggerStackDisplay::sel3BytesPerLine(void)
{
   stackBytesPerLine = 3;
   updateText();
}
//----------------------------------------------------------------------------
void DebuggerStackDisplay::sel4BytesPerLine(void)
{
   stackBytesPerLine = 4;
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
		   sprintf( stmp, " %02X", GetMem(stackPtr) );
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
			   	   sprintf( stmp, "\n %02X", GetMem(stackPtr) );
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
}
//----------------------------------------------------------------------------
