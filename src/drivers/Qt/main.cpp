#include <stdio.h>
#include <stdlib.h>
#include <QApplication>

#include "Qt/ConsoleWindow.h"
#include "Qt/fceuWrapper.h"

consoleWin_t *consoleWindow = NULL;

static void MessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    const char *file = context.file ? context.file : "";
    const char *function = context.function ? context.function : "";
    switch (type) 
    {
       case QtDebugMsg:
           fprintf(stderr, "Qt Debug: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
           break;
       case QtInfoMsg:
           fprintf(stderr, "Qt Info: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
           break;
       case QtWarningMsg:
           fprintf(stderr, "Qt Warning: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
           break;
       case QtCriticalMsg:
           fprintf(stderr, "Qt Critical: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
           break;
       case QtFatalMsg:
           fprintf(stderr, "Qt Fatal: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
           break;
    }
}


int main( int argc, char *argv[] )
{
	int retval;
   qInstallMessageHandler(MessageOutput);
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

