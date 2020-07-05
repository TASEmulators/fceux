#include <QApplication>

#include "Qt/GameApp.h"
#include "Qt/fceuWrapper.h"

gameWin_t *gameWindow = NULL;

int main( int argc, char *argv[] )
{
	int retval;
	QApplication app(argc, argv);

	fceuWrapperInit( argc, argv );

	gameWindow = new gameWin_t();

	gameWindow->resize( 512, 512 );
	gameWindow->show();

	gameWindow->viewport->init();

	retval = app.exec();

	//printf("App Return: %i \n", retval );

	delete gameWindow;

	return retval;
}

