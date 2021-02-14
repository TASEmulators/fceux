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
// AboutWindow.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <QPixmap>
#include <QUrl>
#include <QTextEdit>
#include <QDesktopServices>
#include "Qt/AboutWindow.h"
#include "Qt/fceux_git_info.h"
#include "../../version.h"
#include "../../fceu.h"

static const char *Authors[] = {
	"Linux/SDL Developers:",
	"\t mjbudd77",
	"\t Lukas Sabota //punkrockguy318", "\t Soules", "\t Bryan Cain", "\t radsaq",
		"\t Shinydoofy",
	"FceuX 2.0 Developers:",
	"\t SP", "\t zeromus", "\t adelikat", "\t caH4e3", "\t qfox",
	"\t Luke Gustafson", "\t _mz", "\t UncombedCoconut", "\t DwEdit", "\t AnS",
		"\t rainwarrior", "\t feos",
	"Pre 2.0 Guys:",
	"\t Bero", "\t Xodnizel", "\t Aaron Oneal", "\t Joe Nahmias",
	"\t Paul Kuliniewicz", "\t Quietust", "\t Ben Parnell",
		"\t Parasyte &amp; bbitmaster",
	"\t blip & nitsuja",
	"Included components:",
	"\t Mitsutaka Okazaki - YM2413 emulator",
		"\t Andrea Mazzoleni - Scale2x/Scale3x scalers",
		"\t Gilles Vollant - unzip.c PKZIP fileio",
	NULL
};
//----------------------------------------------------------------------------
AboutWindow::AboutWindow(QWidget *parent)
	: QDialog( parent )
{
	int i;
	QVBoxLayout *mainLayout;
	QHBoxLayout *hbox1;
	QPixmap pm(":fceux1.png");
	QPixmap pm2;
	QLabel *lbl;
	QTextEdit *credits;
	char stmp[256];

	pm2 = pm.scaled( 64, 64 );

	setWindowTitle( tr("About fceuX") );

	resize( 512, 512 );

	mainLayout = new QVBoxLayout();

	hbox1 = new QHBoxLayout();
	lbl = new QLabel();
	lbl->setPixmap(pm2);

	hbox1->addWidget( lbl );
	hbox1->setAlignment( Qt::AlignCenter );

	mainLayout->addLayout( hbox1 );

	hbox1 = new QHBoxLayout();
	lbl = new QLabel( tr("fceuX") );

	hbox1->addWidget( lbl );
	hbox1->setAlignment( Qt::AlignCenter );

	mainLayout->addLayout( hbox1 );

	hbox1 = new QHBoxLayout();
	lbl = new QLabel( tr(FCEU_VERSION_STRING) );

	hbox1->addWidget( lbl );
	hbox1->setAlignment( Qt::AlignCenter );

	mainLayout->addLayout( hbox1 );

	sprintf( stmp, "git URL: %s", fceu_get_git_url() );

	hbox1 = new QHBoxLayout();
	lbl = new QLabel( tr(stmp) );

	hbox1->addWidget( lbl );
	hbox1->setAlignment( Qt::AlignCenter );

	mainLayout->addLayout( hbox1 );

	sprintf( stmp, "git Revision: %s", fceu_get_git_rev() );

	hbox1 = new QHBoxLayout();
	lbl = new QLabel( tr(stmp) );

	hbox1->addWidget( lbl );
	hbox1->setAlignment( Qt::AlignCenter );

	mainLayout->addLayout( hbox1 );

	hbox1 = new QHBoxLayout();
	lbl = new QLabel();
	lbl->setText("<a href=\"http://fceux.com\">Website</a>");
	lbl->setTextInteractionFlags(Qt::TextBrowserInteraction);
	lbl->setOpenExternalLinks(true);

	hbox1->addWidget( lbl );
	hbox1->setAlignment( Qt::AlignCenter );

	mainLayout->addLayout( hbox1 );

	hbox1 = new QHBoxLayout();
	lbl = new QLabel( tr("License: GPL") );

	hbox1->addWidget( lbl );
	hbox1->setAlignment( Qt::AlignCenter );

	mainLayout->addLayout( hbox1 );

	hbox1 = new QHBoxLayout();
	lbl = new QLabel( tr("© 2020 FceuX Development Team") );

	hbox1->addWidget( lbl );
	hbox1->setAlignment( Qt::AlignCenter );

	mainLayout->addLayout( hbox1 );

	credits = new QTextEdit();

	credits->insertPlainText( FCEUI_GetAboutString() );
	credits->insertPlainText( "\n\n");

	i=0;
	while ( Authors[i] != NULL )
	{
		credits->insertPlainText( Authors[i] ); i++;
		credits->insertPlainText( "\n");
	}
	credits->moveCursor(QTextCursor::Start);
	credits->setReadOnly(true);

	mainLayout->addWidget( credits );

	setLayout( mainLayout );
}
//----------------------------------------------------------------------------
AboutWindow::~AboutWindow(void)
{

}
//----------------------------------------------------------------------------
