// ConsoleVideoConf.cpp
//
#include "Qt/config.h"
#include "Qt/fceuWrapper.h"
#include "Qt/ConsoleVideoConf.h"


//----------------------------------------------------
ConsoleVideoConfDialog_t::ConsoleVideoConfDialog_t(QWidget *parent)
	: QDialog( parent )
{
	QVBoxLayout *main_vbox;
	QHBoxLayout *hbox1;
	QLabel *lbl;

	setWindowTitle( tr("Video Config") );

	main_vbox = new QVBoxLayout();

	// Video Driver Select
	lbl = new QLabel( tr("Driver:") );

	drvSel = new QComboBox();

	drvSel->addItem( tr("OpenGL"), 0 );
	//drvSel->addItem( tr("SDL"), 1 );
	
	hbox1 = new QHBoxLayout();

	hbox1->addWidget( lbl );
	hbox1->addWidget( drvSel );

	main_vbox->addLayout( hbox1 );

	// Enable OpenGL Linear Filter
	gl_LF_chkBox  = new QCheckBox( tr("Enable OpenGL Linear Filter") );

	main_vbox->addWidget( gl_LF_chkBox );

	// Region Select
	lbl = new QLabel( tr("Region:") );

	regionSelect = new QComboBox();

	regionSelect->addItem( tr("NTSC") , 0 );
	regionSelect->addItem( tr("PAL")  , 1 );
	regionSelect->addItem( tr("Dendy"), 2 );

	hbox1 = new QHBoxLayout();

	hbox1->addWidget( lbl );
	hbox1->addWidget( regionSelect );

	main_vbox->addLayout( hbox1 );

	setLayout( main_vbox );

}
//----------------------------------------------------
ConsoleVideoConfDialog_t::~ConsoleVideoConfDialog_t(void)
{

}
//----------------------------------------------------
