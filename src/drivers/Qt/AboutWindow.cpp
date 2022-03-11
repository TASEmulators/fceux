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
#include <unzip.h>

#ifdef _S9XLUA_H
#include <lua.h>
#endif

#ifdef _USE_X264
#include "x264.h"
#endif

#ifdef _USE_LIBAV
#ifdef __cplusplus
extern "C"
{
#include "libavutil/version.h"
#include "libavformat/version.h"
#include "libavcodec/version.h"
#include "libswscale/version.h"
#include "libswresample/version.h"
}
#endif
#endif


#include <QPixmap>
#include <QUrl>
#include <QTextEdit>
#include <QDesktopServices>
#include "Qt/sdl.h"
#include "Qt/AboutWindow.h"
#include "Qt/fceux_git_info.h"
#include "../../version.h"
#include "../../fceu.h"

static const char *Authors[] = {
	"Linux/SDL Developers:",
	"\t mjbudd77",
	"\t Lukas Sabota //punkrockguy318", "\t Soules", "\t Bryan Cain", "\t radsaq",
		"\t Shinydoofy",
	"\nQt GUI written by mjbudd77\n",
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
	QHBoxLayout *hbox1, *hbox;
	QPixmap pm(":fceux1.png");
	QPixmap pm2;
	QLabel *lbl;
	QTextEdit *credits;
	QPushButton *closeButton;
	char stmp[256];

	pm2 = pm.scaled( 128, 128 );

	setWindowTitle( tr("About fceuX") );

	resize( 512, 600 );

	mainLayout = new QVBoxLayout();

	hbox1 = new QHBoxLayout();
	lbl = new QLabel();
	lbl->setPixmap(pm2);

	hbox1->addWidget( lbl );
	hbox1->setAlignment( Qt::AlignCenter );

	mainLayout->addLayout( hbox1 );

	//hbox1 = new QHBoxLayout();
	//lbl = new QLabel( tr("fceuX") );

	//hbox1->addWidget( lbl );
	//hbox1->setAlignment( Qt::AlignCenter );

	//mainLayout->addLayout( hbox1 );

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

	credits->insertPlainText( "\nOpen Source Dependencies:\n" );

	sprintf( stmp, "	Compiled with Qt version %d.%d.%d\n", QT_VERSION_MAJOR, QT_VERSION_MINOR, QT_VERSION_PATCH );
	credits->insertPlainText( stmp );

	sprintf( stmp, "	Compiled with SDL version %d.%d.%d\n", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL );
	credits->insertPlainText( stmp );

	SDL_version v; 
	SDL_GetVersion(&v);
	sprintf( stmp, "	Linked with SDL version %d.%d.%d\n", v.major, v.minor, v.patch);
	credits->insertPlainText( stmp );

#ifdef ZLIB_VERSION
	sprintf( stmp, "	Compiled with zlib %s\n", ZLIB_VERSION );
	credits->insertPlainText( stmp );
#endif

#ifdef _S9XLUA_H
	sprintf( stmp, "	Compiled with %s\n", LUA_RELEASE );
	credits->insertPlainText( stmp );
#endif

#ifdef _USE_LIBAV
	sprintf( stmp, "	Compiled with ffmpeg libraries:\n");
	credits->insertPlainText( stmp );
	sprintf( stmp, "		libavutil    %i.%i.%i\n", LIBAVUTIL_VERSION_MAJOR, LIBAVUTIL_VERSION_MINOR, LIBAVUTIL_VERSION_MICRO);
	credits->insertPlainText( stmp );
	sprintf( stmp, "		libavformat  %i.%i.%i\n", LIBAVFORMAT_VERSION_MAJOR, LIBAVFORMAT_VERSION_MINOR, LIBAVFORMAT_VERSION_MICRO);
	credits->insertPlainText( stmp );
	sprintf( stmp, "		libavcodec   %i.%i.%i\n", LIBAVCODEC_VERSION_MAJOR, LIBAVCODEC_VERSION_MINOR, LIBAVCODEC_VERSION_MICRO);
	credits->insertPlainText( stmp );
	sprintf( stmp, "		libswscale   %i.%i.%i\n", LIBSWSCALE_VERSION_MAJOR, LIBSWSCALE_VERSION_MINOR, LIBSWSCALE_VERSION_MICRO);
	credits->insertPlainText( stmp );
	sprintf( stmp, "		libswresample  %i.%i.%i\n", LIBSWRESAMPLE_VERSION_MAJOR, LIBSWRESAMPLE_VERSION_MINOR, LIBSWRESAMPLE_VERSION_MICRO);
	credits->insertPlainText( stmp );
#endif

#ifdef _USE_X264
	sprintf( stmp, "	Compiled with x264 version %s\n", X264_POINTVER );
	credits->insertPlainText( stmp );
#endif

	credits->moveCursor(QTextCursor::Start);
	credits->setReadOnly(true);

	mainLayout->addWidget( credits );

	closeButton = new QPushButton( tr("OK") );
	closeButton->setIcon(style()->standardIcon(QStyle::SP_DialogOkButton));
	connect(closeButton, SIGNAL(clicked(void)), this, SLOT(closeWindow(void)));

	hbox = new QHBoxLayout();
	hbox->addStretch(5);
	hbox->addWidget( closeButton, 1 );
	mainLayout->addLayout( hbox );

	setLayout( mainLayout );

	closeButton->setFocus();
	closeButton->setDefault(true);
}
//----------------------------------------------------------------------------
AboutWindow::~AboutWindow(void)
{

}
//----------------------------------------------------------------------------
void AboutWindow::closeEvent(QCloseEvent *event)
{
	printf("About Window Close Event\n");
	done(0);
	deleteLater();
	event->accept();
}
//----------------------------------------------------------------------------
void AboutWindow::closeWindow(void)
{
	//printf("Close Window\n");
	done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
