#include <QApplication>

#include "GameApp.h"
#include "fceuWrapper.h"

int main( int argc, char *argv[] )
{
	 QApplication app(argc, argv);
    gameWin_t win;

	 fceuWrapperInit( argc, argv );

	 win.resize( 512, 512 );
	 win.show();

    return app.exec();
}

