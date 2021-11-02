/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2021 mjbudd77
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
// TasEditorWindow.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <QDir>
#include <QPainter>
#include <QSettings>
#include <QHeaderView>
#include <QMessageBox>
#include <QFontMetrics>
#include <QFileDialog>
#include <QStandardPaths>

#include "fceu.h"
#include "movie.h"
#include "driver.h"

#include "Qt/config.h"
#include "Qt/throttle.h"
#include "Qt/fceuWrapper.h"
#include "Qt/ConsoleUtilities.h"
#include "Qt/TasEditor/TasEditorWindow.h"

TasEditorWindow   *tasWin = NULL;
TASEDITOR_PROJECT *project = NULL;
TASEDITOR_CONFIG  *taseditorConfig = NULL;
TASEDITOR_LUA     *taseditor_lua = NULL;
MARKERS_MANAGER   *markersManager = NULL;
SELECTION         *selection = NULL;
GREENZONE         *greenzone = NULL;
BOOKMARKS         *bookmarks = NULL;
BRANCHES          *branches = NULL;
PLAYBACK          *playback = NULL;
RECORDER          *recorder = NULL;
HISTORY           *history = NULL;
SPLICER           *splicer = NULL;

// Piano Roll Definitions
#define BOOKMARKS_WITH_BLUE_ARROW 20
#define BOOKMARKS_WITH_GREEN_ARROW 40
#define BLUE_ARROW_IMAGE_ID 60
#define GREEN_ARROW_IMAGE_ID 61
#define GREEN_BLUE_ARROW_IMAGE_ID 62

// Piano Roll Colors
#define NORMAL_TEXT_COLOR 0x0
#define NORMAL_BACKGROUND_COLOR 0xFFFFFF

//#define NORMAL_FRAMENUM_COLOR 0xFFFFFF
//#define NORMAL_INPUT_COLOR1   0xEDEDED
//#define NORMAL_INPUT_COLOR2   0xE2E2E2

#define NORMAL_FRAMENUM_COLOR 0xFF, 0xFF, 0xFF
#define NORMAL_INPUT_COLOR1   0xED, 0xED, 0xED
#define NORMAL_INPUT_COLOR2   0xE2, 0xE2, 0xE2

//#define GREENZONE_FRAMENUM_COLOR 0xDDFFDD
//#define GREENZONE_INPUT_COLOR1   0xC8F7C4
//#define GREENZONE_INPUT_COLOR2   0xADE7AD

#define GREENZONE_FRAMENUM_COLOR 0xDD, 0xFF, 0xDD
#define GREENZONE_INPUT_COLOR1   0xC4, 0xF7, 0xC8
#define GREENZONE_INPUT_COLOR2   0xAD, 0xE7, 0xAD

//#define PALE_GREENZONE_FRAMENUM_COLOR 0xE4FFE4
//#define PALE_GREENZONE_INPUT_COLOR1   0xD3F9D2
//#define PALE_GREENZONE_INPUT_COLOR2   0xBAEBBA

#define PALE_GREENZONE_FRAMENUM_COLOR 0xE4, 0xFF, 0xE4
#define PALE_GREENZONE_INPUT_COLOR1   0xD2, 0xF9, 0xD3
#define PALE_GREENZONE_INPUT_COLOR2   0xBA, 0xEB, 0xBA

//#define VERY_PALE_GREENZONE_FRAMENUM_COLOR 0xF9FFF9
//#define VERY_PALE_GREENZONE_INPUT_COLOR1 0xE0FBE0
//#define VERY_PALE_GREENZONE_INPUT_COLOR2 0xD2F2D2

#define VERY_PALE_GREENZONE_FRAMENUM_COLOR 0xF9, 0xFF, 0xF9
#define VERY_PALE_GREENZONE_INPUT_COLOR1   0xE0, 0xFB, 0xE0
#define VERY_PALE_GREENZONE_INPUT_COLOR2   0xD2, 0xF2, 0xD2

//#define LAG_FRAMENUM_COLOR 0xDDDCFF
//#define LAG_INPUT_COLOR1 0xD2D0F0
//#define LAG_INPUT_COLOR2 0xC9C6E8

#define LAG_FRAMENUM_COLOR 0xFF, 0xDC, 0xDD
#define LAG_INPUT_COLOR1   0xF0, 0xD0, 0xD2
#define LAG_INPUT_COLOR2   0xE8, 0xC6, 0xC9

//#define PALE_LAG_FRAMENUM_COLOR 0xE3E3FF
//#define PALE_LAG_INPUT_COLOR1 0xDADAF4
//#define PALE_LAG_INPUT_COLOR2 0xCFCEEA

#define PALE_LAG_FRAMENUM_COLOR 0xFF, 0xE3, 0xE3
#define PALE_LAG_INPUT_COLOR1   0xF4, 0xDA, 0xDA
#define PALE_LAG_INPUT_COLOR2   0xEA, 0xCE, 0xCF

//#define VERY_PALE_LAG_FRAMENUM_COLOR 0xE9E9FF 
//#define VERY_PALE_LAG_INPUT_COLOR1 0xE5E5F7
//#define VERY_PALE_LAG_INPUT_COLOR2 0xE0E0F1

#define VERY_PALE_LAG_FRAMENUM_COLOR 0xFF, 0xE9, 0xE9 
#define VERY_PALE_LAG_INPUT_COLOR1   0xF7, 0xE5, 0xE5
#define VERY_PALE_LAG_INPUT_COLOR2   0xF1, 0xE0, 0xE0

//#define CUR_FRAMENUM_COLOR 0xFCEDCF
//#define CUR_INPUT_COLOR1   0xF7E7B5
//#define CUR_INPUT_COLOR2   0xE5DBA5

#define CUR_FRAMENUM_COLOR 0xCF, 0xED, 0xFC
#define CUR_INPUT_COLOR1   0xB5, 0xE7, 0xF7
#define CUR_INPUT_COLOR2   0xA5, 0xDB, 0xE5

//#define UNDOHINT_FRAMENUM_COLOR 0xF9DDE6
//#define UNDOHINT_INPUT_COLOR1   0xF7D2E1
//#define UNDOHINT_INPUT_COLOR2   0xE9BED1

#define UNDOHINT_FRAMENUM_COLOR 0xE6, 0xDD, 0xF9
#define UNDOHINT_INPUT_COLOR1   0xE1, 0xD2, 0xF7
#define UNDOHINT_INPUT_COLOR2   0xD1, 0xBE, 0xE9

#define MARKED_FRAMENUM_COLOR 0xAEF0FF
#define CUR_MARKED_FRAMENUM_COLOR 0xCAEDEA
#define MARKED_UNDOHINT_FRAMENUM_COLOR 0xDDE5E9

#define BINDMARKED_FRAMENUM_COLOR 0xC9FFF7
#define CUR_BINDMARKED_FRAMENUM_COLOR 0xD5F2EC
#define BINDMARKED_UNDOHINT_FRAMENUM_COLOR 0xE1EBED

#define PLAYBACK_MARKER_COLOR 0xC9AF00


//----------------------------------------------------------------------------
//----  Main TAS Editor Window
//----------------------------------------------------------------------------
bool tasWindowIsOpen(void)
{
	return tasWin != NULL;
}
//----------------------------------------------------------------------------
void tasWindowSetFocus(bool val)
{
	if ( tasWin )
	{
		tasWin->activateWindow();
		tasWin->raise();
		tasWin->setFocus();
	}
}
// this getter contains formula to decide whether to record or replay movie
bool isTaseditorRecording(void)
{
	if ( tasWin == NULL )
	{
		return false;
	}
	if (movie_readonly || playback->getPauseFrame() >= 0 || (taseditorConfig->oldControlSchemeForBranching && !recorder->stateWasLoadedInReadWriteMode))
	{
		return false;		// replay
	}
	return true;			// record
}

void recordInputByTaseditor(void)
{
	if ( recorder )
	{
		recorder->recordInput();
	}
	return;
}

void applyMovieInputConfig(void)
{
	// update FCEUX input config
	FCEUD_SetInput(currMovieData.fourscore, currMovieData.microphone, (ESI)currMovieData.ports[0], (ESI)currMovieData.ports[1], (ESIFC)currMovieData.ports[2]);
	// update PAL flag
	pal_emulation = currMovieData.palFlag;
	if (pal_emulation)
	{
		dendy = 0;
	}
	FCEUI_SetVidSystem(pal_emulation);
	RefreshThrottleFPS();
	//PushCurrentVideoSettings();
	// update PPU type
	newppu = currMovieData.PPUflag;
	//SetMainWindowText();
	// return focus to TAS Editor window
	//SetFocus(taseditorWindow.hwndTASEditor);
	RAMInitOption = currMovieData.RAMInitOption;
}
//----------------------------------------------------------------------------
TasEditorWindow::TasEditorWindow(QWidget *parent)
	: QDialog( parent, Qt::Window )
{
	QSettings  settings;
	QVBoxLayout *mainLayout;
	//QHBoxLayout *hbox;
	QMenuBar    *menuBar;

	tasWin = this;
	::project         = &this->project;
	::taseditorConfig = &this->taseditorConfig;
	::taseditor_lua   = &this->taseditor_lua;
	::markersManager  = &this->markersManager;
	::selection       = &this->selection;
	::greenzone       = &this->greenzone;
	::bookmarks       = &this->bookmarks;
	::playback        = &this->playback;
	::recorder        = &this->recorder;
	::history         = &this->history;
	::branches        = &this->branches;
	::splicer         = &this->splicer;

	setWindowTitle("TAS Editor");

	resize(512, 512);

	mainLayout = new QVBoxLayout();
	mainHBox   = new QSplitter( Qt::Horizontal );

	buildPianoRollDisplay();
	buildSideControlPanel();

	mainHBox->addWidget( pianoRollContainerWidget );
	mainHBox->addWidget( controlPanelContainerWidget );
	mainLayout->addWidget(mainHBox);

	menuBar = buildMenuBar();

	setLayout(mainLayout);
	mainLayout->setMenuBar( menuBar );

	initModules();

	// Restore Window Geometry
	restoreGeometry(settings.value("tasEditor/geometry").toByteArray());

	// Restore Horizontal Panel State
	mainHBox->restoreState( settings.value("tasEditor/hPanelState").toByteArray() );
}
//----------------------------------------------------------------------------
TasEditorWindow::~TasEditorWindow(void)
{
	QSettings  settings;

	printf("Destroy Tas Editor Window\n");

	fceuWrapperLock();
	//if (!askToSaveProject()) return false;

	// destroy window
	//taseditorWindow.exit();
	//disableGeneralKeyboardInput();
	// release memory
	//editor.free();
	//pianoRoll.free();
	markersManager.free();
	greenzone.free();
	bookmarks.free();
	branches.free();
	//popupDisplay.free();
	history.free();
	playback.stopSeeking();
	selection.free();

	// switch off TAS Editor mode
	movieMode = MOVIEMODE_INACTIVE;
	FCEU_DispMessage("TAS Editor disengaged", 0);
	FCEUMOV_CreateCleanMovie();

	if ( tasWin == this )
	{
		tasWin = NULL;
	}

	::project         = NULL;
	::taseditorConfig = NULL;
	::taseditor_lua   = NULL;
	::markersManager  = NULL;
	::selection       = NULL;
	::greenzone       = NULL;
	::bookmarks       = NULL;
	::playback        = NULL;
	::recorder        = NULL;
	::history         = NULL;
	::branches        = NULL;
	::splicer         = NULL;

	fceuWrapperUnLock();

	// Save Horizontal Panel State
	settings.setValue("tasEditor/hPanelState", mainHBox->saveState());

	// Save Window Geometry
	settings.setValue("tasEditor/geometry", saveGeometry());
}
//----------------------------------------------------------------------------
void TasEditorWindow::closeEvent(QCloseEvent *event)
{
	printf("Tas Editor Close Window Event\n");

	if (!askToSaveProject()) return;

	done(0);
	deleteLater();
	event->accept();
}
//----------------------------------------------------------------------------
void TasEditorWindow::closeWindow(void)
{
	if (!askToSaveProject()) return;
	//printf("Close Window\n");
	done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
QMenuBar *TasEditorWindow::buildMenuBar(void)
{
	QMenu       *fileMenu;
	//QActionGroup *actGroup;
	QAction     *act;
	int useNativeMenuBar=0;

	QMenuBar *menuBar = new QMenuBar(this);

	// This is needed for menu bar to show up on MacOS
	g_config->getOption( "SDL.UseNativeMenuBar", &useNativeMenuBar );

	menuBar->setNativeMenuBar( useNativeMenuBar ? true : false );

	//-----------------------------------------------------------------------
	// Menu Start
	//-----------------------------------------------------------------------
	// File
	fileMenu = menuBar->addMenu(tr("&File"));

	// File -> New
	act = new QAction(tr("&New"), this);
	act->setShortcut(QKeySequence(tr("Ctrl+N")));
	act->setStatusTip(tr("Open New Project"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered()), this, SLOT(createNewProject(void)) );

	fileMenu->addAction(act);

	// File -> Open
	act = new QAction(tr("&Open"), this);
	act->setShortcut(QKeySequence(tr("Ctrl+O")));
	act->setStatusTip(tr("Open Project"));
	//act->setIcon( style()->standardIcon( QStyle::SP_BrowserStop ) );
	connect(act, SIGNAL(triggered()), this, SLOT(openProject(void)) );

	fileMenu->addAction(act);

	// File -> Save
	act = new QAction(tr("&Save"), this);
	act->setShortcut(QKeySequence(tr("Ctrl+S")));
	act->setStatusTip(tr("Save Project"));
	//act->setIcon( style()->standardIcon( QStyle::SP_BrowserStop ) );
	connect(act, SIGNAL(triggered()), this, SLOT(saveProjectCb(void)) );

	fileMenu->addAction(act);

	// File -> Save As
	act = new QAction(tr("Save &As"), this);
	act->setShortcut(QKeySequence(tr("Ctrl+Shift+S")));
	act->setStatusTip(tr("Save Project As"));
	//act->setIcon( style()->standardIcon( QStyle::SP_BrowserStop ) );
	connect(act, SIGNAL(triggered()), this, SLOT(saveProjectAsCb(void)) );

	fileMenu->addAction(act);

	// File -> Save Compact
	act = new QAction(tr("Save &Compact"), this);
	//act->setShortcut(QKeySequence(tr("Ctrl+Shift+S")));
	act->setStatusTip(tr("Save Compact"));
	//act->setIcon( style()->standardIcon( QStyle::SP_BrowserStop ) );
	connect(act, SIGNAL(triggered()), this, SLOT(saveProjectCompactCb(void)) );

	fileMenu->addAction(act);

	// File -> Recent
	recentMenu = fileMenu->addMenu( tr("Recent") );
	
	recentMenu->setEnabled(false); // TODO: setup recent projects menu

	fileMenu->addSeparator();

	// File -> Import Input
	act = new QAction(tr("&Import Input"), this);
	//act->setShortcut(QKeySequence(tr("Ctrl+Shift+S")));
	act->setStatusTip(tr("Import Input"));
	//act->setIcon( style()->standardIcon( QStyle::SP_BrowserStop ) );
	//connect(act, SIGNAL(triggered()), this, SLOT(closeFile(void)) );

	fileMenu->addAction(act);

	// File -> Export to fm2
	act = new QAction(tr("&Export to fm2"), this);
	//act->setShortcut(QKeySequence(tr("Ctrl+Shift+S")));
	act->setStatusTip(tr("Export to fm2"));
	//act->setIcon( style()->standardIcon( QStyle::SP_BrowserStop ) );
	//connect(act, SIGNAL(triggered()), this, SLOT(closeFile(void)) );

	fileMenu->addAction(act);

	fileMenu->addSeparator();

	// File -> Quit
	act = new QAction(tr("&Quit Window"), this);
	act->setShortcut(QKeySequence::Close);
	act->setStatusTip(tr("Close Window"));
	act->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
	connect(act, SIGNAL(triggered()), this, SLOT(closeWindow(void)) );

	fileMenu->addAction(act);

	return menuBar;
}
//----------------------------------------------------------------------------
void TasEditorWindow::buildPianoRollDisplay(void)
{
	QVBoxLayout *vbox;
	QHBoxLayout *hbox;
	QGridLayout *grid;

	grid             = new QGridLayout();
	pianoRoll        = new QPianoRoll(this);
	pianoRollVBar    = new QScrollBar( Qt::Vertical, this );
	pianoRollHBar    = new QScrollBar( Qt::Horizontal, this );
	upperMarkerLabel = new QLabel( tr("Marker 0") );
	lowerMarkerLabel = new QLabel( tr("Marker 1") );
	upperMarkerName  = new QLineEdit();
	lowerMarkerName  = new QLineEdit();

	pianoRoll->setScrollBars( pianoRollHBar, pianoRollVBar );
	connect( pianoRollHBar, SIGNAL(valueChanged(int)), pianoRoll, SLOT(hbarChanged(int)) );
	connect( pianoRollVBar, SIGNAL(valueChanged(int)), pianoRoll, SLOT(vbarChanged(int)) );

	grid->addWidget( pianoRoll    , 0, 0 );
	grid->addWidget( pianoRollVBar, 0, 1 );
	grid->addWidget( pianoRollHBar, 1, 0 );

	vbox = new QVBoxLayout();

	pianoRollHBar->setMinimum(0);
	pianoRollHBar->setMaximum(100);
	pianoRollVBar->setMinimum(0);
	pianoRollVBar->setMaximum(100);

	hbox = new QHBoxLayout();
	hbox->addWidget( upperMarkerLabel, 1 );
	hbox->addWidget( upperMarkerName, 10 );

	vbox->addLayout( hbox, 1 );
	vbox->addLayout( grid, 100 );

	hbox = new QHBoxLayout();
	hbox->addWidget( lowerMarkerLabel, 1 );
	hbox->addWidget( lowerMarkerName, 10 );

	vbox->addLayout( hbox, 1 );
	
	pianoRollContainerWidget = new QWidget();
	pianoRollContainerWidget->setLayout( vbox );
}
//----------------------------------------------------------------------------
void TasEditorWindow::buildSideControlPanel(void)
{
	QShortcut   *shortcut;
	QVBoxLayout *vbox;
	QHBoxLayout *hbox;
	QGridLayout *grid;

	ctlPanelMainVbox = new QVBoxLayout();

	playbackGBox  = new QGroupBox( tr("Playback") );
	recorderGBox  = new QGroupBox( tr("Recorder") );
	splicerGBox   = new QGroupBox( tr("Splicer") );
	luaGBox       = new QGroupBox( tr("Lua") );
	bookmarksGBox = new QGroupBox( tr("BookMarks/Branches") );
	historyGBox   = new QGroupBox( tr("History") );

	rewindMkrBtn  = new QPushButton();
	rewindFrmBtn  = new QPushButton();
	playPauseBtn  = new QPushButton();
	advFrmBtn     = new QPushButton();
	advMkrBtn     = new QPushButton();

	rewindMkrBtn->setIcon( style()->standardIcon( QStyle::SP_MediaSkipBackward ) );
	rewindFrmBtn->setIcon( style()->standardIcon( QStyle::SP_MediaSeekBackward ) );
	playPauseBtn->setIcon( style()->standardIcon( QStyle::SP_MediaPause ) );
	   advFrmBtn->setIcon( style()->standardIcon( QStyle::SP_MediaSeekForward ) );
	   advMkrBtn->setIcon( style()->standardIcon( QStyle::SP_MediaSkipForward ) );

	followCursorCbox = new QCheckBox( tr("Follow Cursor") );
	   turboSeekCbox = new QCheckBox( tr("Turbo Seek") );
	 autoRestoreCbox = new QCheckBox( tr("Auto-Restore Last Position") );

	recRecordingCbox   = new QCheckBox( tr("Recording") );
	recSuperImposeCbox = new QCheckBox( tr("Superimpose") );
	recUsePatternCbox  = new QCheckBox( tr("Use Pattern") );
	recAllBtn          = new QRadioButton( tr("All") );
	rec1PBtn           = new QRadioButton( tr("1P") );
	rec2PBtn           = new QRadioButton( tr("2P") );
	rec3PBtn           = new QRadioButton( tr("3P") );
	rec4PBtn           = new QRadioButton( tr("4P") );

	selectionLbl = new QLabel( tr("Empty") );
	clipboardLbl = new QLabel( tr("Empty") );

	runLuaBtn   = new QPushButton( tr("Run Function") );
	autoLuaCBox = new QCheckBox( tr("Auto Function") );
	runLuaBtn->setEnabled(false);
	autoLuaCBox->setChecked(true);

	bkbrTree = new QTreeWidget();
	histTree = new QTreeWidget();

	prevMkrBtn = new QPushButton();
	nextMkrBtn = new QPushButton();
	similarBtn = new QPushButton( tr("Similar") );
	moreBtn    = new QPushButton( tr("More") );

	prevMkrBtn->setIcon( style()->standardIcon( QStyle::SP_MediaSkipBackward ) );
	nextMkrBtn->setIcon( style()->standardIcon( QStyle::SP_MediaSkipForward  ) );

	vbox = new QVBoxLayout();
	hbox = new QHBoxLayout();
	vbox->addLayout( hbox );
	hbox->addWidget( rewindMkrBtn );
	hbox->addWidget( rewindFrmBtn );
	hbox->addWidget( playPauseBtn );
	hbox->addWidget( advFrmBtn    );
	hbox->addWidget( advMkrBtn    );

	hbox = new QHBoxLayout();
	vbox->addLayout( hbox );
	hbox->addWidget( followCursorCbox );
	hbox->addWidget( turboSeekCbox    );

	vbox->addWidget( autoRestoreCbox );

	playbackGBox->setLayout( vbox );

	grid = new QGridLayout();
	grid->addWidget( recRecordingCbox, 0, 0, 1, 2 );
	grid->addWidget( recAllBtn       , 0, 3, 1, 1 );
	grid->addWidget( rec1PBtn        , 1, 0, 1, 1 );
	grid->addWidget( rec2PBtn        , 1, 1, 1, 1 );
	grid->addWidget( rec3PBtn        , 1, 2, 1, 1 );
	grid->addWidget( rec4PBtn        , 1, 3, 1, 1 );
	grid->addWidget( recSuperImposeCbox, 2, 0, 1, 2 );
	grid->addWidget( recUsePatternCbox , 2, 2, 1, 2 );
	recorderGBox->setLayout( grid );

	grid = new QGridLayout();
	grid->addWidget( new QLabel( tr("Selection:") ), 0, 0, 1, 1 );
	grid->addWidget( new QLabel( tr("Clipboard:") ), 1, 0, 1, 1 );
	grid->addWidget( selectionLbl, 0, 1, 1, 3 );
	grid->addWidget( clipboardLbl, 1, 1, 1, 3 );
	splicerGBox->setLayout( grid );

	hbox = new QHBoxLayout();
	hbox->addWidget( runLuaBtn );
	hbox->addWidget( autoLuaCBox );
	luaGBox->setLayout( hbox );

	vbox = new QVBoxLayout();
	vbox->addWidget( bkbrTree );
	bookmarksGBox->setLayout( vbox );

	vbox = new QVBoxLayout();
	vbox->addWidget( histTree );
	historyGBox->setLayout( vbox );

	ctlPanelMainVbox->addWidget( playbackGBox  );
	ctlPanelMainVbox->addWidget( recorderGBox  );
	ctlPanelMainVbox->addWidget( splicerGBox   );
	ctlPanelMainVbox->addWidget( luaGBox       );
	ctlPanelMainVbox->addWidget( bookmarksGBox );
	ctlPanelMainVbox->addWidget( historyGBox   );

	hbox = new QHBoxLayout();
	hbox->addWidget( prevMkrBtn );
	hbox->addWidget( similarBtn );
	hbox->addWidget( moreBtn    );
	hbox->addWidget( nextMkrBtn );
	ctlPanelMainVbox->addLayout( hbox );

	controlPanelContainerWidget = new QWidget();
	controlPanelContainerWidget->setLayout( ctlPanelMainVbox );

	recRecordingCbox->setChecked( !movie_readonly );
	connect( recRecordingCbox, SIGNAL(stateChanged(int)), this, SLOT(recordingChanged(int)) );

	recUsePatternCbox->setChecked( taseditorConfig.recordingUsePattern );
	connect( recUsePatternCbox, SIGNAL(stateChanged(int)), this, SLOT(usePatternChanged(int)) );

	recSuperImposeCbox->setTristate(true);
	connect( recSuperImposeCbox, SIGNAL(stateChanged(int)), this, SLOT(superImposedChanged(int)) );

	connect( recAllBtn, &QRadioButton::clicked, [ this ] { recordInputChanged( MULTITRACK_RECORDING_ALL ); } );
	connect( rec1PBtn , &QRadioButton::clicked, [ this ] { recordInputChanged( MULTITRACK_RECORDING_1P  ); } );
	connect( rec2PBtn , &QRadioButton::clicked, [ this ] { recordInputChanged( MULTITRACK_RECORDING_2P  ); } );
	connect( rec3PBtn , &QRadioButton::clicked, [ this ] { recordInputChanged( MULTITRACK_RECORDING_3P  ); } );
	connect( rec4PBtn , &QRadioButton::clicked, [ this ] { recordInputChanged( MULTITRACK_RECORDING_4P  ); } );

	connect( rewindMkrBtn, SIGNAL(clicked(void)), this, SLOT(playbackFrameRewindFull(void)) );
	connect( rewindFrmBtn, SIGNAL(clicked(void)), this, SLOT(playbackFrameRewind(void))     );
	connect( playPauseBtn, SIGNAL(clicked(void)), this, SLOT(playbackPauseCB(void))         );
	connect( advFrmBtn   , SIGNAL(clicked(void)), this, SLOT(playbackFrameForward(void))    );
	connect( advMkrBtn   , SIGNAL(clicked(void)), this, SLOT(playbackFrameForwardFull(void)));

	shortcut = new QShortcut( QKeySequence("Pause"), this);
	connect( shortcut, SIGNAL(activated(void)), this, SLOT(playbackPauseCB(void)) );

	shortcut = new QShortcut( QKeySequence("Shift+Up"), this);
	connect( shortcut, SIGNAL(activated(void)), this, SLOT(playbackFrameRewind(void)) );

	shortcut = new QShortcut( QKeySequence("Shift+Down"), this);
	connect( shortcut, SIGNAL(activated(void)), this, SLOT(playbackFrameForward(void)) );

	shortcut = new QShortcut( QKeySequence("Shift+PgUp"), this);
	connect( shortcut, SIGNAL(activated(void)), this, SLOT(playbackFrameRewindFull(void)) );

	shortcut = new QShortcut( QKeySequence("Shift+PgDown"), this);
	connect( shortcut, SIGNAL(activated(void)), this, SLOT(playbackFrameForwardFull(void)) );

}
//----------------------------------------------------------------------------
int TasEditorWindow::initModules(void)
{
	// init modules
	//editor.init();
	//pianoRoll.init();
	selection.init();
	splicer.init();
	playback.init();
	greenzone.init();
	recorder.init();
	markersManager.init();
	project.init();
	bookmarks.init();
	branches.init();
	//popupDisplay.init();
	history.init();
	taseditor_lua.init();
	// either start new movie or use current movie
	if (!FCEUMOV_Mode(MOVIEMODE_RECORD|MOVIEMODE_PLAY) || currMovieData.savestate.size() != 0)
	{
		if (currMovieData.savestate.size() != 0)
		{
			FCEUD_PrintError("This version of TAS Editor doesn't work with movies starting from savestate.");
		}
		// create new movie
		FCEUI_StopMovie();
		movieMode = MOVIEMODE_TASEDITOR;
		FCEUMOV_CreateCleanMovie();
		playback.restartPlaybackFromZeroGround();
	}
	else
	{
		// use current movie to create a new project
		FCEUI_StopMovie();
		movieMode = MOVIEMODE_TASEDITOR;
	}
	// if movie length is less or equal to currFrame, pad it with empty frames
	if (((int)currMovieData.records.size() - 1) < currFrameCounter)
	{
		currMovieData.insertEmpty(-1, currFrameCounter - ((int)currMovieData.records.size() - 1));
	}
	// ensure that movie has correct set of ports/fourscore
	setInputType(currMovieData, getInputType(currMovieData));
	// force the input configuration stored in the movie to apply to FCEUX config
	applyMovieInputConfig();
	// reset some modules that need MovieData info
	//pianoRoll.reset();
	recorder.reset();
	// create initial snapshot in history
	history.reset();
	// reset Taseditor variables
	//mustCallManualLuaFunction = false;
	
	//SetFocus(history.hwndHistoryList);		// set focus only once, to show blue selection cursor
	//SetFocus(pianoRoll.hwndList);
	FCEU_DispMessage("TAS Editor engaged", 0);
	update();
	return 0;
}
//----------------------------------------------------------------------------
void TasEditorWindow::frameUpdate(void)
{
	fceuWrapperLock();

	//printf("TAS Frame Update: %zi\n", currMovieData.records.size());

	//taseditorWindow.update();
	greenzone.update();
	recorder.update();
	//pianoRoll.update();
	markersManager.update();
	playback.update();
	bookmarks.update();
	branches.update();
	//popupDisplay.update();
	selection.update();
	splicer.update();
	history.update();
	project.update();
	// run Lua functions if needed
	if (taseditorConfig.enableLuaAutoFunction)
	{
		//TaseditorAutoFunction();
	}
	//if (mustCallManualLuaFunction)
	//{
	//	TaseditorManualFunction();
	//	mustCallManualLuaFunction = false;
	//}
	
	pianoRoll->update();

	fceuWrapperUnLock();
}
//----------------------------------------------------------------------------
bool TasEditorWindow::loadProject(const char* fullname)
{
	bool success = false;

	fceuWrapperLock();

	// try to load project
	if (project.load(fullname))
	{
		// loaded successfully
		applyMovieInputConfig();
		// add new file to Recent menu
		//taseditorWindow.updateRecentProjectsArray(fullname);
		//taseditorWindow.updateCaption();
		update();
		success = true;
	} else
	{
		// failed to load
		//taseditorWindow.updateCaption();
		update();
	}
	fceuWrapperUnLock();

	return success;
}
bool TasEditorWindow::saveProject(bool save_compact)
{
	bool ret = true;

	fceuWrapperLock();

	if (project.getProjectFile().empty())
	{
		ret = saveProjectAs(save_compact);
	} else
	{
		if (save_compact)
		{
			project.save(0, taseditorConfig.saveCompact_SaveInBinary, taseditorConfig.saveCompact_SaveMarkers, taseditorConfig.saveCompact_SaveBookmarks, taseditorConfig.saveCompact_GreenzoneSavingMode, taseditorConfig.saveCompact_SaveHistory, taseditorConfig.saveCompact_SavePianoRoll, taseditorConfig.saveCompact_SaveSelection);
		}
		else
		{
			project.save(0, taseditorConfig.projectSavingOptions_SaveInBinary, taseditorConfig.projectSavingOptions_SaveMarkers, taseditorConfig.projectSavingOptions_SaveBookmarks, taseditorConfig.projectSavingOptions_GreenzoneSavingMode, taseditorConfig.projectSavingOptions_SaveHistory, taseditorConfig.projectSavingOptions_SavePianoRoll, taseditorConfig.projectSavingOptions_SaveSelection);
		}
		//taseditorWindow.updateCaption();
	}

	fceuWrapperUnLock();

	return ret;
}

bool TasEditorWindow::saveProjectAs(bool save_compact)
{
	std::string last;
	int ret, useNativeFileDialogVal;
	QString filename;
	std::string lastPath;
	//char dir[512];
	const char *base, *rom;
	QFileDialog  dialog(this, tr("Save TAS Editor Project As") );
	QList<QUrl> urls;
	QDir d;

	dialog.setFileMode(QFileDialog::AnyFile);

	dialog.setNameFilter(tr("TAS Project Files (*.fm3) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::AllDirs | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Save") );

	base = FCEUI_GetBaseDirectory();

	urls << QUrl::fromLocalFile( QDir::rootPath() );
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).first());

	if ( base )
	{
		urls << QUrl::fromLocalFile( QDir( base ).absolutePath() );

		d.setPath( QString(base) + "/movies");

		if ( d.exists() )
		{
			urls << QUrl::fromLocalFile( d.absolutePath() );
		}

		dialog.setDirectory( d.absolutePath() );
	}
	dialog.setDefaultSuffix( tr(".fm3") );

	g_config->getOption ("SDL.TasProjectFilePath", &lastPath);
	if ( lastPath.size() > 0 )
	{
		dialog.setDirectory( QString::fromStdString(lastPath) );
	}

	rom = getRomFile();

	if ( rom )
	{
		char baseName[512];
		getFileBaseName( rom, baseName );

		if ( baseName[0] != 0 )
		{
			dialog.selectFile(baseName);
		}
	}

	// Check config option to use native file dialog or not
	g_config->getOption ("SDL.UseNativeFileDialog", &useNativeFileDialogVal);

	dialog.setOption(QFileDialog::DontUseNativeDialog, !useNativeFileDialogVal);
	dialog.setSidebarUrls(urls);

	ret = dialog.exec();

	if ( ret )
	{
		QStringList fileList;
		fileList = dialog.selectedFiles();

		if ( fileList.size() > 0 )
		{
			filename = fileList[0];
		}
	}

	if ( filename.isNull() )
	{
	   return false;
	}
	//qDebug() << "selected file path : " << filename.toUtf8();

	project.renameProject( filename.toStdString().c_str(), true);
	if (save_compact)
	{
		project.save( filename.toStdString().c_str(), taseditorConfig.saveCompact_SaveInBinary, taseditorConfig.saveCompact_SaveMarkers, taseditorConfig.saveCompact_SaveBookmarks, taseditorConfig.saveCompact_GreenzoneSavingMode, taseditorConfig.saveCompact_SaveHistory, taseditorConfig.saveCompact_SavePianoRoll, taseditorConfig.saveCompact_SaveSelection);
	}
	else
	{
		project.save( filename.toStdString().c_str(), taseditorConfig.projectSavingOptions_SaveInBinary, taseditorConfig.projectSavingOptions_SaveMarkers, taseditorConfig.projectSavingOptions_SaveBookmarks, taseditorConfig.projectSavingOptions_GreenzoneSavingMode, taseditorConfig.projectSavingOptions_SaveHistory, taseditorConfig.projectSavingOptions_SavePianoRoll, taseditorConfig.projectSavingOptions_SaveSelection);
	}
	//taseditorWindow.updateRecentProjectsArray(nameo);
	// saved successfully - remove * mark from caption
	//taseditorWindow.updateCaption();
	return true;
}

// returns false if user doesn't want to exit
bool TasEditorWindow::askToSaveProject(void)
{
	bool changesFound = false;
	if (project.getProjectChanged())
	{
		changesFound = true;
	}

	// ask saving project
	if (changesFound)
	{
		int ans = QMessageBox::question( this, tr("TAS Editor"), tr("Save project changes?"),
				QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes );

		//int answer = MessageBox(taseditorWindow.hwndTASEditor, "Save Project changes?", "TAS Editor", MB_YESNOCANCEL);
		if (ans == QMessageBox::Yes)
		{
			return saveProject();
		}
		return (ans != QMessageBox::No);
	}
	return true;
}
//----------------------------------------------------------------------------
void TasEditorWindow::openProject(void)
{
	std::string last;
	int ret, useNativeFileDialogVal;
	QString filename;
	std::string lastPath;
	//char dir[512];
	const char *base, *rom;
	QFileDialog  dialog(this, tr("Open TAS Editor Project") );
	QList<QUrl> urls;
	QDir d;

	dialog.setFileMode(QFileDialog::ExistingFile);

	dialog.setNameFilter(tr("TAS Project Files (*.fm3) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::AllDirs | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Open") );

	base = FCEUI_GetBaseDirectory();

	urls << QUrl::fromLocalFile( QDir::rootPath() );
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).first());

	if ( base )
	{
		urls << QUrl::fromLocalFile( QDir( base ).absolutePath() );

		d.setPath( QString(base) + "/movies");

		if ( d.exists() )
		{
			urls << QUrl::fromLocalFile( d.absolutePath() );
		}

		dialog.setDirectory( d.absolutePath() );
	}
	dialog.setDefaultSuffix( tr(".fm3") );

	g_config->getOption ("SDL.TasProjectFilePath", &lastPath);
	if ( lastPath.size() > 0 )
	{
		dialog.setDirectory( QString::fromStdString(lastPath) );
	}

	rom = getRomFile();

	if ( rom )
	{
		char baseName[512];
		getFileBaseName( rom, baseName );

		if ( baseName[0] != 0 )
		{
			dialog.selectFile(baseName);
		}
	}

	// Check config option to use native file dialog or not
	g_config->getOption ("SDL.UseNativeFileDialog", &useNativeFileDialogVal);

	dialog.setOption(QFileDialog::DontUseNativeDialog, !useNativeFileDialogVal);
	dialog.setSidebarUrls(urls);

	ret = dialog.exec();

	if ( ret )
	{
		QStringList fileList;
		fileList = dialog.selectedFiles();

		if ( fileList.size() > 0 )
		{
			filename = fileList[0];
		}
	}

	if ( filename.isNull() )
	{
	   return;
	}
	//qDebug() << "selected file path : " << filename.toUtf8();

	loadProject( filename.toStdString().c_str());

	return;
}
//----------------------------------------------------------------------------
void TasEditorWindow::createNewProject(void)
{
	//if (!askToSaveProject()) return;
	
	fceuWrapperLock();

	static struct NewProjectParameters params;
	//if (DialogBoxParam(fceu_hInstance, MAKEINTRESOURCE(IDD_TASEDITOR_NEWPROJECT), taseditorWindow.hwndTASEditor, newProjectProc, (LPARAM)&params) > 0)
	//{
		FCEUMOV_CreateCleanMovie();
		// apply selected options
		setInputType(currMovieData, params.inputType);
		applyMovieInputConfig();
		if (params.copyCurrentInput)
		{
			// copy Input from current snapshot (from history)
			history.getCurrentSnapshot().inputlog.toMovie(currMovieData);
		}
		if (!params.copyCurrentMarkers)
		{
			markersManager.reset();
		}
		if (params.authorName != L"") currMovieData.comments.push_back(L"author " + params.authorName);
		
		// reset Taseditor
		project.init();			// new project has blank name
		greenzone.reset();
		if (params.copyCurrentInput)
		{
			// copy LagLog from current snapshot (from history)
			greenzone.lagLog = history.getCurrentSnapshot().laglog;
		}
		playback.reset();
		playback.restartPlaybackFromZeroGround();
		bookmarks.reset();
		branches.reset();
		history.reset();
		//pianoRoll.reset();
		selection.reset();
		//editor.reset();
		splicer.reset();
		recorder.reset();
		//popupDisplay.reset();
		//taseditorWindow.redraw();
		//taseditorWindow.updateCaption();
		update();
	//}
	fceuWrapperUnLock();
}
//----------------------------------------------------------------------------
void TasEditorWindow::saveProjectCb(void)
{
	saveProject();
}
//----------------------------------------------------------------------------
void TasEditorWindow::saveProjectAsCb(void)
{
	saveProjectAs();
}
//----------------------------------------------------------------------------
void TasEditorWindow::saveProjectCompactCb(void)
{
	saveProject(true);
}
//----------------------------------------------------------------------------
void TasEditorWindow::recordingChanged(int state)
{
	FCEUI_MovieToggleReadOnly();
}
//----------------------------------------------------------------------------
void TasEditorWindow::superImposedChanged(int state)
{
	if ( state == Qt::Checked )
	{
		taseditorConfig.superimpose = SUPERIMPOSE_CHECKED;
	}
	else if ( state == Qt::PartiallyChecked )
	{
		taseditorConfig.superimpose = SUPERIMPOSE_INDETERMINATE;
	}
	else
	{
		taseditorConfig.superimpose = SUPERIMPOSE_UNCHECKED;
	}
}
//----------------------------------------------------------------------------
void TasEditorWindow::usePatternChanged(int state)
{
	taseditorConfig.recordingUsePattern ^= 1;
	recorder.patternOffset = 0;
}
//----------------------------------------------------------------------------
void TasEditorWindow::recordInputChanged(int input)
{
	//printf("Input Change: %i\n", input);
	recorder.multitrackRecordingJoypadNumber = input;
}
//----------------------------------------------------------------------------
void TasEditorWindow::playbackPauseCB(void)
{
	fceuWrapperLock();
	playback.toggleEmulationPause();
	pianoRoll->update();
	fceuWrapperUnLock();
}
//----------------------------------------------------------------------------
void TasEditorWindow::playbackFrameRewind(void)
{
	fceuWrapperLock();
	playback.handleRewindFrame();
	pianoRoll->update();
	fceuWrapperUnLock();
}
//----------------------------------------------------------------------------
void TasEditorWindow::playbackFrameForward(void)
{
	fceuWrapperLock();
	playback.handleForwardFrame();
	pianoRoll->update();
	fceuWrapperUnLock();
}
//----------------------------------------------------------------------------
void TasEditorWindow::playbackFrameRewindFull(void)
{
	fceuWrapperLock();
	playback.handleRewindFull();
	pianoRoll->update();
	fceuWrapperUnLock();
}
//----------------------------------------------------------------------------
void TasEditorWindow::playbackFrameForwardFull(void)
{
	fceuWrapperLock();
	playback.handleForwardFull();
	pianoRoll->update();
	fceuWrapperUnLock();
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----  TAS Piano Roll Widget
//----------------------------------------------------------------------------
QPianoRoll::QPianoRoll(QWidget *parent)
	: QWidget( parent )
{
	QPalette pal;
	std::string fontString;
	QColor fg("black"), bg("white"), c;

	useDarkTheme = false;

	viewWidth  = 512;
	viewHeight = 512;

	g_config->getOption("SDL.TasPianoRollFont", &fontString);

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

	windowColor = pal.color(QPalette::Window);

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
	this->parent = qobject_cast <TasEditorWindow*>( parent );
	this->setMouseTracking(true);
	this->setPalette(pal);

	numCtlr = 2;
	calcFontData();

	vbar = NULL;
	hbar = NULL;

	lineOffset = 0;
	maxLineOffset = 0;
}
//----------------------------------------------------------------------------
QPianoRoll::~QPianoRoll(void)
{

}
//----------------------------------------------------------------------------
void QPianoRoll::setScrollBars( QScrollBar *h, QScrollBar *v )
{
	hbar = h; vbar = v;
}
//----------------------------------------------------------------------------
void QPianoRoll::hbarChanged(int val)
{
	if ( viewWidth >= pxLineWidth )
	{
		pxLineXScroll = 0;
	}
	else
	{
		pxLineXScroll = val;
	}
	update();
}
//----------------------------------------------------------------------------
void QPianoRoll::vbarChanged(int val)
{
	lineOffset = val;

	if ( lineOffset < 0 )
	{
		lineOffset = 0;
	}
	else if ( lineOffset > maxLineOffset )
	{
		lineOffset = maxLineOffset;
	}
	update();
}
//----------------------------------------------------------------------------
void QPianoRoll::calcFontData(void)
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
	pxLineTextOfs  = pxCharHeight + (pxLineSpacing - pxCharHeight) / 2;

	//printf("W:%i  H:%i  LS:%i  \n", pxCharWidth, pxCharHeight, pxLineSpacing );

	viewLines   = (viewHeight / pxLineSpacing) + 1;

	pxWidthCol1     =  3 * pxCharWidth;
	pxWidthFrameCol = 12 * pxCharWidth;
	pxWidthBtnCol   =  3 * pxCharWidth;
	pxWidthCtlCol   =  8 * pxWidthBtnCol;

	pxFrameColX     = pxWidthCol1;

	for (int i=0; i<4; i++)
	{
		pxFrameCtlX[i] = pxFrameColX + pxWidthFrameCol + (i*pxWidthCtlCol);
	}
	pxLineWidth = pxFrameCtlX[ numCtlr-1 ] + pxWidthCtlCol;
}
//----------------------------------------------------------------------------
void QPianoRoll::drawArrow( QPainter *painter, int xl, int yl, int value )
{
	int x, y, w, h, b, b2;
	QPoint p[3];

	x = xl+(pxCharWidth/3);
	y = yl+1;
	w = pxCharWidth;
	h = pxLineSpacing-2;

	p[0] = QPoint( x, y );
	p[1] = QPoint( x, y+h );
	p[2] = QPoint( x+w, y+(h/2) );

	if ( value == GREEN_BLUE_ARROW_IMAGE_ID )
	{
		painter->setBrush( QColor( 96, 192, 192) );
	}
	else if ( value == GREEN_ARROW_IMAGE_ID )
	{
		painter->setBrush( QColor(0, 192, 64) );
	}
	else if ( value == BLUE_ARROW_IMAGE_ID )
	{
		painter->setBrush( QColor(10, 36, 106) );
	}

	painter->drawPolygon( p, 3 );
}
//----------------------------------------------------------------------------
void QPianoRoll::resizeEvent(QResizeEvent *event)
{
	viewWidth  = event->size().width();
	viewHeight = event->size().height();

	//printf("QPianoRoll Resize: %ix%i  $%04X\n", viewWidth, viewHeight );

	viewLines = (viewHeight / pxLineSpacing) + 1;

	maxLineOffset = currMovieData.records.size() - viewLines + 1;

	if ( maxLineOffset < 0 )
	{
		maxLineOffset = 0;
	}
	vbar->setMinimum(0);
	vbar->setMaximum(maxLineOffset);

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
void QPianoRoll::paintEvent(QPaintEvent *event)
{
	int x, y, row, nrow, lineNum;
	QPainter painter(this);
	QColor white("white"), black("black"), blkColor;
	static const char *buttonNames[] = { "A", "B", "S", "T", "U", "D", "L", "R", NULL };
	char stmp[32];

	painter.setFont(font);
	viewWidth  = event->rect().width();
	viewHeight = event->rect().height();

	nrow = (viewHeight / pxLineSpacing) + 1;

	if ( nrow < 1 ) nrow = 1;

	viewLines = nrow;

	maxLineOffset = currMovieData.records.size() - nrow + 1;

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
	vbar->setMinimum(0);
	vbar->setMaximum(maxLineOffset);

	painter.fillRect( 0, 0, viewWidth, viewHeight, this->palette().color(QPalette::Window) );

	// Draw Title Bar
	x = -pxLineXScroll; y = 0;
	painter.fillRect( 0, 0, viewWidth, pxLineSpacing, windowColor );
	painter.setPen( black );

	x = -pxLineXScroll + pxFrameColX + (pxWidthFrameCol - 6*pxCharWidth) / 2;
	painter.drawText( x, pxLineTextOfs, tr("Frame#") );

	// Draw Grid
	painter.drawLine( -pxLineXScroll, 0, -pxLineXScroll, viewHeight );

	x = pxFrameColX - pxLineXScroll;
	painter.drawLine( x, 0, x, viewHeight );

	for (int i=0; i<numCtlr; i++)
	{
		x = pxFrameCtlX[i] - pxLineXScroll;

		if ( i % 2 )
		{
			painter.fillRect( x, pxLineSpacing, pxWidthCtlCol, viewHeight, this->palette().color(QPalette::AlternateBase) );
		}
	}

	y = pxLineSpacing;

	for (row=0; row<nrow; row++)
	{
		uint8_t data;

		lineNum = lineOffset + row;

		if ( lineNum >= currMovieData.records.size() )
		{
			break;
		}
		int frame_lag = greenzone->lagLog.getLagInfoAtFrame(lineNum);

		for (int i=0; i<numCtlr; i++)
		{
			x = pxFrameCtlX[i] - pxLineXScroll;

			if ( lineNum == history->getUndoHint())
			{
				// undo hint here
				blkColor = (i%2) ? QColor(UNDOHINT_INPUT_COLOR2) : QColor(UNDOHINT_INPUT_COLOR1);
			}
			else if ( lineNum == currFrameCounter ||  lineNum == (playback->getFlashingPauseFrame() - 1))
			{
				// this is current frame
				blkColor = (i%2) ? QColor(CUR_INPUT_COLOR2) : QColor(CUR_INPUT_COLOR1);
			}
			else if ( lineNum < greenzone->getSize() )
			{
				if (!greenzone->isSavestateEmpty(lineNum))
				{
					// the frame is normal Greenzone frame
					if (frame_lag == LAGGED_YES)
					{
						blkColor = (i%2) ? QColor(LAG_INPUT_COLOR2) : QColor(LAG_INPUT_COLOR1);
					}
					else
					{
						blkColor = (i%2) ? QColor(GREENZONE_INPUT_COLOR2) : QColor(GREENZONE_INPUT_COLOR1);
					}
				}
				else if (  !greenzone->isSavestateEmpty(lineNum & EVERY16TH)
					|| !greenzone->isSavestateEmpty(lineNum & EVERY8TH)
					|| !greenzone->isSavestateEmpty(lineNum & EVERY4TH)
					|| !greenzone->isSavestateEmpty(lineNum & EVERY2ND))
				{
					// the frame is in a gap (in Greenzone tail)
					if (frame_lag == LAGGED_YES)
					{
						blkColor = (i%2) ? QColor(PALE_LAG_INPUT_COLOR2) : QColor(PALE_LAG_INPUT_COLOR1);
					}
					else
					{
						blkColor = (i%2) ? QColor(PALE_GREENZONE_INPUT_COLOR2) : QColor(PALE_GREENZONE_INPUT_COLOR1);
					}
				}
				else 
				{
					// the frame is above Greenzone tail
					if (frame_lag == LAGGED_YES)
					{
						blkColor = (i%2) ? QColor(VERY_PALE_LAG_INPUT_COLOR2) : QColor(VERY_PALE_LAG_INPUT_COLOR1);
					}
					else if (frame_lag == LAGGED_NO)
					{
						blkColor = (i%2) ? QColor(VERY_PALE_GREENZONE_INPUT_COLOR2) : QColor(VERY_PALE_GREENZONE_INPUT_COLOR1);
					}
					else
					{
						blkColor = (i%2) ? QColor(NORMAL_INPUT_COLOR2) : QColor(NORMAL_INPUT_COLOR1);
					}
				}
			}
			else
			{
				// the frame is below Greenzone head
				if (frame_lag == LAGGED_YES)
				{
					blkColor = (i%2) ? QColor(VERY_PALE_LAG_INPUT_COLOR2) : QColor(VERY_PALE_LAG_INPUT_COLOR1);
				}
				else if (frame_lag == LAGGED_NO)
				{
					blkColor = (i%2) ? QColor(VERY_PALE_GREENZONE_INPUT_COLOR2) : QColor(VERY_PALE_GREENZONE_INPUT_COLOR1);
				}
				else
				{
					blkColor = (i%2) ? QColor(NORMAL_INPUT_COLOR2) : QColor(NORMAL_INPUT_COLOR1);
				}
			}
			painter.fillRect( x, y, pxWidthCtlCol, pxLineSpacing, blkColor );
		}

		for (int i=0; i<numCtlr; i++)
		{
			data = currMovieData.records[ lineNum ].joysticks[i];

			x = pxFrameCtlX[i] - pxLineXScroll;

			for (int j=0; j<8; j++)
			{
				if ( data & (0x01 << j) )
				{
					painter.drawText( x + pxCharWidth, y+pxLineTextOfs, tr(buttonNames[j]) );
				}
				x += pxWidthBtnCol;
			}
			painter.drawLine( x, 0, x, viewHeight );
		}

		x = -pxLineXScroll + pxFrameColX + (pxWidthFrameCol - 10*pxCharWidth) / 2;

		sprintf( stmp, "%010i", lineNum );

		painter.drawText( x, y+pxLineTextOfs, tr(stmp) );

		x = -pxLineXScroll;

		int iImage = bookmarks->findBookmarkAtFrame(lineNum);
		if (iImage < 0)
		{
			// no bookmark at this frame
			if (lineNum == playback->getLastPosition())
			{
				if (lineNum == currFrameCounter)
				{
					iImage = GREEN_BLUE_ARROW_IMAGE_ID;
				}
				else
				{
					iImage = GREEN_ARROW_IMAGE_ID;
				}
			}
			else if (lineNum == currFrameCounter)
			{
				iImage = BLUE_ARROW_IMAGE_ID;
			}
		}
		else
		{
			// bookmark at this frame
			if (lineNum == playback->getLastPosition())
			{
				iImage += BOOKMARKS_WITH_GREEN_ARROW;
			}
			else if (lineNum == currFrameCounter)
			{
				iImage += BOOKMARKS_WITH_BLUE_ARROW;
			}
		}

		if ( iImage >= 0 )
		{
			drawArrow( &painter, x, y, iImage );
		}

		y += pxLineSpacing;
	}

	for (int i=0; i<numCtlr; i++)
	{
		x = pxFrameCtlX[i] - pxLineXScroll;

		for (int j=0; j<8; j++)
		{
			painter.drawLine( x, 0, x, viewHeight );

			painter.drawText( x + pxCharWidth, pxLineTextOfs, tr(buttonNames[j]) );

			x += pxWidthBtnCol;
		}
		painter.drawLine( x, 0, x, viewHeight );
	}
	y = 0;
	for (int i=0; i<nrow; i++)
	{
		painter.drawLine( 0, y, viewWidth, y );
		
		y += pxLineSpacing;
	}

}
//----------------------------------------------------------------------------
