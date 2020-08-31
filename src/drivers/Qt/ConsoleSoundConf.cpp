// ConsoleSoundConf.cpp
//
#include <QCloseEvent>

#include "../../fceu.h"
#include "../../driver.h"
#include "Qt/ConsoleSoundConf.h"
#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/config.h"
#include "Qt/fceuWrapper.h"

//----------------------------------------------------
ConsoleSndConfDialog_t::ConsoleSndConfDialog_t(QWidget *parent)
	: QDialog( parent )
{
	int buf;
	QHBoxLayout *hbox1, *hbox2;
	QVBoxLayout *vbox1, *vbox2;
	QLabel *lbl;
	QGroupBox *frame;
	QSlider *vslider;

	setWindowTitle( tr("Sound Config") );

	hbox1 = new QHBoxLayout();
	vbox1 = new QVBoxLayout();

	// Enable Sound Select
	enaChkbox  = new QCheckBox( tr("Enable Sound") );
	// Enable Low Pass Filter Select
	enaLowPass = new QCheckBox( tr("Enable Low Pass Filter") );

	setCheckBoxFromProperty( enaChkbox , "SDL.Sound" );
	setCheckBoxFromProperty( enaLowPass, "SDL.Sound.LowPass" );

	connect(enaChkbox , SIGNAL(stateChanged(int)), this, SLOT(enaSoundStateChange(int)) );
	connect(enaLowPass, SIGNAL(stateChanged(int)), this, SLOT(enaSoundLowPassChange(int)) );

	vbox1->addWidget( enaChkbox  );
	vbox1->addWidget( enaLowPass );

	// Audio Quality Select
	hbox2 = new QHBoxLayout();

	lbl = new QLabel( tr("Quality:") );

	qualitySelect = new QComboBox();

	qualitySelect->addItem( tr("Low")      , 0 );
   qualitySelect->addItem( tr("High")     , 1 );
   qualitySelect->addItem( tr("Very High"), 2 );

	setComboBoxFromProperty( qualitySelect, "SDL.Sound.Quality" );

	connect(qualitySelect, SIGNAL(currentIndexChanged(int)), this, SLOT(soundQualityChanged(int)) );

	hbox2->addWidget( lbl );
	hbox2->addWidget( qualitySelect );

	vbox1->addLayout( hbox2 );

	// Sample Rate Select
	hbox2 = new QHBoxLayout();

	lbl = new QLabel("Rate:");

	rateSelect = new QComboBox();

	rateSelect->addItem( tr("11025"), 11025 );
	rateSelect->addItem( tr("22050"), 22050 );
	rateSelect->addItem( tr("44100"), 44100 );
	rateSelect->addItem( tr("48000"), 48000 );
	rateSelect->addItem( tr("96000"), 96000 );

	setComboBoxFromProperty( rateSelect, "SDL.Sound.Rate" );

	connect(rateSelect, SIGNAL(currentIndexChanged(int)), this, SLOT(soundRateChanged(int)) );

	g_config->getOption ("SDL.Sound.Rate", &buf);

	hbox2->addWidget( lbl );
	hbox2->addWidget( rateSelect );

	vbox1->addLayout( hbox2 );

	// Buffer Size Select
	//
	hbox2 = new QHBoxLayout();

	lbl = new QLabel( tr("Buffer Size (in ms):") );

	bufSizeLabel  = new QLabel("128");
	bufSizeSlider = new QSlider( Qt::Horizontal );

	bufSizeSlider->setMinimum( 15);
	bufSizeSlider->setMaximum(200);
	//bufSizeSlider->setSliderPosition(128);
	setSliderFromProperty( bufSizeSlider, bufSizeLabel, "SDL.Sound.BufSize" );

	hbox2->addWidget( lbl );
	hbox2->addWidget( bufSizeLabel );

	vbox1->addLayout( hbox2 );
	vbox1->addWidget( bufSizeSlider );

	connect(bufSizeSlider, SIGNAL(valueChanged(int)), this, SLOT(bufSizeChanged(int)) );
	
	// Swap Duty Cycles
	swapDutyChkbox  = new QCheckBox( tr("Swap Duty Cycles") );
	vbox1->addWidget( swapDutyChkbox );

	setCheckBoxFromProperty( swapDutyChkbox , "SDL.SwapDuty" );

	connect(swapDutyChkbox , SIGNAL(stateChanged(int)), this, SLOT(swapDutyCallback(int)) );

	hbox1->addLayout( vbox1 );

	frame = new QGroupBox(tr("Mixer:"));
	hbox2 = new QHBoxLayout();

	frame->setLayout( hbox2 );

	hbox1->addWidget( frame );

	frame   = new QGroupBox(tr("Volume"));
	vbox2   = new QVBoxLayout();
	volLbl  = new QLabel("150");
	vslider = new QSlider( Qt::Vertical );
	vslider->setMinimum(  0);
	vslider->setMaximum(255);
	setSliderFromProperty( vslider, volLbl, "SDL.Sound.Volume" );

	vbox2->addWidget( volLbl  );
	vbox2->addWidget( vslider );
	frame->setLayout( vbox2   );
	hbox2->addWidget( frame   );

	connect(vslider, SIGNAL(valueChanged(int)), this, SLOT(volumeChanged(int)) );

	frame   = new QGroupBox(tr("Triangle"));
	vbox2   = new QVBoxLayout();
	triLbl  = new QLabel("255");
	vslider = new QSlider( Qt::Vertical );
	vslider->setMinimum(  0);
	vslider->setMaximum(255);
	setSliderFromProperty( vslider, triLbl, "SDL.Sound.TriangleVolume" );

	vbox2->addWidget( triLbl  );
	vbox2->addWidget( vslider );
	frame->setLayout( vbox2   );
	hbox2->addWidget( frame   );

	connect(vslider, SIGNAL(valueChanged(int)), this, SLOT(triangleChanged(int)) );
	
	frame   = new QGroupBox(tr("Square1"));
	vbox2   = new QVBoxLayout();
	sqr1Lbl = new QLabel("255");
	vslider = new QSlider( Qt::Vertical );
	vslider->setMinimum(  0);
	vslider->setMaximum(255);
	setSliderFromProperty( vslider, sqr1Lbl, "SDL.Sound.Square1Volume" );

	vbox2->addWidget( sqr1Lbl );
	vbox2->addWidget( vslider );
	frame->setLayout( vbox2   );
	hbox2->addWidget( frame   );

	connect(vslider, SIGNAL(valueChanged(int)), this, SLOT(square1Changed(int)) );

	frame   = new QGroupBox(tr("Square2"));
	vbox2   = new QVBoxLayout();
	sqr2Lbl = new QLabel("255");
	vslider = new QSlider( Qt::Vertical );
	vslider->setMinimum(  0);
	vslider->setMaximum(255);
	setSliderFromProperty( vslider, sqr2Lbl, "SDL.Sound.Square2Volume" );

	vbox2->addWidget( sqr2Lbl );
	vbox2->addWidget( vslider );
	frame->setLayout( vbox2   );
	hbox2->addWidget( frame   );

	connect(vslider, SIGNAL(valueChanged(int)), this, SLOT(square2Changed(int)) );

	frame   = new QGroupBox(tr("Noise"));
	vbox2   = new QVBoxLayout();
	nseLbl  = new QLabel("255");
	vslider = new QSlider( Qt::Vertical );
	vslider->setMinimum(  0);
	vslider->setMaximum(255);
	setSliderFromProperty( vslider, nseLbl, "SDL.Sound.NoiseVolume" );

	vbox2->addWidget( nseLbl  );
	vbox2->addWidget( vslider );
	frame->setLayout( vbox2   );
	hbox2->addWidget( frame   );

	connect(vslider, SIGNAL(valueChanged(int)), this, SLOT(noiseChanged(int)) );

	frame   = new QGroupBox(tr("PCM"));
	vbox2   = new QVBoxLayout();
	pcmLbl  = new QLabel("255");
	vslider = new QSlider( Qt::Vertical );
	vslider->setMinimum(  0);
	vslider->setMaximum(255);
	setSliderFromProperty( vslider, pcmLbl, "SDL.Sound.PCMVolume" );

	vbox2->addWidget( pcmLbl  );
	vbox2->addWidget( vslider );
	frame->setLayout( vbox2   );
	hbox2->addWidget( frame   );

	connect(vslider, SIGNAL(valueChanged(int)), this, SLOT(pcmChanged(int)) );

	// Set Final Layout
	setLayout( hbox1 );
}
//----------------------------------------------------
ConsoleSndConfDialog_t::~ConsoleSndConfDialog_t(void)
{
   printf("Destroy Sound Config Window\n");
}
//----------------------------------------------------------------------------
void ConsoleSndConfDialog_t::closeEvent(QCloseEvent *event)
{
   printf("Sound Config Close Window Event\n");
   done(0);
	deleteLater();
   event->accept();
}
//----------------------------------------------------------------------------
void ConsoleSndConfDialog_t::closeWindow(void)
{
   //printf("Sound Close Window\n");
   done(0);
	deleteLater();
}
//----------------------------------------------------
void  ConsoleSndConfDialog_t::setCheckBoxFromProperty( QCheckBox *cbx, const char *property )
{
	int  pval;
	g_config->getOption (property, &pval);

	cbx->setCheckState( pval ? Qt::Checked : Qt::Unchecked );
}
//----------------------------------------------------
void  ConsoleSndConfDialog_t::setComboBoxFromProperty( QComboBox *cbx, const char *property )
{
	int  i, pval;
	g_config->getOption (property, &pval);

	for (i=0; i<cbx->count(); i++)
	{
		if ( pval == cbx->itemData(i).toInt() )
		{
			cbx->setCurrentIndex(i); break;
		}
	}
}
//----------------------------------------------------
void  ConsoleSndConfDialog_t::setSliderFromProperty( QSlider *slider, QLabel *lbl, const char *property )
{
	int  pval;
	char stmp[32];
	g_config->getOption (property, &pval);
	slider->setValue( pval );
	sprintf( stmp, "%i", pval );
	lbl->setText( stmp );
}
//----------------------------------------------------
void ConsoleSndConfDialog_t::bufSizeChanged(int value)
{
	char stmp[32];

	sprintf( stmp, "%i", value );

	bufSizeLabel->setText(stmp);

	g_config->setOption ("SDL.Sound.BufSize", value);
	// reset sound subsystem for changes to take effect
	if ( fceuWrapperTryLock() )
	{
		KillSound ();
		InitSound ();
		fceuWrapperUnLock();
	}
}
//----------------------------------------------------
void ConsoleSndConfDialog_t::volumeChanged(int value)
{
	char stmp[32];

	sprintf( stmp, "%i", value );

	volLbl->setText(stmp);

	g_config->setOption ("SDL.Sound.Volume", value);

	if ( fceuWrapperTryLock() )
	{
		FCEUI_SetSoundVolume (value);
		fceuWrapperUnLock();
	}
}
//----------------------------------------------------
void ConsoleSndConfDialog_t::triangleChanged(int value)
{
	char stmp[32];

	sprintf( stmp, "%i", value );

	triLbl->setText(stmp);

	g_config->setOption ("SDL.Sound.TriangleVolume", value);

	if ( fceuWrapperTryLock() )
	{
		FCEUI_SetTriangleVolume (value);
		fceuWrapperUnLock();
	}
}
//----------------------------------------------------
void ConsoleSndConfDialog_t::square1Changed(int value)
{
	char stmp[32];

	sprintf( stmp, "%i", value );

	sqr1Lbl->setText(stmp);

	g_config->setOption ("SDL.Sound.Square1Volume", value);

	if ( fceuWrapperTryLock() )
	{
		FCEUI_SetSquare1Volume (value);
		fceuWrapperUnLock();
	}
}
//----------------------------------------------------
void ConsoleSndConfDialog_t::square2Changed(int value)
{
	char stmp[32];

	sprintf( stmp, "%i", value );

	sqr2Lbl->setText(stmp);

	g_config->setOption ("SDL.Sound.Square2Volume", value);

	if ( fceuWrapperTryLock() )
	{
		FCEUI_SetSquare2Volume (value);
		fceuWrapperUnLock();
	}
}
//----------------------------------------------------
void ConsoleSndConfDialog_t::noiseChanged(int value)
{
	char stmp[32];

	sprintf( stmp, "%i", value );

	nseLbl->setText(stmp);

	g_config->setOption ("SDL.Sound.NoiseVolume", value);

	if ( fceuWrapperTryLock() )
	{
		FCEUI_SetNoiseVolume (value);
		fceuWrapperUnLock();
	}
}
//----------------------------------------------------
void ConsoleSndConfDialog_t::pcmChanged(int value)
{
	char stmp[32];

	sprintf( stmp, "%i", value );

	pcmLbl->setText(stmp);

	g_config->setOption ("SDL.Sound.PCMVolume", value);

	if ( fceuWrapperTryLock() )
	{
		FCEUI_SetPCMVolume (value);
		fceuWrapperUnLock();
	}
}
//----------------------------------------------------
void ConsoleSndConfDialog_t::enaSoundStateChange(int value)
{
	if ( value )
	{
		int last_soundopt;
		g_config->getOption ("SDL.Sound", &last_soundopt);
		g_config->setOption ("SDL.Sound", 1);

		fceuWrapperLock();

		if (GameInfo && !last_soundopt)
		{
			InitSound ();
		}
		fceuWrapperUnLock();
	}
	else
	{
		g_config->setOption ("SDL.Sound", 0);

		fceuWrapperLock();
		KillSound ();
		fceuWrapperUnLock();
	}
}
//----------------------------------------------------
void ConsoleSndConfDialog_t::enaSoundLowPassChange(int value)
{
	if (value)
	{
		g_config->setOption ("SDL.Sound.LowPass", 1);

		fceuWrapperLock();
		FCEUI_SetLowPass (1);
		fceuWrapperUnLock();
	}
	else
	{
		g_config->setOption ("SDL.Sound.LowPass", 0);

		fceuWrapperLock();
		FCEUI_SetLowPass (0);
		fceuWrapperUnLock();
	}
	g_config->save ();
}
//----------------------------------------------------
void ConsoleSndConfDialog_t::swapDutyCallback(int value)
{
	if (value)
	{
		g_config->setOption ("SDL.SwapDuty", 1);
		swapDuty = 1;
	}
	else
	{
		g_config->setOption ("SDL.SwapDuty", 0);
		swapDuty = 0;
	}
	g_config->save ();
}
//----------------------------------------------------
void ConsoleSndConfDialog_t::soundQualityChanged(int index)
{
	//printf("Sound Quality: %i : %i \n", index, qualitySelect->itemData(index).toInt() );

	g_config->setOption ("SDL.Sound.Quality", qualitySelect->itemData(index).toInt() );

	// reset sound subsystem for changes to take effect
	if ( fceuWrapperTryLock() )
	{
		KillSound ();
		InitSound ();
		fceuWrapperUnLock();
	}
	g_config->save ();
}
//----------------------------------------------------
void ConsoleSndConfDialog_t::soundRateChanged(int index)
{
	//printf("Sound Rate: %i : %i \n", index, rateSelect->itemData(index).toInt() );

	g_config->setOption ("SDL.Sound.Rate", rateSelect->itemData(index).toInt() );
	// reset sound subsystem for changes to take effect
	if ( fceuWrapperTryLock() )
	{
		KillSound ();
		InitSound ();
		fceuWrapperUnLock();
	}
	g_config->save ();
}
//----------------------------------------------------
