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

#include <QHeaderView>
#include <QMessageBox>
#include <QFontMetrics>

#include "fceu.h"
#include "movie.h"
#include "driver.h"

#include "Qt/config.h"
#include "Qt/fceuWrapper.h"
#include "Qt/TasEditor/TasEditorWindow.h"

TasEditorWindow  *tasWin = NULL;
TASEDITOR_CONFIG *taseditorConfig = NULL;
MARKERS_MANAGER  *markersManager = NULL;
SPLICER          *splicer = NULL;

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
	//if (movie_readonly || playback.getPauseFrame() >= 0 || (taseditorConfig.oldControlSchemeForBranching && !recorder.stateWasLoadedInReadWriteMode))
	//	return false;		// replay
	return true;			// record
}

bool recordInputByTaseditor(void)
{
	//recorder.recordInput();
	return 0;
}
//----------------------------------------------------------------------------
TasEditorWindow::TasEditorWindow(QWidget *parent)
	: QDialog( parent, Qt::Window )
{
	QVBoxLayout *mainLayout;
	//QHBoxLayout *hbox;
	QMenuBar    *menuBar;

	tasWin = this;
	::taseditorConfig = &this->taseditorConfig;
	::markersManager  = &this->markersManager;
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
}
//----------------------------------------------------------------------------
TasEditorWindow::~TasEditorWindow(void)
{
	printf("Destroy Tas Editor Window\n");

	if ( tasWin == this )
	{
		tasWin = NULL;
	}

	// switch off TAS Editor mode
	movieMode = MOVIEMODE_INACTIVE;
	FCEU_DispMessage("TAS Editor disengaged", 0);
	FCEUMOV_CreateCleanMovie();
}
//----------------------------------------------------------------------------
void TasEditorWindow::closeEvent(QCloseEvent *event)
{
	printf("Tas Editor Close Window Event\n");
	done(0);
	deleteLater();
	event->accept();
}
//----------------------------------------------------------------------------
void TasEditorWindow::closeWindow(void)
{
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
	//connect(act, SIGNAL(triggered()), this, SLOT(openAviFileDialog(void)) );

	fileMenu->addAction(act);

	// File -> Open
	act = new QAction(tr("&Open"), this);
	act->setShortcut(QKeySequence(tr("Ctrl+O")));
	act->setStatusTip(tr("Open Project"));
	//act->setIcon( style()->standardIcon( QStyle::SP_BrowserStop ) );
	//connect(act, SIGNAL(triggered()), this, SLOT(closeFile(void)) );

	fileMenu->addAction(act);

	// File -> Save
	act = new QAction(tr("&Save"), this);
	act->setShortcut(QKeySequence(tr("Ctrl+S")));
	act->setStatusTip(tr("Save Project"));
	//act->setIcon( style()->standardIcon( QStyle::SP_BrowserStop ) );
	//connect(act, SIGNAL(triggered()), this, SLOT(closeFile(void)) );

	fileMenu->addAction(act);

	// File -> Save As
	act = new QAction(tr("Save &As"), this);
	act->setShortcut(QKeySequence(tr("Ctrl+Shift+S")));
	act->setStatusTip(tr("Save Project As"));
	//act->setIcon( style()->standardIcon( QStyle::SP_BrowserStop ) );
	//connect(act, SIGNAL(triggered()), this, SLOT(closeFile(void)) );

	fileMenu->addAction(act);

	// File -> Save Compact
	act = new QAction(tr("Save &Compact"), this);
	//act->setShortcut(QKeySequence(tr("Ctrl+Shift+S")));
	act->setStatusTip(tr("Save Compact"));
	//act->setIcon( style()->standardIcon( QStyle::SP_BrowserStop ) );
	//connect(act, SIGNAL(triggered()), this, SLOT(closeFile(void)) );

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
}
//----------------------------------------------------------------------------
int TasEditorWindow::initModules(void)
{
	// init modules
	//editor.init();
	//pianoRoll.init();
	//selection.init();
	//splicer.init();
	//playback.init();
	//greenzone.init();
	//recorder.init();
	//markersManager.init();
	//project.init();
	//bookmarks.init();
	//branches.init();
	//popupDisplay.init();
	//history.init();
	//taseditor_lua.init();
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
		//playback.restartPlaybackFromZeroGround();
	} else
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
	//setInputType(currMovieData, getInputType(currMovieData));
	// force the input configuration stored in the movie to apply to FCEUX config
	//applyMovieInputConfig();
	// reset some modules that need MovieData info
	//pianoRoll.reset();
	//recorder.reset();
	// create initial snapshot in history
	//history.reset();
	// reset Taseditor variables
	//mustCallManualLuaFunction = false;
	
	//SetFocus(history.hwndHistoryList);		// set focus only once, to show blue selection cursor
	//SetFocus(pianoRoll.hwndList);
	FCEU_DispMessage("TAS Editor engaged", 0);
	//taseditorWindow.redraw();
	return 0;
}
//----------------------------------------------------------------------------
//----  TAS Piano Roll Widget
//----------------------------------------------------------------------------
QPianoRoll::QPianoRoll(QWidget *parent)
	: QWidget( parent )
{
	std::string fontString;

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

	this->parent = qobject_cast <TasEditorWindow*>( parent );
	this->setMouseTracking(true);

	calcFontData();

	vbar = NULL;
	hbar = NULL;
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

	//printf("W:%i  H:%i  LS:%i  \n", pxCharWidth, pxCharHeight, pxLineSpacing );

	viewLines   = (viewHeight / pxLineSpacing) + 1;
}
//----------------------------------------------------------------------------
