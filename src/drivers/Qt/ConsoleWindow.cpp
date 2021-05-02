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
// ConsoleWindow.cpp
//
#if defined(__linux__) || defined(__unix__)
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <pthread.h>
#endif

#include <fstream>
#include <iostream>
#include <cstdlib>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QDesktopServices>
#include <QStyleFactory>
#include <QApplication>
#include <QActionGroup>
#include <QShortcut>
#include <QUrl>

#include "../../fceu.h"
#include "../../fds.h"
#include "../../file.h"
#include "../../input.h"
#include "../../movie.h"
#include "../../version.h"

#ifdef _S9XLUA_H
#include "../../fceulua.h"
#endif

#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/ConsoleWindow.h"
#include "Qt/InputConf.h"
#include "Qt/GamePadConf.h"
#include "Qt/HotKeyConf.h"
#include "Qt/PaletteConf.h"
#include "Qt/PaletteEditor.h"
#include "Qt/GuiConf.h"
#include "Qt/AviRecord.h"
#include "Qt/MoviePlay.h"
#include "Qt/MovieOptions.h"
#include "Qt/TimingConf.h"
#include "Qt/FrameTimingStats.h"
#include "Qt/LuaControl.h"
#include "Qt/CheatsConf.h"
#include "Qt/GameGenie.h"
#include "Qt/HexEditor.h"
#include "Qt/TraceLogger.h"
#include "Qt/CodeDataLogger.h"
#include "Qt/ConsoleDebugger.h"
#include "Qt/ConsoleUtilities.h"
#include "Qt/ConsoleSoundConf.h"
#include "Qt/ConsoleVideoConf.h"
#include "Qt/MsgLogViewer.h"
#include "Qt/AboutWindow.h"
#include "Qt/fceuWrapper.h"
#include "Qt/ppuViewer.h"
#include "Qt/NameTableViewer.h"
#include "Qt/iNesHeaderEditor.h"
#include "Qt/RamWatch.h"
#include "Qt/RamSearch.h"
#include "Qt/keyscan.h"
#include "Qt/nes_shm.h"

consoleWin_t::consoleWin_t(QWidget *parent)
	: QMainWindow( parent )
{
	int opt, xWinSize = 256, yWinSize = 240;
	int use_SDL_video = false;
	int setFullScreen = false;

	//QString libpath = QLibraryInfo::location(QLibraryInfo::PluginsPath);
	//printf("LibPath: '%s'\n", libpath.toStdString().c_str() );

	QApplication::setStyle( new fceuStyle() );

	initHotKeys();

	createMainMenu();

	firstResize    = true;
	closeRequested = false;
	errorMsgValid  = false;
	viewport_GL    = NULL;
	viewport_SDL   = NULL;

	mainMenuEmuPauseSet   = false;
	mainMenuEmuWasPaused  = false;
	mainMenuPauseWhenActv = false;

	g_config->getOption( "SDL.PauseOnMainMenuAccess", &mainMenuPauseWhenActv );

	if ( use_SDL_video )
	{
		viewport_SDL = new ConsoleViewSDL_t(this);

		setCentralWidget(viewport_SDL);
	}
	else
	{
		viewport_GL = new ConsoleViewGL_t(this);

		setCentralWidget(viewport_GL);
	}
	setViewportAspect();

	setWindowTitle( tr(FCEU_NAME_AND_VERSION) );
	setWindowIcon(QIcon(":fceux1.png"));

	gameTimer  = new QTimer( this );
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
	mutex      = new QRecursiveMutex();
#else
	mutex      = new QMutex( QMutex::Recursive );
#endif
	emulatorThread = new emulatorThread_t(this);

	connect(emulatorThread, &QThread::finished, emulatorThread, &QObject::deleteLater);

	connect( gameTimer, &QTimer::timeout, this, &consoleWin_t::updatePeriodic );

	gameTimer->setTimerType( Qt::PreciseTimer );
	gameTimer->start( 8 ); // 120hz

	emulatorThread->start();

	g_config->getOption( "SDL.SetSchedParam", &opt );

	if ( opt )
	{
		#ifndef WIN32
		int policy, prio, nice;

		g_config->getOption( "SDL.GuiSchedPolicy", &policy );
		g_config->getOption( "SDL.GuiSchedPrioRt", &prio   );
		g_config->getOption( "SDL.GuiSchedNice"  , &nice   );

		setNicePriority( nice );

		setSchedParam( policy, prio );
		#endif
	}


	g_config->getOption( "SDL.WinSizeX", &xWinSize );
	g_config->getOption( "SDL.WinSizeY", &yWinSize );

	if ( (xWinSize >= 256) && (yWinSize >= 224) )
	{
		this->resize( xWinSize, yWinSize );
	}
	else
	{
		QSize reqSize = calcRequiredSize();

		// Since the height of menu is unknown until Qt has shows the window
		// Set the minimum viewport sizes to exactly what we need so that 
		// the window is resized appropriately. On the first resize event,
		// we will set the minimum viewport size back to 1x values that the
		// window can be shrunk by dragging lower corner.
		if ( viewport_GL != NULL )
		{
			viewport_GL->setMinimumSize( reqSize );
		}
		else if ( viewport_SDL != NULL )
		{
			viewport_SDL->setMinimumSize( reqSize );
		}
		//this->resize( reqSize );
	}

	g_config->getOption( "SDL.Fullscreen", &setFullScreen );
	g_config->setOption( "SDL.Fullscreen", 0 ); // Reset full screen config parameter to false so it is never saved this way

	if ( setFullScreen )
	{
		this->showFullScreen();
	}

	updateCounter = 0;
	recentRomMenuReset = false;

	// Viewport Cursor Type and Visibility
	loadCursor();

	// Create AVI Recording Disk Thread
	aviDiskThread = new AviRecordDiskThread_t(this);

}

consoleWin_t::~consoleWin_t(void)
{
	QSize w;
	QClipboard *clipboard;

	// Save window size and image scaling parameters at app exit.
	w = consoleWindow->size();

	if ( viewport_GL != NULL )
	{
		g_config->setOption( "SDL.XScale", viewport_GL->getScaleX() );
		g_config->setOption( "SDL.YScale", viewport_GL->getScaleY() );
	}
	else if ( viewport_SDL != NULL )
	{
		g_config->setOption( "SDL.XScale", viewport_SDL->getScaleX() );
		g_config->setOption( "SDL.YScale", viewport_SDL->getScaleY() );
	}
	g_config->setOption( "SDL.WinSizeX", w.width() );
	g_config->setOption( "SDL.WinSizeY", w.height() );
	g_config->save();

	// Signal Emulator Thread to Stop
	nes_shm->runEmulator = 0;

	gameTimer->stop(); 

	closeGamePadConfWindow();

	//printf("Thread Finished: %i \n", gameThread->isFinished() );
	emulatorThread->quit();
	emulatorThread->wait( 1000 );

	aviDiskThread->requestInterruption();
	aviDiskThread->quit();
	aviDiskThread->wait( 10000 );

	fceuWrapperLock();
	fceuWrapperClose();
	fceuWrapperUnLock();

	if ( viewport_GL != NULL )
	{
		delete viewport_GL; viewport_GL = NULL;
	}
	if ( viewport_SDL != NULL )
	{
		delete viewport_SDL; viewport_SDL = NULL;
	}
	delete mutex;

	// LoadGame() checks for an IP and if it finds one begins a network session
	// clear the NetworkIP field so this doesn't happen unintentionally
	g_config->setOption ("SDL.NetworkIP", "");
	g_config->save ();

	// Clear Clipboard Contents on Program Exit
	clipboard = QGuiApplication::clipboard();

	if ( clipboard->ownsClipboard() )
	{
		clipboard->clear( QClipboard::Clipboard );
	}
	if ( clipboard->ownsSelection() )
	{
		clipboard->clear( QClipboard::Selection );
	}

	clearRomList();

	if ( this == consoleWindow )
	{
		consoleWindow = NULL;
	}

}

QSize consoleWin_t::calcRequiredSize(void)
{
	QSize out( GL_NES_WIDTH, GL_NES_HEIGHT );

	QSize w, v;
	double xscale = 1.0, yscale = 1.0, aspectRatio = 1.0;
	int texture_width = GL_NES_WIDTH;
	int texture_height = GL_NES_HEIGHT;
	int l=0, r=texture_width;
	int t=0, b=texture_height;
	int dw=0, dh=0, rw, rh;
	bool forceAspect = true;

	CalcVideoDimensions();

	texture_width  = nes_shm->video.ncol;
	texture_height = nes_shm->video.nrow;

	l=0, r=texture_width;
	t=0, b=texture_height;

	w = size();

	if ( viewport_GL )
	{
		v = viewport_GL->size();
		forceAspect = viewport_GL->getForceAspectOpt();
		aspectRatio = viewport_GL->getAspectRatio();
		xscale = viewport_GL->getScaleX();
		yscale = viewport_GL->getScaleY();
	}
	else if ( viewport_SDL )
	{
		v = viewport_SDL->size();
		forceAspect = viewport_SDL->getForceAspectOpt();
		aspectRatio = viewport_SDL->getAspectRatio();
		xscale = viewport_SDL->getScaleX();
		yscale = viewport_SDL->getScaleY();
	}

	dw = 0;
	dh = 0;

	if ( forceAspect )
	{
		yscale = xscale * (double)nes_shm->video.xyRatio;
	}
	rw=(int)((r-l)*xscale);
	rh=(int)((b-t)*yscale);

	//printf("view %i x %i \n", rw, rh );

	if ( forceAspect )
	{
		double rr;

		rr = (double)rh / (double)rw;

		if ( rr > aspectRatio )
		{
			rw = (int)( (((double)rh) / aspectRatio) + 0.50);
		}
		else
		{
			rh = (int)( (((double)rw) * aspectRatio) + 0.50);
		}
	}

	out.setWidth( rw + dw );
	out.setHeight( rh + dh );

	//printf("Win %i x %i \n", rw + dw, rh + dh );

	return out;
}

void consoleWin_t::setViewportAspect(void)
{
	int aspectSel;
	double x,y;

	g_config->getOption ("SDL.AspectSelect", &aspectSel);

	switch ( aspectSel )
	{
		default:
		case 0:
			x =  1.0; y = 1.0;
		break;
		case 1:
			x =  8.0; y = 7.0;
		break;
		case 2:
			x = 11.0; y = 8.0;
		break;
		case 3:
			x =  4.0; y = 3.0;
		break;
		case 4:
			x = 16.0; y = 9.0;
		break;
		case 5:
		{
			x = 1.0; y = 1.0;
		}
		break;
	}

	if ( viewport_GL )
	{
		viewport_GL->setAspectXY( x, y );
	}
	else if ( viewport_SDL )
	{
		viewport_SDL->setAspectXY( x, y );
	}
}

void consoleWin_t::setMenuAccessPauseEnable( bool enable )
{
	mainMenuPauseWhenActv = enable;
}

void consoleWin_t::loadCursor(void)
{
	int cursorVis;

	// Viewport Cursor Type and Visibility
	g_config->getOption("SDL.CursorVis", &cursorVis );

	if ( cursorVis )
	{
		int cursorType;

		g_config->getOption("SDL.CursorType", &cursorType );

		switch ( cursorType )
		{
			case 4:
			{
				QPixmap reticle(":/icons/reticle.png");

				setViewerCursor( QCursor(reticle.scaled(64,64)) );
			}
			break;
			case 3:
			{
				QPixmap reticle(":/icons/reticle.png");

				setViewerCursor( QCursor(reticle.scaled(32,32)) );
			}
			break;
			case 2:
				setViewerCursor( Qt::BlankCursor );
			break;
			case 1:
				setViewerCursor( Qt::CrossCursor );
			break;
			default:
			case 0:
				setViewerCursor( Qt::ArrowCursor );
			break;
		}
	}
	else
	{
		setViewerCursor( Qt::BlankCursor );
	}
}

void consoleWin_t::setViewerCursor( QCursor s )
{
	if ( viewport_GL )
	{
		viewport_GL->setCursor(s);
	}
	else if ( viewport_SDL )
	{
		viewport_SDL->setCursor(s);
	}
}

void consoleWin_t::setViewerCursor( Qt::CursorShape s )
{
	if ( viewport_GL )
	{
		viewport_GL->setCursor(s);
	}
	else if ( viewport_SDL )
	{
		viewport_SDL->setCursor(s);
	}
}

Qt::CursorShape consoleWin_t::getViewerCursor(void)
{
	Qt::CursorShape s = Qt::ArrowCursor;

	if ( viewport_GL )
	{
		s = viewport_GL->cursor().shape();
	}
	else if ( viewport_SDL )
	{
		s = viewport_SDL->cursor().shape();
	}
	return s;
}

void consoleWin_t::resizeEvent(QResizeEvent *event)
{
	if ( firstResize )
	{
		// We are assuming that window has been exposed and all sizing of menu is finished
		// Restore minimum sizes to 1x values after first resize event so that
		// window is still able to be shrunk by dragging lower corners.
		if ( viewport_GL != NULL )
		{
			viewport_GL->setMinimumSize( QSize( 256, 224 ) );
		}
		else if ( viewport_SDL != NULL )
		{
			viewport_SDL->setMinimumSize( QSize( 256, 224 ) );
		}
		firstResize = false;
	}
	//printf("%i x %i \n", event->size().width(), event->size().height() );
}

void consoleWin_t::setCyclePeriodms( int ms )
{
	// If timer is already running, it will be restarted.
	gameTimer->start( ms );
   
	//printf("Period Set to: %i ms \n", ms );
}

void consoleWin_t::showErrorMsgWindow()
{
	QMessageBox msgBox(this);

	fceuWrapperLock();
	msgBox.resize( this->size() );
	msgBox.setIcon( QMessageBox::Critical );
	msgBox.setText( tr(errorMsg.c_str()) );
	errorMsg.clear();
	fceuWrapperUnLock();
	//msgBox.show();
	msgBox.exec();
}

void consoleWin_t::QueueErrorMsgWindow( const char *msg )
{
	errorMsg.append( msg );
	errorMsg.append("\n");
	errorMsgValid = true;
}

void consoleWin_t::closeEvent(QCloseEvent *event)
{
	//printf("Main Window Close Event\n");
	closeGamePadConfWindow();

	event->accept();

	closeApp();
}

void consoleWin_t::requestClose(void)
{
	closeRequested = true;
}

void consoleWin_t::keyPressEvent(QKeyEvent *event)
{
	//printf("Key Press: 0x%x \n", event->key() );
	pushKeyEvent( event, 1 );

	event->accept();
}

void consoleWin_t::keyReleaseEvent(QKeyEvent *event)
{
	//printf("Key Release: 0x%x \n", event->key() );
	pushKeyEvent( event, 0 );

	event->accept();
}

//---------------------------------------------------------------------------
void consoleWin_t::initHotKeys(void)
{
	for (int i = 0; i < HK_MAX; i++)
	{
		Hotkeys[i].init( this );
	}

	for (int i = 0; i < HK_MAX; i++)
	{
		QShortcut *shortcut = Hotkeys[i].getShortcut();

		// Use Lambda Function to set callback
		connect( shortcut, &QShortcut::activatedAmbiguously, [ this, shortcut ] { warnAmbiguousShortcut( shortcut ); } );
	}

	// Frame Advance uses key state directly, disable shortcut events
	Hotkeys[HK_FRAME_ADVANCE].getShortcut()->setEnabled(false);
	Hotkeys[HK_TURBO        ].getShortcut()->setEnabled(false);

	connect( Hotkeys[ HK_VOLUME_DOWN ].getShortcut(), SIGNAL(activated()), this, SLOT(decrSoundVolume(void)) );
	connect( Hotkeys[ HK_VOLUME_UP   ].getShortcut(), SIGNAL(activated()), this, SLOT(incrSoundVolume(void)) );

	connect( Hotkeys[ HK_LAG_COUNTER_DISPLAY  ].getShortcut(), SIGNAL(activated()), this, SLOT(toggleLagCounterDisplay(void)) );
	connect( Hotkeys[ HK_FA_LAG_SKIP          ].getShortcut(), SIGNAL(activated()), this, SLOT(toggleFrameAdvLagSkip(void))   );
	connect( Hotkeys[ HK_BIND_STATE           ].getShortcut(), SIGNAL(activated()), this, SLOT(toggleMovieBindSaveState(void)));
	connect( Hotkeys[ HK_TOGGLE_FRAME_DISPLAY ].getShortcut(), SIGNAL(activated()), this, SLOT(toggleMovieFrameDisplay(void)) );
	connect( Hotkeys[ HK_MOVIE_TOGGLE_RW      ].getShortcut(), SIGNAL(activated()), this, SLOT(toggleMovieReadWrite(void))    );
	connect( Hotkeys[ HK_TOGGLE_INPUT_DISPLAY ].getShortcut(), SIGNAL(activated()), this, SLOT(toggleInputDisplay(void))      );
	connect( Hotkeys[ HK_TOGGLE_BG            ].getShortcut(), SIGNAL(activated()), this, SLOT(toggleBackground(void))        );
	connect( Hotkeys[ HK_TOGGLE_FG            ].getShortcut(), SIGNAL(activated()), this, SLOT(toggleForeground(void))        );
	connect( Hotkeys[ HK_FKB_ENABLE           ].getShortcut(), SIGNAL(activated()), this, SLOT(toggleFamKeyBrdEnable(void))   );

	connect( Hotkeys[ HK_SAVE_STATE_0         ].getShortcut(), SIGNAL(activated()), this, SLOT(saveState0(void))        );
	connect( Hotkeys[ HK_SAVE_STATE_1         ].getShortcut(), SIGNAL(activated()), this, SLOT(saveState1(void))        );
	connect( Hotkeys[ HK_SAVE_STATE_2         ].getShortcut(), SIGNAL(activated()), this, SLOT(saveState2(void))        );
	connect( Hotkeys[ HK_SAVE_STATE_3         ].getShortcut(), SIGNAL(activated()), this, SLOT(saveState3(void))        );
	connect( Hotkeys[ HK_SAVE_STATE_4         ].getShortcut(), SIGNAL(activated()), this, SLOT(saveState4(void))        );
	connect( Hotkeys[ HK_SAVE_STATE_5         ].getShortcut(), SIGNAL(activated()), this, SLOT(saveState5(void))        );
	connect( Hotkeys[ HK_SAVE_STATE_6         ].getShortcut(), SIGNAL(activated()), this, SLOT(saveState6(void))        );
	connect( Hotkeys[ HK_SAVE_STATE_7         ].getShortcut(), SIGNAL(activated()), this, SLOT(saveState7(void))        );
	connect( Hotkeys[ HK_SAVE_STATE_8         ].getShortcut(), SIGNAL(activated()), this, SLOT(saveState8(void))        );
	connect( Hotkeys[ HK_SAVE_STATE_9         ].getShortcut(), SIGNAL(activated()), this, SLOT(saveState9(void))        );

	connect( Hotkeys[ HK_LOAD_STATE_0         ].getShortcut(), SIGNAL(activated()), this, SLOT(loadState0(void))        );
	connect( Hotkeys[ HK_LOAD_STATE_1         ].getShortcut(), SIGNAL(activated()), this, SLOT(loadState1(void))        );
	connect( Hotkeys[ HK_LOAD_STATE_2         ].getShortcut(), SIGNAL(activated()), this, SLOT(loadState2(void))        );
	connect( Hotkeys[ HK_LOAD_STATE_3         ].getShortcut(), SIGNAL(activated()), this, SLOT(loadState3(void))        );
	connect( Hotkeys[ HK_LOAD_STATE_4         ].getShortcut(), SIGNAL(activated()), this, SLOT(loadState4(void))        );
	connect( Hotkeys[ HK_LOAD_STATE_5         ].getShortcut(), SIGNAL(activated()), this, SLOT(loadState5(void))        );
	connect( Hotkeys[ HK_LOAD_STATE_6         ].getShortcut(), SIGNAL(activated()), this, SLOT(loadState6(void))        );
	connect( Hotkeys[ HK_LOAD_STATE_7         ].getShortcut(), SIGNAL(activated()), this, SLOT(loadState7(void))        );
	connect( Hotkeys[ HK_LOAD_STATE_8         ].getShortcut(), SIGNAL(activated()), this, SLOT(loadState8(void))        );
	connect( Hotkeys[ HK_LOAD_STATE_9         ].getShortcut(), SIGNAL(activated()), this, SLOT(loadState9(void))        );
}
//---------------------------------------------------------------------------
void consoleWin_t::createMainMenu(void)
{
	QAction *act;
	QMenu *subMenu, *aviMenu;
	QActionGroup *group;
	int useNativeMenuBar;
	//QShortcut *shortcut;

	menubar = new consoleMenuBar(this);

	this->setMenuBar(menubar);

	// This is needed for menu bar to show up on MacOS
	g_config->getOption( "SDL.UseNativeMenuBar", &useNativeMenuBar );

	menubar->setNativeMenuBar( useNativeMenuBar ? true : false );

	//-----------------------------------------------------------------------
	// File
	fileMenu = menubar->addMenu(tr("&File"));
	
	connect( fileMenu, SIGNAL(aboutToShow(void)), this, SLOT(mainMenuOpen(void)) );
	connect( fileMenu, SIGNAL(aboutToHide(void)), this, SLOT(mainMenuClose(void)) );

	// File -> Open ROM
	openROM = new QAction(tr("&Open ROM"), this);
	//openROM->setShortcuts(QKeySequence::Open);
	openROM->setStatusTip(tr("Open ROM File"));
	//openROM->setIcon( QIcon(":icons/rom.png") );
	//openROM->setIcon( style->standardIcon( QStyle::SP_FileIcon ) );
	openROM->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(openROM, SIGNAL(triggered()), this, SLOT(openROMFile(void)) );

	Hotkeys[ HK_OPEN_ROM ].setAction( openROM );
	connect( Hotkeys[ HK_OPEN_ROM ].getShortcut(), SIGNAL(activated()), this, SLOT(openROMFile(void)) );
	
	fileMenu->addAction(openROM);

	// File -> Close ROM
	closeROM = new QAction(tr("&Close ROM"), this);
	//closeROM->setShortcut( QKeySequence(tr("Ctrl+C")));
	closeROM->setStatusTip(tr("Close Loaded ROM"));
	closeROM->setIcon( style()->standardIcon( QStyle::SP_BrowserStop ) );
	connect(closeROM, SIGNAL(triggered()), this, SLOT(closeROMCB(void)) );
	
	Hotkeys[ HK_CLOSE_ROM ].setAction( closeROM );
	connect( Hotkeys[ HK_CLOSE_ROM ].getShortcut(), SIGNAL(activated()), this, SLOT(closeROMCB(void)) );
	
	fileMenu->addAction(closeROM);
	
	// File -> Recent ROMs
	recentRomMenu = fileMenu->addMenu( tr("&Recent ROMs") );

	buildRecentRomMenu();

	fileMenu->addSeparator();

	// File -> Play NSF
	playNSF = new QAction(tr("Play &NSF"), this);
	//playNSF->setShortcut( QKeySequence(tr("Ctrl+N")));
	playNSF->setStatusTip(tr("Play NSF"));
	connect(playNSF, SIGNAL(triggered()), this, SLOT(loadNSF(void)) );
	
	fileMenu->addAction(playNSF);
	
	fileMenu->addSeparator();

	// File -> Load State From
	loadStateAct = new QAction(tr("Load State &From"), this);
	//loadStateAct->setShortcut( QKeySequence(tr("Ctrl+N")));
	loadStateAct->setStatusTip(tr("Load State From"));
	loadStateAct->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(loadStateAct, SIGNAL(triggered()), this, SLOT(loadStateFrom(void)) );
	
	fileMenu->addAction(loadStateAct);

	// File -> Save State As
	saveStateAct = new QAction(tr("Save State &As"), this);
	//loadStateAct->setShortcut( QKeySequence(tr("Ctrl+N")));
	saveStateAct->setStatusTip(tr("Save State As"));
	saveStateAct->setIcon( style()->standardIcon( QStyle::SP_DialogSaveButton ) );
	connect(saveStateAct, SIGNAL(triggered()), this, SLOT(saveStateAs(void)) );
	
	fileMenu->addAction(saveStateAct);

	// File -> Quick Load
	quickLoadAct = new QAction(tr("Quick &Load"), this);
	//quickLoadAct->setShortcut( QKeySequence(tr("Shift+I")));
	quickLoadAct->setStatusTip(tr("Quick Load"));
	connect(quickLoadAct, SIGNAL(triggered()), this, SLOT(quickLoad(void)) );
	
	fileMenu->addAction(quickLoadAct);

	Hotkeys[ HK_LOAD_STATE ].setAction( quickLoadAct );
	connect( Hotkeys[ HK_LOAD_STATE ].getShortcut(), SIGNAL(activated()), this, SLOT(quickLoad(void)) );
	
	// File -> Quick Save
	quickSaveAct = new QAction(tr("Quick &Save"), this);
	//quickSaveAct->setShortcut( QKeySequence(tr("F5")));
	quickSaveAct->setStatusTip(tr("Quick Save"));
	connect(quickSaveAct, SIGNAL(triggered()), this, SLOT(quickSave(void)) );
	
	fileMenu->addAction(quickSaveAct);

	Hotkeys[ HK_SAVE_STATE ].setAction( quickSaveAct );
	connect( Hotkeys[ HK_SAVE_STATE ].getShortcut(), SIGNAL(activated()), this, SLOT(quickSave(void)) );
	
	// File -> Change State
	subMenu = fileMenu->addMenu(tr("Change &State"));
	group   = new QActionGroup(this);

	group->setExclusive(true);

	for (int i=0; i<10; i++)
	{
	        char stmp[8];

	        sprintf( stmp, "&%i", i );

	        state[i] = new QAction(tr(stmp), this);
	        state[i]->setCheckable(true);

	        group->addAction(state[i]);
		subMenu->addAction(state[i]);
	}
	state[0]->setChecked(true);

	connect(state[0], SIGNAL(triggered()), this, SLOT(changeState0(void)) );
	connect(state[1], SIGNAL(triggered()), this, SLOT(changeState1(void)) );
	connect(state[2], SIGNAL(triggered()), this, SLOT(changeState2(void)) );
	connect(state[3], SIGNAL(triggered()), this, SLOT(changeState3(void)) );
	connect(state[4], SIGNAL(triggered()), this, SLOT(changeState4(void)) );
	connect(state[5], SIGNAL(triggered()), this, SLOT(changeState5(void)) );
	connect(state[6], SIGNAL(triggered()), this, SLOT(changeState6(void)) );
	connect(state[7], SIGNAL(triggered()), this, SLOT(changeState7(void)) );
	connect(state[8], SIGNAL(triggered()), this, SLOT(changeState8(void)) );
	connect(state[9], SIGNAL(triggered()), this, SLOT(changeState9(void)) );
	
	fileMenu->addSeparator();

	Hotkeys[ HK_SELECT_STATE_0 ].setAction( state[0] );
	Hotkeys[ HK_SELECT_STATE_1 ].setAction( state[1] );
	Hotkeys[ HK_SELECT_STATE_2 ].setAction( state[2] );
	Hotkeys[ HK_SELECT_STATE_3 ].setAction( state[3] );
	Hotkeys[ HK_SELECT_STATE_4 ].setAction( state[4] );
	Hotkeys[ HK_SELECT_STATE_5 ].setAction( state[5] );
	Hotkeys[ HK_SELECT_STATE_6 ].setAction( state[6] );
	Hotkeys[ HK_SELECT_STATE_7 ].setAction( state[7] );
	Hotkeys[ HK_SELECT_STATE_8 ].setAction( state[8] );
	Hotkeys[ HK_SELECT_STATE_9 ].setAction( state[9] );

	connect( Hotkeys[ HK_SELECT_STATE_0 ].getShortcut(), SIGNAL(activated()), this, SLOT(changeState0(void)) );
	connect( Hotkeys[ HK_SELECT_STATE_1 ].getShortcut(), SIGNAL(activated()), this, SLOT(changeState1(void)) );
	connect( Hotkeys[ HK_SELECT_STATE_2 ].getShortcut(), SIGNAL(activated()), this, SLOT(changeState2(void)) );
	connect( Hotkeys[ HK_SELECT_STATE_3 ].getShortcut(), SIGNAL(activated()), this, SLOT(changeState3(void)) );
	connect( Hotkeys[ HK_SELECT_STATE_4 ].getShortcut(), SIGNAL(activated()), this, SLOT(changeState4(void)) );
	connect( Hotkeys[ HK_SELECT_STATE_5 ].getShortcut(), SIGNAL(activated()), this, SLOT(changeState5(void)) );
	connect( Hotkeys[ HK_SELECT_STATE_6 ].getShortcut(), SIGNAL(activated()), this, SLOT(changeState6(void)) );
	connect( Hotkeys[ HK_SELECT_STATE_7 ].getShortcut(), SIGNAL(activated()), this, SLOT(changeState7(void)) );
	connect( Hotkeys[ HK_SELECT_STATE_8 ].getShortcut(), SIGNAL(activated()), this, SLOT(changeState8(void)) );
	connect( Hotkeys[ HK_SELECT_STATE_9 ].getShortcut(), SIGNAL(activated()), this, SLOT(changeState9(void)) );
	
	connect( Hotkeys[ HK_SELECT_STATE_PREV ].getShortcut(), SIGNAL(activated()), this, SLOT(decrementState(void)) );
	connect( Hotkeys[ HK_SELECT_STATE_NEXT ].getShortcut(), SIGNAL(activated()), this, SLOT(incrementState(void)) );

#ifdef _S9XLUA_H
	// File -> Quick Save
	loadLuaAct = new QAction(tr("Load &Lua Script"), this);
	//loadLuaAct->setShortcut( QKeySequence(tr("F5")));
	loadLuaAct->setStatusTip(tr("Load Lua Script"));
	//loadLuaAct->setIcon( QIcon(":icons/lua-logo.png") );
	connect(loadLuaAct, SIGNAL(triggered()), this, SLOT(loadLua(void)) );
	
	fileMenu->addAction(loadLuaAct);
	
	fileMenu->addSeparator();
#else
	loadLuaAct = NULL;
#endif

	// File -> Screenshot
	scrShotAct = new QAction(tr("Screens&hot"), this);
	//scrShotAct->setShortcut( QKeySequence(tr("F12")));
	scrShotAct->setStatusTip(tr("Screenshot"));
	scrShotAct->setIcon( QIcon(":icons/camera.png") );
	connect(scrShotAct, SIGNAL(triggered()), this, SLOT(takeScreenShot()));
	
	fileMenu->addAction(scrShotAct);

	Hotkeys[ HK_SCREENSHOT ].setAction( scrShotAct );
	connect( Hotkeys[ HK_SCREENSHOT ].getShortcut(), SIGNAL(activated()), this, SLOT(takeScreenShot(void)) );

	// File -> Quit
	quitAct = new QAction(tr("&Quit"), this);
	//quitAct->setShortcut( QKeySequence(tr("Ctrl+Q")));
	quitAct->setStatusTip(tr("Quit the Application"));
	//quitAct->setIcon( style()->standardIcon( QStyle::SP_DialogCloseButton ) );
	quitAct->setIcon( QIcon(":icons/application-exit.png") );
	connect(quitAct, SIGNAL(triggered()), this, SLOT(closeApp()));
	
	fileMenu->addAction(quitAct);

	Hotkeys[ HK_QUIT ].setAction( quitAct );
	connect( Hotkeys[ HK_QUIT ].getShortcut(), SIGNAL(activated()), this, SLOT(closeApp(void)) );

	//-----------------------------------------------------------------------
	// Options
	optMenu = menubar->addMenu(tr("&Options"));

	connect( optMenu, SIGNAL(aboutToShow(void)), this, SLOT(mainMenuOpen(void)) );
	connect( optMenu, SIGNAL(aboutToHide(void)), this, SLOT(mainMenuClose(void)) );

	// Options -> Input Config
	inputConfig = new QAction(tr("&Input Config"), this);
	//inputConfig->setShortcut( QKeySequence(tr("Ctrl+C")));
	inputConfig->setStatusTip(tr("Input Configure"));
	inputConfig->setIcon( QIcon(":icons/input-gaming.png") );
	connect(inputConfig, SIGNAL(triggered()), this, SLOT(openInputConfWin(void)) );
	
	optMenu->addAction(inputConfig);

	// Options -> GamePad Config
	gamePadConfig = new QAction(tr("&GamePad Config"), this);
	//gamePadConfig->setShortcut( QKeySequence(tr("Ctrl+C")));
	gamePadConfig->setStatusTip(tr("GamePad Configure"));
	gamePadConfig->setIcon( QIcon(":icons/input-gaming-symbolic.png") );
	connect(gamePadConfig, SIGNAL(triggered()), this, SLOT(openGamePadConfWin(void)) );
	
	optMenu->addAction(gamePadConfig);

	// Options -> Sound Config
	gameSoundConfig = new QAction(tr("&Sound Config"), this);
	//gameSoundConfig->setShortcut( QKeySequence(tr("Ctrl+C")));
	gameSoundConfig->setStatusTip(tr("Sound Configure"));
	gameSoundConfig->setIcon( style()->standardIcon( QStyle::SP_MediaVolume ) );
	connect(gameSoundConfig, SIGNAL(triggered()), this, SLOT(openGameSndConfWin(void)) );
	
	optMenu->addAction(gameSoundConfig);

	// Options -> Video Config
	gameVideoConfig = new QAction(tr("&Video Config"), this);
	//gameVideoConfig->setShortcut( QKeySequence(tr("Ctrl+C")));
	gameVideoConfig->setStatusTip(tr("Video Preferences"));
	gameVideoConfig->setIcon( style()->standardIcon( QStyle::SP_ComputerIcon ) );
	connect(gameVideoConfig, SIGNAL(triggered()), this, SLOT(openGameVideoConfWin(void)) );
	
	optMenu->addAction(gameVideoConfig);

	// Options -> HotKey Config
	hotkeyConfig = new QAction(tr("Hot&Key Config"), this);
	//hotkeyConfig->setShortcut( QKeySequence(tr("Ctrl+C")));
	hotkeyConfig->setStatusTip(tr("Hotkey Configure"));
	hotkeyConfig->setIcon( QIcon(":icons/input-keyboard.png") );
	connect(hotkeyConfig, SIGNAL(triggered()), this, SLOT(openHotkeyConfWin(void)) );
	
	optMenu->addAction(hotkeyConfig);

	// Options -> Palette Config
	paletteConfig = new QAction(tr("&Palette Config"), this);
	//paletteConfig->setShortcut( QKeySequence(tr("Ctrl+C")));
	paletteConfig->setStatusTip(tr("Palette Configure"));
	paletteConfig->setIcon( QIcon(":icons/graphics-palette.png") );
	connect(paletteConfig, SIGNAL(triggered()), this, SLOT(openPaletteConfWin(void)) );
	
	optMenu->addAction(paletteConfig);

	// Options -> GUI Config
	guiConfig = new QAction(tr("G&UI Config"), this);
	//guiConfig->setShortcut( QKeySequence(tr("Ctrl+C")));
	guiConfig->setStatusTip(tr("GUI Configure"));
	guiConfig->setIcon( style()->standardIcon( QStyle::SP_TitleBarNormalButton ) );
	connect(guiConfig, SIGNAL(triggered()), this, SLOT(openGuiConfWin(void)) );
	
	optMenu->addAction(guiConfig);

	// Options -> Timing Config
	timingConfig = new QAction(tr("&Timing Config"), this);
	//timingConfig->setShortcut( QKeySequence(tr("Ctrl+C")));
	timingConfig->setStatusTip(tr("Timing Configure"));
	timingConfig->setIcon( QIcon(":icons/timer.png") );
	connect(timingConfig, SIGNAL(triggered()), this, SLOT(openTimingConfWin(void)) );
	
	optMenu->addAction(timingConfig);

	// Options -> Movie Options
	movieConfig = new QAction(tr("&Movie Options"), this);
	//movieConfig->setShortcut( QKeySequence(tr("Ctrl+C")));
	movieConfig->setStatusTip(tr("Movie Options"));
	movieConfig->setIcon( QIcon(":icons/movie.png") );
	connect(movieConfig, SIGNAL(triggered()), this, SLOT(openMovieOptWin(void)) );
	
	optMenu->addAction(movieConfig);

	// Options -> Auto-Resume
	autoResume = new QAction(tr("Auto-&Resume Play"), this);
	//autoResume->setShortcut( QKeySequence(tr("Ctrl+C")));
	autoResume->setCheckable(true);
	autoResume->setStatusTip(tr("Auto-Resume Play"));
	connect(autoResume, SIGNAL(triggered()), this, SLOT(toggleAutoResume(void)) );

	optMenu->addAction(autoResume);
	
	optMenu->addSeparator();

	// Options -> Full Screen
	fullscreen = new QAction(tr("&Fullscreen"), this);
	//fullscreen->setShortcut( QKeySequence(tr("Alt+Return")));
	fullscreen->setStatusTip(tr("Fullscreen"));
	fullscreen->setIcon( QIcon(":icons/view-fullscreen.png") );
	connect(fullscreen, SIGNAL(triggered()), this, SLOT(toggleFullscreen(void)) );
	
	optMenu->addAction(fullscreen);

	Hotkeys[ HK_FULLSCREEN ].setAction( fullscreen );
	connect( Hotkeys[ HK_FULLSCREEN ].getShortcut(), SIGNAL(activated()), this, SLOT(toggleFullscreen(void)) );

	// Options -> Hide Menu Screen
	act = new QAction(tr("&Hide Menu"), this);
	//act->setShortcut( QKeySequence(tr("Alt+/")));
	act->setStatusTip(tr("Hide Menu"));
	act->setIcon( style()->standardIcon( QStyle::SP_TitleBarMaxButton ) );
	connect(act, SIGNAL(triggered()), this, SLOT(toggleMenuVis(void)) );
	
	optMenu->addAction(act);

	Hotkeys[ HK_MAIN_MENU_HIDE ].setAction( act );
	connect( Hotkeys[ HK_MAIN_MENU_HIDE ].getShortcut(), SIGNAL(activated()), this, SLOT(toggleMenuVis(void)) );

	//-----------------------------------------------------------------------
	// Emulation
	emuMenu = menubar->addMenu(tr("&Emulation"));

	connect( emuMenu, SIGNAL(aboutToShow(void)), this, SLOT(mainMenuOpen(void)) );
	connect( emuMenu, SIGNAL(aboutToHide(void)), this, SLOT(mainMenuClose(void)) );

	// Emulation -> Power
	powerAct = new QAction(tr("&Power"), this);
	//powerAct->setShortcut( QKeySequence(tr("Ctrl+P")));
	powerAct->setStatusTip(tr("Power On Console"));
	powerAct->setIcon( QIcon(":icons/power.png") );
	connect(powerAct, SIGNAL(triggered()), this, SLOT(powerConsoleCB(void)) );
	
	emuMenu->addAction(powerAct);

	Hotkeys[ HK_POWER ].setAction( powerAct );
	connect( Hotkeys[ HK_POWER ].getShortcut(), SIGNAL(activated()), this, SLOT(powerConsoleCB(void)) );

	// Emulation -> Reset
	resetAct = new QAction(tr("&Reset"), this);
	//resetAct->setShortcut( QKeySequence(tr("Ctrl+R")));
	resetAct->setStatusTip(tr("Reset Console"));
	resetAct->setIcon( style()->standardIcon( QStyle::SP_DialogResetButton ) );
	connect(resetAct, SIGNAL(triggered()), this, SLOT(consoleHardReset(void)) );
	
	emuMenu->addAction(resetAct);

	Hotkeys[ HK_RESET ].setAction( resetAct );
	connect( Hotkeys[ HK_RESET ].getShortcut(), SIGNAL(activated()), this, SLOT(consoleHardReset(void)) );

	// Emulation -> Soft Reset
	sresetAct = new QAction(tr("&Soft Reset"), this);
	//sresetAct->setShortcut( QKeySequence(tr("Ctrl+R")));
	sresetAct->setStatusTip(tr("Soft Reset of Console"));
	sresetAct->setIcon( style()->standardIcon( QStyle::SP_BrowserReload ) );
	connect(sresetAct, SIGNAL(triggered()), this, SLOT(consoleSoftReset(void)) );
	
	emuMenu->addAction(sresetAct);

	// Emulation -> Pause
	pauseAct = new QAction(tr("&Pause"), this);
	//pauseAct->setShortcut( QKeySequence(tr("Pause")));
	pauseAct->setStatusTip(tr("Pause Console"));
	pauseAct->setIcon( style()->standardIcon( QStyle::SP_MediaPause ) );
	connect(pauseAct, SIGNAL(triggered()), this, SLOT(consolePause(void)) );
	
	emuMenu->addAction(pauseAct);
	
	Hotkeys[ HK_PAUSE ].setAction( pauseAct );
	connect( Hotkeys[ HK_PAUSE ].getShortcut(), SIGNAL(activated()), this, SLOT(consolePause(void)) );

	emuMenu->addSeparator();

	// Emulation -> Region
	subMenu = emuMenu->addMenu(tr("&Region"));
	group   = new QActionGroup(this);

	group->setExclusive(true);

	for (int i=0; i<3; i++)
	{
		const char *txt;

		if ( i == 1 )
		{
			txt = "&PAL";
		}
		else if ( i == 2 )
		{
			txt = "&Dendy";
		}
		else
		{
			txt = "&NTSC";
		}

	        region[i] = new QAction(tr(txt), this);
	        region[i]->setCheckable(true);

	        group->addAction(region[i]);
		subMenu->addAction(region[i]);
	}
	region[ FCEUI_GetRegion() ]->setChecked(true);

	connect( region[0], SIGNAL(triggered(void)), this, SLOT(setRegionNTSC(void)) );
	connect( region[1], SIGNAL(triggered(void)), this, SLOT(setRegionPAL(void)) );
	connect( region[2], SIGNAL(triggered(void)), this, SLOT(setRegionDendy(void)) );

	// Emulation -> RAM Init
	subMenu = emuMenu->addMenu(tr("&RAM Init"));
	group   = new QActionGroup(this);

	group->setExclusive(true);

	for (int i=0; i<4; i++)
	{
		const char *txt;

		switch (i)
		{
			default:
			case 0:
				txt = "&Default";
			break;
			case 1:
				txt = "Fill $&FF";
			break;
			case 2:
				txt = "Fill $&00";
			break;
			case 3:
				txt = "&Random";
			break;
		}

	        ramInit[i] = new QAction(tr(txt), this);
	        ramInit[i]->setCheckable(true);

	        group->addAction(ramInit[i]);
		subMenu->addAction(ramInit[i]);
	}

	g_config->getOption ("SDL.RamInitMethod", &RAMInitOption);

	ramInit[ RAMInitOption ]->setChecked(true);

	connect( ramInit[0], SIGNAL(triggered(void)), this, SLOT(setRamInit0(void)) );
	connect( ramInit[1], SIGNAL(triggered(void)), this, SLOT(setRamInit1(void)) );
	connect( ramInit[2], SIGNAL(triggered(void)), this, SLOT(setRamInit2(void)) );
	connect( ramInit[3], SIGNAL(triggered(void)), this, SLOT(setRamInit3(void)) );

	emuMenu->addSeparator();

	// Emulation -> Enable Game Genie
	gameGenieAct = new QAction(tr("Enable Game &Genie"), this);
	//gameGenieAct->setShortcut( QKeySequence(tr("Ctrl+G")));
	gameGenieAct->setCheckable(true);
	gameGenieAct->setStatusTip(tr("Enable Game Genie"));
	connect(gameGenieAct, SIGNAL(triggered(bool)), this, SLOT(toggleGameGenie(bool)) );

	syncActionConfig( gameGenieAct, "SDL.GameGenie" );

	emuMenu->addAction(gameGenieAct);

	// Emulation -> Load Game Genie ROM
	loadGgROMAct = new QAction(tr("Load Game Genie ROM"), this);
	//loadGgROMAct->setShortcut( QKeySequence(tr("Ctrl+G")));
	loadGgROMAct->setStatusTip(tr("Load Game Genie ROM"));
	connect(loadGgROMAct, SIGNAL(triggered()), this, SLOT(loadGameGenieROM(void)) );
	
	emuMenu->addAction(loadGgROMAct);
	
	emuMenu->addSeparator();

	// Emulation -> Insert Coin
	insCoinAct = new QAction(tr("&Insert Coin"), this);
	//insCoinAct->setShortcut( QKeySequence(tr("Ctrl+G")));
	insCoinAct->setStatusTip(tr("Insert Coin"));
	connect(insCoinAct, SIGNAL(triggered()), this, SLOT(insertCoin(void)) );
	
	emuMenu->addAction(insCoinAct);
	
	Hotkeys[ HK_VS_INSERT_COIN ].setAction( insCoinAct );
	connect( Hotkeys[ HK_VS_INSERT_COIN ].getShortcut(), SIGNAL(activated()), this, SLOT(insertCoin(void)) );

	emuMenu->addSeparator();

	// Emulation -> FDS
	subMenu = emuMenu->addMenu(tr("&FDS"));

	// Emulation -> FDS -> Switch Disk
	fdsSwitchAct = new QAction(tr("&Switch Disk"), this);
	//fdsSwitchAct->setShortcut( QKeySequence(tr("Ctrl+G")));
	fdsSwitchAct->setStatusTip(tr("Switch Disk"));
	connect(fdsSwitchAct, SIGNAL(triggered()), this, SLOT(fdsSwitchDisk(void)) );
	
	Hotkeys[ HK_FDS_SELECT ].setAction( fdsSwitchAct );
	connect( Hotkeys[ HK_FDS_SELECT ].getShortcut(), SIGNAL(activated()), this, SLOT(fdsSwitchDisk(void)) );

	subMenu->addAction(fdsSwitchAct);

	// Emulation -> FDS -> Eject Disk
	fdsEjectAct = new QAction(tr("&Eject Disk"), this);
	//fdsEjectAct->setShortcut( QKeySequence(tr("Ctrl+G")));
	fdsEjectAct->setStatusTip(tr("Eject Disk"));
	connect(fdsEjectAct, SIGNAL(triggered()), this, SLOT(fdsEjectDisk(void)) );
	
	Hotkeys[ HK_FDS_EJECT ].setAction( fdsEjectAct );
	connect( Hotkeys[ HK_FDS_EJECT ].getShortcut(), SIGNAL(activated()), this, SLOT(fdsEjectDisk(void)) );

	subMenu->addAction(fdsEjectAct);

	// Emulation -> FDS -> Load BIOS
	fdsLoadBiosAct = new QAction(tr("&Load BIOS"), this);
	//fdsLoadBiosAct->setShortcut( QKeySequence(tr("Ctrl+G")));
	fdsLoadBiosAct->setStatusTip(tr("Load BIOS"));
	connect(fdsLoadBiosAct, SIGNAL(triggered()), this, SLOT(fdsLoadBiosFile(void)) );
	
	subMenu->addAction(fdsLoadBiosAct);
	
	emuMenu->addSeparator();

	// Emulation -> Speed
	subMenu = emuMenu->addMenu(tr("&Speed"));

	// Emulation -> Speed -> Speed Up
	act = new QAction(tr("Speed &Up"), this);
	//act->setShortcut( QKeySequence(tr("=")));
	act->setStatusTip(tr("Speed Up"));
	act->setIcon( style()->standardIcon( QStyle::SP_MediaSeekForward ) );
	connect(act, SIGNAL(triggered()), this, SLOT(emuSpeedUp(void)) );
	
	Hotkeys[ HK_INCREASE_SPEED ].setAction( act );
	connect( Hotkeys[ HK_INCREASE_SPEED ].getShortcut(), SIGNAL(activated()), this, SLOT(emuSpeedUp(void)) );

	subMenu->addAction(act);

	// Emulation -> Speed -> Slow Down
	act = new QAction(tr("Slow &Down"), this);
	//act->setShortcut( QKeySequence(tr("-")));
	act->setStatusTip(tr("Slow Down"));
	act->setIcon( style()->standardIcon( QStyle::SP_MediaSeekBackward ) );
	connect(act, SIGNAL(triggered()), this, SLOT(emuSlowDown(void)) );
	
	Hotkeys[ HK_DECREASE_SPEED ].setAction( act );
	connect( Hotkeys[ HK_DECREASE_SPEED ].getShortcut(), SIGNAL(activated()), this, SLOT(emuSlowDown(void)) );

	subMenu->addAction(act);
	
	subMenu->addSeparator();

	// Emulation -> Speed -> Slowest Speed
	act = new QAction(tr("&Slowest"), this);
	//act->setShortcut( QKeySequence(tr("-")));
	act->setStatusTip(tr("Slowest"));
	act->setIcon( style()->standardIcon( QStyle::SP_MediaSkipBackward ) );
	connect(act, SIGNAL(triggered()), this, SLOT(emuSlowestSpd(void)) );
	
	subMenu->addAction(act);

	// Emulation -> Speed -> Normal Speed
	act = new QAction(tr("&Normal"), this);
	//act->setShortcut( QKeySequence(tr("-")));
	act->setStatusTip(tr("Normal"));
	act->setIcon( style()->standardIcon( QStyle::SP_MediaPlay ) );
	connect(act, SIGNAL(triggered()), this, SLOT(emuNormalSpd(void)) );
	
	subMenu->addAction(act);
	
	// Emulation -> Speed -> Fastest Speed
	act = new QAction(tr("&Turbo"), this);
	//act->setShortcut( QKeySequence(tr("-")));
	act->setStatusTip(tr("Turbo (Fastest)"));
	act->setIcon( style()->standardIcon( QStyle::SP_MediaSkipForward ) );
	connect(act, SIGNAL(triggered()), this, SLOT(emuFastestSpd(void)) );
	
	subMenu->addAction(act);
	
	// Emulation -> Speed -> Custom Speed
	act = new QAction(tr("&Custom"), this);
	//act->setShortcut( QKeySequence(tr("-")));
	act->setStatusTip(tr("Custom"));
	connect(act, SIGNAL(triggered()), this, SLOT(emuCustomSpd(void)) );
	
	subMenu->addAction(act);
	
	subMenu->addSeparator();
	
	// Emulation -> Speed -> Set Frame Advance Delay
	act = new QAction(tr("Set Frame &Advance Delay"), this);
	//act->setShortcut( QKeySequence(tr("-")));
	act->setStatusTip(tr("Set Frame Advance Delay"));
	connect(act, SIGNAL(triggered()), this, SLOT(emuSetFrameAdvDelay(void)) );
	
	subMenu->addAction(act);

	emuMenu->addSeparator();

	// Emulation -> AutoFire Pattern
	subMenu = emuMenu->addMenu(tr("&AutoFire Pattern"));
	
	// Emulation -> AutoFire Pattern -> # On Frames
	act = new QAction(tr("# O&N Frames"), this);
	act->setStatusTip(tr("# ON Frames"));
	connect(act, SIGNAL(triggered()), this, SLOT(setAutoFireOnFrames(void)) );
	
	subMenu->addAction(act);

	// Emulation -> AutoFire Pattern -> # Off Frames
	act = new QAction(tr("# O&FF Frames"), this);
	act->setStatusTip(tr("# OFF Frames"));
	connect(act, SIGNAL(triggered()), this, SLOT(setAutoFireOffFrames(void)) );
	
	subMenu->addAction(act);

	//-----------------------------------------------------------------------
	// Tools
	toolsMenu = menubar->addMenu(tr("&Tools"));

	connect( toolsMenu, SIGNAL(aboutToShow(void)), this, SLOT(mainMenuOpen(void)) );
	connect( toolsMenu, SIGNAL(aboutToHide(void)), this, SLOT(mainMenuClose(void)) );

	// Tools -> Cheats
	cheatsAct = new QAction(tr("&Cheats..."), this);
	//cheatsAct->setShortcut( QKeySequence(tr("Shift+F7")));
	cheatsAct->setStatusTip(tr("Open Cheat Window"));
	connect(cheatsAct, SIGNAL(triggered()), this, SLOT(openCheats(void)) );
	
	Hotkeys[ HK_CHEAT_MENU ].setAction( cheatsAct );
	connect( Hotkeys[ HK_CHEAT_MENU ].getShortcut(), SIGNAL(activated()), this, SLOT(openCheats(void)) );

	toolsMenu->addAction(cheatsAct);

	// Tools -> RAM Search
	ramSearchAct = new QAction(tr("RAM &Search..."), this);
	//ramSearchAct->setShortcut( QKeySequence(tr("Shift+F7")));
	ramSearchAct->setStatusTip(tr("Open RAM Search Window"));
	connect(ramSearchAct, SIGNAL(triggered()), this, SLOT(openRamSearch(void)) );
	
	toolsMenu->addAction(ramSearchAct);

	// Tools -> RAM Watch
	ramWatchAct = new QAction(tr("RAM &Watch..."), this);
	//ramWatchAct->setShortcut( QKeySequence(tr("Shift+F7")));
	ramWatchAct->setStatusTip(tr("Open RAM Watch Window"));
	connect(ramWatchAct, SIGNAL(triggered()), this, SLOT(openRamWatch(void)) );
	
	toolsMenu->addAction(ramWatchAct);

	// Tools -> Frame Timing
	act = new QAction(tr("&Frame Timing ..."), this);
	//act->setShortcut( QKeySequence(tr("Shift+F7")));
	act->setStatusTip(tr("Open Frame Timing Window"));
	connect(act, SIGNAL(triggered()), this, SLOT(openTimingStatWin(void)) );

	toolsMenu->addAction(act);

	// Tools -> Palette Editor
	act = new QAction(tr("&Palette Editor ..."), this);
	//act->setShortcut( QKeySequence(tr("Shift+F7")));
	act->setStatusTip(tr("Open Palette Editor Window"));
	connect(act, SIGNAL(triggered()), this, SLOT(openPaletteEditorWin(void)) );

	toolsMenu->addAction(act);

	 //-----------------------------------------------------------------------
	 // Debug
	debugMenu = menubar->addMenu(tr("&Debug"));

	connect( debugMenu, SIGNAL(aboutToShow(void)), this, SLOT(mainMenuOpen(void)) );
	connect( debugMenu, SIGNAL(aboutToHide(void)), this, SLOT(mainMenuClose(void)) );

	// Debug -> Debugger 
	debuggerAct = new QAction(tr("&Debugger..."), this);
	//debuggerAct->setShortcut( QKeySequence(tr("Shift+F7")));
	debuggerAct->setStatusTip(tr("Open 6502 Debugger"));
	connect(debuggerAct, SIGNAL(triggered()), this, SLOT(openDebugWindow(void)) );
	
	debugMenu->addAction(debuggerAct);

	// Debug -> Hex Editor
	hexEditAct = new QAction(tr("&Hex Editor..."), this);
	//hexEditAct->setShortcut( QKeySequence(tr("Shift+F7")));
	hexEditAct->setStatusTip(tr("Open Memory Hex Editor"));
	connect(hexEditAct, SIGNAL(triggered()), this, SLOT(openHexEditor(void)) );
	
	debugMenu->addAction(hexEditAct);

	// Debug -> PPU Viewer
	ppuViewAct = new QAction(tr("&PPU Viewer..."), this);
	//ppuViewAct->setShortcut( QKeySequence(tr("Shift+F7")));
	ppuViewAct->setStatusTip(tr("Open PPU Viewer"));
	connect(ppuViewAct, SIGNAL(triggered()), this, SLOT(openPPUViewer(void)) );
	
	debugMenu->addAction(ppuViewAct);

	// Debug -> Sprite Viewer
	oamViewAct = new QAction(tr("&Sprite Viewer..."), this);
	//oamViewAct->setShortcut( QKeySequence(tr("Shift+F7")));
	oamViewAct->setStatusTip(tr("Open Sprite Viewer"));
	connect(oamViewAct, SIGNAL(triggered()), this, SLOT(openOAMViewer(void)) );
	
	debugMenu->addAction(oamViewAct);

	// Debug -> Name Table Viewer
	ntViewAct = new QAction(tr("&Name Table Viewer..."), this);
	//ntViewAct->setShortcut( QKeySequence(tr("Shift+F7")));
	ntViewAct->setStatusTip(tr("Open Name Table Viewer"));
	connect(ntViewAct, SIGNAL(triggered()), this, SLOT(openNTViewer(void)) );
	
	debugMenu->addAction(ntViewAct);

	// Debug -> Trace Logger
	traceLogAct = new QAction(tr("&Trace Logger..."), this);
	//traceLogAct->setShortcut( QKeySequence(tr("Shift+F7")));
	traceLogAct->setStatusTip(tr("Open Trace Logger"));
	connect(traceLogAct, SIGNAL(triggered()), this, SLOT(openTraceLogger(void)) );
	
	debugMenu->addAction(traceLogAct);

	// Debug -> Code/Data Logger
	codeDataLogAct = new QAction(tr("&Code/Data Logger..."), this);
	//codeDataLogAct->setShortcut( QKeySequence(tr("Shift+F7")));
	codeDataLogAct->setStatusTip(tr("Open Code Data Logger"));
	connect(codeDataLogAct, SIGNAL(triggered()), this, SLOT(openCodeDataLogger(void)) );
	
	debugMenu->addAction(codeDataLogAct);

	// Debug -> Game Genie Encode/Decode Viewer
	ggEncodeAct = new QAction(tr("&Game Genie Encode/Decode"), this);
	//ggEncodeAct->setShortcut( QKeySequence(tr("Shift+F7")));
	ggEncodeAct->setStatusTip(tr("Open Game Genie Encode/Decode"));
	connect(ggEncodeAct, SIGNAL(triggered()), this, SLOT(openGGEncoder(void)) );
	
	debugMenu->addAction(ggEncodeAct);

	// Debug -> iNES Header Editor
	iNesEditAct = new QAction(tr("&iNES Header Editor..."), this);
	//iNesEditAct->setShortcut( QKeySequence(tr("Shift+F7")));
	iNesEditAct->setStatusTip(tr("Open iNES Header Editor"));
	connect(iNesEditAct, SIGNAL(triggered()), this, SLOT(openNesHeaderEditor(void)) );
	
	debugMenu->addAction(iNesEditAct);

	//-----------------------------------------------------------------------
	// Movie
	movieMenu = menubar->addMenu(tr("&Movie"));

	connect( movieMenu, SIGNAL(aboutToShow(void)), this, SLOT(mainMenuOpen(void)) );
	connect( movieMenu, SIGNAL(aboutToHide(void)), this, SLOT(mainMenuClose(void)) );

	// Movie -> Play
	openMovAct = new QAction(tr("&Play"), this);
	//openMovAct->setShortcut( QKeySequence(tr("Shift+F7")));
	openMovAct->setStatusTip(tr("Play Movie File"));
	openMovAct->setIcon( style()->standardIcon( QStyle::SP_MediaPlay ) );
	connect(openMovAct, SIGNAL(triggered()), this, SLOT(openMovie(void)) );
	
	Hotkeys[ HK_PLAY_MOVIE_FROM ].setAction( openMovAct );
	connect( Hotkeys[ HK_PLAY_MOVIE_FROM ].getShortcut(), SIGNAL(activated()), this, SLOT(openMovie(void)) );

	movieMenu->addAction(openMovAct);

	// Movie -> Play From Beginning
	playMovBeginAct = new QAction(tr("Play From &Beginning"), this);
	//playMovBeginAct->setShortcut( QKeySequence(tr("Shift+F7")));
	playMovBeginAct->setStatusTip(tr("Play Movie From Beginning"));
	//playMovBeginAct->setIcon( style()->standardIcon( QStyle::SP_MediaPlay ) );
	connect(playMovBeginAct, SIGNAL(triggered()), this, SLOT(playMovieFromBeginning(void)) );
	
	Hotkeys[ HK_MOVIE_PLAY_RESTART ].setAction( playMovBeginAct );
	connect( Hotkeys[ HK_MOVIE_PLAY_RESTART ].getShortcut(), SIGNAL(activated()), this, SLOT(playMovieFromBeginning(void)) );

	movieMenu->addAction(playMovBeginAct);

	// Movie -> Stop
	stopMovAct = new QAction(tr("&Stop"), this);
	//stopMovAct->setShortcut( QKeySequence(tr("Shift+F7")));
	stopMovAct->setStatusTip(tr("Stop Movie Recording"));
	stopMovAct->setIcon( style()->standardIcon( QStyle::SP_MediaStop ) );
	connect(stopMovAct, SIGNAL(triggered()), this, SLOT(stopMovie(void)) );
	
	Hotkeys[ HK_STOP_MOVIE ].setAction( stopMovAct );
	connect( Hotkeys[ HK_STOP_MOVIE ].getShortcut(), SIGNAL(activated()), this, SLOT(stopMovie(void)) );

	movieMenu->addAction(stopMovAct);
	
	movieMenu->addSeparator();

	// Movie -> Record
	recMovAct = new QAction(tr("&Record"), this);
	//recMovAct->setShortcut( QKeySequence(tr("Shift+F5")));
	recMovAct->setStatusTip(tr("Record Movie"));
	recMovAct->setIcon( QIcon(":icons/media-record.png") );
	connect(recMovAct, SIGNAL(triggered()), this, SLOT(recordMovie(void)) );
	
	movieMenu->addAction(recMovAct);

	// Movie -> Record As
	recAsMovAct = new QAction(tr("Record &As"), this);
	//recAsMovAct->setShortcut( QKeySequence(tr("Shift+F5")));
	recAsMovAct->setStatusTip(tr("Record Movie"));
	connect(recAsMovAct, SIGNAL(triggered()), this, SLOT(recordMovieAs(void)) );
	
	Hotkeys[ HK_RECORD_MOVIE_TO ].setAction( recAsMovAct );
	connect( Hotkeys[ HK_RECORD_MOVIE_TO ].getShortcut(), SIGNAL(activated()), this, SLOT(recordMovieAs(void)) );

	movieMenu->addAction(recAsMovAct);

	movieMenu->addSeparator();

	// Movie -> Avi Recording
	aviMenu = movieMenu->addMenu( tr("A&VI Recording") );

	// Movie -> Avi Recording -> Record
	recAviAct = new QAction(tr("&Record"), this);
	//recAviAct->setShortcut( QKeySequence(tr("Shift+F5")));
	recAviAct->setStatusTip(tr("AVI Record Start"));
	recAviAct->setIcon( QIcon(":icons/media-record.png") );
	connect(recAviAct, SIGNAL(triggered()), this, SLOT(aviRecordStart(void)) );
	
	aviMenu->addAction(recAviAct);

	// Movie -> Avi Recording -> Record
	recAsAviAct = new QAction(tr("Record &As"), this);
	//recAsAviAct->setShortcut( QKeySequence(tr("Shift+F5")));
	recAsAviAct->setStatusTip(tr("AVI Record As Start"));
	//recAsAviAct->setIcon( QIcon(":icons/media-record.png") );
	connect(recAsAviAct, SIGNAL(triggered()), this, SLOT(aviRecordAsStart(void)) );
	
	aviMenu->addAction(recAsAviAct);

	// Movie -> Avi Recording -> Stop
	stopAviAct = new QAction(tr("&Stop"), this);
	//stopAviAct->setShortcut( QKeySequence(tr("Shift+F5")));
	stopAviAct->setStatusTip(tr("AVI Record Stop"));
	stopAviAct->setIcon( style()->standardIcon( QStyle::SP_MediaStop ) );
	connect(stopAviAct, SIGNAL(triggered()), this, SLOT(aviRecordStop(void)) );
	
	aviMenu->addAction(stopAviAct);

	// Movie -> Avi Recording -> Video Format
	subMenu = aviMenu->addMenu( tr("Video Format") );

	{
		std::vector <std::string> formatList;
		group   = new QActionGroup(this);

		group->setExclusive(true);

		FCEUD_AviGetFormatOpts( formatList );

		for (size_t i=0; i<formatList.size(); i++)
		{
			act = new QAction(tr( formatList[i].c_str() ), this);

			printf("%s\n", formatList[i].c_str() );

	        	act->setCheckable(true);
	        	group->addAction(act);
			subMenu->addAction(act);

			act->setChecked( aviGetSelVideoFormat() == i );

			// Use Lambda Function to set callback
			connect( act, &QAction::triggered, [ this, i ] { aviVideoFormatChanged( i ); } );
		}

	}

	// Movie -> Avi Recording -> Include Audio
	act = new QAction(tr("Include Audio"), this);
       	act->setCheckable(true);
	act->setChecked( aviGetAudioEnable() );
	connect(act, SIGNAL(triggered(bool)), this, SLOT(aviAudioEnableChange(bool)) );
	aviMenu->addAction(act);

	//-----------------------------------------------------------------------
	// Help
	helpMenu = menubar->addMenu(tr("&Help"));
 
	connect( helpMenu, SIGNAL(aboutToShow(void)), this, SLOT(mainMenuOpen(void)) );
	connect( helpMenu, SIGNAL(aboutToHide(void)), this, SLOT(mainMenuClose(void)) );

	// Help -> About FCEUX
	aboutAct = new QAction(tr("&About FCEUX"), this);
	aboutAct->setStatusTip(tr("About FCEUX"));
	aboutAct->setIcon( style()->standardIcon( QStyle::SP_MessageBoxInformation ) );
	connect(aboutAct, SIGNAL(triggered()), this, SLOT(aboutFCEUX(void)) );
	
	helpMenu->addAction(aboutAct);

	// Help -> About Qt
	aboutActQt = new QAction(tr("About &Qt"), this);
	aboutActQt->setStatusTip(tr("About Qt"));
	aboutActQt->setIcon( style()->standardIcon( QStyle::SP_TitleBarMenuButton ) );
	connect(aboutActQt, SIGNAL(triggered()), this, SLOT(aboutQt(void)) );
	
	helpMenu->addAction(aboutActQt);

	// Help -> Message Log
	msgLogAct = new QAction(tr("&Message Log"), this);
	msgLogAct->setStatusTip(tr("Message Log"));
	msgLogAct->setIcon( style()->standardIcon( QStyle::SP_MessageBoxWarning ) );
	connect(msgLogAct, SIGNAL(triggered()), this, SLOT(openMsgLogWin(void)) );
	
	helpMenu->addAction(msgLogAct);

	// Help -> Documentation
	act = new QAction(tr("&Docs (Online)"), this);
	act->setStatusTip(tr("Documentation"));
	act->setIcon( style()->standardIcon( QStyle::SP_DialogHelpButton ) );
	connect(act, SIGNAL(triggered()), this, SLOT(openOnlineDocs(void)) );
	
	helpMenu->addAction(act);
};
//---------------------------------------------------------------------------
int consoleWin_t::loadVideoDriver( int driverId )
{
	if ( driverId )
	{  // SDL Driver
		if ( viewport_SDL != NULL )
		{  // Already Loaded
			return 0;
		}

		if ( viewport_GL != NULL )
		{
			if ( viewport_GL == centralWidget() )
			{
				takeCentralWidget();
			}
			delete viewport_GL;

			viewport_GL = NULL;
		}
		
		viewport_SDL = new ConsoleViewSDL_t(this);

		setCentralWidget(viewport_SDL);

		viewport_SDL->init();
	}
	else
	{  // OpenGL Driver
		if ( viewport_GL != NULL )
		{  // Already Loaded
			return 0;
		}

		if ( viewport_SDL != NULL )
		{
			if ( viewport_SDL == centralWidget() )
			{
				takeCentralWidget();
			}
			delete viewport_SDL;

			viewport_SDL = NULL;
		}
		viewport_GL = new ConsoleViewGL_t(this);

		setCentralWidget(viewport_GL);
	}
	return 0;
}
//---------------------------------------------------------------------------
void consoleWin_t::clearRomList(void)
{
	std::list <std::string*>::iterator it;

	for (it=romList.begin(); it != romList.end(); it++)
	{
		delete *it;
	}
	romList.clear();
}
//---------------------------------------------------------------------------
void consoleWin_t::buildRecentRomMenu(void)
{
	QAction *act;
	std::string s;
	std::string *sptr;
	char buf[128];

	clearRomList();
	recentRomMenu->clear();

	for (int i=0; i<10; i++)
	{
		sprintf(buf, "SDL.RecentRom%02i", i);

		g_config->getOption( buf, &s);

		//printf("Recent Rom:%i  '%s'\n", i, s.c_str() );

		if ( s.size() > 0 )
		{
			act = new consoleRecentRomAction( tr(s.c_str()), recentRomMenu);

			recentRomMenu->addAction( act );

			connect(act, SIGNAL(triggered()), act, SLOT(activateCB(void)) );

			sptr = new std::string();

			sptr->assign( s.c_str() );

			romList.push_front( sptr );
		}
	}
}
//---------------------------------------------------------------------------
void consoleWin_t::saveRecentRomMenu(void)
{
	int i;
	std::string *s;
	std::list <std::string*>::iterator it;
	char buf[128];

	i = romList.size() - 1;

	for (it=romList.begin(); it != romList.end(); it++)
	{
		s = *it;
		sprintf(buf, "SDL.RecentRom%02i", i);

		g_config->setOption( buf, s->c_str() );

		//printf("Recent Rom:%u  '%s'\n", i, s->c_str() );
		i--;
	}
}
//---------------------------------------------------------------------------
void consoleWin_t::addRecentRom( const char *rom )
{
	std::string *s;
	std::list <std::string*>::iterator match_it;

	for (match_it=romList.begin(); match_it != romList.end(); match_it++)
	{
		s = *match_it;

		if ( s->compare( rom ) == 0 )
		{
			//printf("Found Match: %s\n", rom );
			break;
		}
	}

	if ( match_it != romList.end() )
	{
		s = *match_it;

		romList.erase(match_it);

		romList.push_back(s);
	}
	else
	{
		s = new std::string();

		s->assign( rom );
		
		romList.push_back(s);

		if ( romList.size() > 10 )
		{
			s = romList.front();

			romList.pop_front();

			delete s;
		}
	}

	saveRecentRomMenu();

	recentRomMenuReset = true;
}
//---------------------------------------------------------------------------
void consoleWin_t::toggleMenuVis(void)
{
	if ( menubar->isVisible() )
	{
		menubar->setVisible( false );
	}
	else
	{
		menubar->setVisible( true );
	}
}
//---------------------------------------------------------------------------
void consoleWin_t::closeApp(void)
{
	nes_shm->runEmulator = 0;

	emulatorThread->quit();
	emulatorThread->wait( 1000 );

	fceuWrapperLock();
	fceuWrapperClose();
	fceuWrapperUnLock();

	// LoadGame() checks for an IP and if it finds one begins a network session
	// clear the NetworkIP field so this doesn't happen unintentionally
	g_config->setOption ("SDL.NetworkIP", "");
	g_config->save ();

	//qApp::quit();
	qApp->quit();
}
//---------------------------------------------------------------------------
int  consoleWin_t::showListSelectDialog( const char *title, std::vector <std::string> &l )
{
	if ( QThread::currentThread() == emulatorThread )
	{
		printf("Cannot display list selection dialog from within emulation thread...\n");
		return 0;
	}
	int ret, idx = 0;
	QDialog dialog(this);
	QVBoxLayout *mainLayout;
	QHBoxLayout *hbox;
	QPushButton *okButton, *cancelButton;
	QTreeWidget *tree;
	QTreeWidgetItem *item;

	dialog.setWindowTitle( tr(title) );

	tree = new QTreeWidget();

	tree->setColumnCount(1);

	item = new QTreeWidgetItem();
	item->setText( 0, QString::fromStdString( "File" ) );
	item->setTextAlignment( 0, Qt::AlignLeft);

	tree->setHeaderItem( item );

	tree->header()->setSectionResizeMode( QHeaderView::ResizeToContents );

	for (size_t i=0; i<l.size(); i++)
	{
		item = new QTreeWidgetItem();

		item->setText( 0, QString::fromStdString( l[i] ) );

		item->setTextAlignment( 0, Qt::AlignLeft);

		tree->addTopLevelItem( item );
	}

	mainLayout = new QVBoxLayout();

	hbox         = new QHBoxLayout();
	okButton     = new QPushButton( tr("OK") );
	cancelButton = new QPushButton( tr("Cancel") );

	mainLayout->addWidget( tree );
	mainLayout->addLayout( hbox );
	hbox->addWidget( cancelButton );
	hbox->addWidget(     okButton );

	connect(     okButton, SIGNAL(clicked(void)), &dialog, SLOT(accept(void)) );
	connect( cancelButton, SIGNAL(clicked(void)), &dialog, SLOT(reject(void)) );

	dialog.setLayout( mainLayout );

	ret = dialog.exec();

	if ( ret == QDialog::Accepted )
	{
		idx = 0;

		item = tree->currentItem();

	   if ( item != NULL )
	   {
			idx = tree->indexOfTopLevelItem(item);
		}
	}
	else
	{
		idx = -1;
	}
	return idx;
}
//---------------------------------------------------------------------------

void consoleWin_t::openROMFile(void)
{
	int ret, useNativeFileDialogVal;
	QString filename;
	std::string last;
	char dir[512];
	char *romDir;
	QFileDialog  dialog(this, tr("Open ROM File") );
	QList<QUrl> urls;
	QDir d;

	const QStringList filters(
			{ "All Useable files (*.nes *.NES *.nsf *.NSF *.fds *.FDS *.unf *.UNF *.unif *.UNIF *.zip *.ZIP)",
           "NES files (*.nes *.NES)",
           "NSF files (*.nsf *.NSF)",
           "UNF files (*.unf *.UNF *.unif *.UNIF)",
           "FDS files (*.fds *.FDS)",
           "Any files (*)"
         });

	urls << QUrl::fromLocalFile( QDir::rootPath() );
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).first());
	urls << QUrl::fromLocalFile( QDir( FCEUI_GetBaseDirectory() ).absolutePath() );

	romDir = getenv("FCEUX_ROM_PATH");

	if ( romDir != NULL )
	{
		d.setPath(romDir);

		if ( d.exists() )
		{
			urls << QUrl::fromLocalFile( d.absolutePath() );
		}
	}

	dialog.setFileMode(QFileDialog::ExistingFile);

	dialog.setNameFilters( filters );

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::AllDirs | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Open") );

	g_config->getOption ("SDL.LastOpenFile", &last );

	getDirFromFile( last.c_str(), dir );

	dialog.setDirectory( tr(dir) );

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
	qDebug() << "selected file path : " << filename.toUtf8();

	g_config->setOption ("SDL.LastOpenFile", filename.toStdString().c_str() );

	fceuWrapperLock();
	CloseGame ();
	LoadGame ( filename.toStdString().c_str() );
	fceuWrapperUnLock();

   return;
}

void consoleWin_t::closeROMCB(void)
{
	fceuWrapperLock();
	CloseGame();
	fceuWrapperUnLock();
}

void consoleWin_t::loadNSF(void)
{
	int ret, useNativeFileDialogVal;
	QString filename;
	std::string last;
	char dir[512];
	char *romDir;
	QFileDialog  dialog(this, tr("Load NSF File") );
	QList<QUrl> urls;
	QDir d;

	urls << QUrl::fromLocalFile( QDir::rootPath() );
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).first());
	urls << QUrl::fromLocalFile( QDir( FCEUI_GetBaseDirectory() ).absolutePath() );

	romDir = getenv("FCEUX_ROM_PATH");

	if ( romDir != NULL )
	{
		d.setPath(romDir);

		if ( d.exists() )
		{
			urls << QUrl::fromLocalFile( d.absolutePath() );
		}
	}
	dialog.setFileMode(QFileDialog::ExistingFile);

	dialog.setNameFilter(tr("NSF Sound Files (*.nsf *.NSF) ;; Zip Files (*.zip *.ZIP) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::AllDirs | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Load") );

	g_config->getOption ("SDL.LastOpenNSF", &last );

	getDirFromFile( last.c_str(), dir );

	dialog.setDirectory( tr(dir) );

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
	qDebug() << "selected file path : " << filename.toUtf8();

	g_config->setOption ("SDL.LastOpenNSF", filename.toStdString().c_str() );

	fceuWrapperLock();
	LoadGame( filename.toStdString().c_str() );
	fceuWrapperUnLock();
}

void consoleWin_t::loadStateFrom(void)
{
	int ret, useNativeFileDialogVal;
	QString filename;
	std::string last;
	char dir[512];
	const char *base;
	QFileDialog  dialog(this, tr("Load State From File") );
	QList<QUrl> urls;
	QDir d;

	base = FCEUI_GetBaseDirectory();

	urls << QUrl::fromLocalFile( QDir::rootPath() );
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).first());

	if ( base )
	{
		urls << QUrl::fromLocalFile( QDir( base ).absolutePath() );

		d.setPath( QString(base) + "/fcs");

		if ( d.exists() )
		{
			urls << QUrl::fromLocalFile( d.absolutePath() );
		}

		d.setPath( QString(base) + "/sav");

		if ( d.exists() )
		{
			urls << QUrl::fromLocalFile( d.absolutePath() );
		}
	}


	dialog.setFileMode(QFileDialog::ExistingFile);

	dialog.setNameFilter(tr("FCS & SAV Files (*.sav *.SAV *.fc? *.FC?) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::AllDirs | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Load") );

	g_config->getOption ("SDL.LastLoadStateFrom", &last );

	getDirFromFile( last.c_str(), dir );

	dialog.setDirectory( tr(dir) );

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
	qDebug() << "selected file path : " << filename.toUtf8();

	g_config->setOption ("SDL.LastLoadStateFrom", filename.toStdString().c_str() );

	fceuWrapperLock();
	FCEUI_LoadState( filename.toStdString().c_str() );
	fceuWrapperUnLock();
}

void consoleWin_t::saveStateAs(void)
{
	int ret, useNativeFileDialogVal;
	QString filename;
	std::string last;
	char dir[512];
	const char *base;
	QFileDialog  dialog(this, tr("Save State To File") );
	QList<QUrl> urls;
	QDir d;

	base = FCEUI_GetBaseDirectory();

	urls << QUrl::fromLocalFile( QDir::rootPath() );
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).first());

	if ( base )
	{
		urls << QUrl::fromLocalFile( QDir( base ).absolutePath() );

		d.setPath( QString(base) + "/fcs");

		if ( d.exists() )
		{
			urls << QUrl::fromLocalFile( d.absolutePath() );
		}

		d.setPath( QString(base) + "/sav");

		if ( d.exists() )
		{
			urls << QUrl::fromLocalFile( d.absolutePath() );
		}
	}

	dialog.setFileMode(QFileDialog::AnyFile);

	dialog.setNameFilter(tr("SAV Files (*.sav *.SAV) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::AllDirs | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Save") );
	dialog.setDefaultSuffix( tr(".sav") );

	g_config->getOption ("SDL.LastSaveStateAs", &last );

	if ( last.size() == 0 )
	{
		if ( base )
		{
			last = std::string(base) + "/sav";
		}
	}
	getDirFromFile( last.c_str(), dir );

	dialog.setDirectory( tr(dir) );

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
	qDebug() << "selected file path : " << filename.toUtf8();

	g_config->setOption ("SDL.LastSaveStateAs", filename.toStdString().c_str() );

	fceuWrapperLock();
	FCEUI_SaveState( filename.toStdString().c_str() );
	fceuWrapperUnLock();
}

void consoleWin_t::quickLoad(void)
{
	fceuWrapperLock();
	FCEUI_LoadState( NULL );
	fceuWrapperUnLock();
}

void consoleWin_t::loadState(int slot)
{
	int prevState;
	fceuWrapperLock();
	prevState = FCEUI_SelectState( slot, false );
	FCEUI_LoadState( NULL, true );
	FCEUI_SelectState( prevState, false );
	fceuWrapperUnLock();
}
void consoleWin_t::loadState0(void){ loadState(0); }
void consoleWin_t::loadState1(void){ loadState(1); }
void consoleWin_t::loadState2(void){ loadState(2); }
void consoleWin_t::loadState3(void){ loadState(3); }
void consoleWin_t::loadState4(void){ loadState(4); }
void consoleWin_t::loadState5(void){ loadState(5); }
void consoleWin_t::loadState6(void){ loadState(6); }
void consoleWin_t::loadState7(void){ loadState(7); }
void consoleWin_t::loadState8(void){ loadState(8); }
void consoleWin_t::loadState9(void){ loadState(9); }

void consoleWin_t::quickSave(void)
{
	fceuWrapperLock();
	FCEUI_SaveState( NULL );
	fceuWrapperUnLock();
}

void consoleWin_t::saveState(int slot)
{
	int prevState;
	fceuWrapperLock();
	prevState = FCEUI_SelectState( slot, false );
	FCEUI_SaveState( NULL, true );
	FCEUI_SelectState( prevState, false );
	fceuWrapperUnLock();
}
void consoleWin_t::saveState0(void){ saveState(0); }
void consoleWin_t::saveState1(void){ saveState(1); }
void consoleWin_t::saveState2(void){ saveState(2); }
void consoleWin_t::saveState3(void){ saveState(3); }
void consoleWin_t::saveState4(void){ saveState(4); }
void consoleWin_t::saveState5(void){ saveState(5); }
void consoleWin_t::saveState6(void){ saveState(6); }
void consoleWin_t::saveState7(void){ saveState(7); }
void consoleWin_t::saveState8(void){ saveState(8); }
void consoleWin_t::saveState9(void){ saveState(9); }

void consoleWin_t::changeState(int slot)
{
	fceuWrapperLock();
	FCEUI_SelectState( slot, true );
	fceuWrapperUnLock();
	state[slot]->setChecked(true);
}
void consoleWin_t::changeState0(void){ changeState(0); }
void consoleWin_t::changeState1(void){ changeState(1); }
void consoleWin_t::changeState2(void){ changeState(2); }
void consoleWin_t::changeState3(void){ changeState(3); }
void consoleWin_t::changeState4(void){ changeState(4); }
void consoleWin_t::changeState5(void){ changeState(5); }
void consoleWin_t::changeState6(void){ changeState(6); }
void consoleWin_t::changeState7(void){ changeState(7); }
void consoleWin_t::changeState8(void){ changeState(8); }
void consoleWin_t::changeState9(void){ changeState(9); }

void consoleWin_t::incrementState(void)
{
	fceuWrapperLock();
	FCEUI_SelectStateNext(1);
	fceuWrapperUnLock();
}

void consoleWin_t::decrementState(void)
{
	fceuWrapperLock();
	FCEUI_SelectStateNext(-1);
	fceuWrapperUnLock();
}

void consoleWin_t::mainMenuOpen(void)
{
	//printf("Main Menu Open\n");

	mainMenuEmuWasPaused = FCEUI_EmulationPaused() ? true : false;

	if ( mainMenuPauseWhenActv && !mainMenuEmuPauseSet && !mainMenuEmuWasPaused )
	{
		FCEUI_ToggleEmulationPause();
		mainMenuEmuPauseSet  = true;
	}
}

void consoleWin_t::mainMenuClose(void)
{
	//printf("Main Menu Close\n");

	if ( mainMenuEmuPauseSet )
	{
		bool isPaused = FCEUI_EmulationPaused() ? true : false;

		if ( isPaused != mainMenuEmuWasPaused )
		{
			FCEUI_ToggleEmulationPause();
		}
		mainMenuEmuPauseSet = false;
	}
}

void consoleWin_t::takeScreenShot(void)
{
	fceuWrapperLock();
	FCEUI_SaveSnapshot();
	fceuWrapperUnLock();
}

void consoleWin_t::loadLua(void)
{
#ifdef _S9XLUA_H
	LuaControlDialog_t *luaCtrlWin;

	//printf("Open Lua Control Window\n");
	
   luaCtrlWin = new LuaControlDialog_t(this);
	
   luaCtrlWin->show();
#endif
}

void consoleWin_t::openInputConfWin(void)
{
	//printf("Open Input Config Window\n");
	
	openInputConfWindow(this);
}

void consoleWin_t::openGamePadConfWin(void)
{
	//printf("Open GamePad Config Window\n");
	
	openGamePadConfWindow(this);
}

void consoleWin_t::openGameSndConfWin(void)
{
	ConsoleSndConfDialog_t *sndConfWin;

	//printf("Open Sound Config Window\n");
	
   sndConfWin = new ConsoleSndConfDialog_t(this);
	
   sndConfWin->show();
}

void consoleWin_t::openGameVideoConfWin(void)
{
	ConsoleVideoConfDialog_t *vidConfWin;

	//printf("Open Video Config Window\n");
	
   vidConfWin = new ConsoleVideoConfDialog_t(this);
	
   vidConfWin->show();
}

void consoleWin_t::openHotkeyConfWin(void)
{
	HotKeyConfDialog_t *hkConfWin;

	//printf("Open Hot Key Config Window\n");
	
   hkConfWin = new HotKeyConfDialog_t(this);
	
   hkConfWin->show();
}

void consoleWin_t::openPaletteConfWin(void)
{
	PaletteConfDialog_t *paletteConfWin;

	//printf("Open Palette Config Window\n");
	
   paletteConfWin = new PaletteConfDialog_t(this);
	
   paletteConfWin->show();
}

void consoleWin_t::openGuiConfWin(void)
{
	GuiConfDialog_t *guiConfWin;

	//printf("Open GUI Config Window\n");
	
   guiConfWin = new GuiConfDialog_t(this);
	
   guiConfWin->show();
}

void consoleWin_t::openTimingConfWin(void)
{
	TimingConfDialog_t *tmConfWin;

	//printf("Open Timing Config Window\n");
	
   tmConfWin = new TimingConfDialog_t(this);
	
   tmConfWin->show();
}

void consoleWin_t::openTimingStatWin(void)
{
	FrameTimingDialog_t *tmStatWin;

	//printf("Open Timing Statistics Window\n");
	
   tmStatWin = new FrameTimingDialog_t(this);
	
   tmStatWin->show();
}

void consoleWin_t::openPaletteEditorWin(void)
{
	PaletteEditorDialog_t *win;

	//printf("Open Palette Editor Window\n");
	
   win = new PaletteEditorDialog_t(this);
	
   win->show();
}

void consoleWin_t::openMovieOptWin(void)
{
	MovieOptionsDialog_t *win;

	//printf("Open Movie Options Window\n");
	
   win = new MovieOptionsDialog_t(this);
	
   win->show();
}

void consoleWin_t::openCheats(void)
{
	//printf("Open GUI Cheat Window\n");
	
   openCheatDialog(this);
}

void consoleWin_t::openRamWatch(void)
{
	RamWatchDialog_t *ramWatchWin;

	//printf("Open GUI RAM Watch Window\n");
	
   ramWatchWin = new RamWatchDialog_t(this);
	
   ramWatchWin->show();
}

void consoleWin_t::openRamSearch(void)
{
	//printf("Open GUI RAM Search Window\n");
	openRamSearchWindow(this);
}

void consoleWin_t::openDebugWindow(void)
{
	ConsoleDebugger *debugWin;

	//printf("Open GUI 6502 Debugger Window\n");
	
   debugWin = new ConsoleDebugger(this);
	
   debugWin->show();
}

void consoleWin_t::openHexEditor(void)
{
	HexEditorDialog_t *hexEditWin;

	//printf("Open GUI Hex Editor Window\n");
	
   hexEditWin = new HexEditorDialog_t(this);
	
   hexEditWin->show();
}

void consoleWin_t::openPPUViewer(void)
{
	//printf("Open GUI PPU Viewer Window\n");
	
	openPPUViewWindow(this);
}

void consoleWin_t::openOAMViewer(void)
{
	//printf("Open GUI OAM Viewer Window\n");
	
	openOAMViewWindow(this);
}

void consoleWin_t::openNTViewer(void)
{
	//printf("Open GUI Name Table Viewer Window\n");
	
	openNameTableViewWindow(this);
}

void consoleWin_t::openCodeDataLogger(void)
{
	CodeDataLoggerDialog_t *cdlWin;

	//printf("Open Code Data Logger Window\n");
	
   cdlWin = new CodeDataLoggerDialog_t(this);
	
   cdlWin->show();
}

void consoleWin_t::openGGEncoder(void)
{
	GameGenieDialog_t *win;

	//printf("Open Game Genie Window\n");
	
   win = new GameGenieDialog_t(this);
	
   win->show();
}

void consoleWin_t::openNesHeaderEditor(void)
{
	iNesHeaderEditor_t *win;

	//printf("Open iNES Header Editor Window\n");
	
   win = new iNesHeaderEditor_t(this);
	
	if ( win->isInitialized() )
	{
		win->show();
	}
	else
	{
		delete win;
	}
}

void consoleWin_t::openTraceLogger(void)
{
	openTraceLoggerWindow(this);
}

void consoleWin_t::toggleAutoResume(void)
{
   //printf("Auto Resume: %i\n", autoResume->isChecked() );

	g_config->setOption ("SDL.AutoResume", (int) autoResume->isChecked() );

	AutoResumePlay = autoResume->isChecked();
}

void consoleWin_t::toggleFullscreen(void)
{
	if ( isFullScreen() )
	{
		showNormal();
	}
	else
	{
		showFullScreen();
	}
}

void consoleWin_t::toggleFamKeyBrdEnable(void)
{
	toggleFamilyKeyboardFunc();
}

void consoleWin_t::warnAmbiguousShortcut( QShortcut *shortcut)
{
	char stmp[256];
	std::string msg;
	int c = 0;

	sprintf( stmp, "Error: Ambiguous Shortcut Activation for Key Sequence: '%s'\n", shortcut->key().toString().toStdString().c_str() );

	msg.assign( stmp );

	for (int i = 0; i < HK_MAX; i++)
	{
		QShortcut *sc = Hotkeys[i].getShortcut();

		if ( sc == NULL )
		{
			continue;
		}

		if ( (sc == shortcut) || (shortcut->key().matches( sc->key() ) == QKeySequence::ExactMatch) )
		{
			if ( c == 0 )
			{
				msg.append("Hot Key Conflict: "); c++;
			}
			else
			{
				msg.append(" and "); c++;
			}
			msg.append( Hotkeys[i].getConfigName() );
		}
	}
	QueueErrorMsgWindow( msg.c_str() );
}

void consoleWin_t::powerConsoleCB(void)
{
	fceuWrapperLock();
	FCEUI_PowerNES();
	fceuWrapperUnLock();
   return;
}

void consoleWin_t::consoleHardReset(void)
{
	fceuWrapperLock();
	fceuWrapperHardReset();
	fceuWrapperUnLock();
   return;
}

void consoleWin_t::consoleSoftReset(void)
{
	fceuWrapperLock();
	fceuWrapperSoftReset();
	fceuWrapperUnLock();
   return;
}

void consoleWin_t::consolePause(void)
{
	fceuWrapperLock();
	fceuWrapperTogglePause();
	fceuWrapperUnLock();

	mainMenuEmuPauseSet = false;
   return;
}

void consoleWin_t::setRegion(int region)
{
	int currentRegion;

	g_config->setOption ("SDL.PAL", region);
	g_config->save ();

	currentRegion = FCEUI_GetRegion();

	if ( currentRegion != region )
	{
		fceuWrapperLock();
		FCEUI_SetRegion (region, true);
		fceuWrapperUnLock();
	}
	return;
}

void consoleWin_t::setRegionNTSC(void)
{
	setRegion(0);
	return;
}

void consoleWin_t::setRegionPAL(void)
{
	setRegion(1);
	return;
}

void consoleWin_t::setRegionDendy(void)
{
	setRegion(2);
	return;
}

void consoleWin_t::setRamInit0(void)
{
	RAMInitOption = 0;

	g_config->setOption ("SDL.RamInitMethod", RAMInitOption);
	return;
}

void consoleWin_t::setRamInit1(void)
{
	RAMInitOption = 1;

	g_config->setOption ("SDL.RamInitMethod", RAMInitOption);
	return;
}

void consoleWin_t::setRamInit2(void)
{
	RAMInitOption = 2;

	g_config->setOption ("SDL.RamInitMethod", RAMInitOption);
	return;
}

void consoleWin_t::setRamInit3(void)
{
	RAMInitOption = 3;

	g_config->setOption ("SDL.RamInitMethod", RAMInitOption);
	return;
}

void consoleWin_t::toggleGameGenie(bool checked)
{
	int gg_enabled;

	fceuWrapperLock();
	g_config->getOption ("SDL.GameGenie", &gg_enabled);
	g_config->setOption ("SDL.GameGenie", !gg_enabled);
	g_config->save ();
	FCEUI_SetGameGenie (gg_enabled);
	fceuWrapperUnLock();
   return;
}

void consoleWin_t::loadGameGenieROM(void)
{
	int ret, useNativeFileDialogVal;
	QString filename;
	std::string last;
	char dir[512];
	QFileDialog  dialog(this, tr("Open Game Genie ROM") );
	QList<QUrl> urls;

	urls << QUrl::fromLocalFile( QDir::rootPath() );
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).first());

	dialog.setFileMode(QFileDialog::ExistingFile);

	dialog.setNameFilter(tr("GG ROM File (gg.rom  *Genie*.nes) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::AllDirs | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Load") );

	g_config->getOption ("SDL.LastOpenFile", &last );

	getDirFromFile( last.c_str(), dir );

	dialog.setDirectory( tr(dir) );

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
	qDebug() << "selected file path : " << filename.toUtf8();

	g_config->setOption ("SDL.LastOpenFile", filename.toStdString().c_str() );

	// copy file to proper place (~/.fceux/gg.rom)
	std::ifstream f1 ( filename.toStdString().c_str(), std::fstream::binary);
	std::string fn_out = FCEU_MakeFName (FCEUMKF_GGROM, 0, "");
	std::ofstream f2 (fn_out.c_str (),
	std::fstream::trunc | std::fstream::binary);
	f2 << f1.rdbuf ();

   return;
}

void consoleWin_t::insertCoin(void)
{
	fceuWrapperLock();
	FCEUI_VSUniCoin();
	fceuWrapperUnLock();
   return;
}

void consoleWin_t::fdsSwitchDisk(void)
{
	fceuWrapperLock();
	FCEU_FDSSelect();
	fceuWrapperUnLock();
   return;
}

void consoleWin_t::fdsEjectDisk(void)
{
	fceuWrapperLock();
	FCEU_FDSInsert();
	fceuWrapperUnLock();
   return;
}

void consoleWin_t::fdsLoadBiosFile(void)
{
	int ret, useNativeFileDialogVal;
	QString filename;
	std::string last;
	char dir[512];
	QFileDialog  dialog(this, tr("Load FDS BIOS (disksys.rom)") );
	QList<QUrl> urls;

	urls << QUrl::fromLocalFile( QDir::rootPath() );
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).first());

	dialog.setFileMode(QFileDialog::ExistingFile);

	dialog.setNameFilter(tr("ROM files (*.rom *.ROM) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::AllDirs | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Load") );

	g_config->getOption ("SDL.LastOpenFile", &last );

	getDirFromFile( last.c_str(), dir );

	dialog.setDirectory( tr(dir) );

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
	qDebug() << "selected file path : " << filename.toUtf8();

	// copy BIOS file to proper place (~/.fceux/disksys.rom)
	std::ifstream fdsBios (filename.toStdString().c_str(), std::fstream::binary);
	std::string output_filename =
		FCEU_MakeFName (FCEUMKF_FDSROM, 0, "");
	std::ofstream outFile (output_filename.c_str (),
			       std::fstream::trunc | std::fstream::
			       binary);
	outFile << fdsBios.rdbuf ();
	if (outFile.fail ())
	{
		FCEUD_PrintError ("Error copying the FDS BIOS file.");
	}
	else
	{
		printf("Famicom Disk System BIOS loaded.  If you are you having issues, make sure your BIOS file is 8KB in size.\n");
	}

   return;
}

void consoleWin_t::emuSpeedUp(void)
{
   IncreaseEmulationSpeed();
}

void consoleWin_t::emuSlowDown(void)
{
   DecreaseEmulationSpeed();
}

void consoleWin_t::emuSlowestSpd(void)
{
   FCEUD_SetEmulationSpeed( EMUSPEED_SLOWEST );
}

void consoleWin_t::emuNormalSpd(void)
{
   FCEUD_SetEmulationSpeed( EMUSPEED_NORMAL );
}

void consoleWin_t::emuFastestSpd(void)
{
   FCEUD_SetEmulationSpeed( EMUSPEED_FASTEST );
}

void consoleWin_t::emuCustomSpd(void)
{
	int ret;
	QInputDialog dialog(this);

	dialog.setWindowTitle( tr("Emulation Speed") );
	dialog.setLabelText( tr("Enter a percentage from 1 to 1000.") );
	dialog.setOkButtonText( tr("Ok") );
	dialog.setInputMode( QInputDialog::IntInput );
	dialog.setIntRange( 1, 1000 );
	dialog.setIntValue( 100 );
	
	ret = dialog.exec();
	
	if ( QDialog::Accepted == ret )
	{
	   int spdPercent;
	
	   spdPercent = dialog.intValue();
	
	   CustomEmulationSpeed( spdPercent );
	}
}

void consoleWin_t::emuSetFrameAdvDelay(void)
{
	int ret;
	QInputDialog dialog(this);

	dialog.setWindowTitle( tr("Frame Advance Delay") );
	dialog.setLabelText( tr("How much time should elapse before holding the frame advance unpauses the simulation?") );
	dialog.setOkButtonText( tr("Ok") );
	dialog.setInputMode( QInputDialog::IntInput );
	dialog.setIntRange( 0, 1000 );
	dialog.setIntValue( frameAdvance_Delay );
	
	ret = dialog.exec();
	
	if ( QDialog::Accepted == ret )
	{
	   frameAdvance_Delay = dialog.intValue();
	}
}

void consoleWin_t::setAutoFireOnFrames(void)
{
	int ret;
	QInputDialog dialog(this);

	dialog.setWindowTitle( tr("AutoFire Pattern ON Frames") );
	dialog.setLabelText( tr("Specify desired number of ON frames in autofire pattern:") );
	dialog.setOkButtonText( tr("Ok") );
	dialog.setInputMode( QInputDialog::IntInput );
	dialog.setIntRange( 1, 30 );
	dialog.setIntValue( autoFireOnFrames );
	
	ret = dialog.exec();
	
	if ( QDialog::Accepted == ret )
	{
	   autoFireOnFrames = dialog.intValue();
	}
}

void consoleWin_t::setAutoFireOffFrames(void)
{
	int ret;
	QInputDialog dialog(this);

	dialog.setWindowTitle( tr("AutoFire Pattern OFF Frames") );
	dialog.setLabelText( tr("Specify desired number of OFF frames in autofire pattern:") );
	dialog.setOkButtonText( tr("Ok") );
	dialog.setInputMode( QInputDialog::IntInput );
	dialog.setIntRange( 1, 30 );
	dialog.setIntValue( autoFireOffFrames );
	
	ret = dialog.exec();
	
	if ( QDialog::Accepted == ret )
	{
	   autoFireOffFrames = dialog.intValue();
	}
}

void consoleWin_t::incrSoundVolume(void)
{
	fceuWrapperLock();
	FCEUD_SoundVolumeAdjust( 1);
	fceuWrapperUnLock();
}

void consoleWin_t::decrSoundVolume(void)
{
	fceuWrapperLock();
	FCEUD_SoundVolumeAdjust(-1);
	fceuWrapperUnLock();
}

void consoleWin_t::toggleLagCounterDisplay(void)
{
	fceuWrapperLock();
	lagCounterDisplay = !lagCounterDisplay;
	fceuWrapperUnLock();
}

void consoleWin_t::toggleFrameAdvLagSkip(void)
{
	fceuWrapperLock();
	frameAdvanceLagSkip = !frameAdvanceLagSkip;
	FCEUI_DispMessage ("Skipping lag in Frame Advance %sabled.", 0, frameAdvanceLagSkip ? "en" : "dis");
	fceuWrapperUnLock();
}

void consoleWin_t::toggleMovieBindSaveState(void)
{
	fceuWrapperLock();
	bindSavestate = !bindSavestate;
	FCEUI_DispMessage ("Savestate binding to movie %sabled.", 0, bindSavestate ? "en" : "dis");
	fceuWrapperUnLock();
}

void consoleWin_t::toggleMovieFrameDisplay(void)
{
	fceuWrapperLock();
	FCEUI_MovieToggleFrameDisplay();
	fceuWrapperUnLock();
}

void consoleWin_t::toggleMovieReadWrite(void)
{
	fceuWrapperLock();
	FCEUI_SetMovieToggleReadOnly (!FCEUI_GetMovieToggleReadOnly ());
	fceuWrapperUnLock();
}

void consoleWin_t::toggleInputDisplay(void)
{
	fceuWrapperLock();
	FCEUI_ToggleInputDisplay();
	g_config->setOption ("SDL.InputDisplay", input_display);
	fceuWrapperUnLock();
}

void consoleWin_t::toggleBackground(void)
{
	bool fgOn, bgOn;
	fceuWrapperLock();
	FCEUI_GetRenderPlanes( fgOn,  bgOn );
	FCEUI_SetRenderPlanes( fgOn, !bgOn );
	fceuWrapperUnLock();
}

void consoleWin_t::toggleForeground(void)
{
	bool fgOn, bgOn;
	fceuWrapperLock();
	FCEUI_GetRenderPlanes(  fgOn, bgOn );
	FCEUI_SetRenderPlanes( !fgOn, bgOn );
	fceuWrapperUnLock();
}

void consoleWin_t::toggleTurboMode(void)
{
	NoWaiting ^= 1;
}

void consoleWin_t::openMovie(void)
{
	MoviePlayDialog_t *win;

	win = new MoviePlayDialog_t(this);

	win->show();
}

void consoleWin_t::playMovieFromBeginning(void)
{
	fceuWrapperLock();
	FCEUI_MoviePlayFromBeginning();
	fceuWrapperUnLock();
}

void consoleWin_t::stopMovie(void)
{
	fceuWrapperLock();
	FCEUI_StopMovie();
	fceuWrapperUnLock();
   return;
}

void consoleWin_t::recordMovie(void)
{
	fceuWrapperLock();
	if (fceuWrapperGameLoaded())
	{
		std::string name = FCEU_MakeFName (FCEUMKF_MOVIE, 0, 0);
		FCEUI_printf ("Recording movie to %s\n", name.c_str ());
		FCEUI_SaveMovie (name.c_str (), MOVIE_FLAG_NONE, L"");
	}
	fceuWrapperUnLock();
   return;
}

void consoleWin_t::recordMovieAs(void)
{
	int ret, useNativeFileDialogVal;
	QString filename;
	std::string last;
	char dir[512];
	QFileDialog  dialog(this, tr("Save FM2 Movie for Recording") );

	dialog.setFileMode(QFileDialog::AnyFile);

	dialog.setNameFilter(tr("FM2 Movies (*.fm2) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::AllDirs | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Save") );

	g_config->getOption ("SDL.LastOpenMovie", &last );

	getDirFromFile( last.c_str(), dir );

	dialog.setDirectory( tr(dir) );

	// Check config option to use native file dialog or not
	g_config->getOption ("SDL.UseNativeFileDialog", &useNativeFileDialogVal);

	dialog.setOption(QFileDialog::DontUseNativeDialog, !useNativeFileDialogVal);

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
	qDebug() << "selected file path : " << filename.toUtf8();

	int pauseframe;
	g_config->getOption ("SDL.PauseFrame", &pauseframe);
	g_config->setOption ("SDL.PauseFrame", 0);

	FCEUI_printf ("Recording movie to %s\n", filename.toStdString().c_str() );

	fceuWrapperLock();
	std::string s = GetUserText ("Author name");
	std::wstring author (s.begin (), s.end ());

	FCEUI_SaveMovie ( filename.toStdString().c_str(), MOVIE_FLAG_NONE, author);
	fceuWrapperUnLock();

	return;
}

void consoleWin_t::aviRecordStart(void)
{
	if ( !aviRecordRunning() )
	{
		fceuWrapperLock();
		aviRecordOpenFile(NULL);
		aviDiskThread->start();
		fceuWrapperUnLock();
	}
}

void consoleWin_t::aviRecordAsStart(void)
{
	if ( aviRecordRunning() )
	{
		return;
	}
	int ret, useNativeFileDialogVal;
	QString filename;
	std::string last;
	//char dir[512];
	const char *base;
	QFileDialog  dialog(this, tr("Save AVI Movie for Recording") );
	QList<QUrl> urls;
	QDir d;

	dialog.setFileMode(QFileDialog::AnyFile);

	dialog.setNameFilter(tr("AVI Movies (*.avi) ;; All files (*)"));

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

		d.setPath( QString(base) + "/avi");

		if ( d.exists() )
		{
			urls << QUrl::fromLocalFile( d.absolutePath() );
		}

		dialog.setDirectory( d.absolutePath() );
	}
	dialog.setDefaultSuffix( tr(".avi") );

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
	qDebug() << "selected file path : " << filename.toUtf8();

	FCEUI_printf ("AVI Recording movie to %s\n", filename.toStdString().c_str() );

	fceuWrapperLock();
	aviRecordOpenFile( filename.toStdString().c_str() );
	aviDiskThread->start();
	fceuWrapperUnLock();
}

void consoleWin_t::aviRecordStop(void)
{
	if ( aviRecordRunning() )
	{
		fceuWrapperLock();
		aviDiskThread->requestInterruption();
		aviDiskThread->quit();
		aviDiskThread->wait(10000);
		fceuWrapperUnLock();
	}
}

void consoleWin_t::aviAudioEnableChange(bool checked)
{
	aviSetAudioEnable( checked );

	return;
}

void consoleWin_t::aviVideoFormatChanged(int idx)
{
	aviSetSelVideoFormat(idx);
}

void consoleWin_t::aboutFCEUX(void)
{
	AboutWindow *aboutWin;

	//printf("About FCEUX Window\n");
	
	aboutWin = new AboutWindow(this);
	
	aboutWin->show();
	return;
}

void consoleWin_t::aboutQt(void)
{
	//printf("About Qt Window\n");
	
	QMessageBox::aboutQt(this);

	//printf("About Qt Destroyed\n");
	return;
}

void consoleWin_t::openMsgLogWin(void)
{
	//printf("Open Message Log Window\n");
	MsgLogViewDialog_t *msgLogWin;
	
	msgLogWin = new MsgLogViewDialog_t(this);

	msgLogWin->show();

	return;
}

void consoleWin_t::openOnlineDocs(void)
{
	if ( QDesktopServices::openUrl( QUrl("http://fceux.com/web/help/fceux.html") ) == false )
	{
		QueueErrorMsgWindow("Error: Failed to open link to: http://fceux.com/web/help/fceux.html");
	}
	return;
}

#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
int consoleWin_t::setNicePriority( int value )
{
	int ret = 0;
#if defined(__linux__) || defined(__unix__)

	if ( value < -20 )
	{
		value = -20;
	}
	else if ( value > 19 )
	{
		value =  19;
	}

	if ( ::setpriority( PRIO_PROCESS, getpid(), value ) )
	{
		perror("Emulator thread setpriority error: ");
		ret = -1;
	}
#elif defined(__APPLE__)

	if ( value < -20 )
	{
		value = -20;
	}
	else if ( value > 20 )
	{
		value =  20;
	}

	if ( ::setpriority( PRIO_PROCESS, getpid(), value ) )
	{
		perror("Emulator thread setpriority error: ");
		ret = -1;
	}
#endif
	return ret;
}

int consoleWin_t::getNicePriority(void)
{
	return ::getpriority( PRIO_PROCESS, getpid() );
}

int consoleWin_t::getMinSchedPriority(void)
{
	int policy, prio;

	if ( getSchedParam( policy, prio ) )
	{
		return 0;
	}
	return sched_get_priority_min( policy );
}

int consoleWin_t::getMaxSchedPriority(void)
{
	int policy, prio;

	if ( getSchedParam( policy, prio ) )
	{
		return 0;
	}
	return sched_get_priority_max( policy );
}

int consoleWin_t::getSchedParam( int &policy, int &priority )
{
	int ret = 0;

#if defined(__linux__) || defined(__unix__)
	struct sched_param  p;

	policy = sched_getscheduler( getpid() );

	if ( sched_getparam( getpid(), &p ) )
	{
		perror("GUI thread sched_getparam error: ");
		ret = -1;
		priority = 0;
	}
	else
	{
		priority = p.sched_priority;
	}

#elif defined(__APPLE__)
	struct sched_param  p;

	if ( pthread_getschedparam( pthread_self(), &policy, &p ) )
	{
		perror("GUI thread pthread_getschedparam error: ");
		ret = -1;
		priority = 0;
	}
	else
	{
		priority = p.sched_priority;
	}
#endif
	return ret;
}

int consoleWin_t::setSchedParam( int policy, int priority )
{
	int ret = 0;
#if defined(__linux__) || defined(__unix__)
	struct sched_param  p;
	int minPrio, maxPrio;

	minPrio = sched_get_priority_min( policy );
	maxPrio = sched_get_priority_max( policy );

	if ( priority < minPrio )
	{
		priority = minPrio;
	}
	else if ( priority > maxPrio )
	{
		priority = maxPrio;
	}
	p.sched_priority = priority;

	if ( sched_setscheduler( getpid(), policy, &p ) )
	{
		perror("GUI thread sched_setscheduler error");
		ret = -1;
	}
#elif defined(__APPLE__)
	struct sched_param  p;
	int minPrio, maxPrio;

	minPrio = sched_get_priority_min( policy );
	maxPrio = sched_get_priority_max( policy );

	if ( priority < minPrio )
	{
		priority = minPrio;
	}
	else if ( priority > maxPrio )
	{
		priority = maxPrio;
	}
	p.sched_priority = priority;

	if ( ::pthread_setschedparam( pthread_self(), policy, &p ) != 0 )
	{
		perror("GUI thread pthread_setschedparam error: ");
	}
#endif
	return ret;
}
#endif

void consoleWin_t::syncActionConfig( QAction *act, const char *property )
{
	if ( act->isCheckable() )
	{
		int enable;
		g_config->getOption ( property, &enable);

		act->setChecked( enable ? true : false );
	}
}

void consoleWin_t::updatePeriodic(void)
{
	
	// Process all events before attempting to render viewport
	QCoreApplication::processEvents();

	// Update Input Devices
	FCEUD_UpdateInput();
	
	// RePaint Game Viewport
	if ( nes_shm->blitUpdated )
	{
		nes_shm->blitUpdated = 0;

		if ( viewport_SDL )
		{
			viewport_SDL->transfer2LocalBuffer();
			viewport_SDL->render();
		}
		else
		{
			viewport_GL->transfer2LocalBuffer();
			viewport_GL->update();
		}
	}

	// Low Rate Updates
	if ( (updateCounter % 30) == 0 )
	{
		// Keep region menu selection sync'd to actual state
		int actRegion = FCEUI_GetRegion();

		if ( !region[ actRegion ]->isChecked() )
		{
			region[ actRegion ]->setChecked(true);
		}

		powerAct->setEnabled( FCEU_IsValidUI( FCEUI_POWER ) );
		resetAct->setEnabled( FCEU_IsValidUI( FCEUI_RESET ) );
		sresetAct->setEnabled( FCEU_IsValidUI( FCEUI_RESET ) );
		playMovBeginAct->setEnabled( FCEU_IsValidUI( FCEUI_PLAYFROMBEGINNING ) );
		insCoinAct->setEnabled( FCEU_IsValidUI( FCEUI_INSERT_COIN ) );
		fdsSwitchAct->setEnabled( FCEU_IsValidUI( FCEUI_SWITCH_DISK ) );
		fdsEjectAct->setEnabled( FCEU_IsValidUI( FCEUI_EJECT_DISK ) );
		stopMovAct->setEnabled( FCEU_IsValidUI( FCEUI_STOPMOVIE ) );
		recentRomMenu->setEnabled( !recentRomMenu->isEmpty() );
		quickLoadAct->setEnabled( FCEU_IsValidUI( FCEUI_QUICKLOAD ) );
		quickSaveAct->setEnabled( FCEU_IsValidUI( FCEUI_QUICKSAVE ) );
		loadStateAct->setEnabled( FCEU_IsValidUI( FCEUI_LOADSTATE ) );
		saveStateAct->setEnabled( FCEU_IsValidUI( FCEUI_SAVESTATE ) );
		openMovAct->setEnabled( FCEU_IsValidUI( FCEUI_PLAYMOVIE ) );
		recMovAct->setEnabled( FCEU_IsValidUI( FCEUI_RECORDMOVIE ) );
		recAsMovAct->setEnabled( FCEU_IsValidUI( FCEUI_RECORDMOVIE ) );
		recAviAct->setEnabled( FCEU_IsValidUI( FCEUI_RECORDMOVIE ) );
		recAsAviAct->setEnabled( FCEU_IsValidUI( FCEUI_RECORDMOVIE ) );
		stopAviAct->setEnabled( FCEU_IsValidUI( FCEUI_STOPAVI ) );
	}

	if ( errorMsgValid )
	{
		showErrorMsgWindow();
		errorMsgValid = false;
	}

	if ( recentRomMenuReset )
	{
		fceuWrapperLock();
		buildRecentRomMenu();
		recentRomMenuReset = false;
		fceuWrapperUnLock();
	}

	if ( closeRequested )
	{
		closeApp();
		closeRequested = false;
	}

	updateCounter++;

   return;
}

emulatorThread_t::emulatorThread_t( QObject *parent )
	: QThread(parent)
{
	#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
	pself = 0;
	#endif

}

#if defined(__linux__) 
#ifndef SYS_gettid
#error "SYS_gettid unavailable on this system"
#endif

#define gettid() ((pid_t)syscall(SYS_gettid))
#endif


void emulatorThread_t::init(void)
{
	int opt;

	#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
	if ( pthread_self() == (pthread_t)QThread::currentThreadId() )
	{
		pself = pthread_self();
		//printf("EMU is using PThread: %p\n", (void*)pself);
	}
	#endif

	#if defined(__linux__)
	pid = gettid();
	#elif defined(__APPLE__) || defined(__unix__)
	pid = getpid();
	#endif

	g_config->getOption( "SDL.SetSchedParam", &opt );

	if ( opt )
	{
		#ifndef WIN32
		int policy, prio, nice;

		g_config->getOption( "SDL.EmuSchedPolicy", &policy );
		g_config->getOption( "SDL.EmuSchedPrioRt", &prio   );
		g_config->getOption( "SDL.EmuSchedNice"  , &nice   );

		setNicePriority( nice );

		setSchedParam( policy, prio );
		#endif
	}
}

void emulatorThread_t::setPriority( QThread::Priority priority_req )
{
	//printf("New Priority: %i \n", priority_req );
	//printf("Old Priority: %i \n", priority() );

	QThread::setPriority( priority_req );

	//printf("Set Priority: %i \n", priority() );
}

#if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
int emulatorThread_t::setNicePriority( int value )
{
	int ret = 0;
#if defined(__linux__) || defined(__unix__)

	if ( value < -20 )
	{
		value = -20;
	}
	else if ( value > 19 )
	{
		value =  19;
	}

	if ( ::setpriority( PRIO_PROCESS, pid, value ) )
	{
		perror("Emulator thread setpriority error: ");
		ret = -1;
	}
#elif defined(__APPLE__)

	if ( value < -20 )
	{
		value = -20;
	}
	else if ( value > 20 )
	{
		value =  20;
	}

	if ( ::setpriority( PRIO_PROCESS, pid, value ) )
	{
		perror("Emulator thread setpriority error: ");
		ret = -1;
	}
#endif
	return ret;
}

int emulatorThread_t::getNicePriority(void)
{
	return ::getpriority( PRIO_PROCESS, pid );
}

int emulatorThread_t::getMinSchedPriority(void)
{
	int policy, prio;

	if ( getSchedParam( policy, prio ) )
	{
		return 0;
	}
	return sched_get_priority_min( policy );
}

int emulatorThread_t::getMaxSchedPriority(void)
{
	int policy, prio;

	if ( getSchedParam( policy, prio ) )
	{
		return 0;
	}
	return sched_get_priority_max( policy );
}

int emulatorThread_t::getSchedParam( int &policy, int &priority )
{
	struct sched_param  p;

	if ( pthread_getschedparam( pself, &policy, &p ) )
	{
		perror("Emulator thread pthread_getschedparam error: ");
		return -1;
	}
	priority = p.sched_priority;

	return 0;
}

int emulatorThread_t::setSchedParam( int policy, int priority )
{
	int ret = 0;
#if defined(__linux__) || defined(__unix__)
	struct sched_param  p;
	int minPrio, maxPrio;

	minPrio = sched_get_priority_min( policy );
	maxPrio = sched_get_priority_max( policy );

	if ( priority < minPrio )
	{
		priority = minPrio;
	}
	else if ( priority > maxPrio )
	{
		priority = maxPrio;
	}
	p.sched_priority = priority;

	if ( ::pthread_setschedparam( pself, policy, &p ) != 0 )
	{
		perror("Emulator thread pthread_setschedparam error: ");
		ret = -1;
	}

#elif defined(__APPLE__)
	struct sched_param  p;
	int minPrio, maxPrio;

	minPrio = sched_get_priority_min( policy );
	maxPrio = sched_get_priority_max( policy );

	if ( priority < minPrio )
	{
		priority = minPrio;
	}
	else if ( priority > maxPrio )
	{
		priority = maxPrio;
	}
	p.sched_priority = priority;

	if ( ::pthread_setschedparam( pself, policy, &p ) != 0 )
	{
		perror("Emulator thread pthread_setschedparam error: ");
	}
#endif
	return ret;
}
#endif

void emulatorThread_t::run(void)
{
	printf("Emulator Start\n");
	nes_shm->runEmulator = 1;

	init();

	while ( nes_shm->runEmulator )
	{
		fceuWrapperUpdate();
	}
	printf("Emulator Exit\n");
	emit finished();
}

//-----------------------------------------------------------------------------
// Custom QMenuBar for Console
//-----------------------------------------------------------------------------
consoleMenuBar::consoleMenuBar(QWidget *parent)
	: QMenuBar(parent)
{

}
consoleMenuBar::~consoleMenuBar(void)
{

}

void consoleMenuBar::keyPressEvent(QKeyEvent *event)
{
	QMenuBar::keyPressEvent(event);

	// Force de-focus of menu bar when escape key is pressed.
	// This prevents the menubar from hi-jacking keyboard input focus
	// when using menu accelerators
	if ( event->key() == Qt::Key_Escape )
	{
		((QWidget*)parent())->setFocus();
	}
	event->accept();
}

void consoleMenuBar::keyReleaseEvent(QKeyEvent *event)
{
	QMenuBar::keyReleaseEvent(event);

	event->accept();
}
//-----------------------------------------------------------------------------
//----------------------------------------------------------------------------
consoleRecentRomAction::consoleRecentRomAction(QString desc, QWidget *parent)
	: QAction( desc, parent )
{
	path = desc.toStdString();
}
//----------------------------------------------------------------------------
consoleRecentRomAction::~consoleRecentRomAction(void)
{
	//printf("Recent ROM Menu Action Deleted\n");
}
//----------------------------------------------------------------------------
void consoleRecentRomAction::activateCB(void)
{
	printf("Activate Recent ROM: %s \n", path.c_str() );

	fceuWrapperLock();
	CloseGame ();
	LoadGame ( path.c_str() );
	fceuWrapperUnLock();
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
