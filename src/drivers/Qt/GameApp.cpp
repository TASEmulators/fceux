// GameApp.cpp
//
#include <QFileDialog>

#include "Qt/GameApp.h"
#include "Qt/GamePadConf.h"
#include "Qt/fceuWrapper.h"
#include "Qt/keyscan.h"

gameWin_t::gameWin_t(QWidget *parent)
	: QMainWindow( parent )
{

	createMainMenu();

	viewport = new gameViewGL_t(this);
	//viewport = new gameViewSDL_t(this);

   setCentralWidget(viewport);

	gameTimer = new QTimer( this );

	connect( gameTimer, &QTimer::timeout, this, &gameWin_t::runGameFrame );

	gameTimer->setTimerType( Qt::PreciseTimer );
	gameTimer->start( 10 );

   gamePadConfWin = NULL;
}

gameWin_t::~gameWin_t(void)
{
   if ( gamePadConfWin != NULL )
   {
      gamePadConfWin->closeWindow();
   }
	fceuWrapperClose();

	delete viewport;
}

void gameWin_t::closeEvent(QCloseEvent *event)
{
   //printf("Main Window Close Event\n");
   if ( gamePadConfWin != NULL )
   {
      //printf("Command Game Pad Close\n");
      gamePadConfWin->closeWindow();
   }
   event->accept();

	closeApp();
}

void gameWin_t::keyPressEvent(QKeyEvent *event)
{
   //printf("Key Press: 0x%x \n", event->key() );
	pushKeyEvent( event, 1 );
}

void gameWin_t::keyReleaseEvent(QKeyEvent *event)
{
   //printf("Key Release: 0x%x \n", event->key() );
	pushKeyEvent( event, 0 );
}

//---------------------------------------------------------------------------
void gameWin_t::createMainMenu(void)
{
    // This is needed for menu bar to show up on MacOS
	 menuBar()->setNativeMenuBar(false);

	 //-----------------------------------------------------------------------
	 // File
    fileMenu = menuBar()->addMenu(tr("&File"));

	 // File -> Open ROM
	 openROM = new QAction(tr("&Open ROM"), this);
    openROM->setShortcuts(QKeySequence::Open);
    openROM->setStatusTip(tr("Open ROM File"));
    connect(openROM, SIGNAL(triggered()), this, SLOT(openROMFile(void)) );

    fileMenu->addAction(openROM);

	 // File -> Close ROM
	 closeROM = new QAction(tr("&Close ROM"), this);
    closeROM->setShortcut( QKeySequence(tr("Ctrl+C")));
    closeROM->setStatusTip(tr("Close Loaded ROM"));
    connect(closeROM, SIGNAL(triggered()), this, SLOT(closeROMCB(void)) );

    fileMenu->addAction(closeROM);

    fileMenu->addSeparator();

	 // File -> Quit
	 quitAct = new QAction(tr("&Quit"), this);
    quitAct->setStatusTip(tr("Quit the Application"));
    connect(quitAct, SIGNAL(triggered()), this, SLOT(closeApp()));

    fileMenu->addAction(quitAct);

	 //-----------------------------------------------------------------------
	 // Options
    optMenu = menuBar()->addMenu(tr("&Options"));

	 // Options -> GamePad Config
	 gamePadConfig = new QAction(tr("&GamePad Config"), this);
    //gamePadConfig->setShortcut( QKeySequence(tr("Ctrl+C")));
    gamePadConfig->setStatusTip(tr("GamePad Configure"));
    connect(gamePadConfig, SIGNAL(triggered()), this, SLOT(openGamePadConfWin(void)) );

    optMenu->addAction(gamePadConfig);

	 //-----------------------------------------------------------------------
	 // Help
    helpMenu = menuBar()->addMenu(tr("&Help"));

	 aboutAct = new QAction(tr("&About"), this);
    aboutAct->setStatusTip(tr("About Qplot"));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(aboutQPlot(void)) );

    helpMenu->addAction(aboutAct);
};
//---------------------------------------------------------------------------
void gameWin_t::closeApp(void)
{
	fceuWrapperClose();

	// LoadGame() checks for an IP and if it finds one begins a network session
	// clear the NetworkIP field so this doesn't happen unintentionally
	g_config->setOption ("SDL.NetworkIP", "");
	g_config->save ();
	//SDL_Quit (); // Already called by fceuWrapperClose

	//qApp::quit();
	qApp->quit();
}
//---------------------------------------------------------------------------

void gameWin_t::openROMFile(void)
{
	int ret;
	QString filename;
	QFileDialog  dialog(this, "Open ROM File");

	dialog.setFileMode(QFileDialog::ExistingFile);

	dialog.setNameFilter(tr("All files (*.*) ;; NES files (*.nes)"));

	dialog.setViewMode(QFileDialog::List);

	// the gnome default file dialog is not playing nice with QT.
	// TODO make this a config option to use native file dialog.
	dialog.setOption(QFileDialog::DontUseNativeDialog, true);

	dialog.show();
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

   //filename =  QFileDialog::getOpenFileName( this,
   //       "Open ROM File",
   //       QDir::currentPath(),
   //       "All files (*.*) ;; NES files (*.nes)");
 
   if ( filename.isNull() )
   {
      return;
   }
	qDebug() << "selected file path : " << filename.toUtf8();

	g_config->setOption ("SDL.LastOpenFile", filename.toStdString().c_str() );
	CloseGame ();
	LoadGame ( filename.toStdString().c_str() );

   return;
}

void gameWin_t::closeROMCB(void)
{
	CloseGame();
}

void gameWin_t::openGamePadConfWin(void)
{
   if ( gamePadConfWin != NULL )
   {
      printf("GamePad Config Window Already Open\n");
      return;
   }
	//printf("Open GamePad Config Window\n");
   gamePadConfWin = new GamePadConfDialog_t(this);
	
   gamePadConfWin->show();
   gamePadConfWin->exec();

   delete gamePadConfWin;
   gamePadConfWin = NULL;
   //printf("GamePad Config Window Destroyed\n");
}

void gameWin_t::aboutQPlot(void)
{
   printf("About QPlot\n");
   return;
}

void gameWin_t::runGameFrame(void)
{
	//struct timespec ts;
	//double t;

	//clock_gettime( CLOCK_REALTIME, &ts );

	//t = (double)ts.tv_sec + (double)(ts.tv_nsec * 1.0e-9);
   //printf("Run Frame %f\n", t);
	
	fceuWrapperUpdate();

	viewport->repaint();

   return;
}
