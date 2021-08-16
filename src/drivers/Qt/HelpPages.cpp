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

#include "driver.h"
#include "Qt/HelpPages.h"
#include "Qt/fceuWrapper.h"
#include "Qt/ConsoleWindow.h"
#include "Qt/ConsoleUtilities.h"

#ifdef WIN32
#include <Windows.h>
#include <htmlhelp.h>
#else // Linux or Unix or APPLE
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

#if  defined(__linux__) || defined(__unix__) || defined(__APPLE__)
static int forkHelpFileViewer( const char *chmViewer, const char *filepath );
#endif

void consoleWin_t::OpenHelpWindow(std::string subpage)
{
	std::string helpFileName;

	g_config->getOption ("SDL.HelpFilePath", &helpFileName );

	if ( helpFileName.length() == 0 )
	{
		std::string helpFileName = FCEUI_GetBaseDirectory();
		helpFileName += "\\..\\doc\\fceux.chm";
	}

	if ( !QFile( QString::fromStdString(helpFileName) ).exists() )
	{
		helpFileName = find_chm();
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
	std::string helpFileViewer;
	g_config->getOption ("SDL.HelpFileViewer", &helpFileViewer );

	helpWin = forkHelpFileViewer( helpFileViewer.c_str(), helpFileName.c_str() );
#endif
}

void consoleWin_t::helpPageMaint(void)
{
#ifdef WIN32
	// Does any help page cleanup need to be done in windows?
#else
	if ( helpWin > 0 )
	{	// Calling waitpid is important to ensure that CHM viewer process is cleaned up
		// in the event that it exits. Otherwise zombie processes will be left.
		int pid, wstat=0;

		pid = waitpid( -1, &wstat, WNOHANG );

		if ( pid == helpWin )
		{
			//printf("Help CHM Viewer Closed\n");
			helpWin = 0;
		}
	}

#endif
}
std::string consoleWin_t::find_chm(void)
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

	dialog.setNameFilter(tr("Compiled HTML Files (*.chm *.CHM) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::AllDirs | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Open") );

	g_config->getOption ("SDL.HelpFilePath", &last );

	if ( last.size() > 0 )
	{
		getDirFromFile( last.c_str(), dir );

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

#if  defined(__linux__) || defined(__unix__) || defined(__APPLE__)
static int forkHelpFileViewer( const char *chmViewer, const char *filepath )
{
	int pid = 0;

	if ( chmViewer[0] == 0 )
	{
		return -1;
	}

	pid = fork();

	if ( pid == 0 )
	{  // Child process
		execl( chmViewer, chmViewer, filepath, NULL );
		exit(0);	
	}
	return pid;
}
#endif
