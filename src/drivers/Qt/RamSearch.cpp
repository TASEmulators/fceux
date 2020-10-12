// RamSearch.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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
#include "Qt/RamSearch.h"
#include "Qt/CheatsConf.h"
#include "Qt/ConsoleUtilities.h"

static bool ShowROM = false;
static RamSearchDialog_t *ramSearchWin = NULL;

static uint64_t iterMask = 0x01;

struct memoryState_t
{
	union {
	   int8_t    i;
	   uint8_t   u;
	} v8;
	union {
	   int16_t    i;
	   uint16_t   u;
	} v16;
	union {
	   int32_t    i;
	   uint32_t   u;
	} v32;
};

struct memoryLocation_t
{
	int  addr;

	memoryState_t  val;

	std::vector <memoryState_t> hist;

	uint32_t  chgCount;
	uint64_t  elimMask;

	memoryLocation_t(void)
	{
		addr = 0; val.v32.u = 0;
		chgCount = 0; elimMask = 0;
	}
};
static struct memoryLocation_t  memLoc[0x8000];

static std::list <struct memoryLocation_t*> actvSrchList;

static int dpySize = 'b';
static int dpyType = 's';

//----------------------------------------------------------------------------
void openRamSearchWindow( QWidget *parent )
{
	if ( ramSearchWin != NULL )
	{
		return;
	}
   ramSearchWin = new RamSearchDialog_t(parent);
	
   ramSearchWin->show();
}

//----------------------------------------------------------------------------
RamSearchDialog_t::RamSearchDialog_t(QWidget *parent)
	: QDialog( parent )
{
	QVBoxLayout *mainLayout;
	QHBoxLayout *hbox, *hbox1, *hbox2, *hbox3;
	QVBoxLayout *vbox, *vbox1, *vbox2;
	QTreeWidgetItem *item;
	QGridLayout *grid;
	QGroupBox *frame;

	font.setFamily("Courier New");
	font.setStyle( QFont::StyleNormal );
	font.setStyleHint( QFont::Monospace );

	QFontMetrics fm(font);

#if QT_VERSION > QT_VERSION_CHECK(5, 11, 0)
    fontCharWidth = 2 * fm.horizontalAdvance(QLatin1Char('2'));
#else
    fontCharWidth = 2 * fm.width(QLatin1Char('2'));
#endif

	setWindowTitle("RAM Search");

	resize( 512, 512 );

	mainLayout = new QVBoxLayout();
	hbox1      = new QHBoxLayout();

	mainLayout->addLayout( hbox1 );

	tree = new QTreeWidget();

	tree->setColumnCount(4);

	item = new QTreeWidgetItem();
	item->setText( 0, QString::fromStdString( "Address" ) );
	item->setText( 1, QString::fromStdString( "Value" ) );
	item->setText( 2, QString::fromStdString( "Previous" ) );
	item->setText( 3, QString::fromStdString( "Changes" ) );
	item->setTextAlignment( 0, Qt::AlignLeft);
	item->setTextAlignment( 1, Qt::AlignLeft);
	item->setTextAlignment( 2, Qt::AlignLeft);
	item->setTextAlignment( 3, Qt::AlignLeft);

	//connect( tree, SIGNAL(itemClicked(QTreeWidgetItem*, int)),
	//		   this, SLOT(watchClicked( QTreeWidgetItem*, int)) );

	tree->setHeaderItem( item );

	//tree->header()->setSectionResizeMode( QHeaderView::ResizeToContents );

	vbox  = new QVBoxLayout();
	hbox1->addWidget( tree  );
	hbox1->addLayout( vbox  );

	searchButton = new QPushButton( tr("Search") );
	vbox->addWidget( searchButton );
	connect( searchButton, SIGNAL(clicked(void)), this, SLOT(runSearch(void)));
	//searchButton->setEnabled(false);

	resetButton = new QPushButton( tr("Reset") );
	vbox->addWidget( resetButton );
	connect( resetButton, SIGNAL(clicked(void)), this, SLOT(resetSearch(void)));
	//down_btn->setEnabled(false);

	clearChangeButton = new QPushButton( tr("Clear Change") );
	vbox->addWidget( clearChangeButton );
	//connect( clearChangeButton, SIGNAL(clicked(void)), this, SLOT(editWatchClicked(void)));
	//clearChangeButton->setEnabled(false);

	undoButton = new QPushButton( tr("Remove") );
	vbox->addWidget( undoButton );
	//connect( undoButton, SIGNAL(clicked(void)), this, SLOT(removeWatchClicked(void)));
	//undoButton->setEnabled(false);
	
	searchROMCbox = new QCheckBox( tr("Search ROM") );
	vbox->addWidget( searchROMCbox );
	//connect( undoButton, SIGNAL(clicked(void)), this, SLOT(removeWatchClicked(void)));
	//undoButton->setEnabled(false);

	elimButton = new QPushButton( tr("Eliminate") );
	vbox->addWidget( elimButton );
	//connect( elimButton, SIGNAL(clicked(void)), this, SLOT(newWatchClicked(void)));
	//elimButton->setEnabled(false);

	watchButton = new QPushButton( tr("Watch") );
	vbox->addWidget( watchButton );
	//connect( watchButton, SIGNAL(clicked(void)), this, SLOT(dupWatchClicked(void)));
	watchButton->setEnabled(false);

	addCheatButton = new QPushButton( tr("Add Cheat") );
	vbox->addWidget( addCheatButton );
	//connect( addCheatButton, SIGNAL(clicked(void)), this, SLOT(sepWatchClicked(void)));
	addCheatButton->setEnabled(false);

	hexEditButton = new QPushButton( tr("Hex Editor") );
	vbox->addWidget( hexEditButton );
	//connect( hexEditButton, SIGNAL(clicked(void)), this, SLOT(sepWatchClicked(void)));
	hexEditButton->setEnabled(false);

	hbox2 = new QHBoxLayout();
	mainLayout->addLayout( hbox2 );
	frame = new QGroupBox( tr("Comparison Operator") );
	vbox  = new QVBoxLayout();

	hbox2->addWidget( frame );
	frame->setLayout(vbox);

	lt_btn = new QRadioButton( tr("Less Than") );
	gt_btn = new QRadioButton( tr("Greater Than") );
	le_btn = new QRadioButton( tr("Less Than or Equal To") );
	ge_btn = new QRadioButton( tr("Greater Than or Equal To") );
	eq_btn = new QRadioButton( tr("Equal To") );
	ne_btn = new QRadioButton( tr("Not Equal To") );
	df_btn = new QRadioButton( tr("Different By:") );
	md_btn = new QRadioButton( tr("Modulo") );

	eq_btn->setChecked(true);

	diffByEdit = new QLineEdit();
	moduloEdit = new QLineEdit();

	vbox->addWidget( lt_btn );
	vbox->addWidget( gt_btn );
	vbox->addWidget( le_btn );
	vbox->addWidget( ge_btn );
	vbox->addWidget( eq_btn );
	vbox->addWidget( ne_btn );

	hbox = new QHBoxLayout();
	vbox->addLayout( hbox );
	hbox->addWidget( df_btn );
	hbox->addWidget( diffByEdit );

	hbox = new QHBoxLayout();
	vbox->addLayout( hbox );
	hbox->addWidget( md_btn );
	hbox->addWidget( moduloEdit );

	vbox1 = new QVBoxLayout();
	grid  = new QGridLayout();
	hbox2->addLayout( vbox1 );
	frame = new QGroupBox( tr("Compare To/By") );
	frame->setLayout( grid );
	vbox1->addWidget( frame );

	pv_btn = new QRadioButton( tr("Previous Value") );
	sv_btn = new QRadioButton( tr("Specific Value:") );
	sa_btn = new QRadioButton( tr("Specific Address:") );
	nc_btn = new QRadioButton( tr("Number of Changes:") );

	pv_btn->setChecked(true);

	specValEdit   = new QLineEdit();
	specAddrEdit  = new QLineEdit();
	numChangeEdit = new QLineEdit();

	grid->addWidget( pv_btn       , 0, 0, Qt::AlignLeft );
	grid->addWidget( sv_btn       , 1, 0, Qt::AlignLeft );
	grid->addWidget( specValEdit  , 1, 1, Qt::AlignLeft );
	grid->addWidget( sa_btn       , 2, 0, Qt::AlignLeft );
	grid->addWidget( specAddrEdit , 2, 1, Qt::AlignLeft );
	grid->addWidget( nc_btn       , 3, 0, Qt::AlignLeft );
	grid->addWidget( numChangeEdit, 3, 1, Qt::AlignLeft );

	vbox  = new QVBoxLayout();
	hbox3 = new QHBoxLayout();
	frame = new QGroupBox( tr("Data Size") );
	frame->setLayout( vbox );
	vbox1->addLayout( hbox3 );
	hbox3->addWidget( frame );

	ds1_btn = new QRadioButton( tr("1 Byte") );
	ds2_btn = new QRadioButton( tr("2 Byte") );
	ds4_btn = new QRadioButton( tr("4 Byte") );
	misalignedCbox = new QCheckBox( tr("Check Misaligned") );
	misalignedCbox->setEnabled(false);

	ds1_btn->setChecked(true);

	vbox->addWidget( ds1_btn );
	vbox->addWidget( ds2_btn );
	vbox->addWidget( ds4_btn );
	vbox->addWidget( misalignedCbox );

	vbox  = new QVBoxLayout();
	vbox2 = new QVBoxLayout();
	frame = new QGroupBox( tr("Data Type / Display") );
	frame->setLayout( vbox );
	vbox2->addWidget( frame );
	hbox3->addLayout( vbox2 );

	signed_btn   = new QRadioButton( tr("Signed") );
	unsigned_btn = new QRadioButton( tr("Unsigned") );
	hex_btn      = new QRadioButton( tr("Hexadecimal") );

	vbox->addWidget( signed_btn );
	vbox->addWidget( unsigned_btn );
	vbox->addWidget( hex_btn );
	signed_btn->setChecked(true);

	autoSearchCbox = new QCheckBox( tr("Auto-Search") );
	autoSearchCbox->setEnabled(true);
	vbox2->addWidget( autoSearchCbox );

	setLayout( mainLayout );

	resetSearch();

	updateTimer  = new QTimer( this );

   connect( updateTimer, &QTimer::timeout, this, &RamSearchDialog_t::periodicUpdate );

	updateTimer->start( 100 ); // 10hz
}
//----------------------------------------------------------------------------
RamSearchDialog_t::~RamSearchDialog_t(void)
{
	updateTimer->stop();
	printf("Destroy RAM Watch Config Window\n");
	ramSearchWin = NULL;

	actvSrchList.clear();

	for (unsigned int addr=0; addr<0x08000; addr++)
	{
		memLoc[addr].hist.clear();
	}
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::closeEvent(QCloseEvent *event)
{
   printf("RAM Watch Close Window Event\n");
   done(0);
	deleteLater();
   event->accept();
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::closeWindow(void)
{
   //printf("Close Window\n");
   done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::periodicUpdate(void)
{
	refreshRamList();
}
//----------------------------------------------------------------------------
static unsigned int ReadValueAtHardwareAddress(int address, unsigned int size)
{
	unsigned int value = 0;

	// read as little endian
	for (unsigned int i = 0; i < size; i++)
	{
		value <<= 8;
		value |= GetMem(address);
		address++;
	}
	return value;
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::runSearch(void)
{
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::resetSearch(void)
{
	actvSrchList.clear();

	for (unsigned int addr=0; addr<0x08000; addr++)
	{
		memLoc[addr].hist.clear();
		memLoc[addr].addr      = addr;
		memLoc[addr].val.v8.u  = GetMem(addr);
		memLoc[addr].val.v16.u = ReadValueAtHardwareAddress(addr, 2);
		memLoc[addr].val.v32.u = ReadValueAtHardwareAddress(addr, 4);
		memLoc[addr].elimMask  = 0;
		memLoc[addr].hist.push_back( memLoc[addr].val );

		actvSrchList.push_back( &memLoc[addr] );
	}
	iterMask = 0x01;

	refreshRamList();
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::refreshRamList(void)
{
	int idx=0;
	QTreeWidgetItem *item;
	std::list <struct memoryLocation_t*>::iterator it;
	char addrStr[32], valStr[32], prevStr[32], chgStr[32];
	memoryLocation_t *loc;

	for (it=actvSrchList.begin(); it != actvSrchList.end(); it++)
	{
		loc = *it;

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

		sprintf (addrStr, "$%04X", loc->addr);

		if ( dpySize == 'd' )
		{
			if ( dpyType == 'h' )
			{
				sprintf( valStr , "0x%08X", loc->val.v32.u );
				sprintf( prevStr, "0x%08X", loc->hist.back().v32.u );
			}
			else if ( dpyType == 'u' )
			{
				sprintf( valStr , "%u", loc->val.v32.u );
				sprintf( prevStr, "%u", loc->hist.back().v32.u );
			}
			else 
			{
				sprintf( valStr , "%i", loc->val.v32.i );
				sprintf( prevStr, "%i", loc->hist.back().v32.i );
			}
		}
		else if ( dpySize == 'w' )
		{
			if ( dpyType == 'h' )
			{
				sprintf( valStr , "0x%04X", loc->val.v16.u );
				sprintf( prevStr, "0x%04X", loc->hist.back().v16.u );
			}
			else if ( dpyType == 'u' )
			{
				sprintf( valStr , "%u", loc->val.v16.u );
				sprintf( prevStr, "%u", loc->hist.back().v16.u );
			}
			else 
			{
				sprintf( valStr , "%i", loc->val.v16.i );
				sprintf( prevStr, "%i", loc->hist.back().v16.i );
			}
		}
		else 
		{
			if ( dpyType == 'h' )
			{
				sprintf( valStr , "0x%02X", loc->val.v8.u );
				sprintf( prevStr, "0x%02X", loc->hist.back().v8.u );
			}
			else if ( dpyType == 'u' )
			{
				sprintf( valStr , "%u", loc->val.v8.u );
				sprintf( prevStr, "%u", loc->hist.back().v8.u );
			}
			else 
			{
				sprintf( valStr , "%i", loc->val.v8.i );
				sprintf( prevStr, "%i", loc->hist.back().v8.i );
			}
		}
		sprintf( chgStr, "%i", loc->chgCount );

		item->setText( 0, tr(addrStr) );
		item->setText( 1, tr(valStr)  );
		item->setText( 2, tr(prevStr) );
		item->setText( 3, tr(chgStr)  );


		item->setTextAlignment( 0, Qt::AlignLeft);
		item->setTextAlignment( 1, Qt::AlignCenter);
		item->setTextAlignment( 2, Qt::AlignCenter);
		item->setTextAlignment( 3, Qt::AlignCenter);

		idx++;
	}

}
//----------------------------------------------------------------------------
