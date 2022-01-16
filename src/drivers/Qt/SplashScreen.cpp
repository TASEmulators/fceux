// SplashScreen.cpp
#include "Qt/SplashScreen.h"

fceuSplashScreen::fceuSplashScreen(void)
	: QSplashScreen( QPixmap(":/fceux1.png") )
{

	alpha = 255;

	showMessage("Initializing GUI...");
	updateTimer = new QTimer(this);

	connect(updateTimer, &QTimer::timeout, this, &fceuSplashScreen::periodicUpdate);
}

fceuSplashScreen::~fceuSplashScreen(void)
{
	//printf("SplashScreen Detroyed\n");
	updateTimer->stop();
}

void fceuSplashScreen::closeEvent(QCloseEvent *event)
{
	//printf("Splash CloseEvent\n");

	if ( alpha > 0 )
	{
		if ( !updateTimer->isActive() )
		{
			updateTimer->start(33);
		}
		event->ignore();
	}
	else
	{
		updateTimer->stop();
		QSplashScreen::closeEvent(event);
	}
}

void fceuSplashScreen::periodicUpdate(void)
{
	if ( alpha > 0 )
	{
		alpha -= 10;

		if ( alpha < 0 )
		{
			alpha = 0;
		}
		setWindowOpacity( (double)alpha / 255.0 );
		update();
	}
	else
	{
		close();
		deleteLater();
		updateTimer->stop();
	}
}
