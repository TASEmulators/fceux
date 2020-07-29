// HotKeyConf.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <SDL.h>
#include <QHeaderView>

#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/fceuWrapper.h"
#include "Qt/CheatsConf.h"

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

	hbox->addWidget( lbl );
	hbox->addWidget( cheatNameEntry );

	vbox1->addLayout( hbox );

	hbox = new QHBoxLayout();

	lbl = new QLabel( tr("Address:") );
	cheatAddrEntry = new QLineEdit();

	hbox->addWidget( lbl );
	hbox->addWidget( cheatAddrEntry );

	lbl = new QLabel( tr("Value:") );
	cheatValEntry = new QLineEdit();

	hbox->addWidget( lbl );
	hbox->addWidget( cheatValEntry );

	lbl = new QLabel( tr("Compare:") );
	cheatCmpEntry = new QLineEdit();

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

	hbox->addWidget( importCheatFileBtn );
	hbox->addWidget( exportCheatFileBtn );

	vbox1->addLayout( hbox );

	actCheatFrame->setLayout( vbox1 );

	cheatSearchFrame = new QGroupBox( tr("Cheat Search") );
	cheatResultFrame = new QGroupBox( tr("Possibilities") );

	srchResults = new QTreeWidget();
	srchResults->setColumnCount(3);

	item = new QTreeWidgetItem();
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

	vbox2->addWidget( srchResetBtn );

	frame = new QFrame();
	frame->setFrameShape( QFrame::Box );
	vbox2->addWidget( frame );
	vbox = new QVBoxLayout();

	frame->setLayout( vbox );

	knownValBtn  = new QPushButton( tr("Known Value:") );

	vbox->addWidget( knownValBtn  );

	hbox1 = new QHBoxLayout();
	vbox->addLayout( hbox1 );

	lbl = new QLabel( tr("0x") );
	knownValEntry = new QLineEdit();
	hbox1->addWidget( lbl );
	hbox1->addWidget( knownValEntry );

	groupBox = new QGroupBox( tr("Previous Compare") );
	vbox2->addWidget( groupBox );

	vbox3 = new QVBoxLayout();

	frame = new QFrame();
	frame->setFrameShape( QFrame::Box );
	vbox3->addWidget( frame );
	vbox = new QVBoxLayout();

	eqValBtn = new QPushButton( tr("Equal") );
	vbox->addWidget( eqValBtn );

	frame->setLayout( vbox );

	frame = new QFrame();
	frame->setFrameShape( QFrame::Box );
	vbox3->addWidget( frame );
	vbox = new QVBoxLayout();

	neValBtn = new QPushButton( tr("Not Equal") );

	hbox = new QHBoxLayout();
	useNeVal = new QCheckBox( tr("By:") );
	neValEntry = new QLineEdit();

	hbox->addWidget( useNeVal );
	hbox->addWidget( neValEntry );

	vbox->addWidget( neValBtn );
	vbox->addLayout( hbox );
	frame->setLayout( vbox );

	frame = new QFrame();
	frame->setFrameShape( QFrame::Box );
	vbox3->addWidget( frame );
	vbox = new QVBoxLayout();

	grValBtn = new QPushButton( tr("Greater Than") );

	hbox = new QHBoxLayout();
	useGrVal = new QCheckBox( tr("By:") );
	grValEntry = new QLineEdit();

	hbox->addWidget( useGrVal );
	hbox->addWidget( grValEntry );

	vbox->addWidget( grValBtn );
	vbox->addLayout( hbox );
	frame->setLayout( vbox );

	frame = new QFrame();
	frame->setFrameShape( QFrame::Box );
	vbox3->addWidget( frame );
	vbox = new QVBoxLayout();

	ltValBtn = new QPushButton( tr("Less Than") );

	hbox = new QHBoxLayout();
	useLtVal = new QCheckBox( tr("By:") );
	ltValEntry = new QLineEdit();

	hbox->addWidget( useLtVal );
	hbox->addWidget( ltValEntry );

	vbox->addWidget( ltValBtn );
	vbox->addLayout( hbox );
	frame->setLayout( vbox );

	groupBox->setLayout( vbox3 );

	mainLayout->addWidget( cheatSearchFrame );

	setLayout( mainLayout );
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
