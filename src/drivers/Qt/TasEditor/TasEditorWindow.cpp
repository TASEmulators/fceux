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
#include <math.h>
#include <string>
#include <zlib.h>

#include <QDir>
#include <QDrag>
#include <QString>
#include <QPainter>
#include <QSettings>
#include <QTextEdit>
#include <QHeaderView>
#include <QMessageBox>
#include <QFontMetrics>
#include <QFileDialog>
#include <QFontDialog>
#include <QInputDialog>
#include <QStandardPaths>
#include <QApplication>
#include <QGuiApplication>
#include <QDesktopServices>

#include "fceu.h"
#include "movie.h"
#include "driver.h"

#include "common/vidblit.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/throttle.h"
#include "Qt/fceuWrapper.h"
#include "Qt/ColorMenu.h"
#include "Qt/ConsoleWindow.h"
#include "Qt/ConsoleUtilities.h"
#include "Qt/TasEditor/TasColors.h"
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
enum DRAG_MODES
{
	DRAG_MODE_NONE,
	DRAG_MODE_OBSERVE,
	DRAG_MODE_PLAYBACK,
	DRAG_MODE_MARKER,
	DRAG_MODE_SET,
	DRAG_MODE_UNSET,
	DRAG_MODE_SELECTION,
	DRAG_MODE_DESELECTION,
};
#define BOOKMARKS_WITH_NO_ARROW      0x00010000
#define BOOKMARKS_WITH_BLUE_ARROW    0x00020000
#define BOOKMARKS_WITH_GREEN_ARROW   0x00040000
#define BLUE_ARROW_IMAGE_ID          0x00080000
#define GREEN_ARROW_IMAGE_ID         0x00100000
#define GREEN_BLUE_ARROW_IMAGE_ID   (BLUE_ARROW_IMAGE_ID | GREEN_ARROW_IMAGE_ID)

#define MARKER_DRAG_COUNTDOWN_MAX 14
#define PIANO_ROLL_ID_LEN 11
#define PLAYBACK_WHEEL_BOOST 2

// resources
static char pianoRollSaveID[PIANO_ROLL_ID_LEN] = "PIANO_ROLL";
static char pianoRollSkipSaveID[PIANO_ROLL_ID_LEN] = "PIANO_ROLX";
static TasFindNoteWindow *findWin = NULL;

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
	: QDialog( parent, Qt::Window ), bookmarks(this), branches(this)
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

	this->taseditorConfig.load();

	clipboard = QGuiApplication::clipboard();

	setWindowTitle("TAS Editor");
	//setWindowIcon( QIcon(":icons/taseditor-icon32.png") );

	resize(512, 512);

	mainLayout = new QVBoxLayout();
	mainHBox   = new TasEditorSplitter();

	initPatterns();
	buildPianoRollDisplay();
	buildSideControlPanel();

	mainHBox->addWidget( pianoRollContainerWidget );
	mainHBox->addWidget( controlPanelContainerWidget );
	mainLayout->addWidget(mainHBox);

	mainHBox->setStretchFactor( 0, 5 );
	mainHBox->setStretchFactor( 1, 1 );

	menuBar = buildMenuBar();

	setLayout(mainLayout);
	mainLayout->setMenuBar( menuBar );
	pianoRoll->setFocus();

	for (int i=0; i<HK_MAX; i++)
	{
		hotkeyShortcut[i] = nullptr;	
	}
	initHotKeys();
	initModules();

	updateCheckedItems();
	updateToolTips();

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

	FCEU_WRAPPER_LOCK();
	//if (!askToSaveProject()) return false;

	// destroy window
	taseditorConfig.save();
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

	clearProjectList();

	FCEU_WRAPPER_UNLOCK();

	// Save Horizontal Panel State
	settings.setValue("tasEditor/hPanelState", mainHBox->saveState());

	// Save Window Geometry
	settings.setValue("tasEditor/geometry", saveGeometry());
}
//----------------------------------------------------------------------------
void TasEditorWindow::closeEvent(QCloseEvent *event)
{
	printf("Tas Editor Close Window Event\n");

	if (!askToSaveProject())
	{
	        event->ignore();
		return;
	}
	project.reset();

	done(0);
	deleteLater();
	event->accept();
}
//----------------------------------------------------------------------------
void TasEditorWindow::closeWindow(void)
{
	if (!askToSaveProject())
	{
		return;
	}
	project.reset();

	printf("Tas Editor Close Window\n");
	done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
int TasEditorWindow::requestWindowClose(void)
{
	askToSaveProject();

	project.reset();

	printf("Tas Editor Close Window\n");
	done(0);
	deleteLater();

	return 0;
}
//----------------------------------------------------------------------------
QMenuBar *TasEditorWindow::buildMenuBar(void)
{
	QMenu       *fileMenu, *editMenu, *viewMenu,
		    *confMenu, *luaMenu,  *helpMenu,
		    *patternMenu;
	QActionGroup *actGroup;
	QAction     *act;
	ColorMenuItem  *colorAct;
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
	recentProjectMenu = fileMenu->addMenu( tr("&Recent") );

	buildRecentProjectMenu();
	recentProjectMenuReset = false;

	fileMenu->addSeparator();

	// File -> Import Input
	act = new QAction(tr("&Import Input"), this);
	//act->setShortcut(QKeySequence(tr("Ctrl+Shift+S")));
	act->setStatusTip(tr("Import Input"));
	//act->setIcon( style()->standardIcon( QStyle::SP_BrowserStop ) );
	connect(act, SIGNAL(triggered()), this, SLOT(importMovieFile(void)) );

	fileMenu->addAction(act);

	// File -> Export to fm2
	act = new QAction(tr("&Export to fm2"), this);
	//act->setShortcut(QKeySequence(tr("Ctrl+Shift+S")));
	act->setStatusTip(tr("Export to fm2"));
	//act->setIcon( style()->standardIcon( QStyle::SP_BrowserStop ) );
	connect(act, SIGNAL(triggered()), this, SLOT(exportMovieFile(void)) );

	fileMenu->addAction(act);

	fileMenu->addSeparator();

	// File -> Quit
	act = new QAction(tr("&Quit Window"), this);
	act->setShortcut(QKeySequence(tr("Alt+F4")));
	act->setStatusTip(tr("Close Window"));
	act->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
	connect(act, SIGNAL(triggered()), this, SLOT(closeWindow(void)) );

	fileMenu->addAction(act);

	// Edit
	editMenu = menuBar->addMenu(tr("&Edit"));

	// Edit -> Undo
	act = new QAction(tr("&Undo"), this);
	act->setShortcut(QKeySequence(tr("Ctrl+Z")));
	act->setStatusTip(tr("Undo Changes"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered()), this, SLOT(editUndoCB(void)) );

	editMenu->addAction(act);

	// Edit -> Redo
	act = new QAction(tr("&Redo"), this);
	act->setShortcut(QKeySequence(tr("Ctrl+Y")));
	act->setStatusTip(tr("Redo Changes"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered()), this, SLOT(editRedoCB(void)) );

	editMenu->addAction(act);

	// Edit -> Selection Undo
	act = new QAction(tr("Selection &Undo"), this);
	act->setShortcut(QKeySequence(tr("Ctrl+Q")));
	act->setStatusTip(tr("Undo Selection"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered()), this, SLOT(editUndoSelCB(void)) );

	editMenu->addAction(act);

	// Edit -> Selection Redo
	act = new QAction(tr("Selection &Redo"), this);
	act->setShortcut(QKeySequence(tr("Ctrl+W")));
	act->setStatusTip(tr("Redo Selection"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered()), this, SLOT(editRedoSelCB(void)) );

	editMenu->addAction(act);

	editMenu->addSeparator();

	// Edit -> Deselect
	act = new QAction(tr("Deselect"), this);
	//act->setShortcut(QKeySequence(tr("Ctrl+W")));
	act->setStatusTip(tr("Deselect"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered()), this, SLOT(editDeselectAll(void)) );

	editMenu->addAction(act);

	// Edit -> Select All
	act = new QAction(tr("Select All"), this);
	//act->setShortcut(QKeySequence(tr("Ctrl+W")));
	act->setStatusTip(tr("Select All"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered()), this, SLOT(editSelectAll(void)) );

	editMenu->addAction(act);

	// Edit -> Select Between Markers
	act = new QAction(tr("Select Between Markers"), this);
	act->setShortcut(QKeySequence(tr("Ctrl+A")));
	act->setStatusTip(tr("Select Between Markers"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered()), this, SLOT(editSelBtwMkrs(void)) );

	editMenu->addAction(act);

	// Edit -> Reselect Clipboard
	act = new QAction(tr("Reselect Clipboard"), this);
	act->setShortcut(QKeySequence(tr("Ctrl+B")));
	act->setStatusTip(tr("Reselect Clipboard"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered()), this, SLOT(editReselectClipboard(void)) );

	editMenu->addAction(act);

	editMenu->addSeparator();

	// Edit -> Copy
	act = new QAction(tr("Copy"), this);
	act->setShortcut(QKeySequence(tr("Ctrl+C")));
	act->setStatusTip(tr("Copy"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered()), this, SLOT(editCopyCB(void)) );

	editMenu->addAction(act);

	// Edit -> Paste
	act = new QAction(tr("Paste"), this);
	act->setShortcut(QKeySequence(tr("Ctrl+V")));
	act->setStatusTip(tr("Paste"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered()), this, SLOT(editPasteCB(void)) );

	editMenu->addAction(act);

	// Edit -> Paste Insert
	act = new QAction(tr("Paste Insert"), this);
	act->setShortcut(QKeySequence(tr("Ctrl+Shift+V")));
	act->setStatusTip(tr("Paste Insert"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered()), this, SLOT(editPasteInsertCB(void)) );

	editMenu->addAction(act);

	// Edit -> Cut
	act = new QAction(tr("Cut"), this);
	act->setShortcut(QKeySequence(tr("Ctrl+X")));
	act->setStatusTip(tr("Cut"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered()), this, SLOT(editCutCB(void)) );

	editMenu->addAction(act);

	editMenu->addSeparator();

	// Edit -> Clear
	act = new QAction(tr("Clear"), this);
	act->setShortcut(QKeySequence(tr("Del")));
	act->setStatusTip(tr("Clear"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered()), this, SLOT(editClearCB(void)) );

	editMenu->addAction(act);

	// Edit -> Delete
	act = new QAction(tr("Delete"), this);
	act->setShortcut(QKeySequence(tr("Ctrl+Del")));
	act->setStatusTip(tr("Delete"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered()), this, SLOT(editDeleteCB(void)) );

	editMenu->addAction(act);

	// Edit -> Clone
	act = new QAction(tr("Clone"), this);
	act->setShortcut(QKeySequence(tr("Ctrl+Ins")));
	act->setStatusTip(tr("Clone"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered()), this, SLOT(editCloneCB(void)) );

	editMenu->addAction(act);

	// Edit -> Insert
	act = new QAction(tr("Insert"), this);
	act->setShortcut(QKeySequence(tr("Ctrl+Shift+Ins")));
	act->setStatusTip(tr("Insert"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered()), this, SLOT(editInsertCB(void)) );

	editMenu->addAction(act);

	// Edit -> Insert # of Frames
	act = new QAction(tr("Insert # of Frames"), this);
	act->setShortcut(QKeySequence(tr("Ins")));
	act->setStatusTip(tr("Insert # of Frames"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered()), this, SLOT(editInsertNumFramesCB(void)) );

	editMenu->addAction(act);

	editMenu->addSeparator();

	// Edit -> Truncate Movie
	act = new QAction(tr("Truncate Movie"), this);
	//act->setShortcut(QKeySequence(tr("Ctrl+Ins")));
	act->setStatusTip(tr("Truncate Movie"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered()), this, SLOT(editTruncateMovieCB(void)) );

	editMenu->addAction(act);

	// View
	viewMenu = menuBar->addMenu(tr("&View"));

	// View -> Find Note Window
	act = new QAction(tr("Find Note Window"), this);
	act->setShortcut(QKeySequence(tr("Ctrl+F")));
	act->setStatusTip(tr("Find Note Window"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered()), this, SLOT(openFindNoteWindow(void)) );

	viewMenu->addAction(act);

	viewMenu->addSeparator();
	
	// View -> Display Branch Screenshots
	dpyBrnchScrnAct = act = new QAction(tr("Display Branch Screenshots"), this);
	act->setCheckable(true);
	//act->setShortcut(QKeySequence(tr("Ctrl+F")));
	act->setStatusTip(tr("Display Branch Screenshots"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered(bool)), this, SLOT(dpyBrnchScrnChanged(bool)) );

	viewMenu->addAction(act);

	// View -> Display Branch Screenshots
	dpyBrnchDescAct = act = new QAction(tr("Display Branch Descriptions"), this);
	act->setCheckable(true);
	//act->setShortcut(QKeySequence(tr("Ctrl+F")));
	act->setStatusTip(tr("Display Branch Descriptions"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered(bool)), this, SLOT(dpyBrnchDescChanged(bool)) );

	viewMenu->addAction(act);

	// View -> Enable Hot Changes
	enaHotChgAct = act = new QAction(tr("Enable Hot Changes"), this);
	act->setCheckable(true);
	//act->setShortcut(QKeySequence(tr("Ctrl+F")));
	act->setStatusTip(tr("Enable Hot Changes"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered(bool)), this, SLOT(enaHotChgChanged(bool)) );

	viewMenu->addAction(act);

	viewMenu->addSeparator();

	// View -> Follow Undo Content
	followUndoAct = act = new QAction(tr("Follow Undo Content"), this);
	act->setCheckable(true);
	//act->setShortcut(QKeySequence(tr("Ctrl+F")));
	act->setStatusTip(tr("Follow Undo Content"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered(bool)), this, SLOT(followUndoActChanged(bool)) );

	viewMenu->addAction(act);

	// View -> Follow Marker Note Content
	followMkrAct = act = new QAction(tr("Follow Marker Note Content"), this);
	act->setCheckable(true);
	//act->setShortcut(QKeySequence(tr("Ctrl+F")));
	act->setStatusTip(tr("Follow Marker Note Content"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered(bool)), this, SLOT(followMkrActChanged(bool)) );

	viewMenu->addAction(act);

	viewMenu->addSeparator();

	// View -> Piano Roll Font
	act = new QAction(tr("Piano Roll Font..."), this);
	//act->setShortcut(QKeySequence(tr("Ctrl+F")));
	act->setStatusTip(tr("Select Piano Roll Font"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered(void)), this, SLOT(changePianoRollFontCB(void)) );

	viewMenu->addAction(act);

	// View -> Bookmarks Font
	act = new QAction(tr("Bookmarks View Font..."), this);
	//act->setShortcut(QKeySequence(tr("Ctrl+F")));
	act->setStatusTip(tr("Select Bookmarks View Font"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered(void)), this, SLOT(changeBookmarksFontCB(void)) );

	viewMenu->addAction(act);

	// View -> Branches Font
	act = new QAction(tr("Branches View Font..."), this);
	//act->setShortcut(QKeySequence(tr("Ctrl+F")));
	act->setStatusTip(tr("Select Branches View Font"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered(void)), this, SLOT(changeBranchesFontCB(void)) );

	viewMenu->addAction(act);

	viewMenu->addSeparator();

	// View -> Piano Roll Grid Color
	colorAct = new ColorMenuItem(tr("Piano Roll Grid Color..."), "SDL.TasPianoRollGridColor", this);
	colorAct->setStatusTip(tr("Select Piano Roll Grid Color"));

	colorAct->connectColor( &pianoRoll->gridColor );

	viewMenu->addAction(colorAct);

	// Config
	confMenu = menuBar->addMenu(tr("&Config"));

	// Config -> Project File Saving Options
	act = new QAction(tr("Project File Saving Options"), this);
	//act->setShortcut(QKeySequence(tr("Ctrl+N")));
	act->setStatusTip(tr("Project File Saving Options"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered()), this, SLOT(openProjectSaveOptions(void)) );

	confMenu->addAction(act);

	// Config -> Set Max Undo Levels
	act = new QAction(tr("Set Max Undo Levels"), this);
	//act->setShortcut(QKeySequence(tr("Ctrl+N")));
	act->setStatusTip(tr("Set Max Undo History"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered()), this, SLOT(setMaxUndoCapacity(void)) );

	confMenu->addAction(act);

	// Config -> Set Greenzone Capacity
	act = new QAction(tr("Set Greenzone Capacity"), this);
	//act->setShortcut(QKeySequence(tr("Ctrl+N")));
	act->setStatusTip(tr("Set Greenzone Capacity"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered()), this, SLOT(setGreenzoneCapacity(void)) );

	confMenu->addAction(act);

	confMenu->addSeparator();

	// Config -> Enable Greenzoneing
	enaGrnznAct = act = new QAction(tr("Enable Greenzoning"), this);
	act->setCheckable(true);
	//act->setShortcut(QKeySequence(tr("Ctrl+N")));
	act->setStatusTip(tr("Enable Greenzoning"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered(bool)), this, SLOT(enaGrnznActChanged(bool)) );

	confMenu->addAction(act);

	// Config -> Autofire Pattern skips Lag
	afPtrnSkipLagAct = act = new QAction(tr("Autofire Pattern skips Lag"), this);
	act->setCheckable(true);
	//act->setShortcut(QKeySequence(tr("Ctrl+N")));
	act->setStatusTip(tr("Autofire Pattern skips Lag"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered(bool)), this, SLOT(afPtrnSkipLagActChanged(bool)) );

	confMenu->addAction(act);

	// Config -> Auto Adjust Input According to Lag
	adjInputLagAct = act = new QAction(tr("Auto Adjust Input According to Lag"), this);
	act->setCheckable(true);
	//act->setShortcut(QKeySequence(tr("Ctrl+N")));
	act->setStatusTip(tr("Auto Adjust Input According to Lag"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered(bool)), this, SLOT(adjInputLagActChanged(bool)) );

	confMenu->addAction(act);

	confMenu->addSeparator();

	// Config -> Draw Input by Dragging
	drawInputDragAct = act = new QAction(tr("Draw Input by Dragging"), this);
	act->setCheckable(true);
	//act->setShortcut(QKeySequence(tr("Ctrl+N")));
	act->setStatusTip(tr("Draw Input by Dragging"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered(bool)), this, SLOT(drawInputDragActChanged(bool)) );

	confMenu->addAction(act);

	// Config -> Combine Consecutive Recordings/Draws
	cmbRecDrawAct = act = new QAction(tr("Combine Consecutive Recordings/Draws"), this);
	act->setCheckable(true);
	//act->setShortcut(QKeySequence(tr("Ctrl+N")));
	act->setStatusTip(tr("Combine Consecutive Recordings/Draws"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered(bool)), this, SLOT(cmbRecDrawActChanged(bool)) );

	confMenu->addAction(act);

	// Config -> Use 1P Keys for all Single Recordings
	use1PforRecAct = act = new QAction(tr("Use 1P Keys for all Single Recordings"), this);
	act->setCheckable(true);
	//act->setShortcut(QKeySequence(tr("Ctrl+N")));
	act->setStatusTip(tr("Use 1P Keys for all Single Recordings"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered(bool)), this, SLOT(use1PforRecActChanged(bool)) );

	confMenu->addAction(act);

	// Config -> Use Input Keys for Column Set
	useInputColSetAct = act = new QAction(tr("Use Input Keys for Column Set"), this);
	act->setCheckable(true);
	//act->setShortcut(QKeySequence(tr("Ctrl+N")));
	act->setStatusTip(tr("Use Input Keys for Column Set"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered(bool)), this, SLOT(useInputColSetActChanged(bool)) );

	confMenu->addAction(act);

	confMenu->addSeparator();

	// Config -> Bind Markers to Input
	bindMkrInputAct = act = new QAction(tr("Bind Markers to Input"), this);
	act->setCheckable(true);
	//act->setShortcut(QKeySequence(tr("Ctrl+N")));
	act->setStatusTip(tr("Bind Markers to Input"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered(bool)), this, SLOT(bindMkrInputActChanged(bool)) );

	confMenu->addAction(act);

	// Config -> Empty New Marker Notes
	emptyNewMkrNotesAct = act = new QAction(tr("Empty New Marker Notes"), this);
	act->setCheckable(true);
	//act->setShortcut(QKeySequence(tr("Ctrl+N")));
	act->setStatusTip(tr("Empty New Marker Notes"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered(bool)), this, SLOT(emptyNewMkrNotesActChanged(bool)) );

	confMenu->addAction(act);

	confMenu->addSeparator();

	// Config -> Old Control Scheme for Branching
	oldCtlBrnhSchemeAct = act = new QAction(tr("Old Control Scheme for Branching"), this);
	act->setCheckable(true);
	//act->setShortcut(QKeySequence(tr("Ctrl+N")));
	act->setStatusTip(tr("Old Control Scheme for Branching"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered(bool)), this, SLOT(oldCtlBrnhSchemeActChanged(bool)) );

	confMenu->addAction(act);

	// Config -> Branches Restore Entire Movie
	brnchRestoreMovieAct = act = new QAction(tr("Branches Restore Entire Movie"), this);
	act->setCheckable(true);
	//act->setShortcut(QKeySequence(tr("Ctrl+N")));
	act->setStatusTip(tr("Branches Restore Entire Movie"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered(bool)), this, SLOT(brnchRestoreMovieActChanged(bool)) );

	confMenu->addAction(act);

	// Config -> HUD in Branch Screenshots
	hudInScrnBranchAct = act = new QAction(tr("HUD in Branch Screenshots"), this);
	act->setCheckable(true);
	//act->setShortcut(QKeySequence(tr("Ctrl+N")));
	act->setStatusTip(tr("HUD in Branch Screenshots"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered(bool)), this, SLOT(hudInScrnBranchActChanged(bool)) );

	confMenu->addAction(act);

	confMenu->addSeparator();

	// Config -> Autopause at End of Movie
	pauseAtEndAct = act = new QAction(tr("Autopause at End of Movie"), this);
	act->setCheckable(true);
	//act->setShortcut(QKeySequence(tr("Ctrl+N")));
	act->setStatusTip(tr("Autopause at End of Movie"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered(bool)), this, SLOT(pauseAtEndActChanged(bool)) );

	confMenu->addAction(act);

	// Lua
	luaMenu = menuBar->addMenu(tr("&Lua"));

	// Lua -> Run Function
	act = new QAction(tr("Run Function"), this);
	//act->setShortcut(QKeySequence(tr("Ctrl+N")));
	act->setStatusTip(tr("Run Function"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered()), this, SLOT(manLuaRun(void)) );

	luaMenu->addAction(act);

	luaMenu->addSeparator();

	// Lua -> Auto Function
	autoLuaAct = act = new QAction(tr("Auto Function"), this);
	act->setCheckable(true);
	//act->setShortcut(QKeySequence(tr("Ctrl+N")));
	act->setStatusTip(tr("Auto Function"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered(bool)), this, SLOT(autoLuaRunChanged(bool)) );

	luaMenu->addAction(act);

	// Pattern
	patternMenu = menuBar->addMenu(tr("&Pattern"));

	actGroup = new QActionGroup(this);

	for (size_t i=0; i<patternsNames.size(); i++)
	{
		// Pattern -> Names
		act = new QAction(tr(patternsNames[i].c_str()), this);
		act->setCheckable(true);
		//act->setShortcut(QKeySequence(tr("Ctrl+N")));
		act->setStatusTip(tr(patternsNames[i].c_str()));
		//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
		connect(act, &QAction::triggered, [this, i] { setCurrentPattern(i); } );

		actGroup->addAction(act);
		patternMenu->addAction(act);

		act->setChecked( taseditorConfig.currentPattern == i );
	}

	// Help
	helpMenu = menuBar->addMenu(tr("&Help"));

	// Help -> Open TAS Editor Manual
	act = new QAction(tr("Open TAS Editor Manual"), this);
	//act->setShortcut(QKeySequence(tr("Ctrl+N")));
	act->setStatusTip(tr("Open TAS Editor Manual"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered()), this, SLOT(openOnlineDocs(void)) );

	helpMenu->addAction(act);

	// Help -> Enable Tool Tips
	showToolTipsAct = act = new QAction(tr("Enable Tool Tips"), this);
	act->setCheckable(true);
	//act->setShortcut(QKeySequence(tr("Ctrl+N")));
	act->setStatusTip(tr("Enable Tool Tips"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered(bool)), this, SLOT(showToolTipsActChanged(bool)) );

	helpMenu->addAction(act);

	helpMenu->addSeparator();

	// Help -> About
	act = new QAction(tr("About"), this);
	//act->setShortcut(QKeySequence(tr("Ctrl+N")));
	act->setStatusTip(tr("About"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered()), this, SLOT(openAboutWindow(void)) );

	helpMenu->addAction(act);

	return menuBar;
}
//----------------------------------------------------------------------------
void TasEditorWindow::buildPianoRollDisplay(void)
{
	QVBoxLayout *vbox;
	QHBoxLayout *hbox;
	QGridLayout *grid;

	pianoRollFrame   = new QFrame();
	grid             = new QGridLayout();
	pianoRoll        = new QPianoRoll(this);
	pianoRollVBar    = new PianoRollScrollBar( this );
	pianoRollHBar    = new QScrollBar( Qt::Horizontal, this );
	upperMarkerLabel = new QPushButton( tr("Marker 0") );
	lowerMarkerLabel = new QPushButton( tr("Marker 0") );
	upperMarkerNote  = new UpperMarkerNoteEdit();
	lowerMarkerNote  = new LowerMarkerNoteEdit();

	//upperMarkerLabel->setFlat(true);
	//lowerMarkerLabel->setFlat(true);

	pianoRollFrame->setLineWidth(2);
	pianoRollFrame->setMidLineWidth(1);
	//pianoRollFrame->setFrameShape( QFrame::StyledPanel );
	pianoRollFrame->setFrameShape( QFrame::Box );

	pianoRollVBar->setInvertedControls(false);
	pianoRollVBar->setInvertedAppearance(false);
	pianoRoll->setScrollBars( pianoRollHBar, pianoRollVBar );
	connect( pianoRollHBar, SIGNAL(valueChanged(int)), pianoRoll, SLOT(hbarChanged(int)) );
	connect( pianoRollVBar, SIGNAL(valueChanged(int)), pianoRoll, SLOT(vbarChanged(int)) );
	//connect( pianoRollVBar, SIGNAL(actionTriggered(int)), pianoRoll, SLOT(vbarActionTriggered(int)) );

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
	hbox->addWidget( upperMarkerNote, 10 );

	vbox->addLayout( hbox, 1 );
	vbox->addWidget( pianoRollFrame, 100 );
	//vbox->addLayout( grid, 100 );
	pianoRollFrame->setLayout( grid );

	hbox = new QHBoxLayout();
	hbox->addWidget( lowerMarkerLabel, 1 );
	hbox->addWidget( lowerMarkerNote, 10 );

	vbox->addLayout( hbox, 1 );
	
	pianoRollContainerWidget = new QWidget();
	pianoRollContainerWidget->setLayout( vbox );

	connect( upperMarkerLabel, SIGNAL(clicked(void)), this, SLOT(upperMarkerLabelClicked(void)) );
	connect( lowerMarkerLabel, SIGNAL(clicked(void)), this, SLOT(lowerMarkerLabelClicked(void)) );
}
//----------------------------------------------------------------------------
void TasEditorWindow::initPatterns(void)
{
	if (patterns.size() == 0)
	{
		FCEU_printf("Will be using default set of patterns...\n");
		patterns.resize(4);
		patternsNames.resize(4);
		// Default Pattern 0: Alternating (1010...)
		patternsNames[0] = "Alternating (1010...)";
		patterns[0].resize(2);
		patterns[0][0] = 1;
		patterns[0][1] = 0;
		// Default Pattern 1: Alternating at 30FPS (11001100...)
		patternsNames[1] = "Alternating at 30FPS (11001100...)";
		patterns[1].resize(4);
		patterns[1][0] = 1;
		patterns[1][1] = 1;
		patterns[1][2] = 0;
		patterns[1][3] = 0;
		// Default Pattern 2: One Quarter (10001000...)
		patternsNames[2] = "One Quarter (10001000...)";
		patterns[2].resize(4);
		patterns[2][0] = 1;
		patterns[2][1] = 0;
		patterns[2][2] = 0;
		patterns[2][3] = 0;
		// Default Pattern 3: Tap'n'Hold (1011111111111111111111111111111111111...)
		patternsNames[3] = "Tap'n'Hold (101111111...)";
		patterns[3].resize(1000);
		patterns[3][0] = 1;
		patterns[3][1] = 0;
		for (int i = 2; i < 1000; ++i)
		{
			patterns[3][i] = 1;
		}
	}
}
//----------------------------------------------------------------------------
void TasEditorWindow::buildSideControlPanel(void)
{
	QShortcut   *shortcut;
	QVBoxLayout *vbox;
	QHBoxLayout *hbox;
	QGridLayout *grid;
	QScrollArea *scrollArea1, *scrollArea2;
	QTreeWidgetItem *item;

	ctlPanelMainVbox = new QVBoxLayout();

	playbackGBox  = new QGroupBox( tr("Playback") );
	recorderGBox  = new QGroupBox( tr("Recorder") );
	splicerGBox   = new QGroupBox( tr("Splicer") );
	//luaGBox       = new QGroupBox( tr("Lua") );
	//historyGBox   = new QGroupBox( tr("History") );
	bbFrame       = new QFrame();

	bbFrame->setFrameShape( QFrame::StyledPanel );

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

	//runLuaBtn   = new QPushButton( tr("Run Function") );
	//autoLuaCBox = new QCheckBox( tr("Auto Function") );
	//runLuaBtn->setEnabled(false);
	//autoLuaCBox->setChecked(true);

	histTree = new QTreeWidget();

	histTree->setColumnCount(1);
	histTree->setSelectionMode( QAbstractItemView::SingleSelection );
	histTree->setAlternatingRowColors(true);

	item = new QTreeWidgetItem();
	item->setText(0, QString::fromStdString("Time / Description"));

	histTree->setHeaderItem(item);

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

	//hbox = new QHBoxLayout();
	//hbox->addWidget( runLuaBtn );
	//hbox->addWidget( autoLuaCBox );
	//luaGBox->setLayout( hbox );
	
	scrollArea1 = new QScrollArea();
	scrollArea1->setWidgetResizable(false);
	scrollArea1->setSizeAdjustPolicy( QAbstractScrollArea::AdjustToContents );
	scrollArea1->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	scrollArea1->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	scrollArea1->setMinimumSize( QSize( 128, 128 ) );
	scrollArea1->setWidget( &bookmarks );

	scrollArea2 = new QScrollArea();
	scrollArea2->setWidgetResizable(true);
	scrollArea2->setSizeAdjustPolicy( QAbstractScrollArea::AdjustToContents );
	scrollArea2->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	scrollArea2->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	scrollArea2->setMinimumSize( QSize( 128, 128 ) );
	scrollArea2->setWidget( &branches );

	bkmkBrnchStack = new QTabWidget();
	bkmkBrnchStack->addTab( scrollArea1, tr("Bookmarks") );
	bkmkBrnchStack->addTab( scrollArea2, tr("Branches")  );
	bkmkBrnchStack->addTab( histTree   , tr("History")   );

	taseditorConfig.displayBranchesTree = 0;

	vbox = new QVBoxLayout();
	vbox->addWidget( bkmkBrnchStack );
	bbFrame->setLayout( vbox );

	//vbox = new QVBoxLayout();
	//vbox->addWidget( histTree );
	//historyGBox->setLayout( vbox );

	ctlPanelMainVbox->addWidget( playbackGBox  );
	ctlPanelMainVbox->addWidget( recorderGBox  );
	ctlPanelMainVbox->addWidget( splicerGBox   );
	//ctlPanelMainVbox->addWidget( luaGBox       );
	ctlPanelMainVbox->addWidget( bbFrame       );
	//ctlPanelMainVbox->addWidget( historyGBox   );

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

	connect( rewindMkrBtn, SIGNAL(pressed(void)), this, SLOT(playbackFrameRewindFull(void)) );
	connect( rewindFrmBtn, SIGNAL(pressed(void)), this, SLOT(playbackFrameRewind(void))     );
	connect( playPauseBtn, SIGNAL(pressed(void)), this, SLOT(playbackPauseCB(void))         );
	connect( advFrmBtn   , SIGNAL(pressed(void)), this, SLOT(playbackFrameForward(void))    );
	connect( advMkrBtn   , SIGNAL(pressed(void)), this, SLOT(playbackFrameForwardFull(void)));

	connect( followCursorCbox, SIGNAL(clicked(bool)), this, SLOT(playbackFollowCursorCb(bool)));
	connect( turboSeekCbox   , SIGNAL(clicked(bool)), this, SLOT(playbackTurboSeekCb(bool)));
	connect( autoRestoreCbox , SIGNAL(clicked(bool)), this, SLOT(playbackAutoRestoreCb(bool)));

	connect( prevMkrBtn, SIGNAL(clicked(void)), this, SLOT(jumpToPreviousMarker(void)) );
	connect( nextMkrBtn, SIGNAL(clicked(void)), this, SLOT(jumpToNextMarker(void)) );
	connect( similarBtn, SIGNAL(clicked(void)), this, SLOT(findSimilarNote(void)) );
	connect( moreBtn   , SIGNAL(clicked(void)), this, SLOT(findNextSimilarNote(void)) );

	//shortcut = new QShortcut( QKeySequence("Pause"), this);
	//connect( shortcut, SIGNAL(activated(void)), this, SLOT(playbackPauseCB(void)) );

	shortcut = new QShortcut( QKeySequence("Shift+Up"), this);
	connect( shortcut, SIGNAL(activated(void)), this, SLOT(playbackFrameRewind(void)) );

	shortcut = new QShortcut( QKeySequence("Shift+Down"), this);
	connect( shortcut, SIGNAL(activated(void)), this, SLOT(playbackFrameForward(void)) );

	shortcut = new QShortcut( QKeySequence("Shift+PgUp"), this);
	connect( shortcut, SIGNAL(activated(void)), this, SLOT(playbackFrameRewindFull(void)) );

	shortcut = new QShortcut( QKeySequence("Shift+PgDown"), this);
	connect( shortcut, SIGNAL(activated(void)), this, SLOT(playbackFrameForwardFull(void)) );

	shortcut = new QShortcut( QKeySequence("Ctrl+Up"), this);
	connect( shortcut, SIGNAL(activated(void)), this, SLOT(scrollSelectionUpOne(void)) );

	shortcut = new QShortcut( QKeySequence("Ctrl+Down"), this);
	connect( shortcut, SIGNAL(activated(void)), this, SLOT(scrollSelectionDnOne(void)) );

	connect( histTree, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(histTreeItemActivated(QTreeWidgetItem*,int) ) );
	connect( histTree, SIGNAL(itemActivated(QTreeWidgetItem*,int)), this, SLOT(histTreeItemActivated(QTreeWidgetItem*,int) ) );

	connect( bkmkBrnchStack, SIGNAL(currentChanged(int)), this, SLOT(tabViewChanged(int) ) );
}
//----------------------------------------------------------------------------
void TasEditorWindow::initHotKeys(void)
{
	for (int i=0; i<HK_MAX; i++)
	{
		QKeySequence ks = Hotkeys[i].getKeySeq();
		QShortcut *shortcut = Hotkeys[i].getShortcut();

		//printf("HotKey: %i   %s\n", i, ks.toString().toStdString().c_str() );

		if ( hotkeyShortcut[i] == nullptr )
		{
			hotkeyShortcut[i] = new QShortcut( ks, this );

			if ( shortcut != nullptr )
			{
				connect( hotkeyShortcut[i], &QShortcut::activated, [ this, i, shortcut ] { activateHotkey( i, shortcut ); } );
			}
		}
		else
		{
			hotkeyShortcut[i]->setKey( ks );
		}
	}

	// Frame Advance uses key state directly, disable shortcut events
	hotkeyShortcut[HK_FRAME_ADVANCE]->setEnabled(false);
	hotkeyShortcut[HK_TURBO        ]->setEnabled(false);

	// Disable shortcuts that are not allowed with TAS Editor
	hotkeyShortcut[HK_OPEN_ROM      ]->setEnabled(false);
	hotkeyShortcut[HK_CLOSE_ROM     ]->setEnabled(false);
	hotkeyShortcut[HK_QUIT          ]->setEnabled(false);
	hotkeyShortcut[HK_FULLSCREEN    ]->setEnabled(false);
	hotkeyShortcut[HK_MAIN_MENU_HIDE]->setEnabled(false);
	hotkeyShortcut[HK_LOAD_LUA      ]->setEnabled(false);
	hotkeyShortcut[HK_FA_LAG_SKIP   ]->setEnabled(false);
}
//----------------------------------------------------------------------------
void TasEditorWindow::activateHotkey( int hkIdx, QShortcut *shortcut )
{
	shortcut->activated();
}
//----------------------------------------------------------------------------
void TasEditorWindow::updateRecordStatus(void)
{
	recRecordingCbox->setChecked( !movie_readonly );
}
//----------------------------------------------------------------------------
void TasEditorWindow::updateCheckedItems(void)
{

	followCursorCbox->setChecked( taseditorConfig.followPlaybackCursor );
	autoRestoreCbox->setChecked( taseditorConfig.autoRestoreLastPlaybackPosition );
	turboSeekCbox->setChecked( taseditorConfig.turboSeek );

	if ( taseditorConfig.superimpose == SUPERIMPOSE_CHECKED )
	{
		recSuperImposeCbox->setCheckState( Qt::Checked );
	}
	else if ( taseditorConfig.superimpose == SUPERIMPOSE_INDETERMINATE )
	{
		recSuperImposeCbox->setCheckState( Qt::PartiallyChecked );
	}
	else
	{	//taseditorConfig.superimpose == SUPERIMPOSE_UNCHECKED;
		recSuperImposeCbox->setCheckState( Qt::Unchecked );
	}
	recRecordingCbox->setChecked( !movie_readonly );
	recUsePatternCbox->setChecked( taseditorConfig.recordingUsePattern );
	dpyBrnchScrnAct->setChecked( taseditorConfig.displayBranchScreenshots );
	dpyBrnchDescAct->setChecked( taseditorConfig.displayBranchDescriptions );
	enaHotChgAct->setChecked( taseditorConfig.enableHotChanges );
	followMkrAct->setChecked( taseditorConfig.followMarkerNoteContext );
	followUndoAct->setChecked( taseditorConfig.followUndoContext );
	//autoLuaCBox->setChecked( taseditorConfig.enableLuaAutoFunction );
	autoLuaAct->setChecked( taseditorConfig.enableLuaAutoFunction );
	enaGrnznAct->setChecked( taseditorConfig.enableGreenzoning );
	afPtrnSkipLagAct->setChecked( taseditorConfig.autofirePatternSkipsLag );
	adjInputLagAct->setChecked( taseditorConfig.autoAdjustInputAccordingToLag );
	drawInputDragAct->setChecked( taseditorConfig.drawInputByDragging );
	cmbRecDrawAct->setChecked( taseditorConfig.combineConsecutiveRecordingsAndDraws );
	use1PforRecAct->setChecked( taseditorConfig.use1PKeysForAllSingleRecordings );
	useInputColSetAct->setChecked( taseditorConfig.useInputKeysForColumnSet );
	bindMkrInputAct->setChecked( taseditorConfig.bindMarkersToInput );
	emptyNewMkrNotesAct->setChecked( taseditorConfig.emptyNewMarkerNotes );
	oldCtlBrnhSchemeAct->setChecked( taseditorConfig.oldControlSchemeForBranching );
	brnchRestoreMovieAct->setChecked( taseditorConfig.branchesRestoreEntireMovie );
	hudInScrnBranchAct->setChecked( taseditorConfig.HUDInBranchScreenshots );
	pauseAtEndAct->setChecked( taseditorConfig.autopauseAtTheEndOfMovie );
	showToolTipsAct->setChecked( taseditorConfig.tooltipsEnabled );
}
//----------------------------------------------------------------------------
bool TasEditorWindow::updateHistoryItems(void)
{
	int i, cursorPos;
	QTreeWidgetItem *item;
	const char *txt;
	bool isVisible;

	isVisible = histTree->isVisible();

	if ( !isVisible )
	{
		return false;
	}

	cursorPos = history.getCursorPos();

	for (i=0; i<history.getNumItems(); i++)
	{
		txt = history.getItemDesc(i);

		item = histTree->topLevelItem(i);

		if (item == NULL)
		{
			item = new QTreeWidgetItem();

			histTree->addTopLevelItem(item);

			//histTree->setCurrentItem(item);
		}

		if ( txt )
		{
			if ( item->text(0).compare( tr(txt) ) != 0 )
			{
				item->setText(0, tr(txt));

				//histTree->setCurrentItem(item);
			}
		}
		if ( cursorPos == i )
		{
			histTree->setCurrentItem(item);
		}
	}

	while ( (histTree->topLevelItemCount() > 0) && (history.getNumItems() < histTree->topLevelItemCount()) )
	{
		item = histTree->takeTopLevelItem( histTree->topLevelItemCount()-1 );

		if ( item )
		{
			delete item;
		}
	}
	histTree->viewport()->update();

	return true;
}
//----------------------------------------------------------------------------
QPoint TasEditorWindow::getPreviewPopupCoordinates(void)
{
	return bkmkBrnchStack->mapToGlobal(QPoint(0,0));
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
	pianoRoll->reset();
	recorder.reset();
	// create initial snapshot in history
	history.reset();
	// reset Taseditor variables
	mustCallManualLuaFunction = false;
	
	//SetFocus(history.hwndHistoryList);		// set focus only once, to show blue selection cursor
	//SetFocus(pianoRoll.hwndList);
	FCEU_DispMessage("TAS Editor engaged", 0);
	update();
	return 0;
}
//----------------------------------------------------------------------------
void TasEditorWindow::frameUpdate(void)
{
	FCEU_WRAPPER_LOCK();

	//printf("TAS Frame Update: %zi\n", currMovieData.records.size());

	//taseditorWindow.update();
	greenzone.update();
	recorder.update();
	pianoRoll->periodicUpdate();
	markersManager.update();
	playback.update();
	bookmarks.update();
	branches.update();
	//popupDisplay.update();
	selection.update();
	splicer.update();
	history.update();
	project.update();

#ifdef _S9XLUA_H
	// run Lua functions if needed
	if (taseditorConfig.enableLuaAutoFunction)
	{
		TaseditorAutoFunction();
	}
	if (mustCallManualLuaFunction)
	{
		TaseditorManualFunction();
		mustCallManualLuaFunction = false;
	}
#endif

	pianoRoll->update();

	if ( recentProjectMenuReset )
	{
		buildRecentProjectMenu();
		recentProjectMenuReset = false;
	}

	FCEU_WRAPPER_UNLOCK();
}
//----------------------------------------------------------------------------
bool TasEditorWindow::loadProject(const char* fullname)
{
	bool success = false;

	FCEU_WRAPPER_LOCK();

	// try to load project
	if (project.load(fullname))
	{
		// loaded successfully
		applyMovieInputConfig();
		// add new file to Recent menu
		addRecentProject( fullname );
		updateCaption();
		update();
		success = true;
	}
	else
	{
		// failed to load
		updateCaption();
		update();
	}
	FCEU_WRAPPER_UNLOCK();

	return success;
}
bool TasEditorWindow::saveProject(bool save_compact)
{
	bool ret = true;

	FCEU_WRAPPER_LOCK();

	if (project.getProjectFile().empty())
	{
		ret = saveProjectAs(save_compact);
	}
	else
	{
		if (save_compact)
		{
			project.save(0, taseditorConfig.saveCompact_SaveInBinary, taseditorConfig.saveCompact_SaveMarkers, taseditorConfig.saveCompact_SaveBookmarks, taseditorConfig.saveCompact_GreenzoneSavingMode, taseditorConfig.saveCompact_SaveHistory, taseditorConfig.saveCompact_SavePianoRoll, taseditorConfig.saveCompact_SaveSelection);
		}
		else
		{
			project.save(0, taseditorConfig.projectSavingOptions_SaveInBinary, taseditorConfig.projectSavingOptions_SaveMarkers, taseditorConfig.projectSavingOptions_SaveBookmarks, taseditorConfig.projectSavingOptions_GreenzoneSavingMode, taseditorConfig.projectSavingOptions_SaveHistory, taseditorConfig.projectSavingOptions_SavePianoRoll, taseditorConfig.projectSavingOptions_SaveSelection);
		}
		updateCaption();
	}

	FCEU_WRAPPER_UNLOCK();

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
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DesktopLocation).first());
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
			strcat( baseName, ".fm3");

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
	QFileInfo fi( filename );

	if ( fi.exists() )
	{
		int ret;
		std::string msg;

		msg = "Pre-existing TAS project file will be overwritten:\n\n" +
			fi.fileName().toStdString() + "\n\nReplace file?";

		ret = QMessageBox::warning( this, QObject::tr("Overwrite Warning"),
				QString::fromStdString(msg), QMessageBox::Yes | QMessageBox::No, QMessageBox::No );

		if ( ret == QMessageBox::No )
		{
			return false;
		}
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
	addRecentProject( filename.toStdString().c_str() );
	// saved successfully - remove * mark from caption
	project.reset();
	updateCaption();
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
				QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes );

		//int answer = MessageBox(taseditorWindow.hwndTASEditor, "Save Project changes?", "TAS Editor", MB_YESNOCANCEL);
		if (ans == QMessageBox::Yes)
		{
			return saveProject();
		}
		return (ans != QMessageBox::Cancel);
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
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DesktopLocation).first());
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
	int ret;
	QDialog dialog(this);
	QGroupBox *gbox;
	QVBoxLayout *mainLayout, *vbox;
	QHBoxLayout *hbox;
	QPushButton *okButton, *cancelButton;
	QRadioButton *p1, *p2, *p4;
	QCheckBox    *copyInput, *copyMarkers;
	QLineEdit    *authorEdit;
	static struct NewProjectParameters params;

	if (!askToSaveProject())
	{
		return;
	}
	
	params.inputType = getInputType(currMovieData);
	params.copyCurrentInput = params.copyCurrentMarkers = false;
	if (strlen(taseditorConfig.lastAuthorName) > 0)
	{
		int i=0;
		// convert UTF8 char* string to Unicode wstring
		wchar_t savedAuthorName[AUTHOR_NAME_MAX_LEN] = {0};

		while ( taseditorConfig.lastAuthorName[i] != 0 )
		{
			savedAuthorName[i] = taseditorConfig.lastAuthorName[i]; i++;
		}
		savedAuthorName[i] = 0;
		params.authorName = savedAuthorName;
	}
	else
	{
		params.authorName = L"";
	}

	mainLayout = new QVBoxLayout();
	hbox       = new QHBoxLayout();
	vbox       = new QVBoxLayout();
	gbox       = new QGroupBox( tr("Input Type") );

	mainLayout->addLayout( hbox );
	hbox->addWidget( gbox );
	gbox->setLayout( vbox );

	p1 = new QRadioButton( tr("1 Player") );
	p2 = new QRadioButton( tr("2 Players") );
	p4 = new QRadioButton( tr("4 Score") );

	p1->setChecked( params.inputType == INPUT_TYPE_1P );
	p2->setChecked( params.inputType == INPUT_TYPE_2P );
	p4->setChecked( params.inputType == INPUT_TYPE_FOURSCORE );

	vbox->addWidget( p1 );
	vbox->addWidget( p2 );
	vbox->addWidget( p4 );

	vbox       = new QVBoxLayout();
	hbox->addLayout( vbox );

	copyInput   = new QCheckBox( tr("Copy Input") );
	copyMarkers = new QCheckBox( tr("Copy Markers") );

	vbox->addWidget( copyInput );
	vbox->addWidget( copyMarkers );

	hbox       = new QHBoxLayout();
	mainLayout->addLayout( hbox );

	authorEdit = new QLineEdit();
	hbox->addWidget( new QLabel( tr("Author") ), 1 );
	hbox->addWidget( authorEdit, 5 );

	hbox       = new QHBoxLayout();
	mainLayout->addLayout( hbox );

	okButton     = new QPushButton( tr("Ok") );
	cancelButton = new QPushButton( tr("Cancel") );

	hbox->addWidget( cancelButton, 1 );
	hbox->addStretch( 5 );
	hbox->addWidget( okButton    , 1 );

	okButton->setIcon( style()->standardIcon( QStyle::SP_DialogApplyButton ) );
	cancelButton->setIcon( style()->standardIcon( QStyle::SP_DialogCancelButton ) );

	connect(     okButton, SIGNAL(clicked(void)), &dialog, SLOT(accept(void)) );
	connect( cancelButton, SIGNAL(clicked(void)), &dialog, SLOT(reject(void)) );

	dialog.setLayout( mainLayout );

	dialog.setWindowTitle( tr("Create New Project") );

	okButton->setDefault(true);

	ret = dialog.exec();
	
	if ( p4->isChecked() )
	{
		params.inputType = INPUT_TYPE_FOURSCORE;
	}
	else if ( p2->isChecked() )
	{
		params.inputType = INPUT_TYPE_2P;
	}
	else
	{
		params.inputType = INPUT_TYPE_1P;
	}
	params.copyCurrentInput   = copyInput->isChecked();
	params.copyCurrentMarkers = copyMarkers->isChecked();
	params.authorName = authorEdit->text().toStdWString();

	FCEU_WRAPPER_LOCK();

	if ( QDialog::Accepted == ret )
	{
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
		pianoRoll->reset();
		selection.reset();
		//editor.reset();
		splicer.reset();
		recorder.reset();
		//popupDisplay.reset();
		//taseditorWindow.redraw();
		updateCaption();
		update();
	}
	FCEU_WRAPPER_UNLOCK();
}
//----------------------------------------------------------------------------
void TasEditorWindow::importMovieFile( const char *path )
{
	EMUFILE_FILE ifs( path, "rb");

	// Load Input to temporary moviedata
	MovieData md;
	if (LoadFM2(md, &ifs, ifs.size(), false))
	{
		QFileInfo fi( path );
		// loaded successfully, now register Input changes
		//char drv[512], dir[512], name[1024], ext[512];
		//splitpath(filename.toStdString().c_str(), drv, dir, name, ext);
		//strcat(name, ext);
		int result = history.registerImport(md, fi.fileName().toStdString().c_str() );
		if (result >= 0)
		{
			greenzone.invalidateAndUpdatePlayback(result);
			greenzone.lagLog.invalidateFromFrame(result);
			// keep current snapshot laglog in touch
			history.getCurrentSnapshot().laglog.invalidateFromFrame(result);
		}
		else
		{
			//MessageBox(taseditorWindow.hwndTASEditor, "Imported movie has the same Input.\nNo changes were made.", "TAS Editor", MB_OK);
		}
	}
	else
	{
		FCEUD_PrintError("Error loading movie data!");
	}
}
//----------------------------------------------------------------------------
void TasEditorWindow::importMovieFile(void)
{
	std::string last;
	int ret, useNativeFileDialogVal;
	QString filename;
	std::string lastPath;
	//char dir[512];
	const char *base, *rom;
	QFileDialog  dialog(this, tr("Import Movie File") );
	QList<QUrl> urls;
	QDir d;

	dialog.setFileMode(QFileDialog::ExistingFile);

	dialog.setNameFilter(tr("FCEUX Movie Files (*.fm2) ;; TAS Project Files (*.fm3) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::AllDirs | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Import") );

	base = FCEUI_GetBaseDirectory();

	urls << QUrl::fromLocalFile( QDir::rootPath() );
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DesktopLocation).first());
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
	dialog.setDefaultSuffix( tr(".fm2") );

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

	importMovieFile( filename.toStdString().c_str() );

	//EMUFILE_FILE ifs( filename.toStdString().c_str(), "rb");

	//// Load Input to temporary moviedata
	//MovieData md;
	//if (LoadFM2(md, &ifs, ifs.size(), false))
	//{
	//	QFileInfo fi( filename );
	//	// loaded successfully, now register Input changes
	//	//char drv[512], dir[512], name[1024], ext[512];
	//	//splitpath(filename.toStdString().c_str(), drv, dir, name, ext);
	//	//strcat(name, ext);
	//	int result = history.registerImport(md, fi.fileName().toStdString().c_str() );
	//	if (result >= 0)
	//	{
	//		greenzone.invalidateAndUpdatePlayback(result);
	//		greenzone.lagLog.invalidateFromFrame(result);
	//		// keep current snapshot laglog in touch
	//		history.getCurrentSnapshot().laglog.invalidateFromFrame(result);
	//	}
	//	else
	//	{
	//		//MessageBox(taseditorWindow.hwndTASEditor, "Imported movie has the same Input.\nNo changes were made.", "TAS Editor", MB_OK);
	//	}
	//}
	//else
	//{
	//	FCEUD_PrintError("Error loading movie data!");
	//}

	return;
}
//----------------------------------------------------------------------------
void TasEditorWindow::exportMovieFile(void)
{
	std::string last;
	int ret, useNativeFileDialogVal;
	QString filename;
	std::string lastPath;
	//char dir[512];
	const char *base, *rom;
	QFileDialog  dialog(this, tr("Export to FM2 File") );
	QList<QUrl> urls;
	QDir d;

	dialog.setFileMode(QFileDialog::AnyFile);

	dialog.setNameFilter(tr("FCEUX Movie File (*.fm2) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::AllDirs | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Export") );

	base = FCEUI_GetBaseDirectory();

	urls << QUrl::fromLocalFile( QDir::rootPath() );
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DesktopLocation).first());
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
	dialog.setDefaultSuffix( tr(".fm2") );

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

	EMUFILE* osRecordingMovie = FCEUD_UTF8_fstream( filename.toStdString().c_str(), "wb");
	// create copy of current movie data
	MovieData temp_md = currMovieData;
	// modify the copy according to selected type of export
	setInputType(temp_md, taseditorConfig.lastExportedInputType);
	temp_md.loadFrameCount = -1;
	// also add subtitles if needed
	if (taseditorConfig.lastExportedSubtitlesStatus)
	{
		// convert Marker Notes to Movie Subtitles
		char framenum[16];
		std::string subtitle;
		int markerID;
		for (int i = 0; i < markersManager.getMarkersArraySize(); ++i)
		{
			markerID = markersManager.getMarkerAtFrame(i);
			if (markerID)
			{
				sprintf( framenum, "%i ", i );
				//_itoa(i, framenum, 10);
				//strcat(framenum, " ");
				subtitle = framenum;
				subtitle.append(markersManager.getNoteCopy(markerID));
				temp_md.subtitles.push_back(subtitle);
			}
		}
	}
	// dump to disk
	temp_md.dump(osRecordingMovie, false);
	delete osRecordingMovie;
	osRecordingMovie = 0;

}
//----------------------------------------------------------------------------
void TasEditorWindow::updateCaption(void)
{
	char newCaption[300];
	strcpy(newCaption, "TAS Editor");
	if (!movie_readonly)
	{
		strcat(newCaption, recorder.getRecordingCaption());
	}
	// add project name
	std::string projectname = project.getProjectName();
	if (!projectname.empty())
	{
		strcat(newCaption, " - ");
		strcat(newCaption, projectname.c_str());
	}
	// and * if project has unsaved changes
	if (project.getProjectChanged())
	{
		strcat(newCaption, "*");
	}
	setWindowTitle( tr(newCaption) );
	//SetWindowText(hwndTASEditor, newCaption);
}
//----------------------------------------------------------------------------
void TasEditorWindow::clearProjectList(void)
{
	std::list <std::string*>::iterator it;

	for (it=projList.begin(); it != projList.end(); it++)
	{
		delete *it;
	}
	projList.clear();
}
//----------------------------------------------------------------------------
void TasEditorWindow::buildRecentProjectMenu(void)
{
	QAction *act;
	std::string s;
	std::string *sptr;
	char buf[128];

	clearProjectList();
	recentProjectMenu->clear();

	for (int i=0; i<10; i++)
	{
		sprintf(buf, "SDL.RecentTasProject%02i", i);

		g_config->getOption( buf, &s);

		//printf("Recent Rom:%i  '%s'\n", i, s.c_str() );

		if ( s.size() > 0 )
		{
			act = new TasRecentProjectAction( tr(s.c_str()), recentProjectMenu);

			recentProjectMenu->addAction( act );

			connect(act, SIGNAL(triggered()), act, SLOT(activateCB(void)) );

			sptr = new std::string();

			sptr->assign( s.c_str() );

			projList.push_front( sptr );
		}
	}
	recentProjectMenu->setEnabled( !recentProjectMenu->isEmpty() );
}
//---------------------------------------------------------------------------
void TasEditorWindow::saveRecentProjectMenu(void)
{
	int i;
	std::string *s;
	std::list <std::string*>::iterator it;
	char buf[128];

	i = projList.size() - 1;

	for (it=projList.begin(); it != projList.end(); it++)
	{
		s = *it;
		sprintf(buf, "SDL.RecentTasProject%02i", i);

		g_config->setOption( buf, s->c_str() );

		//printf("Recent Rom:%u  '%s'\n", i, s->c_str() );
		i--;
	}
}
//---------------------------------------------------------------------------
void TasEditorWindow::addRecentProject( const char *proj )
{
	std::string *s;
	std::list <std::string*>::iterator match_it;

	for (match_it=projList.begin(); match_it != projList.end(); match_it++)
	{
		s = *match_it;

		if ( s->compare( proj ) == 0 )
		{
			//printf("Found Match: %s\n", proj );
			break;
		}
	}

	if ( match_it != projList.end() )
	{
		s = *match_it;

		projList.erase(match_it);

		projList.push_back(s);
	}
	else
	{
		s = new std::string();

		s->assign( proj );
		
		projList.push_back(s);

		if ( projList.size() > 10 )
		{
			s = projList.front();

			projList.pop_front();

			delete s;
		}
	}

	saveRecentProjectMenu();

	recentProjectMenuReset = true;
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
bool TasEditorWindow::saveCompactGetFilename( QString &outputFilePath )
{
	std::string last;
	int ret, useNativeFileDialogVal;
	QString filename;
	std::string lastPath;
	//char dir[512];
	const char *base, *rom;
	QFileDialog  dialog(this, tr("Save Compact TAS Editor Project As") );
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
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DesktopLocation).first());
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

	if (!project.getProjectName().empty())
	{
		char baseName[512];

		strcpy(baseName, project.getProjectName().c_str());

		if (strstr(baseName, "-compact") == NULL)
		{
			strcat(baseName, "-compact");
		}
		strcat( baseName, ".fm3");

		dialog.selectFile(baseName);
	}
	else if ( rom )
	{
		char baseName[512];
		getFileBaseName( rom, baseName );

		if ( baseName[0] != 0 )
		{
			if (strstr(baseName, "-compact") == NULL)
			{
				strcat(baseName, "-compact");
			}
			strcat( baseName, ".fm3");

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
	QFileInfo fi( filename );

	if ( fi.exists() )
	{
		int ret;
		std::string msg;

		msg = "Pre-existing TAS project file will be overwritten:\n\n" +
			fi.fileName().toStdString() + "\n\nReplace file?";

		ret = QMessageBox::warning( this, QObject::tr("Overwrite Warning"),
				QString::fromStdString(msg), QMessageBox::Yes | QMessageBox::No, QMessageBox::No );

		if ( ret == QMessageBox::No )
		{
			return false;
		}
	}
	outputFilePath = filename;

	//qDebug() << "selected file path : " << filename.toUtf8();

	return true;
}
//----------------------------------------------------------------------------
void TasEditorWindow::saveProjectCompactCb(void)
{
	int ret;
	QDialog dialog(this);
	FCEU_CRITICAL_SECTION(emuLock);
	QGroupBox *fileContentsBox, *greenZoneSaveBox;
	QVBoxLayout *mainLayout, *vbox1, *vbox;
	QHBoxLayout *hbox;
	QCheckBox *binaryInput, *saveMarkers, *saveBookmarks;
	QCheckBox *saveHistory, *savePianoRoll, *saveSelection;
	QRadioButton *allFrames, *every16thFrame, *markedFrames, *dontSave;
	QPushButton  *okButton, *cancelButton;

	dialog.setWindowTitle( tr("Save Compact") );

	mainLayout       = new QVBoxLayout();
	fileContentsBox  = new QGroupBox( tr("File Contents") );
	greenZoneSaveBox = new QGroupBox( tr("Greenzone Saving Options") );

	binaryInput    = new QCheckBox( tr("Binary Input") );
	saveMarkers    = new QCheckBox( tr("Markers") );
	saveBookmarks  = new QCheckBox( tr("Bookmarks") );
	saveHistory    = new QCheckBox( tr("History") );
	savePianoRoll  = new QCheckBox( tr("Piano Roll") );
	saveSelection  = new QCheckBox( tr("Selection") );

	allFrames      = new QRadioButton( tr("All Frames") );
	every16thFrame = new QRadioButton( tr("Every 16th Frame") );
	markedFrames   = new QRadioButton( tr("Marked Frame") );
	dontSave       = new QRadioButton( tr("Don't Save") );

	okButton       = new QPushButton( tr("Ok") );
	cancelButton   = new QPushButton( tr("Cancel") );

	okButton->setIcon( style()->standardIcon( QStyle::SP_DialogApplyButton ) );
	cancelButton->setIcon( style()->standardIcon( QStyle::SP_DialogCancelButton ) );

	connect(     okButton, SIGNAL(clicked(void)), &dialog, SLOT(accept(void)) );
	connect( cancelButton, SIGNAL(clicked(void)), &dialog, SLOT(reject(void)) );

	vbox1 = new QVBoxLayout();

	dialog.setLayout( mainLayout );
	mainLayout->addWidget( fileContentsBox );

	fileContentsBox->setLayout( vbox1 );

	vbox1->addWidget( binaryInput    );
	vbox1->addWidget( saveMarkers    );
	vbox1->addWidget( saveBookmarks  );
	vbox1->addWidget( saveHistory    );
	vbox1->addWidget( savePianoRoll  );
	vbox1->addWidget( saveSelection  );
	vbox1->addWidget( greenZoneSaveBox );

	vbox  = new QVBoxLayout();
	greenZoneSaveBox->setLayout( vbox );

	vbox->addWidget( allFrames      );
	vbox->addWidget( every16thFrame );
	vbox->addWidget( markedFrames   );
	vbox->addWidget( dontSave       );

	hbox = new QHBoxLayout();
	mainLayout->addLayout( hbox );
	hbox->addStretch(5);
	hbox->addWidget( okButton );
	hbox->addWidget( cancelButton );

	binaryInput->setChecked( taseditorConfig.saveCompact_SaveInBinary );
	saveMarkers->setChecked( taseditorConfig.saveCompact_SaveMarkers );
	saveBookmarks->setChecked( taseditorConfig.saveCompact_SaveBookmarks );
	saveHistory->setChecked( taseditorConfig.saveCompact_SaveHistory );
	savePianoRoll->setChecked( taseditorConfig.saveCompact_SavePianoRoll );
	saveSelection->setChecked( taseditorConfig.saveCompact_SaveSelection );

	     allFrames->setChecked( taseditorConfig.saveCompact_GreenzoneSavingMode == GREENZONE_SAVING_MODE_ALL );
	every16thFrame->setChecked( taseditorConfig.saveCompact_GreenzoneSavingMode == GREENZONE_SAVING_MODE_16TH );
	  markedFrames->setChecked( taseditorConfig.saveCompact_GreenzoneSavingMode == GREENZONE_SAVING_MODE_MARKED );
	      dontSave->setChecked( taseditorConfig.saveCompact_GreenzoneSavingMode == GREENZONE_SAVING_MODE_NO );

	okButton->setDefault(true);

	ret = dialog.exec();

	if ( ret == QDialog::Accepted )
	{
		QString filename;

		taseditorConfig.saveCompact_SaveInBinary  = binaryInput->isChecked();
		taseditorConfig.saveCompact_SaveMarkers   = saveMarkers->isChecked();
		taseditorConfig.saveCompact_SaveBookmarks = saveBookmarks->isChecked();
		taseditorConfig.saveCompact_SaveHistory   = saveHistory->isChecked();
		taseditorConfig.saveCompact_SavePianoRoll = savePianoRoll->isChecked();
		taseditorConfig.saveCompact_SaveSelection = saveSelection->isChecked();

		if ( allFrames->isChecked() )
		{
			taseditorConfig.saveCompact_GreenzoneSavingMode = GREENZONE_SAVING_MODE_ALL;
		}
		else if ( every16thFrame->isChecked() )
		{
			taseditorConfig.saveCompact_GreenzoneSavingMode = GREENZONE_SAVING_MODE_16TH;
		}
		else if ( markedFrames->isChecked() )
		{
			taseditorConfig.saveCompact_GreenzoneSavingMode = GREENZONE_SAVING_MODE_MARKED;
		}
		else
		{
			taseditorConfig.saveCompact_GreenzoneSavingMode = GREENZONE_SAVING_MODE_NO;
		}

		if ( saveCompactGetFilename( filename ) )
		{
			project.save(filename.toStdString().c_str(), taseditorConfig.saveCompact_SaveInBinary, taseditorConfig.saveCompact_SaveMarkers, taseditorConfig.saveCompact_SaveBookmarks, taseditorConfig.saveCompact_GreenzoneSavingMode, taseditorConfig.saveCompact_SaveHistory, taseditorConfig.saveCompact_SavePianoRoll, taseditorConfig.saveCompact_SaveSelection);
		}
	}
}
//----------------------------------------------------------------------------
void TasEditorWindow::openOnlineDocs(void)
{
	if ( QDesktopServices::openUrl( QUrl("https://fceux.com/web/help/taseditor/Title.html") ) == false )
	{
		QMessageBox::critical( this, tr("Error"), 
		                        tr("Error: Failed to open link to: https://fceux.com/web/help/taseditor/Title.html") );
	}
	return;
}
//----------------------------------------------------------------------------
void TasEditorWindow::setCurrentPattern(int idx)
{
	if ( idx < 0 )
	{
		return;
	}
	if ( (size_t)idx >= patternsNames.size() )
	{
		return;
	}
	//printf("Set Pattern: %i\n", idx);
	taseditorConfig.currentPattern = idx;
}
//----------------------------------------------------------------------------
void TasEditorWindow::recordingChanged(int newState)
{
	FCEU_CRITICAL_SECTION( emuLock );
	int oldState = !movie_readonly ? Qt::Checked : Qt::Unchecked;

	if ( newState != oldState )
	{
		FCEUI_MovieToggleReadOnly();
	}
}
//----------------------------------------------------------------------------
void TasEditorWindow::editUndoCB(void)
{
	FCEU_CRITICAL_SECTION( emuLock );

	history.undo();
}
//----------------------------------------------------------------------------
void TasEditorWindow::editRedoCB(void)
{
	FCEU_CRITICAL_SECTION( emuLock );

	history.redo();
}
//----------------------------------------------------------------------------
void TasEditorWindow::editUndoSelCB(void)
{
	FCEU_CRITICAL_SECTION( emuLock );

	int dragMode = pianoRoll->getDragMode();

	if ( (dragMode != DRAG_MODE_SELECTION) && (dragMode != DRAG_MODE_DESELECTION) )
	{
		selection.undo();
		pianoRoll->followSelection();
	}
}
//----------------------------------------------------------------------------
void TasEditorWindow::editRedoSelCB(void)
{
	FCEU_CRITICAL_SECTION( emuLock );

	int dragMode = pianoRoll->getDragMode();

	if ( (dragMode != DRAG_MODE_SELECTION) && (dragMode != DRAG_MODE_DESELECTION) )
	{
		selection.redo();
		pianoRoll->followSelection();
	}
}
//----------------------------------------------------------------------------
void TasEditorWindow::editDeselectAll(void)
{
	FCEU_CRITICAL_SECTION( emuLock );

	int dragMode = pianoRoll->getDragMode();

	if ( (dragMode != DRAG_MODE_SELECTION) && (dragMode != DRAG_MODE_DESELECTION) )
	{
		selection.clearAllRowsSelection();
	}
}
//----------------------------------------------------------------------------
void TasEditorWindow::editSelectAll(void)
{
	FCEU_CRITICAL_SECTION( emuLock );

	int dragMode = pianoRoll->getDragMode();

	if ( (dragMode != DRAG_MODE_SELECTION) && (dragMode != DRAG_MODE_DESELECTION) )
	{
		selection.selectAllRows();
	}
}
//----------------------------------------------------------------------------
void TasEditorWindow::editSelBtwMkrs(void)
{
	FCEU_CRITICAL_SECTION( emuLock );

	int dragMode = pianoRoll->getDragMode();

	if ( (dragMode != DRAG_MODE_SELECTION) && (dragMode != DRAG_MODE_DESELECTION) )
	{
		selection.selectAllRowsBetweenMarkers();
	}
}
//----------------------------------------------------------------------------
void TasEditorWindow::editReselectClipboard(void)
{
	FCEU_CRITICAL_SECTION( emuLock );

	int dragMode = pianoRoll->getDragMode();

	if ( (dragMode != DRAG_MODE_SELECTION) && (dragMode != DRAG_MODE_DESELECTION) )
	{
		selection.reselectClipboard();
		pianoRoll->followSelection();
	}
}
//----------------------------------------------------------------------------
void TasEditorWindow::editCutCB(void)
{
	FCEU_CRITICAL_SECTION( emuLock );

	splicer.cutSelectedInputToClipboard();
}
//----------------------------------------------------------------------------
void TasEditorWindow::editCopyCB(void)
{
	FCEU_CRITICAL_SECTION( emuLock );

	splicer.copySelectedInputToClipboard();
}
//----------------------------------------------------------------------------
void TasEditorWindow::editPasteCB(void)
{
	FCEU_CRITICAL_SECTION( emuLock );

	splicer.pasteInputFromClipboard();
}
//----------------------------------------------------------------------------
void TasEditorWindow::editPasteInsertCB(void)
{
	FCEU_CRITICAL_SECTION( emuLock );

	splicer.pasteInsertInputFromClipboard();
}
//----------------------------------------------------------------------------
void TasEditorWindow::editClearCB(void)
{
	FCEU_CRITICAL_SECTION( emuLock );

	splicer.clearSelectedFrames();
}
//----------------------------------------------------------------------------
void TasEditorWindow::editDeleteCB(void)
{
	FCEU_CRITICAL_SECTION( emuLock );

	splicer.deleteSelectedFrames();
}
//----------------------------------------------------------------------------
void TasEditorWindow::editCloneCB(void)
{
	FCEU_CRITICAL_SECTION( emuLock );

	splicer.cloneSelectedFrames();
}
//----------------------------------------------------------------------------
void TasEditorWindow::editInsertCB(void)
{
	FCEU_CRITICAL_SECTION( emuLock );

	splicer.insertSelectedFrames();
}
//----------------------------------------------------------------------------
void TasEditorWindow::editInsertNumFramesCB(void)
{
	FCEU_CRITICAL_SECTION( emuLock );

	splicer.insertNumberOfFrames();
}
//----------------------------------------------------------------------------
void TasEditorWindow::editTruncateMovieCB(void)
{
	FCEU_CRITICAL_SECTION( emuLock );

	splicer.truncateMovie();
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
void TasEditorWindow::openFindNoteWindow(void)
{
	if ( findWin )
	{
		findWin->activateWindow();
		findWin->raise();
		findWin->setFocus();
	}
	else
	{
		findWin = new TasFindNoteWindow(this);
		findWin->show();
	}
}
//----------------------------------------------------------------------------
void TasEditorWindow::dpyBrnchScrnChanged(bool val)
{
	taseditorConfig.displayBranchScreenshots = val;
}
//----------------------------------------------------------------------------
void TasEditorWindow::dpyBrnchDescChanged(bool val)
{
	taseditorConfig.displayBranchDescriptions = val;
}
//----------------------------------------------------------------------------
void TasEditorWindow::enaHotChgChanged(bool val)
{
	taseditorConfig.enableHotChanges = val;
}
//----------------------------------------------------------------------------
void TasEditorWindow::followUndoActChanged(bool val)
{
	taseditorConfig.followUndoContext = val;
}
//----------------------------------------------------------------------------
void TasEditorWindow::followMkrActChanged(bool val)
{
	taseditorConfig.followMarkerNoteContext = val;
}
//----------------------------------------------------------------------------
void TasEditorWindow::enaGrnznActChanged(bool val)
{
	taseditorConfig.enableGreenzoning = val;
}
//----------------------------------------------------------------------------
void TasEditorWindow::afPtrnSkipLagActChanged(bool val)
{
	taseditorConfig.autofirePatternSkipsLag = val;
}
//----------------------------------------------------------------------------
void TasEditorWindow::adjInputLagActChanged(bool val)
{
	taseditorConfig.autoAdjustInputAccordingToLag = val;
}
//----------------------------------------------------------------------------
void TasEditorWindow::drawInputDragActChanged(bool val)
{
	taseditorConfig.drawInputByDragging = val;
}
//----------------------------------------------------------------------------
void TasEditorWindow::cmbRecDrawActChanged(bool val)
{
	taseditorConfig.combineConsecutiveRecordingsAndDraws = val;
}
//----------------------------------------------------------------------------
void TasEditorWindow::use1PforRecActChanged(bool val)
{
	taseditorConfig.use1PKeysForAllSingleRecordings = val;
}
//----------------------------------------------------------------------------
void TasEditorWindow::useInputColSetActChanged(bool val)
{
	taseditorConfig.useInputKeysForColumnSet = val;
}
//----------------------------------------------------------------------------
void TasEditorWindow::bindMkrInputActChanged(bool val)
{
	taseditorConfig.bindMarkersToInput = val;
}
//----------------------------------------------------------------------------
void TasEditorWindow::emptyNewMkrNotesActChanged(bool val)
{
	taseditorConfig.emptyNewMarkerNotes = val;
}
//----------------------------------------------------------------------------
void TasEditorWindow::oldCtlBrnhSchemeActChanged(bool val)
{
	taseditorConfig.oldControlSchemeForBranching = val;
}
//----------------------------------------------------------------------------
void TasEditorWindow::brnchRestoreMovieActChanged(bool val)
{
	taseditorConfig.branchesRestoreEntireMovie = val;
}
//----------------------------------------------------------------------------
void TasEditorWindow::hudInScrnBranchActChanged(bool val)
{
	taseditorConfig.HUDInBranchScreenshots = val;
}
//----------------------------------------------------------------------------
void TasEditorWindow::pauseAtEndActChanged(bool val)
{
	taseditorConfig.autopauseAtTheEndOfMovie = val;
}
//----------------------------------------------------------------------------
void TasEditorWindow::manLuaRun(void)
{
	mustCallManualLuaFunction = true;
}
//----------------------------------------------------------------------------
void TasEditorWindow::autoLuaRunChanged(bool val)
{
	taseditorConfig.enableLuaAutoFunction = val;
}
//----------------------------------------------------------------------------
void TasEditorWindow::showToolTipsActChanged(bool val)
{
	taseditorConfig.tooltipsEnabled = val;

	updateToolTips();
}
//----------------------------------------------------------------------------
void TasEditorWindow::updateToolTips(void)
{
	if ( taseditorConfig.tooltipsEnabled )
	{
		upperMarkerLabel->setToolTip( tr("Click here to scroll Piano Roll to Playback cursor") );
		lowerMarkerLabel->setToolTip( tr("Click here to scroll Piano Roll to Selection") );
		upperMarkerNote->setToolTip( tr("Click to edit text") );
		lowerMarkerNote->setToolTip( tr("Click to edit text") );

		recRecordingCbox->setToolTip( tr("Switch Input Recording on/off") );
		recSuperImposeCbox->setToolTip( tr("Allows to superimpose old Input with new buttons, instead of overwriting") );
		recUsePatternCbox->setToolTip( tr("Applies current Autofire Pattern to Input recording") );
		recAllBtn->setToolTip( tr("Switch off Multitracking") );
		rec1PBtn->setToolTip( tr("Select Joypad 1 as Current") );
		rec2PBtn->setToolTip( tr("Select Joypad 2 as Current") );
		rec3PBtn->setToolTip( tr("Select Joypad 3 as Current") );
		rec4PBtn->setToolTip( tr("Select Joypad 4 as Current") );

		rewindMkrBtn->setToolTip( tr("Send Playback to previous Marker (mouse: Shift+Wheel up) (hotkey: Shift+PageUp)") );
		rewindFrmBtn->setToolTip( tr("Rewind 1 frame (mouse: Right button+Wheel up) (hotkey: Shift+Up)") );
		playPauseBtn->setToolTip( tr("Pause/Unpause Emulation (mouse: Middle button)") );
		   advFrmBtn->setToolTip( tr("Advance 1 frame (mouse: Right button+Wheel down) (hotkey: Shift+Down)") );
		   advMkrBtn->setToolTip( tr("Send Playback to next Marker (mouse: Shift+Wheel down) (hotkey: Shift+PageDown)") );

		followCursorCbox->setToolTip( tr("The Piano Roll will follow Playback cursor movements") );
		   turboSeekCbox->setToolTip( tr("Uncheck when you need to watch seeking in slow motion") );
		 autoRestoreCbox->setToolTip( tr("Whenever you change Input above Playback cursor, the cursor returns to where it was before the change") );

		 selectionLbl->setToolTip( tr("Current size of Selection") );
		 clipboardLbl->setToolTip( tr("Current size of Input in the Clipboard") );

		 prevMkrBtn->setToolTip( tr("Send Selection to previous Marker (mouse: Ctrl+Wheel up) (hotkey: Ctrl+PageUp)") );
		 nextMkrBtn->setToolTip( tr("Send Selection to next Marker (mouse: Ctrl+Wheel up) (hotkey: Ctrl+PageDown)") );
		 similarBtn->setToolTip( tr("Auto-search for Marker Note") );
		    moreBtn->setToolTip( tr("Continue Auto-search") );
	}
	else
	{
		upperMarkerLabel->setToolTip( tr("") );
		lowerMarkerLabel->setToolTip( tr("") );
		upperMarkerNote->setToolTip( tr("") );
		lowerMarkerNote->setToolTip( tr("") );

		recRecordingCbox->setToolTip( tr("") );
		recSuperImposeCbox->setToolTip( tr("") );
		recUsePatternCbox->setToolTip( tr("") );
		recAllBtn->setToolTip( tr("") );
		rec1PBtn->setToolTip( tr("") );
		rec2PBtn->setToolTip( tr("") );
		rec3PBtn->setToolTip( tr("") );
		rec4PBtn->setToolTip( tr("") );

		rewindMkrBtn->setToolTip( tr("") );
		rewindFrmBtn->setToolTip( tr("") );
		playPauseBtn->setToolTip( tr("") );
		   advFrmBtn->setToolTip( tr("") );
		   advMkrBtn->setToolTip( tr("") );

		followCursorCbox->setToolTip( tr("") );
		   turboSeekCbox->setToolTip( tr("") );
		 autoRestoreCbox->setToolTip( tr("") );

		 selectionLbl->setToolTip( tr("") );
		 clipboardLbl->setToolTip( tr("") );

		 prevMkrBtn->setToolTip( tr("") );
		 nextMkrBtn->setToolTip( tr("") );
		 similarBtn->setToolTip( tr("") );
		    moreBtn->setToolTip( tr("") );
	}
}
//----------------------------------------------------------------------------
void TasEditorWindow::changePianoRollFontCB(void)
{
	bool ok = false;

	QFont selFont = QFontDialog::getFont( &ok, pianoRoll->QWidget::font(), this, tr("Select Font"), QFontDialog::MonospacedFonts );

	if ( ok )
	{
		pianoRoll->setFont( selFont );

		//printf("Font Changed to: '%s'\n", selFont.toString().toStdString().c_str() );

		g_config->setOption("SDL.TasPianoRollFont", selFont.toString().toStdString().c_str() );
	}
}
//----------------------------------------------------------------------------
void TasEditorWindow::changeBookmarksFontCB(void)
{
	bool ok = false;

	QFont selFont = QFontDialog::getFont( &ok, bookmarks.QWidget::font(), this, tr("Select Font"), QFontDialog::MonospacedFonts );

	if ( ok )
	{
		bookmarks.setFont( selFont );

		//printf("Font Changed to: '%s'\n", selFont.toString().toStdString().c_str() );

		g_config->setOption("SDL.TasBookmarksFont", selFont.toString().toStdString().c_str() );
	}
}
//----------------------------------------------------------------------------
void TasEditorWindow::changeBranchesFontCB(void)
{
	bool ok = false;

	QFont selFont = QFontDialog::getFont( &ok, branches.QWidget::font(), this, tr("Select Font"), QFontDialog::MonospacedFonts );

	if ( ok )
	{
		branches.setFont( selFont );

		//printf("Font Changed to: '%s'\n", selFont.toString().toStdString().c_str() );

		g_config->setOption("SDL.TasBranchesFont", selFont.toString().toStdString().c_str() );
	}
}
//----------------------------------------------------------------------------
void TasEditorWindow::playbackPauseCB(void)
{
	FCEU_CRITICAL_SECTION( emuLock );
	playback.toggleEmulationPause();
	pianoRoll->update();
}
//----------------------------------------------------------------------------
void TasEditorWindow::playbackFrameRewind(void)
{
	FCEU_CRITICAL_SECTION( emuLock );
	playback.handleRewindFrame();
	pianoRoll->update();
}
//----------------------------------------------------------------------------
void TasEditorWindow::playbackFrameForward(void)
{
	FCEU_CRITICAL_SECTION( emuLock );
	playback.handleForwardFrame();
	pianoRoll->update();
}
//----------------------------------------------------------------------------
void TasEditorWindow::playbackFrameRewindFull(void)
{
	FCEU_CRITICAL_SECTION( emuLock );
	playback.handleRewindFull();
	pianoRoll->update();
}
//----------------------------------------------------------------------------
void TasEditorWindow::playbackFrameForwardFull(void)
{
	FCEU_CRITICAL_SECTION( emuLock );
	playback.handleForwardFull();
	pianoRoll->update();
}
// ----------------------------------------------------------------------------------------------
void TasEditorWindow::playbackFollowCursorCb(bool val)
{
	taseditorConfig.followPlaybackCursor = val;

	if ( val )
	{
		pianoRoll->ensureTheLineIsVisible( currFrameCounter );
	}
}
// ----------------------------------------------------------------------------------------------
void TasEditorWindow::playbackTurboSeekCb(bool val)
{
	FCEU_CRITICAL_SECTION( emuLock );

	taseditorConfig.turboSeek = val;

	// if currently seeking, apply this option immediately
	if (playback.getPauseFrame() >= 0)
	{
		turbo = taseditorConfig.turboSeek;
	}
}
// ----------------------------------------------------------------------------------------------
void TasEditorWindow::playbackAutoRestoreCb(bool val)
{
	taseditorConfig.autoRestoreLastPlaybackPosition = val;
}
// ----------------------------------------------------------------------------------------------
void TasEditorWindow::scrollSelectionUpOne(void)
{
	FCEU_CRITICAL_SECTION( emuLock );
	int dragMode = pianoRoll->getDragMode();

	//printf("DragMode: %i\n", dragMode);

	if ( (dragMode != DRAG_MODE_SELECTION) && (dragMode != DRAG_MODE_DESELECTION) )
	{
		selection.transposeVertically(-1);
		int selectionBeginning = selection.getCurrentRowsSelectionBeginning();
		if (selectionBeginning >= 0)
		{
			pianoRoll->ensureTheLineIsVisible(selectionBeginning);
		}
		pianoRoll->update();
	}
}
// ----------------------------------------------------------------------------------------------
void TasEditorWindow::scrollSelectionDnOne(void)
{
	FCEU_CRITICAL_SECTION( emuLock );
	int dragMode = pianoRoll->getDragMode();

	//printf("DragMode: %i\n", dragMode);

	if ( (dragMode != DRAG_MODE_SELECTION) && (dragMode != DRAG_MODE_DESELECTION) )
	{
		selection.transposeVertically(1);
		int selectionEnd = selection.getCurrentRowsSelectionEnd();
		if (selectionEnd >= 0)
		{
			pianoRoll->ensureTheLineIsVisible(selectionEnd);
		}
		pianoRoll->update();
	}
}
// ----------------------------------------------------------------------------------------------
void TasEditorWindow::histTreeItemActivated(QTreeWidgetItem *item, int col)
{
	int row = histTree->indexOfTopLevelItem(item);

	if ( row < 0 )
	{
		return;
	}
	FCEU_CRITICAL_SECTION( emuLock );
	history.handleSingleClick(row);
}
// ----------------------------------------------------------------------------------------------
void TasEditorWindow::tabViewChanged(int idx)
{
	FCEU_CRITICAL_SECTION( emuLock );
	taseditorConfig.displayBranchesTree = (idx == 1);
	bookmarks.redrawBookmarksSectionCaption();
}
// ----------------------------------------------------------------------------------------------
void TasEditorWindow::openProjectSaveOptions(void)
{
	int ret;
	QDialog dialog(this);
	FCEU_CRITICAL_SECTION( emuLock );
	QGroupBox *settingsBox, *fileContentsBox, *greenZoneSaveBox;
	QVBoxLayout *mainLayout, *vbox1, *vbox;
	QHBoxLayout *hbox1, *hbox;
	QCheckBox *autoSaveOpt, *saveSilentOpt;
	QSpinBox  *autoSavePeriod;
	QCheckBox *binaryInput, *saveMarkers, *saveBookmarks;
	QCheckBox *saveHistory, *savePianoRoll, *saveSelection;
	QRadioButton *allFrames, *every16thFrame, *markedFrames, *dontSave;
	QPushButton  *okButton, *cancelButton;

	dialog.setWindowTitle( tr("Project File Saving Options") );

	mainLayout       = new QVBoxLayout();
	settingsBox      = new QGroupBox( tr("Settings") );
	fileContentsBox  = new QGroupBox( tr("File Contents") );
	greenZoneSaveBox = new QGroupBox( tr("Greenzone Saving Options") );
	hbox1            = new QHBoxLayout();

	autoSaveOpt    = new QCheckBox( tr("Autosave project") );
	saveSilentOpt  = new QCheckBox( tr("silently") );
	autoSavePeriod = new QSpinBox();

	binaryInput    = new QCheckBox( tr("Binary Input") );
	saveMarkers    = new QCheckBox( tr("Markers") );
	saveBookmarks  = new QCheckBox( tr("Bookmarks") );
	saveHistory    = new QCheckBox( tr("History") );
	savePianoRoll  = new QCheckBox( tr("Piano Roll") );
	saveSelection  = new QCheckBox( tr("Selection") );

	allFrames      = new QRadioButton( tr("All Frames") );
	every16thFrame = new QRadioButton( tr("Every 16th Frame") );
	markedFrames   = new QRadioButton( tr("Marked Frame") );
	dontSave       = new QRadioButton( tr("Don't Save") );

	okButton       = new QPushButton( tr("Ok") );
	cancelButton   = new QPushButton( tr("Cancel") );

	okButton->setIcon( style()->standardIcon( QStyle::SP_DialogApplyButton ) );
	cancelButton->setIcon( style()->standardIcon( QStyle::SP_DialogCancelButton ) );

	connect(     okButton, SIGNAL(clicked(void)), &dialog, SLOT(accept(void)) );
	connect( cancelButton, SIGNAL(clicked(void)), &dialog, SLOT(reject(void)) );

	hbox1->addWidget( settingsBox );
	hbox1->addWidget( fileContentsBox );

	dialog.setLayout( mainLayout );
	mainLayout->addLayout( hbox1 );

	vbox = new QVBoxLayout();
	hbox = new QHBoxLayout();
	settingsBox->setLayout( vbox );

	hbox->addWidget( new QLabel( tr("every") ) );
	hbox->addWidget( autoSavePeriod );
	hbox->addWidget( new QLabel( tr("minutes") ) );

	vbox->addWidget( autoSaveOpt );
	vbox->addLayout( hbox );
	vbox->addWidget( saveSilentOpt );
	vbox->addStretch( 10 );

	vbox1 = new QVBoxLayout();
	fileContentsBox->setLayout( vbox1 );

	vbox1->addWidget( binaryInput    );
	vbox1->addWidget( saveMarkers    );
	vbox1->addWidget( saveBookmarks  );
	vbox1->addWidget( saveHistory    );
	vbox1->addWidget( savePianoRoll  );
	vbox1->addWidget( saveSelection  );
	vbox1->addWidget( greenZoneSaveBox );

	vbox  = new QVBoxLayout();
	greenZoneSaveBox->setLayout( vbox );

	vbox->addWidget( allFrames      );
	vbox->addWidget( every16thFrame );
	vbox->addWidget( markedFrames   );
	vbox->addWidget( dontSave       );

	hbox1 = new QHBoxLayout();
	mainLayout->addLayout( hbox1 );
	hbox1->addStretch(5);
	hbox1->addWidget( okButton );
	hbox1->addWidget( cancelButton );

	autoSavePeriod->setRange( AUTOSAVE_PERIOD_MIN, AUTOSAVE_PERIOD_MAX );

	autoSaveOpt->setChecked( taseditorConfig.autosaveEnabled );
	autoSavePeriod->setValue( taseditorConfig.autosavePeriod );
	saveSilentOpt->setChecked( taseditorConfig.autosaveSilent );

	autoSavePeriod->setEnabled( taseditorConfig.autosaveEnabled );
	saveSilentOpt->setEnabled( taseditorConfig.autosaveEnabled );

	binaryInput->setChecked( taseditorConfig.projectSavingOptions_SaveInBinary );
	saveMarkers->setChecked( taseditorConfig.projectSavingOptions_SaveMarkers );
	saveBookmarks->setChecked( taseditorConfig.projectSavingOptions_SaveBookmarks );
	saveHistory->setChecked( taseditorConfig.projectSavingOptions_SaveHistory );
	savePianoRoll->setChecked( taseditorConfig.projectSavingOptions_SavePianoRoll );
	saveSelection->setChecked( taseditorConfig.projectSavingOptions_SaveSelection );

	     allFrames->setChecked( taseditorConfig.projectSavingOptions_GreenzoneSavingMode == GREENZONE_SAVING_MODE_ALL );
	every16thFrame->setChecked( taseditorConfig.projectSavingOptions_GreenzoneSavingMode == GREENZONE_SAVING_MODE_16TH );
	  markedFrames->setChecked( taseditorConfig.projectSavingOptions_GreenzoneSavingMode == GREENZONE_SAVING_MODE_MARKED );
	      dontSave->setChecked( taseditorConfig.projectSavingOptions_GreenzoneSavingMode == GREENZONE_SAVING_MODE_NO );

	connect( autoSaveOpt, SIGNAL(clicked(bool)), autoSavePeriod, SLOT(setEnabled(bool)) );
	connect( autoSaveOpt, SIGNAL(clicked(bool)), saveSilentOpt , SLOT(setEnabled(bool)) );

	okButton->setDefault(true);

	ret = dialog.exec();

	if ( ret == QDialog::Accepted )
	{
		taseditorConfig.autosaveEnabled = autoSaveOpt->isChecked();
		taseditorConfig.autosavePeriod  = autoSavePeriod->value();
		taseditorConfig.autosaveSilent  = saveSilentOpt->isChecked();

		taseditorConfig.projectSavingOptions_SaveInBinary  = binaryInput->isChecked();
		taseditorConfig.projectSavingOptions_SaveMarkers   = saveMarkers->isChecked();
		taseditorConfig.projectSavingOptions_SaveBookmarks = saveBookmarks->isChecked();
		taseditorConfig.projectSavingOptions_SaveHistory   = saveHistory->isChecked();
		taseditorConfig.projectSavingOptions_SavePianoRoll = savePianoRoll->isChecked();
		taseditorConfig.projectSavingOptions_SaveSelection = saveSelection->isChecked();

		if ( allFrames->isChecked() )
		{
			taseditorConfig.projectSavingOptions_GreenzoneSavingMode = GREENZONE_SAVING_MODE_ALL;
		}
		else if ( every16thFrame->isChecked() )
		{
			taseditorConfig.projectSavingOptions_GreenzoneSavingMode = GREENZONE_SAVING_MODE_16TH;
		}
		else if ( markedFrames->isChecked() )
		{
			taseditorConfig.projectSavingOptions_GreenzoneSavingMode = GREENZONE_SAVING_MODE_MARKED;
		}
		else
		{
			taseditorConfig.projectSavingOptions_GreenzoneSavingMode = GREENZONE_SAVING_MODE_NO;
		}
	}
}
// ----------------------------------------------------------------------------------------------
void TasEditorWindow::setGreenzoneCapacity(void)
{
	int ret;
	int newValue = taseditorConfig.greenzoneCapacity;
	QInputDialog dialog(this);
	FCEU_CRITICAL_SECTION( emuLock );

	dialog.setWindowTitle( tr("Greenzone Capacity") );
	dialog.setInputMode( QInputDialog::IntInput );
	dialog.setIntRange( GREENZONE_CAPACITY_MIN, GREENZONE_CAPACITY_MAX );
	dialog.setLabelText( tr("Keep savestates for how many frames?\n(actual limit of savestates can be 5 times more than the number provided)") );
	dialog.setIntValue( newValue );

	ret = dialog.exec();

	if ( ret == QDialog::Accepted )
	{
		newValue = dialog.intValue();

		if (newValue < GREENZONE_CAPACITY_MIN)
		{
			newValue = GREENZONE_CAPACITY_MIN;
		}
		else if (newValue > GREENZONE_CAPACITY_MAX)
		{
			newValue = GREENZONE_CAPACITY_MAX;
		}
		if (newValue < taseditorConfig.greenzoneCapacity)
		{
			taseditorConfig.greenzoneCapacity = newValue;
			greenzone.runGreenzoneCleaning();
		}
		else
		{
			taseditorConfig.greenzoneCapacity = newValue;
		}
	}
}
// ----------------------------------------------------------------------------------------------
void TasEditorWindow::setMaxUndoCapacity(void)
{
	int ret;
	int newValue = taseditorConfig.maxUndoLevels;
	QInputDialog dialog(this);
	FCEU_CRITICAL_SECTION( emuLock );

	dialog.setWindowTitle( tr("Max undo levels") );
	dialog.setInputMode( QInputDialog::IntInput );
	dialog.setIntRange( UNDO_LEVELS_MIN, UNDO_LEVELS_MAX );
	dialog.setLabelText( tr("Keep history of how many changes?") );
	dialog.setIntValue( newValue );

	ret = dialog.exec();

	if ( ret == QDialog::Accepted )
	{
		newValue = dialog.intValue();

		if (newValue < UNDO_LEVELS_MIN)
		{
			newValue = UNDO_LEVELS_MIN;
		}
		else if (newValue > UNDO_LEVELS_MAX)
		{
			newValue = UNDO_LEVELS_MAX;
		}
		if (newValue != taseditorConfig.maxUndoLevels)
		{
			taseditorConfig.maxUndoLevels = newValue;
			history.updateHistoryLogSize();
			selection.updateHistoryLogSize();
		}
	}
}
// ----------------------------------------------------------------------------------------------
void TasEditorWindow::loadClipboard(const char *txt)
{
	clipboard->setText( tr(txt), QClipboard::Clipboard );

	if ( clipboard->supportsSelection() )
	{
		clipboard->setText( tr(txt), QClipboard::Selection );
	}
}
// ----------------------------------------------------------------------------------------------
// following functions use function parameters to determine range of frames
void TasEditorWindow::toggleInput(int start, int end, int joy, int button, int consecutivenessTag)
{
	if (joy < 0 || joy >= joysticksPerFrame[getInputType(currMovieData)]) return;

	int check_frame = end;
	if (start > end)
	{
		// swap
		int temp_start = start;
		start = end;
		end = temp_start;
	}
	if (start < 0) start = end;
	if (end >= currMovieData.getNumRecords())
		return;

	if (currMovieData.records[check_frame].checkBit(joy, button))
	{
		// clear range
		for (int i = start; i <= end; ++i)
			currMovieData.records[i].clearBit(joy, button);
		greenzone.invalidateAndUpdatePlayback(history.registerChanges(MODTYPE_UNSET, start, end, 0, NULL, consecutivenessTag));
	} else
	{
		// set range
		for (int i = start; i <= end; ++i)
			currMovieData.records[i].setBit(joy, button);
		greenzone.invalidateAndUpdatePlayback(history.registerChanges(MODTYPE_SET, start, end, 0, NULL, consecutivenessTag));
	}
}
void TasEditorWindow::setInputUsingPattern(int start, int end, int joy, int button, int consecutivenessTag)
{
	if (joy < 0 || joy >= joysticksPerFrame[getInputType(currMovieData)]) return;

	if (start > end)
	{
		// swap
		int temp_start = start;
		start = end;
		end = temp_start;
	}
	if (start < 0) start = end;
	if (end >= currMovieData.getNumRecords())
	{
		return;
	}

	int pattern_offset = 0, current_pattern = taseditorConfig.currentPattern;
	bool changes_made = false;
	bool value;

	for (int i = start; i <= end; ++i)
	{
		// skip lag frames
		if (taseditorConfig.autofirePatternSkipsLag && greenzone.lagLog.getLagInfoAtFrame(i) == LAGGED_YES)
		{
			continue;
		}
		value = (patterns[current_pattern][pattern_offset] != 0);
		if (currMovieData.records[i].checkBit(joy, button) != value)
		{
			changes_made = true;
			currMovieData.records[i].setBitValue(joy, button, value);
		}
		pattern_offset++;
		if (pattern_offset >= (int)patterns[current_pattern].size())
		{
			pattern_offset -= patterns[current_pattern].size();
		}
	}
	if (changes_made)
	{
		greenzone.invalidateAndUpdatePlayback(history.registerChanges(MODTYPE_PATTERN, start, end, 0, patternsNames[current_pattern].c_str(), consecutivenessTag));
	}
}

// following functions use current Selection to determine range of frames
bool TasEditorWindow::handleColumnSet(void)
{
	RowsSelection* current_selection = selection.getCopyOfCurrentRowsSelection();
	if (current_selection->size() == 0) return false;
	RowsSelection::iterator current_selection_begin(current_selection->begin());
	RowsSelection::iterator current_selection_end(current_selection->end());

	// inspect the selected frames, if they are all set, then unset all, else set all
	bool unset_found = false, changes_made = false;
	for(RowsSelection::iterator it(current_selection_begin); it != current_selection_end; it++)
	{
		if (!markersManager.getMarkerAtFrame(*it))
		{
			unset_found = true;
			break;
		}
	}
	if (unset_found)
	{
		// set all
		for(RowsSelection::iterator it(current_selection_begin); it != current_selection_end; it++)
		{
			if (!markersManager.getMarkerAtFrame(*it))
			{
				if (markersManager.setMarkerAtFrame(*it))
				{
					changes_made = true;
					//pianoRoll.redrawRow(*it);
					//pianoRoll->update(); // Piano roll will update at next periodic cycle
					lowerMarkerNote->setFocus();
				}
			}
		}
		if (changes_made)
		{
			history.registerMarkersChange(MODTYPE_MARKER_SET, *current_selection_begin, *current_selection->rbegin());
		}
	}
	else
	{
		// unset all
		for(RowsSelection::iterator it(current_selection_begin); it != current_selection_end; it++)
		{
			if (markersManager.getMarkerAtFrame(*it))
			{
				markersManager.removeMarkerFromFrame(*it);
				changes_made = true;
				//pianoRoll.redrawRow(*it);
				//pianoRoll->update(); // Piano roll will update at next periodic cycle
			}
		}
		if (changes_made)
		{
			history.registerMarkersChange(MODTYPE_MARKER_REMOVE, *current_selection_begin, *current_selection->rbegin());
		}
	}
	if (changes_made)
	{
		selection.mustFindCurrentMarker = playback.mustFindCurrentMarker = true;
	}
	return changes_made;
}

bool TasEditorWindow::handleColumnSetUsingPattern(void)
{
	RowsSelection* current_selection = selection.getCopyOfCurrentRowsSelection();
	if (current_selection->size() == 0) return false;
	RowsSelection::iterator current_selection_begin(current_selection->begin());
	RowsSelection::iterator current_selection_end(current_selection->end());
	int pattern_offset = 0, current_pattern = taseditorConfig.currentPattern;
	bool changes_made = false;

	for(RowsSelection::iterator it(current_selection_begin); it != current_selection_end; it++)
	{
		// skip lag frames
		if (taseditorConfig.autofirePatternSkipsLag && greenzone.lagLog.getLagInfoAtFrame(*it) == LAGGED_YES)
			continue;
		if (patterns[current_pattern][pattern_offset])
		{
			if (!markersManager.getMarkerAtFrame(*it))
			{
				if (markersManager.setMarkerAtFrame(*it))
				{
					changes_made = true;
					pianoRoll->update();
				}
			}
		}
		else
		{
			if (markersManager.getMarkerAtFrame(*it))
			{
				markersManager.removeMarkerFromFrame(*it);
				changes_made = true;
				pianoRoll->update();
			}
		}
		pattern_offset++;
		if (pattern_offset >= (int)patterns[current_pattern].size())
			pattern_offset -= patterns[current_pattern].size();
	}
	if (changes_made)
	{
		history.registerMarkersChange(MODTYPE_MARKER_PATTERN, *current_selection_begin, *current_selection->rbegin(), patternsNames[current_pattern].c_str());
		selection.mustFindCurrentMarker = playback.mustFindCurrentMarker = true;
		return true;
	}
	return false;
}
bool TasEditorWindow::handleInputColumnSetUsingPattern(int joy, int button)
{
	if (joy < 0 || joy >= joysticksPerFrame[getInputType(currMovieData)]) return false;

	RowsSelection* current_selection = selection.getCopyOfCurrentRowsSelection();
	if (current_selection->size() == 0) return false;
	RowsSelection::iterator current_selection_begin(current_selection->begin());
	RowsSelection::iterator current_selection_end(current_selection->end());
	int pattern_offset = 0, current_pattern = taseditorConfig.currentPattern;

	for(RowsSelection::iterator it(current_selection_begin); it != current_selection_end; it++)
	{
		// skip lag frames
		if (taseditorConfig.autofirePatternSkipsLag && greenzone.lagLog.getLagInfoAtFrame(*it) == LAGGED_YES)
			continue;
		currMovieData.records[*it].setBitValue(joy, button, patterns[current_pattern][pattern_offset] != 0);
		pattern_offset++;
		if (pattern_offset >= (int)patterns[current_pattern].size())
			pattern_offset -= patterns[current_pattern].size();
	}
	int first_changes = history.registerChanges(MODTYPE_PATTERN, *current_selection_begin, *current_selection->rbegin(), 0, patternsNames[current_pattern].c_str());
	if (first_changes >= 0)
	{
		greenzone.invalidateAndUpdatePlayback(first_changes);
		return true;
	} else
		return false;
}

bool TasEditorWindow::handleInputColumnSet(int joy, int button)
{
	if (joy < 0 || joy >= joysticksPerFrame[getInputType(currMovieData)]) return false;

	RowsSelection* current_selection = selection.getCopyOfCurrentRowsSelection();
	if (current_selection->size() == 0) return false;
	RowsSelection::iterator current_selection_begin(current_selection->begin());
	RowsSelection::iterator current_selection_end(current_selection->end());

	//inspect the selected frames, if they are all set, then unset all, else set all
	bool newValue = false;
	for(RowsSelection::iterator it(current_selection_begin); it != current_selection_end; it++)
	{
		if (!(currMovieData.records[*it].checkBit(joy,button)))
		{
			newValue = true;
			break;
		}
	}
	// apply newValue
	for(RowsSelection::iterator it(current_selection_begin); it != current_selection_end; it++)
		currMovieData.records[*it].setBitValue(joy,button,newValue);

	int first_changes;
	if (newValue)
	{
		first_changes = history.registerChanges(MODTYPE_SET, *current_selection_begin, *current_selection->rbegin());
	} else
	{
		first_changes = history.registerChanges(MODTYPE_UNSET, *current_selection_begin, *current_selection->rbegin());
	}
	if (first_changes >= 0)
	{
		greenzone.invalidateAndUpdatePlayback(first_changes);
		return true;
	}
	return false;
}

void TasEditorWindow::setMarkers(void)
{
	FCEU_CRITICAL_SECTION( emuLock );

	RowsSelection* current_selection = selection.getCopyOfCurrentRowsSelection();
	if (current_selection->size())
	{
		RowsSelection::iterator current_selection_begin(current_selection->begin());
		RowsSelection::iterator current_selection_end(current_selection->end());
		bool changes_made = false;
		for(RowsSelection::iterator it(current_selection_begin); it != current_selection_end; it++)
		{
			if (!markersManager.getMarkerAtFrame(*it))
			{
				if (markersManager.setMarkerAtFrame(*it))
				{
					changes_made = true;
					//pianoRoll->update();
				}
			}
		}
		if (changes_made)
		{
			selection.mustFindCurrentMarker = playback.mustFindCurrentMarker = true;
			history.registerMarkersChange(MODTYPE_MARKER_SET, *current_selection_begin, *current_selection->rbegin());
		}
	}
}
void TasEditorWindow::removeMarkers(void)
{
	FCEU_CRITICAL_SECTION( emuLock );

	RowsSelection* current_selection = selection.getCopyOfCurrentRowsSelection();
	if (current_selection->size())
	{
		RowsSelection::iterator current_selection_begin(current_selection->begin());
		RowsSelection::iterator current_selection_end(current_selection->end());
		bool changes_made = false;
		for(RowsSelection::iterator it(current_selection_begin); it != current_selection_end; it++)
		{
			if (markersManager.getMarkerAtFrame(*it))
			{
				markersManager.removeMarkerFromFrame(*it);
				changes_made = true;
				//pianoRoll->update();
			}
		}
		if (changes_made)
		{
			selection.mustFindCurrentMarker = playback.mustFindCurrentMarker = true;
			history.registerMarkersChange(MODTYPE_MARKER_REMOVE, *current_selection_begin, *current_selection->rbegin());
		}
	}
}
//----------------------------------------------------------------------------
void TasEditorWindow::ungreenzoneSelectedFrames(void)
{
	FCEU_CRITICAL_SECTION( emuLock );

	greenzone.ungreenzoneSelectedFrames();
}
//----------------------------------------------------------------------------
void TasEditorWindow::upperMarkerLabelClicked(void)
{
	pianoRoll->followPlaybackCursor();
}
//----------------------------------------------------------------------------
void TasEditorWindow::lowerMarkerLabelClicked(void)
{
	int dragMode = pianoRoll->getDragMode();

	if (dragMode != DRAG_MODE_SELECTION && dragMode != DRAG_MODE_DESELECTION)
	{
		pianoRoll->followSelection();
	}
}
//----------------------------------------------------------------------------
void TasEditorWindow::jumpToPreviousMarker(void)
{
	selection.jumpToPreviousMarker();
}
//----------------------------------------------------------------------------
void TasEditorWindow::jumpToNextMarker(void)
{
	selection.jumpToNextMarker();
}
//----------------------------------------------------------------------------
void TasEditorWindow::findSimilarNote(void)
{
	markersManager.findSimilarNote();
}
//----------------------------------------------------------------------------
void TasEditorWindow::findNextSimilarNote(void)
{
	markersManager.findNextSimilarNote();
}
//----------------------------------------------------------------------------
void TasEditorWindow::openAboutWindow(void)
{
	QDialog about(this);
	QVBoxLayout *mainLayout, *vbox;
	QHBoxLayout *hbox;
	QPixmap pm(":icons/taseditor-icon32.png");
	QPixmap pm2;
	QLabel *imgLbl;
	QTextEdit *txtEdit;
	QPushButton *okButton;
	const char *txt = "\
Created by AnS\n\n\
Originated from TASEdit\n\
made by zeromus & adelikat\n\n\
Ported to Qt by mjbudd77\n\
";
	
	pm2 = pm.scaled( 64, 64 );

	mainLayout = new QVBoxLayout();
	vbox       = new QVBoxLayout();
	hbox       = new QHBoxLayout();
	txtEdit    = new QTextEdit();
	okButton   = new QPushButton( tr("OK") );

	about.setWindowTitle( tr("About") );
	about.setLayout( mainLayout );

	imgLbl = new QLabel();
	imgLbl->setPixmap(pm2);

	mainLayout->addLayout( hbox );
	hbox->addWidget( imgLbl, 2, Qt::AlignCenter );
	hbox->addLayout( vbox, 2 );
	vbox->addWidget( new QLabel( tr("TAS Editor") ), 1, Qt::AlignCenter );
	vbox->addWidget( new QLabel( tr("Version 1.01") ), 1, Qt::AlignCenter );
	mainLayout->addWidget( txtEdit );

	hbox = new QHBoxLayout();
	hbox->addStretch(5);
	hbox->addWidget( okButton, 1 );
	mainLayout->addLayout( hbox );

	txtEdit->setText( tr(txt) );
	txtEdit->setReadOnly(true);

	okButton->setDefault(true);
	okButton->setIcon(style()->standardIcon(QStyle::SP_DialogOkButton));
	connect( okButton, SIGNAL(clicked(void)), &about, SLOT(accept(void)) );

	about.exec();
}
//----------------------------------------------------------------------------
//------ Custom Vertical Scroll For Piano Roll
//----------------------------------------------------------------------------
PianoRollScrollBar::PianoRollScrollBar( QWidget *parent )
	: QScrollBar( Qt::Vertical, parent )
{
	pxLineSpacing = 12;
	wheelPixelCounter = 0;
	wheelAngleCounter = 0;
}
//----------------------------------------------------------------------------
PianoRollScrollBar::~PianoRollScrollBar(void)
{
}
//----------------------------------------------------------------------------
void PianoRollScrollBar::wheelEvent(QWheelEvent *event)
{
	int ofs, zDelta = 0;

	//QScrollBar::wheelEvent(event);
	QPoint numPixels = event->pixelDelta();
	QPoint numDegrees = event->angleDelta();

	ofs = value();

	if (!numPixels.isNull())
	{
		wheelPixelCounter -= numPixels.y();
		//printf("numPixels: (%i,%i) \n", numPixels.x(), numPixels.y() );

		if ( wheelPixelCounter >= pxLineSpacing )
		{
			zDelta = wheelPixelCounter / pxLineSpacing;

			wheelPixelCounter = wheelPixelCounter % pxLineSpacing;
		}
		else if ( wheelPixelCounter <= -pxLineSpacing )
		{
			zDelta = wheelPixelCounter / pxLineSpacing;

			wheelPixelCounter = wheelPixelCounter % pxLineSpacing;
		}
	}
	else if (!numDegrees.isNull())
	{
		int stepDeg = 120;
		//QPoint numSteps = numDegrees / 15;
		//printf("numSteps: (%i,%i) \n", numSteps.x(), numSteps.y() );
		//printf("numDegrees: (%i,%i)  %i\n", numDegrees.x(), numDegrees.y(), pxLineSpacing );
		wheelAngleCounter -= numDegrees.y();

		if ( wheelAngleCounter <= stepDeg )
		{
			zDelta = wheelAngleCounter / stepDeg;

			wheelAngleCounter = wheelAngleCounter % stepDeg;
		}
		else if ( wheelAngleCounter >= stepDeg )
		{
			zDelta = wheelAngleCounter / stepDeg;

			wheelAngleCounter = wheelAngleCounter % stepDeg;
		}
	}

	if ( zDelta != 0 )
	{
		ofs = ofs + (6 * zDelta);

		if ( ofs < 0 )
		{
			ofs = 0;
		}
		else if ( ofs > maximum() )
		{
			ofs = maximum();
		}
		setValue( ofs );
	}
	event->accept();
}
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

	viewWidth  = 256;
	viewHeight = 512;
	setMinimumWidth( viewWidth );
	setMinimumHeight( viewHeight );
	setAcceptDrops(true);

	g_config->getOption("SDL.TasPianoRollFont", &fontString);

	if ( fontString.size() > 0 )
	{
		//printf("Font String: '%s'\n", fontString.c_str() );
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
	this->setFocusPolicy(Qt::StrongFocus);
	this->setMouseTracking(true);
	this->setPalette(pal);

	numCtlr = 2;
	numColumns = 2 + (NUM_JOYPAD_BUTTONS * numCtlr);

	vbar = NULL;
	hbar = NULL;

	mkrDrag = NULL;
	lineOffset = 0;
	maxLineOffset = 0;
	playbackCursorPos = 0;
	dragMode = DRAG_MODE_NONE;
	dragSelectionStartingFrame = 0;
	dragSelectionEndingFrame = 0;
	realRowUnderMouse = -1;
	rowUnderMouse = -1;
	columnUnderMouse = 0;
	rowUnderMouseAtPress = -1;
	columnUnderMouseAtPress = 0;
	markerDragFrameNumber = 0;
	markerDragCountdown = 0;
	drawingStartTimestamp = 0;
	wheelPixelCounter = 0;
	wheelAngleCounter = 0;
	headerItemUnderMouse = 0;
	nextHeaderUpdateTime = 0;
	rightButtonDragMode = false;
	mouse_x = mouse_y = -1;
	scroll_x = scroll_y = 0;
	memset( headerColors, 0, sizeof(headerColors) );

	headerLightsColors[ 0] = QColor( 0x00, 0x00, 0x00 );
	headerLightsColors[ 1] = QColor( 0x13, 0x73, 0x00 );
	headerLightsColors[ 2] = QColor( 0x00, 0x91, 0x00 );
	headerLightsColors[ 3] = QColor( 0x00, 0xAF, 0x1D );
	headerLightsColors[ 4] = QColor( 0x00, 0xC7, 0x42 );
	headerLightsColors[ 5] = QColor( 0x00, 0xD9, 0x65 );
	headerLightsColors[ 6] = QColor( 0x00, 0xE5, 0x91 );
	headerLightsColors[ 7] = QColor( 0x00, 0xF0, 0xB0 );
	headerLightsColors[ 8] = QColor( 0x00, 0xF7, 0xDA );
	headerLightsColors[ 9] = QColor( 0x7C, 0xFC, 0xF0 );
	headerLightsColors[10] = QColor( 0xBA, 0xFF, 0xFC );

	hotChangesColors[ 0] = QColor( 0x00, 0x00, 0x00 );
	hotChangesColors[ 1] = QColor( 0x35, 0x40, 0x00 );
	hotChangesColors[ 2] = QColor( 0x18, 0x52, 0x18 );
	hotChangesColors[ 3] = QColor( 0x34, 0x5C, 0x5E );
	hotChangesColors[ 4] = QColor( 0x00, 0x4C, 0x80 );
	hotChangesColors[ 5] = QColor( 0x00, 0x03, 0xBA );
	hotChangesColors[ 6] = QColor( 0x38, 0x00, 0xD1 );
	hotChangesColors[ 7] = QColor( 0x72, 0x12, 0xB2 );
	hotChangesColors[ 8] = QColor( 0xAB, 0x00, 0xBA );
	hotChangesColors[ 9] = QColor( 0xB0, 0x00, 0x6F );
	hotChangesColors[10] = QColor( 0xC2, 0x00, 0x37 );
	hotChangesColors[11] = QColor( 0xBA, 0x0C, 0x00 );
	hotChangesColors[12] = QColor( 0xC9, 0x2C, 0x00 );
	hotChangesColors[13] = QColor( 0xBF, 0x53, 0x00 );
	hotChangesColors[14] = QColor( 0xCF, 0x72, 0x00 );
	hotChangesColors[15] = QColor( 0xC7, 0x8B, 0x3C );

	gridPixelWidth = 1;
	gridColor = QColor( 0x00, 0x00, 0x00 );

	fceuLoadConfigColor("SDL.TasPianoRollGridColor"   , &gridColor );

	calcFontData();
}
//----------------------------------------------------------------------------
QPianoRoll::~QPianoRoll(void)
{

}
//----------------------------------------------------------------------------
void QPianoRoll::reset(void)
{
	int num_joysticks = joysticksPerFrame[getInputType(currMovieData)];

	numCtlr = num_joysticks;

	numColumns = 2 + (NUM_JOYPAD_BUTTONS * num_joysticks);

}
//----------------------------------------------------------------------------
void QPianoRoll::save(EMUFILE *os, bool really_save)
{
	if (really_save)
	{
		updateLinesCount();
		// write "PIANO_ROLL" string
		os->fwrite(pianoRollSaveID, PIANO_ROLL_ID_LEN);
		// write current top item
		int top_item = lineOffset;
		write32le(top_item, os);
	}
	else
	{
		// write "PIANO_ROLX" string
		os->fwrite(pianoRollSkipSaveID, PIANO_ROLL_ID_LEN);
	}
}
//----------------------------------------------------------------------------
// returns true if couldn't load
bool QPianoRoll::load(EMUFILE *is, unsigned int offset)
{
	reset();
	updateLinesCount();
	if (offset)
	{
		if (is->fseek(offset, SEEK_SET)) goto error;
	}
	else
	{
		// scroll to the beginning
		//ListView_EnsureVisible(hwndList, 0, FALSE);
		lineOffset = 0;
		return false;
	}
	// read "PIANO_ROLL" string
	char save_id[PIANO_ROLL_ID_LEN];
	if ((int)is->fread(save_id, PIANO_ROLL_ID_LEN) < PIANO_ROLL_ID_LEN) goto error;
	if (!strcmp(pianoRollSkipSaveID, save_id))
	{
		// string says to skip loading Piano Roll
		FCEU_printf("No Piano Roll data in the file\n");
		// scroll to the beginning
		//ListView_EnsureVisible(hwndList, 0, FALSE);
		lineOffset = 0;
		return false;
	}
	if (strcmp(pianoRollSaveID, save_id)) goto error;		// string is not valid
	// read current top item and scroll Piano Roll there
	int top_item;
	if (!read32le(&top_item, is)) goto error;
	//ListView_EnsureVisible(hwndList, currMovieData.getNumRecords() - 1, FALSE);
	//ListView_EnsureVisible(hwndList, top_item, FALSE);
	ensureTheLineIsVisible( currMovieData.getNumRecords() - 1 );
	ensureTheLineIsVisible( top_item );
	return false;
error:
	FCEU_printf("Error loading Piano Roll data\n");
	// scroll to the beginning
	//ListView_EnsureVisible(hwndList, 0, FALSE);
	lineOffset = 0;
	return true;
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
//void QPianoRoll::vbarActionTriggered(int act)
//{
//	int val = vbar->value();
//
//	if ( act == QAbstractSlider::SliderSingleStepAdd )
//	{
//		val = val - vbar->singleStep();
//
//		if ( val < 0 )
//		{
//			val = 0;
//		}
//		vbar->setSliderPosition(val);
//	}
//	else if ( act == QAbstractSlider::SliderSingleStepSub )
//	{
//		val = val + vbar->singleStep();
//
//		if ( val >= maxLineOffset )
//		{
//			val = maxLineOffset;
//		}
//		vbar->setSliderPosition(val);
//	}
//        else if ( act == QAbstractSlider::SliderPageStepAdd )
//        {
//               	val = val - vbar->pageStep();
//
//		if ( val < 0 )
//		{
//			val = 0;
//		}
//		vbar->setSliderPosition(val);
//        }
//        else if ( act == QAbstractSlider::SliderPageStepSub )
//        {
//                val = val + vbar->pageStep();
//
//		if ( val >= maxLineOffset )
//		{
//			val = maxLineOffset;
//		}
//		vbar->setSliderPosition(val);
//        }
//	//printf("ACT:%i\n", act);
//}
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
void QPianoRoll::setFont( QFont &newFont )
{
	font = newFont;
	QWidget::setFont( newFont );
	calcFontData();
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
	pxWidthFrameCol =  9 * pxCharWidth;
	pxWidthBtnCol   =  3 * pxCharWidth;
	pxWidthCtlCol   =  8 * pxWidthBtnCol;

	pxFrameColX     = pxWidthCol1;

	for (int i=0; i<4; i++)
	{
		pxFrameCtlX[i] = pxFrameColX + pxWidthFrameCol + (i*pxWidthCtlCol);
	}
	pxLineWidth = pxFrameCtlX[ numCtlr-1 ] + pxWidthCtlCol;

	if ( vbar )
	{
		if ( maxLineOffset < 0 )
		{
			vbar->hide();
			maxLineOffset = 0;
		}
		else
		{
			vbar->show();
		}
		vbar->setMinimum(0);
		vbar->setMaximum(maxLineOffset);
		vbar->setPageStep( (7*viewLines)/8 );
	}

	if ( hbar )
	{
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
	}
}
//----------------------------------------------------------------------------
QPoint QPianoRoll::convPixToCursor( QPoint p )
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
		c.setY( -1 );
	}
	else 
	{
		float py = ( (float)p.y() ) /  (float)pxLineSpacing;

		c.setY( (int)py - 1 );
	}
	return c;
}
//----------------------------------------------------------------------------
int  QPianoRoll::calcColumn( int px )
{
	int col = -1;

	px = px + pxLineXScroll;

	if ( px < pxFrameColX )
	{
		col = COLUMN_ICONS;
	}
	else if ( px < pxFrameCtlX[0] )
	{
		col = COLUMN_FRAMENUM;
	}
	else
	{
		int i=0;

		while ( px < pxFrameCtlX[i] )
		{
			if ( i >= 3 )
			{
				break;
			}
			i++;
		}
		col = COLUMN_JOYPAD1_A + (i*8) + ( (px - pxFrameCtlX[i]) / pxWidthBtnCol);
	}
	return col;
}
//----------------------------------------------------------------------------
void QPianoRoll::drawArrow( QPainter *painter, int xl, int yl, int value )
{
	int x, y, w, h;
	QPoint p[3];
	bool hasBookmark = false;
	bool draw2ndArrow = false;
	bool draw1stArrow = true;
	QColor green( 0, 0xC0, 0x40 ), blue( 0x60, 0xC0, 0xC0 );
	QColor arrowColor1 = green;
	QColor arrowColor2 = blue;

	x = xl+(pxCharWidth/3);
	y = yl+1;
	w = pxCharWidth;
	h = pxLineSpacing-2;

	if ( (value & BOOKMARKS_WITH_GREEN_ARROW) || (value & BOOKMARKS_WITH_BLUE_ARROW) || (value & BOOKMARKS_WITH_NO_ARROW) )
	{
		char txt[4];
		int bookmarkNum;

		bookmarkNum = (value & 0x0000FFFF);

		txt[0] = (bookmarkNum % TOTAL_BOOKMARKS) + '0';
		txt[1] = 0;
		
		painter->drawText( x, y+pxLineTextOfs, tr(txt) );

		hasBookmark  = true;
		draw1stArrow = false;
		draw2ndArrow = (value & BOOKMARKS_WITH_NO_ARROW) ? false : true;

		x += pxCharWidth;

	}

	p[0] = QPoint( x, y );
	p[1] = QPoint( x, y+h );
	p[2] = QPoint( x+w, y+(h/2) );

	if ( hasBookmark )
	{
		if ( value & BOOKMARKS_WITH_GREEN_ARROW )
		{
			arrowColor1 = green;
		}
		else if ( value & BOOKMARKS_WITH_BLUE_ARROW )
		{
			arrowColor1 = blue;
		}
	}
	else
	{
		if ( value & GREEN_ARROW_IMAGE_ID )
		{
			arrowColor1 = green;

			if ( value & BLUE_ARROW_IMAGE_ID )
			{
				draw2ndArrow = true;
				arrowColor2 = blue;
			}
		}
		else if ( value & BLUE_ARROW_IMAGE_ID )
		{
			arrowColor1 = blue;
		}
	}
	if ( draw1stArrow )
	{
		painter->setBrush( arrowColor1 );
		painter->drawPolygon( p, 3 );
		x += pxCharWidth;
	}

	if ( draw2ndArrow )
	{
		x += (pxCharWidth / 4);

		p[0] = QPoint( x, y+1 );
		p[1] = QPoint( x, y+h-1 );
		p[2] = QPoint( x+w-1, y+(h/2) );

		painter->setBrush( arrowColor2 );

		painter->drawPolygon( p, 3 );
	}
}
//----------------------------------------------------------------------------
void QPianoRoll::updateLinesCount(void)
{
	// update the number of items in the list
	int movie_size = currMovieData.getNumRecords();

	maxLineOffset = movie_size - viewLines + 2;

	if ( maxLineOffset < 0 )
	{
		maxLineOffset = 0;
	}
}
//----------------------------------------------------------------------------
bool QPianoRoll::lineIsVisible( int lineNum )
{
	int lineEnd = lineOffset + viewLines - 2;

	return ( (lineNum >= lineOffset) && (lineNum < lineEnd) );
}
//----------------------------------------------------------------------------
void QPianoRoll::ensureTheLineIsVisible( int lineNum )
{
	if ( !lineIsVisible( lineNum ) )
	{
		//int lineEnd = lineOffset + viewLines - 2;
		//printf("Seeking Frame %i\n", lineNum );

		if ( lineNum < lineOffset )
		{
			lineOffset = lineNum;
		}
		else
		{
			//printf("Seeking View Frame %i\n", lineNum );
			lineOffset = lineOffset - viewLines + 2;
		}

		if ( lineOffset < 0 )
		{
			lineOffset = 0;
		}
		else if ( lineOffset > maxLineOffset )
		{
			lineOffset = maxLineOffset;
		}
		vbar->setValue( lineOffset );

		update();
	}
}
//----------------------------------------------------------------------------
void QPianoRoll::resizeEvent(QResizeEvent *event)
{
	viewWidth  = event->size().width();
	viewHeight = event->size().height();

	//printf("QPianoRoll Resize: %ix%i  $%04X\n", viewWidth, viewHeight );

	viewLines = (viewHeight / pxLineSpacing) + 1;

	maxLineOffset = currMovieData.records.size() - viewLines + 2;

	if ( maxLineOffset < 0 )
	{
		vbar->hide();
		maxLineOffset = 0;
	}
	else
	{
		vbar->show();
	}
	vbar->setMinimum(0);
	vbar->setMaximum(maxLineOffset);
	vbar->setPageStep( (7*viewLines)/8 );

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

}
//----------------------------------------------------------------------------
void QPianoRoll::mouseDoubleClickEvent(QMouseEvent * event)
{
	FCEU_CRITICAL_SECTION( emuLock );
	int col, line, row_index, column_index, kbModifiers, alt_pressed;
	bool headerClicked, row_valid;
	QPoint c = convPixToCursor( event->pos() );

	//printf("Mouse Double Click Pressed: 0x%x (%i,%i)\n", event->button(), c.x(), c.y() );

	mouse_x = event->pos().x();
	mouse_y = event->pos().y();

	if ( c.y() >= 0 )
	{
		line = lineOffset + c.y();
		headerClicked = false;
	}
	else
	{
		line = -1;
		headerClicked = true;
	}
	col  = calcColumn( event->pos().x() );

	rowUnderMouseAtPress = rowUnderMouse = realRowUnderMouse = row_index = line;
	columnUnderMouseAtPress = columnUnderMouse = column_index = col;

	row_valid = (row_index >= 0) && ( (size_t)row_index < currMovieData.records.size() );

	kbModifiers = QApplication::keyboardModifiers();
	alt_pressed = (kbModifiers & Qt::AltModifier) ? 1 : 0;

	if ( event->button() == Qt::LeftButton )
	{
		if (col == COLUMN_ICONS)
		{
			// clicked on the "icons" column
			startDraggingPlaybackCursor();
		}
		else if ( (col == COLUMN_FRAMENUM) || (col == COLUMN_FRAMENUM2) )
		{
			//handleColumnSet( col, alt_pressed );

			// doubleclick - set Marker and start dragging it
			if (!markersManager->getMarkerAtFrame(row_index))
			{
				if (markersManager->setMarkerAtFrame(row_index))
				{
					selection->mustFindCurrentMarker = playback->mustFindCurrentMarker = true;
					history->registerMarkersChange(MODTYPE_MARKER_SET, row_index);
					update();
				}
			}
			// Delay drag event by 100ms incase the button is quickly released
			QTimer::singleShot( 100, this, SLOT(setupMarkerDrag(void)) );

			//startDraggingMarker( mouse_x, mouse_y, row_index, column_index);
		}
		else if (column_index >= COLUMN_JOYPAD1_A && column_index <= COLUMN_JOYPAD4_R)
		{
			// clicked on Input
			if (headerClicked)
			{
				drawingStartTimestamp = clock();
				int joy = (column_index - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS;
				int button = (column_index - COLUMN_JOYPAD1_A) % NUM_JOYPAD_BUTTONS;
				int selection_beginning = selection->getCurrentRowsSelectionBeginning();
				int selection_end       = selection->getCurrentRowsSelectionEnd();

				if ( (selection_beginning >= 0) && (selection_end >= 0) )
				{
					tasWin->toggleInput(selection_beginning, selection_end, joy, button, drawingStartTimestamp);
				}
			}
			else if (row_index >= 0)
			{
				if (!alt_pressed && !(kbModifiers & Qt::ShiftModifier))
				{
					// clicked without Shift/Alt - bring Selection cursor to this row
					selection->clearAllRowsSelection();
					selection->setRowSelection(row_index);
				}
				// toggle Input
				drawingStartTimestamp = clock();
				int joy = (column_index - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS;
				int button = (column_index - COLUMN_JOYPAD1_A) % NUM_JOYPAD_BUTTONS;
				int selection_beginning = selection->getCurrentRowsSelectionBeginning();
				if (alt_pressed && selection_beginning >= 0)
				{
					tasWin->setInputUsingPattern(selection_beginning, row_index, joy, button, drawingStartTimestamp);
				}
				else if ((kbModifiers & Qt::ShiftModifier) && selection_beginning >= 0)
				{
					tasWin->toggleInput(selection_beginning, row_index, joy, button, drawingStartTimestamp);
				}
				else
				{
					tasWin->toggleInput(row_index, row_index, joy, button, drawingStartTimestamp);
				}
				// and start dragging/drawing
				if (dragMode == DRAG_MODE_NONE)
				{
					if (taseditorConfig->drawInputByDragging)
					{
						// if clicked this click created buttonpress, then start painting, else start erasing
						if ( row_valid && currMovieData.records[row_index].checkBit(joy, button))
						{
							dragMode = DRAG_MODE_SET;
						}
						else
						{
							dragMode = DRAG_MODE_UNSET;
						}
					}
					else
					{
						dragMode = DRAG_MODE_OBSERVE;
					}
				}
			}
		}
	}
	else if ( event->button() == Qt::MiddleButton )
	{
		playback->handleMiddleButtonClick();
	}
	event->accept();
}
//----------------------------------------------------------------------------
void QPianoRoll::contextMenuEvent(QContextMenuEvent *event)
{
	bool drawContext, rowIsSel;

	rowIsSel = selection->isRowSelected( rowUnderMouse );

	drawContext = rowIsSel && 
		( (columnUnderMouse == COLUMN_ICONS) || (columnUnderMouse == COLUMN_FRAMENUM) || (columnUnderMouse == COLUMN_FRAMENUM2) );

	if ( !drawContext )
	{
		return;
	}
	int mkr;
	QAction *act;
	QMenu menu(this);
	FCEU_CRITICAL_SECTION( emuLock );

	mkr = markersManager->getMarkerAtFrame( rowUnderMouse );

	act = new QAction(tr("Set Markers\tDbl-Clk"), &menu);
	menu.addAction(act);
	act->setEnabled( mkr == 0 );
	//act->setShortcut(QKeySequence(tr("Double Click")));
	connect(act, SIGNAL(triggered(void)), tasWin, SLOT(setMarkers(void)));

	act = new QAction(tr("Remove Markers"), &menu);
	menu.addAction(act);
	act->setEnabled( mkr > 0 );
	//act->setShortcut(QKeySequence(tr("Dbl-clk")));
	connect(act, SIGNAL(triggered(void)), tasWin, SLOT(removeMarkers(void)));

	menu.addSeparator();

	act = new QAction(tr("Deselect"), &menu);
	menu.addAction(act);
	//act->setShortcut(QKeySequence(tr("D")));
	connect(act, SIGNAL(triggered(void)), tasWin, SLOT(editDeselectAll(void)));

	act = new QAction(tr("Select between markers"), &menu);
	menu.addAction(act);
	act->setShortcut(QKeySequence(tr("Ctrl-A")));
	connect(act, SIGNAL(triggered(void)), tasWin, SLOT(editSelBtwMkrs(void)));

	menu.addSeparator();

	act = new QAction(tr("Ungreenzone"), &menu);
	menu.addAction(act);
	//act->setShortcut(QKeySequence(tr("Ctrl-A")));
	connect(act, SIGNAL(triggered(void)), tasWin, SLOT(ungreenzoneSelectedFrames(void)));

	menu.addSeparator();

	act = new QAction(tr("Clear"), &menu);
	menu.addAction(act);
	act->setShortcut(QKeySequence(tr("Del")));
	connect(act, SIGNAL(triggered(void)), tasWin, SLOT(editClearCB(void)));

	act = new QAction(tr("Delete"), &menu);
	menu.addAction(act);
	act->setShortcut(QKeySequence(tr("Ctrl+Del")));
	connect(act, SIGNAL(triggered(void)), tasWin, SLOT(editDeleteCB(void)));

	act = new QAction(tr("Clone"), &menu);
	menu.addAction(act);
	act->setShortcut(QKeySequence(tr("Ctrl+Ins")));
	connect(act, SIGNAL(triggered(void)), tasWin, SLOT(editCloneCB(void)));

	act = new QAction(tr("Insert"), &menu);
	menu.addAction(act);
	act->setShortcut(QKeySequence(tr("Ctrl+Shift+Ins")));
	connect(act, SIGNAL(triggered(void)), tasWin, SLOT(editInsertCB(void)));

	act = new QAction(tr("Insert # of Frames"), &menu);
	menu.addAction(act);
	act->setShortcut(QKeySequence(tr("Ins")));
	connect(act, SIGNAL(triggered(void)), tasWin, SLOT(editInsertNumFramesCB(void)));

	menu.addSeparator();

	act = new QAction(tr("Truncate Movie"), &menu);
	menu.addAction(act);
	//act->setShortcut(QKeySequence(tr("Ins")));
	connect(act, SIGNAL(triggered(void)), tasWin, SLOT(editTruncateMovieCB(void)));

	menu.exec(event->globalPos());

	event->accept();
}
//----------------------------------------------------------------------------
void QPianoRoll::mousePressEvent(QMouseEvent * event)
{
	FCEU_CRITICAL_SECTION( emuLock );
	int col, line, row_index, column_index, kbModifiers, alt_pressed;
	bool row_valid, headerClicked;
	QPoint c = convPixToCursor( event->pos() );

	mouse_x = event->pos().x();
	mouse_y = event->pos().y();

	if ( c.y() >= 0 )
	{
		line = lineOffset + c.y();
		headerClicked = false;
	}
	else
	{
		line = -1;
		headerClicked = true;
	}
	col  = calcColumn( event->pos().x() );

	row_index = line;
	rowUnderMouseAtPress = rowUnderMouse = realRowUnderMouse = line;
	columnUnderMouseAtPress = columnUnderMouse = column_index = col;

	row_valid = (row_index >= 0) && ( (size_t)row_index < currMovieData.records.size() );

	kbModifiers = QApplication::keyboardModifiers();
	alt_pressed = (kbModifiers & Qt::AltModifier) ? 1 : 0;

	//printf("Mouse Button Pressed: 0x%x (%i,%i)\n", event->button(), c.x(), c.y() );
	
	if ( event->button() == Qt::LeftButton )
	{
		if (col == COLUMN_ICONS)
		{
			// clicked on the "icons" column
			startDraggingPlaybackCursor();
		}
		else if ( (col == COLUMN_FRAMENUM) || (col == COLUMN_FRAMENUM2) )
		{
			// clicked on the "Frame#" column
			if (row_index >= 0)
			{
				if (kbModifiers & Qt::ShiftModifier)
				{
					// select region from selection_beginning to row_index
					int selection_beginning = selection->getCurrentRowsSelectionBeginning();
					if (selection_beginning >= 0)
					{
						if (selection_beginning < row_index)
						{
							selection->setRegionOfRowsSelection(selection_beginning, row_index + 1);
						}
						else
						{
							selection->setRegionOfRowsSelection(row_index, selection_beginning + 1);
						}
					}
					startSelectingDrag(row_index);
				}
				else if (kbModifiers & Qt::AltModifier)
				{
					// make Selection by Pattern
					int selection_beginning = selection->getCurrentRowsSelectionBeginning();
					if (selection_beginning >= 0)
					{
						selection->clearAllRowsSelection();
						if (selection_beginning < row_index)
						{
							selection->setRegionOfRowsSelectionUsingPattern(selection_beginning, row_index);
						}
						else
						{
							selection->setRegionOfRowsSelectionUsingPattern(row_index, selection_beginning);
						}
					}
					if (selection->isRowSelected(row_index))
					{
						startDeselectingDrag(row_index);
					}
					else
					{
						startSelectingDrag(row_index);
					}
				}
				else if (kbModifiers & Qt::ControlModifier)
				{
					// clone current selection, so that user will be able to revert
					if (selection->getCurrentRowsSelectionSize() > 0)
					{
						selection->addCurrentSelectionToHistory();
					}
					if (selection->isRowSelected(row_index))
					{
						selection->clearSingleRowSelection(row_index);
						startDeselectingDrag(row_index);
					}
					else
					{
						selection->setRowSelection(row_index);
						startSelectingDrag(row_index);
					}
				}
				else	// just click
				{
					selection->clearAllRowsSelection();
					selection->setRowSelection(row_index);
					startSelectingDrag(row_index);
				}
			}
		}
		else if (column_index >= COLUMN_JOYPAD1_A && column_index <= COLUMN_JOYPAD4_R)
		{
			// clicked on Input
			if (headerClicked)
			{
				drawingStartTimestamp = clock();
				int joy = (column_index - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS;
				int button = (column_index - COLUMN_JOYPAD1_A) % NUM_JOYPAD_BUTTONS;
				int selection_beginning = selection->getCurrentRowsSelectionBeginning();
				int selection_end       = selection->getCurrentRowsSelectionEnd();

				if ( (selection_beginning >= 0) && (selection_end >= 0) )
				{
					tasWin->toggleInput(selection_beginning, selection_end, joy, button, drawingStartTimestamp);
				}
			}
			else if (row_index >= 0)
			{
				if (!alt_pressed && !(kbModifiers & Qt::ShiftModifier))
				{
					// clicked without Shift/Alt - bring Selection cursor to this row
					selection->clearAllRowsSelection();
					selection->setRowSelection(row_index);
				}
				// toggle Input
				drawingStartTimestamp = clock();
				int joy = (column_index - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS;
				int button = (column_index - COLUMN_JOYPAD1_A) % NUM_JOYPAD_BUTTONS;
				int selection_beginning = selection->getCurrentRowsSelectionBeginning();
				if (alt_pressed && selection_beginning >= 0)
				{
					tasWin->setInputUsingPattern(selection_beginning, row_index, joy, button, drawingStartTimestamp);
				}
				else if ((kbModifiers & Qt::ShiftModifier) && selection_beginning >= 0)
				{
					tasWin->toggleInput(selection_beginning, row_index, joy, button, drawingStartTimestamp);
				}
				else
				{
					tasWin->toggleInput(row_index, row_index, joy, button, drawingStartTimestamp);
				}
				// and start dragging/drawing
				if (dragMode == DRAG_MODE_NONE)
				{
					if (taseditorConfig->drawInputByDragging)
					{
						// if clicked this click created buttonpress, then start painting, else start erasing
						if ( row_valid && currMovieData.records[row_index].checkBit(joy, button))
						{
							dragMode = DRAG_MODE_SET;
						}
						else
						{
							dragMode = DRAG_MODE_UNSET;
						}
					}
					else
					{
						dragMode = DRAG_MODE_OBSERVE;
					}
				}
			}
		}
	}
	else if ( event->button() == Qt::MiddleButton )
	{
		playback->handleMiddleButtonClick();
	}
	else if ( event->button() == Qt::RightButton )
	{
		//rightButtonDragMode = true;
	}
}
//----------------------------------------------------------------------------
void QPianoRoll::mouseReleaseEvent(QMouseEvent * event)
{
	FCEU_CRITICAL_SECTION( emuLock );
	int col, line;
	QPoint c = convPixToCursor( event->pos() );

	mouse_x = event->pos().x();
	mouse_y = event->pos().y();

	if ( c.y() >= 0 )
	{
		line = lineOffset + c.y();
	}
	else
	{
		line = lineOffset;
	}
	col  = calcColumn( event->pos().x() );

	rowUnderMouse = realRowUnderMouse = line;
	columnUnderMouse = col;

	//printf("Mouse Button Released: 0x%x (%i,%i)\n", event->button(), c.x(), c.y() );
	
	if ( event->button() == Qt::LeftButton )
	{
		if (dragMode != DRAG_MODE_NONE)
		{
			// check if user released left button
			finishDrag();
		}
	}
	else if ( event->button() == Qt::RightButton )
	{
		//rightButtonDragMode = false;
	}
}
//----------------------------------------------------------------------------
void QPianoRoll::mouseMoveEvent(QMouseEvent * event)
{
	FCEU_CRITICAL_SECTION( emuLock );
	int col, line;
	QPoint c = convPixToCursor( event->pos() );

	mouse_x = event->pos().x();
	mouse_y = event->pos().y();

	if ( c.y() >= 0 )
	{
		line = lineOffset + c.y();
	}
	else
	{
		line = lineOffset;
	}
	col =  calcColumn( event->pos().x() );

	rowUnderMouse = realRowUnderMouse = line;
	columnUnderMouse = col;

	//printf("Mouse Move Event: 0x%x (%i,%i)  Col:%i\n", event->button(), c.x(), c.y(), col );
	
	if ( event->button() == Qt::LeftButton )
	{

	}
	updateDrag();
}
//----------------------------------------------------------------------------
void QPianoRoll::wheelEvent(QWheelEvent *event)
{
	FCEU_CRITICAL_SECTION( emuLock );
	int ofs, kbModifiers, msButtons, zDelta = 0;

	QPoint numPixels = event->pixelDelta();
	QPoint numDegrees = event->angleDelta();

	msButtons   = QApplication::mouseButtons();
	kbModifiers = QApplication::keyboardModifiers();

	ofs = vbar->value();

	if (!numPixels.isNull())
	{
		wheelPixelCounter += numPixels.y();
		//printf("numPixels: (%i,%i) \n", numPixels.x(), numPixels.y() );

		if (wheelPixelCounter <= -pxLineSpacing)
		{
			zDelta = (wheelPixelCounter / pxLineSpacing);

			wheelPixelCounter = wheelPixelCounter % pxLineSpacing;
		}
		else if (wheelPixelCounter >= pxLineSpacing)
		{
			zDelta = (wheelPixelCounter / pxLineSpacing);

			wheelPixelCounter = wheelPixelCounter % pxLineSpacing;
		}
	}
	else if (!numDegrees.isNull())
	{
		int stepDeg = 120;
		//QPoint numSteps = numDegrees / 15;
		//printf("numSteps: (%i,%i) \n", numSteps.x(), numSteps.y() );
		//printf("numDegrees: (%i,%i)  %i\n", numDegrees.x(), numDegrees.y(), pxLineSpacing );
		wheelAngleCounter += numDegrees.y();

		if ( wheelAngleCounter <= stepDeg )
		{
			zDelta = wheelAngleCounter / stepDeg;

			wheelAngleCounter = wheelAngleCounter % stepDeg;
		}
		else if ( wheelAngleCounter >= stepDeg )
		{
			zDelta = wheelAngleCounter / stepDeg;

			wheelAngleCounter = wheelAngleCounter % stepDeg;
		}
	}
	//printf("Wheel Event: %i\n", wheelPixelCounter);

	if ( kbModifiers & Qt::ShiftModifier )
	{
		// Shift + wheel = Playback rewind full(speed)/forward full(speed)
		if (zDelta < 0)
		{
			playback->handleForwardFull( -zDelta );
		}
		else if (zDelta > 0)
		{
			playback->handleRewindFull( zDelta );
		}
	}
	else if ( kbModifiers & Qt::ControlModifier )
	{
		// Ctrl + wheel = Selection rewind full(speed)/forward full(speed)
		if (zDelta < 0)
		{
			selection->jumpToNextMarker( -zDelta );
		}
		else if (zDelta > 0)
		{
			selection->jumpToPreviousMarker( zDelta );
		}
	}
	else if ( msButtons & Qt::RightButton )
	{
		// Right button + wheel = rewind/forward Playback
		int delta = zDelta;
		if (delta < -1 || delta > 1)
		{
			delta *= PLAYBACK_WHEEL_BOOST;
		}
		int destination_frame;
		if (FCEUI_EmulationPaused() || playback->getPauseFrame() < 0)
		{
			destination_frame = currFrameCounter - delta;
		}
		else
		{
			destination_frame = playback->getPauseFrame() - delta;
		}
		if (destination_frame < 0)
		{
			destination_frame = 0;
		}
		playback->jump(destination_frame);
	}
	else if (kbModifiers & Qt::AltModifier)
	{
		// cross gaps in Input/Markers
		if ( zDelta != 0 )
		{
			crossGaps(zDelta);
		}
	}
	else
	{
		if (zDelta > 0)
		{
			ofs -= (zDelta*6);

			if (ofs > maxLineOffset)
			{
				ofs = maxLineOffset;
			}
			vbar->setValue(ofs);
		}
		else if (zDelta < 0)
		{
			ofs -= (zDelta*6);

			if (ofs < 0)
			{
				ofs = 0;
			}
			vbar->setValue(ofs);
		}
	}

	event->accept();
}
//----------------------------------------------------------------------------
void QPianoRoll::keyPressEvent(QKeyEvent *event)
{
	//printf("Key Press: 0x%x \n", event->key() );
	pushKeyEvent( event, 1 );

	event->accept();
}

void QPianoRoll::keyReleaseEvent(QKeyEvent *event)
{
	//printf("Key Release: 0x%x \n", event->key() );
	pushKeyEvent( event, 0 );

	event->accept();
}
//----------------------------------------------------------------------------
void QPianoRoll::focusInEvent(QFocusEvent *event)
{
	QWidget::focusInEvent(event);

	//printf("PianoRoll Focus In\n");

	parent->pianoRollFrame->setStyleSheet("QFrame { border: 2px solid rgb(48,140,198); }");
}
//----------------------------------------------------------------------------
void QPianoRoll::focusOutEvent(QFocusEvent *event)
{
	QWidget::focusOutEvent(event);

	//printf("PianoRoll Focus Out\n");

	parent->pianoRollFrame->setStyleSheet(NULL);
}
//----------------------------------------------------------------------------
void QPianoRoll::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->mimeData()->hasUrls() )
	{
		QList<QUrl> urls = event->mimeData()->urls();
		QFileInfo fi( urls[0].toString( QUrl::PreferLocalFile ) );

		//printf("Suffix: '%s'\n", fi.suffix().toStdString().c_str() );

		if ( fi.suffix().compare("fm3") == 0)
		{
			event->acceptProposedAction();
		}
		else if ( fi.suffix().compare("fm2") == 0 )
		{
			event->acceptProposedAction();
		}
	}
	else
	{
		if ( event->source() == this )
		{
			event->acceptProposedAction();
		}
	}
}

//----------------------------------------------------------------------------
void QPianoRoll::dropEvent(QDropEvent *event)
{
	if (event->mimeData()->hasUrls() )
	{
		QList<QUrl> urls = event->mimeData()->urls();
		QFileInfo fi( urls[0].toString( QUrl::PreferLocalFile ) );

		if ( fi.suffix().compare("fm3") == 0 )
		{
			FCEU_WRAPPER_LOCK();
			tasWin->loadProject( fi.filePath().toStdString().c_str() );
			FCEU_WRAPPER_UNLOCK();
			event->accept();
		}
		else if ( fi.suffix().compare("fm2") == 0 )
		{
			FCEU_WRAPPER_LOCK();
			tasWin->importMovieFile( fi.filePath().toStdString().c_str() );
			FCEU_WRAPPER_UNLOCK();
			event->accept();
		}
	}
}
//----------------------------------------------------------------------------
bool QPianoRoll::checkIfTheresAnIconAtFrame(int frame)
{
	if (frame == currFrameCounter)
		return true;
	if (frame == playback->getLastPosition())
		return true;
	if (frame == playback->getPauseFrame())
		return true;
	if (bookmarks->findBookmarkAtFrame(frame) >= 0)
		return true;
	return false;
}
//----------------------------------------------------------------------------
void QPianoRoll::crossGaps(int zDelta)
{
	int row_index = rowUnderMouse;
	int column_index = columnUnderMouse;

	if (row_index >= 0 && column_index >= COLUMN_ICONS && column_index <= COLUMN_FRAMENUM2)
	{
		if (column_index == COLUMN_ICONS)
		{
			// cross gaps in Icons
			if (zDelta < 0)
			{
				// search down
				int last_frame = currMovieData.getNumRecords() - 1;
				if (row_index < last_frame)
				{
					int frame = row_index + 1;
					bool result_of_closest_frame = checkIfTheresAnIconAtFrame(frame);
					while ((++frame) <= last_frame)
					{
						if (checkIfTheresAnIconAtFrame(frame) != result_of_closest_frame)
						{
							// found different result, so we crossed the gap
							//ListView_Scroll(hwndList, 0, listRowHeight * (frame - row_index));
							centerListAroundLine(frame);
							break;
						}
					}
				}
			}
			else
			{
				// search up
				int first_frame = 0;
				if (row_index > first_frame)
				{
					int frame = row_index - 1;
					bool result_of_closest_frame = checkIfTheresAnIconAtFrame(frame);
					while ((--frame) >= first_frame)
					{
						if (checkIfTheresAnIconAtFrame(frame) != result_of_closest_frame)
						{
							// found different result, so we crossed the gap
							//ListView_Scroll(hwndList, 0, listRowHeight * (frame - row_index));
							centerListAroundLine(frame);
							break;
						}
					}
				}
			}
		}
		else if (column_index == COLUMN_FRAMENUM || column_index == COLUMN_FRAMENUM2)
		{
			// cross gaps in Markers
			if (zDelta < 0)
			{
				// search down
				int last_frame = currMovieData.getNumRecords() - 1;
				if (row_index < last_frame)
				{
					int frame = row_index + 1;
					bool result_of_closest_frame = (markersManager->getMarkerAtFrame(frame) != 0);
					while ((++frame) <= last_frame)
					{
						if ((markersManager->getMarkerAtFrame(frame) != 0) != result_of_closest_frame)
						{
							// found different result, so we crossed the gap
							//ListView_Scroll(hwndList, 0, listRowHeight * (frame - row_index));
							centerListAroundLine(frame);
							break;
						}
					}
				}
			}
			else
			{
				// search up
				int first_frame = 0;
				if (row_index > first_frame)
				{
					int frame = row_index - 1;
					bool result_of_closest_frame = (markersManager->getMarkerAtFrame(frame) != 0);
					while ((--frame) >= first_frame)
					{
						if ((markersManager->getMarkerAtFrame(frame) != 0) != result_of_closest_frame)
						{
							// found different result, so we crossed the gap
							//ListView_Scroll(hwndList, 0, listRowHeight * (frame - row_index));
							centerListAroundLine(frame);
							break;
						}
					}
				}
			}
		}
		else
		{
			// cross gaps in Input
			int joy = (column_index - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS;
			int button = (column_index - COLUMN_JOYPAD1_A) % NUM_JOYPAD_BUTTONS;
			if (zDelta < 0)
			{
				// search down
				int last_frame = currMovieData.getNumRecords() - 1;
				if (row_index < last_frame)
				{
					int frame = row_index + 1;
					bool result_of_closest_frame = currMovieData.records[frame].checkBit(joy, button);
					while ((++frame) <= last_frame)
					{
						if (currMovieData.records[frame].checkBit(joy, button) != result_of_closest_frame)
						{
							// found different result, so we crossed the gap
							//ListView_Scroll(hwndList, 0, listRowHeight * (frame - row_index));
							centerListAroundLine(frame);
							break;
						}
					}
				}
			}
			else
			{
				// search up
				int first_frame = 0;
				if (row_index > first_frame)
				{
					int frame = row_index - 1;
					bool result_of_closest_frame = currMovieData.records[frame].checkBit(joy, button);
					while ((--frame) >= first_frame)
					{
						if (currMovieData.records[frame].checkBit(joy, button) != result_of_closest_frame)
						{
							// found different result, so we crossed the gap
							//ListView_Scroll(hwndList, 0, listRowHeight * (frame - row_index));
							centerListAroundLine(frame);
							break;
						}
					}
				}
			}
		}
	}
}
//----------------------------------------------------------------------------
void QPianoRoll::updateDrag(void)
{
	int kbModifiers, altHeld;

	if ( dragMode == DRAG_MODE_NONE )
	{
		return;
	}
	kbModifiers = QApplication::keyboardModifiers();

	altHeld = (kbModifiers & Qt::AltModifier) ? 1 : 0;

	// perform drag
	switch (dragMode)
	{
		case DRAG_MODE_PLAYBACK:
		{
			handlePlaybackCursorDragging();
			break;
		}
		case DRAG_MODE_MARKER:
		{
			// if suddenly source frame lost its Marker, abort drag
			if (!markersManager->getMarkerAtFrame(markerDragFrameNumber))
			{
				//if (hwndMarkerDragBox)
				//{
				//	DestroyWindow(hwndMarkerDragBox);
				//	hwndMarkerDragBox = 0;
				//}
				setCursor( Qt::ArrowCursor );
				dragMode = DRAG_MODE_NONE;
				break;
			}
			// when dragging, always show semi-transparent yellow rectangle under mouse
			//POINT p = {0, 0};
			//GetCursorPos(&p);
			//markerDragBoxX = p.x - markerDragBoxDX;
			//markerDragBoxY = p.y - markerDragBoxDY;
			//if (!hwndMarkerDragBox)
			//{
			//	hwndMarkerDragBox = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT, markerDragBoxClassName, markerDragBoxClassName, WS_POPUP, markerDragBoxX, markerDragBoxY, COLUMN_FRAMENUM_WIDTH, listRowHeight, taseditorWindow.hwndTASEditor, NULL, fceu_hInstance, NULL);
			//	ShowWindow(hwndMarkerDragBox, SW_SHOWNA);
			//} else
			//{
			//	SetWindowPos(hwndMarkerDragBox, 0, markerDragBoxX, markerDragBoxY, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
			//}
			//SetLayeredWindowAttributes(hwndMarkerDragBox, 0, MARKER_DRAG_BOX_ALPHA, LWA_ALPHA);
			//UpdateLayeredWindow(hwndMarkerDragBox, 0, 0, 0, 0, 0, 0, &blend, ULW_ALPHA);
			break;
		}
		case DRAG_MODE_SET:
		case DRAG_MODE_UNSET:
		{
			//ScreenToClient(hwndList, &p);
			//int drawing_current_x = p.x + GetScrollPos(hwndList, SB_HORZ);
			//int drawing_current_y = p.y + GetScrollPos(hwndList, SB_VERT) * listRowHeight;
			//// draw (or erase) line from [drawing_current_x, drawing_current_y] to (drawing_last_x, drawing_last_y)
			//int total_dx = drawingLastX - drawing_current_x, total_dy = drawingLastY - drawing_current_y;
			//if (!shiftHeld)
			//{
			//	// when user is not holding Shift, draw only vertical lines
			//	total_dx = 0;
			//	drawing_current_x = drawingLastX;
			//	p.x = drawing_current_x - GetScrollPos(hwndList, SB_HORZ);
			//}
			//LVHITTESTINFO info;
			int row_index, column_index, joy, bit;
			int min_row_index = currMovieData.getNumRecords(), max_row_index = -1;
			bool changes_made = false;
			if (altHeld)
			{
				// special mode: draw pattern
				int selection_beginning = selection->getCurrentRowsSelectionBeginning();
				if (selection_beginning >= 0)
				{
					// perform hit test
					row_index = rowUnderMouse;
					// pad movie size if user tries to draw pattern below Piano Roll limit
					if (row_index >= currMovieData.getNumRecords())
					{
						currMovieData.insertEmpty(-1, row_index + 1 - currMovieData.getNumRecords());
					}
					column_index = columnUnderMouseAtPress;

					if (row_index >= 0 && column_index >= COLUMN_JOYPAD1_A && column_index <= COLUMN_JOYPAD4_R)
					{
						joy = (column_index - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS;
						bit = (column_index - COLUMN_JOYPAD1_A) % NUM_JOYPAD_BUTTONS;
						tasWin->setInputUsingPattern(selection_beginning, row_index, joy, bit, drawingStartTimestamp);
					}
				}
			}
			else
			{
				row_index = rowUnderMouseAtPress;

				while (row_index != rowUnderMouse)
				{
					// perform hit test
					//row_index = rowUnderMouse;
					if ( row_index < 0 )
					{
						break;
					}
					// pad movie size if user tries to draw below Piano Roll limit
					if (row_index >= currMovieData.getNumRecords())
					{
						currMovieData.insertEmpty(-1, row_index + 1 - currMovieData.getNumRecords());
					}
					column_index = columnUnderMouseAtPress;

					if (row_index >= 0 && column_index >= COLUMN_JOYPAD1_A && column_index <= COLUMN_JOYPAD4_R)
					{
						joy = (column_index - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS;
						bit = (column_index - COLUMN_JOYPAD1_A) % NUM_JOYPAD_BUTTONS;
						if (dragMode == DRAG_MODE_SET && !currMovieData.records[row_index].checkBit(joy, bit))
						{
							currMovieData.records[row_index].setBit(joy, bit);
							changes_made = true;
							if (min_row_index > row_index) min_row_index = row_index;
							if (max_row_index < row_index) max_row_index = row_index;
						}
						else if (dragMode == DRAG_MODE_UNSET && currMovieData.records[row_index].checkBit(joy, bit))
						{
							currMovieData.records[row_index].clearBit(joy, bit);
							changes_made = true;
							if (min_row_index > row_index) min_row_index = row_index;
							if (max_row_index < row_index) max_row_index = row_index;
						}
					}
					if ( row_index < rowUnderMouse )
					{
						row_index++;
					}
					else if ( row_index > rowUnderMouse )
					{
						row_index--;
					}
				}
				// pad movie size if user tries to draw below Piano Roll limit
				if (row_index >= currMovieData.getNumRecords())
				{
					currMovieData.insertEmpty(-1, row_index + 1 - currMovieData.getNumRecords());
				}
				column_index = columnUnderMouseAtPress;

				if (row_index >= 0 && column_index >= COLUMN_JOYPAD1_A && column_index <= COLUMN_JOYPAD4_R)
				{
					joy = (column_index - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS;
					bit = (column_index - COLUMN_JOYPAD1_A) % NUM_JOYPAD_BUTTONS;
					if (dragMode == DRAG_MODE_SET && !currMovieData.records[row_index].checkBit(joy, bit))
					{
						currMovieData.records[row_index].setBit(joy, bit);
						changes_made = true;
						if (min_row_index > row_index) min_row_index = row_index;
						if (max_row_index < row_index) max_row_index = row_index;
					}
					else if (dragMode == DRAG_MODE_UNSET && currMovieData.records[row_index].checkBit(joy, bit))
					{
						currMovieData.records[row_index].clearBit(joy, bit);
						changes_made = true;
						if (min_row_index > row_index) min_row_index = row_index;
						if (max_row_index < row_index) max_row_index = row_index;
					}
				}
				if (changes_made)
				{
					if (dragMode == DRAG_MODE_SET)
					{
						greenzone->invalidateAndUpdatePlayback(history->registerChanges(MODTYPE_SET, min_row_index, max_row_index, 0, NULL, drawingStartTimestamp));
					}
					else
					{
						greenzone->invalidateAndUpdatePlayback(history->registerChanges(MODTYPE_UNSET, min_row_index, max_row_index, 0, NULL, drawingStartTimestamp));
					}
				}
			}
			break;
		}
		case DRAG_MODE_SELECTION:
		{
			int new_drag_selection_ending_frame = realRowUnderMouse;
			// if trying to select above Piano Roll, select from frame 0
			if (new_drag_selection_ending_frame < 0)
				new_drag_selection_ending_frame = 0;
			else if (new_drag_selection_ending_frame >= currMovieData.getNumRecords())
				new_drag_selection_ending_frame = currMovieData.getNumRecords() - 1;
			if (new_drag_selection_ending_frame >= 0 && new_drag_selection_ending_frame != dragSelectionEndingFrame)
			{
				// change Selection shape
				if (new_drag_selection_ending_frame >= dragSelectionStartingFrame)
				{
					// selecting from upper to lower
					if (dragSelectionEndingFrame < dragSelectionStartingFrame)
					{
						selection->clearRegionOfRowsSelection(dragSelectionEndingFrame, dragSelectionStartingFrame);
						selection->setRegionOfRowsSelection(dragSelectionStartingFrame, new_drag_selection_ending_frame + 1);
					} else	// both ending_frame and new_ending_frame are >= starting_frame
					{
						if (dragSelectionEndingFrame > new_drag_selection_ending_frame)
							selection->clearRegionOfRowsSelection(new_drag_selection_ending_frame + 1, dragSelectionEndingFrame + 1);
						else
							selection->setRegionOfRowsSelection(dragSelectionEndingFrame + 1, new_drag_selection_ending_frame + 1);
					}
				} else
				{
					// selecting from lower to upper
					if (dragSelectionEndingFrame > dragSelectionStartingFrame)
					{
						selection->clearRegionOfRowsSelection(dragSelectionStartingFrame + 1, dragSelectionEndingFrame + 1);
						selection->setRegionOfRowsSelection(new_drag_selection_ending_frame, dragSelectionStartingFrame);
					} else	// both ending_frame and new_ending_frame are <= starting_frame
					{
						if (dragSelectionEndingFrame < new_drag_selection_ending_frame)
							selection->clearRegionOfRowsSelection(dragSelectionEndingFrame, new_drag_selection_ending_frame);
						else
							selection->setRegionOfRowsSelection(new_drag_selection_ending_frame, dragSelectionEndingFrame);
					}
				}
				dragSelectionEndingFrame = new_drag_selection_ending_frame;
			}
			break;
		}
		case DRAG_MODE_DESELECTION:
		{
			int new_drag_selection_ending_frame = realRowUnderMouse;
			// if trying to deselect above Piano Roll, deselect from frame 0
			if (new_drag_selection_ending_frame < 0)
				new_drag_selection_ending_frame = 0;
			else if (new_drag_selection_ending_frame >= currMovieData.getNumRecords())
				new_drag_selection_ending_frame = currMovieData.getNumRecords() - 1;
			if (new_drag_selection_ending_frame >= 0 && new_drag_selection_ending_frame != dragSelectionEndingFrame)
			{
				// change Deselection shape
				if (new_drag_selection_ending_frame >= dragSelectionStartingFrame)
				{
					// deselecting from upper to lower
					selection->clearRegionOfRowsSelection(dragSelectionStartingFrame, new_drag_selection_ending_frame + 1);
				}
				else
				{
					// deselecting from lower to upper
					selection->clearRegionOfRowsSelection(new_drag_selection_ending_frame, dragSelectionStartingFrame + 1);
				}
				dragSelectionEndingFrame = new_drag_selection_ending_frame;
			}
			break;
		}
	}
}
//----------------------------------------------------------------------------
void QPianoRoll::handleColumnSet(int column, bool altPressed)
{
	if (column == COLUMN_FRAMENUM || column == COLUMN_FRAMENUM2)
	{
		// user clicked on "Frame#" - apply ColumnSet to Markers
		if (altPressed)
		{
			if (parent->handleColumnSetUsingPattern())
			{
				setLightInHeaderColumn(COLUMN_FRAMENUM, HEADER_LIGHT_MAX);
			}
		}
		else
		{
			if (parent->handleColumnSet())
			{
				setLightInHeaderColumn(COLUMN_FRAMENUM, HEADER_LIGHT_MAX);
			}
		}
	}
	else
	{
		// user clicked on Input column - apply ColumnSet to Input
		int joy = (column - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS;
		int button = (column - COLUMN_JOYPAD1_A) % NUM_JOYPAD_BUTTONS;
		if (altPressed)
		{
			if (parent->handleInputColumnSetUsingPattern(joy, button))
			{
				setLightInHeaderColumn(column, HEADER_LIGHT_MAX);
			}
		}
		else
		{
			if (parent->handleInputColumnSet(joy, button))
			{
				setLightInHeaderColumn(column, HEADER_LIGHT_MAX);
			}
		}
	}
}
//----------------------------------------------------------------------------
void QPianoRoll::periodicUpdate(void)
{
	// scroll Piano Roll if user is dragging cursor outside
	if ( (dragMode != DRAG_MODE_NONE) || rightButtonDragMode)
	{
		int x, y, line, col, scroll_up_threshold, scroll_down_threshold;
		QPoint p, c;

		x = mouse_x;
		y = mouse_y;

		p.setX(x);
		p.setY(y);

		c = convPixToCursor(p);

		if ( c.y() >= 0 )
		{
			line = lineOffset + c.y();
		}
		else
		{
			line = lineOffset;
		}
		col =  calcColumn( c.x() );

		rowUnderMouse = realRowUnderMouse = line;
		columnUnderMouse = col;

		scroll_up_threshold = pxLineSpacing;
		scroll_down_threshold = (viewHeight - pxLineSpacing/2);

		//if (dragMode != DRAG_MODE_MARKER)		// in DRAG_MODE_MARKER user can't scroll Piano Roll horizontally
		//{
		//	if (p.x < DRAG_SCROLLING_BORDER_SIZE)
		//		scroll_dx = p.x - DRAG_SCROLLING_BORDER_SIZE;
		//	else if (p.x > (wrect.right - wrect.left - DRAG_SCROLLING_BORDER_SIZE))
		//		scroll_dx = p.x - (wrect.right - wrect.left - DRAG_SCROLLING_BORDER_SIZE);
		//}
		if (y < scroll_up_threshold )
		{
			scroll_y += (scroll_up_threshold - y);
			
			if ( scroll_y > pxLineSpacing )
			{
				int d, v = vbar->value();
				
				d = scroll_y / pxLineSpacing;

				v += d; scroll_y = 0;

				if ( v > maxLineOffset )
				{
					v = maxLineOffset;
				}
				vbar->setValue(v);
			}
		}
		else if (y > scroll_down_threshold)
		{
			scroll_y += (scroll_down_threshold - y);

			if ( scroll_y < -pxLineSpacing )
			{
				int d, v = vbar->value();
				
				d = scroll_y / pxLineSpacing;

				v += d; scroll_y = 0;

				if ( v < 0 )
				{
					v = 0;
				}
				vbar->setValue(v);
			}
		}
	}
	else
	{
		scroll_x = scroll_y = 0;
	}

	updateDrag();

	// once per 40 milliseconds update colors alpha in the Header
	if (clock() > nextHeaderUpdateTime)
	{
		nextHeaderUpdateTime = clock() + HEADER_LIGHT_UPDATE_TICK;
		bool changes_made = false;
		int light_value = 0;
		// 1 - update Frame# columns' heads
		//if (GetAsyncKeyState(VK_MENU) & 0x8000) light_value = HEADER_LIGHT_HOLD; else
		if (dragMode == DRAG_MODE_NONE && (headerItemUnderMouse == COLUMN_FRAMENUM || headerItemUnderMouse == COLUMN_FRAMENUM2))
		{
			light_value = (selection->getCurrentRowsSelectionSize() > 0) ? HEADER_LIGHT_MOUSEOVER_SEL : HEADER_LIGHT_MOUSEOVER;
		}
		if (headerColors[COLUMN_FRAMENUM] < light_value)
		{
			headerColors[COLUMN_FRAMENUM]++;
			changes_made = true;
		}
		else if (headerColors[COLUMN_FRAMENUM] > light_value)
		{
			headerColors[COLUMN_FRAMENUM]--;
			changes_made = true;
		}
		headerColors[COLUMN_FRAMENUM2] = headerColors[COLUMN_FRAMENUM];
		// 2 - update Input columns' heads
		int i = numColumns-1;
		if (i == COLUMN_FRAMENUM2) i--;
		for (; i >= COLUMN_JOYPAD1_A; i--)
		{
			light_value = 0;
			if (recorder->currentJoypadData[(i - COLUMN_JOYPAD1_A) / NUM_JOYPAD_BUTTONS] & (1 << ((i - COLUMN_JOYPAD1_A) % NUM_JOYPAD_BUTTONS)))
			{
				light_value = HEADER_LIGHT_HOLD;
			}
			else if (dragMode == DRAG_MODE_NONE && headerItemUnderMouse == i)
			{
				light_value = (selection->getCurrentRowsSelectionSize() > 0) ? HEADER_LIGHT_MOUSEOVER_SEL : HEADER_LIGHT_MOUSEOVER;
			}

			if (headerColors[i] < light_value)
			{
				headerColors[i]++;
				changes_made = true;
			}
			else if (headerColors[i] > light_value)
			{
				headerColors[i]--;
				changes_made = true;
			}
		}
		// 3 - redraw
		if (changes_made)
		{
			update();
		}
	}

}
//----------------------------------------------------------------------------
void QPianoRoll::setLightInHeaderColumn(int column, int level)
{
	if (column < COLUMN_FRAMENUM || column >= numColumns || level < 0 || level > HEADER_LIGHT_MAX)
	{
		return;
	}

	if (headerColors[column] != level)
	{
		headerColors[column] = level;
		//redrawHeader();
		nextHeaderUpdateTime = clock() + HEADER_LIGHT_UPDATE_TICK;
	}
}
//----------------------------------------------------------------------------
void QPianoRoll::followSelection(void)
{
	RowsSelection* current_selection = selection->getCopyOfCurrentRowsSelection();
	if (current_selection->size() == 0) return;

	int list_items = viewLines - 1;
	int selection_start = *current_selection->begin();
	int selection_end = *current_selection->rbegin();
	int selection_items = 1 + selection_end - selection_start;
	
	if (selection_items <= list_items)
	{
		// selected region can fit in screen
		int lower_border = (list_items - selection_items) / 2;
		int upper_border = (list_items - selection_items) - lower_border;
		int index = selection_end + lower_border;
		if (index >= currMovieData.getNumRecords())
		{
			index = currMovieData.getNumRecords()-1;
		}
		ensureTheLineIsVisible(index);

		index = selection_start - upper_border;
		if (index < 0)
		{
			index = 0;
		}
		ensureTheLineIsVisible(index);
	}
	else
	{
		// selected region is too big to fit in screen
		// oh well, just center at selection_start
		centerListAroundLine(selection_start);
	}
}

//----------------------------------------------------------------------------
void QPianoRoll::followMarker(int markerID)
{
	if (markerID > 0)
	{
		int frame = markersManager->getMarkerFrameNumber(markerID);
		if (frame >= 0)
		{
			centerListAroundLine(frame);
		}
	}
	else
	{
		ensureTheLineIsVisible(0);
	}
}

//----------------------------------------------------------------------------
void QPianoRoll::followPlaybackCursor(void)
{
	centerListAroundLine(currFrameCounter);
}
//----------------------------------------------------------------------------
void QPianoRoll::followPlaybackCursorIfNeeded(bool followPauseframe)
{
	if (taseditorConfig->followPlaybackCursor)
	{
		if (playback->getPauseFrame() < 0)
		{
			ensureTheLineIsVisible( currFrameCounter );
		}
		else if (followPauseframe)
		{
			ensureTheLineIsVisible( playback->getPauseFrame() );
		}
	}
}

//----------------------------------------------------------------------------
void QPianoRoll::followPauseframe(void)
{
	if (playback->getPauseFrame() >= 0)
	{
		centerListAroundLine(playback->getPauseFrame());
	}
}
//----------------------------------------------------------------------------
void QPianoRoll::followUndoHint(void)
{
	int keyframe = history->getUndoHint();
	if (taseditorConfig->followUndoContext && keyframe >= 0)
	{
		if (!lineIsVisible(keyframe))
		{
			centerListAroundLine(keyframe);
		}
	}
}
//----------------------------------------------------------------------------
void QPianoRoll::centerListAroundLine(int rowIndex)
{
	int numItemsPerPage = viewLines - 1;
	int lowerBorder = (numItemsPerPage - 1) / 2;
	int upperBorder = (numItemsPerPage - 1) - lowerBorder;
	int index = rowIndex + lowerBorder;
	if (index >= currMovieData.getNumRecords())
	{
		index = currMovieData.getNumRecords()-1;
	}
	ensureTheLineIsVisible(index);

	index = rowIndex - upperBorder;
	if (index < 0)
	{
		index = 0;
	}
	ensureTheLineIsVisible(index);
}
//----------------------------------------------------------------------------

void QPianoRoll::startDraggingPlaybackCursor(void)
{
	if (dragMode == DRAG_MODE_NONE)
	{
		dragMode = DRAG_MODE_PLAYBACK;
		// call it once
		handlePlaybackCursorDragging();
	}
}
void QPianoRoll::setupMarkerDrag(void)
{
	if ( QApplication::mouseButtons() & Qt::LeftButton )
	{
		startDraggingMarker( mouse_x, mouse_y, rowUnderMouseAtPress, columnUnderMouseAtPress);
	}
	else
	{
		tasWin->lowerMarkerNote->setFocus();
	}
}

void QPianoRoll::startDraggingMarker(int mouseX, int mouseY, int rowIndex, int columnIndex)
{
	if (dragMode == DRAG_MODE_NONE)
	{
		QColor bgColor = (taseditorConfig->bindMarkersToInput) ? QColor( BINDMARKED_FRAMENUM_COLOR ) : QColor( MARKED_FRAMENUM_COLOR );

		QSize iconSize(pxWidthFrameCol, pxLineSpacing);
		//QPixmap pixmap( iconSize );
		//pixmap.fill(Qt::transparent);

		//QPainter painter(&pixmap);

		//if (painter.isActive())
		//{
		//	char txt[32];

		//	sprintf( txt, "%07i", rowIndex );
		//	font.setItalic(true);
		//	font.setBold(false);

		//	painter.setFont(font);
		//	//I want to make the title bar pasted on the content
		//	//But you can't get the image of the default title bar, just draw a rectangular box
		//	//If the external theme color is set, you need to change it
		//	QRect title_rect{0,0,pixmap.width(),pixmap.height()};
		//	painter.fillRect(title_rect,bgColor);
		//	painter.drawText(title_rect,Qt::AlignCenter, txt);
		//	painter.drawRect(pixmap.rect().adjusted(0,0,-1,-1));

		//	font.setItalic(false);
		//	font.setBold(true);
		//}
		//painter.end();

		//QMimeData *mime = new QMimeData;
		//mime->setText( QString("MARKER") );

		mkrDrag = new markerDragPopup(this);
		mkrDrag->resize( iconSize );
		mkrDrag->setInitialPosition( QCursor::pos() );

		mkrDrag->setBgColor( bgColor );
		mkrDrag->setRowIndex( rowIndex );

		font.setItalic(true);
		font.setBold(false);
		mkrDrag->setFont(font);
		font.setItalic(false);
		font.setBold(true);

		//mkrDrag->setPixmap(pixmap);

		//QDrag *drag = new QDrag(this);
		//drag->setMimeData(mime);
		//drag->setPixmap(pixmap);
		//drag->setHotSpot(QPoint(0,0));

		// start dragging the Marker
		dragMode = DRAG_MODE_MARKER;
		markerDragFrameNumber = rowIndex;
		markerDragCountdown = MARKER_DRAG_COUNTDOWN_MAX;
		setCursor( Qt::ClosedHandCursor );

		connect(mkrDrag, &QDialog::destroyed, this,[=]
		{
			if ( mkrDrag == sender() )
			{
				//printf("Drag Destroyed\n");
				mkrDrag = NULL;
			}
		});
		
		//Drag is released after the mouse bounces up, at this time to judge whether it is dragged to the outside
		//connect(drag, &QDrag::destroyed, this,[=]
		//{
		//	int line, col;
		//	fceuCriticalSection emuLock;
		//	QPoint pos = this->mapFromGlobal(QCursor::pos());
		//	QPoint c = convPixToCursor( pos );

		//	//printf("Drag Destroyed\n");

		//	mouse_x = pos.x();
		//	mouse_y = pos.y();

		//	if ( c.y() >= 0 )
		//	{
		//		line = lineOffset + c.y();
		//	}
		//	else
		//	{
		//		line = -1;
		//	}
		//	col  = calcColumn( pos.x() );

		//	rowUnderMouse = realRowUnderMouse = line;
		//	columnUnderMouse = col;
		//	finishDrag();
		//});
		//drag->exec(Qt::MoveAction);

		mkrDrag->show();

		update();
	}
}
void QPianoRoll::startSelectingDrag(int start_frame)
{
	if (dragMode == DRAG_MODE_NONE)
	{
		dragMode = DRAG_MODE_SELECTION;
		dragSelectionStartingFrame = start_frame;
		dragSelectionEndingFrame = dragSelectionStartingFrame;	// assuming that start_frame is already selected
	}
}
void QPianoRoll::startDeselectingDrag(int start_frame)
{
	if (dragMode == DRAG_MODE_NONE)
	{
		dragMode = DRAG_MODE_DESELECTION;
		dragSelectionStartingFrame = start_frame;
		dragSelectionEndingFrame = dragSelectionStartingFrame;	// assuming that start_frame is already deselected
	}
}

void QPianoRoll::handlePlaybackCursorDragging(void)
{
	int target_frame = realRowUnderMouse;
	if (target_frame < 0)
	{
		target_frame = 0;
	}
	if (currFrameCounter != target_frame)
	{
		playback->jump(target_frame);
	}
}

void QPianoRoll::finishDrag(void)
{
	switch (dragMode)
	{
		case DRAG_MODE_MARKER:
		{
			// place Marker here
			if (markersManager->getMarkerAtFrame(markerDragFrameNumber))
			{
				//POINT p = {0, 0};
				//GetCursorPos(&p);
				//int mouse_x = p.x, mouse_y = p.y;
				//ScreenToClient(hwndList, &p);
				//RECT wrect;
				//GetClientRect(hwndList, &wrect);
				if (mouse_x < 0 || mouse_x > viewWidth || mouse_y < 0 || mouse_y > viewHeight)
				{
					// user threw the Marker away
					markersManager->removeMarkerFromFrame(markerDragFrameNumber);
					//redrawRow(markerDragFrameNumber);
					history->registerMarkersChange(MODTYPE_MARKER_REMOVE, markerDragFrameNumber);
					selection->mustFindCurrentMarker = playback->mustFindCurrentMarker = true;
					// calculate vector of movement
					if ( mkrDrag )
					{
						mkrDrag->throwAway();
					}
				}
				else
				{
					if (rowUnderMouse >= 0 && (columnUnderMouse <= COLUMN_FRAMENUM || columnUnderMouse >= COLUMN_FRAMENUM2))
					{
						if (rowUnderMouse == markerDragFrameNumber)
						{
							// it was just double-click and release
							// if Selection points at dragged Marker, set focus to lower Note edit field
							int dragged_marker_id = markersManager->getMarkerAtFrame(markerDragFrameNumber);
							int selection_marker_id = markersManager->getMarkerAboveFrame(selection->getCurrentRowsSelectionBeginning());
							if (dragged_marker_id == selection_marker_id)
							{
								//SetFocus(selection.hwndSelectionMarkerEditField);
								// select all text in case user wants to overwrite it
								//SendMessage(selection.hwndSelectionMarkerEditField, EM_SETSEL, 0, -1); 
								tasWin->lowerMarkerNote->setFocus();
							}
						}
						else if (markersManager->getMarkerAtFrame(rowUnderMouse))
						{
							int dragged_marker_id = markersManager->getMarkerAtFrame(markerDragFrameNumber);
							int destination_marker_id = markersManager->getMarkerAtFrame(rowUnderMouse);
							// swap Notes of these Markers
							char dragged_marker_note[MAX_NOTE_LEN];
							strcpy(dragged_marker_note, markersManager->getNoteCopy(dragged_marker_id).c_str());
							if (strcmp(markersManager->getNoteCopy(destination_marker_id).c_str(), dragged_marker_note))
							{
								// notes are different, swap them
								markersManager->setNote(dragged_marker_id, markersManager->getNoteCopy(destination_marker_id).c_str());
								markersManager->setNote(destination_marker_id, dragged_marker_note);
								history->registerMarkersChange(MODTYPE_MARKER_SWAP, markerDragFrameNumber, rowUnderMouse);
								selection->mustFindCurrentMarker = playback->mustFindCurrentMarker = true;
								setLightInHeaderColumn(COLUMN_FRAMENUM, HEADER_LIGHT_MAX);
							}
						}
						else
						{
							// move Marker
							int new_marker_id = markersManager->setMarkerAtFrame(rowUnderMouse);
							if (new_marker_id)
							{
								markersManager->setNote(new_marker_id, markersManager->getNoteCopy(markersManager->getMarkerAtFrame(markerDragFrameNumber)).c_str());
								// and delete it from old frame
								markersManager->removeMarkerFromFrame(markerDragFrameNumber);
								history->registerMarkersChange(MODTYPE_MARKER_DRAG, markerDragFrameNumber, rowUnderMouse, markersManager->getNoteCopy(markersManager->getMarkerAtFrame(rowUnderMouse)).c_str());
								selection->mustFindCurrentMarker = playback->mustFindCurrentMarker = true;
								setLightInHeaderColumn(COLUMN_FRAMENUM, HEADER_LIGHT_MAX);
								//redrawRow(rowUnderMouse);
							}
						}
						if ( mkrDrag )
						{
							mkrDrag->dropAccept();
						}
					}
					else
					{
						if ( mkrDrag )
						{
							mkrDrag->dropAbort();
						}
					}

					//redrawRow(markerDragFrameNumber);
					//if (hwndMarkerDragBox)
					//{
					//	DestroyWindow(hwndMarkerDragBox);
					//	hwndMarkerDragBox = 0;
					//}
				}
			}
			else
			{
				// abort drag
				if ( mkrDrag )
				{
					mkrDrag->dropAbort();
				}
				//if (hwndMarkerDragBox)
				//{
				//	DestroyWindow(hwndMarkerDragBox);
				//	hwndMarkerDragBox = 0;
				//}
			}
			//if ( mkrDrag )
			//{
			//	mkrDrag->done(0);
			//	mkrDrag->deleteLater();
			//}
			setCursor( Qt::ArrowCursor );
		}
		break;
	}
	dragMode = DRAG_MODE_NONE;
	//mustCheckItemUnderMouse = true;
}
//----------------------------------------------------------------------------
void QPianoRoll::paintEvent(QPaintEvent *event)
{
	FCEU_CRITICAL_SECTION( emuLock );
	int x, y, row, nrow, lineNum;
	QPainter painter(this);
	QColor white(255,255,255), black(0,0,0), blkColor, rowTextColor, hdrGridColor;
	static const char *buttonNames[] = { "A", "B", "S", "T", "U", "D", "L", "R", NULL };
	char stmp[32];
	char rowIsSel=0;
	char rowSelArray[256];
	int numSelRows=0;
	QRect rect;

	font.setBold(true);
	painter.setFont(font);
	viewWidth  = event->rect().width();
	viewHeight = event->rect().height();

	nrow = (viewHeight / pxLineSpacing) + 1;

	if ( nrow < 1 ) nrow = 1;

	memset( rowSelArray, 0, nrow+1 );

	viewLines = nrow;

	maxLineOffset = currMovieData.records.size() - nrow + 2;

	vbar->setMinimum(0);
	vbar->setMaximum(maxLineOffset);

	if ( maxLineOffset < 0 )
	{
		vbar->hide();
		maxLineOffset = 0;
	}
	else
	{
		vbar->show();
	}

	if ( taseditorConfig->followPlaybackCursor )
	{
		lineOffset = vbar->value();

		if ( playbackCursorPos != currFrameCounter )
		{
			int lineOffsetLowerLim, lineOffsetUpperLim;

			playbackCursorPos = currFrameCounter;

			lineOffsetLowerLim = lineOffset;
			lineOffsetUpperLim = lineOffset + nrow - 2;

			if ( playbackCursorPos < lineOffsetLowerLim )
			{
				lineOffset = playbackCursorPos;
				vbar->setValue( lineOffset );
			}
			else if ( playbackCursorPos >= lineOffsetUpperLim )
			{
				lineOffset = playbackCursorPos - nrow + 3;
				if ( lineOffset < 0 )
				{
					lineOffset = 0;
				}
				vbar->setValue( lineOffset );
			}
		}
	}
	else
	{
		vbar->setValue( lineOffset );
	}

	if ( lineOffset < 0 )
	{
		lineOffset = 0;
	}
	if ( lineOffset > maxLineOffset )
	{
		lineOffset = maxLineOffset;
	}

	painter.fillRect( 0, 0, viewWidth, viewHeight, this->palette().color(QPalette::Window) );

	// Draw Title Bar
	x = -pxLineXScroll; y = 0;
	painter.fillRect( 0, 0, viewWidth, pxLineSpacing, windowColor );
	painter.setPen( black );

	//font.setBold(true);
	//painter.setFont(font);

	//x = -pxLineXScroll + pxFrameColX + (pxWidthFrameCol - 6*pxCharWidth) / 2;
	//painter.drawText( x, pxLineTextOfs, tr("Frame#") );
	
	rect = QRect( -pxLineXScroll + pxFrameColX, 0, pxWidthFrameCol, pxLineSpacing );
	painter.drawText( rect, Qt::AlignCenter, tr("Frame#") );

	//font.setBold(false);
	//painter.setFont(font);

	// Draw Grid
	painter.drawLine( -pxLineXScroll, 0, -pxLineXScroll, viewHeight );

	//x = pxFrameColX - pxLineXScroll;
	//painter.drawLine( x, 0, x, viewHeight );

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

		rowSelArray[row] = rowIsSel = selection->isRowSelected( lineNum );

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

		// Frame number column
		// font
		//if (markersManager.getMarkerAtFrame(lineNum))
		//	SelectObject(msg->nmcd.hdc, hMainListSelectFont);
		//else
		//	SelectObject(msg->nmcd.hdc, hMainListFont);
		// bg
		// frame number
		if (lineNum == history->getUndoHint())
		{
			// undo hint here
			if (markersManager->getMarkerAtFrame(lineNum) && (dragMode != DRAG_MODE_MARKER || markerDragFrameNumber != lineNum))
			{
				blkColor = (taseditorConfig->bindMarkersToInput) ? QColor( BINDMARKED_UNDOHINT_FRAMENUM_COLOR ) : QColor( MARKED_UNDOHINT_FRAMENUM_COLOR );
			}
			else
			{
				blkColor = QColor( UNDOHINT_FRAMENUM_COLOR );
			}
		}
		else if (lineNum == currFrameCounter || lineNum == (playback->getFlashingPauseFrame() - 1))
		{
			// this is current frame
			if (markersManager->getMarkerAtFrame(lineNum) && (dragMode != DRAG_MODE_MARKER || markerDragFrameNumber != lineNum))
			{
				blkColor = (taseditorConfig->bindMarkersToInput) ? QColor( CUR_BINDMARKED_FRAMENUM_COLOR ) : QColor( CUR_MARKED_FRAMENUM_COLOR );
			}
			else
			{
				blkColor = QColor( CUR_FRAMENUM_COLOR );
			}
		}
		else if (markersManager->getMarkerAtFrame(lineNum) && (dragMode != DRAG_MODE_MARKER || markerDragFrameNumber != lineNum))
		{
			// this is marked frame
			blkColor = (taseditorConfig->bindMarkersToInput) ? QColor( BINDMARKED_FRAMENUM_COLOR ) : QColor( MARKED_FRAMENUM_COLOR );
		}
		else if (lineNum < greenzone->getSize())
		{
			if (!greenzone->isSavestateEmpty(lineNum))
			{
				// the frame is normal Greenzone frame
				if (frame_lag == LAGGED_YES)
				{
					blkColor = QColor( LAG_FRAMENUM_COLOR );
				}
				else
				{
					blkColor = QColor( GREENZONE_FRAMENUM_COLOR );
				}
			}
			else if (!greenzone->isSavestateEmpty(lineNum & EVERY16TH)
				|| !greenzone->isSavestateEmpty(lineNum & EVERY8TH)
				|| !greenzone->isSavestateEmpty(lineNum & EVERY4TH)
				|| !greenzone->isSavestateEmpty(lineNum & EVERY2ND))
			{
				// the frame is in a gap (in Greenzone tail)
				if (frame_lag == LAGGED_YES)
				{
					blkColor = QColor( PALE_LAG_FRAMENUM_COLOR );
				}
				else
				{
					blkColor = QColor( PALE_GREENZONE_FRAMENUM_COLOR );
				}
			}
			else 
			{
				// the frame is above Greenzone tail
				if (frame_lag == LAGGED_YES)
				{
					blkColor = QColor( VERY_PALE_LAG_FRAMENUM_COLOR );
				}
				else if (frame_lag == LAGGED_NO)
				{
					blkColor = QColor( VERY_PALE_GREENZONE_FRAMENUM_COLOR );
				}
				else
				{
					blkColor = QColor( NORMAL_FRAMENUM_COLOR );
				}
			}
		}
		else
		{
			// the frame is below Greenzone head
			if (frame_lag == LAGGED_YES)
			{
				blkColor = QColor( VERY_PALE_LAG_FRAMENUM_COLOR );
			}
			else if (frame_lag == LAGGED_NO)
			{
				blkColor = QColor( VERY_PALE_GREENZONE_FRAMENUM_COLOR );
			}
			else
			{
				blkColor = QColor( NORMAL_FRAMENUM_COLOR );
			}
		}
		x = -pxLineXScroll + pxFrameColX;

		painter.fillRect( x, y, pxWidthFrameCol, pxLineSpacing, blkColor );

		// Selected Line
		if ( rowIsSel )
		{
			painter.fillRect( 0, y, viewWidth, pxLineSpacing, QColor( 10, 36, 106 ) );

			rowTextColor = QColor( 255, 255, 255 );

			numSelRows++;
		}
		else
		{
			rowTextColor = QColor( 0, 0, 0 );
		}
		painter.setPen( rowTextColor );

		for (int i=0; i<numCtlr; i++)
		{
			int ctlrOfs, btnOfs, hotChangeVal;
			data = currMovieData.records[ lineNum ].joysticks[i];

			x = pxFrameCtlX[i] - pxLineXScroll;

			ctlrOfs = i*8;

			for (int j=0; j<8; j++)
			{
				btnOfs = ctlrOfs+j;

				if (taseditorConfig->enableHotChanges)
				{
					hotChangeVal = history->getCurrentSnapshot().inputlog.getHotChangesInfo( lineNum, btnOfs );

					if ( !rowIsSel && (hotChangeVal >= 0) && (hotChangeVal < 16) )
					{
						painter.setPen( hotChangesColors[hotChangeVal] );
					}
					else
					{
						painter.setPen( rowTextColor );
					}
				}
				else
				{
					hotChangeVal = -1;
				}
				rect = QRect( x, y, pxWidthBtnCol, pxLineSpacing );

				if ( data & (0x01 << j) )
				{
					//painter.drawText( x + pxCharWidth, y+pxLineTextOfs, tr(buttonNames[j]) );
					painter.drawText( rect, Qt::AlignCenter, tr(buttonNames[j]) );
				}
				else if ( hotChangeVal > 0 )
				{
					//painter.drawText( x + pxCharWidth, y+pxLineTextOfs, tr("-") );
					painter.drawText( rect, Qt::AlignCenter, tr("-") );
				}
				x += pxWidthBtnCol;
			}
			//painter.drawLine( x, y, x, pxLineSpacing );
		}

		// Frame number column
		// font
		//if (markersManager.getMarkerAtFrame(lineNum))
		//	SelectObject(msg->nmcd.hdc, hMainListSelectFont);
		//else
		//	SelectObject(msg->nmcd.hdc, hMainListFont);
		// bg
		painter.setPen( rowTextColor );

		rect = QRect( -pxLineXScroll + pxFrameColX, y, pxWidthFrameCol, pxLineSpacing );

		//x = -pxLineXScroll + pxFrameColX + (pxWidthFrameCol - 7*pxCharWidth) / 2;

		sprintf( stmp, "%07i", lineNum );

		if (markersManager->getMarkerAtFrame(lineNum))
		{
			font.setItalic(true);
			font.setBold(false);
		}
		else
		{
			font.setBold(true);
			font.setItalic(false);
		}
		painter.setFont(font);
		//painter.drawText( x, y+pxLineTextOfs, tr(stmp) );
		painter.drawText( rect, Qt::AlignCenter, tr(stmp) );

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
				iImage |= BOOKMARKS_WITH_GREEN_ARROW;
			}
			else if (lineNum == currFrameCounter)
			{
				iImage |= BOOKMARKS_WITH_BLUE_ARROW;
			}
			else
			{
				iImage |= BOOKMARKS_WITH_NO_ARROW;
			}
		}

		if ( iImage >= 0 )
		{
			drawArrow( &painter, x, y, iImage );
		}

		y += pxLineSpacing;
	}

	int gridBlack = gridColor.black();
	hdrGridColor = gridColor;

	if ( gridBlack < 128 )
	{
		hdrGridColor = QColor(128,128,128);
	}

	// Draw Grid lines
	painter.setPen( QPen(gridColor,gridPixelWidth) );
	x = pxFrameColX - pxLineXScroll;
	painter.drawLine( x, 0, x, viewHeight );
	
	painter.setPen( QPen(hdrGridColor,gridPixelWidth) );
	painter.drawLine( x, 0, x, pxLineSpacing );

	font.setBold(true);
	painter.setFont(font);


	for (int i=0; i<numCtlr; i++)
	{
		x = pxFrameCtlX[i] - pxLineXScroll;

		for (int j=0; j<8; j++)
		{
			//painter.setPen( QColor( 128, 128, 128 ) );
			//painter.drawLine( x, 0, x, viewHeight ); x++;
			painter.setPen( QPen(gridColor,gridPixelWidth) );
			painter.drawLine( x, 0, x, viewHeight ); //x--;

			painter.setPen( QPen(hdrGridColor,gridPixelWidth) );
			painter.drawLine( x, 0, x, pxLineSpacing );

			rect = QRect( x, 0, pxWidthBtnCol, pxLineSpacing );
			painter.setPen( QPen(headerLightsColors[ headerColors[COLUMN_JOYPAD1_A + (i*8) + j] ],1) );
			//painter.drawText( x + pxCharWidth, pxLineTextOfs, tr(buttonNames[j]) );
			painter.drawText( rect, Qt::AlignCenter, tr(buttonNames[j]) );

			x += pxWidthBtnCol;
		}
		//painter.setPen( QColor( 128, 128, 128 ) );
		//painter.drawLine( x, 0, x, viewHeight ); x++;
		painter.setPen( QPen(gridColor,gridPixelWidth) );
		painter.drawLine( x, 0, x, viewHeight );

		painter.setPen( QPen(hdrGridColor,gridPixelWidth) );
		painter.drawLine( x, 0, x, pxLineSpacing );

	}
	painter.setPen( QPen(gridColor,gridPixelWidth) );

	y = 0;
	for (int i=0; i<nrow; i++)
	{
		painter.drawLine( 0, y, viewWidth, y );
		
		y += pxLineSpacing;
	}

	painter.setPen( QPen(hdrGridColor,gridPixelWidth) );
	painter.drawLine( 0, 0, viewWidth, 0 );
	painter.drawLine( 0, pxLineSpacing, viewWidth, pxLineSpacing );

	// Draw grid lines for selections
	if ( numSelRows > 0 )
	{
		int inv;
		QColor invGrid;

		inv = gridColor.black();

		if ( inv < 128 )
		{
			inv = 255 - inv;
		}

		invGrid.setRed( inv );
		invGrid.setGreen( inv );
		invGrid.setBlue( inv );

		painter.setPen( QPen(invGrid,gridPixelWidth) );

		y = pxLineSpacing;

		for (row=0; row<nrow; row++)
		{
			if ( rowSelArray[row] )
			{
				int yl = y + pxLineSpacing;

				x = pxFrameColX - pxLineXScroll;
				painter.drawLine( x, y, x, yl );

				for (int i=0; i<numCtlr; i++)
				{
					x = pxFrameCtlX[i] - pxLineXScroll;

					for (int j=0; j<8; j++)
					{
						painter.drawLine( x, y, x, yl );

						x += pxWidthBtnCol;
					}
				}
				painter.drawLine( x, y, x, yl );
				painter.drawLine( 0, y , viewWidth, y  );
				painter.drawLine( 0, yl, viewWidth, yl );
			}
			y += pxLineSpacing;
		}
		painter.setPen( QPen(gridColor,gridPixelWidth) );
	}

	font.setBold(false);
	painter.setFont(font);
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//---- Bookmark Preview Popup
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bookmarkPreviewPopup *bookmarkPreviewPopup::instance = 0;
//----------------------------------------------------------------------------
bookmarkPreviewPopup::bookmarkPreviewPopup( int index, QWidget *parent )
	: QDialog( parent, Qt::ToolTip )
{
	int p;
	QPoint pos;
	QVBoxLayout *vbox;
	uint32_t *pixBuf;
	uint32_t  pixel;
	QPixmap pixmap;

	if ( instance )
	{
		//instance->done(0);
		//instance->deleteLater();
		instance->actv = false;
		instance = 0;
	}
	instance = this;

	imageIndex = index;

	//qApp->installEventFilter(this);

	//FCEU_WRAPPER_LOCK();

	// retrieve info from the pointed bookmark's Markers
	int frame = bookmarks->bookmarksArray[index].snapshot.keyFrame;
	int markerID = markersManager->getMarkerAboveFrame(bookmarks->bookmarksArray[index].snapshot.markers, frame);

	screenShotRaster = (unsigned char *)malloc( SCREENSHOT_SIZE );

	if ( screenShotRaster == NULL )
	{
		printf("Error: Failed to allocate screenshot image memory\n");
	}
	// bookmarks.itemUnderMouse

	pixBuf = (uint32_t *)malloc( SCREENSHOT_SIZE * sizeof(uint32_t) );

	loadImage(index);

	p=0;
	for (int h=0; h<SCREENSHOT_HEIGHT; h++)
	{
		for (int w=0; w<SCREENSHOT_WIDTH; w++)
		{
			pixel = ModernDeemphColorMap( &screenShotRaster[p], screenShotRaster, 1 );
			pixBuf[p]  = 0xFF000000;
			pixBuf[p] |= (pixel & 0x000000FF) << 16;
			pixBuf[p] |= (pixel & 0x00FF0000) >> 16;
			pixBuf[p] |= (pixel & 0x0000FF00);
			p++;
		}
	}
	QImage img( (unsigned char*)pixBuf, SCREENSHOT_WIDTH, SCREENSHOT_HEIGHT, SCREENSHOT_WIDTH*4, QImage::Format_RGBA8888 );
	pixmap.convertFromImage( img );

	vbox = new QVBoxLayout();

	setLayout( vbox );

	imgLbl  = new QLabel();
	descLbl = new QLabel();

	imgLbl->setPixmap( pixmap );

	vbox->addWidget( imgLbl , 100 );
	vbox->addWidget( descLbl, 1 );

	descLbl->setText( tr(markersManager->getNoteCopy(bookmarks->bookmarksArray[index].snapshot.markers, markerID).c_str()) );

	resize( 256, 256 );

	if ( pixBuf )
	{
		free( pixBuf ); pixBuf = NULL;
	}

	pos = tasWin->getPreviewPopupCoordinates();

	pos.setX( pos.x() - 300 );

	move(pos);

	//FCEU_WRAPPER_UNLOCK();

	alpha = 0;
	actv  = true;

	setWindowOpacity(0.0f);

	timer = new QTimer(this);

	connect( timer, &QTimer::timeout, this, &bookmarkPreviewPopup::periodicUpdate );

	timer->start(33);

}
//----------------------------------------------------------------------------
bookmarkPreviewPopup::~bookmarkPreviewPopup( void )
{
	timer->stop();

	if ( screenShotRaster != NULL )
	{
		free( screenShotRaster ); screenShotRaster = NULL;
	}
	//printf("Popup Deleted\n");
}
//----------------------------------------------------------------------------
void bookmarkPreviewPopup::periodicUpdate(void)
{
	if ( actv )
	{
		if ( alpha < 255 )
		{
			alpha += 25;

			if ( alpha > 255 )
			{
				alpha = 255;
			}
			setWindowOpacity( alpha / 255.0f );

			update();
		}
	}
	else
	{
		if ( alpha > 0 )
		{
			alpha -= 25;

			if ( alpha < 0 )
			{
				alpha = 0;
			}
			setWindowOpacity( alpha / 255.0f );

			update();
		}
		else
		{
			if ( instance == this )
			{
				instance = NULL;
			}
			done(0);
			deleteLater();
		}
	}
}
//----------------------------------------------------------------------------
bookmarkPreviewPopup *bookmarkPreviewPopup::currentInstance(void)
{
	return instance;
}
//----------------------------------------------------------------------------
int bookmarkPreviewPopup::currentIndex(void)
{
	if ( instance )
	{
		return instance->imageIndex;
	}
	return -1;
}
//----------------------------------------------------------------------------
void bookmarkPreviewPopup::imageIndexChanged(int newIndex)
{
	FCEU_CRITICAL_SECTION(emuLock);
	//printf("newIndex:%i\n", newIndex );

	if ( newIndex >= 0 )
	{
		reloadImage(newIndex);
		actv = true;
	}
	else
	{
		actv = false;
	}

	//if ( instance == this )
	//{
	//	instance = NULL;
	//}
}
//----------------------------------------------------------------------------
int bookmarkPreviewPopup::loadImage(int index)
{
	// uncompress
	int ret = 0;
	uLongf destlen = SCREENSHOT_SIZE;
	int e = uncompress(screenShotRaster, &destlen, &bookmarks->bookmarksArray[index].savedScreenshot[0], bookmarks->bookmarksArray[index].savedScreenshot.size());
	if (e != Z_OK && e != Z_BUF_ERROR)
	{
		// error decompressing
		FCEU_printf("Error decompressing screenshot %d\n", index);
		// at least fill bitmap with zeros
		memset(screenShotRaster, 0, SCREENSHOT_SIZE);
		ret = -1;
	}
	return ret;
}
//----------------------------------------------------------------------------
int bookmarkPreviewPopup::reloadImage(int index)
{
	int p, ret = 0;
	uint32_t *pixBuf;
	uint32_t  pixel;
	QPixmap pixmap;

	if ( index == imageIndex )
	{	// no change
		return 0;
	}
	actv = true;
	imageIndex = index;

	// retrieve info from the pointed bookmark's Markers
	int frame = bookmarks->bookmarksArray[index].snapshot.keyFrame;
	int markerID = markersManager->getMarkerAboveFrame(bookmarks->bookmarksArray[index].snapshot.markers, frame);

	pixBuf = (uint32_t *)malloc( SCREENSHOT_SIZE * sizeof(uint32_t) );

	loadImage(index);

	p=0;
	for (int h=0; h<SCREENSHOT_HEIGHT; h++)
	{
		for (int w=0; w<SCREENSHOT_WIDTH; w++)
		{
			pixel = ModernDeemphColorMap( &screenShotRaster[p], screenShotRaster, 1 );
			pixBuf[p]  = 0xFF000000;
			pixBuf[p] |= (pixel & 0x000000FF) << 16;
			pixBuf[p] |= (pixel & 0x00FF0000) >> 16;
			pixBuf[p] |= (pixel & 0x0000FF00);
			p++;
		}
	}
	QImage img( (unsigned char*)pixBuf, SCREENSHOT_WIDTH, SCREENSHOT_HEIGHT, SCREENSHOT_WIDTH*4, QImage::Format_RGBA8888 );
	pixmap.convertFromImage( img );

	if ( pixBuf )
	{
		free( pixBuf ); pixBuf = NULL;
	}

	imgLbl->setPixmap( pixmap );

	descLbl->setText( tr(markersManager->getNoteCopy(bookmarks->bookmarksArray[index].snapshot.markers, markerID).c_str()) );

	update();

	return ret;
}
//----------------------------------------------------------------------------
//---- TAS Find Note Window
//----------------------------------------------------------------------------
TasFindNoteWindow::TasFindNoteWindow( QWidget *parent )
	: QDialog( parent, Qt::Window )
{
	QSettings  settings;
	QVBoxLayout *mainLayout, *vbox;
	QHBoxLayout *hbox, *hbox1;
	QGroupBox   *gbox;

	setWindowTitle( tr("Find Note") );

	mainLayout = new QVBoxLayout();
	hbox1      = new QHBoxLayout();
	hbox       = new QHBoxLayout();
	vbox       = new QVBoxLayout();

	setLayout( mainLayout );

	searchPattern = new QLineEdit();
	matchCase     = new QCheckBox( tr("Match Case") );
	up            = new QRadioButton( tr("Up") );
	down          = new QRadioButton( tr("Down") );
	nextBtn       = new QPushButton( tr("Next") );
	closeBtn      = new QPushButton( tr("Close") );
	gbox          = new QGroupBox( tr("Direction") );

	mainLayout->addWidget( searchPattern );
	mainLayout->addLayout( hbox1 );

	hbox1->addWidget( matchCase );
	hbox1->addWidget( gbox );
	hbox1->addLayout( vbox );

	gbox->setLayout( hbox );

	hbox->addWidget( up );
	hbox->addWidget( down );

	vbox->addWidget( nextBtn );
	vbox->addWidget( closeBtn );

	findWin = this;

	nextBtn->setDefault(true);

	matchCase->setChecked( taseditorConfig->findnoteMatchCase );
	up->setChecked( taseditorConfig->findnoteSearchUp );
	down->setChecked( !taseditorConfig->findnoteSearchUp );

	searchPattern->setText( QString(markersManager->findNoteString) );

	nextBtn->setEnabled( searchPattern->text().size() > 0 );

	connect( matchCase, SIGNAL(clicked(bool)), this, SLOT(matchCaseChanged(bool)) );
	connect( up       , SIGNAL(clicked(void)), this, SLOT(upDirectionSelected(void)) );
	connect( down     , SIGNAL(clicked(void)), this, SLOT(downDirectionSelected(void)) );
	connect( closeBtn , SIGNAL(clicked(void)), this, SLOT(closeWindow(void)) );
	connect( nextBtn  , SIGNAL(clicked(void)), this, SLOT(findNextClicked(void)) );

	connect( searchPattern, SIGNAL(textChanged(const QString &)), this, SLOT(searchPatternChanged(const QString &)) );

	// Restore Window Geometry
	restoreGeometry(settings.value("tasEditorFindDialog/geometry").toByteArray());
}
//----------------------------------------------------------------------------
TasFindNoteWindow::~TasFindNoteWindow(void)
{
	QSettings  settings;

	if ( findWin == this )
	{
		findWin = NULL;
	}

	// Save Window Geometry
	settings.setValue("tasEditorFindDialog/geometry", saveGeometry());
}
//----------------------------------------------------------------------------
void TasFindNoteWindow::closeEvent(QCloseEvent *event)
{
	done(0);
	deleteLater();
	event->accept();
}
//----------------------------------------------------------------------------
void TasFindNoteWindow::closeWindow(void)
{
	done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
void TasFindNoteWindow::matchCaseChanged(bool val)
{
	taseditorConfig->findnoteMatchCase = val;
}
//----------------------------------------------------------------------------
void TasFindNoteWindow::upDirectionSelected(void)
{
	taseditorConfig->findnoteSearchUp = true;
}
//----------------------------------------------------------------------------
void TasFindNoteWindow::downDirectionSelected(void)
{
	taseditorConfig->findnoteSearchUp = false;
}
//----------------------------------------------------------------------------
void TasFindNoteWindow::searchPatternChanged(const QString &s)
{
	nextBtn->setEnabled( s.size() > 0 );
}
//----------------------------------------------------------------------------
void TasFindNoteWindow::findNextClicked(void)
{

	if ( searchPattern->text().size() == 0 )
	{
		return;
	}
	strncpy( markersManager->findNoteString, searchPattern->text().toStdString().c_str(), MAX_NOTE_LEN );

	// scan frames from current Selection to the border
	int cur_marker = 0;
	bool result;
	int movie_size = currMovieData.getNumRecords();
	int current_frame = selection->getCurrentRowsSelectionBeginning();
	if ( (current_frame < 0) && taseditorConfig->findnoteSearchUp)
	{
		current_frame = movie_size;
	}
	while (true)
	{
		// move forward
		if (taseditorConfig->findnoteSearchUp)
		{
			current_frame--;
			if (current_frame < 0)
			{
				QMessageBox::information( this, tr("Find Note"), tr("Nothing was found!") );
				printf("Nothing was found\n");
				//MessageBox(taseditorWindow.hwndFindNote, "Nothing was found.", "Find Note", MB_OK);
				break;
			}
		}
		else
		{
			current_frame++;
			if (current_frame >= movie_size)
			{
				QMessageBox::information( this, tr("Find Note"), tr("Nothing was found!") );
				printf("Nothing was found\n");
				//MessageBox(taseditorWindow.hwndFindNote, "Nothing was found!", "Find Note", MB_OK);
				break;
			}
		}
		// scan marked frames
		cur_marker = markersManager->getMarkerAtFrame(current_frame);
		if (cur_marker)
		{
			QString haystack, needle;

			needle   = QString(markersManager->findNoteString);
			haystack = QString::fromStdString(markersManager->getNoteCopy(cur_marker));

			if (taseditorConfig->findnoteMatchCase)
			{
				result = haystack.indexOf( needle, 0, Qt::CaseSensitive ) >= 0;
				//result = (strstr(markersManager->getNoteCopy(cur_marker).c_str(), markersManager->findNoteString) != 0);
			}
			else
			{
				result = haystack.indexOf( needle, 0, Qt::CaseInsensitive ) >= 0;
//#ifdef WIN32
//				result = (StrStrI(markersManager->getNoteCopy(cur_marker).c_str(), markersManager->findNoteString) != 0);
//#else
//				result = (strcasestr(markersManager->getNoteCopy(cur_marker).c_str(), markersManager->findNoteString) != 0);
//#endif
			}
			if (result)
			{
				// found note containing searched string - jump there
				selection->jumpToFrame(current_frame);
				break;
			}
		}
	}
}
//----------------------------------------------------------------------------
//---- TAS Recent Project Menu Action
//----------------------------------------------------------------------------
TasRecentProjectAction::TasRecentProjectAction(QString desc, QWidget *parent)
	: QAction( desc, parent )
{
	path = desc.toStdString();
}
//----------------------------------------------------------------------------
TasRecentProjectAction::~TasRecentProjectAction(void)
{
	//printf("Recent TAS Project Menu Action Deleted\n");
}
//----------------------------------------------------------------------------
void TasRecentProjectAction::activateCB(void)
{
	//printf("Activate Recent TAS Project: %s \n", path.c_str() );

	if ( tasWin )
	{
		tasWin->loadProject( path.c_str() );
	}
}
//----------------------------------------------------------------------------
//---- Marker Drag
//----------------------------------------------------------------------------
markerDragPopup::markerDragPopup(QWidget *parent)
	: QDialog( parent, Qt::ToolTip )
{
	rowIndex = 0;
	alpha = 255;
	bgColor = QColor( 255,255,255 );
	liveCount = 30;

	qApp->installEventFilter(this);

	timer = new QTimer(this);

	connect( timer, &QTimer::timeout, this, &markerDragPopup::fadeAway );

	timer->start(33);

	released = false;
	thrownAway = false;
	dropAborted = false;
	dropAccepted = false;
}
//----------------------------------------------------------------------------
markerDragPopup::~markerDragPopup(void)
{

}
//----------------------------------------------------------------------------
void markerDragPopup::setRowIndex( int row )
{
	rowIndex = row;
}
//----------------------------------------------------------------------------
void markerDragPopup::setBgColor( QColor c )
{
	bgColor = c;
}
//----------------------------------------------------------------------------
void markerDragPopup::setInitialPosition( QPoint p )
{
	initialPos = p;

	move( initialPos );
}
//----------------------------------------------------------------------------
void markerDragPopup::throwAway(void)
{
	thrownAway = true;
}
//----------------------------------------------------------------------------
void markerDragPopup::dropAccept(void)
{
	dropAccepted = true;
}
//----------------------------------------------------------------------------
void markerDragPopup::dropAbort(void)
{
	dropAborted = true;
}
//----------------------------------------------------------------------------
void markerDragPopup::fadeAway(void)
{

	if ( released )
	{
		if ( thrownAway )
		{
			QPoint p = pos();
			//printf("Fade:%i\n", alpha);

			p.setY( p.y() + 2 );

			move(p);

			if ( alpha > 0 )
			{
				alpha -= 10;

				if ( alpha < 0 )
				{
					alpha = 0;
				}
			}
			else
			{
				done(0);
				deleteLater();
			}
			setWindowOpacity( alpha / 255.0f );

			update();
		}
		else if ( dropAborted )
		{
			QPoint p = pos();
			int vx, vy, vm = 10;

			vx = initialPos.x() - p.x();

			if ( vx < -vm )
			{
				vx = -vm;
			}
			else if ( vx > vm )
			{
				vx = vm;
			}

			vy = initialPos.y() - p.y();

			if ( vy < -vm )
			{
				vy = -vm;
			}
			else if ( vy > vm )
			{
				vy = vm;
			}

			p.setX( p.x() + vx );
			p.setY( p.y() + vy );

			if ( (vx == 0) && (vy == 0) )
			{
				done(0);
				deleteLater();
			}
			else
			{
				move(p);
			}
		}
		else if ( dropAccepted )
		{
			done(0);
			deleteLater();
		}
		else
		{
			if ( liveCount > 0 )
			{
				liveCount--;
			}
			if ( liveCount == 0 )
			{
				done(0);
				deleteLater();
			}
		}
	}
}
//----------------------------------------------------------------------------
void markerDragPopup::paintEvent(QPaintEvent *event)
{
	int w,h;
	QPainter painter(this);
	char txt[32];

	w = event->rect().width();
	h = event->rect().height();

	sprintf( txt, "%07i", rowIndex );

	//painter.setFont(font);
	//I want to make the title bar pasted on the content
	//But you can't get the image of the default title bar, just draw a rectangular box
	//If the external theme color is set, you need to change it
	QRect title_rect{0,0,w,h};
	painter.fillRect(title_rect,bgColor);
	painter.drawText(title_rect,Qt::AlignCenter, txt);
	//painter.drawRect(pixmap.rect().adjusted(0,0,-1,-1));
}
//----------------------------------------------------------------------------
bool markerDragPopup::eventFilter( QObject *obj, QEvent *event)
{
	//printf("Event:%i   %p\n", event->type(), obj);
	switch (event->type() )
	{
		case QEvent::MouseMove:
		{
			if ( !released )
			{
				move( QCursor::pos() );
			}
			break;
		}
		case QEvent::MouseButtonRelease:
		{
			released = true;
			break;
		}
		default:
			// Ignore
		break;
	}
	return false;
}
//----------------------------------------------------------------------------
//---- TAS Window Main Horizontal Splitter
//----------------------------------------------------------------------------
TasEditorSplitter::TasEditorSplitter( QWidget *parent )
	: QSplitter( Qt::Horizontal, parent )
{
	panelInitDone = false;
}
//----------------------------------------------------------------------------
TasEditorSplitter::~TasEditorSplitter(void)
{

}
//----------------------------------------------------------------------------
void TasEditorSplitter::resizeEvent(QResizeEvent *event)
{
       	int minWidth;
	//int widthDelta;
	QList<int> panelWidth;

	//printf("Panel Resize\n");
	if ( !panelInitDone )
	{
		QSplitter::resizeEvent(event);
		panelInitDone = true;
		return;
	}
	//widthDelta = event->size().width() - event->oldSize().width();

	panelWidth = sizes();


	//for (int i=0; i<panelWidth.count(); i++)
	//{
	//	printf("Panel %i: %i\n", i, panelWidth[i] );
	//}
	panelWidth[0] = event->size().width() - panelWidth[1] - handleWidth();
	//panelWidth[0] += widthDelta;

	minWidth = widget(0)->minimumWidth();

	if ( panelWidth[0] < minWidth )
	{
		panelWidth[0] = minWidth;
	}
	setSizes( panelWidth );
}
//----------------------------------------------------------------------------
