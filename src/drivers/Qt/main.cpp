#include <stdio.h>
#include <stdlib.h>
#include <QApplication>

#include "Qt/ConsoleWindow.h"
#include "Qt/fceuWrapper.h"

consoleWin_t *consoleWindow = NULL;

int main( int argc, char *argv[] )
{
	int retval;
	QApplication app(argc, argv);
   const char *styleSheetEnv = NULL;

   styleSheetEnv = ::getenv("FCEUX_QT_STYLESHEET");

   if ( styleSheetEnv != NULL )
   {
      QFile File(styleSheetEnv);

      if ( File.open(QFile::ReadOnly) )
      {
         QString StyleSheet = QLatin1String(File.readAll());

         app.setStyleSheet(StyleSheet);

         printf("Using Qt Stylesheet file '%s'\n", styleSheetEnv );
      }
      else
      {
         printf("Warning: Could not open Qt Stylesheet file '%s'\n", styleSheetEnv );
      }
   }

	fceuWrapperInit( argc, argv );

	consoleWindow = new consoleWin_t();

	consoleWindow->resize( 512, 512 );
	consoleWindow->show();

	if ( consoleWindow->viewport_SDL )
	{
		consoleWindow->viewport_SDL->init();
	}
	else
	{
		consoleWindow->viewport_GL->init();
	}

	retval = app.exec();

	//printf("App Return: %i \n", retval );

	delete consoleWindow;

	return retval;
}

