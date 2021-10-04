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
// AviRiffViewer.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <QHeaderView>
#include <QCloseEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>

#include "Qt/main.h"
#include "Qt/config.h"
#include "Qt/fceuWrapper.h"
#include "Qt/AviRiffViewer.h"
#include "Qt/ConsoleUtilities.h"

static bool showSizeHex = true;

static const char *riff_tags[] = {
    "IARL", "IART", "IAS1", "IAS2", "IAS3", "IAS4", "IAS5", "IAS6", "IAS7",
    "IAS8", "IAS9", "ICMS", "ICMT", "ICOP", "ICRD", "ICRP", "IDIM", "IDPI",
    "IENG", "IGNR", "IKEY", "ILGT", "ILNG", "IMED", "INAM", "IPLT", "IPRD",
    "IPRT", "ITRK", "ISBJ", "ISFT", "ISHP", "ISMP", "ISRC", "ISRF", "ITCH",
     NULL
};

static const char *riff_info_conv[] = {
    "IARL", "Archival Location",
    "IART", "Artist"    ,
    "ICMS", "Commissioned",
    "ICMT", "Comments"  ,
    "ICOP", "Copyright" ,
    "ICRD", "Date"      ,
    "IENG", "Engineer"  ,
    "IGNR", "Genre"     ,
    "IKEY", "KeyWords"  ,
    "IMED", "Medium"    ,
    "ILNG", "Language"  ,
    "INAM", "Title"     ,
    "IPRD", "Album"     ,
    "IPRT", "Track"     ,
    "ITRK", "Track"     ,
    "ISFT", "Software"  ,
    "ISMP", "Timecode"  ,
    "ISBJ", "Subject"   ,
    "ISRC", "Source"    ,
    "ISRF", "Source Form",
    "ITCH", "Technician",
     NULL , NULL        ,
};
//----------------------------------------------------------------------------
static int riffWalkCallback( int type, long long int fpos, const char *fourcc, size_t size, void *userData )
{
	int ret = 0;

	if ( userData )
	{
		ret = static_cast <AviRiffViewerDialog*>(userData)->riffWalkCallback( type, fpos, fourcc, size );
	}
	return ret;
}
//----------------------------------------------------------------------------
//--- AVI RIFF Viewer Dialog
//----------------------------------------------------------------------------
AviRiffViewerDialog::AviRiffViewerDialog(QWidget *parent)
	: QDialog(parent)
{
	QMenuBar    *menuBar;
	QVBoxLayout *mainLayout;
	QHBoxLayout *hbox;
	QPushButton *closeButton;
	QTreeWidgetItem *item;

	avi = NULL;
	lastChunk = NULL;
	riffSize = 0;
	progressDialog = NULL;
	memset( strhType, 0, sizeof(strhType) );

	setWindowTitle("AVI RIFF Viewer");

	resize(512, 512);

	menuBar = buildMenuBar();
	mainLayout = new QVBoxLayout();
	setLayout(mainLayout);
	mainLayout->setMenuBar( menuBar );

	tabs     = new QTabWidget();
	riffTree = new AviRiffTree();

	tabs->addTab( riffTree, tr("RIFF TREE") );

	riffTree->setColumnCount(4);
	riffTree->setSelectionMode( QAbstractItemView::SingleSelection );
	riffTree->setAlternatingRowColors(true);

	item = new QTreeWidgetItem();
	item->setText(0, QString::fromStdString("Block"));
	item->setText(1, QString::fromStdString("FCC"));
	item->setText(2, QString::fromStdString("Size"));
	item->setText(3, QString::fromStdString("FilePos"));
	item->setTextAlignment(0, Qt::AlignLeft);
	item->setTextAlignment(1, Qt::AlignLeft);
	item->setTextAlignment(2, Qt::AlignLeft);
	item->setTextAlignment(3, Qt::AlignLeft);

	riffTree->setHeaderItem(item);

	riffTree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

	//connect( riffTree, SIGNAL(itemActivated(QTreeWidgetItem*,int)), this, SLOT(hotKeyActivated(QTreeWidgetItem*,int) ) );

	mainLayout->addWidget(tabs);

	closeButton = new QPushButton( tr("Close") );
	closeButton->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
	connect(closeButton, SIGNAL(clicked(void)), this, SLOT(closeWindow(void)));

	hbox = new QHBoxLayout();
	hbox->addStretch(5);
	hbox->addWidget( closeButton, 1 );
	mainLayout->addLayout( hbox );

}
//----------------------------------------------------------------------------
AviRiffViewerDialog::~AviRiffViewerDialog(void)
{
	printf("Destroy AVI RIFF Viewer Window\n");

	if ( avi )
	{
		delete avi; avi = NULL;
	}
}
//----------------------------------------------------------------------------
void AviRiffViewerDialog::closeEvent(QCloseEvent *event)
{
	printf("AVI RIFF Viewer Window Event\n");
	done(0);
	deleteLater();
	event->accept();
}
//----------------------------------------------------------------------------
void AviRiffViewerDialog::closeWindow(void)
{
	//printf("Close Window\n");
	done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
QMenuBar *AviRiffViewerDialog::buildMenuBar(void)
{
	QMenu       *fileMenu;
	//QActionGroup *actGroup;
	QAction     *act;
	int useNativeMenuBar=0;

	QMenuBar *menuBar = new QMenuBar();

	// This is needed for menu bar to show up on MacOS
	g_config->getOption( "SDL.UseNativeMenuBar", &useNativeMenuBar );

	menuBar->setNativeMenuBar( useNativeMenuBar ? true : false );

	//-----------------------------------------------------------------------
	// Menu Start
	//-----------------------------------------------------------------------
	// File
	fileMenu = menuBar->addMenu(tr("&File"));

	// File -> Open
	act = new QAction(tr("&Open AVI File"), this);
	act->setShortcut(QKeySequence::Open);
	act->setStatusTip(tr("Open AVI File"));
	act->setIcon( style()->standardIcon( QStyle::SP_FileDialogStart ) );
	connect(act, SIGNAL(triggered()), this, SLOT(openAviFileDialog(void)) );

	fileMenu->addAction(act);

	// File -> Close
	act = new QAction(tr("&Close AVI File"), this);
	act->setShortcut(QKeySequence(tr("Ctrl+C")));
	act->setStatusTip(tr("Close AVI File"));
	//act->setIcon( style()->standardIcon( QStyle::SP_BrowserStop ) );
	connect(act, SIGNAL(triggered()), this, SLOT(closeFile(void)) );

	fileMenu->addAction(act);

	fileMenu->addSeparator();

	// File -> Quit
	act = new QAction(tr("&Quit Window"), this);
	act->setShortcut(QKeySequence::Close);
	act->setStatusTip(tr("Close Window"));
	act->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
	connect(act, SIGNAL(triggered()), this, SLOT(closeWindow(void)) );

	fileMenu->addAction(act);

	return menuBar;
}
//----------------------------------------------------------------------------
void AviRiffViewerDialog::openAviFileDialog(void)
{
	std::string last;
	int ret, useNativeFileDialogVal;
	QString filename;
	std::string lastPath;
	//char dir[512];
	const char *base;
	QFileDialog  dialog(this, tr("Open AVI Movie for Inspection") );
	QList<QUrl> urls;
	QDir d;

	dialog.setFileMode(QFileDialog::ExistingFile);

	dialog.setNameFilter(tr("AVI Movies (*.avi) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::AllDirs | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Open") );

	base = FCEUI_GetBaseDirectory();

	urls << QUrl::fromLocalFile( QDir::rootPath() );
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).first());

	if ( base )
	{
		urls << QUrl::fromLocalFile( QDir( base ).absolutePath() );

		d.setPath( QString(base) + "/avi");

		if ( d.exists() )
		{
			urls << QUrl::fromLocalFile( d.absolutePath() );
		}

		dialog.setDirectory( d.absolutePath() );
	}
	dialog.setDefaultSuffix( tr(".avi") );

	g_config->getOption ("SDL.AviFilePath", &lastPath);
	if ( lastPath.size() > 0 )
	{
		dialog.setDirectory( QString::fromStdString(lastPath) );
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
	   return;
	}
	//qDebug() << "selected file path : " << filename.toUtf8();

	printf( "AVI Debug movie %s\n", filename.toStdString().c_str() );

	lastPath = QFileInfo(filename).absolutePath().toStdString();

	if ( lastPath.size() > 0 )
	{
		g_config->setOption ("SDL.AviFilePath", lastPath);
	}

	openFile( filename.toStdString().c_str() );
}
//----------------------------------------------------------------------------
int AviRiffViewerDialog::openFile( const char *filepath )
{
	int ret;

	if ( avi )
	{
		closeFile();
	}

	avi = new gwavi_t();

	if ( avi->openIn( filepath ) )
	{
		return -1;
	}

	progressDialog = new QProgressDialog( tr("Loading AVI File"), tr("Cancel"), 0, 1000, this );
	progressDialog->setWindowModality(Qt::WindowModal);

	itemStack.clear();
	lastChunk = NULL;
	memset( strhType, 0, sizeof(strhType) );

	riffSize = 0;
	avi->setRiffWalkCallback( ::riffWalkCallback, this );
	ret = avi->riffwalk();

	if ( ret )
	{
		if ( progressDialog->wasCanceled() )
		{
			QMessageBox::information( this, tr("AVI Load Canceled"), tr("AVI Load Canceled By User.") );
		}
		else
		{
			progressDialog->reset();
			QMessageBox::critical( this, tr("AVI Load Error"), tr("AVI format errors detected. Unable to load file.") );
		}
		closeFile();
	}
	delete progressDialog; progressDialog = NULL;

	return 0;
}
//----------------------------------------------------------------------------
void AviRiffViewerDialog::closeFile(void)
{
	if ( avi )
	{
		delete avi; avi = NULL;
	}
	riffTree->clear();
	itemStack.clear();
}
//----------------------------------------------------------------------------
int AviRiffViewerDialog::riffWalkCallback( int type, long long int fpos, const char *fourcc, size_t size )
{
	AviRiffTreeItem *item, *groupItem;

	if ( riffSize > 0 )
	{
		int prog;

		prog = (1000llu * fpos) / riffSize;

		if ( prog > 1000 )
		{
			prog = 1000;
		}
		if ( progressDialog )
		{
			if (progressDialog->wasCanceled())
			{
				return -1;
			}
			progressDialog->setValue( prog );
		}
	}
	switch ( type )
	{
		case gwavi_t::RIFF_START:
		{
			item = new AviRiffTreeItem(type, fpos, fourcc, size);

			riffSize = size;

			itemStack.push_back(item);

			riffTree->addTopLevelItem(item);
		}
		break;
		case gwavi_t::RIFF_END:
		{
			groupItem = itemStack.back();
			groupItem->setExpanded(true);
			itemStack.pop_back();
		}
		break;
		case gwavi_t::LIST_START:
		{
			item = new AviRiffTreeItem(type, fpos, fourcc, size);

			groupItem = itemStack.back();

			itemStack.push_back(item);

			groupItem->addChild(item);
		}
		break;
		case gwavi_t::LIST_END:
		{
			itemStack.pop_back();
		}
		break;
		case gwavi_t::CHUNK_START:
		{
			item = new AviRiffTreeItem(type, fpos, fourcc, size);

			groupItem = itemStack.back();

			groupItem->addChild(item);

			lastChunk = item;

			processChunk(lastChunk);
		}
		break;
		default:
			// UnHandled Type
		break;
	}

	return 0;
}
//----------------------------------------------------------------------------
static bool isRiffTag( const char *fcc, int *matchIdx )
{
	int i=0;

	while ( riff_tags[i] != NULL )
	{
		if ( strcmp( fcc, riff_tags[i] ) == 0 )
		{
			if ( matchIdx )
			{
				*matchIdx = i;
			}
			return true;
		}
		i++;
	}
	return false;
}
//----------------------------------------------------------------------------
int  AviRiffViewerDialog::processChunk( AviRiffTreeItem *item )
{
	int i, riffIdx;
	QTreeWidgetItem *twi;
	char stmp[256];
	gwavi_dataBuffer data;

	if ( strcmp( item->getFourcc(), "avih" ) == 0 )
	{
		data.malloc( item->getSize()+8 );

		avi->getChunkData( item->filePos(), data.buf, item->getSize()+8 );

		sprintf( stmp, "%c%c%c%c", data.buf[0], data.buf[1], data.buf[2], data.buf[3] );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("fcc") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);

		sprintf( stmp, "%u", data.readU32(4) );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("cb") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);

		sprintf( stmp, "%u", data.readU32(8) );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("dwMicroSecPerFrame") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);

		sprintf( stmp, "%u", data.readU32(12) );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("dwMaxBytesPerSec") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);

		sprintf( stmp, "%u", data.readU32(16) );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("dwPaddingGranularity") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);

		sprintf( stmp, "0x%X", data.readU32(20) );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("dwFlags") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);

		sprintf( stmp, "%u", data.readU32(24) );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("dwTotalFrames") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);

		sprintf( stmp, "%u", data.readU32(28) );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("dwInitialFrames") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);

		sprintf( stmp, "%u", data.readU32(32) );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("dwStreams") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);

		sprintf( stmp, "%u", data.readU32(36) );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("dwSuggestedBufferSize") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);

		sprintf( stmp, "%u", data.readU32(40) );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("dwWidth") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);

		sprintf( stmp, "%u", data.readU32(44) );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("dwHeight") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);

		sprintf( stmp, "%u", data.readU32(48) );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("dwScale") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);

		sprintf( stmp, "%u", data.readU32(52) );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("dwRate") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);

		sprintf( stmp, "%u", data.readU32(56) );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("dwStart") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);

		sprintf( stmp, "%u", data.readU32(60) );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("dwLength") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);
	}
	else if ( strcmp( item->getFourcc(), "strh" ) == 0 )
	{
		data.malloc( item->getSize()+8 );

		avi->getChunkData( item->filePos(), data.buf, item->getSize()+8 );

		sprintf( stmp, "%c%c%c%c", data.buf[0], data.buf[1], data.buf[2], data.buf[3] );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("fcc") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);

		sprintf( stmp, "%u", data.readU32(4) );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("cb") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);

		sprintf( stmp, "%c%c%c%c", data.buf[8], data.buf[9], data.buf[10], data.buf[11] );
		strcpy( strhType, stmp );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("fccType") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);

		if ( isalnum(data.buf[12]) )
		{
			sprintf( stmp, "%c%c%c%c", data.buf[12], data.buf[13], data.buf[14], data.buf[15] );
		}
		else
		{
			sprintf( stmp, "0x%X", data.readU32(12) );
		}

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("fccHandler") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);

		sprintf( stmp, "0x%X", data.readU32(16) );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("dwFlags") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);

		sprintf( stmp, "%u", data.readU16(20) );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("wPriority") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);

		sprintf( stmp, "%u", data.readU16(22) );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("wLanguage") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);

		sprintf( stmp, "%u", data.readU32(24) );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("dwInitialFrames") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);

		sprintf( stmp, "%u", data.readU32(28) );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("dwScale") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);

		sprintf( stmp, "%u", data.readU32(32) );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("dwRate") );
		twi->setText( 2, tr(stmp) );

		if ( strcmp( strhType, "vids" ) == 0 )
		{
			sprintf( stmp, "(%13.10f Hz)", (double)data.readU32(32) / (double)data.readU32(28) );
			twi->setText( 3, tr(stmp) );
		}
		item->addChild(twi);

		sprintf( stmp, "%u", data.readU32(36) );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("dwStart") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);

		sprintf( stmp, "%u", data.readU32(40) );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("dwLength") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);

		sprintf( stmp, "%u", data.readU32(44) );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("dwSuggestedBufferSize") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);

		sprintf( stmp, "%i", data.readI32(48) );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("dwQuality") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);

		sprintf( stmp, "%u", data.readU32(52) );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("dwSampleSize") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);

		sprintf( stmp, "%u", data.readU16(56) );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("rcFrame.left") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);

		sprintf( stmp, "%u", data.readU16(58) );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("rcFrame.top") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);

		sprintf( stmp, "%u", data.readU16(60) );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("rcFrame.right") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);

		sprintf( stmp, "%u", data.readU16(62) );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("rcFrame.bottom") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);
	}
	else if ( strcmp( item->getFourcc(), "strf" ) == 0 )
	{
		if ( strcmp( strhType, "vids" ) == 0 )
		{
			data.malloc( item->getSize()+8 );

			avi->getChunkData( item->filePos(), data.buf, item->getSize()+8 );

			sprintf( stmp, "%c%c%c%c", data.buf[0], data.buf[1], data.buf[2], data.buf[3] );

			twi = new QTreeWidgetItem();
			twi->setText( 0, tr("fcc") );
			twi->setText( 2, tr(stmp) );
			item->addChild(twi);

			sprintf( stmp, "%u", data.readU32(4) );

			twi = new QTreeWidgetItem();
			twi->setText( 0, tr("cb") );
			twi->setText( 2, tr(stmp) );
			item->addChild(twi);

			sprintf( stmp, "%u", data.readU32(8) );

			twi = new QTreeWidgetItem();
			twi->setText( 0, tr("biSize") );
			twi->setText( 2, tr(stmp) );
			item->addChild(twi);

			sprintf( stmp, "%i", data.readI32(12) );

			twi = new QTreeWidgetItem();
			twi->setText( 0, tr("biWidth") );
			twi->setText( 2, tr(stmp) );
			item->addChild(twi);

			sprintf( stmp, "%i", data.readI32(16) );

			twi = new QTreeWidgetItem();
			twi->setText( 0, tr("biHeight") );
			twi->setText( 2, tr(stmp) );
			item->addChild(twi);

			sprintf( stmp, "%u", data.readU16(20) );

			twi = new QTreeWidgetItem();
			twi->setText( 0, tr("biPlanes") );
			twi->setText( 2, tr(stmp) );
			item->addChild(twi);

			sprintf( stmp, "%u", data.readU16(22) );

			twi = new QTreeWidgetItem();
			twi->setText( 0, tr("biBitCount") );
			twi->setText( 2, tr(stmp) );
			item->addChild(twi);

			if ( isalnum(data.buf[24]) )
			{
				sprintf( stmp, "%c%c%c%c", data.buf[24], data.buf[25], data.buf[26], data.buf[27] );
			}
			else
			{
				sprintf( stmp, "0x%X", data.readU32(24) );
			}

			twi = new QTreeWidgetItem();
			twi->setText( 0, tr("biCompression") );
			twi->setText( 2, tr(stmp) );
			item->addChild(twi);

			sprintf( stmp, "%u", data.readU32(28) );

			twi = new QTreeWidgetItem();
			twi->setText( 0, tr("biSizeImage") );
			twi->setText( 2, tr(stmp) );
			item->addChild(twi);

			sprintf( stmp, "%i", data.readI32(32) );

			twi = new QTreeWidgetItem();
			twi->setText( 0, tr("biXPelsPerMeter") );
			twi->setText( 2, tr(stmp) );
			item->addChild(twi);

			sprintf( stmp, "%i", data.readI32(36) );

			twi = new QTreeWidgetItem();
			twi->setText( 0, tr("biYPelsPerMeter") );
			twi->setText( 2, tr(stmp) );
			item->addChild(twi);

			sprintf( stmp, "%u", data.readU32(40) );

			twi = new QTreeWidgetItem();
			twi->setText( 0, tr("biClrUsed") );
			twi->setText( 2, tr(stmp) );
			item->addChild(twi);

			sprintf( stmp, "%u", data.readU32(44) );

			twi = new QTreeWidgetItem();
			twi->setText( 0, tr("biClrImportant") );
			twi->setText( 2, tr(stmp) );
			item->addChild(twi);
		}
		else if ( strcmp( strhType, "auds" ) == 0 )
		{
			data.malloc( item->getSize()+8 );

			avi->getChunkData( item->filePos(), data.buf, item->getSize()+8 );

			sprintf( stmp, "%c%c%c%c", data.buf[0], data.buf[1], data.buf[2], data.buf[3] );

			twi = new QTreeWidgetItem();
			twi->setText( 0, tr("fcc") );
			twi->setText( 2, tr(stmp) );
			item->addChild(twi);

			sprintf( stmp, "%u", data.readU32(4) );

			twi = new QTreeWidgetItem();
			twi->setText( 0, tr("cb") );
			twi->setText( 2, tr(stmp) );
			item->addChild(twi);

			sprintf( stmp, "%u", data.readU16(8) );

			twi = new QTreeWidgetItem();
			twi->setText( 0, tr("wFormatTag") );
			twi->setText( 2, tr(stmp) );
			item->addChild(twi);

			sprintf( stmp, "%u", data.readU16(10) );

			twi = new QTreeWidgetItem();
			twi->setText( 0, tr("nChannels") );
			twi->setText( 2, tr(stmp) );
			item->addChild(twi);

			sprintf( stmp, "%u", data.readU32(12) );

			twi = new QTreeWidgetItem();
			twi->setText( 0, tr("nSamplesPerSec") );
			twi->setText( 2, tr(stmp) );
			item->addChild(twi);

			sprintf( stmp, "%u", data.readU32(16) );

			twi = new QTreeWidgetItem();
			twi->setText( 0, tr("nAvgBytesPerSec") );
			twi->setText( 2, tr(stmp) );
			item->addChild(twi);

			sprintf( stmp, "%u", data.readU16(20) );

			twi = new QTreeWidgetItem();
			twi->setText( 0, tr("nBlockAlign") );
			twi->setText( 2, tr(stmp) );
			item->addChild(twi);

			sprintf( stmp, "%u", data.readU16(22) );

			twi = new QTreeWidgetItem();
			twi->setText( 0, tr("nBitsPerSample") );
			twi->setText( 2, tr(stmp) );
			item->addChild(twi);

			sprintf( stmp, "%u", data.readU16(24) );

			twi = new QTreeWidgetItem();
			twi->setText( 0, tr("cbSize") );
			twi->setText( 2, tr(stmp) );
			item->addChild(twi);
		}
	}
	else if ( isRiffTag( item->getFourcc(), &riffIdx ) )
	{
		int j=0;
		const char *name = "MetaData";

		while ( riff_info_conv[j] != NULL )
		{
			if ( strcmp( item->getFourcc(), riff_info_conv[j] ) == 0 )
			{
				name = riff_info_conv[j+1]; break;
			}
			j += 2;
		}

		data.malloc( item->getSize()+8 );

		avi->getChunkData( item->filePos(), data.buf, item->getSize()+8 );

		sprintf( stmp, "%c%c%c%c", data.buf[0], data.buf[1], data.buf[2], data.buf[3] );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("fcc") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);

		sprintf( stmp, "%u", data.readU32(4) );

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr("cb") );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);

		sprintf( stmp, "%u", data.readU32(8) );

		for (i=0; i<item->getSize(); i++)
		{
			if ( i >= ( sizeof(stmp)-1 ) )
			{
				i = sizeof(stmp)-1; break;
			}
			stmp[i] = data.buf[i+8];
		}
		stmp[i] = 0;

		twi = new QTreeWidgetItem();
		twi->setText( 0, tr(name) );
		twi->setText( 2, tr(stmp) );
		item->addChild(twi);
	}

	return 0;
}
//----------------------------------------------------------------------------
//--- AVI RIFF Tree View
//----------------------------------------------------------------------------
AviRiffTree::AviRiffTree(QWidget *parent)
	: QTreeWidget(parent)
{

}
//----------------------------------------------------------------------------
AviRiffTree::~AviRiffTree(void)
{

}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//--- AVI RIFF Viewer Dialog
//----------------------------------------------------------------------------
AviRiffTreeItem::AviRiffTreeItem(int typeIn, long long int fposIn, const char *fourccIn, size_t sizeIn, QTreeWidgetItem *parent)
	: QTreeWidgetItem(parent,1)
{
	char stmp[64];

	type = typeIn;
	fpos = fposIn;
	size = sizeIn;

	strcpy( fourcc, fourccIn );

	//sprintf( stmp, "0x%08llX", fposIn );
	
	switch ( type )
	{
		case gwavi_t::RIFF_START:
		case gwavi_t::RIFF_END:
			setText( 0, QString("<RIFF>") );
		break;
		case gwavi_t::LIST_START:
		case gwavi_t::LIST_END:
			setText( 0, QString("<LIST>") );
		break;
		default:
		case gwavi_t::CHUNK_START:
			setText( 0, QString("<CHUNK>") );
		break;
	}

	setText( 1, QString(fourcc) );

	if ( showSizeHex )
	{
		sprintf( stmp, "0x%08lX", (unsigned long)size );
	}
	else
	{
		sprintf( stmp, "%zu", size );
	}

	setText( 2, QString(stmp) );

	sprintf( stmp, "0x%08llX", fposIn );

	setText( 3, QString(stmp) );
}
//----------------------------------------------------------------------------
AviRiffTreeItem::~AviRiffTreeItem(void)
{

}
//----------------------------------------------------------------------------
