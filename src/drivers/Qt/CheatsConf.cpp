// HotKeyConf.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <SDL.h>
#include <QHeaderView>

#include "../../types.h"
#include "../../fceu.h"
#include "../../cheat.h"
#include "../../debug.h"
#include "../../driver.h"
#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/fceuWrapper.h"
#include "Qt/CheatsConf.h"

static GuiCheatsDialog_t *win = NULL;
//----------------------------------------------------------------------------
GuiCheatsDialog_t::GuiCheatsDialog_t(QWidget *parent)
	: QDialog( parent )
{
	QHBoxLayout *mainLayout, *hbox, *hbox1;
	QVBoxLayout *vbox, *vbox1, *vbox2, *vbox3;
	QTreeWidgetItem *item;
	QLabel *lbl;
	QGroupBox *groupBox;
	QFrame *frame;
	QFont font;

	font.setStyle( QFont::StyleNormal );
	font.setStyleHint( QFont::Monospace );

	setWindowTitle("Cheat Search");

	//resize( 512, 512 );

	// Window Layout Box
	mainLayout = new QHBoxLayout();

	//-------------------------------------------------------
	// Left Side Active Cheats Frame
	actCheatFrame = new QGroupBox( tr("Active Cheats") );

	vbox1 = new QVBoxLayout();

	mainLayout->addWidget( actCheatFrame );
	
	tree = new QTreeWidget();

	tree->setColumnCount(2);

	item = new QTreeWidgetItem();
	item->setFont( 0, font );
	item->setFont( 1, font );
	item->setText( 0, QString::fromStdString( "Code" ) );
	item->setText( 1, QString::fromStdString( "Name" ) );
	item->setTextAlignment( 0, Qt::AlignLeft);
	item->setTextAlignment( 1, Qt::AlignLeft);

	tree->setHeaderItem( item );

	tree->header()->setSectionResizeMode( QHeaderView::ResizeToContents );

	//for (int i=0; i<HK_MAX; i++)
	//{
	//	std::string optionName = prefix + getHotkeyString(i);

	//	g_config->getOption (optionName.c_str (), &keycode);

	//	item = new QTreeWidgetItem();

	//	item->setText( 0, QString::fromStdString( optionName ) );
	//	item->setText( 1, QString::fromStdString( SDL_GetKeyName (keycode) ) );

	//	item->setTextAlignment( 0, Qt::AlignLeft);
	//	item->setTextAlignment( 1, Qt::AlignCenter);

	//	tree->addTopLevelItem( item );
	//}
	//
	
	vbox1->addWidget( tree );

	hbox = new QHBoxLayout();

	lbl = new QLabel( tr("Name:") );
	cheatNameEntry = new QLineEdit();
	cheatNameEntry->setFont( font );

	hbox->addWidget( lbl );
	hbox->addWidget( cheatNameEntry );

	vbox1->addLayout( hbox );

	hbox = new QHBoxLayout();

	lbl = new QLabel( tr("Address:") );
	cheatAddrEntry = new QLineEdit();
	cheatAddrEntry->setMaxLength(4);
	cheatAddrEntry->setInputMask( ">HHHH;0" );
	cheatAddrEntry->setFont( font );
	cheatAddrEntry->setCursorPosition(0);
	cheatAddrEntry->setMaximumWidth( 5 * cheatAddrEntry->fontMetrics().boundingRect('0').width() );

	hbox->addWidget( lbl );
	hbox->addWidget( cheatAddrEntry );

	lbl = new QLabel( tr("Value:") );
	cheatValEntry = new QLineEdit();
	cheatValEntry->setMaxLength(2);
	cheatValEntry->setInputMask( ">HH;0" );
	cheatValEntry->setFont( font );
	cheatValEntry->setCursorPosition(0);
	cheatValEntry->setMaximumWidth( 3 * cheatValEntry->fontMetrics().boundingRect('0').width() );

	hbox->addWidget( lbl );
	hbox->addWidget( cheatValEntry );

	lbl = new QLabel( tr("Compare:") );
	cheatCmpEntry = new QLineEdit();
	cheatCmpEntry->setMaxLength(2);
	cheatCmpEntry->setInputMask( ">HH;X" );
	cheatCmpEntry->setFont( font );
	cheatCmpEntry->setCursorPosition(0);
	cheatCmpEntry->setMaximumWidth( 3 * cheatCmpEntry->fontMetrics().boundingRect('0').width() );

	hbox->addWidget( lbl );
	hbox->addWidget( cheatCmpEntry );

	vbox1->addLayout( hbox );

	hbox = new QHBoxLayout();

	addCheatBtn = new QPushButton( tr("Add") );
	delCheatBtn = new QPushButton( tr("Delete") );
	modCheatBtn = new QPushButton( tr("Update") );

	hbox->addWidget( addCheatBtn );
	hbox->addWidget( delCheatBtn );
	hbox->addWidget( modCheatBtn );

	vbox1->addLayout( hbox );

	hbox = new QHBoxLayout();

	importCheatFileBtn = new QPushButton( tr("Import") );
	exportCheatFileBtn = new QPushButton( tr("Export") );
	importCheatFileBtn->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	exportCheatFileBtn->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );

	hbox->addWidget( importCheatFileBtn );
	hbox->addWidget( exportCheatFileBtn );

	vbox1->addLayout( hbox );

	actCheatFrame->setLayout( vbox1 );

	cheatSearchFrame = new QGroupBox( tr("Cheat Search") );
	cheatResultFrame = new QGroupBox( tr("Possibilities") );

	srchResults = new QTreeWidget();
	srchResults->setColumnCount(3);

	item = new QTreeWidgetItem();
	item->setFont( 0, font );
	item->setFont( 1, font );
	item->setFont( 2, font );
	item->setText( 0, QString::fromStdString( "Addr" ) );
	item->setText( 1, QString::fromStdString( "Pre"  ) );
	item->setText( 2, QString::fromStdString( "Cur"  ) );
	item->setTextAlignment( 0, Qt::AlignCenter);
	item->setTextAlignment( 1, Qt::AlignCenter);
	item->setTextAlignment( 2, Qt::AlignCenter);

	srchResults->setHeaderItem( item );

	//srchResults->header()->setSectionResizeMode( QHeaderView::ResizeToContents );
	srchResults->header()->setSectionResizeMode( QHeaderView::Interactive );
	//srchResults->header()->setSectionResizeMode( QHeaderView::Fixed );
	//srchResults->header()->setSectionResizeMode( QHeaderView::Stretch );
	//srchResults->header()->setDefaultSectionSize( 200 );
	//srchResults->setReadOnly(true);

	vbox = new QVBoxLayout();
	vbox->addWidget( srchResults );
	cheatResultFrame->setLayout( vbox );

	hbox1 = new QHBoxLayout();

	vbox2 = new QVBoxLayout();

	hbox1->addLayout( vbox2 );
	hbox1->addWidget( cheatResultFrame );

	cheatSearchFrame->setLayout( hbox1 );

	srchResetBtn = new QPushButton( tr("Reset") );
	srchResetBtn->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );

	vbox2->addWidget( srchResetBtn );

	frame = new QFrame();
	frame->setFrameShape( QFrame::Box );
	frame->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	vbox2->addWidget( frame );
	vbox = new QVBoxLayout();

	frame->setLayout( vbox );

	knownValBtn  = new QPushButton( tr("Known Value:") );
	knownValBtn->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );

	vbox->addWidget( knownValBtn  );

	hbox1 = new QHBoxLayout();
	vbox->addLayout( hbox1 );

	lbl = new QLabel( tr("0x") );
	lbl->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	knownValEntry = new QLineEdit();
	knownValEntry->setMaxLength(2);
	knownValEntry->setInputMask( ">HH;0" );
	knownValEntry->setFont( font );
	knownValEntry->setCursorPosition(0);
	knownValEntry->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	knownValEntry->setMaximumWidth( 3 * knownValEntry->fontMetrics().boundingRect('0').width() );
	hbox1->addWidget( lbl, 0, Qt::AlignRight );
	hbox1->addWidget( knownValEntry, 0, Qt::AlignLeft );

	groupBox = new QGroupBox( tr("Previous Compare") );
	vbox2->addWidget( groupBox );
	groupBox->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );

	vbox3 = new QVBoxLayout();

	frame = new QFrame();
	frame->setFrameShape( QFrame::Box );
	frame->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	vbox3->addWidget( frame );
	vbox = new QVBoxLayout();

	eqValBtn = new QPushButton( tr("Equal") );
	eqValBtn->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	vbox->addWidget( eqValBtn );

	frame->setLayout( vbox );

	frame = new QFrame();
	frame->setFrameShape( QFrame::Box );
	frame->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	vbox3->addWidget( frame );
	vbox = new QVBoxLayout();

	neValBtn = new QPushButton( tr("Not Equal") );
	neValBtn->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );

	hbox = new QHBoxLayout();
	useNeVal = new QCheckBox( tr("By:") );
	useNeVal->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	neValEntry = new QLineEdit();
	neValEntry->setMaxLength(2);
	neValEntry->setInputMask( ">HH;0" );
	neValEntry->setFont( font );
	neValEntry->setCursorPosition(0);
	neValEntry->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	neValEntry->setMaximumWidth( 3 * neValEntry->fontMetrics().boundingRect('0').width() );

	hbox->addWidget( useNeVal, 0, Qt::AlignRight );
	hbox->addWidget( neValEntry, 0, Qt::AlignLeft );

	vbox->addWidget( neValBtn );
	vbox->addLayout( hbox );
	frame->setLayout( vbox );

	frame = new QFrame();
	frame->setFrameShape( QFrame::Box );
	frame->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	vbox3->addWidget( frame );
	vbox = new QVBoxLayout();

	grValBtn = new QPushButton( tr("Greater Than") );
	grValBtn->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );

	hbox = new QHBoxLayout();
	useGrVal = new QCheckBox( tr("By:") );
	useGrVal->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	grValEntry = new QLineEdit();
	grValEntry->setMaxLength(2);
	grValEntry->setInputMask( ">HH;0" );
	grValEntry->setFont( font );
	grValEntry->setCursorPosition(0);
	grValEntry->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	grValEntry->setMaximumWidth( 3 * grValEntry->fontMetrics().boundingRect('0').width() );

	hbox->addWidget( useGrVal, 0, Qt::AlignRight );
	hbox->addWidget( grValEntry, 0, Qt::AlignLeft );

	vbox->addWidget( grValBtn );
	vbox->addLayout( hbox );
	frame->setLayout( vbox );

	frame = new QFrame();
	frame->setFrameShape( QFrame::Box );
	frame->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	vbox3->addWidget( frame );
	vbox = new QVBoxLayout();

	ltValBtn = new QPushButton( tr("Less Than") );
	ltValBtn->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );

	hbox = new QHBoxLayout();
	useLtVal = new QCheckBox( tr("By:") );
	useLtVal->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	ltValEntry = new QLineEdit();
	ltValEntry->setMaxLength(2);
	ltValEntry->setInputMask( ">HH;0" );
	ltValEntry->setFont( font );
	ltValEntry->setCursorPosition(0);
	ltValEntry->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	ltValEntry->setMaximumWidth( 3 * ltValEntry->fontMetrics().boundingRect('0').width() );

	hbox->addWidget( useLtVal, 0, Qt::AlignRight );
	hbox->addWidget( ltValEntry, 0, Qt::AlignLeft );

	vbox->addWidget( ltValBtn );
	vbox->addLayout( hbox );
	frame->setLayout( vbox );

	groupBox->setLayout( vbox3 );

	mainLayout->addWidget( cheatSearchFrame );

	setLayout( mainLayout );

	connect( srchResetBtn, SIGNAL(clicked(void)), this, SLOT(resetSearchCallback(void)) );
	connect( knownValBtn , SIGNAL(clicked(void)), this, SLOT(knownValueCallback(void)) );

	cheat_search_known_value = 0;

}
//----------------------------------------------------------------------------
GuiCheatsDialog_t::~GuiCheatsDialog_t(void)
{

}
//----------------------------------------------------------------------------
void GuiCheatsDialog_t::closeWindow(void)
{
   //printf("Close Window\n");
   done(0);
}
//----------------------------------------------------------------------------
int GuiCheatsDialog_t::addSearchResult (uint32_t a, uint8_t last, uint8_t current)
{
	QTreeWidgetItem *item;
	char addrStr[8], lastStr[8], curStr[8];

	item = new QTreeWidgetItem();

	sprintf (addrStr, "$%04X", a);
	sprintf (lastStr, "%02X", last);
	sprintf (curStr , "%02X", current);

	item->setText( 0, tr(addrStr) );
	item->setText( 1, tr(lastStr) );
	item->setText( 2, tr(curStr)  );

	item->setTextAlignment( 0, Qt::AlignCenter);
	item->setTextAlignment( 1, Qt::AlignCenter);
	item->setTextAlignment( 2, Qt::AlignCenter);

	srchResults->addTopLevelItem( item );

	return 1;
}
//----------------------------------------------------------------------------
static int ShowCheatSearchResultsCallB (uint32 a, uint8 last, uint8 current)
{
	return win->addSearchResult( a, last, current );
}
//----------------------------------------------------------------------------
void GuiCheatsDialog_t::showCheatSearchResults(void)
{
	int total_matches;

	win = this;

	srchResults->clear();

	total_matches = FCEUI_CheatSearchGetCount ();

	FCEUI_CheatSearchGetRange (0, total_matches,
				   ShowCheatSearchResultsCallB);

	printf("Num Matches: %i \n", total_matches );
}
//----------------------------------------------------------------------------
void GuiCheatsDialog_t::resetSearchCallback(void)
{
	fceuWrapperLock();

	FCEUI_CheatSearchBegin ();

	showCheatSearchResults();

	fceuWrapperUnLock();
}
//----------------------------------------------------------------------------
void GuiCheatsDialog_t::knownValueCallback(void)
{
	//printf("Cheat Search Known!\n");
	fceuWrapperLock();

	//printf("%s\n", knownValEntry->text().toStdString().c_str() );

	cheat_search_known_value = strtol( knownValEntry->text().toStdString().c_str(), NULL, 16 );

	FCEUI_CheatSearchEnd (FCEU_SEARCH_NEWVAL_KNOWN,
			      cheat_search_known_value, 0);

	showCheatSearchResults();

	fceuWrapperUnLock();
}
//----------------------------------------------------------------------------
