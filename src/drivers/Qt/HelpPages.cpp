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
// HelpPages.cpp

#include <string>
#include <QFile>
#include <QFileDialog>
#include <QStandardPaths>
#include <QMenuBar>
#include <QToolBar>
#include <QMenu>
#include <QAction>
#include <QSettings>

#ifdef _USE_QHELP
#include <QHelpEngine>
#include <QHelpIndexModel>
#include <QHelpContentWidget>
#endif

#include "driver.h"
#include "Qt/HelpPages.h"
#include "Qt/fceuWrapper.h"
#include "Qt/ConsoleWindow.h"
#include "Qt/ConsoleUtilities.h"

#ifdef WIN32
#include <Windows.h>
#include <htmlhelp.h>
//#else // Linux or Unix or APPLE
//#include <unistd.h>
//#include <sys/types.h>
//#include <sys/wait.h>
#endif

//#if  defined(__linux__) || defined(__unix__) || defined(__APPLE__)
//static int forkHelpFileViewer( const char *chmViewer, const char *filepath );
//#endif

void consoleWin_t::OpenHelpWindow(std::string subpage)
{
	std::string helpFileName;

	g_config->getOption ("SDL.HelpFilePath", &helpFileName );

	if ( helpFileName.length() == 0 )
	{
		#ifdef WIN32
		helpFileName = FCEUI_GetBaseDirectory();
		helpFileName += "\\..\\doc\\fceux.chm";
		#else
		helpFileName = "/usr/share/fceux/fceux.qhc";
		#endif

		#ifdef __APPLE__
		if ( !QFile( QString::fromStdString(helpFileName) ).exists() )
		{
			// Search for MacOSX DragNDrop Resources
			helpFileName = QApplication::applicationDirPath().toStdString() + "/../Resources/fceux.qhc";
		}
		#endif
	}

	if ( !QFile( QString::fromStdString(helpFileName) ).exists() )
	{
		helpFileName = findHelpFile();
	}

	if ( helpFileName.length() == 0 )
	{
		return;
	}

#ifdef WIN32
	if (subpage.length() > 0)
	{
		helpFileName = helpFileName + "::/" + subpage + ".htm";
	}
#else
	// Subpage indexing is not supported by linux chm viewer
#endif

	//printf("Looking for HelpFile '%s'\n", helpFileName.c_str() );

#ifdef WIN32
	// Windows specific HtmlHelp library function
	helpWin = HtmlHelp( HWND(winId()), helpFileName.c_str(), HH_DISPLAY_TOPIC, (DWORD)NULL);
	if ( helpWin == NULL )
	{
		printf("Error: Failed to open help file '%s'\n", helpFileName.c_str() );
	}
#else
	if ( helpWin > 0 )
	{
		printf("There is already a CHM Viewer open somewhere...\n");
		return;
	}
	//helpWin = forkHelpFileViewer( helpFileViewer.c_str(), helpFileName.c_str() );

	#ifdef _USE_QHELP
	HelpDialog *win = new HelpDialog( helpFileName.c_str(), this);

	win->show();
	#endif

#endif
}

//void consoleWin_t::helpPageMaint(void)
//{
//#ifdef WIN32
//	// Does any help page cleanup need to be done in windows?
//#else
//	if ( helpWin > 0 )
//	{	// Calling waitpid is important to ensure that CHM viewer process is cleaned up
//		// in the event that it exits. Otherwise zombie processes will be left.
//		int pid, wstat=0;
//
//		pid = waitpid( -1, &wstat, WNOHANG );
//
//		if ( pid == helpWin )
//		{
//			//printf("Help CHM Viewer Closed\n");
//			helpWin = 0;
//		}
//	}
//
//#endif
//}
std::string consoleWin_t::findHelpFile(void)
{
	int ret, useNativeFileDialogVal;
	QString filename;
	std::string last;
	char dir[512];
	QFileDialog  dialog(this, tr("Open Help File") );
	QList<QUrl> urls;
	//QDir d;

	urls << QUrl::fromLocalFile( QDir::rootPath() );
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).first());
	urls << QUrl::fromLocalFile( QDir( FCEUI_GetBaseDirectory() ).absolutePath() );

	dialog.setFileMode(QFileDialog::ExistingFile);

#ifdef WIN32
	dialog.setNameFilter(tr("Compiled HTML Files (*.chm *.CHM) ;; All files (*)"));
#else
	dialog.setNameFilter(tr("QHelp Files (*.qhc *.QHC) ;; All files (*)"));
#endif

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::AllDirs | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Open") );

	g_config->getOption ("SDL.HelpFilePath", &last );

	if ( last.size() > 0 )
	{
		getDirFromFile( last.c_str(), dir, sizeof(dir) );

		dialog.setDirectory( tr(dir) );
	}
	else
	{
		dialog.setDirectory( QDir( FCEUI_GetBaseDirectory() ).absolutePath() );
	}

	// Check config option to use native file dialog or not
	g_config->getOption ("SDL.UseNativeFileDialog", &useNativeFileDialogVal);

	dialog.setOption(QFileDialog::DontUseNativeDialog, !useNativeFileDialogVal);
	dialog.setSidebarUrls(urls);

	ret = dialog.exec();

	if ( ret )
	{
		QStringList fileList;
		fileList = dialog.selectedFiles();

		if ( fileList.size() > 0 )
		{
			filename = fileList[0];
		}
	}

	if ( filename.isNull() )
	{
		return last;
	}
	//qDebug() << "selected file path : " << filename.toUtf8();

	g_config->setOption ("SDL.HelpFilePath", filename.toStdString().c_str() );

	return filename.toStdString();
}

//#if  defined(__linux__) || defined(__unix__) || defined(__APPLE__)
//static int forkHelpFileViewer( const char *chmViewer, const char *filepath )
//{
//	int pid = 0;
//
//	if ( chmViewer[0] == 0 )
//	{
//		return -1;
//	}
//
//	pid = fork();
//
//	if ( pid == 0 )
//	{  // Child process
//		execl( chmViewer, chmViewer, filepath, NULL );
//		exit(0);	
//	}
//	return pid;
//}
//#endif

#ifdef _USE_QHELP
//-----------------------------------------------------------------------------------------------
//---  Help Page Dialog
//-----------------------------------------------------------------------------------------------
HelpDialog::HelpDialog( const char *helpFileName, QWidget *parent)
	: QDialog(parent, Qt::Window)
{
	int useNativeMenuBar;
	QMenu *fileMenu;
	QMenuBar *menuBar;
	QToolBar *toolBar;
	QAction *act;
	QVBoxLayout *mainLayoutv;
	QSettings settings;

	mainLayoutv = new QVBoxLayout();

	setLayout( mainLayoutv );

	toolBar = new QToolBar(this);
	menuBar = new QMenuBar(this);
	mainLayoutv->setMenuBar( menuBar );

	// This is needed for menu bar to show up on MacOS
	g_config->getOption( "SDL.UseNativeMenuBar", &useNativeMenuBar );

	menuBar->setNativeMenuBar( useNativeMenuBar ? true : false );

	helpEngine = new QHelpEngine( helpFileName, this );
	helpEngine->setupData();

	hsplitter  = new QSplitter( Qt::Horizontal );
	tabWgt     = new QTabWidget();
	textViewer = new HelpBrowser( helpEngine );

	textViewer->setSource(
                QUrl("qthelp://TasVideos.fceux/doc/help/fceux.html"));

	//-----------------------------------------------------------------------
	// Menu Start
	//-----------------------------------------------------------------------
	// File
	fileMenu = menuBar->addMenu(tr("&File"));

	// File -> Close
	act = new QAction(tr("&Close"), this);
	act->setShortcut(QKeySequence::Close);
	act->setStatusTip(tr("Close Window"));
	connect(act, SIGNAL(triggered()), this, SLOT(closeWindow(void)) );

	fileMenu->addAction(act);

	//-----------------------------------------------------------------------
	// Menu End
	//-----------------------------------------------------------------------
	
	//-----------------------------------------------------------------------
	// Tool Bar Setup Start
	//-----------------------------------------------------------------------
	
	backButton = act = new QAction(tr("&Backward"), this);
	act->setShortcut( QKeySequence(tr("Alt+Left") ));
	act->setToolTip(tr("Navigate Backward"));
	act->setIcon( style()->standardIcon(QStyle::SP_ArrowBack) );
	connect(act, SIGNAL(triggered(void)), textViewer, SLOT(backward(void)) );

	toolBar->addAction(act);

	forwardButton = act = new QAction(tr("&Forward"), this);
	act->setShortcut( QKeySequence(tr("Alt+Right") ));
	act->setToolTip(tr("Navigate Forward"));
	act->setIcon( style()->standardIcon(QStyle::SP_ArrowForward) );
	connect(act, SIGNAL(triggered(void)), textViewer, SLOT(forward(void)) );

	toolBar->addAction(act);

	//-----------------------------------------------------------------------
	// Tool Bar Setup End
	//-----------------------------------------------------------------------

	   backButton->setEnabled(false);
	forwardButton->setEnabled(false);

	tabWgt->addTab( helpEngine->contentWidget(), tr("Contents") );
	tabWgt->addTab( helpEngine->indexWidget()  , tr("Index")    );

	hsplitter->addWidget( tabWgt );
	hsplitter->addWidget( textViewer );

	mainLayoutv->addWidget( toolBar   );
	mainLayoutv->addWidget( hsplitter );

	connect(helpEngine->contentWidget(),
		SIGNAL(linkActivated(QUrl)),
		textViewer, SLOT(setSource(QUrl)));

	connect(helpEngine->indexWidget(),
		SIGNAL(linkActivated(QUrl, QString)),
		textViewer, SLOT(setSource(QUrl)));

	connect( textViewer, SIGNAL(backwardAvailable(bool)), this, SLOT(navBackwardAvailable(bool)) );
	connect( textViewer, SIGNAL(forwardAvailable(bool)) , this, SLOT(navForwardAvailable(bool)) );
	
	// Restore Window Geometry
	restoreGeometry(settings.value("HelpPage/geometry").toByteArray());

	// Restore Horizontal Panel State
	hsplitter->restoreState( settings.value("HelpPage/hPanelState").toByteArray() );

}
//-----------------------------------------------------------------------------------------------
HelpDialog::~HelpDialog(void)
{
	QSettings settings;

	// Save Horizontal Panel State
	settings.setValue("HelpPage/hPanelState", hsplitter->saveState());

	// Save Window Geometry
	settings.setValue("HelpPage/geometry", saveGeometry());
}
//-----------------------------------------------------------------------------------------------
void HelpDialog::closeEvent(QCloseEvent *event)
{
	//printf("Help Dialog Close Window Event\n");
	done(0);
	deleteLater();
	event->accept();
}
//-----------------------------------------------------------------------------------------------
void HelpDialog::closeWindow(void)
{
	//printf("Close Window\n");
	done(0);
	deleteLater();
}
//-----------------------------------------------------------------------------------------------
void HelpDialog::navBackwardAvailable(bool avail)
{
	backButton->setEnabled( avail );
}
//-----------------------------------------------------------------------------------------------
void HelpDialog::navForwardAvailable(bool avail)
{
	forwardButton->setEnabled( avail );
}
//-----------------------------------------------------------------------------------------------
//---- Help Browser Class
//-----------------------------------------------------------------------------------------------
HelpBrowser::HelpBrowser(QHelpEngine* helpEngine,
                         QWidget* parent):QTextBrowser(parent),
                         helpEngine(helpEngine)
{
}
//-----------------------------------------------------------------------------------------------
QVariant HelpBrowser::loadResource(int type, const QUrl &name)
{
    if (name.scheme() == "qthelp")
        return QVariant(helpEngine->fileData(name));
    else
        return QTextBrowser::loadResource(type, name);
}
//-----------------------------------------------------------------------------------------------
#endif // _USE_QHELP
