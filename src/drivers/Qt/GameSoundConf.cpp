// GameSoundConf.cpp
//
#include "Qt/GameSoundConf.h"
#include "Qt/main.h"
#include "Qt/config.h"
#include "Qt/fceuWrapper.h"

//----------------------------------------------------
GameSndConfDialog_t::GameSndConfDialog_t(QWidget *parent)
	: QDialog( parent )
{
	QHBoxLayout *hbox1, *hbox2;
	QVBoxLayout *vbox1;
	QLabel *lbl;

	setWindowTitle("Sound Config");

	hbox1 = new QHBoxLayout();
	vbox1 = new QVBoxLayout();

	// Enable Sound Select
	enaChkbox  = new QCheckBox("Enable Sound");
	// Enable Low Pass Filter Select
	enaLowPass = new QCheckBox("Enable Low Pass Filter");

	vbox1->addWidget( enaChkbox  );
	vbox1->addWidget( enaLowPass );

	// Audio Quality Select
	hbox2 = new QHBoxLayout();

	lbl = new QLabel("Quality:");

	qualitySelect = new QComboBox();

	qualitySelect->addItem( tr("Low")      , 0 );
   qualitySelect->addItem( tr("High")     , 1 );
   qualitySelect->addItem( tr("Very High"), 2 );

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

	hbox2->addWidget( lbl );
	hbox2->addWidget( rateSelect );

	vbox1->addLayout( hbox2 );

	// Buffer Size Select
	//
	lbl = new QLabel("Buffer Size (in ms)");

	bufSizeSlider = new QSlider( Qt::Horizontal );

	bufSizeSlider->setMinimum( 15);
	bufSizeSlider->setMaximum(200);
	bufSizeSlider->setSliderPosition(128);

	vbox1->addWidget( lbl );
	vbox1->addWidget( bufSizeSlider );
	
	// Swap Duty Cycles
	swapDutyChkbox  = new QCheckBox("Swap Duty Cycles");
	vbox1->addWidget( swapDutyChkbox );

	hbox1->addLayout( vbox1 );

	// Set Final Layout
	setLayout( hbox1 );
}
//----------------------------------------------------
GameSndConfDialog_t::~GameSndConfDialog_t(void)
{

}
//----------------------------------------------------
