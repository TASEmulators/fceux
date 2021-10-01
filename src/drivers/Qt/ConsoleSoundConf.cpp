/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020 mjbudd77
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
// ConsoleSoundConf.cpp
//
#include <QCloseEvent>

#include "../../fceu.h"
#include "../../driver.h"
#include "Qt/ConsoleWindow.h"
#include "Qt/ConsoleSoundConf.h"
#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/config.h"
#include "Qt/nes_shm.h"
#include "Qt/fceuWrapper.h"

//----------------------------------------------------
ConsoleSndConfDialog_t::ConsoleSndConfDialog_t(QWidget *parent)
	: QDialog(parent)
{
	int buf;
	QHBoxLayout *hbox, *hbox1, *hbox2;
	QVBoxLayout *mainLayout, *vbox1, *vbox2;
	QPushButton *closeButton;
	QPushButton *resetCountBtn;
	QLabel *lbl;
	QGroupBox *frame;
	QSlider *vslider;

	sndQuality = 1;

	setWindowTitle(tr("Sound Config"));

	mainLayout = new QVBoxLayout();
	hbox1 = new QHBoxLayout();
	vbox1 = new QVBoxLayout();

	// Enable Sound Select
	enaChkbox = new QCheckBox(tr("Enable Sound"));
	// Enable Low Pass Filter Select
	enaLowPass = new QCheckBox(tr("Enable Low Pass Filter"));

	setCheckBoxFromProperty(enaChkbox, "SDL.Sound");
	setCheckBoxFromProperty(enaLowPass, "SDL.Sound.LowPass");

	connect(enaChkbox, SIGNAL(stateChanged(int)), this, SLOT(enaSoundStateChange(int)));
	connect(enaLowPass, SIGNAL(stateChanged(int)), this, SLOT(enaSoundLowPassChange(int)));

	vbox1->addWidget(enaChkbox);
	vbox1->addWidget(enaLowPass);

	// Audio Quality Select
	hbox2 = new QHBoxLayout();

	lbl = new QLabel(tr("Quality:"));

	qualitySelect = new QComboBox();

	qualitySelect->addItem(tr("Low"), 0);
	qualitySelect->addItem(tr("High"), 1);
	qualitySelect->addItem(tr("Very High"), 2);

	setComboBoxFromProperty(qualitySelect, "SDL.Sound.Quality");
	g_config->getOption("SDL.Sound.Quality", &sndQuality );

	connect(qualitySelect, SIGNAL(currentIndexChanged(int)), this, SLOT(soundQualityChanged(int)));

	hbox2->addWidget(lbl);
	hbox2->addWidget(qualitySelect);

	vbox1->addLayout(hbox2);

	// Sample Rate Select
	hbox2 = new QHBoxLayout();

	lbl = new QLabel("Rate:");

	rateSelect = new QComboBox();

	rateSelect->addItem(tr("11025"), 11025);
	rateSelect->addItem(tr("22050"), 22050);
	rateSelect->addItem(tr("44100"), 44100);
	rateSelect->addItem(tr("48000"), 48000);
	rateSelect->addItem(tr("96000"), 96000);

	setComboBoxFromProperty(rateSelect, "SDL.Sound.Rate");

	connect(rateSelect, SIGNAL(currentIndexChanged(int)), this, SLOT(soundRateChanged(int)));

	g_config->getOption("SDL.Sound.Rate", &buf);

	hbox2->addWidget(lbl);
	hbox2->addWidget(rateSelect);

	vbox1->addLayout(hbox2);

	// Buffer Size Select
	//
	hbox2 = new QHBoxLayout();

	lbl = new QLabel(tr("Buffer Size (in ms):"));

	bufSizeLabel = new QLabel("128");
	bufSizeSlider = new QSlider(Qt::Horizontal);

	bufSizeSlider->setMinimum(15);
	bufSizeSlider->setMaximum(200);
	//bufSizeSlider->setSliderPosition(128);
	setSliderFromProperty(bufSizeSlider, bufSizeLabel, "SDL.Sound.BufSize");

	hbox2->addWidget(lbl);
	hbox2->addWidget(bufSizeLabel);

	vbox1->addLayout(hbox2);
	vbox1->addWidget(bufSizeSlider);

	connect(bufSizeSlider, SIGNAL(valueChanged(int)), this, SLOT(bufSizeChanged(int)));

	bufUsage = new QProgressBar();
	bufUsage->setToolTip( tr("% use of audio samples FIFO buffer.\n\nThe emulation thread fills the buffer and the audio thread drains it.") );
	bufUsage->setOrientation( Qt::Horizontal );
	bufUsage->setMinimum(   0 );
	bufUsage->setMaximum( 100 );
	bufUsage->setValue( 0 );
	vbox1->addWidget(bufUsage);

	// Use Global Focus
	useGlobalFocus = new QCheckBox(tr("Use Global Focus"));
	useGlobalFocus->setToolTip( tr("Mute sound when window is not in focus") );
	vbox1->addWidget(useGlobalFocus);

	setCheckBoxFromProperty(useGlobalFocus, "SDL.Sound.UseGlobalFocus");

	connect(useGlobalFocus, SIGNAL(stateChanged(int)), this, SLOT(useGlobalFocusChanged(int)));

	// Swap Duty Cycles
	swapDutyChkbox = new QCheckBox(tr("Swap Duty Cycles"));
	vbox1->addWidget(swapDutyChkbox);

	setCheckBoxFromProperty(swapDutyChkbox, "SDL.SwapDuty");

	connect(swapDutyChkbox, SIGNAL(stateChanged(int)), this, SLOT(swapDutyCallback(int)));

	hbox1->addLayout(vbox1);

	frame = new QGroupBox(tr("Mixer:"));
	hbox2 = new QHBoxLayout();

	frame->setLayout(hbox2);

	hbox1->addWidget(frame);

	frame = new QGroupBox(tr("Volume"));
	vbox2 = new QVBoxLayout();
	volLbl = new QLabel("150");
	vslider = new QSlider(Qt::Vertical);
	vslider->setMinimum(0);
	vslider->setMaximum(255);
	setSliderFromProperty(vslider, volLbl, "SDL.Sound.Volume");

	vbox2->addWidget(volLbl);
	vbox2->addWidget(vslider);
	frame->setLayout(vbox2);
	hbox2->addWidget(frame);

	connect(vslider, SIGNAL(valueChanged(int)), this, SLOT(volumeChanged(int)));

	frame = new QGroupBox(tr("Triangle"));
	vbox2 = new QVBoxLayout();
	triLbl = new QLabel("255");
	vslider = new QSlider(Qt::Vertical);
	vslider->setMinimum(0);
	vslider->setMaximum(255);
	setSliderFromProperty(vslider, triLbl, "SDL.Sound.TriangleVolume");

	vbox2->addWidget(triLbl);
	vbox2->addWidget(vslider);
	frame->setLayout(vbox2);
	hbox2->addWidget(frame);

	connect(vslider, SIGNAL(valueChanged(int)), this, SLOT(triangleChanged(int)));

	frame = new QGroupBox(tr("Square1"));
	vbox2 = new QVBoxLayout();
	sqr1Lbl = new QLabel("255");
	vslider = new QSlider(Qt::Vertical);
	vslider->setMinimum(0);
	vslider->setMaximum(255);
	setSliderFromProperty(vslider, sqr1Lbl, "SDL.Sound.Square1Volume");

	vbox2->addWidget(sqr1Lbl);
	vbox2->addWidget(vslider);
	frame->setLayout(vbox2);
	hbox2->addWidget(frame);

	connect(vslider, SIGNAL(valueChanged(int)), this, SLOT(square1Changed(int)));

	frame = new QGroupBox(tr("Square2"));
	vbox2 = new QVBoxLayout();
	sqr2Lbl = new QLabel("255");
	sqr2Slider = new QSlider(Qt::Vertical);
	sqr2Slider->setMinimum(0);
	sqr2Slider->setMaximum(255);
	setSliderFromProperty(sqr2Slider, sqr2Lbl, "SDL.Sound.Square2Volume");

	vbox2->addWidget(sqr2Lbl);
	vbox2->addWidget(sqr2Slider);
	frame->setLayout(vbox2);
	hbox2->addWidget(frame);

	connect(sqr2Slider, SIGNAL(valueChanged(int)), this, SLOT(square2Changed(int)));

	frame = new QGroupBox(tr("Noise"));
	vbox2 = new QVBoxLayout();
	nseLbl = new QLabel("255");
	nseSlider = new QSlider(Qt::Vertical);
	nseSlider->setMinimum(0);
	nseSlider->setMaximum(255);
	setSliderFromProperty(nseSlider, nseLbl, "SDL.Sound.NoiseVolume");

	vbox2->addWidget(nseLbl);
	vbox2->addWidget(nseSlider);
	frame->setLayout(vbox2);
	hbox2->addWidget(frame);

	connect(nseSlider, SIGNAL(valueChanged(int)), this, SLOT(noiseChanged(int)));

	frame = new QGroupBox(tr("PCM"));
	vbox2 = new QVBoxLayout();
	pcmLbl = new QLabel("255");
	pcmSlider = new QSlider(Qt::Vertical);
	pcmSlider->setMinimum(0);
	pcmSlider->setMaximum(255);
	setSliderFromProperty(pcmSlider, pcmLbl, "SDL.Sound.PCMVolume");

	vbox2->addWidget(pcmLbl);
	vbox2->addWidget(pcmSlider);
	frame->setLayout(vbox2);
	hbox2->addWidget(frame);

	connect(pcmSlider, SIGNAL(valueChanged(int)), this, SLOT(pcmChanged(int)));

	closeButton = new QPushButton( tr("Close") );
	closeButton->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
	connect(closeButton, SIGNAL(clicked(void)), this, SLOT(closeWindow(void)));

	starveLbl = new QLabel( tr("Sink Starve Count:") );
	starveLbl->setToolTip( tr("Running count of the number of samples that the audio sink is starved of.") );
	resetCountBtn = new QPushButton( tr("Reset Counter") );
	resetCountBtn->setIcon(style()->standardIcon(QStyle::SP_DialogResetButton));
	connect(resetCountBtn, SIGNAL(clicked(void)), this, SLOT(resetCounters(void)));

	hbox = new QHBoxLayout();
	hbox->addWidget(resetCountBtn, 1);
	hbox->addWidget(starveLbl,1);
	hbox->addStretch(5);
	hbox->addWidget( closeButton, 1 );

	mainLayout->addLayout(hbox1);
	mainLayout->addLayout( hbox );

	// Set Final Layout
	setLayout(mainLayout);

	setSliderEnables();

	updateTimer = new QTimer(this);
	connect( updateTimer, &QTimer::timeout, this, &ConsoleSndConfDialog_t::periodicUpdate );
	updateTimer->start(1000);
}
//----------------------------------------------------
ConsoleSndConfDialog_t::~ConsoleSndConfDialog_t(void)
{
	printf("Destroy Sound Config Window\n");
	updateTimer->stop();
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
void ConsoleSndConfDialog_t::resetCounters(void)
{
	nes_shm->sndBuf.starveCounter = 0;

	periodicUpdate();
}
//----------------------------------------------------
void ConsoleSndConfDialog_t::periodicUpdate(void)
{
	uint32_t c, m;
	double percBufUse;
	char stmp[64];

	c = GetWriteSound();
	m = GetMaxSound();

	percBufUse = 100.0f - (100.0f * (double)c / (double)m );

	bufUsage->setValue( (int)(percBufUse) );

	sprintf( stmp, "Sink Starve Count: %u", nes_shm->sndBuf.starveCounter );

	starveLbl->setText( tr(stmp) );
}
//----------------------------------------------------
void ConsoleSndConfDialog_t::setSliderEnables(void)
{
	// For now, always leave all sliders enabled.
	// Until we figure out how low qualtiy sound uses these
	//if ( sndQuality == 0 )
	//{
	//	sqr2Lbl->setEnabled(false);
	//	nseLbl->setEnabled(false);
	//	pcmLbl->setEnabled(false);
	//	sqr2Slider->setEnabled(false);
	//	nseSlider->setEnabled(false);
	//	pcmSlider->setEnabled(false);
	//}
	//else
	//{
	//	sqr2Lbl->setEnabled(true);
	//	nseLbl->setEnabled(true);
	//	pcmLbl->setEnabled(true);
	//	sqr2Slider->setEnabled(true);
	//	nseSlider->setEnabled(true);
	//	pcmSlider->setEnabled(true);
	//}

}
//----------------------------------------------------
void ConsoleSndConfDialog_t::setCheckBoxFromProperty(QCheckBox *cbx, const char *property)
{
	int pval;
	g_config->getOption(property, &pval);

	cbx->setCheckState(pval ? Qt::Checked : Qt::Unchecked);
}
//----------------------------------------------------
void ConsoleSndConfDialog_t::setComboBoxFromProperty(QComboBox *cbx, const char *property)
{
	int i, pval;
	g_config->getOption(property, &pval);

	for (i = 0; i < cbx->count(); i++)
	{
		if (pval == cbx->itemData(i).toInt())
		{
			cbx->setCurrentIndex(i);
			break;
		}
	}
}
//----------------------------------------------------
void ConsoleSndConfDialog_t::setSliderFromProperty(QSlider *slider, QLabel *lbl, const char *property)
{
	int pval;
	char stmp[32];
	g_config->getOption(property, &pval);
	slider->setValue(pval);
	sprintf(stmp, "%i", pval);
	lbl->setText(stmp);
}
//----------------------------------------------------
void ConsoleSndConfDialog_t::bufSizeChanged(int value)
{
	char stmp[32];

	sprintf(stmp, "%i", value);

	bufSizeLabel->setText(stmp);

	g_config->setOption("SDL.Sound.BufSize", value);
	// reset sound subsystem for changes to take effect
	if (fceuWrapperTryLock())
	{
		KillSound();
		InitSound();
		fceuWrapperUnLock();
	}
}
//----------------------------------------------------
void ConsoleSndConfDialog_t::volumeChanged(int value)
{
	char stmp[32];

	sprintf(stmp, "%i", value);

	volLbl->setText(stmp);

	g_config->setOption("SDL.Sound.Volume", value);

	if (fceuWrapperTryLock())
	{
		FCEUI_SetSoundVolume(value);
		fceuWrapperUnLock();
	}
}
//----------------------------------------------------
void ConsoleSndConfDialog_t::triangleChanged(int value)
{
	char stmp[32];

	sprintf(stmp, "%i", value);

	triLbl->setText(stmp);

	g_config->setOption("SDL.Sound.TriangleVolume", value);

	if (fceuWrapperTryLock())
	{
		FCEUI_SetTriangleVolume(value);
		fceuWrapperUnLock();
	}
}
//----------------------------------------------------
void ConsoleSndConfDialog_t::square1Changed(int value)
{
	char stmp[32];

	sprintf(stmp, "%i", value);

	sqr1Lbl->setText(stmp);

	g_config->setOption("SDL.Sound.Square1Volume", value);

	if (fceuWrapperTryLock())
	{
		FCEUI_SetSquare1Volume(value);
		fceuWrapperUnLock();
	}
}
//----------------------------------------------------
void ConsoleSndConfDialog_t::square2Changed(int value)
{
	char stmp[32];

	sprintf(stmp, "%i", value);

	sqr2Lbl->setText(stmp);

	g_config->setOption("SDL.Sound.Square2Volume", value);

	if (fceuWrapperTryLock())
	{
		FCEUI_SetSquare2Volume(value);
		fceuWrapperUnLock();
	}
}
//----------------------------------------------------
void ConsoleSndConfDialog_t::noiseChanged(int value)
{
	char stmp[32];

	sprintf(stmp, "%i", value);

	nseLbl->setText(stmp);

	g_config->setOption("SDL.Sound.NoiseVolume", value);

	if (fceuWrapperTryLock())
	{
		FCEUI_SetNoiseVolume(value);
		fceuWrapperUnLock();
	}
}
//----------------------------------------------------
void ConsoleSndConfDialog_t::pcmChanged(int value)
{
	char stmp[32];

	sprintf(stmp, "%i", value);

	pcmLbl->setText(stmp);

	g_config->setOption("SDL.Sound.PCMVolume", value);

	if (fceuWrapperTryLock())
	{
		FCEUI_SetPCMVolume(value);
		fceuWrapperUnLock();
	}
}
//----------------------------------------------------
void ConsoleSndConfDialog_t::enaSoundStateChange(int value)
{
	if (value)
	{
		int last_soundopt;
		g_config->getOption("SDL.Sound", &last_soundopt);
		g_config->setOption("SDL.Sound", 1);

		fceuWrapperLock();

		if (GameInfo && !last_soundopt)
		{
			InitSound();
		}
		fceuWrapperUnLock();
	}
	else
	{
		g_config->setOption("SDL.Sound", 0);

		fceuWrapperLock();
		KillSound();
		fceuWrapperUnLock();
	}
}
//----------------------------------------------------
void ConsoleSndConfDialog_t::enaSoundLowPassChange(int value)
{
	if (value)
	{
		g_config->setOption("SDL.Sound.LowPass", 1);

		fceuWrapperLock();
		FCEUI_SetLowPass(1);
		fceuWrapperUnLock();
	}
	else
	{
		g_config->setOption("SDL.Sound.LowPass", 0);

		fceuWrapperLock();
		FCEUI_SetLowPass(0);
		fceuWrapperUnLock();
	}
	g_config->save();
}
//----------------------------------------------------
void ConsoleSndConfDialog_t::useGlobalFocusChanged(int value)
{
	bool bval = value != Qt::Unchecked;

	//printf("SDL.Sound.UseGlobalFocus = %i\n", bval);
	g_config->setOption("SDL.Sound.UseGlobalFocus", bval);
	g_config->save();

	if ( consoleWindow )
	{
		consoleWindow->setSoundUseGlobalFocus(bval);
	}
}
//----------------------------------------------------
void ConsoleSndConfDialog_t::swapDutyCallback(int value)
{
	if (value)
	{
		g_config->setOption("SDL.SwapDuty", 1);
		swapDuty = 1;
	}
	else
	{
		g_config->setOption("SDL.SwapDuty", 0);
		swapDuty = 0;
	}
	g_config->save();
}
//----------------------------------------------------
void ConsoleSndConfDialog_t::soundQualityChanged(int index)
{
	//printf("Sound Quality: %i : %i \n", index, qualitySelect->itemData(index).toInt() );

	g_config->setOption("SDL.Sound.Quality", qualitySelect->itemData(index).toInt());
	g_config->getOption("SDL.Sound.Quality", &sndQuality );

	// reset sound subsystem for changes to take effect
	if (fceuWrapperTryLock())
	{
		KillSound();
		InitSound();
		fceuWrapperUnLock();
	}
	g_config->save();

	setSliderEnables();
}
//----------------------------------------------------
void ConsoleSndConfDialog_t::soundRateChanged(int index)
{
	//printf("Sound Rate: %i : %i \n", index, rateSelect->itemData(index).toInt() );

	g_config->setOption("SDL.Sound.Rate", rateSelect->itemData(index).toInt());
	// reset sound subsystem for changes to take effect
	if (fceuWrapperTryLock())
	{
		KillSound();
		InitSound();
		fceuWrapperUnLock();
	}
	g_config->save();
}
//----------------------------------------------------
