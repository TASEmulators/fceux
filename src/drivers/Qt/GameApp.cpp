// GameApp.cpp
//
#include "GameApp.h"
#include "fceuWrapper.h"
#include "keyscan.h"

gameWin_t::gameWin_t(QWidget *parent)
	: QMainWindow( parent )
{
	QWidget *widget = new QWidget( this );

   setCentralWidget(widget);

	QVBoxLayout *layout = new QVBoxLayout;
   layout->setMargin(5);

	createMainMenu();

	viewport = new gameView_t();
	//viewport.resize( 200, 200 );

	layout->addWidget(viewport);

	widget->setLayout(layout);

	gameTimer = new QTimer( this );

	connect( gameTimer, &QTimer::timeout, this, &gameWin_t::runGameFrame );

	gameTimer->setTimerType( Qt::PreciseTimer );
	gameTimer->start( 10 );
}

gameWin_t::~gameWin_t(void)
{
	fceuWrapperClose();

	delete viewport;
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

void gameWin_t::createMainMenu(void)
{
	 menuBar()->setNativeMenuBar(false);
    fileMenu = menuBar()->addMenu(tr("&File"));

	 openAct = new QAction(tr("&Open"), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an Existing File"));
    connect(openAct, SIGNAL(triggered()), this, SLOT(openFile(void)) );

    fileMenu->addAction(openAct);

	 quitAct = new QAction(tr("&Quit"), this);
    quitAct->setStatusTip(tr("Quit the Application"));
    connect(quitAct, SIGNAL(triggered()), qApp, SLOT(quit()));

    fileMenu->addAction(quitAct);

    helpMenu = menuBar()->addMenu(tr("&Help"));

	 aboutAct = new QAction(tr("&About"), this);
    aboutAct->setStatusTip(tr("About Qplot"));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(aboutQPlot(void)) );

    helpMenu->addAction(aboutAct);
};

void gameWin_t::openFile(void)
{
   printf("Open File\n");
   return;
}
void gameWin_t::aboutQPlot(void)
{
   printf("About QPlot\n");
   return;
}

void gameWin_t::runGameFrame(void)
{
	struct timespec ts;
	double t;

	clock_gettime( CLOCK_REALTIME, &ts );

	t = (double)ts.tv_sec + (double)(ts.tv_nsec * 1.0e-9);
   //printf("Run Frame %f\n", t);
	
	fceuWrapperUpdate();

	viewport->repaint();

   return;
}
