// PaletteConf.cpp
//
#include <QFileDialog>

#include "Qt/PaletteConf.h"
#include "Qt/main.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/fceuWrapper.h"

//----------------------------------------------------
PaletteConfDialog_t::PaletteConfDialog_t(QWidget *parent)
	: QDialog( parent )
{
	QVBoxLayout *mainLayout, *vbox;
	QHBoxLayout *hbox1;
   QGroupBox *frame;
   QPushButton *closebutton;
	QPushButton *button;
	int hue, tint;
	char stmp[64];

	// sync with config
	g_config->getOption ("SDL.Hue", &hue);
	g_config->getOption ("SDL.Tint", &tint);

   setWindowTitle( tr("Palette Config") );

	mainLayout = new QVBoxLayout();

	frame = new QGroupBox( tr("Custom Palette:") );
	hbox1 = new QHBoxLayout();

	button = new QPushButton( tr("Open Palette") );
	hbox1->addWidget( button );

	connect( button, SIGNAL(clicked(void)), this, SLOT(openPaletteFile(void)) );

	custom_palette_path = new QLineEdit();
	custom_palette_path->setReadOnly(true);
	hbox1->addWidget( custom_palette_path );

	button = new QPushButton( tr("Clear") );
	hbox1->addWidget( button );

	connect( button, SIGNAL(clicked(void)), this, SLOT(clearPalette(void)) );

	frame->setLayout( hbox1 );

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

	setLayout( mainLayout );
}

//----------------------------------------------------
PaletteConfDialog_t::~PaletteConfDialog_t(void)
{

}
//----------------------------------------------------
void PaletteConfDialog_t::closeWindow(void)
{
   //printf("Close Window\n");
   done(0);
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

	fceuWrapperLock();
	FCEUI_SetNTSCTH (c, t, v);
	fceuWrapperUnLock();
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

	fceuWrapperLock();
	FCEUI_SetNTSCTH (c, v, h);
	fceuWrapperUnLock();
}
//----------------------------------------------------
void PaletteConfDialog_t::use_NTSC_Changed(int state)
{
	int value = (state == Qt::Unchecked) ? 0 : 1;

	g_config->setOption ("SDL.NTSCpalette", value);
	g_config->save ();
	UpdateEMUCore (g_config);
}
//----------------------------------------------------
void PaletteConfDialog_t::clearPalette(void)
{
	g_config->setOption ("SDL.Palette", 0);
	custom_palette_path->setText("");
}
//----------------------------------------------------
void PaletteConfDialog_t::openPaletteFile(void)
{
	int ret;
	QString filename;
	QFileDialog  dialog(this, tr("Open NES Palette") );

	dialog.setFileMode(QFileDialog::ExistingFile);

	dialog.setNameFilter(tr("NES Palettes (*.pal)(*.PAL) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);

	dialog.setDirectory( tr("/usr/share/fceux/palettes") );

	// the gnome default file dialog is not playing nice with QT.
	// TODO make this a config option to use native file dialog.
	dialog.setOption(QFileDialog::DontUseNativeDialog, true);

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

	g_config->setOption ("SDL.Palette", filename.toStdString().c_str() );
	g_config->setOption ("SDL.NTSCpalette", 0);

	fceuWrapperLock();
	LoadCPalette ( filename.toStdString().c_str() );
	fceuWrapperUnLock();

	custom_palette_path->setText( filename.toStdString().c_str() );

	useNTSC->setChecked( 0 );

   return;
}
//----------------------------------------------------
