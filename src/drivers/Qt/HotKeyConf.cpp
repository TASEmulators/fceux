// HotKeyConf.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <SDL.h>
#include <QHeaderView>
#include <QCloseEvent>

#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/fceuWrapper.h"
#include "Qt/HotKeyConf.h"

//----------------------------------------------------------------------------
HotKeyConfDialog_t::HotKeyConfDialog_t(QWidget *parent)
	: QDialog( parent )
{
	QVBoxLayout *mainLayout;
	QTreeWidgetItem *item;
	std::string prefix = "SDL.Hotkeys.";
	int keycode;

	setWindowTitle("Hotkey Configuration");

	resize( 512, 512 );

	mainLayout = new QVBoxLayout();

	tree = new QTreeWidget();

	tree->setColumnCount(2);

	item = new QTreeWidgetItem();
	item->setText( 0, QString::fromStdString( "Command" ) );
	item->setText( 1, QString::fromStdString( "Key" ) );
	item->setTextAlignment( 0, Qt::AlignLeft);
	item->setTextAlignment( 1, Qt::AlignCenter);

	tree->setHeaderItem( item );

	tree->header()->setSectionResizeMode( QHeaderView::ResizeToContents );

	for (int i=0; i<HK_MAX; i++)
	{
		std::string optionName = prefix + getHotkeyString(i);

		g_config->getOption (optionName.c_str (), &keycode);

		item = new QTreeWidgetItem();

		item->setText( 0, QString::fromStdString( optionName ) );
		item->setText( 1, QString::fromStdString( SDL_GetKeyName (keycode) ) );

		item->setTextAlignment( 0, Qt::AlignLeft);
		item->setTextAlignment( 1, Qt::AlignCenter);

		tree->addTopLevelItem( item );
	}
	mainLayout->addWidget( tree );

	setLayout( mainLayout );
}
//----------------------------------------------------------------------------
HotKeyConfDialog_t::~HotKeyConfDialog_t(void)
{
	printf("Destroy Hot Key Config Window\n");
}
//----------------------------------------------------------------------------
void HotKeyConfDialog_t::closeEvent(QCloseEvent *event)
{
   printf("Hot Key Close Window Event\n");
   done(0);
	deleteLater();
   event->accept();
}
//----------------------------------------------------------------------------
void HotKeyConfDialog_t::closeWindow(void)
{
   //printf("Close Window\n");
   done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
void HotKeyConfDialog_t::assignHotkey(QKeyEvent *event)
{
	SDL_Keycode k = convQtKey2SDLKeyCode( (Qt::Key)event->key() );
	
	if ( k != SDLK_UNKNOWN )
	{
		QList <QTreeWidgetItem *> l;
			
		l = tree->selectedItems();

		for (size_t i=0; i < l.size(); i++)
		{
			//int idx;
			QString qs;
			QTreeWidgetItem *item;

			item = l.at(i);

			//idx = tree->indexOfTopLevelItem( item );

			qs = item->text(0);

			g_config->setOption ( qs.toStdString(), k );

			setHotKeys();

			item->setText( 1, QString::fromStdString( SDL_GetKeyName (k) ) );

   		//printf("Hotkey Window Key Press: 0x%x  item:%p\n  '%s' : %i\n", 
			//		k, item, qs.toStdString().c_str(), idx );
		}
	}
}
//----------------------------------------------------------------------------
void HotKeyConfDialog_t::keyPressEvent(QKeyEvent *event)
{
   //printf("Hotkey Window Key Press: 0x%x \n", event->key() );
	assignHotkey( event );
}
//----------------------------------------------------------------------------
void HotKeyConfDialog_t::keyReleaseEvent(QKeyEvent *event)
{
   //printf("Hotkey Window Key Release: 0x%x \n", event->key() );
	assignHotkey( event );
}
//----------------------------------------------------------------------------
