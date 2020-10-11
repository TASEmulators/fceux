// HotKeyConf.cpp
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
#include "Qt/RamSearch.h"
#include "Qt/ConsoleUtilities.h"

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
	//connect( searchButton, SIGNAL(clicked(void)), this, SLOT(moveWatchUpClicked(void)));
	//searchButton->setEnabled(false);

	resetButton = new QPushButton( tr("Reset") );
	vbox->addWidget( resetButton );
	//connect( resetButton, SIGNAL(clicked(void)), this, SLOT(moveWatchDownClicked(void)));
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

	updateTimer  = new QTimer( this );

   connect( updateTimer, &QTimer::timeout, this, &RamSearchDialog_t::periodicUpdate );

	updateTimer->start( 100 ); // 10hz
}
//----------------------------------------------------------------------------
RamSearchDialog_t::~RamSearchDialog_t(void)
{
	updateTimer->stop();
	printf("Destroy RAM Watch Config Window\n");
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
}
//----------------------------------------------------------------------------
