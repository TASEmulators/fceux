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
