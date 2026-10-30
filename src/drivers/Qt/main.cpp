/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020 mjbudd77
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include <stdio.h>
#include <stdlib.h>
#include <QApplication>
#include <QAbstractEventDispatcher>
#include <QSplashScreen>
#include <QSettings>
#include <QtGlobal>
//#include <QProxyStyle>

#include "Qt/ConsoleWindow.h"
#include "Qt/fceuWrapper.h"
#include "Qt/SplashScreen.h"
#include "Qt/QtScriptManager.h"
#include "profiler.h"

#if defined(WIN32) 
#include <windows.h>
#if (QT_VERSION_MAJOR < 6)
#include <QtPlatformHeaders/QWindowsWindowFunctions>
#endif
#endif

consoleWin_t *consoleWindow = NULL;

static void MessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    const char *file = context.file ? context.file : "";
    const char *function = context.function ? context.function : "";
    char cmsg[2048];
    switch (type) 
    {
       case QtDebugMsg:
           snprintf( cmsg, sizeof(cmsg), "Qt Debug: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
	   FCEUD_Message(cmsg);
           break;
       case QtInfoMsg:
           snprintf( cmsg, sizeof(cmsg), "Qt Info: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
	   FCEUD_Message(cmsg);
           break;
       case QtWarningMsg:
           snprintf( cmsg, sizeof(cmsg), "Qt Warning: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
	   FCEUD_Message(cmsg);
           break;
       case QtCriticalMsg:
           snprintf( cmsg, sizeof(cmsg), "Qt Critical: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
	   FCEUD_PrintError(cmsg);
           break;
       case QtFatalMsg:
           snprintf( cmsg, sizeof(cmsg), "Qt Fatal: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
	   FCEUD_PrintError(cmsg);
           break;
    }
    cmsg[sizeof(cmsg)-1] = 0;
    fprintf(stderr, "%s", cmsg );

#ifdef  __FCEU_QSCRIPT_ENABLE__
    QtScriptManager::logMessageQt(type, msg);
#endif
}


// This custom menu style wrapper used to prevent the menu bar from permanently stealing window focus when the ALT key is pressed.
//class MenuStyle : public QProxyStyle
//{
//public:
//    int styleHint(StyleHint stylehint, const QStyleOption *opt, const QWidget *widget, QStyleHintReturn *returnData) const
//    {
//        if (stylehint == QStyle::SH_MenuBar_AltKeyNavigation)
//            return 0;
//
//        return QProxyStyle::styleHint(stylehint, opt, widget, returnData);
//    }
//};


#undef main   // undef main in case SDL_Main

static bool showSplashScreen(void)
{
	bool show = false;
	QSettings  settings;

	show = settings.value("mainWindow/showSplashScreen", false).toBool();

	return show;
}
#ifdef __FCEU_PROFILER_ENABLE__
// Marks the time the GUI thread spends busy: from the event loop waking up until it
// is about to block again, rather than one marker per widget event.
class GuiBusyProfiler : public QObject
{
public:
	GuiBusyProfiler(QObject *parent = nullptr) : QObject(parent)
	{
		QAbstractEventDispatcher *disp = QAbstractEventDispatcher::instance();

		connect(disp, &QAbstractEventDispatcher::awake, this, [this](){ beginBusy(); });
		connect(disp, &QAbstractEventDispatcher::aboutToBlock, this, [this](){
			delete busy;
			busy = nullptr;
		});
	}
	~GuiBusyProfiler(void)
	{
		delete busy;
	}

protected:
	// Sees events (including timer events) before their handlers run, which can be
	// earlier than the dispatcher's awake signal.
	bool eventFilter(QObject *watched, QEvent *event) override
	{
		beginBusy();
		return QObject::eventFilter(watched, event);
	}

private:
	void beginBusy(void)
	{
		if (!busy)
		{
			static constexpr FCEU::profileSite site = { __LINE__, __FILE__, "GuiBusyProfiler", "GUI Event Loop Busy" };
			busy = new FCEU::profileFuncScoped(&site);
		}
	}
	FCEU::profileFuncScoped *busy = nullptr;
};
#endif

#define orgName    QStringLiteral("TasEmulators")
#define orgDomain  QStringLiteral("TasEmulators.org")
#define appName    QStringLiteral("fceux")

#if defined(Q_OS_LINUX)
static void WaylandWindowFix()
{
	QSettings settings(orgName, appName);

	const bool isWayland = qgetenv("XDG_SESSION_TYPE") == "wayland" ||
	                      !qgetenv("WAYLAND_DISPLAY").isEmpty();

	const bool waylandOverride = settings.value("Application/WaylandPlatformOverride", false).toBool();
	if (isWayland && waylandOverride && !qEnvironmentVariableIsSet("QT_QPA_PLATFORM"))
	{
		if (!qputenv("QT_QPA_PLATFORM", "xcb"))
		{
			qWarning("Failed to set QT_QPA_PLATFORM=xcb for the Wayland platform override.");
		}
	}
}
#endif

static void QtPreInit()
{
#if defined(Q_OS_LINUX)
	WaylandWindowFix();
#endif
}

int main( int argc, char *argv[] )
{
	int retval = 0;

	QtPreInit();
	fceuWrapperPreInit(argc, argv);

	qInstallMessageHandler(MessageOutput);
	QApplication app(argc, argv);
#ifdef __FCEU_PROFILER_ENABLE__
	GuiBusyProfiler *guiBusyProfiler = new GuiBusyProfiler();
	app.installEventFilter(guiBusyProfiler);
#endif

	QCoreApplication::setOrganizationName(orgName);
	QCoreApplication::setOrganizationDomain(orgDomain);
	QCoreApplication::setApplicationName(appName);

	fceuSplashScreen *splash = NULL;
	
	if ( showSplashScreen() )
	{
		splash = new fceuSplashScreen();
		splash->show();
		app.processEvents();
	}

	#ifdef WIN32
	if (AttachConsole(ATTACH_PARENT_PROCESS))
	{
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);
	}
	#endif
	//app.setStyle( new MenuStyle() );

	//styleSheetEnv = ::getenv("FCEUX_QT_STYLESHEET");
	//
	//if ( styleSheetEnv != NULL )
	//{
	//   QFile File(styleSheetEnv);
	//
	//   if ( File.open(QFile::ReadOnly) )
	//   {
	//      QString StyleSheet = QLatin1String(File.readAll());
	//
	//      app.setStyleSheet(StyleSheet);
	//
	//      printf("Using Qt Stylesheet file '%s'\n", styleSheetEnv );
	//   }
	//   else
	//   {
	//      printf("Warning: Could not open Qt Stylesheet file '%s'\n", styleSheetEnv );
	//   }
	//}

	fceuWrapperInit( argc, argv );

	consoleWindow = new consoleWin_t();

	consoleWindow->show();

	// Need to wait for window to initialize before video init can be called.
	//consoleWindow->videoInit();

#if defined(WIN32) && (QT_VERSION_MAJOR < 6)
	// This function is needed to fix the issue referenced below. It adds a 1-pixel border
	// around the fullscreen window due to some limitation in windows.
	// https://doc.qt.io/qt-5/windows-issues.html#fullscreen-opengl-based-windows
	bool fullScreenBorderOpt = false;
	g_config->getOption("SDL.winFullScreenBorder", &fullScreenBorderOpt );
	QWindowsWindowFunctions::setHasBorderInFullScreen( consoleWindow->windowHandle(), fullScreenBorderOpt);
#endif

	if ( splash )
	{
		splash->finish( consoleWindow );
		//delete splash; this is handled by Qt event loop
	}

	retval = app.exec();

	//printf("App Return: %i \n", retval );

	delete consoleWindow;

#ifdef __FCEU_PROFILER_ENABLE__
	app.removeEventFilter(guiBusyProfiler);
	delete guiBusyProfiler;
#endif

	fceuWrapperMemoryCleanup();

	return retval;
}
