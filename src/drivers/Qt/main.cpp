#include <QApplication>

#include "GameApp.h"

int main( int argc, char *argv[] )
{
	 QApplication app(argc, argv);
    gameWin_t win;

	 win.resize( 200, 200 );
	 win.show();

    return app.exec();
}

