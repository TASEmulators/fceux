// GameSoundConf.cpp
//
#include "../../fceu.h"
#include "../../driver.h"
#include "Qt/GameSoundConf.h"
#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/config.h"
#include "Qt/fceuWrapper.h"

//----------------------------------------------------
GameSndConfDialog_t::GameSndConfDialog_t(QWidget *parent)
	: QDialog( parent )
{
	int buf;
	QHBoxLayout *hbox1, *hbox2;
	QVBoxLayout *vbox1, *vbox2;
	QLabel *lbl;
	QGroupBox *frame;
	QSlider *vslider;

	setWindowTitle("Sound Config");

	hbox1 = new QHBoxLayout();
	vbox1 = new QVBoxLayout();

	// Enable Sound Select
	enaChkbox  = new QCheckBox("Enable Sound");
	// Enable Low Pass Filter Select
	enaLowPass = new QCheckBox("Enable Low Pass Filter");

	setCheckBoxFromProperty( enaChkbox , "SDL.Sound" );
	setCheckBoxFromProperty( enaLowPass, "SDL.Sound.LowPass" );

	connect(enaChkbox , SIGNAL(stateChanged(int)), this, SLOT(enaSoundStateChange(int)) );
	connect(enaLowPass, SIGNAL(stateChanged(int)), this, SLOT(enaSoundLowPassChange(int)) );

	vbox1->addWidget( enaChkbox  );
	vbox1->addWidget( enaLowPass );

	// Audio Quality Select
	hbox2 = new QHBoxLayout();

	lbl = new QLabel("Quality:");

	qualitySelect = new QComboBox();

	qualitySelect->addItem( tr("Low")      , 0 );
   qualitySelect->addItem( tr("High")     , 1 );
   qualitySelect->addItem( tr("Very High"), 2 );

	setComboBoxFromProperty( qualitySelect, "SDL.Sound.Quality" );

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

	g_config->getOption ("SDL.Sound.Rate", &buf);

	hbox2->addWidget( lbl );
	hbox2->addWidget( rateSelect );

	vbox1->addLayout( hbox2 );

	// Buffer Size Select
	//
	hbox2 = new QHBoxLayout();

	lbl = new QLabel("Buffer Size (in ms):");

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
	swapDutyChkbox  = new QCheckBox("Swap Duty Cycles");
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
GameSndConfDialog_t::~GameSndConfDialog_t(void)
{

}
//----------------------------------------------------
void  GameSndConfDialog_t::setCheckBoxFromProperty( QCheckBox *cbx, const char *property )
{
	int  pval;
	g_config->getOption (property, &pval);

	cbx->setCheckState( pval ? Qt::Checked : Qt::Unchecked );
}
//----------------------------------------------------
void  GameSndConfDialog_t::setComboBoxFromProperty( QComboBox *cbx, const char *property )
{
	int  i, pval;
	g_config->getOption (property, &pval);

	for (i=0; i<cbx->count(); i++)
	{
		if ( pval == cbx->itemData(i) )
		{
			cbx->setCurrentIndex(i); break;
		}
	}
}
//----------------------------------------------------
void  GameSndConfDialog_t::setSliderFromProperty( QSlider *slider, QLabel *lbl, const char *property )
{
	int  pval;
	char stmp[32];
	g_config->getOption (property, &pval);
	slider->setValue( pval );
	sprintf( stmp, "%i", pval );
	lbl->setText( stmp );
}
//----------------------------------------------------
void GameSndConfDialog_t::bufSizeChanged(int value)
{
	char stmp[32];

	sprintf( stmp, "%i", value );

	bufSizeLabel->setText(stmp);

	g_config->setOption ("SDL.Sound.BufSize", value);
	// reset sound subsystem for changes to take effect
	KillSound ();
	InitSound ();
}
//----------------------------------------------------
void GameSndConfDialog_t::volumeChanged(int value)
{
	char stmp[32];

	sprintf( stmp, "%i", value );

	volLbl->setText(stmp);

	g_config->setOption ("SDL.Sound.Volume", value);
	FCEUI_SetSoundVolume (value);
}
//----------------------------------------------------
void GameSndConfDialog_t::triangleChanged(int value)
{
	char stmp[32];

	sprintf( stmp, "%i", value );

	triLbl->setText(stmp);

	g_config->setOption ("SDL.Sound.TriangleVolume", value);
	FCEUI_SetTriangleVolume (value);
}
//----------------------------------------------------
void GameSndConfDialog_t::square1Changed(int value)
{
	char stmp[32];

	sprintf( stmp, "%i", value );

	sqr1Lbl->setText(stmp);

	g_config->setOption ("SDL.Sound.Square1Volume", value);
	FCEUI_SetSquare1Volume (value);
}
//----------------------------------------------------
void GameSndConfDialog_t::square2Changed(int value)
{
	char stmp[32];

	sprintf( stmp, "%i", value );

	sqr2Lbl->setText(stmp);

	g_config->setOption ("SDL.Sound.Square2Volume", value);
	FCEUI_SetSquare2Volume (value);
}
//----------------------------------------------------
void GameSndConfDialog_t::noiseChanged(int value)
{
	char stmp[32];

	sprintf( stmp, "%i", value );

	nseLbl->setText(stmp);

	g_config->setOption ("SDL.Sound.NoiseVolume", value);
	FCEUI_SetNoiseVolume (value);
}
//----------------------------------------------------
void GameSndConfDialog_t::pcmChanged(int value)
{
	char stmp[32];

	sprintf( stmp, "%i", value );

	pcmLbl->setText(stmp);

	g_config->setOption ("SDL.Sound.PCMVolume", value);
	FCEUI_SetPCMVolume (value);
}
//----------------------------------------------------
void GameSndConfDialog_t::enaSoundStateChange(int value)
{
	if ( value )
	{
		int last_soundopt;
		g_config->getOption ("SDL.Sound", &last_soundopt);
		g_config->setOption ("SDL.Sound", 1);
		if (GameInfo && !last_soundopt)
		{
			InitSound ();
		}
	}
	else
	{
		g_config->setOption ("SDL.Sound", 0);
		KillSound ();
	}
}
//----------------------------------------------------
void GameSndConfDialog_t::enaSoundLowPassChange(int value)
{
	if (value)
	{
		g_config->setOption ("SDL.Sound.LowPass", 1);
		FCEUI_SetLowPass (1);
	}
	else
	{
		g_config->setOption ("SDL.Sound.LowPass", 0);
		FCEUI_SetLowPass (0);
	}
	g_config->save ();
}
//----------------------------------------------------
void GameSndConfDialog_t::swapDutyCallback(int value)
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
