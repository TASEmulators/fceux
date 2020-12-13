// PaletteConf.cpp
//
#include <QFileDialog>
#include <QTextEdit>

#include "Qt/PaletteConf.h"
#include "Qt/main.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/fceuWrapper.h"
#include "Qt/ConsoleUtilities.h"

#include "../../ppu.h"

extern bool force_grayscale;

static const char *commentText = 
"Palette Selection uses the 1st Matching Condition:\n\
   1. Game type is NSF (always uses fixed palette) \n\
   2. Custom User Palette is Available and Enabled \n\
   3. NTSC Color Emulation is Enabled \n\
   4. Individual Game Palette is Available \n\
   5. Default Built-in Palette   ";
//----------------------------------------------------
PaletteConfDialog_t::PaletteConfDialog_t(QWidget *parent)
	: QDialog( parent )
{
	QVBoxLayout *mainLayout, *vbox;
	QHBoxLayout *hbox1;
   QGroupBox *frame;
   //QPushButton *closebutton;
	QPushButton *button;
	QTextEdit *comments;
	QStyle    *style;
	int hue, tint;
	char stmp[64];
	std::string paletteFile;

	style = this->style();

	resize( 512, 600 );

	// sync with config
	g_config->getOption ("SDL.Hue", &hue);
	g_config->getOption ("SDL.Tint", &tint);

   setWindowTitle( tr("Palette Config") );

	mainLayout = new QVBoxLayout();

	frame = new QGroupBox( tr("Custom Palette:") );
	vbox  = new QVBoxLayout();
	hbox1 = new QHBoxLayout();

	useCustom  = new QCheckBox( tr("Use Custom Palette") );
	GrayScale  = new QCheckBox( tr("Force Grayscale") );
	deemphSwap = new QCheckBox( tr("De-emphasis Bit Swap") );

	useCustom->setChecked( FCEUI_GetUserPaletteAvail() );
	GrayScale->setChecked( force_grayscale );
	deemphSwap->setChecked( paldeemphswap );

	connect(useCustom , SIGNAL(stateChanged(int)), this, SLOT(use_Custom_Changed(int)) );
	connect(GrayScale , SIGNAL(stateChanged(int)), this, SLOT(force_GrayScale_Changed(int)) );
	connect(deemphSwap, SIGNAL(stateChanged(int)), this, SLOT(deemphswap_Changed(int)) );

	button = new QPushButton( tr("Open Palette") );
	button->setIcon( style->standardIcon( QStyle::SP_FileDialogStart ) );
	hbox1->addWidget( button );

	connect( button, SIGNAL(clicked(void)), this, SLOT(openPaletteFile(void)) );

	g_config->getOption ("SDL.Palette", &paletteFile);

	custom_palette_path = new QLineEdit();
	custom_palette_path->setReadOnly(true);
	custom_palette_path->setText( paletteFile.c_str() );

	vbox->addWidget( useCustom );
	vbox->addLayout( hbox1 );
	vbox->addWidget( custom_palette_path );
	vbox->addWidget( GrayScale );
	vbox->addWidget( deemphSwap);


	button = new QPushButton( tr("Clear") );
	button->setIcon( style->standardIcon( QStyle::SP_LineEditClearButton ) );
	hbox1->addWidget( button );

	connect( button, SIGNAL(clicked(void)), this, SLOT(clearPalette(void)) );

	frame->setLayout( vbox );

	mainLayout->addWidget( frame );

	frame = new QGroupBox( tr("NTSC Palette Controls:") );

	vbox    = new QVBoxLayout();
	useNTSC = new QCheckBox( tr("Use NTSC Palette") );

	int ntscPaletteEnable;
	g_config->getOption("SDL.NTSCpalette", &ntscPaletteEnable);
	useNTSC->setChecked( ntscPaletteEnable );

	connect(useNTSC , SIGNAL(stateChanged(int)), this, SLOT(use_NTSC_Changed(int)) );

	vbox->addWidget( useNTSC );

	sprintf( stmp, "Tint: %3i \n", tint );
	tintFrame = new QGroupBox( tr(stmp) );
	hbox1 = new QHBoxLayout();
	tintSlider = new QSlider( Qt::Horizontal );
	tintSlider->setMinimum(  0);
	tintSlider->setMaximum(128);
	tintSlider->setValue( tint );

	connect( tintSlider, SIGNAL(valueChanged(int)), this, SLOT(tintChanged(int)) );

	hbox1->addWidget( tintSlider );
	tintFrame->setLayout( hbox1 );
	vbox->addWidget( tintFrame );

	sprintf( stmp, "Hue: %3i \n", hue );
	hueFrame = new QGroupBox( tr(stmp) );
	hbox1 = new QHBoxLayout();
	hueSlider = new QSlider( Qt::Horizontal );
	hueSlider->setMinimum(  0);
	hueSlider->setMaximum(128);
	hueSlider->setValue( hue );

	connect( hueSlider, SIGNAL(valueChanged(int)), this, SLOT(hueChanged(int)) );

	hbox1->addWidget( hueSlider );
	hueFrame->setLayout( hbox1 );
	vbox->addWidget( hueFrame );

	frame->setLayout( vbox );

	mainLayout->addWidget( frame );

	comments = new QTextEdit();

	comments->setText( commentText );
	comments->moveCursor(QTextCursor::Start);
	comments->setReadOnly(true);

	mainLayout->addWidget( comments );

	setLayout( mainLayout );
}

//----------------------------------------------------
PaletteConfDialog_t::~PaletteConfDialog_t(void)
{
   printf("Destroy Palette Config Window\n");
}
//----------------------------------------------------------------------------
void PaletteConfDialog_t::closeEvent(QCloseEvent *event)
{
   printf("Palette Config Close Window Event\n");
   done(0);
	deleteLater();
   event->accept();
}
//----------------------------------------------------
void PaletteConfDialog_t::closeWindow(void)
{
   //printf("Close Window\n");
   done(0);
	deleteLater();
}
//----------------------------------------------------
void PaletteConfDialog_t::hueChanged(int v)
{
	int c, t;
	char stmp[64];

	sprintf( stmp, "Hue: %3i", v );

	hueFrame->setTitle(stmp);

	g_config->setOption ("SDL.Hue", v);
	g_config->save ();
	g_config->getOption ("SDL.Tint", &t);
	g_config->getOption ("SDL.NTSCpalette", &c);

	if ( fceuWrapperTryLock() )
	{
		FCEUI_SetNTSCTH (c, t, v);
		fceuWrapperUnLock();
	}
}
//----------------------------------------------------
void PaletteConfDialog_t::tintChanged(int v)
{
	int c, h;
	char stmp[64];

	sprintf( stmp, "Tint: %3i", v );

	tintFrame->setTitle(stmp);

	g_config->setOption ("SDL.Tint", v);
	g_config->save ();
	g_config->getOption ("SDL.NTSCpalette", &c);
	g_config->getOption ("SDL.Hue", &h);

	if ( fceuWrapperTryLock() )
	{
		FCEUI_SetNTSCTH (c, v, h);
		fceuWrapperUnLock();
	}
}
//----------------------------------------------------
void PaletteConfDialog_t::use_Custom_Changed(int state)
{
	int value = (state == Qt::Unchecked) ? 0 : 1;
	std::string filename;

	//printf("Use Custom:%i \n", state );

	g_config->getOption ("SDL.Palette", &filename);

	if ( fceuWrapperTryLock() )
	{
		if ( value && (filename.size() > 0) )
		{
			LoadCPalette ( filename.c_str() );
		}
		else
		{
			FCEUI_SetUserPalette( NULL, 0);
		}
		fceuWrapperUnLock();
	}
}
//----------------------------------------------------
void PaletteConfDialog_t::force_GrayScale_Changed(int state)
{
	int value = (state == Qt::Unchecked) ? 0 : 1;

	if ( fceuWrapperTryLock() )
	{
		int e, h, t;
		g_config->getOption ("SDL.NTSCpalette", &e);
		g_config->getOption ("SDL.Hue", &h);
		g_config->getOption ("SDL.Tint", &t);
		force_grayscale = value ? true : false;
		FCEUI_SetNTSCTH( e, t, h);
		fceuWrapperUnLock();
	}
}
//----------------------------------------------------
void PaletteConfDialog_t::deemphswap_Changed(int state)
{
	int value = (state == Qt::Unchecked) ? 0 : 1;

	if ( fceuWrapperTryLock() )
	{
		int e, h, t;
		g_config->getOption ("SDL.NTSCpalette", &e);
		g_config->getOption ("SDL.Hue", &h);
		g_config->getOption ("SDL.Tint", &t);
		paldeemphswap = value ? true : false;
		FCEUI_SetNTSCTH( e, t, h);
		fceuWrapperUnLock();
	}
}
//----------------------------------------------------
void PaletteConfDialog_t::use_NTSC_Changed(int state)
{
	int h, t;
	int value = (state == Qt::Unchecked) ? 0 : 1;

	g_config->setOption ("SDL.NTSCpalette", value);
	g_config->save ();

	g_config->getOption ("SDL.Hue", &h);
	g_config->getOption ("SDL.Tint", &t);

	if ( fceuWrapperTryLock() )
	{
		FCEUI_SetNTSCTH (value, t, h);
		//UpdateEMUCore (g_config);
		fceuWrapperUnLock();
	}
}
//----------------------------------------------------
void PaletteConfDialog_t::clearPalette(void)
{
	g_config->setOption ("SDL.Palette", "");
	custom_palette_path->setText("");

	if ( fceuWrapperTryLock() )
	{
		FCEUI_SetUserPalette( NULL, 0);
		fceuWrapperUnLock();
		useCustom->setChecked(false);
	}
}
//----------------------------------------------------
void PaletteConfDialog_t::openPaletteFile(void)
{
	int ret, useNativeFileDialogVal;
	QString filename;
	std::string last;
	char dir[512];
	QFileDialog  dialog(this, tr("Open NES Palette") );

	dialog.setFileMode(QFileDialog::ExistingFile);

	dialog.setNameFilter(tr("NES Palettes (*.pal *.PAL) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::AllDirs | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Load") );

	g_config->getOption ("SDL.Palette", &last );

   if ( last.size() == 0 )
   {
      last.assign( "/usr/share/fceux/palettes" );
   }

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
      return;
   }
	qDebug() << "selected file path : " << filename.toUtf8();

	if ( fceuWrapperTryLock() )
	{
		if ( LoadCPalette ( filename.toStdString().c_str() ) )
		{
			g_config->setOption ("SDL.Palette", filename.toStdString().c_str() );
			custom_palette_path->setText( filename.toStdString().c_str() );
		}
		else
		{
			printf("Error: Failed to Load Palette File: %s \n", filename.toStdString().c_str() );
		}
		fceuWrapperUnLock();

		useCustom->setChecked( FCEUI_GetUserPaletteAvail() );
	}


   return;
}
//----------------------------------------------------
