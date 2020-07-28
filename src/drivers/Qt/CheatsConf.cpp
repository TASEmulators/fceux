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
	QHBoxLayout *mainLayout, *hbox;
	QVBoxLayout *vbox;
	QTreeWidgetItem *item;
	QLabel *lbl;

	setWindowTitle("Cheat Search");

	//resize( 512, 512 );

	mainLayout = new QHBoxLayout();

	actCheatFrame = new QGroupBox( tr("Active Cheats") );

	vbox = new QVBoxLayout();

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
	
	vbox->addWidget( tree );

	hbox = new QHBoxLayout();

	lbl = new QLabel( tr("Name:") );
	cheatNameEntry = new QLineEdit();

	hbox->addWidget( lbl );
	hbox->addWidget( cheatNameEntry );

	vbox->addLayout( hbox );

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

	vbox->addLayout( hbox );

	hbox = new QHBoxLayout();

	addCheatBtn = new QPushButton( tr("Add") );
	delCheatBtn = new QPushButton( tr("Delete") );
	modCheatBtn = new QPushButton( tr("Update") );

	hbox->addWidget( addCheatBtn );
	hbox->addWidget( delCheatBtn );
	hbox->addWidget( modCheatBtn );

	vbox->addLayout( hbox );

	hbox = new QHBoxLayout();

	importCheatFileBtn = new QPushButton( tr("Import") );
	exportCheatFileBtn = new QPushButton( tr("Export") );

	hbox->addWidget( importCheatFileBtn );
	hbox->addWidget( exportCheatFileBtn );

	vbox->addLayout( hbox );

	actCheatFrame->setLayout( vbox );

	cheatSearchFrame = new QGroupBox( tr("Cheat Search") );
	cheatResultFrame = new QGroupBox( tr("Possibilities") );

	hbox = new QHBoxLayout();

	vbox = new QVBoxLayout();

	hbox->addLayout( vbox );
	hbox->addWidget( cheatResultFrame );

	cheatSearchFrame->setLayout( hbox );

	srchResetBtn = new QPushButton( tr("Reset") );
	knownValBtn  = new QPushButton( tr("Known Value:") );

	vbox->addWidget( srchResetBtn );
	vbox->addWidget( knownValBtn  );

	hbox = new QHBoxLayout();
	vbox->addLayout( hbox );

	lbl = new QLabel( tr("0x") );
	knownValEntry = new QLineEdit();
	hbox->addWidget( lbl );
	hbox->addWidget( knownValEntry );

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
