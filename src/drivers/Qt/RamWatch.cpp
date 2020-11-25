// RamWatch.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <string>

#include <SDL.h>
#include <QMenuBar>
#include <QAction>
#include <QHeaderView>
#include <QCloseEvent>
#include <QGroupBox>
#include <QLineEdit>
#include <QRadioButton>
#include <QFileDialog>

#include "../../types.h"
#include "../../fceu.h"
#include "../../cheat.h"
#include "../../debug.h"

#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/fceuWrapper.h"
#include "Qt/RamWatch.h"
#include "Qt/CheatsConf.h"
#include "Qt/ConsoleUtilities.h"

ramWatchList_t ramWatchList;
static RamWatchDialog_t *ramWatchMainWin = NULL;
//----------------------------------------------------------------------------
void openRamWatchWindow( QWidget *parent, int force )
{
   if ( !force )
   {
      if ( ramWatchMainWin != NULL ) return;
   }
   ramWatchMainWin = new RamWatchDialog_t(parent);

   ramWatchMainWin->show();
}
//----------------------------------------------------------------------------
RamWatchDialog_t::RamWatchDialog_t(QWidget *parent)
	: QDialog( parent, Qt::Window )
{
	QMenuBar *menuBar;
	QHBoxLayout *mainLayout;
	QVBoxLayout *vbox, *vbox1;
	QTreeWidgetItem *item;
	QMenu *fileMenu, *watchMenu;
	QAction *menuAct;
	QGroupBox *frame;
	int useNativeMenuBar;

	font.setFamily("Courier New");
	font.setStyle( QFont::StyleNormal );
	font.setStyleHint( QFont::Monospace );

	QFontMetrics fm(font);

#if QT_VERSION > QT_VERSION_CHECK(5, 11, 0)
    fontCharWidth = 2 * fm.horizontalAdvance(QLatin1Char('2'));
#else
    fontCharWidth = 2 * fm.width(QLatin1Char('2'));
#endif

	setWindowTitle("RAM Watch");

	resize( 512, 512 );

	menuBar = new QMenuBar(this);

	// This is needed for menu bar to show up on MacOS
	g_config->getOption( "SDL.UseNativeMenuBar", &useNativeMenuBar );

	menuBar->setNativeMenuBar( useNativeMenuBar ? true : false );

	//-----------------------------------------------------------------------
	// Menu 
	//-----------------------------------------------------------------------
	// File
   fileMenu = menuBar->addMenu(tr("File"));

	// File -> New List
	menuAct = new QAction(tr("New List"), this);
   menuAct->setShortcut( QKeySequence(tr("Ctrl+N")) );
   menuAct->setStatusTip(tr("New List"));
   connect(menuAct, SIGNAL(triggered()), this, SLOT(newListCB(void)) );

   fileMenu->addAction(menuAct);

	// File -> Open 
	menuAct = new QAction(tr("Open"), this);
   menuAct->setShortcut( QKeySequence(tr("Ctrl+O")) );
   menuAct->setStatusTip(tr("Open Watch File"));
   connect(menuAct, SIGNAL(triggered()), this, SLOT(openListCB(void)) );

   fileMenu->addAction(menuAct);

	// File -> Save 
	menuAct = new QAction(tr("Save"), this);
   menuAct->setShortcut( QKeySequence(tr("Ctrl+S")) );
   menuAct->setStatusTip(tr("Save Watch File"));
   connect(menuAct, SIGNAL(triggered()), this, SLOT(saveListCB(void)) );

   fileMenu->addAction(menuAct);

	// File -> Save As
	menuAct = new QAction(tr("Save As"), this);
   menuAct->setShortcut( QKeySequence(tr("Ctrl+Shift+S")) );
   menuAct->setStatusTip(tr("Save As Watch File"));
   connect(menuAct, SIGNAL(triggered()), this, SLOT(saveListAs(void)) );

   fileMenu->addAction(menuAct);

	// File -> Append
	menuAct = new QAction(tr("Append from File"), this);
   //menuAct->setShortcut( QKeySequence(tr("Ctrl+A")) );
   menuAct->setStatusTip(tr("Append from File"));
   connect(menuAct, SIGNAL(triggered()), this, SLOT(appendListCB(void)) );

   fileMenu->addAction(menuAct);

	fileMenu->addSeparator();

	// File -> Append
	menuAct = new QAction(tr("Close"), this);
   menuAct->setShortcut( QKeySequence(tr("Alt+F4")) );
   menuAct->setStatusTip(tr("Close Window"));
   connect(menuAct, SIGNAL(triggered()), this, SLOT(closeWindow(void)) );

   fileMenu->addAction(menuAct);

	// Watch
   watchMenu = menuBar->addMenu(tr("Watch"));

	// Watch -> New Watch
	menuAct = new QAction(tr("New Watch"), this);
   menuAct->setShortcut( QKeySequence(tr("N")) );
   menuAct->setStatusTip(tr("New Watch"));
   connect(menuAct, SIGNAL(triggered()), this, SLOT(newWatchClicked(void)) );

   watchMenu->addAction(menuAct);

	// Watch -> Edit Watch
	menuAct = new QAction(tr("Edit Watch"), this);
   menuAct->setShortcut( QKeySequence(tr("E")) );
   menuAct->setStatusTip(tr("Edit Watch"));
   connect(menuAct, SIGNAL(triggered()), this, SLOT(editWatchClicked(void)) );

   watchMenu->addAction(menuAct);

	// Watch -> Remove Watch
	menuAct = new QAction(tr("Remove Watch"), this);
   menuAct->setShortcut( QKeySequence(tr("R")) );
   menuAct->setStatusTip(tr("Remove Watch"));
   connect(menuAct, SIGNAL(triggered()), this, SLOT(removeWatchClicked(void)) );

   watchMenu->addAction(menuAct);

	// Watch -> Duplicate Watch
	menuAct = new QAction(tr("Duplicate Watch"), this);
   menuAct->setShortcut( QKeySequence(tr("A")) );
   menuAct->setStatusTip(tr("Duplicate Watch"));
   connect(menuAct, SIGNAL(triggered()), this, SLOT(dupWatchClicked(void)) );

   watchMenu->addAction(menuAct);

	// Watch -> Add Separator
	menuAct = new QAction(tr("Add Separator"), this);
   menuAct->setShortcut( QKeySequence(tr("S")) );
   menuAct->setStatusTip(tr("Add Separator"));
   //connect(menuAct, SIGNAL(triggered()), this, SLOT(newListCB(void)) );

   watchMenu->addAction(menuAct);

	watchMenu->addSeparator();

	// Watch -> Move Up
	menuAct = new QAction(tr("Move Up"), this);
   menuAct->setShortcut( QKeySequence(tr("U")) );
   menuAct->setStatusTip(tr("Move Up"));
   connect(menuAct, SIGNAL(triggered()), this, SLOT(moveWatchUpClicked(void)) );

   watchMenu->addAction(menuAct);

	// Watch -> Move Down
	menuAct = new QAction(tr("Move Down"), this);
   menuAct->setShortcut( QKeySequence(tr("D")) );
   menuAct->setStatusTip(tr("Move Down"));
   connect(menuAct, SIGNAL(triggered()), this, SLOT(moveWatchDownClicked(void)) );

   watchMenu->addAction(menuAct);

	//-----------------------------------------------------------------------
	// End Menu
	//-----------------------------------------------------------------------

	mainLayout = new QHBoxLayout();

	tree = new QTreeWidget();

	tree->setColumnCount(4);

	item = new QTreeWidgetItem();
	item->setText( 0, QString::fromStdString( "Address" ) );
	item->setText( 1, QString::fromStdString( "Value Dec" ) );
	item->setText( 2, QString::fromStdString( "Value Hex" ) );
	item->setText( 3, QString::fromStdString( "Notes" ) );
	item->setTextAlignment( 0, Qt::AlignLeft);
	item->setTextAlignment( 1, Qt::AlignLeft);
	item->setTextAlignment( 2, Qt::AlignLeft);
	item->setTextAlignment( 3, Qt::AlignLeft);

	connect( tree, SIGNAL(itemClicked(QTreeWidgetItem*, int)),
			   this, SLOT(watchClicked( QTreeWidgetItem*, int)) );

	tree->setHeaderItem( item );

	//tree->header()->setSectionResizeMode( QHeaderView::ResizeToContents );

	vbox1 = new QVBoxLayout();
	vbox  = new QVBoxLayout();
	frame = new QGroupBox( tr("Watches") );
	vbox1->addWidget( frame );
	frame->setLayout( vbox  );

	up_btn = new QPushButton( tr("Up") );
	vbox->addWidget( up_btn );
	connect( up_btn, SIGNAL(clicked(void)), this, SLOT(moveWatchUpClicked(void)));
	up_btn->setEnabled(false);

	down_btn = new QPushButton( tr("Down") );
	vbox->addWidget( down_btn );
	connect( down_btn, SIGNAL(clicked(void)), this, SLOT(moveWatchDownClicked(void)));
	down_btn->setEnabled(false);

	edit_btn = new QPushButton( tr("Edit") );
	vbox->addWidget( edit_btn );
	connect( edit_btn, SIGNAL(clicked(void)), this, SLOT(editWatchClicked(void)));
	edit_btn->setEnabled(false);

	del_btn = new QPushButton( tr("Remove") );
	vbox->addWidget( del_btn );
	connect( del_btn, SIGNAL(clicked(void)), this, SLOT(removeWatchClicked(void)));
	del_btn->setEnabled(false);

	new_btn = new QPushButton( tr("New") );
	vbox->addWidget( new_btn );
	connect( new_btn, SIGNAL(clicked(void)), this, SLOT(newWatchClicked(void)));
	new_btn->setEnabled(true);

	dup_btn = new QPushButton( tr("Duplicate") );
	vbox->addWidget( dup_btn );
	connect( dup_btn, SIGNAL(clicked(void)), this, SLOT(dupWatchClicked(void)));
	dup_btn->setEnabled(false);

	sep_btn = new QPushButton( tr("Separator") );
	vbox->addWidget( sep_btn );
	sep_btn->setEnabled(true);
	connect( sep_btn, SIGNAL(clicked(void)), this, SLOT(sepWatchClicked(void)));

	mainLayout->addWidget( tree  );
	mainLayout->addLayout( vbox1 );
	mainLayout->setMenuBar( menuBar );

	cht_btn = new QPushButton( tr("Add Cheat") );
	vbox1->addWidget( cht_btn );
	cht_btn->setEnabled(false);
	connect( cht_btn, SIGNAL(clicked(void)), this, SLOT(addCheatClicked(void)));

	setLayout( mainLayout );

   ramWatchMainWin = this;

	updateTimer  = new QTimer( this );

   connect( updateTimer, &QTimer::timeout, this, &RamWatchDialog_t::periodicUpdate );

	updateTimer->start( 100 ); // 10hz
}
//----------------------------------------------------------------------------
RamWatchDialog_t::~RamWatchDialog_t(void)
{
	updateTimer->stop();

   if ( ramWatchMainWin == this )
   {
      ramWatchMainWin = NULL;
   }
	printf("Destroy RAM Watch Config Window\n");
}
//----------------------------------------------------------------------------
void RamWatchDialog_t::closeEvent(QCloseEvent *event)
{
   printf("RAM Watch Close Window Event\n");
   done(0);
	deleteLater();
   event->accept();
}
//----------------------------------------------------------------------------
void RamWatchDialog_t::closeWindow(void)
{
   //printf("Close Window\n");
   done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
void RamWatchDialog_t::periodicUpdate(void)
{
	bool buttonEnable;
	QTreeWidgetItem *item;

	item = tree->currentItem();

	if ( item == NULL )
	{
		buttonEnable = false;
	}
	else
	{
		buttonEnable = true;
	}
	up_btn->setEnabled(buttonEnable);
	down_btn->setEnabled(buttonEnable);
	edit_btn->setEnabled(buttonEnable);
	del_btn->setEnabled(buttonEnable);
	dup_btn->setEnabled(buttonEnable);
	cht_btn->setEnabled(buttonEnable);


	updateRamWatchDisplay();

}
//----------------------------------------------------------------------------
void RamWatchDialog_t::updateRamWatchDisplay(void)
{
	int idx=0;
	QTreeWidgetItem *item;
	std::list < ramWatch_t * >::iterator it;
	char addrStr[32], valStr1[16], valStr2[16];
	ramWatch_t *rw;

	for (it = ramWatchList.ls.begin (); it != ramWatchList.ls.end (); it++)
	{
		rw = *it;

		item = tree->topLevelItem(idx);

		if ( item == NULL )
		{
			item = new QTreeWidgetItem();

			tree->addTopLevelItem( item );

			item->setFont( 0, font);
			item->setFont( 1, font);
			item->setFont( 2, font);
			item->setFont( 3, font);
		}
		if ( rw->isSep || (rw->addr < 0) )
		{
			strcpy (addrStr, "--------");
		}
		else
		{
			if ( rw->size > 1 )
			{
				sprintf (addrStr, "$%04X-$%04X", rw->addr, rw->addr + rw->size - 1);
			}
			else
			{
				sprintf (addrStr, "$%04X", rw->addr);
			}
		}

		rw->updateMem ();

		if ( rw->isSep || (rw->addr < 0) )
		{
			strcpy( valStr1, "--------");
			strcpy( valStr2, "--------");
		}
		else
		{
		   if (rw->size == 4)
		   {
		   	if (rw->type == 's')
		   	{
		   		sprintf (valStr1, "%i", rw->val.i32);
		   	}
		   	else
		   	{
		   		sprintf (valStr1, "%u", rw->val.u32);
		   	}
		   	sprintf (valStr2, "0x%08X", rw->val.u32);
		   }
		   else if (rw->size == 2)
		   {
		   	if (rw->type == 's')
		   	{
		   		sprintf (valStr1, "%6i", rw->val.i16);
		   	}
		   	else
		   	{
		   		sprintf (valStr1, "%6u", rw->val.u16);
		   	}
		   	sprintf (valStr2, "0x%04X", rw->val.u16);
		   }
		   else
		   {
		   	if (rw->type == 's')
		   	{
		   		sprintf (valStr1, "%6i", rw->val.i8);
		   	}
		   	else
		   	{
		   		sprintf (valStr1, "%6u", rw->val.u8);
		   	}
		   	sprintf (valStr2, "0x%02X", rw->val.u8);
		   }
		}

		item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemNeverHasChildren );

		if ( rw->isSep )
		{
			int i,j;
			char stmp[256];
			const char *c;

			j=0;

			for (i=0; i<3; i++)
			{
				stmp[j] = '-'; j++;
			}
			stmp[j] = ' '; j++;

			c = rw->name.c_str(); i = 0;

			while ( c[i] != 0 )
			{
				stmp[j] = c[i]; j++; i++;
			}
			stmp[j] = ' '; j++;

			while ( j < 64 )
			{
				stmp[j] = '-'; j++;
			}
			stmp[j] = 0;

			item->setFirstColumnSpanned(true);
			item->setText( 0, tr(stmp) );
		}
		else
		{
			item->setFirstColumnSpanned(false);
			item->setText( 0, tr(addrStr) );
			item->setText( 1, tr(valStr1) );
			item->setText( 2, tr(valStr2) );
			item->setText( 3, tr(rw->name.c_str())  );
		}
   
		item->setTextAlignment( 0, Qt::AlignLeft);
		item->setTextAlignment( 1, Qt::AlignCenter);
		item->setTextAlignment( 2, Qt::AlignCenter);
		item->setTextAlignment( 3, Qt::AlignLeft);

		idx++;
	}
	tree->viewport()->update();
}
//----------------------------------------------------------------------------
void 	RamWatchDialog_t::watchClicked( QTreeWidgetItem *item, int column)
{
//	int row = tree->indexOfTopLevelItem(item);

}
//----------------------------------------------------------------------------
void RamWatchDialog_t::newListCB(void)
{
	ramWatchList.clear();
	tree->clear();
	tree->viewport()->update();
}
//----------------------------------------------------------------------------
void RamWatchDialog_t::openListCB(void)
{
	int ret, useNativeFileDialogVal;
	QString filename;
	const char *romFile = NULL;
	QFileDialog  dialog(this, tr("Open Watch File") );

	dialog.setFileMode(QFileDialog::ExistingFile);

	dialog.setNameFilter(tr("Watch files (*.wch *.WCH) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::AllDirs | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Open") );

	//g_config->getOption ("SDL.LastOpenFile", &last );

	romFile = getRomFile();

	if ( romFile != NULL )
	{
		char dir[512], base[256];

		parseFilepath( romFile, dir, base );

		strcat( base, ".wch");

		dialog.setDirectory( tr(dir) );

		dialog.selectFile( tr(base) );
	}

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
	//qDebug() << "selected file path : " << filename.toUtf8();

	loadWatchFile ( filename.toStdString().c_str() );

   return;
}
//----------------------------------------------------------------------------
void RamWatchDialog_t::appendListCB(void)
{
	int ret, useNativeFileDialogVal;
	QString filename;
	const char *romFile = NULL;
	QFileDialog  dialog(this, tr("Append from Watch File") );

	dialog.setFileMode(QFileDialog::ExistingFile);

	dialog.setNameFilter(tr("Watch Files (*.wch *.WCH) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::AllDirs | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Load") );
	dialog.setDefaultSuffix( tr(".wch") );

	romFile = getRomFile();

	if ( romFile != NULL )
	{
		char dir[512], base[256];

		parseFilepath( romFile, dir, base );

		strcat( base, ".wch");

		dialog.setDirectory( tr(dir) );

		dialog.selectFile( tr(base) );
	}

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
	//qDebug() << "selected file path : " << filename.toUtf8();

	loadWatchFile( filename.toStdString().c_str(), 1 );
}
//----------------------------------------------------------------------------
void RamWatchDialog_t::saveListCB(void)
{
	if ( ramWatchList.size() == 0 )
	{
		return;
	}

	if ( saveFileName.size() > 0 )
	{
		char file[512];

		strcpy( file, saveFileName.c_str() );

		saveWatchFile( file );
	}
	else
	{
		saveListAs();
	}
}
//----------------------------------------------------------------------------
void RamWatchDialog_t::saveListAs(void)
{
	int ret, useNativeFileDialogVal;
	QString filename;
	const char *romFile = NULL;
	QFileDialog  dialog(this, tr("Save Watch List To File") );

	if ( ramWatchList.size() == 0 )
	{
		return;
	}
	dialog.setFileMode(QFileDialog::AnyFile);

	dialog.setNameFilter(tr("Watch Files (*.wch *.WCH) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::AllDirs | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Save") );
	dialog.setDefaultSuffix( tr(".wch") );

	romFile = getRomFile();

	if ( romFile != NULL )
	{
		char dir[512], base[256];

		parseFilepath( romFile, dir, base );

		strcat( base, ".wch");

		dialog.setDirectory( tr(dir) );

		dialog.selectFile( tr(base) );
	}

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
	//qDebug() << "selected file path : " << filename.toUtf8();

	saveWatchFile( filename.toStdString().c_str() );
}
//----------------------------------------------------------------------------
void ramWatch_t::updateMem (void)
{
	if ( addr < 0 )
	{
		return;
	}
	if (size == 1)
	{
		val.u8 = GetMem (addr);
	}
	else if (size == 2)
	{
		val.u16 = GetMem (addr) | (GetMem (addr + 1) << 8);
	}
	else
	{
		val.u8 = GetMem (addr);
	}
}
//----------------------------------------------------------------------------
void RamWatchDialog_t::openWatchEditWindow( ramWatch_t *rw, int mode)
{
	int ret, isSep = 0;
	QDialog dialog(this);
	QVBoxLayout *mainLayout, *vbox;
	QHBoxLayout *hbox;
	QLabel *lbl;
	QLineEdit *addrEntry, *notesEntry;
	QGroupBox *frame;
	QRadioButton *signedTypeBtn, *unsignedTypeBtn;
	QRadioButton *dataSize1Btn, *dataSize2Btn, *dataSize4Btn;
	QPushButton *cancelButton, *okButton;

	if ( rw == NULL )
	{
		dialog.setWindowTitle("Add Watch");
	}
	else
	{
		if ( rw->isSep )
		{
			if ( mode )
			{
				dialog.setWindowTitle("Add Separator");
			}
			else
			{
				dialog.setWindowTitle("Edit Separator");
			}
		}
		else
		{
			dialog.setWindowTitle("Edit Watch");
		}
	}	

	mainLayout = new QVBoxLayout();

	dialog.setLayout( mainLayout );

	hbox      = new QHBoxLayout();
	lbl       = new QLabel( tr("Address") );
	addrEntry = new QLineEdit();

	addrEntry->setMaxLength(4);
	addrEntry->setInputMask( ">HHHH;" );
	addrEntry->setCursorPosition(0);

	mainLayout->addLayout( hbox );
	hbox->addWidget( lbl );
	hbox->addWidget( addrEntry );

	hbox       = new QHBoxLayout();
	lbl        = new QLabel( tr("Notes") );
	notesEntry = new QLineEdit();

	mainLayout->addLayout( hbox );
	hbox->addWidget( lbl );
	hbox->addWidget( notesEntry );

	hbox  = new QHBoxLayout();
	mainLayout->addLayout( hbox );

	vbox  = new QVBoxLayout();
	frame = new QGroupBox( tr("Data Type") );
	hbox->addWidget( frame );
	frame->setLayout( vbox  );

	signedTypeBtn   = new QRadioButton( tr("Signed") );
	unsignedTypeBtn = new QRadioButton( tr("Unsigned") );

	vbox->addWidget( signedTypeBtn   );
	vbox->addWidget( unsignedTypeBtn );

	vbox  = new QVBoxLayout();
	frame = new QGroupBox( tr("Data Size") );
	hbox->addWidget( frame );
	frame->setLayout( vbox  );

	dataSize1Btn = new QRadioButton( tr("1 Byte") );
	dataSize2Btn = new QRadioButton( tr("2 Bytes") );
	dataSize4Btn = new QRadioButton( tr("4 Bytes") );

	vbox->addWidget( dataSize1Btn );
	vbox->addWidget( dataSize2Btn );
	vbox->addWidget( dataSize4Btn );

	hbox = new QHBoxLayout();
	mainLayout->addLayout( hbox );

	okButton     = new QPushButton( tr("OK") );
	cancelButton = new QPushButton( tr("Cancel") );
	hbox->addWidget( cancelButton );
	hbox->addWidget( okButton );
	okButton->setDefault(true);

	connect(     okButton, SIGNAL(clicked(void)), &dialog, SLOT(accept(void)) );
   connect( cancelButton, SIGNAL(clicked(void)), &dialog, SLOT(reject(void)) );

	if ( rw != NULL )
	{
		char stmp[64];

		isSep = rw->isSep;

		if ( (rw->addr >= 0) && !rw->isSep )
		{
			sprintf( stmp, "%04X", rw->addr );
			addrEntry->setText( tr(stmp) );
		}
		else
		{
			addrEntry->setEnabled(false);
		}

		notesEntry->setText( tr(rw->name.c_str()) );

		if ( rw->isSep )
		{
			signedTypeBtn->setEnabled(false);
			unsignedTypeBtn->setEnabled(false);
			dataSize1Btn->setEnabled(false);
			dataSize2Btn->setEnabled(false);
			dataSize4Btn->setEnabled(false);
		}
		else
		{
	   	signedTypeBtn->setChecked( rw->type == 's' );
	   	unsignedTypeBtn->setChecked( rw->type != 's' );
			dataSize1Btn->setChecked( rw->size == 1 );
			dataSize2Btn->setChecked( rw->size == 2 );
			dataSize4Btn->setChecked( rw->size == 4 );
		}
	}
	else
	{
	   signedTypeBtn->setChecked( true );
	   unsignedTypeBtn->setChecked( false );
		dataSize1Btn->setChecked( true );
		dataSize2Btn->setChecked( false );
		dataSize4Btn->setChecked( false );
	}

	ret = dialog.exec();

	if ( ret == QDialog::Accepted )
	{
		int addr = -1, size = 1;

		addr = ::strtol( addrEntry->text().toStdString().c_str(), NULL, 16 );

		if ( dataSize4Btn->isChecked() )
		{
			size = 4;
		}
		else if ( dataSize2Btn->isChecked() )
		{
			size = 2;
		}
		else
		{
			size = 1;
		}	

		if ( (rw == NULL) || mode )
		{
			ramWatchList.add_entry( notesEntry->text().toStdString().c_str(), 
				addr, unsignedTypeBtn->isChecked(), size, isSep);
		}
		else 
		{
			rw->name  = notesEntry->text().toStdString();
			rw->type  = unsignedTypeBtn->isChecked() ? 'u' : 's';
			rw->addr  = addr;
			rw->size  = size;
			rw->isSep = isSep;
		}
	}
}
//----------------------------------------------------------------------------
void RamWatchDialog_t::addCheatClicked(void)
{
	ramWatch_t *rw = NULL;
	QTreeWidgetItem *item;

	item = tree->currentItem();

	if ( item == NULL )
	{
		printf( "No Item Selected\n");
		return;
	}
	int row = tree->indexOfTopLevelItem(item);

	if ( row >= 0 )
	{
		rw = ramWatchList.getIndex(row);
	}

	if ( rw != NULL )
	{
		FCEUI_AddCheat( rw->name.c_str(), rw->addr, GetMem(rw->addr), -1, 1 );

      updateCheatDialog();
	}
}
//----------------------------------------------------------------------------
void RamWatchDialog_t::newWatchClicked(void)
{
	openWatchEditWindow();
}
//----------------------------------------------------------------------------
void RamWatchDialog_t::sepWatchClicked(void)
{
	ramWatch_t rw;

	rw.addr  = -1;
	rw.isSep =  1;

	openWatchEditWindow( &rw, 1 );
}
//----------------------------------------------------------------------------
void RamWatchDialog_t::editWatchClicked(void)
{
	ramWatch_t *rw = NULL;
	QTreeWidgetItem *item;

	item = tree->currentItem();

	if ( item == NULL )
	{
		printf( "No Item Selected\n");
		return;
	}
	int row = tree->indexOfTopLevelItem(item);

	if ( row >= 0 )
	{
		rw = ramWatchList.getIndex(row);
	}
	openWatchEditWindow(rw, 0);
}
//----------------------------------------------------------------------------
void RamWatchDialog_t::dupWatchClicked(void)
{
	ramWatch_t *rw = NULL;
	QTreeWidgetItem *item;

	item = tree->currentItem();

	if ( item == NULL )
	{
		printf( "No Item Selected\n");
		return;
	}
	int row = tree->indexOfTopLevelItem(item);

	if ( row >= 0 )
	{
		rw = ramWatchList.getIndex(row);
	}
	openWatchEditWindow(rw, 1);
}
//----------------------------------------------------------------------------
void RamWatchDialog_t::removeWatchClicked(void)
{
	QTreeWidgetItem *item;

	item = tree->currentItem();

	if ( item == NULL )
	{
		printf( "No Item Selected\n");
		return;
	}
	int row = tree->indexOfTopLevelItem(item);

	ramWatchList.deleteIndex(row);

	tree->clear();

	updateRamWatchDisplay();

	if ( row > 0 )
	{
		if ( row >= tree->topLevelItemCount() ) 
		{
			item = tree->topLevelItem( tree->topLevelItemCount()-1 );
		}
		else
		{
			item = tree->topLevelItem( row );
		}
	}
	else
	{
		item = tree->topLevelItem( 0 );
	}

	if ( item )
	{
		tree->setCurrentItem(item);
		tree->viewport()->update();
	}
}
//----------------------------------------------------------------------------
void RamWatchDialog_t::moveWatchUpClicked(void)
{
	QTreeWidgetItem *item;

	item = tree->currentItem();

	if ( item == NULL )
	{
		printf( "No Item Selected\n");
		return;
	}
	int row = tree->indexOfTopLevelItem(item);

	if ( row > 0 )
	{
		ramWatchList.moveIndexUp(row);
		item = tree->topLevelItem( row-1 );

		if ( item )
		{
			tree->setCurrentItem(item);
			tree->viewport()->update();
		}
	}
}
//----------------------------------------------------------------------------
void RamWatchDialog_t::moveWatchDownClicked(void)
{
	QTreeWidgetItem *item;

	item = tree->currentItem();

	if ( item == NULL )
	{
		printf( "No Item Selected\n");
		return;
	}
	int row = tree->indexOfTopLevelItem(item);

	if ( row < (tree->topLevelItemCount()-1) )
	{
	   ramWatchList.moveIndexDown(row);

		item = tree->topLevelItem( row+1 );

		if ( item )
		{
			tree->setCurrentItem(item);
			tree->viewport()->update();
		}
	}
}
//----------------------------------------------------------------------------
void RamWatchDialog_t::saveWatchFile (const char *filename, int append )
{
	int i, lineCount = 0, sizeChar;
	FILE *fp;
	const char *c, *mode;
	std::list < ramWatch_t * >::iterator it;
	ramWatch_t *rw;

	mode = (append) ? "a" : "w";

	fp = fopen (filename, mode);

	if (fp == NULL)
	{
		printf ("Error: Failed to open file: %s\n", filename);
		return;
	}
	saveFileName.assign( filename );

	fprintf( fp, "\n%zi\n", ramWatchList.size() );

	for (it = ramWatchList.ls.begin (); it != ramWatchList.ls.end (); it++)
	{
		rw = *it;

		c = rw->name.c_str ();

		if ( rw->isSep )
		{
			fprintf (fp, "%05i %04X  %c  %c  0  ", lineCount, rw->addr, 'S', 'S' );
		}
		else
		{
			if ( rw->size == 4 )
			{
				sizeChar = 'd';
			}
			else if ( rw->size == 2 )
			{
				sizeChar = 'w';
			}
			else 
			{
				sizeChar = 'b';
			}
			fprintf (fp, "%05i %04X  %c  %c  0  ", lineCount, rw->addr, sizeChar, rw->type);
		}

		i = 0;
		while (c[i])
		{
			if (c[i] == '"')
			{
				fprintf (fp, "\\%c", c[i]);
			}
			else
			{
				fprintf (fp, "%c", c[i]);
			}
			i++;
		}
		fprintf (fp, "\n");

		lineCount++;
	}
	fclose (fp);

}
//----------------------------------------------------------------------------
void RamWatchDialog_t::loadWatchFile (const char *filename, int append )
{
	FILE *fp;
	int i, j, a, t, s, isSep, literal=0;
	char line[512], stmp[512];
	ramWatch_t *rw;

	fp = fopen (filename, "r");

	if (fp == NULL)
	{
		printf ("Error: Failed to open file: %s\n", filename);
		return;
	}
	saveFileName.assign( filename );

	if ( !append )
	{
		ramWatchList.clear();
		tree->clear();
		tree->viewport()->update();
	}

	while (fgets (line, sizeof (line) - 1, fp) != NULL)
	{
		a = -1;
		t = -1;
		s = -1;
		isSep = 0;
		// Check for Comments
		i = 0;
		while (line[i] != 0)
		{
			if (literal)
			{
				literal = 0;
			}
			else
			{
				if (line[i] == '#')
				{
					line[i] = 0;
					break;
				}
				else if (line[i] == '\\')
				{
					literal = 1;
				}
			}
			i++;
		}

		i = 0;
		j = 0;
		while (isspace (line[i])) i++;

		while ( isdigit(line[i]) ) i++;

		while (isspace (line[i])) i++;

		if ((line[i] == '0') && (tolower (line[i + 1]) == 'x'))
		{
			i += 2;

			while (isxdigit (line[i]))
			{
				stmp[j] = line[i];
				i++;
				j++;
			}
		}
		else
		{
			while (isxdigit (line[i]))
			{
				stmp[j] = line[i];
				i++;
				j++;
			}
		}
		stmp[j] = 0;

		if (j == 0) continue;

		a = strtol (stmp, NULL, 16);

		while (isspace (line[i])) i++;

		s = tolower(line[i]);
		i++;

		while (isspace (line[i])) i++;

		t = tolower(line[i]);
		i++;

		if ((t != 'u') && (t != 's') && (t != 'h') && (t != 'b') )
		{
			printf ("Error: Invalid RAM Watch Byte Type: %c", t);
			continue;
		}
		if (!isdigit (s) && (s != 's') && (s != 'b') && (s != 'w') && (s != 'd') )
		{
			printf ("Error: Invalid RAM Watch Byte Size: %c", s);
			continue;
		}
		if (s == 's')
		{
			isSep = 1; s = 1;
		}
		else if ( s == 'b' )
		{
			s = 1;
		}
		else if ( s == 'w' )
		{
			s = 2;
		}
		else if ( s == 'd' )
		{
			s = 4;
		}
		else if ( isdigit(s) )
		{
			s = s - '0';
		}

		if ((s != 1) && (s != 2) && (s != 4))
		{
			printf ("Error: Invalid RAM Watch Byte Size: %i", s);
			continue;
		}
		while (isspace (line[i])) i++;

		while (isdigit(line[i])) i++;

		while (isspace (line[i])) i++;

		if (line[i] == '"')
		{
			i++;
			j = 0;
			literal = 0;
			while ((line[i] != 0))
			{
				if (literal)
				{
					literal = 0;
				}
				else
				{
					if (line[i] == '"')
					{
						break;
					}
					else if (line[i] == '\\')
					{
						literal = 1;
					}
				}
				if (!literal)
				{
					stmp[j] = line[i];
					j++;
				}
				i++;
			}
			stmp[j] = 0;
		}
		else
		{
			j=0;
			while (line[i] != 0)
			{
				if ( line[i] == '\n')
				{
					break;
				}
				stmp[j] = line[i]; i++; j++;
			}
			stmp[j] = 0;
		}

		j--;
		while ( j >= 0 )
		{
			if ( isspace(stmp[j]) )
			{
				stmp[j] = 0;
			}
			else
			{
				break;
			}
			j--;
		}
		rw = new ramWatch_t;

		rw->addr  = a;
		rw->type  = t;
		rw->size  = s;
		rw->isSep = isSep;
		rw->name.assign (stmp);

		ramWatchList.ls.push_back (rw);
	}

	fclose (fp);
}
//----------------------------------------------------------------------------
