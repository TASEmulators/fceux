#include <QApplication>
#include <QScreen>

#include "Qt/GameApp.h"
#include "Qt/fceuWrapper.h"

int main( int argc, char *argv[] )
{
	double devPixRatio = 1.0;
	 QApplication app(argc, argv);
    gameWin_t win;

    QScreen *screen = QGuiApplication::primaryScreen();

    if ( screen != NULL )
    {
	devPixRatio = screen->devicePixelRatio();
    	printf("Ratio: %f \n", screen->devicePixelRatio() );
    }

	 fceuWrapperInit( argc, argv );

	 //win.resize( 512, 512 );
	 win.show();

	 win.viewport->init( devPixRatio );

    return app.exec();
}

