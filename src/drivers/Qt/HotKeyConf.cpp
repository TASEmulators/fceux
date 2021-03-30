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
	: QDialog(parent)
{
	QVBoxLayout *mainLayout;
	QHBoxLayout *hbox;
	QPushButton *closeButton;
	QTreeWidgetItem *item;
	std::string prefix = "SDL.Hotkeys.";

	setWindowTitle("Hotkey Configuration");

	resize(512, 512);

	mainLayout = new QVBoxLayout();

	tree = new HotKeyConfTree_t(this);

	tree->setColumnCount(2);
	tree->setSelectionMode( QAbstractItemView::SingleSelection );

	item = new QTreeWidgetItem();
	item->setText(0, QString::fromStdString("Command"));
	item->setText(1, QString::fromStdString("Key"));
	item->setTextAlignment(0, Qt::AlignLeft);
	item->setTextAlignment(1, Qt::AlignCenter);

	tree->setHeaderItem(item);

	tree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

	for (int i = 0; i < HK_MAX; i++)
	{
		char keyName[128];
		std::string optionName = prefix + Hotkeys[i].getConfigName();

		//g_config->getOption (optionName.c_str (), &keycode);
		Hotkeys[i].getString(keyName);

		item = new QTreeWidgetItem();

		tree->addTopLevelItem(item);

		item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemNeverHasChildren );
		item->setCheckState( 0, Qt::Checked );

		item->setText(0, QString::fromStdString(optionName));
		item->setText(1, QString::fromStdString(keyName));

		item->setTextAlignment(0, Qt::AlignLeft);
		item->setTextAlignment(1, Qt::AlignCenter);

	}
	mainLayout->addWidget(tree);

	closeButton = new QPushButton( tr("Close") );
	closeButton->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
	connect(closeButton, SIGNAL(clicked(void)), this, SLOT(closeWindow(void)));

	hbox = new QHBoxLayout();
	hbox->addStretch(5);
	hbox->addWidget( closeButton, 1 );
	mainLayout->addLayout( hbox );

	setLayout(mainLayout);
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
	bool keyIsModifier;
	QKeySequence ks( event->modifiers() + event->key() );
	SDL_Keycode k = convQtKey2SDLKeyCode((Qt::Key)event->key());
	SDL_Keymod m = convQtKey2SDLModifier(event->modifiers());

	keyIsModifier = (k == SDLK_LCTRL) || (k == SDLK_RCTRL) ||
					(k == SDLK_LSHIFT) || (k == SDLK_RSHIFT) ||
					(k == SDLK_LALT) || (k == SDLK_RALT) ||
					(k == SDLK_LGUI) || (k == SDLK_RGUI) ||
					(k == SDLK_CAPSLOCK);

	//printf("Assign: '%s' %i  0x%08x\n", ks.toString().toStdString().c_str(), event->key(), event->key() );

	if ((k != SDLK_UNKNOWN) && !keyIsModifier)
	{
		QList<QTreeWidgetItem *> l;

		l = tree->selectedItems();

		for (size_t i = 0; i < l.size(); i++)
		{
			int j, idx;
			QString qs;
			QTreeWidgetItem *item;
			std::string keyText;
			char keyName[128];
			char buf[256];

			keyText = ks.toString().toStdString();

			//j = 0;
			//if (m & (KMOD_LSHIFT | KMOD_RSHIFT))
			//{
			//	if (j > 0)
			//	{
			//		keyText.append("+");
			//	}
			//	keyText.append("Shift");
			//	j++;
			//}
			//if (m & (KMOD_LALT | KMOD_RALT))
			//{
			//	if (j > 0)
			//	{
			//		keyText.append("+");
			//	}
			//	keyText.append("Alt");
			//	j++;
			//}
			//if (m & (KMOD_LCTRL | KMOD_RCTRL))
			//{
			//	if (j > 0)
			//	{
			//		keyText.append("+");
			//	}
			//	keyText.append("Ctrl");
			//	j++;
			//}

			//sprintf(buf, "  key=%s", SDL_GetKeyName(k));

			//keyText.append(buf);

			item = l.at(i);

			idx = tree->indexOfTopLevelItem(item);

			qs = item->text(0);

			g_config->setOption(qs.toStdString(), keyText);

			setHotKeys();

			Hotkeys[idx].getString(keyName);

			item->setText(1, QString::fromStdString(keyName));

			//printf("Hotkey Window Key Press: 0x%x  item:%p\n  '%s' : %i\n",
			//		k, item, qs.toStdString().c_str(), idx );
		}
	}
}
//----------------------------------------------------------------------------
void HotKeyConfDialog_t::keyPressEvent(QKeyEvent *event)
{
	//printf("Hotkey Window Key Press: 0x%x \n", event->key() );
	assignHotkey(event);
	event->accept();
}
//----------------------------------------------------------------------------
void HotKeyConfDialog_t::keyReleaseEvent(QKeyEvent *event)
{
	//printf("Hotkey Window Key Release: 0x%x \n", event->key() );
	assignHotkey(event);
	event->accept();
}
//----------------------------------------------------------------------------
HotKeyConfTree_t::HotKeyConfTree_t(QWidget *parent)
	: QTreeWidget(parent)
{

}
//----------------------------------------------------------------------------
HotKeyConfTree_t::~HotKeyConfTree_t(void)
{

}
//----------------------------------------------------------------------------
void HotKeyConfTree_t::keyPressEvent(QKeyEvent *event)
{
	if ( parent() )
	{
		static_cast<HotKeyConfDialog_t*>(parent())->assignHotkey(event);
	}
	event->accept();
}
//----------------------------------------------------------------------------
void HotKeyConfTree_t::keyReleaseEvent(QKeyEvent *event)
{
	if ( parent() )
	{
		static_cast<HotKeyConfDialog_t*>(parent())->assignHotkey(event);
	}
	event->accept();
}
//----------------------------------------------------------------------------
