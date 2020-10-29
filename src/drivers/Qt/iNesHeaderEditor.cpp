// HotKeyConf.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <SDL.h>
#include <QHeaderView>
#include <QCloseEvent>

#include "../../types.h"
#include "../../fceu.h"
#include "../../cart.h"
#include "../../ines.h"

#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/fceuWrapper.h"
#include "Qt/iNesHeaderEditor.h"

extern BMAPPINGLocal bmap[];

//----------------------------------------------------------------------------
iNesHeaderEditor_t::iNesHeaderEditor_t(QWidget *parent)
	: QDialog( parent )
{
	QVBoxLayout *mainLayout, *hdrLayout;
	QVBoxLayout *vbox, *vbox1, *vbox2, *vbox3, *vbox4, *vbox5;
	QHBoxLayout *hbox, *hbox1, *hbox2, *hbox3;
	QGroupBox *box, *hdrBox;
	char stmp[128];

	setWindowTitle("iNES Header Editor");

	//resize( 512, 512 );

	mainLayout = new QVBoxLayout();
	hdrLayout  = new QVBoxLayout();
	hbox1      = new QHBoxLayout();
	hbox       = new QHBoxLayout();
	hdrBox     = new QGroupBox( tr("iNES Header") );
	box        = new QGroupBox( tr("Version:") );

	mainLayout->addWidget( hdrBox );
	hdrBox->setLayout( hdrLayout );
	hbox1->addWidget( box );
	box->setLayout( hbox );

	iNes1Btn = new QRadioButton( tr("iNES") );
	iNes2Btn = new QRadioButton( tr("NES 2.0") );

	hbox->addWidget( iNes1Btn );
	hbox->addWidget( iNes2Btn );

	hbox       = new QHBoxLayout();
	box        = new QGroupBox( tr("Mapper:") );

	hbox1->addWidget( box );
	box->setLayout( hbox );

	mapperComboBox = new QComboBox();
	mapperSubEdit  = new QLineEdit();

	hbox->addWidget( new QLabel( tr("Mapper #:") ) );
	hbox->addWidget( mapperComboBox );
	hbox->addWidget( new QLabel( tr("Sub #:") ) );
	hbox->addWidget( mapperSubEdit );

	for (int i = 0; bmap[i].init; ++i)
	{
		sprintf(stmp, "%d %s", bmap[i].number, bmap[i].name);
		
		mapperComboBox->addItem( tr(stmp), i );
	}
	hdrLayout->addLayout( hbox1 );

	box   = new QGroupBox( tr("PRG") );
	hbox  = new QHBoxLayout();

	box->setLayout( hbox );
	hdrLayout->addWidget( box );

	prgRomBox   = new QComboBox();
	prgRamBox   = new QComboBox();
	prgNvRamBox = new QComboBox();

	hbox->addWidget( new QLabel( tr("PRG ROM:") ), 0, Qt::AlignRight );
	hbox->addWidget( prgRomBox );
	hbox->addWidget( new QLabel( tr("PRG RAM:") ), 0, Qt::AlignRight );
	hbox->addWidget( prgRamBox );
	hbox->addWidget( new QLabel( tr("PRG NVRAM:") ), 0, Qt::AlignRight );
	hbox->addWidget( prgNvRamBox );

	box   = new QGroupBox( tr("CHR") );
	hbox  = new QHBoxLayout();

	box->setLayout( hbox );
	hdrLayout->addWidget( box );

	chrRomBox   = new QComboBox();
	chrRamBox   = new QComboBox();
	chrNvRamBox = new QComboBox();

	hbox->addWidget( new QLabel( tr("CHR ROM:") ), 0, Qt::AlignRight );
	hbox->addWidget( chrRomBox );
	hbox->addWidget( new QLabel( tr("CHR RAM:") ), 0, Qt::AlignRight );
	hbox->addWidget( chrRamBox );
	hbox->addWidget( new QLabel( tr("CHR NVRAM:") ), 0, Qt::AlignRight );
	hbox->addWidget( chrNvRamBox );

	// add usually used size strings
	strcpy(stmp, "0 B");
	prgRomBox->addItem( tr(stmp), 0 );
	prgRamBox->addItem( tr(stmp), 0 );
	prgNvRamBox->addItem( tr(stmp), 0 );
	chrRomBox->addItem( tr(stmp), 0 );
	chrRamBox->addItem( tr(stmp), 0 );
	chrNvRamBox->addItem( tr(stmp), 0 );

	int size = 128;
	while (size <= 2048 * 1024)
	{
		if (size >= 1024 << 3)
		{
			// The size of CHR ROM must be multiple of 8KB
			sprintf( stmp, "%d KB", size / 1024);
			chrRomBox->addItem( tr(stmp), size );

			// The size of PRG ROM must be multiple of 16KB
			if (size >= 1024 * 16)
			{
				// PRG ROM
				sprintf(stmp, "%d KB", size / 1024);
				prgRomBox->addItem( tr(stmp), size );
			}
		}
		
		if (size >= 1024)
		{
			sprintf( stmp, "%d KB", size / 1024);
		}
		else
		{
			sprintf( stmp, "%d B", size);
		}

		prgRamBox->addItem( tr(stmp), size );
		chrRamBox->addItem( tr(stmp), size );
		prgNvRamBox->addItem( tr(stmp), size );
		chrNvRamBox->addItem( tr(stmp), size );

		size <<= 1;
	}

	hbox2 = new QHBoxLayout();
	hbox3 = new QHBoxLayout();
	vbox1 = new QVBoxLayout();
	vbox2 = new QVBoxLayout();
	vbox3 = new QVBoxLayout();
	vbox4 = new QVBoxLayout();
	vbox5 = new QVBoxLayout();
	vbox  = new QVBoxLayout();

	hdrLayout->addLayout( hbox2 );
	hbox2->addLayout( vbox1 );
	hbox3->addLayout( vbox5 );
	hbox3->addLayout( vbox2 );
	vbox1->addLayout( hbox3 );

	trainerCBox = new QCheckBox( tr("Trainer") );
	iNesUnOfBox = new QCheckBox( tr("iNES 1.0 Unofficial Properties") );

	box = new QGroupBox( tr("Mirroring:") );
	box->setLayout( vbox );
	vbox5->addWidget( box );
	vbox5->addWidget( trainerCBox );

	horzMirrorBtn = new QRadioButton( tr("Horizontal") );
	vertMirrorBtn = new QRadioButton( tr("Vertical") );
	fourMirrorBtn = new QRadioButton( tr("Four Screen") );

	vbox->addWidget( horzMirrorBtn );
	vbox->addWidget( vertMirrorBtn );
	vbox->addWidget( fourMirrorBtn );

	vbox = new QVBoxLayout();
	box  = new QGroupBox( tr("Region:") );
	box->setLayout( vbox );
	vbox2->addWidget( box );

	ntscRegionBtn  = new QRadioButton( tr("NTSC") );
	palRegionBtn   = new QRadioButton( tr("PAL") );
	dendyRegionBtn = new QRadioButton( tr("Dendy") );
	dualRegionBtn  = new QRadioButton( tr("Dual") );

	vbox->addWidget( ntscRegionBtn  );
	vbox->addWidget( palRegionBtn   );
	vbox->addWidget( dendyRegionBtn );
	vbox->addWidget( dualRegionBtn  );

	iNesDualRegBox = new QCheckBox( tr("Dual Region") );
	iNesBusCfltBox = new QCheckBox( tr("Bus Conflict") );
	iNesPrgRamBox  = new QCheckBox( tr("PRG RAM Exists") );

	box  = new QGroupBox( tr("iNES 1.0 Unofficial Properties:") );
	vbox = new QVBoxLayout();

	vbox->addWidget( iNesDualRegBox );
	vbox->addWidget( iNesBusCfltBox );
	vbox->addWidget( iNesPrgRamBox  );

	box->setLayout( vbox );
	vbox1->addWidget( iNesUnOfBox );
	vbox1->addWidget( box );

	hbox = new QHBoxLayout();
	box  = new QGroupBox( tr("System") );
	hbox2->addWidget( box );
	box->setLayout( vbox3 );
	vbox3->addLayout( hbox );

	normSysbtn  = new QRadioButton( tr("Normal") );
	vsSysbtn    = new QRadioButton( tr("VS. Sys") );
	plySysbtn   = new QRadioButton( tr("PlayChoice-10") );
	extSysbtn   = new QRadioButton( tr("Extend") );

	hbox->addWidget( normSysbtn );
	hbox->addWidget( vsSysbtn   );
	hbox->addWidget( plySysbtn  );
	hbox->addWidget( extSysbtn  );

	hbox = new QHBoxLayout();
	box  = new QGroupBox( tr("VS. System") );
	box->setLayout( vbox4 );
	vbox3->addWidget(  box );
	vbox4->addLayout( hbox );

	vsHwBox   = new QComboBox();
	vsPpuBox  = new QComboBox();
	extCslBox = new QComboBox();

	hbox->addWidget( new QLabel( tr("Hardware:") ) );
	hbox->addWidget( vsHwBox );

	hbox = new QHBoxLayout();
	vbox4->addLayout( hbox );

	hbox->addWidget( new QLabel( tr("PPU:") ) );
	hbox->addWidget( vsPpuBox );

	hbox = new QHBoxLayout();
	box  = new QGroupBox( tr("Extend System") );
	vbox3->addWidget( box );
	box->setLayout( hbox );

	hbox->addWidget( new QLabel( tr("Console:") ) );
	hbox->addWidget( extCslBox );

	hbox = new QHBoxLayout();
	inputDevBox  = new QComboBox();
	miscRomsEdit = new QLineEdit();
	hdrLayout->addLayout( hbox );
	hbox->addWidget( miscRomsEdit );
	hbox->addWidget( new QLabel( tr("Misc. ROM(s)") ) );
	hbox->addWidget( new QLabel( tr("Input Device:") ) );
	hbox->addWidget( inputDevBox );

	setLayout( mainLayout );
}
//----------------------------------------------------------------------------
iNesHeaderEditor_t::~iNesHeaderEditor_t(void)
{
	printf("Destroy Header Editor Config Window\n");
}
//----------------------------------------------------------------------------
void iNesHeaderEditor_t::closeEvent(QCloseEvent *event)
{
   printf("iNES Header Editor Close Window Event\n");
   done(0);
	deleteLater();
   event->accept();
}
//----------------------------------------------------------------------------
void iNesHeaderEditor_t::closeWindow(void)
{
   //printf("Close Window\n");
   done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
