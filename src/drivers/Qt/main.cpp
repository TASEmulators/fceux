#include <QApplication>

#include "Qt/GameApp.h"
#include "Qt/fceuWrapper.h"

gameWin_t *gameWindow = NULL;

int main( int argc, char *argv[] )
{
	QApplication app(argc, argv);

	gameWindow = new gameWin_t();

	fceuWrapperInit( argc, argv );

	gameWindow->resize( 512, 512 );
	gameWindow->show();

	gameWindow->viewport->init();

	return app.exec();
}

