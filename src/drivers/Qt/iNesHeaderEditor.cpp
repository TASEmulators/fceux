// HotKeyConf.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <SDL.h>
#include <QHeaderView>
#include <QCloseEvent>
#include <QMessageBox>
#include <QFileDialog>

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
#include "Qt/ConsoleUtilities.h"

extern BMAPPINGLocal bmap[];

// VS System type
static const char* vsSysList[] = {
	"Normal",
	"RBI Baseball",
	"TKO Boxing",
	"Super Xevious",
	"Ice Climber",
	"Dual Normal",
	"Dual Raid on Bungeling Bay",
	0
};

// VS PPU type
static const char* vsPPUList[] = {
	"RP2C03B",
	"RP2C03G",
	"RP2C04-0001",
	"RP2C04-0002",
	"RP2C04-0003",
	"RP2C04-0004",
	"RC2C03B",
	"RC2C03C",
	"RC2C05-01 ($2002 AND $\?\?=$1B)",
	"RC2C05-02 ($2002 AND $3F=$3D)",
	"RC2C05-03 ($2002 AND $1F=$1C)",
	"RC2C05-04 ($2002 AND $1F=$1B)",
	"RC2C05-05 ($2002 AND $1F=$\?\?)",
	"Reserved",
	"Reserved",
	"Reserved",
	0
};

// Extend console type
static const char* extConsoleList[] = {
	"Normal",
	"VS. System",
	"Playchoice 10",
	"Bit Corp. Creator",
	"V.R. Tech. VT01 monochrome",
	"V.R. Tech. VT01 red / cyan",
	"V.R. Tech. VT02",
	"V.R. Tech. VT03",
	"V.R. Tech. VT09",
	"V.R. Tech. VT32",
	"V.R. Tech. VT369",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	0
};

// Default input device list
static const char* inputDevList[] = {
	"Unspecified",
	"Standard NES / Famicom Controllers",
	"Four-score (NES)",
	"Four-score (Famicom)",
	"VS. System",
	"VS. System (controllers swapped)",
	"VS. Pinball (J)",
	"VS. Zapper",
	"Zapper",
	"Double Zappers",
	"Bandai Hyper Shot",
	"Power Pad Side A",
	"Power Pad Side B",
	"Family Trainer Side A",
	"Family Trainer Side B",
	"Arkanoid Paddle (NES)",
	"Arkanoid Paddle (Famicom)",
	"Double Arkanoid Paddle",
	"Konami Hyper Shot",
	"Pachinko",
	"Exciting Boxing Punching Bag",
	"Jissen Mahjong",
	"Party Tap",
	"Oeka Kids Tablet",
	"Barcode Reader",
	"Miracle Piano",
	"Pokkun Moguraa",
	"Top Rider",
	"Double-Fisted",
	"Famicom 3D",
	"Doremikko Keyboard",
	"R.O.B. Gyro Set",
	"Famicom Data Recorder",
	"ASCII Turbo File",
	"IGS Storage Battle Box",
	"Family BASIC Keyboard",
	"PEC-586 Keyboard",
	"Bit Corp. Keyboard",
	"Subor Keyboard",
	"Subor Keyboard + Mouse A",
	"Subor Keyboard + Mouse B",
	"SNES Mouse",
	"Multicart",
	"Double SNES controllers",
	"RacerMate Bicycle",
	"U-Force",
	"R.O.B. Stack-Up",
	0
};
//----------------------------------------------------------------------------
iNesHeaderEditor_t::iNesHeaderEditor_t(QWidget *parent)
	: QDialog( parent )
{
	int i;
	int fontCharWidth;
	fceuDecIntValidtor *validator; 
	QVBoxLayout *mainLayout, *hdrLayout;
	QVBoxLayout *vbox, *vbox1, *vbox2, *vbox3, *vbox4, *vbox5;
	QHBoxLayout *hbox, *hbox1, *hbox2, *hbox3;
	QGroupBox *box, *hdrBox;
	QGridLayout *grid;
	char stmp[128];

	initOK = false;

	font.setFamily("Courier New");
	font.setStyle( QFont::StyleNormal );
	font.setStyleHint( QFont::Monospace );
	QFontMetrics fm(font);

#if QT_VERSION > QT_VERSION_CHECK(5, 11, 0)
    fontCharWidth = fm.horizontalAdvance(QLatin1Char('2'));
#else
    fontCharWidth = fm.width(QLatin1Char('2'));
#endif

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

	connect( iNes1Btn, SIGNAL(clicked(bool)), this, SLOT(iNes1Clicked(bool)) );
	connect( iNes2Btn, SIGNAL(clicked(bool)), this, SLOT(iNes2Clicked(bool)) );

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
	hbox->addWidget( mapperSubLbl = new QLabel( tr("Sub #:") ) );
	hbox->addWidget( mapperSubEdit );

	validator = new fceuDecIntValidtor(0, 15, this);

	mapperSubEdit->setFont( font );
	mapperSubEdit->setMaxLength( 2 );
	mapperSubEdit->setValidator( validator );
	mapperSubEdit->setAlignment(Qt::AlignCenter);
	mapperSubEdit->setMaximumWidth( 4 * fontCharWidth );

	for (i = 0; bmap[i].init; ++i)
	{
		sprintf(stmp, "%d %s", bmap[i].number, bmap[i].name);
		
		mapperComboBox->addItem( tr(stmp), i );
	}
	hdrLayout->addLayout( hbox1 );

	box   = new QGroupBox( tr("PRG") );
	grid  = new QGridLayout();

	box->setLayout( grid );
	hdrLayout->addWidget( box );

	prgRomBox   = new QComboBox();
	prgRamBox   = new QComboBox();
	prgNvRamBox = new QComboBox();
	battNvRamBox= new QCheckBox( tr("Battery-backed NVRAM") );

	hbox  = new QHBoxLayout();
	grid->addLayout( hbox, 0, 0 );
	hbox->addWidget( new QLabel( tr("PRG ROM:") ), 0, Qt::AlignRight );
	hbox->addWidget( prgRomBox, 0, Qt::AlignLeft );
	hbox  = new QHBoxLayout();
	grid->addLayout( hbox, 0, 1 );
	hbox->addWidget( prgRamLbl = new QLabel( tr("PRG RAM:") ), 0, Qt::AlignRight );
	hbox->addWidget( prgRamBox, 0, Qt::AlignLeft );
	hbox  = new QHBoxLayout();
	grid->addLayout( hbox, 0, 2 );
	hbox->addWidget( prgNvRamLbl = new QLabel( tr("PRG NVRAM:") ), 0, Qt::AlignRight );
	hbox->addWidget( prgNvRamBox, 0, Qt::AlignLeft );
	hbox->addWidget( battNvRamBox, 0, Qt::AlignRight );

	box   = new QGroupBox( tr("CHR") );
	grid  = new QGridLayout();

	box->setLayout( grid );
	hdrLayout->addWidget( box );

	chrRomBox   = new QComboBox();
	chrRamBox   = new QComboBox();
	chrNvRamBox = new QComboBox();

	hbox  = new QHBoxLayout();
	grid->addLayout( hbox, 0, 0 );
	hbox->addWidget( new QLabel( tr("CHR ROM:") ), 0, Qt::AlignRight );
	hbox->addWidget( chrRomBox, 0, Qt::AlignLeft );
	hbox  = new QHBoxLayout();
	grid->addLayout( hbox, 0, 1 );
	hbox->addWidget( chrRamLbl = new QLabel( tr("CHR RAM:") ), 0, Qt::AlignRight );
	hbox->addWidget( chrRamBox, 0, Qt::AlignLeft );
	hbox  = new QHBoxLayout();
	grid->addLayout( hbox, 0, 2 );
	hbox->addWidget( chrNvRamLbl = new QLabel( tr("CHR NVRAM:") ), 0, Qt::AlignRight );
	hbox->addWidget( chrNvRamBox, 0, Qt::AlignLeft );

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

	connect( iNesUnOfBox, SIGNAL(stateChanged(int)), this, SLOT(unofficialStateChange(int)) );

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

	connect( iNesPrgRamBox , SIGNAL(stateChanged(int)), this, SLOT(unofficialPrgRamStateChange(int)) );
	connect( iNesDualRegBox, SIGNAL(stateChanged(int)), this, SLOT(unofficialDualRegionStateChange(int)) );

	iNesUnOfGroupBox = new QGroupBox( tr("iNES 1.0 Unofficial Properties:") );
	vbox = new QVBoxLayout();

	vbox->addWidget( iNesDualRegBox );
	vbox->addWidget( iNesBusCfltBox );
	vbox->addWidget( iNesPrgRamBox  );

	iNesUnOfGroupBox->setLayout( vbox );
	vbox1->addWidget( iNesUnOfBox );
	vbox1->addWidget( iNesUnOfGroupBox );

	hbox = new QHBoxLayout();
	sysGroupBox = new QGroupBox( tr("System") );
	hbox2->addWidget( sysGroupBox );
	sysGroupBox->setLayout( vbox3 );
	vbox3->addLayout( hbox );

	normSysbtn  = new QRadioButton( tr("Normal") );
	vsSysbtn    = new QRadioButton( tr("VS. Sys") );
	plySysbtn   = new QRadioButton( tr("PlayChoice-10") );
	extSysbtn   = new QRadioButton( tr("Extend") );

	connect( normSysbtn, SIGNAL(clicked(bool)), this, SLOT(normSysClicked(bool)) );
	connect( vsSysbtn  , SIGNAL(clicked(bool)), this, SLOT(vsSysClicked(bool)) );
	connect( plySysbtn , SIGNAL(clicked(bool)), this, SLOT(plySysClicked(bool)) );
	connect( extSysbtn , SIGNAL(clicked(bool)), this, SLOT(extSysClicked(bool)) );

	hbox->addWidget( normSysbtn );
	hbox->addWidget( vsSysbtn   );
	hbox->addWidget( plySysbtn  );
	hbox->addWidget( extSysbtn  );

	hbox = new QHBoxLayout();
	vsGroupBox = new QGroupBox( tr("VS. System") );
	vsGroupBox->setLayout( vbox4 );
	vbox3->addWidget( vsGroupBox );
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
	extGroupBox  = new QGroupBox( tr("Extend System") );
	vbox3->addWidget( extGroupBox );
	extGroupBox->setLayout( hbox );

	hbox->addWidget( new QLabel( tr("Console:") ) );
	hbox->addWidget( extCslBox );

	hbox = new QHBoxLayout();
	inputDevBox  = new QComboBox();
	miscRomsEdit = new QLineEdit();
	hdrLayout->addLayout( hbox );
	hbox->addWidget( miscRomsEdit, 1, Qt::AlignLeft );
	hbox->addWidget( miscRomsLbl = new QLabel( tr("Misc. ROM(s)") ), 10, Qt::AlignLeft);
	hbox->addWidget( inputDevLbl = new QLabel( tr("Input Device:") ), 1, Qt::AlignRight );
	hbox->addWidget( inputDevBox, 10, Qt::AlignLeft );

	validator = new fceuDecIntValidtor(0, 3, this);

	miscRomsEdit->setFont( font );
	miscRomsEdit->setMaxLength( 1 );
	miscRomsEdit->setValidator( validator );
	miscRomsEdit->setAlignment(Qt::AlignCenter);
	miscRomsEdit->setMaximumWidth( 3 * fontCharWidth );

	grid = new QGridLayout();
	restoreBtn = new QPushButton( tr("Restore") );
	saveAsBtn  = new QPushButton( tr("Save As") );
	closeBtn   = new QPushButton( tr("Close") );

	grid->addWidget( restoreBtn, 0, 0, Qt::AlignLeft );
	grid->addWidget( saveAsBtn , 0, 1, Qt::AlignRight );
	grid->addWidget( closeBtn  , 0, 2, Qt::AlignRight );

	mainLayout->addLayout( grid );

	setLayout( mainLayout );

	connect( restoreBtn, SIGNAL(clicked(void)), this, SLOT(restoreHeader(void)) );
	connect( saveAsBtn , SIGNAL(clicked(void)), this, SLOT(saveHeader(void)) );
	connect( closeBtn  , SIGNAL(clicked(void)), this, SLOT(closeWindow(void)) );

	i=0;
	while ( vsSysList[i] != NULL )
	{
		vsHwBox->addItem( vsSysList[i], i ); i++;
	}

	i=0;
	while ( vsPPUList[i] != NULL )
	{
		vsPpuBox->addItem( vsPPUList[i], i ); i++;
	}

	i=0;
	while ( extConsoleList[i] != NULL )
	{
		extCslBox->addItem( extConsoleList[i], i ); i++;
	}

	i=0;
	while ( inputDevList[i] != NULL )
	{
		inputDevBox->addItem( inputDevList[i], i ); i++;
	}

	iNesHdr = (iNES_HEADER*)::malloc( sizeof(iNES_HEADER) );

	::memset( iNesHdr, 0, sizeof(iNES_HEADER) );

	if (GameInfo == NULL)
	{
		if ( openFile() == false )
		{
			return;
		}
	}
	if ( loadHeader( iNesHdr ) == false )
	{
		return;
	}

	setHeaderData( iNesHdr );

	initOK = true;
}
//----------------------------------------------------------------------------
iNesHeaderEditor_t::~iNesHeaderEditor_t(void)
{
	printf("Destroy Header Editor Config Window\n");

	if ( iNesHdr )
	{
		free( iNesHdr ); iNesHdr = NULL;
	}
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
void iNesHeaderEditor_t::restoreHeader(void)
{
	if ( iNesHdr != NULL )
	{
		setHeaderData( iNesHdr );
	}
}
//----------------------------------------------------------------------------
void iNesHeaderEditor_t::saveHeader(void)
{
	saveFileAs();
}
//----------------------------------------------------------------------------
void iNesHeaderEditor_t::iNes1Clicked(bool checked)
{
	//printf("INES 1.0 State: %i \n", checked);
	ToggleINES20(false);
}
//----------------------------------------------------------------------------
void iNesHeaderEditor_t::iNes2Clicked(bool checked)
{
	//printf("INES 2.0 State: %i \n", checked);
	ToggleINES20(true);
}
//----------------------------------------------------------------------------
void iNesHeaderEditor_t::normSysClicked(bool checked)
{
	ToggleVSSystemGroup(false);
	ToggleExtendSystemList(false);
}
//----------------------------------------------------------------------------
void iNesHeaderEditor_t::vsSysClicked(bool checked)
{

	ToggleVSSystemGroup(true);
	ToggleExtendSystemList(false);
}
//----------------------------------------------------------------------------
void iNesHeaderEditor_t::plySysClicked(bool checked)
{

	ToggleVSSystemGroup(false);
	ToggleExtendSystemList(false);
}
//----------------------------------------------------------------------------
void iNesHeaderEditor_t::extSysClicked(bool checked)
{

	ToggleVSSystemGroup(false);
	ToggleExtendSystemList(true);
}
//----------------------------------------------------------------------------
void iNesHeaderEditor_t::unofficialStateChange(int state)
{
	ToggleUnofficialPropertiesEnabled( iNes2Btn->isChecked(), state != Qt::Unchecked );
}
//----------------------------------------------------------------------------
void iNesHeaderEditor_t::unofficialPrgRamStateChange(int state)
{
	ToggleUnofficialPrgRamPresent( false, true, state != Qt::Unchecked );
}
//----------------------------------------------------------------------------
void iNesHeaderEditor_t::unofficialDualRegionStateChange(int state)
{
	ToggleUnofficialExtraRegionCode( false, true, state != Qt::Unchecked );
}
//----------------------------------------------------------------------------
void iNesHeaderEditor_t::showErrorMsgWindow(const char *str)
{
	QMessageBox msgBox(this);

	msgBox.setIcon( QMessageBox::Critical );
	msgBox.setText( tr(str) );
	msgBox.show();
	msgBox.exec();
}
//----------------------------------------------------------------------------
bool iNesHeaderEditor_t::loadHeader(iNES_HEADER* header)
{
	int error = 0;
	enum errors {
		OK,
		OPEN_FAILED,
		INVALID_HEADER,
		FDS_HEADER,
		UNIF_HEADER,
		NSF_HEADER//,
//		NSFE_HEADER,
//		NSF2_HEADER
	};

	FCEUFILE* fp = FCEU_fopen(LoadedRomFName, NULL, "rb", NULL);

	if (fp)
	{
		if (FCEU_fread(header, 1, sizeof(iNES_HEADER), fp) == sizeof(iNES_HEADER) && !memcmp(header, "NES\x1A", 4))
		{
			header->cleanup();
		}
		else if (!memcmp(header, "FDS\x1A", 4))
		{
			error = errors::FDS_HEADER;
		}
		else if (!memcmp(header, "UNIF", 4))
		{
			error = errors::UNIF_HEADER;
		}
		else if (!memcmp(header, "NESM", 4))
		{
			error = errors::NSF_HEADER;
		}
/*		else if (!memcmp(header, "NSFE", 4))
			error = errors::NSFE_HEADER;
		else if (!memcmp(header, "NESM\2", 4))
			error = errors::NSF2_HEADER;
*/		else
		{
			error = errors::INVALID_HEADER;
		}
		FCEU_fclose(fp);
	}
	else
	{
		error = errors::OPEN_FAILED;
	}

	if (error)
	{
		switch (error)
		{
			case errors::OPEN_FAILED:
			{
				char buf[2200];
				sprintf(buf, "Error opening %s!", LoadedRomFName);
				showErrorMsgWindow( buf );
				break;
			}
			case errors::INVALID_HEADER:
				//MessageBox(parent, "Invalid iNES header.", "iNES Header Editor", MB_OK | MB_ICONERROR);
				showErrorMsgWindow( "Invalid iNES header." );
				break;
			case errors::FDS_HEADER:
				//MessageBox(parent, "Editing header of an FDS file is not supported.", "iNES Header Editor", MB_OK | MB_ICONERROR);
				showErrorMsgWindow("Editing header of an FDS file is not supported.");
				break;
			case errors::UNIF_HEADER:
				//MessageBox(parent, "Editing header of a UNIF file is not supported.", "iNES Header Editor", MB_OK | MB_ICONERROR);
				showErrorMsgWindow("Editing header of a UNIF file is not supported.");
				break;
			case errors::NSF_HEADER:
//			case errors::NSF2_HEADER:
//			case errors::NSFE_HEADER:
				//MessageBox(parent, "Editing header of an NSF file is not supported.", "iNES Header Editor", MB_OK | MB_ICONERROR);
				showErrorMsgWindow("Editing header of an NSF file is not supported.");
				break;
		}
		return false;
	}

	printf("Opened File: '%s'\n", LoadedRomFName );


	return true;
}
//----------------------------------------------------------------------------
bool iNesHeaderEditor_t::SaveINESFile(const char* path, iNES_HEADER* header)
{
	char buf[4096];
	const char* ext[] = { "nes", 0 };

	// Source file
	FCEUFILE* source = FCEU_fopen(LoadedRomFName, NULL, "rb", 0, -1, ext);
	if (!source)
	{
		sprintf(buf, "Opening source file %s failed.", LoadedRomFName);
		showErrorMsgWindow(buf);
		return false;
	}

	// Destination file
	FILE* target = FCEUD_UTF8fopen(path, "wb");
	if (!target)
	{
		sprintf(buf, "Creating target file %s failed.", path);
		showErrorMsgWindow(buf);
		return false;
	}

	memset(buf, 0, sizeof(buf));

	// Write the header first
	fwrite(header, 1, sizeof(iNES_HEADER), target);
	// Skip the original header
	FCEU_fseek(source, sizeof(iNES_HEADER), SEEK_SET);
	int len;
	while (len = FCEU_fread(buf, 1, sizeof(buf), source))
	{
		fwrite(buf, len, 1, target);
	}

	FCEU_fclose(source);
	fclose(target);

	return true;

}

//----------------------------------------------------------------------------
bool iNesHeaderEditor_t::openFile(void)
{
	int ret, useNativeFileDialogVal;
	QString filename;
	std::string last;
	char dir[512];
	QFileDialog  dialog(this, tr("Open NES File") );

	const QStringList filters(
			{ 
           "NES files (*.nes *.NES)",
           "Any files (*)"
         });

	dialog.setFileMode(QFileDialog::ExistingFile);

	dialog.setNameFilters( filters );

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::AllDirs | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Open") );

	g_config->getOption ("SDL.LastOpenFile", &last );

	getDirFromFile( last.c_str(), dir );

	dialog.setDirectory( tr(dir) );

	// Check config option to use native file dialog or not
	g_config->getOption ("SDL.UseNativeFileDialog", &useNativeFileDialogVal);

	dialog.setOption(QFileDialog::DontUseNativeDialog, !useNativeFileDialogVal);

	dialog.show();
	ret = dialog.exec();

	if ( ret )
	{
		QStringList fileList;
		fileList = dialog.selectedFiles();

		if ( fileList.size() > 0 )
		{
			filename = fileList[0];
		}
	}

   if ( filename.isNull() )
   {
      return false;
   }
	qDebug() << "selected file path : " << filename.toUtf8();

	if ( GameInfo == NULL )
	{
		strcpy( LoadedRomFName, filename.toStdString().c_str() );
	}

   return true;
}
//----------------------------------------------------------------------------
void iNesHeaderEditor_t::saveFileAs(void)
{
	int ret, useNativeFileDialogVal;
	QString filename;
	std::string last;
	char dir[512];
	QFileDialog  dialog(this, tr("Save iNES File") );

	dialog.setFileMode(QFileDialog::AnyFile);

	dialog.setNameFilter(tr("NES Files (*.nes *.NES) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::AllDirs | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Save") );
	dialog.setDefaultSuffix( tr(".nes") );

	getDirFromFile( LoadedRomFName, dir );

	dialog.setDirectory( tr(dir) );

	// Check config option to use native file dialog or not
	g_config->getOption ("SDL.UseNativeFileDialog", &useNativeFileDialogVal);

	dialog.setOption(QFileDialog::DontUseNativeDialog, !useNativeFileDialogVal);

	dialog.show();
	ret = dialog.exec();

	if ( ret )
	{
		QStringList fileList;
		fileList = dialog.selectedFiles();

		if ( fileList.size() > 0 )
		{
			filename = fileList[0];
		}
	}

	if ( filename.isNull() )
   {
      return;
   }
	qDebug() << "selected file path : " << filename.toUtf8();

	WriteHeaderData( iNesHdr );

	if ( SaveINESFile( filename.toStdString().c_str(), iNesHdr ) )
	{
		// Error
	}
}
//----------------------------------------------------------------------------
void iNesHeaderEditor_t::setHeaderData(iNES_HEADER* header)
{
	int i;
	// Temporary buffers
	char buf[256];

	bool ines20 = (header->ROM_type2 & 0xC) == 8;
	bool unofficial = false;

	ToggleINES20( ines20 );

	iNes1Btn->setChecked( !ines20 );
	iNes2Btn->setChecked(  ines20 );

	// Mapper#
	int mapper = header->ROM_type >> 4 | header->ROM_type2 & 0xF0;
	if (ines20)
	{
		mapper |= (header->ROM_type3 & 0x0F) << 8;
	}
	mapperComboBox->setCurrentIndex( mapper );

	// Sub Mapper
	sprintf(buf, "%d", ines20 ? header->ROM_type3 >> 4 : 0);

	mapperSubEdit->setText( tr(buf) );

	// PRG ROM
	int prg_rom = header->ROM_size;
	if (ines20) 
	{
		if ((header->Upper_ROM_VROM_size & 0xF) == 0xF)
			prg_rom = pow(2, header->ROM_size >> 2) * ((header->ROM_size & 3) * 2 + 1);
		else {
			prg_rom |= (header->Upper_ROM_VROM_size & 0xF) << 8;
			prg_rom *= 1024 * 16;
		}
	}
	else
	{
		prg_rom *= 1024 * 16;
	}

	for (i=0; i<prgRomBox->count(); i++)
	{
		if ( prgRomBox->itemData(i).toInt() == prg_rom )
		{
			prgRomBox->setCurrentIndex(i); break;
		}
	}

	// PRG RAM
	int prg_ram = 0;
	if (ines20)
	{
		int shift = header->RAM_size & 0xF;
		if (shift)
		{
			prg_ram = 64 << shift;
		}

	} 
	else
	{
		if (!(header->RAM_size & 0x10) && header->ROM_type3)
		{
			prg_ram = (header->ROM_type3 ? 1 : header->ROM_type3 * 8) * 1024;
		}
	}

	for (i=0; i<prgRamBox->count(); i++)
	{
		if ( prgRamBox->itemData(i).toInt() == prg_ram )
		{
			prgRamBox->setCurrentIndex(i); break;
		}
	}

	// PRG NVRAM
	int prg_nvram = 0;
	if (ines20)
	{
		int shift = header->RAM_size >> 4;
		if (shift)
		{
			prg_nvram = 64 << shift;
		}
		battNvRamBox->hide();
		prgNvRamLbl->show();
		prgNvRamBox->show();
	} 
	else
	{
		prgNvRamLbl->hide();
		prgNvRamBox->hide();
		battNvRamBox->show();
		battNvRamBox->setChecked( header->ROM_type & 0x2 ? true : false );
	}

	for (i=0; i<prgNvRamBox->count(); i++)
	{
		if ( prgNvRamBox->itemData(i).toInt() == prg_nvram )
		{
			prgNvRamBox->setCurrentIndex(i); break;
		}
	}

	// CHR ROM
	int chr_rom = header->VROM_size;
	if (ines20)
	{
		if ((header->Upper_ROM_VROM_size & 0xF0) == 0xF0)
		{
			chr_rom = pow(2, header->VROM_size >> 2) * (((header->VROM_size & 3) * 2) + 1);
		}
		else
		{
			chr_rom |= (header->Upper_ROM_VROM_size & 0xF0) << 4;
			chr_rom *= 8 * 1024;
		}
	}
	else
	{
		chr_rom *= 8 * 1024;
	}

	for (i=0; i<chrRomBox->count(); i++)
	{
		if ( chrRomBox->itemData(i).toInt() == chr_rom )
		{
			chrRomBox->setCurrentIndex(i); break;
		}
	}

	// CHR RAM
	int chr_ram = 0;
	if (ines20)
	{
		int shift = header->VRAM_size & 0xF;
		if (shift)
		{
			chr_ram = 64 << shift;
		}
	}

	for (i=0; i<chrRamBox->count(); i++)
	{
		if ( chrRamBox->itemData(i).toInt() == chr_ram )
		{
			chrRamBox->setCurrentIndex(i); break;
		}
	}

	// CHR NVRAM
	int chr_nvram = 0;
	if (ines20)
	{
		int shift = header->VRAM_size >> 4;
		if (shift)
		{
			chr_nvram = 64 << shift;
		}
	}

	for (i=0; i<chrNvRamBox->count(); i++)
	{
		if ( chrNvRamBox->itemData(i).toInt() == chr_nvram )
		{
			chrNvRamBox->setCurrentIndex(i); break;
		}
	}

	// Mirroring
	if ( header->ROM_type & 8 )
	{
		horzMirrorBtn->setChecked(false);
		vertMirrorBtn->setChecked(false);
		fourMirrorBtn->setChecked(true);
	}
	else if ( header->ROM_type & 1 )
	{
		horzMirrorBtn->setChecked(false);
		vertMirrorBtn->setChecked(true);
		fourMirrorBtn->setChecked(false);
	}
	else 
	{
		horzMirrorBtn->setChecked(true);
		vertMirrorBtn->setChecked(false);
		fourMirrorBtn->setChecked(false);
	}

	// Region
	if (ines20)
	{
		int region = header->TV_system & 3;
		switch (region) 
		{
			case 0:
				// NTSC
				ntscRegionBtn->setChecked(true);
				break;
			case 1:
				// PAL
				palRegionBtn->setChecked(true);
				break;
			case 2:
				// Dual
				dualRegionBtn->setChecked(true);
				break;
			case 3:
				// Dendy
				dendyRegionBtn->setChecked(true);
				break;
		}
	}
	else 
	{
		// Only the unofficial 10th byte has a dual region, we must check it first.
		int region = header->RAM_size & 3;
		if (region == 3 || region == 1)
		{
			// Check the unofficial checkmark and the region code
			dualRegionBtn->setChecked(true);
			unofficial = true;
		}
		else
		{
			// When the region in unofficial byte is inconsistent with the official byte, based on the official byte
			if ( header->Upper_ROM_VROM_size & 1 )
			{
				palRegionBtn->setChecked(true);
			}
			else
			{
				ntscRegionBtn->setChecked(true);
			}
		}
	}

	// System
	int system = header->ROM_type2 & 3;
	switch (system)
	{
		default:
		// Normal
		case 0:
			normSysbtn->setChecked(true);
			break;
		// VS. System
		case 1:
			vsSysbtn->setChecked(true);
			break;
		// PlayChoice-10
		case 2:
			// PlayChoice-10 is an unofficial setting for ines 1.0
			plySysbtn->setChecked(true);
			unofficial = !ines20;
			break;
		// Extend
		case 3:
			// The 7th byte is different between ines 1.0 and 2.0, so we need to check it
			if (ines20) 
			{
				extSysbtn->setChecked(true);
			}
			break;
	}

	// it's quite ambiguous to put them here, but it's better to have a default selection than leave the dropdown blank, because user might switch to ines 2.0 anytime
	// Hardware type
	int hardware = header->VS_hardware >> 4;
	for (i=0; i<vsHwBox->count(); i++)
	{
		if ( vsHwBox->itemData(i).toInt() == hardware )
		{
			vsHwBox->setCurrentIndex(i); break;
		}
	}

	// PPU type
	int ppu = header->VS_hardware & 0xF;
	for (i=0; i<vsPpuBox->count(); i++)
	{
		if ( vsPpuBox->itemData(i).toInt() == ppu )
		{
			vsPpuBox->setCurrentIndex(i); break;
		}
	}
	// Extend Console
	for (i=0; i<extCslBox->count(); i++)
	{
		if ( extCslBox->itemData(i).toInt() == ppu )
		{
			extCslBox->setCurrentIndex(i); break;
		}
	}

	// Input Device:
	int input = header->reserved[1] & 0x1F;
	for (i=0; i<inputDevBox->count(); i++)
	{
		if ( inputDevBox->itemData(i).toInt() == input )
		{
			inputDevBox->setCurrentIndex(i); break;
		}
	}

	// Miscellaneous ROM Area(s)
	sprintf(buf, "%d", header->reserved[0] & 3);
	miscRomsEdit->setText( tr(buf) );

	// Trainer
	trainerCBox->setChecked( header->ROM_type & 4 ? true : false );

	// Unofficial Properties Checkmark
	iNesUnOfBox->setChecked( unofficial );

	// Switch the UI to the proper version
	ToggleINES20( ines20 );

}
//----------------------------------------------------------------------------
void iNesHeaderEditor_t::ToggleINES20(bool ines20)
{
	// only ines 2.0 has these values
	// these are always have when in 2.0
	// Submapper#
	mapperSubLbl->setEnabled(ines20);
	mapperSubEdit->setEnabled(ines20);
	// PRG NVRAM
	prgNvRamLbl->setEnabled(ines20);
	prgNvRamBox->setEnabled(ines20);
	// CHR RAM
	chrRamLbl->setEnabled(ines20);
	chrRamBox->setEnabled(ines20);
	// CHR NVRAM
	chrNvRamBox->setEnabled(ines20);
	chrNvRamLbl->setEnabled(ines20);
	// Dendy in Region Groupbox
	dendyRegionBtn->setEnabled(ines20);
	// Multiple in Regtion Groupbox
	dualRegionBtn->setEnabled(ines20);
	// Extend in System Groupbox
	extGroupBox->setEnabled(ines20);
	extSysbtn->setEnabled(ines20);
	// Input Device
	inputDevLbl->setEnabled(ines20);
	inputDevBox->setEnabled(ines20);
	// Miscellaneous ROMs
	miscRomsLbl->setEnabled(ines20);
	miscRomsEdit->setEnabled(ines20);
	//
	
	if ( ines20 )
	{
		battNvRamBox->hide();
		prgNvRamLbl->show();
		prgNvRamBox->show();
	}
	else
	{
		battNvRamBox->show();
		prgNvRamLbl->hide();
		prgNvRamBox->hide();
	}

	if ( ines20 )
	{
		if ( dendyRegionBtn->isChecked() )
		{
			dendyRegionBtn->setChecked(false);
			palRegionBtn->setChecked(true);
		}

		if ( extSysbtn->isChecked() )
		{
			extSysbtn->setChecked(false);
			normSysbtn->setChecked(true);
		}

	}

	// enable extend dialog only when ines 2.0 and extend button is checked
	ToggleExtendSystemList( ines20 && extSysbtn->isChecked() );

	// enable vs dialog only when ines 2.0 and vs button is checked
	ToggleVSSystemGroup( ines20 && vsSysbtn->isChecked() );

	iNesUnOfBox->setEnabled(!ines20);
	ToggleUnofficialPropertiesEnabled( ines20, iNesUnOfBox->isChecked() );

}
//----------------------------------------------------------------------------
void iNesHeaderEditor_t::ToggleUnofficialPropertiesEnabled(bool ines20, bool check)
{
	bool sub_enable = !ines20 && check;

	// Unofficial Properties only available in ines 1.0
	iNesUnOfGroupBox->setEnabled(sub_enable);
	// when unofficial properties is enabled or in ines 2.0 standard, Playchoice-10 in System groupbox is available
	plySysbtn->setEnabled(ines20 || sub_enable);

	ToggleUnofficialPrgRamPresent(ines20, check, iNesPrgRamBox->isChecked() );
	ToggleUnofficialExtraRegionCode(ines20, check, iNesDualRegBox->isChecked() );

	// Playchoice-10 is not available in ines 1.0 and unofficial is not checked, switch back to normal
	if (!ines20 && !check && plySysbtn->isChecked() )
	{
		normSysbtn->setChecked(true);
	}
}
//----------------------------------------------------------------------------
void iNesHeaderEditor_t::ToggleUnofficialExtraRegionCode(bool ines20, bool unofficial_check, bool check)
{
	// The unofficial byte to determine whether multiple region is valid

	// Multiple in Region Groupbox
	dualRegionBtn->setEnabled(ines20 || unofficial_check && check);

	// Dual region is not avalable when in ines 1.0 and extra region in unofficial is not checked, switch it back to NTSC 
	if (!ines20 && (!unofficial_check || !check) && dualRegionBtn->isChecked() )
	{
		ntscRegionBtn->setChecked(true);
	}
}
//----------------------------------------------------------------------------

void iNesHeaderEditor_t::ToggleUnofficialPrgRamPresent(bool ines20, bool unofficial_check, bool check)
{
	// The unofficial byte to determine wheter PRGRAM exists
	// PRG RAM
	bool enable = ines20 || !unofficial_check || check;
	// When unofficial 10th byte was not set, the PRG RAM is defaultly enabled
	prgRamLbl->setEnabled( enable );
	prgRamBox->setEnabled( enable );
}
//----------------------------------------------------------------------------
void iNesHeaderEditor_t::ToggleExtendSystemList(bool enable)
{
	// Extend combo box only enabled when in iNES 2.0 and Extend System was chosen
	// Extend combo box
	extGroupBox->setEnabled( enable );
	//EnableWindow(GetDlgItem(hwnd, IDC_EXTEND_SYSTEM_GROUP), enable);
	//EnableWindow(GetDlgItem(hwnd, IDC_SYSTEM_EXTEND_COMBO), enable);
	//EnableWindow(GetDlgItem(hwnd, IDC_EXTEND_SYSTEM_TEXT), enable);
}
//----------------------------------------------------------------------------
void iNesHeaderEditor_t::ToggleVSSystemGroup(bool enable)
{
	// VS System Groupbox only enabled when in iNES 2.0 and VS System in System groupbox is chosen
	// VS System Groupbox
	vsGroupBox->setEnabled(enable);
	// System
	//EnableWindow(GetDlgItem(hwnd, IDC_VS_SYSTEM_COMBO), enable);
	//EnableWindow(GetDlgItem(hwnd, IDC_VS_SYSTEM_TEXT), enable);
	// PPU
	//EnableWindow(GetDlgItem(hwnd, IDC_VS_PPU_COMBO), enable);
	//EnableWindow(GetDlgItem(hwnd, IDC_VS_PPU_TEXT), enable);
}
//----------------------------------------------------------------------------
bool iNesHeaderEditor_t::WriteHeaderData(iNES_HEADER* header)
{
	// Temporary buffers
	int idx;
	char buf[256];

	iNES_HEADER _header;
	memset(&_header, 0, sizeof(iNES_HEADER));

	// Check iNES 2.0
	bool ines20 = iNes2Btn->isChecked();
	// iNES 1.0 unofficial byte available
	bool unofficial = !ines20 && iNesUnOfBox->isChecked();

	// iNES 2.0 signature
	if (ines20)
	{
		_header.ROM_type2 |= 8;
	}

	// Mapper
	idx = mapperComboBox->currentIndex();
	int mapper = mapperComboBox->itemData(idx).toInt();

	if (mapper < 4096)
	{
		_header.ROM_type |= (mapper & 0xF) << 4;
		_header.ROM_type2 |= (mapper & 0xF0);
		if (mapper >= 256)
		{
			if (ines20)
			{
				_header.ROM_type3 |= mapper >> 8;
			}
			else
			{
				sprintf(buf, "Error: Mapper# should be less than %d in iNES %d.0 format.", 256, 1);
				showErrorMsgWindow(buf);
				return false;
			}
		}	
	}
	else 
	{
		sprintf(buf, "Mapper# should be less than %d in iNES %d.0 format.", 4096, 2);
		showErrorMsgWindow(buf);
		return false;
	}

	// Sub mapper
	if (ines20) 
	{
		strcpy( buf, mapperSubEdit->text().toStdString().c_str() );
		int submapper;
		if (sscanf(buf, "%d", &submapper) > 0)
		{
			if (submapper < 16)
			{
				_header.ROM_type3 |= submapper << 4;
			}
			else
			{
				showErrorMsgWindow("Error: The sub mapper# should less than 16 in iNES 2.0 format.");
				return false;
			}
		} 
		else
		{
			showErrorMsgWindow("Error: The sub mapper# you have entered is invalid. Please enter a decimal number.");
			return false;
		}
	}

	// PRG ROM
	idx = prgRomBox->currentIndex();
	int prg_rom = prgRomBox->itemData(idx).toInt();

	if (prg_rom >= 0)
	{
		// max value which a iNES 2.0 header can hold
		if (prg_rom < 16 * 1024 * 0xEFF)
		{
			// try to fit the irregular value with the alternative way
			if (prg_rom % (16 * 1024) != 0)
			{
				if (ines20)
				{
					// try to calculate the nearest value
					bool fit = false;
					int result = 0x7FFFFFFF;
					for (int multiplier = 0; multiplier < 4; ++multiplier)
					{
						for (int exponent = 0; exponent < 64; ++exponent)
						{
							int new_result = pow(2, exponent) * (multiplier * 2 + 1);
							if (new_result == prg_rom)
							{
								_header.Upper_ROM_VROM_size |= 0xF;
								_header.ROM_size |= exponent << 2;
								_header.ROM_size |= multiplier & 3;
								fit = true;
								break;
							}
							if (new_result > prg_rom && result > new_result)
							{
								result = new_result;
							}
						}
						if (fit) break;
					}

					if (!fit)
					{
						int result10 = (prg_rom / 16 / 1024 + 1) * 16 * 1024;
						if (result10 < result)
						{
							result = result10;
						}
						char buf2[64];
						if (result % 1024 != 0)
						{
							sprintf(buf2, "%dB", result);
						}
						else
						{
							sprintf(buf2, "%dKB", result / 1024);
						}
						sprintf(buf, "PRG ROM size you entered is invalid in iNES 2.0, do you want to set to its nearest value %s?", buf2);
						showErrorMsgWindow(buf);
						//if (MessageBox(hwnd, buf, "Error", MB_YESNO | MB_ICONERROR) == IDYES)
						//{
						//	SetDlgItemText(hwnd, IDC_PRGROM_COMBO, buf2);
						//}
						//else
						//{
						//	SetFocus(GetDlgItem(hwnd, IDC_PRGROM_COMBO));
						//	return false;
						//}
						return false;
					}
				}
				else 
				{
					// ines 1.0 can't handle this kind of value
					showErrorMsgWindow("PRG ROM size must be multiple of 16KB in iNES 1.0");
					return false;
				}
			}
			// it's multiple size of 16KB
			else {
				// it can be fitted in iNES 1.0
				if (prg_rom < 16 * 1024 * 0xFF)
				{
					_header.ROM_size |= prg_rom / 16 / 1024 & 0xFF;
				}
				else
				{
					if (ines20)
					{
						_header.Upper_ROM_VROM_size |= prg_rom / 16 / 1024 >> 8 & 0xF;
					}
					else 
					{
						showErrorMsgWindow("PRG ROM size exceeded the limit of iNES 1.0 (4080KB).");
						return false;
					}
				}
			}
		}
		// A too large size
		else 
		{
			showErrorMsgWindow("PRG ROM size you entered is too large to fit into a cartridge, by the way this is an NES emulator, not for XBOX360 or PlayStation2.");
			return false;
		}
	} 
	else
	{
		return false;
	}

	// PRG RAM
	if (ines20 || !unofficial || iNesPrgRamBox->isChecked() )
	{
		idx = prgRamBox->currentIndex();
		int prg_ram = prgRamBox->itemData(idx).toInt();

		if (prg_ram >= 0)
		{
			if (ines20)
			{
				if (prg_ram < 64 << 0xF)
				{
					if (prg_ram % 64 == 0)
					{
						_header.RAM_size |= (int)log2(prg_ram / 64);
					}
					else
					{
						showErrorMsgWindow("Invalid PRG RAM size");
						return false;
					}
				}
				else 
				{
					showErrorMsgWindow("PRG RAM size exceeded the limit (4096KB)");
					return false;
				}
			}
			else 
			{
				if (prg_ram < 8 * 1024 * 255)
				{
					if (prg_ram % (8 * 1024) == 0)
					{
						_header.ROM_type3 |= prg_ram / 8 / 1024;
					}
					else 
					{
						showErrorMsgWindow("PRG RAM size must be multiple of 8KB in iNES 1.0");
						return false;
					}
				}
				else {
					showErrorMsgWindow("PRG RAM size exceeded the limit (2040KB)");
					return false;
				}
			}
		}
		else
		{
			return false;
		}
	}

	// PRG NVRAM
	if (ines20)
	{
		// only iNES 2.0 has value for PRG VMRAM
		idx = prgNvRamBox->currentIndex();
		int prg_nvram = prgNvRamBox->itemData(idx).toInt();

		if (prg_nvram >= 0)
		{
			if (prg_nvram < 64 << 0xF)
			{
				if (prg_nvram % 64 == 0)
				{
					_header.RAM_size |= (int)log2(prg_nvram / 64) << 4;
				}
				else
				{
					showErrorMsgWindow("Invalid PRG NVRAM size");
					return false;
				}
			}
			else
			{
				showErrorMsgWindow("PRG NVRAM size exceeded the limit (4096KB)");
				return false;
			}

			if (prg_nvram != 0)
			{
				_header.ROM_type |= 2;
			}
		}
		else
		{
			return false;
		}
	}
	else 
	{
		// iNES 1.0 is much simpler, it has only 1 bit for check
		if ( battNvRamBox->isChecked() )
		{
			_header.ROM_type |= 2;
		}
	}

	// CHR ROM
	idx = chrRomBox->currentIndex();
	int chr_rom = chrRomBox->itemData(idx).toInt();

	if (chr_rom >= 0)
	{
		// max value which a iNES 2.0 header can hold
		if (chr_rom < 8 * 1024 * 0xEFF)
		{
			// try to fit the irregular value with the alternative way
			if (chr_rom % (8 * 1024) != 0)
			{
				if (ines20)
				{
					// try to calculate the nearest value
					bool fit = false;
					int result = 0;
					for (int multiplier = 0; multiplier < 4; ++multiplier)
					{
						for (int exponent = 0; exponent < 64; ++exponent)
						{
							int new_result = pow(2, exponent) * (multiplier * 2 + 1);
							if (new_result == chr_rom)
							{
								_header.Upper_ROM_VROM_size |= 0xF0;
								_header.VROM_size |= exponent << 2;
								_header.VROM_size |= multiplier & 3;
								fit = true;
								break;
							}
							if (result > new_result && new_result > chr_rom)
							{
								result = new_result;
							}
						}
						if (fit) break;
					}

					if (!fit)
					{
						int result10 = (chr_rom / 1024 / 8 + 1) * 8 * 1024;
						if (result10 < result)
						{
							result = result10;
						}
						char buf2[64];
						if (result % 1024 != 0)
						{
							sprintf(buf2, "%dB", result);
						}
						else
						{
							sprintf(buf2, "%dKB", result / 1024);
						}
						sprintf(buf, "CHR ROM size you entered is invalid in iNES 2.0, do you want to set to its nearest value %s?", buf2);
						showErrorMsgWindow(buf);
						//if (MessageBox(hwnd, buf, "Error", MB_YESNO | MB_ICONERROR) == IDYES)
						//	SetDlgItemText(hwnd, IDC_CHRROM_COMBO, buf2);
						//else
						//{
						//	SetFocus(GetDlgItem(hwnd, IDC_CHRROM_COMBO));
						//	return false;
						//}
						return false;
					}
				}
				else 
				{
					// ines 1.0 can't handle this kind of value
					showErrorMsgWindow("CHR ROM size must be multiple of 8KB in iNES 1.0");
					return false;
				}
			}
			// it's multiple size of 8KB
			else {
				// it can be fitted in iNES 1.0
				if (chr_rom < 8 * 1024 * 0xFF)
				{
					_header.VROM_size |= chr_rom / 8 / 1024 & 0xFF;
				}
				else
				{
					if (ines20)
					{
						_header.Upper_ROM_VROM_size |= chr_rom / 8 / 1024 >> 4 & 0xF0;
					}
					else
					{
						showErrorMsgWindow("CHR ROM size exceeded the limit of iNES 1.0 (2040KB).");
						return false;
					}
				}
			}
		}
		// A too large size
		else 
		{
			showErrorMsgWindow("CHR ROM size you entered cannot be fitted in iNES 2.0.");
			return false;
		}
	}
	else
	{
		return false;
	}

	// CHR RAM
	if (ines20)
	{
		// only iNES 2.0 has value for CHR RAM
		idx = chrRamBox->currentIndex();
		int chr_ram = chrRamBox->itemData(idx).toInt();

		if (chr_ram >= 0)
		{
			if (chr_ram < 64 << 0xF)
			{
				if (chr_ram % 64 == 0)
				{
					_header.VRAM_size |= (int)log2(chr_ram / 64);
				}
				else
				{
					showErrorMsgWindow("Invalid CHR RAM size");
					return false;
				}
			}
			else 
			{
				showErrorMsgWindow("CHR RAM size exceeded the limit (4096KB)");
				return false;
			}
		}
		else
		{
			return false;
		}
	}

	// CHR NVRAM
	if (ines20)
	{
		// only iNES 2.0 has value for CHR NVRAM
		idx = chrNvRamBox->currentIndex();
		int chr_nvram = chrNvRamBox->itemData(idx).toInt();

		if (chr_nvram >= 0)
		{
			if (chr_nvram < 64 << 0xF)
			{
				if (chr_nvram % 64 == 0)
				{
					_header.VRAM_size |= (int)log2(chr_nvram / 64) << 4;
				}
				else 
				{
					showErrorMsgWindow("Invalid CHR NVRAM size");
					return false;
				}
			}
			else 
			{
				showErrorMsgWindow("CHR NVRAM size exceeded the limit (4096KB)");
				return false;
			}

			if (chr_nvram != 0)
			{
				_header.ROM_type |= 2;
			}
		}
		else
		{
			return false;
		}
	}

	// Mirroring
	if ( fourMirrorBtn->isChecked() )
	{
		_header.ROM_type |= 8;
	}
	else if ( vertMirrorBtn->isChecked() )
	{
		_header.ROM_type |= 1;
	}

	// Region
	if ( palRegionBtn->isChecked() )
	{
		if (ines20)
		{
			_header.TV_system |= 1;
		}
		else 
		{
			_header.Upper_ROM_VROM_size |= 1;
			if ( unofficial && iNesDualRegBox->isChecked() )
			{
				_header.RAM_size |= 2;
			}
		}
	}
	else if ( dualRegionBtn->isChecked() )
	{
		if (ines20)
		{
			_header.TV_system |= 2;
		}
		else
		{
			_header.RAM_size |= 3;
		}
	}
	else if ( dendyRegionBtn->isChecked() )
	{
		_header.TV_system |= 3;
	}

	// System
	if ( vsSysbtn->isChecked() )
	{
		_header.ROM_type2 |= 1;
		if (ines20) {
			// VS System type
			idx = vsHwBox->currentIndex();
			int system = vsHwBox->itemData(idx).toInt();

			if (system <= 0xF)
			{
				_header.VS_hardware |= (system & 0xF) << 4;
			}
			else
			{
				showErrorMsgWindow("Invalid VS System hardware type.");
				return false;
			}
			// VS PPU type
			idx = vsPpuBox->currentIndex();
			int ppu = vsPpuBox->itemData(idx).toInt();
			if ( (ppu >= 0) && (system <= 0xF) )
			{
				_header.VS_hardware |= ppu & 0xF;
			}
			else
			{
				showErrorMsgWindow("Invalid VS System PPU type.");
				return false;
			}
		}
	}
	else if ( plySysbtn->isChecked() )
	{
		_header.ROM_type2 |= 2;
	}
	else if ( extSysbtn->isChecked() )
	{
		// Extend System
		_header.ROM_type2 |= 3;
		idx = extCslBox->currentIndex();
		int extend = extCslBox->itemData(idx).toInt();

		if (extend >= 0)
		{
			_header.VS_hardware |= extend & 0x3F;
		}
		else
		{
			showErrorMsgWindow("Invalid extend system type");
			return false;
		}			
	}

	// Input device
	if (ines20)
	{
		idx = inputDevBox->currentIndex();
		int input = inputDevBox->itemData(idx).toInt();
		if (input <= 0x3F)
		{
			_header.reserved[1] |= input & 0x3F;
		}
		else
		{
			showErrorMsgWindow("Invalid input device.");
			return false;
		}
	}

	// Miscellanous ROM(s)
	if (ines20)
	{
		strcpy( buf, miscRomsEdit->text().toStdString().c_str() );
		int misc_roms = 0;
		if (sscanf(buf, "%d", &misc_roms) < 1)
		{
			showErrorMsgWindow("Invalid miscellanous ROM(s) count. If you don't know what value should be, we recommend to set it to 0.");
			return false;
		}
		if (misc_roms > 3)
		{
			showErrorMsgWindow("Miscellanous ROM(s) count has exceeded the limit of iNES 2.0 (3)");
			return false;
		}
		_header.reserved[0] |= misc_roms & 3;
	}

	// iNES 1.0 unofficial properties
	if ( !ines20 && unofficial )
	{
		// bus conflict configure in unoffcial bit of iNES 1.0
		if ( iNesBusCfltBox->isChecked() )
		{
			_header.RAM_size |= 0x20;
		}
		// PRG RAM non exist bit flag
		if ( iNesPrgRamBox->isChecked() )
		{
			_header.RAM_size |= 0x10;
		}
	}

	// Trainer
	if ( trainerCBox->isChecked() )
	{
		_header.ROM_type |= 4;
	}

	bool fceux_support = false;
	for (int i = 0; bmap[i].init; ++i)
	{
		if (mapper == bmap[i].number)
		{
			fceux_support = true;
			break;
		}
	}

	if (!fceux_support)
	{
		int ret;
		QMessageBox msgBox(this);

		sprintf(buf, "FCEUX doesn't support iNES Mapper# %d, this is not a serious problem, but the ROM will not be run in FCEUX properly.\nDo you want to continue?", mapper);

		msgBox.setIcon( QMessageBox::Warning );
		msgBox.setText( tr(buf) );
		ret = msgBox.exec();

		if (ret == 0)
		{
			return false;
		}
	}

	memcpy(_header.ID, "NES\x1A", 4);

	if (header)
	{
		memcpy(header, &_header, sizeof(iNES_HEADER));
	}

	return true;
}
//----------------------------------------------------------------------------
void iNesHeaderEditor_t::printHeader(iNES_HEADER* _header)
{
	printf("header: ");
	printf("%02X ", _header->ID[0]);
	printf("%02X ", _header->ID[1]);
	printf("%02X ", _header->ID[2]);
	printf("%02X ", _header->ID[3]);
	printf("%02X ", _header->ROM_size);
	printf("%02X ", _header->VROM_size);
	printf("%02X ", _header->ROM_type);
	printf("%02X ", _header->ROM_type2);
	printf("%02X ", _header->ROM_type3);
	printf("%02X ", _header->Upper_ROM_VROM_size);
	printf("%02X ", _header->RAM_size);
	printf("%02X ", _header->VRAM_size);
	printf("%02X ", _header->TV_system);
	printf("%02X ", _header->VS_hardware);
	printf("%02X ", _header->reserved[0]);
	printf("%02X\n", _header->reserved[1]);
}
//----------------------------------------------------------------------------
