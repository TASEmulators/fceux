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
// MovieRecord.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <QHeaderView>
#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QGridLayout>
#include <QSettings>
#include <QFile>
#include <QDir>

#include "../../fceu.h"
#include "../../movie.h"

#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/fceuWrapper.h"
#include "Qt/ConsoleUtilities.h"
#include "Qt/MovieRecord.h"

//----------------------------------------------------------------------------
MovieRecordDialog_t::MovieRecordDialog_t(QWidget *parent)
	: QDialog(parent)
{
	QVBoxLayout *mainLayout;
	QHBoxLayout *hbox;
	QGridLayout *grid;
	QSettings settings;
	std::string name;

	setWindowTitle( tr("Record Input Movie") );

	mainLayout = new QVBoxLayout();
	grid       = new QGridLayout();
	hbox       = new QHBoxLayout();

	setLayout( mainLayout );

	mainLayout->addLayout( grid );
	mainLayout->addLayout( hbox );

	dirEdit      = new QLineEdit();
	fileEdit     = new QLineEdit();
	authorEdit   = new QLineEdit();
	fileBrowse   = new QPushButton( tr("...") );
	stateSel     = new QComboBox();
	okButton     = new QPushButton( tr("OK") );
	cancelButton = new QPushButton( tr("Cancel") );

	    okButton->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
	cancelButton->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));
	  fileBrowse->setIcon(style()->standardIcon(QStyle::SP_FileDialogStart) );

	stateSel->addItem( tr("Start")    , 0 );
	stateSel->addItem( tr("Now")      , 1 );
	stateSel->addItem( tr("Save RAM") , 2 );
	stateSel->addItem( tr("Browse..."), 3 );

	grid->addWidget( new QLabel( tr("Path:") ), 0, 0 );
	grid->addWidget( dirEdit  ,  0, 1 );

	grid->addWidget( new QLabel( tr("File:") ), 1, 0 );
	grid->addWidget( fileEdit  ,  1, 1 );
	grid->addWidget( fileBrowse,  1, 2 );

	grid->addWidget( new QLabel( tr("Record:") ), 2, 0 );
	grid->addWidget( stateSel  , 2, 1 );

	grid->addWidget( new QLabel( tr("Author:") ), 3, 0 );
	grid->addWidget( authorEdit  , 3, 1 );

	hbox->addWidget( cancelButton, 1 );
	hbox->addStretch(10);
	hbox->addWidget(     okButton, 1 );

	dirEdit->setReadOnly(true);

	setFilePath( FCEU_MakeFName (FCEUMKF_MOVIE, 0, 0) );

	connect(   fileBrowse, SIGNAL(clicked(void)), this, SLOT(browseFiles(void)) );
	connect(     okButton, SIGNAL(clicked(void)), this, SLOT(recordMovie(void)) );
	connect( cancelButton, SIGNAL(clicked(void)), this, SLOT(reject(void)) );

	connect( stateSel, SIGNAL(activated(int)), this, SLOT(stateSelChanged(int)) );

	okButton->setDefault(true);

	restoreGeometry(settings.value("movieRecordWindow/geometry").toByteArray());
}
//----------------------------------------------------------------------------
MovieRecordDialog_t::~MovieRecordDialog_t(void)
{
	QSettings settings;
	settings.setValue("movieRecordWindow/geometry", saveGeometry());
	//printf("Destroy Movie Play Window\n");
}
//----------------------------------------------------------------------------
void MovieRecordDialog_t::closeEvent(QCloseEvent *event)
{
	QSettings settings;
	settings.setValue("movieRecordWindow/geometry", saveGeometry());
	done(0);
	deleteLater();
	event->accept();
}
//----------------------------------------------------------------------------
void MovieRecordDialog_t::closeWindow(void)
{
	QSettings settings;
	settings.setValue("movieRecordWindow/geometry", saveGeometry());
	done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
void MovieRecordDialog_t::setFilePath( const char *s )
{
	setFilePath( tr(s) );
}
//----------------------------------------------------------------------------
void MovieRecordDialog_t::setFilePath( std::string s )
{
	setFilePath( QString::fromStdString(s) );
}
//----------------------------------------------------------------------------
void MovieRecordDialog_t::setFilePath( QString s )
{
	QFileInfo fi(s);

	 dirEdit->setText( fi.absolutePath() );
	fileEdit->setText( fi.fileName() );

	filepath = s.toStdString();
}
//----------------------------------------------------------------------------
void MovieRecordDialog_t::stateSelChanged( int idx )
{
	//printf("State:%i\n", idx);

	if ( idx >= 3 )
	{
		setLoadState();
		okButton->setEnabled( ic_file.size() > 0 );
	}
	else
	{
		okButton->setEnabled( true );
	}
}
//----------------------------------------------------------------------------
void MovieRecordDialog_t::setLoadState(void)
{
	int ret, useNativeFileDialogVal;
	QString filename;
	std::string last;
	std::string dir;
	const char *base;
	QFileDialog  dialog(this, tr("Load State From File") );
	QList<QUrl> urls;
	QDir d;

	base = FCEUI_GetBaseDirectory();

	urls << QUrl::fromLocalFile( QDir::rootPath() );
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DesktopLocation).first());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).first());

	if ( base )
	{
		urls << QUrl::fromLocalFile( QDir( base ).absolutePath() );

		d.setPath( QString(base) + "/fcs");

		if ( d.exists() )
		{
			urls << QUrl::fromLocalFile( d.absolutePath() );
		}

		d.setPath( QString(base) + "/sav");

		if ( d.exists() )
		{
			urls << QUrl::fromLocalFile( d.absolutePath() );
		}
	}


	dialog.setFileMode(QFileDialog::ExistingFile);

	dialog.setNameFilter(tr("FCS & SAV Files (*.sav *.SAV *.fc? *.FC?) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::AllDirs | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Select") );

	g_config->getOption ("SDL.LastLoadStateFrom", &last );

	getDirFromFile( last.c_str(), dir );

	dialog.setDirectory( tr(dir.c_str()) );

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
		return;
	}
	qDebug() << "selected file path : " << filename.toUtf8();

	g_config->setOption ("SDL.LastLoadStateFrom", filename.toStdString().c_str() );

	ic_file = filename.toStdString();
}
//----------------------------------------------------------------------------
void MovieRecordDialog_t::recordMovie(void)
{
	bool startRecording = true;

	if (fceuWrapperGameLoaded())
	{
		QString qs;
		int  recordFrom = stateSel->currentIndex();
		EMOVIE_FLAG flags = MOVIE_FLAG_NONE;
		if (recordFrom == 0) flags = MOVIE_FLAG_FROM_POWERON;
		if (recordFrom == 2) flags = MOVIE_FLAG_FROM_SAVERAM;

		if ( recordFrom >= 3 )
		{
			FCEUI_LoadState(ic_file.c_str());
			{
				extern int loadStateFailed;

				if (loadStateFailed)
				{
					char str [1024];
					sprintf(str, "Failed to load save state \"%s\".\nRecording from current state instead...", ic_file.c_str());
					FCEUD_PrintError(str);
				}
			}
		}
		qs = QString::fromStdString(filepath);

		if ( QFile::exists(qs) )
		{
			int ret;

			ret = QMessageBox::warning( this, tr("Overwrite Warning"),
					tr("Pre-existing movie file Will be overwritten.\nReplace file?"), QMessageBox::Yes | QMessageBox::No );

			if ( ret == QMessageBox::No )
			{
				startRecording = false;
			}

		}

		if ( startRecording )
		{
			std::string s = authorEdit->text().toStdString();
			std::wstring author (s.begin (), s.end ());
			FCEUI_printf("Recording movie to %s\n", filepath.c_str ());
			FCEUI_SaveMovie (filepath.c_str(), flags, author);

			done(0);
			deleteLater();
		}
	}
}
//----------------------------------------------------------------------------
void MovieRecordDialog_t::browseFiles(void)
{
	int ret, useNativeFileDialogVal;
	QString filename;
	std::string last;
	std::string dir;
	QFileDialog  dialog(this, tr("Save FM2 Movie for Recording") );

	dialog.setFileMode(QFileDialog::AnyFile);

	dialog.setNameFilter(tr("FM2 Movies (*.fm2) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::AllDirs | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Save To") );
	dialog.setDefaultSuffix( tr(".fm2") );

	g_config->getOption ("SDL.LastOpenMovie", &last );

	getDirFromFile( last.c_str(), dir );

	dialog.setDirectory( tr(dir.c_str()) );

	// Check config option to use native file dialog or not
	g_config->getOption ("SDL.UseNativeFileDialog", &useNativeFileDialogVal);

	dialog.setOption(QFileDialog::DontUseNativeDialog, !useNativeFileDialogVal);

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
	   return;
	}
	qDebug() << "selected file path : " << filename.toUtf8();

	int pauseframe;
	g_config->getOption ("SDL.PauseFrame", &pauseframe);
	g_config->setOption ("SDL.PauseFrame", 0);

	FCEUI_printf ("Recording movie to %s\n", filename.toStdString().c_str() );
	setFilePath( filename );

	return;
}
//----------------------------------------------------------------------------
