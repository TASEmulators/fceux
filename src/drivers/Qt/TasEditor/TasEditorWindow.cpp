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

#include <QDir>
#include <QPainter>
#include <QSettings>
#include <QHeaderView>
#include <QMessageBox>
#include <QFontMetrics>
#include <QFileDialog>
#include <QStandardPaths>
#include <QApplication>
#include <QGuiApplication>

#include "fceu.h"
#include "movie.h"
#include "driver.h"

#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/throttle.h"
#include "Qt/fceuWrapper.h"
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
enum PIANO_ROLL_COLUMNS
{
	COLUMN_ICONS,
	COLUMN_FRAMENUM,
	COLUMN_JOYPAD1_A,
	COLUMN_JOYPAD1_B,
	COLUMN_JOYPAD1_S,
	COLUMN_JOYPAD1_T,
	COLUMN_JOYPAD1_U,
	COLUMN_JOYPAD1_D,
	COLUMN_JOYPAD1_L,
	COLUMN_JOYPAD1_R,
	COLUMN_JOYPAD2_A,
	COLUMN_JOYPAD2_B,
	COLUMN_JOYPAD2_S,
	COLUMN_JOYPAD2_T,
	COLUMN_JOYPAD2_U,
	COLUMN_JOYPAD2_D,
	COLUMN_JOYPAD2_L,
	COLUMN_JOYPAD2_R,
	COLUMN_JOYPAD3_A,
	COLUMN_JOYPAD3_B,
	COLUMN_JOYPAD3_S,
	COLUMN_JOYPAD3_T,
	COLUMN_JOYPAD3_U,
	COLUMN_JOYPAD3_D,
	COLUMN_JOYPAD3_L,
	COLUMN_JOYPAD3_R,
	COLUMN_JOYPAD4_A,
	COLUMN_JOYPAD4_B,
	COLUMN_JOYPAD4_S,
	COLUMN_JOYPAD4_T,
	COLUMN_JOYPAD4_U,
	COLUMN_JOYPAD4_D,
	COLUMN_JOYPAD4_L,
	COLUMN_JOYPAD4_R,
	COLUMN_FRAMENUM2,

	TOTAL_COLUMNS
};

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
#define BOOKMARKS_WITH_BLUE_ARROW 20
#define BOOKMARKS_WITH_GREEN_ARROW 40
#define BLUE_ARROW_IMAGE_ID 60
#define GREEN_ARROW_IMAGE_ID 61
#define GREEN_BLUE_ARROW_IMAGE_ID 62

#define MARKER_DRAG_COUNTDOWN_MAX 14

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

	clipboard = QGuiApplication::clipboard();

	setWindowTitle("TAS Editor");

	resize(512, 512);

	mainLayout = new QVBoxLayout();
	mainHBox   = new QSplitter( Qt::Horizontal );

	initPatterns();
	buildPianoRollDisplay();
	buildSideControlPanel();

	mainHBox->addWidget( pianoRollContainerWidget );
	mainHBox->addWidget( controlPanelContainerWidget );
	mainLayout->addWidget(mainHBox);

	menuBar = buildMenuBar();

	setLayout(mainLayout);
	mainLayout->setMenuBar( menuBar );
	pianoRoll->setFocus();

	initModules();

	updateCheckedItems();

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
	QMenu       *fileMenu, *editMenu, *viewMenu,
		    *confMenu, *luaMenu,  *helpMenu;
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
	//connect(act, SIGNAL(triggered()), this, SLOT(createNewProject(void)) );

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

	// Config
	confMenu = menuBar->addMenu(tr("&Config"));

	// Config -> Project File Saving Options
	act = new QAction(tr("Project File Saving Options"), this);
	//act->setShortcut(QKeySequence(tr("Ctrl+N")));
	act->setStatusTip(tr("Project File Saving Options"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	//connect(act, SIGNAL(triggered()), this, SLOT(createNewProject(void)) );

	confMenu->addAction(act);

	// Config -> Set Max Undo Levels
	act = new QAction(tr("Set Max Undo Levels"), this);
	//act->setShortcut(QKeySequence(tr("Ctrl+N")));
	act->setStatusTip(tr("Set Max Undo History"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	//connect(act, SIGNAL(triggered()), this, SLOT(createNewProject(void)) );

	confMenu->addAction(act);

	// Config -> Set Greenzone Capacity
	act = new QAction(tr("Set Greenzone Capacity"), this);
	//act->setShortcut(QKeySequence(tr("Ctrl+N")));
	act->setStatusTip(tr("Set Greenzone Capacity"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	//connect(act, SIGNAL(triggered()), this, SLOT(createNewProject(void)) );

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
	//connect(act, SIGNAL(triggered()), this, SLOT(createNewProject(void)) );

	luaMenu->addAction(act);

	luaMenu->addSeparator();

	// Lua -> Auto Function
	autoLuaAct = act = new QAction(tr("Auto Function"), this);
	act->setCheckable(true);
	//act->setShortcut(QKeySequence(tr("Ctrl+N")));
	act->setStatusTip(tr("Auto Function"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	//connect(act, SIGNAL(triggered()), this, SLOT(createNewProject(void)) );

	luaMenu->addAction(act);

	// Help
	helpMenu = menuBar->addMenu(tr("&Help"));

	// Help -> Open TAS Editor Manual
	act = new QAction(tr("Open TAS Editor Manual"), this);
	//act->setShortcut(QKeySequence(tr("Ctrl+N")));
	act->setStatusTip(tr("Open TAS Editor Manual"));
	//act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	//connect(act, SIGNAL(triggered()), this, SLOT(createNewProject(void)) );

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
	//connect(act, SIGNAL(triggered()), this, SLOT(createNewProject(void)) );

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
	pianoRollVBar    = new QScrollBar( Qt::Vertical, this );
	pianoRollHBar    = new QScrollBar( Qt::Horizontal, this );
	upperMarkerLabel = new QLabel( tr("Marker 0") );
	lowerMarkerLabel = new QLabel( tr("Marker 0") );
	upperMarkerNote  = new UpperMarkerNoteEdit();
	lowerMarkerNote  = new LowerMarkerNoteEdit();

	pianoRollFrame->setLineWidth(2);
	pianoRollFrame->setMidLineWidth(1);
	//pianoRollFrame->setFrameShape( QFrame::StyledPanel );
	pianoRollFrame->setFrameShape( QFrame::Box );

	pianoRollVBar->setInvertedAppearance(true);
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
	QScrollArea *scrollArea;
	QTreeWidgetItem *item;

	ctlPanelMainVbox = new QVBoxLayout();

	playbackGBox  = new QGroupBox( tr("Playback") );
	recorderGBox  = new QGroupBox( tr("Recorder") );
	splicerGBox   = new QGroupBox( tr("Splicer") );
	//luaGBox       = new QGroupBox( tr("Lua") );
	historyGBox   = new QGroupBox( tr("History") );
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
	
	scrollArea = new QScrollArea();
	scrollArea->setWidgetResizable(false);
	scrollArea->setSizeAdjustPolicy( QAbstractScrollArea::AdjustToContents );
	scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	scrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	scrollArea->setMinimumSize( QSize( 128, 128 ) );
	scrollArea->setWidget( &bookmarks );

	bkmkBrnchStack = new QTabWidget();
	bkmkBrnchStack->addTab( scrollArea, tr("Bookmarks") );
	bkmkBrnchStack->addTab( &branches , tr("Branches")  );

	taseditorConfig.displayBranchesTree = 0;

	vbox = new QVBoxLayout();
	vbox->addWidget( bkmkBrnchStack );
	bbFrame->setLayout( vbox );

	vbox = new QVBoxLayout();
	vbox->addWidget( histTree );
	historyGBox->setLayout( vbox );

	ctlPanelMainVbox->addWidget( playbackGBox  );
	ctlPanelMainVbox->addWidget( recorderGBox  );
	ctlPanelMainVbox->addWidget( splicerGBox   );
	//ctlPanelMainVbox->addWidget( luaGBox       );
	ctlPanelMainVbox->addWidget( bbFrame       );
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

	shortcut = new QShortcut( QKeySequence("Ctrl+Up"), this);
	connect( shortcut, SIGNAL(activated(void)), this, SLOT(scrollSelectionUpOne(void)) );

	shortcut = new QShortcut( QKeySequence("Ctrl+Down"), this);
	connect( shortcut, SIGNAL(activated(void)), this, SLOT(scrollSelectionDnOne(void)) );

	connect( histTree, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(histTreeItemActivated(QTreeWidgetItem*,int) ) );
	connect( histTree, SIGNAL(itemActivated(QTreeWidgetItem*,int)), this, SLOT(histTreeItemActivated(QTreeWidgetItem*,int) ) );

	connect( bkmkBrnchStack, SIGNAL(currentChanged(int)), this, SLOT(tabViewChanged(int) ) );
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
void TasEditorWindow::updateHistoryItems(void)
{
	int i, cursorPos;
	QTreeWidgetItem *item;
	const char *txt;

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
void TasEditorWindow::editUndoCB(void)
{
	history.undo();
}
//----------------------------------------------------------------------------
void TasEditorWindow::editRedoCB(void)
{
	history.redo();
}
//----------------------------------------------------------------------------
void TasEditorWindow::editUndoSelCB(void)
{
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
	int dragMode = pianoRoll->getDragMode();

	if ( (dragMode != DRAG_MODE_SELECTION) && (dragMode != DRAG_MODE_DESELECTION) )
	{
		selection.clearAllRowsSelection();
	}
}
//----------------------------------------------------------------------------
void TasEditorWindow::editSelectAll(void)
{
	int dragMode = pianoRoll->getDragMode();

	if ( (dragMode != DRAG_MODE_SELECTION) && (dragMode != DRAG_MODE_DESELECTION) )
	{
		selection.selectAllRows();
	}
}
//----------------------------------------------------------------------------
void TasEditorWindow::editSelBtwMkrs(void)
{
	int dragMode = pianoRoll->getDragMode();

	if ( (dragMode != DRAG_MODE_SELECTION) && (dragMode != DRAG_MODE_DESELECTION) )
	{
		selection.selectAllRowsBetweenMarkers();
	}
}
//----------------------------------------------------------------------------
void TasEditorWindow::editReselectClipboard(void)
{
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
	splicer.cutSelectedInputToClipboard();
}
//----------------------------------------------------------------------------
void TasEditorWindow::editCopyCB(void)
{
	splicer.copySelectedInputToClipboard();
}
//----------------------------------------------------------------------------
void TasEditorWindow::editPasteCB(void)
{
	splicer.pasteInputFromClipboard();
}
//----------------------------------------------------------------------------
void TasEditorWindow::editPasteInsertCB(void)
{
	splicer.pasteInsertInputFromClipboard();
}
//----------------------------------------------------------------------------
void TasEditorWindow::editClearCB(void)
{
	splicer.clearSelectedFrames();
}
//----------------------------------------------------------------------------
void TasEditorWindow::editDeleteCB(void)
{
	splicer.deleteSelectedFrames();
}
//----------------------------------------------------------------------------
void TasEditorWindow::editCloneCB(void)
{
	splicer.cloneSelectedFrames();
}
//----------------------------------------------------------------------------
void TasEditorWindow::editInsertCB(void)
{
	splicer.insertSelectedFrames();
}
//----------------------------------------------------------------------------
void TasEditorWindow::editInsertNumFramesCB(void)
{
	splicer.insertNumberOfFrames();
}
//----------------------------------------------------------------------------
void TasEditorWindow::editTruncateMovieCB(void)
{
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
void TasEditorWindow::showToolTipsActChanged(bool val)
{
	taseditorConfig.tooltipsEnabled = val;
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
// ----------------------------------------------------------------------------------------------
void TasEditorWindow::scrollSelectionUpOne(void)
{
	int dragMode = pianoRoll->getDragMode();

	fceuWrapperLock();

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
	fceuWrapperUnLock();
}
// ----------------------------------------------------------------------------------------------
void TasEditorWindow::scrollSelectionDnOne(void)
{
	int dragMode = pianoRoll->getDragMode();

	fceuWrapperLock();

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
	fceuWrapperUnLock();
}
// ----------------------------------------------------------------------------------------------
void TasEditorWindow::histTreeItemActivated(QTreeWidgetItem *item, int col)
{
	int row = histTree->indexOfTopLevelItem(item);

	if ( row < 0 )
	{
		return;
	}
	history.handleSingleClick(row);
}
// ----------------------------------------------------------------------------------------------
void TasEditorWindow::tabViewChanged(int idx)
{
	taseditorConfig.displayBranchesTree = (idx == 1);
	bookmarks.redrawBookmarksSectionCaption();
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
		return;

	int pattern_offset = 0, current_pattern = taseditorConfig.currentPattern;
	bool changes_made = false;
	bool value;

	for (int i = start; i <= end; ++i)
	{
		// skip lag frames
		if (taseditorConfig.autofirePatternSkipsLag && greenzone.lagLog.getLagInfoAtFrame(i) == LAGGED_YES)
			continue;
		value = (patterns[current_pattern][pattern_offset] != 0);
		if (currMovieData.records[i].checkBit(joy, button) != value)
		{
			changes_made = true;
			currMovieData.records[i].setBitValue(joy, button, value);
		}
		pattern_offset++;
		if (pattern_offset >= (int)patterns[current_pattern].size())
			pattern_offset -= patterns[current_pattern].size();
	}
	if (changes_made)
		greenzone.invalidateAndUpdatePlayback(history.registerChanges(MODTYPE_PATTERN, start, end, 0, patternsNames[current_pattern].c_str(), consecutivenessTag));
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
					pianoRoll->update();
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
				pianoRoll->update();
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
					pianoRoll->update();
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
				pianoRoll->update();
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
	this->setFocusPolicy(Qt::StrongFocus);
	this->setMouseTracking(true);
	this->setPalette(pal);

	numCtlr = 2;
	calcFontData();

	vbar = NULL;
	hbar = NULL;

	lineOffset = 0;
	maxLineOffset = 0;
	dragMode = DRAG_MODE_NONE;
	dragSelectionStartingFrame = 0;
	dragSelectionEndingFrame = 0;
	realRowUnderMouse = -1;
	markerDragFrameNumber = 0;
	markerDragCountdown = 0;
	drawingStartTimestamp = 0;
	mouse_x = mouse_y = -1;
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
	lineOffset = maxLineOffset - val;

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
bool QPianoRoll::lineIsVisible( int lineNum )
{
	int lineEnd = lineOffset + viewLines;

	return ( (lineNum >= lineOffset) && (lineNum < lineEnd) );
}
//----------------------------------------------------------------------------
void QPianoRoll::ensureTheLineIsVisible( int lineNum )
{
	if ( !lineIsVisible( lineNum ) )
	{
		lineOffset = lineNum - 3;

		if ( lineOffset < 0 )
		{
			lineOffset = 0;
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

	maxLineOffset = currMovieData.records.size() - viewLines + 5;

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
	vbar->setPageStep( (3*viewLines)/4 );

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
	fceuCriticalSection emuLock;
	int col, line, row_index, column_index, kbModifiers, alt_pressed;
	QPoint c = convPixToCursor( event->pos() );

	mouse_x = event->pos().x();
	mouse_y = event->pos().y();

	line = lineOffset + c.y();
	col  = calcColumn( event->pos().x() );

	row_index = line;
	rowUnderMouse = realRowUnderMouse = line;
	columnUnderMouse = column_index = col;

	kbModifiers = QApplication::keyboardModifiers();
	alt_pressed = (kbModifiers & Qt::AltModifier) ? 1 : 0;

	if ( event->button() == Qt::LeftButton )
	{
		if ( (col == COLUMN_FRAMENUM) || (col == COLUMN_FRAMENUM2) )
		{
			handleColumnSet( col, alt_pressed );
		}
	}
}
//----------------------------------------------------------------------------
void QPianoRoll::mousePressEvent(QMouseEvent * event)
{
	fceuCriticalSection emuLock;
	int col, line, row_index, column_index, kbModifiers, alt_pressed;
	QPoint c = convPixToCursor( event->pos() );

	mouse_x = event->pos().x();
	mouse_y = event->pos().y();

	line = lineOffset + c.y();
	col  = calcColumn( event->pos().x() );

	row_index = line;
	rowUnderMouse = realRowUnderMouse = line;
	columnUnderMouse = column_index = col;

	kbModifiers = QApplication::keyboardModifiers();
	alt_pressed = (kbModifiers & Qt::AltModifier) ? 1 : 0;

	printf("Mouse Button Pressed: 0x%x (%i,%i)\n", event->button(), c.x(), c.y() );
	
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
			if (row_index >= 0)
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
						if (currMovieData.records[row_index].checkBit(joy, button))
						{
							dragMode = DRAG_MODE_SET;
						}
						else
						{
							dragMode = DRAG_MODE_UNSET;
						}
						//pianoRoll.drawingLastX = GET_X_LPARAM(lParam) + GetScrollPos(pianoRoll.hwndList, SB_HORZ);
						//pianoRoll.drawingLastY = GET_Y_LPARAM(lParam) + GetScrollPos(pianoRoll.hwndList, SB_VERT) * pianoRoll.listRowHeight;
					}
					else
					{
						dragMode = DRAG_MODE_OBSERVE;
					}
				}
			}
		}
		//updateDrag();
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
	fceuCriticalSection emuLock;
	int col, line;
	QPoint c = convPixToCursor( event->pos() );

	mouse_x = event->pos().x();
	mouse_y = event->pos().y();

	line = lineOffset + c.y();
	col  = calcColumn( event->pos().x() );

	rowUnderMouse = realRowUnderMouse = line;
	columnUnderMouse = col;

	printf("Mouse Button Released: 0x%x (%i,%i)\n", event->button(), c.x(), c.y() );
	
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
		//rightButtonDragMode = true;
	}
}
//----------------------------------------------------------------------------
void QPianoRoll::mouseMoveEvent(QMouseEvent * event)
{
	fceuCriticalSection emuLock;
	int col, line;
	QPoint c = convPixToCursor( event->pos() );

	mouse_x = event->pos().x();
	mouse_y = event->pos().y();

	line = lineOffset + c.y();
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

	printf("PianoRoll Focus In\n");

	parent->pianoRollFrame->setStyleSheet("QFrame { border: 2px solid rgb(48,140,198); }");
}
//----------------------------------------------------------------------------
void QPianoRoll::focusOutEvent(QFocusEvent *event)
{
	QWidget::focusOutEvent(event);

	printf("PianoRoll Focus Out\n");

	parent->pianoRollFrame->setStyleSheet(NULL);
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
					//info.pt.x = p.x;
					//info.pt.y = p.y;
					//ListView_SubItemHitTest(hwndList, &info);
					row_index = rowUnderMouse;
					//if (row_index < 0)
					//{
					//	row_index = ListView_GetTopIndex(hwndList) + (info.pt.y - listTopMargin) / listRowHeight;
					//}
					// pad movie size if user tries to draw pattern below Piano Roll limit
					if (row_index >= currMovieData.getNumRecords())
					{
						currMovieData.insertEmpty(-1, row_index + 1 - currMovieData.getNumRecords());
					}
					column_index = columnUnderMouse;

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
				//double total_len = sqrt((double)(total_dx * total_dx + total_dy * total_dy));
				//int drawing_min_line_len = listRowHeight;		// = min(list_row_width, list_row_height) in pixels
				//for (double len = 0; len < total_len; len += drawing_min_line_len)
				//{
					// perform hit test
					//info.pt.x = p.x + (len / total_len) * total_dx;
					//info.pt.y = p.y + (len / total_len) * total_dy;
					//ListView_SubItemHitTest(hwndList, &info);
					//row_index = info.iItem;
					row_index = rowUnderMouse;
					//if (row_index < 0)
					//	row_index = ListView_GetTopIndex(hwndList) + (info.pt.y - listTopMargin) / listRowHeight;
					// pad movie size if user tries to draw below Piano Roll limit
					if (row_index >= currMovieData.getNumRecords())
					{
						currMovieData.insertEmpty(-1, row_index + 1 - currMovieData.getNumRecords());
					}
					column_index = columnUnderMouse;

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
				//}
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
			//drawingLastX = drawing_current_x;
			//drawingLastY = drawing_current_y;
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
					// deselecting from upper to lower
					selection->clearRegionOfRowsSelection(dragSelectionStartingFrame, new_drag_selection_ending_frame + 1);
				else
					// deselecting from lower to upper
					selection->clearRegionOfRowsSelection(new_drag_selection_ending_frame, dragSelectionStartingFrame + 1);
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
				//setLightInHeaderColumn(COLUMN_FRAMENUM, HEADER_LIGHT_MAX);
			}
		}
		else
		{
			if (parent->handleColumnSet())
			{
				//setLightInHeaderColumn(COLUMN_FRAMENUM, HEADER_LIGHT_MAX);
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
				//setLightInHeaderColumn(column, HEADER_LIGHT_MAX);
			}
		}
		else
		{
			if (parent->handleInputColumnSet(joy, button))
			{
				//setLightInHeaderColumn(column, HEADER_LIGHT_MAX);
			}
		}
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
void QPianoRoll::startDraggingMarker(int mouseX, int mouseY, int rowIndex, int columnIndex)
{
	if (dragMode == DRAG_MODE_NONE)
	{
		// start dragging the Marker
		dragMode = DRAG_MODE_MARKER;
		markerDragFrameNumber = rowIndex;
		markerDragCountdown = MARKER_DRAG_COUNTDOWN_MAX;
		//RECT temp_rect;
		//if (ListView_GetSubItemRect(hwndList, rowIndex, columnIndex, LVIR_BOUNDS, &temp_rect))
		//{
		//	markerDragBoxDX = mouseX - temp_rect.left;
		//	markerDragBoxDY = mouseY - temp_rect.top;
		//} else
		//{
		//	markerDragBoxDX = 0;
		//	markerDragBoxDY = 0;
		//}
		//// redraw the row to show that Marker was lifted
		//redrawRow(rowIndex);
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
					//POINT p = {0, 0};
					//GetCursorPos(&p);
					//markerDragBoxDX = (mouse_x - markerDragBoxDX) - markerDragBoxX;
					//markerDragBoxDY = (mouse_y - markerDragBoxDY) - markerDragBoxY;
					//if (markerDragBoxDX || markerDragBoxDY)
					//{
					//	// limit max speed
					//	double marker_drag_box_speed = sqrt((double)(markerDragBoxDX * markerDragBoxDX + markerDragBoxDY * markerDragBoxDY));
					//	if (marker_drag_box_speed > MARKER_DRAG_MAX_SPEED)
					//	{
					//		markerDragBoxDX *= MARKER_DRAG_MAX_SPEED / marker_drag_box_speed;
					//		markerDragBoxDY *= MARKER_DRAG_MAX_SPEED / marker_drag_box_speed;
					//	}
					//}
					//markerDragCountdown = MARKER_DRAG_COUNTDOWN_MAX;
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
								//setLightInHeaderColumn(COLUMN_FRAMENUM, HEADER_LIGHT_MAX);
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
								//setLightInHeaderColumn(COLUMN_FRAMENUM, HEADER_LIGHT_MAX);
								//redrawRow(rowUnderMouse);
							}
						}
					}
					//redrawRow(markerDragFrameNumber);
					//if (hwndMarkerDragBox)
					//{
					//	DestroyWindow(hwndMarkerDragBox);
					//	hwndMarkerDragBox = 0;
					//}
				}
			} else
			{
				// abort drag
				//if (hwndMarkerDragBox)
				//{
				//	DestroyWindow(hwndMarkerDragBox);
				//	hwndMarkerDragBox = 0;
				//}
			}
			break;
		}
	}
	dragMode = DRAG_MODE_NONE;
	//mustCheckItemUnderMouse = true;
}
//----------------------------------------------------------------------------
void QPianoRoll::paintEvent(QPaintEvent *event)
{
	fceuCriticalSection emuLock;
	int x, y, row, nrow, lineNum;
	QPainter painter(this);
	QColor white(255,255,255), black(0,0,0), blkColor;
	static const char *buttonNames[] = { "A", "B", "S", "T", "U", "D", "L", "R", NULL };
	char stmp[32];
	char rowIsSel=0;

	painter.setFont(font);
	viewWidth  = event->rect().width();
	viewHeight = event->rect().height();

	nrow = (viewHeight / pxLineSpacing) + 1;

	if ( nrow < 1 ) nrow = 1;

	viewLines = nrow;

	maxLineOffset = currMovieData.records.size() - nrow + 5;

	if ( maxLineOffset < 0 )
	{
		vbar->hide();
		maxLineOffset = 0;
	}
	else
	{
		vbar->show();
	}

	lineOffset = maxLineOffset - vbar->value();

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

		rowIsSel = selection->isRowSelected( lineNum );

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

			painter.setPen( QColor( 255, 255, 255 ) );
		}
		else
		{
			painter.setPen( QColor(   0,   0,   0 ) );
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
			//painter.drawLine( x, y, x, pxLineSpacing );
		}

		// Frame number column
		// font
		//if (markersManager.getMarkerAtFrame(lineNum))
		//	SelectObject(msg->nmcd.hdc, hMainListSelectFont);
		//else
		//	SelectObject(msg->nmcd.hdc, hMainListFont);
		// bg
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

	x = pxFrameColX - pxLineXScroll;
	painter.drawLine( x, 0, x, viewHeight );
	
	for (int i=0; i<numCtlr; i++)
	{
		x = pxFrameCtlX[i] - pxLineXScroll;

		for (int j=0; j<8; j++)
		{
			painter.setPen( QColor( 128, 128, 128 ) );
			painter.drawLine( x, 0, x, viewHeight ); x++;
			painter.setPen( QColor(   0,   0,   0 ) );
			painter.drawLine( x, 0, x, viewHeight ); x--;

			painter.drawText( x + pxCharWidth, pxLineTextOfs, tr(buttonNames[j]) );

			x += pxWidthBtnCol;
		}
		painter.setPen( QColor( 128, 128, 128 ) );
		painter.drawLine( x, 0, x, viewHeight ); x++;
		painter.setPen( QColor(   0,   0,   0 ) );
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
