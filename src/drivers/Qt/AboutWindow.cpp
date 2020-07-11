// AboutWindow.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <QPixmap>
#include <QUrl>
#include <QDesktopServices>
#include "Qt/icon.xpm"
#include "Qt/AboutWindow.h"
#include "../../version.h"

//----------------------------------------------------------------------------
AboutWindow::AboutWindow(QWidget *parent)
	: QDialog( parent )
{
	QVBoxLayout *mainLayout;
	QHBoxLayout *hbox1;
	QPixmap pm( icon_xpm );
	QLabel *lbl;

	setWindowTitle( tr("About fceuX") );

	resize( 512, 512 );

	mainLayout = new QVBoxLayout();

	hbox1 = new QHBoxLayout();
	lbl = new QLabel();
	lbl->setPixmap(pm);

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
	lbl = new QLabel( tr("© 2016 FceuX Development Team") );

	hbox1->addWidget( lbl );
	hbox1->setAlignment( Qt::AlignCenter );

	mainLayout->addLayout( hbox1 );

	setLayout( mainLayout );
}
//----------------------------------------------------------------------------
AboutWindow::~AboutWindow(void)
{

}
//----------------------------------------------------------------------------
