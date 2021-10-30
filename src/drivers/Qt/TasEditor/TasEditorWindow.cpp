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
void QPianoRoll::paintEvent(QPaintEvent *event)
{
	int nrow;
	QPainter painter(this);
	QColor white("white"), black("black");

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

	painter.fillRect( 0, 0, viewWidth, viewHeight, this->palette().color(QPalette::Window) );

	// Draw Title Bar
	painter.fillRect( 0, 0, viewWidth, pxLineSpacing, windowColor );
	painter.setPen( black );
	painter.drawRect( 0, 0, viewWidth-1, pxLineSpacing );
}
//----------------------------------------------------------------------------
