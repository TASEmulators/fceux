// MoviePlay.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <QHeaderView>
#include <QCloseEvent>
#include <QFileDialog>
#include <QGridLayout>

#include "../../fceu.h"
#include "../../movie.h"

#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/fceuWrapper.h"
#include "Qt/ConsoleUtilities.h"
#include "Qt/MoviePlay.h"

//----------------------------------------------------------------------------
MoviePlayDialog_t::MoviePlayDialog_t(QWidget *parent)
	: QDialog( parent )
{
	QVBoxLayout *mainLayout, *vbox;
	QHBoxLayout *hbox;
	QGroupBox *frame;
	QGridLayout *grid;
	QLabel *lbl;
	QPushButton *okButton, *cancelButton;

	setWindowTitle("Movie Play");

	mainLayout = new QVBoxLayout();
	hbox       = new QHBoxLayout();

	lbl          = new QLabel( tr("File:") );
	movSelBox    = new QComboBox();
	movBrowseBtn = new QPushButton( tr("Browse") );

	hbox->addWidget( lbl, 1 );
	hbox->addWidget( movSelBox, 100    );
	hbox->addWidget( movBrowseBtn, 1 );

	mainLayout->addLayout( hbox  );

	frame = new QGroupBox( tr("Parameters:") );
	vbox  = new QVBoxLayout();
	hbox  = new QHBoxLayout();

	frame->setLayout( vbox );

	openReadOnly = new QCheckBox( tr("Open Read-Only") );
	pauseAtFrame = new QCheckBox( tr("Pause Movie At Frame") );

	pauseAtFrameEntry = new QLineEdit();

	vbox->addWidget( openReadOnly );
	vbox->addLayout( hbox );
	hbox->addWidget( pauseAtFrame );
	hbox->addWidget( pauseAtFrameEntry );

	mainLayout->addWidget( frame  );

	grid = new QGridLayout();
	grid->setColumnStretch( 0,  1 );
	grid->setColumnStretch( 1, 10 );

	mainLayout->addLayout( grid );

	movLenLbl     = new QLabel();
	movFramesLbl  = new QLabel();
	recCountLbl   = new QLabel();
	recFromLbl    = new QLabel();
	romUsedLbl    = new QLabel();
	romCsumLbl    = new QLabel();
	curCsumLbl    = new QLabel();
	emuUsedLbl    = new QLabel();
	palUsedLbl    = new QLabel();
	newppuUsedLbl = new QLabel();

	grid->addWidget( new QLabel( tr("Length:") )          , 0, 0, Qt::AlignRight );
	grid->addWidget( new QLabel( tr("Frames:") )          , 1, 0, Qt::AlignRight );
	grid->addWidget( new QLabel( tr("Record Count:") )    , 2, 0, Qt::AlignRight );
	grid->addWidget( new QLabel( tr("Recorded From:") )   , 3, 0, Qt::AlignRight );
	grid->addWidget( new QLabel( tr("ROM Used:") )        , 4, 0, Qt::AlignRight );
	grid->addWidget( new QLabel( tr("ROM Checksum:") )    , 5, 0, Qt::AlignRight );
	grid->addWidget( new QLabel( tr("Current ROM Sum:") ) , 6, 0, Qt::AlignRight );
	grid->addWidget( new QLabel( tr("Emulator Used:") )   , 7, 0, Qt::AlignRight );
	grid->addWidget( new QLabel( tr("PAL:") )             , 8, 0, Qt::AlignRight );
	grid->addWidget( new QLabel( tr("New PPU:") )         , 9, 0, Qt::AlignRight );

	grid->addWidget( movLenLbl     , 0, 1, Qt::AlignLeft );
	grid->addWidget( movFramesLbl  , 1, 1, Qt::AlignLeft );
	grid->addWidget( recCountLbl   , 2, 1, Qt::AlignLeft );
	grid->addWidget( recFromLbl    , 3, 1, Qt::AlignLeft );
	grid->addWidget( romUsedLbl    , 4, 1, Qt::AlignLeft );
	grid->addWidget( romCsumLbl    , 5, 1, Qt::AlignLeft );
	grid->addWidget( curCsumLbl    , 6, 1, Qt::AlignLeft );
	grid->addWidget( emuUsedLbl    , 7, 1, Qt::AlignLeft );
	grid->addWidget( palUsedLbl    , 8, 1, Qt::AlignLeft );
	grid->addWidget( newppuUsedLbl , 9, 1, Qt::AlignLeft );

	hbox         = new QHBoxLayout();
	okButton     = new QPushButton( tr("Play") );
	cancelButton = new QPushButton( tr("Cancel") );
	hbox->addWidget( cancelButton );
	hbox->addWidget( okButton );
	okButton->setDefault(true);
	mainLayout->addLayout( hbox  );

	setLayout( mainLayout );

	connect( cancelButton , SIGNAL(clicked(void)), this, SLOT(closeWindow(void)) );

	connect( movBrowseBtn , SIGNAL(clicked(void)), this, SLOT(openMovie(void))  );
}
//----------------------------------------------------------------------------
MoviePlayDialog_t::~MoviePlayDialog_t(void)
{
	printf("Destroy Movie Play Window\n");
}
//----------------------------------------------------------------------------
void MoviePlayDialog_t::closeEvent(QCloseEvent *event)
{
   printf("Movie Play Close Window Event\n");
   done(0);
	deleteLater();
   event->accept();
}
//----------------------------------------------------------------------------
void MoviePlayDialog_t::closeWindow(void)
{
   //printf("Close Window\n");
   done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
//void  MoviePlayDialog_t::readOnlyReplayChanged( int state )
//{
//	suggestReadOnlyReplay = (state != Qt::Unchecked);
//}
//----------------------------------------------------------------------------
void MoviePlayDialog_t::openMovie(void)
{
	int ret, useNativeFileDialogVal;
	QString filename;
	std::string last;
	char dir[512];
	char replayReadOnlySetting;
	QFileDialog  dialog(this, tr("Open FM2 Movie") );

	dialog.setFileMode(QFileDialog::ExistingFile);

	dialog.setNameFilter(tr("FM2 Movies (*.fm2) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::AllDirs | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Open") );

	g_config->getOption ("SDL.LastOpenMovie", &last );

	getDirFromFile( last.c_str(), dir );

	dialog.setDirectory( tr(dir) );

	// Check config option to use native file dialog or not
	g_config->getOption ("SDL.UseNativeFileDialog", &useNativeFileDialogVal);

	dialog.setOption(QFileDialog::DontUseNativeDialog, !useNativeFileDialogVal);

	dialog.show();
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

	FCEUI_printf ("Playing back movie located at %s\n", filename.toStdString().c_str() );

	if (suggestReadOnlyReplay)
	{
		replayReadOnlySetting = true;
	}
	else
	{
		replayReadOnlySetting = FCEUI_GetMovieToggleReadOnly();
	}

	movSelBox->addItem( filename, movSelBox->count() );
	//fceuWrapperLock();
	//if (FCEUI_LoadMovie( filename.toStdString().c_str(),
	//	    replayReadOnlySetting, pauseframe ? pauseframe : false) == false)
	//{
	//	printf("Error: Could not open movie file: %s \n", filename.toStdString().c_str() );
	//}
	g_config->setOption ("SDL.LastOpenMovie", filename.toStdString().c_str() );
	//fceuWrapperUnLock();

   return;
}
//----------------------------------------------------------------------------
