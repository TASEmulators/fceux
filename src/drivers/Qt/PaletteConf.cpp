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
//
// PaletteConf.cpp
//
#include <QFileDialog>
#include <QTextEdit>

#include "Qt/PaletteConf.h"
#include "Qt/main.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/nes_shm.h"
#include "Qt/keyscan.h"
#include "Qt/fceuWrapper.h"
#include "Qt/ConsoleUtilities.h"

#include "../../ppu.h"

extern bool force_grayscale;
extern bool palupdate;
extern  int palnotch;
extern  int palsaturation;
extern  int palsharpness;
extern  int palcontrast;
extern  int palbrightness;

static const char *commentText =
	"Palette Selection uses the 1st Matching Condition:\n\
   1. Game type is NSF (always uses fixed palette) \n\
   2. Custom User Palette is Available and Enabled \n\
   3. NTSC Color Emulation is Enabled \n\
   4. Individual Game Palette is Available \n\
   5. Default Built-in Palette   ";
//----------------------------------------------------
PaletteConfDialog_t::PaletteConfDialog_t(QWidget *parent)
	: QDialog(parent)
{
	QVBoxLayout *mainLayout, *vbox;
	QHBoxLayout *hbox1, *hbox2, *hbox;
	QGridLayout *grid;
	QGroupBox *frame;
	QPushButton *closeButton;
	QPushButton *button;
	QTextEdit *comments;
	QStyle *style;
	int hue, tint;
	char stmp[64];
	std::string paletteFile;

	style = this->style();

	resize(512, 512);

	// sync with config
	g_config->getOption("SDL.Hue", &hue);
	g_config->getOption("SDL.Tint", &tint);

	setWindowTitle(tr("Palette Config"));

	mainLayout = new QVBoxLayout();

	frame = new QGroupBox(tr("Custom Palette:"));
	vbox = new QVBoxLayout();
	hbox1 = new QHBoxLayout();
	hbox2 = new QHBoxLayout();

	useCustom = new QCheckBox(tr("Use Custom Palette"));
	GrayScale = new QCheckBox(tr("Force Grayscale"));
	deemphSwap = new QCheckBox(tr("De-emphasis Bit Swap"));

	hbox1->addWidget(useCustom, 50);

	useCustom->setChecked(FCEUI_GetUserPaletteAvail());
	GrayScale->setChecked(force_grayscale);
	deemphSwap->setChecked(paldeemphswap);

	connect(useCustom, SIGNAL(stateChanged(int)), this, SLOT(use_Custom_Changed(int)));
	connect(GrayScale, SIGNAL(stateChanged(int)), this, SLOT(force_GrayScale_Changed(int)));
	connect(deemphSwap, SIGNAL(stateChanged(int)), this, SLOT(deemphswap_Changed(int)));

	button = new QPushButton(tr("Load"));
	button->setIcon(style->standardIcon(QStyle::SP_FileDialogStart));
	//hbox1->addStretch(10);
	hbox1->addWidget(button, 25);

	connect(button, SIGNAL(clicked(void)), this, SLOT(openPaletteFile(void)));

	g_config->getOption("SDL.Palette", &paletteFile);

	custom_palette_path = new QLineEdit();
	custom_palette_path->setReadOnly(true);
	custom_palette_path->setClearButtonEnabled(false);
	custom_palette_path->setText(paletteFile.c_str());

	hbox = new QHBoxLayout();
	hbox->addWidget(GrayScale);
	hbox->addWidget(deemphSwap);

	hbox2->addWidget( custom_palette_path );
	//vbox->addWidget(useCustom);
	vbox->addLayout(hbox1);
	vbox->addLayout(hbox2);
	vbox->addLayout(hbox);

	button = new QPushButton(tr("Clear"));
	button->setIcon(style->standardIcon(QStyle::SP_LineEditClearButton));
	hbox1->addWidget(button, 25);
	//hbox2->addWidget(button);

	connect(button, SIGNAL(clicked(void)), this, SLOT(clearPalette(void)));

	frame->setLayout(vbox);

	mainLayout->addWidget(frame);

	ntscFrame = new QGroupBox(tr("NTSC Palette Control:"));
	ntscFrame->setCheckable(true);

	vbox  = new QVBoxLayout();
	hbox2 = new QHBoxLayout();

	int ntscPaletteEnable;
	g_config->getOption("SDL.NTSCpalette", &ntscPaletteEnable);
	ntscFrame->setChecked(ntscPaletteEnable);

	connect(ntscFrame, SIGNAL(clicked(bool)), this, SLOT(use_NTSC_Changed(bool)));

	sprintf(stmp, "Tint: %3i", tint);
	tintFrame = new QGroupBox(tr(stmp));
	hbox1 = new QHBoxLayout();
	tintSlider = new QSlider(Qt::Horizontal);
	tintSlider->setMinimum(0);
	tintSlider->setMaximum(128);
	tintSlider->setValue(tint);

	connect(tintSlider, SIGNAL(valueChanged(int)), this, SLOT(tintChanged(int)));

	hbox1->addWidget(tintSlider);
	tintFrame->setLayout(hbox1);
	hbox2->addWidget(tintFrame);

	sprintf(stmp, "Hue: %3i", hue);
	hueFrame = new QGroupBox(tr(stmp));
	hbox1 = new QHBoxLayout();
	hueSlider = new QSlider(Qt::Horizontal);
	hueSlider->setMinimum(0);
	hueSlider->setMaximum(128);
	hueSlider->setValue(hue);

	connect(hueSlider, SIGNAL(valueChanged(int)), this, SLOT(hueChanged(int)));

	hbox1->addWidget(hueSlider);
	hueFrame->setLayout(hbox1);
	hbox2->addWidget(hueFrame);

	ntscReset = new QPushButton( tr("Reset") );
	hbox2->addWidget(ntscReset);
	vbox->addLayout(hbox2);

	connect(ntscReset, SIGNAL(clicked(void)), this, SLOT(ntscResetClicked(void)));

	ntscFrame->setLayout(vbox);

	mainLayout->addWidget(ntscFrame);

	palFrame = new QGroupBox(tr("PAL Emulation:"));
	palFrame->setCheckable(false);
	palFrame->setEnabled( nes_shm->video.preScaler == 9 );

	grid  = new QGridLayout();
	grid->setColumnStretch( 0, 40 );
	grid->setColumnStretch( 1, 40 );
	grid->setColumnStretch( 2, 20 );

	sprintf(stmp, "Notch: %3i%%", palnotch);
	notchFrame = new QGroupBox(tr(stmp));
#if QT_VERSION > QT_VERSION_CHECK(5, 11, 0)
	notchFrame->setMinimumWidth( notchFrame->fontMetrics().horizontalAdvance('2') * strlen(stmp) );
#else
	notchFrame->setMinimumWidth( notchFrame->fontMetrics().width('2') * strlen(stmp) );
#endif
	hbox1 = new QHBoxLayout();
	notchSlider = new QSlider(Qt::Horizontal);
	notchSlider->setMinimumWidth(100);
	notchSlider->setMinimum(0);
	notchSlider->setMaximum(100);
	notchSlider->setValue(palnotch);

	hbox1->addWidget(notchSlider);
	notchFrame->setLayout(hbox1);

	sprintf(stmp, "Saturation: %3i%%", palsaturation);
	saturationFrame = new QGroupBox(tr(stmp));
#if QT_VERSION > QT_VERSION_CHECK(5, 11, 0)
	saturationFrame->setMinimumWidth( saturationFrame->fontMetrics().horizontalAdvance('2') * strlen(stmp) );
#else
	saturationFrame->setMinimumWidth( saturationFrame->fontMetrics().width('2') * strlen(stmp) );
#endif
	hbox1 = new QHBoxLayout();
	saturationSlider = new QSlider(Qt::Horizontal);
	saturationSlider->setMinimumWidth(100);
	saturationSlider->setMinimum(0);
	saturationSlider->setMaximum(200);
	saturationSlider->setValue(palsaturation);

	hbox1->addWidget(saturationSlider);
	saturationFrame->setLayout(hbox1);

	sprintf(stmp, "Sharpness: %3i%%", palsharpness*2);
	sharpnessFrame = new QGroupBox(tr(stmp));
#if QT_VERSION > QT_VERSION_CHECK(5, 11, 0)
	sharpnessFrame->setMinimumWidth( sharpnessFrame->fontMetrics().horizontalAdvance('2') * strlen(stmp) );
#else
	sharpnessFrame->setMinimumWidth( sharpnessFrame->fontMetrics().width('2') * strlen(stmp) );
#endif
	hbox1 = new QHBoxLayout();
	sharpnessSlider = new QSlider(Qt::Horizontal);
	sharpnessSlider->setMinimumWidth(50);
	sharpnessSlider->setMinimum(0);
	sharpnessSlider->setMaximum(50);
	sharpnessSlider->setValue(palsharpness);

	hbox1->addWidget(sharpnessSlider);
	sharpnessFrame->setLayout(hbox1);

	sprintf(stmp, "Contrast: %3i%%", palcontrast);
	contrastFrame = new QGroupBox(tr(stmp));
#if QT_VERSION > QT_VERSION_CHECK(5, 11, 0)
	contrastFrame->setMinimumWidth( contrastFrame->fontMetrics().horizontalAdvance('2') * strlen(stmp) );
#else
	contrastFrame->setMinimumWidth( contrastFrame->fontMetrics().width('2') * strlen(stmp) );
#endif
	hbox1 = new QHBoxLayout();
	contrastSlider = new QSlider(Qt::Horizontal);
	contrastSlider->setMinimumWidth(100);
	contrastSlider->setMinimum(0);
	contrastSlider->setMaximum(200);
	contrastSlider->setValue(palcontrast);

	hbox1->addWidget(contrastSlider);
	contrastFrame->setLayout(hbox1);

	sprintf(stmp, "Brightness: %3i%%", palbrightness);
	brightnessFrame = new QGroupBox(tr(stmp));
#if QT_VERSION > QT_VERSION_CHECK(5, 11, 0)
	brightnessFrame->setMinimumWidth( brightnessFrame->fontMetrics().horizontalAdvance('2') * strlen(stmp) );
#else
	brightnessFrame->setMinimumWidth( brightnessFrame->fontMetrics().width('2') * strlen(stmp) );
#endif
	hbox1 = new QHBoxLayout();
	brightnessSlider = new QSlider(Qt::Horizontal);
	brightnessSlider->setMinimumWidth(100);
	brightnessSlider->setMinimum(0);
	brightnessSlider->setMaximum(100);
	brightnessSlider->setValue(palbrightness);

	hbox1->addWidget(brightnessSlider);
	brightnessFrame->setLayout(hbox1);

	palReset = new QPushButton( tr("Reset") );

	grid->addWidget(notchFrame     , 0, 0);
	grid->addWidget(saturationFrame, 0, 1);
	grid->addWidget(sharpnessFrame , 0, 2);
	grid->addWidget(contrastFrame  , 1, 0);
	grid->addWidget(brightnessFrame, 1, 1);
	grid->addWidget(palReset       , 1, 2);

	connect( palReset        , SIGNAL(clicked(void))    , this, SLOT(palResetClicked(void)) );
	connect( notchSlider     , SIGNAL(valueChanged(int)), this, SLOT(palNotchChanged(int) ) );
	connect( saturationSlider, SIGNAL(valueChanged(int)), this, SLOT(palSaturationChanged(int) ) );
	connect( sharpnessSlider , SIGNAL(valueChanged(int)), this, SLOT(palSharpnessChanged(int) ) );
	connect( contrastSlider  , SIGNAL(valueChanged(int)), this, SLOT(palContrastChanged(int) ) );
	connect( brightnessSlider, SIGNAL(valueChanged(int)), this, SLOT(palBrightnessChanged(int) ) );

	palFrame->setLayout(grid);

	mainLayout->addWidget(palFrame);

	comments = new QTextEdit();

	comments->setText(commentText);
	comments->moveCursor(QTextCursor::Start);
	comments->setReadOnly(true);

	comments->setMinimumHeight( 7 * comments->fontMetrics().lineSpacing() );

	mainLayout->addWidget(comments);

	closeButton = new QPushButton( tr("Close") );
	closeButton->setIcon(style->standardIcon(QStyle::SP_DialogCloseButton));
	connect(closeButton, SIGNAL(clicked(void)), this, SLOT(closeWindow(void)));

	hbox = new QHBoxLayout();
	hbox->addStretch(5);
	hbox->addWidget( closeButton, 1 );
	mainLayout->addLayout( hbox );

	setLayout(mainLayout);

	updateTimer = new QTimer(this);

	connect(updateTimer, &QTimer::timeout, this, &PaletteConfDialog_t::updatePeriodic);

	updateTimer->start(500); // 2hz
}

//----------------------------------------------------
PaletteConfDialog_t::~PaletteConfDialog_t(void)
{
	//printf("Destroy Palette Config Window\n");
	updateTimer->stop();
}
//----------------------------------------------------------------------------
void PaletteConfDialog_t::closeEvent(QCloseEvent *event)
{
	//printf("Palette Config Close Window Event\n");
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
void PaletteConfDialog_t::updatePeriodic(void)
{
	palFrame->setEnabled( nes_shm->video.preScaler == 9 );
}
//----------------------------------------------------
void PaletteConfDialog_t::ntscResetClicked(void)
{
	 hueSlider->setValue( 72 );
	tintSlider->setValue( 56 );
}
//----------------------------------------------------
void PaletteConfDialog_t::hueChanged(int v)
{
	int c, t;
	char stmp[64];

	sprintf(stmp, "Hue: %3i", v);

	hueFrame->setTitle(stmp);

	g_config->setOption("SDL.Hue", v);
	g_config->save();
	g_config->getOption("SDL.Tint", &t);
	g_config->getOption("SDL.NTSCpalette", &c);

	if (fceuWrapperTryLock())
	{
		FCEUI_SetNTSCTH(c, t, v);
		fceuWrapperUnLock();
	}
}
//----------------------------------------------------
void PaletteConfDialog_t::tintChanged(int v)
{
	int c, h;
	char stmp[64];

	sprintf(stmp, "Tint: %3i", v);

	tintFrame->setTitle(stmp);

	g_config->setOption("SDL.Tint", v);
	g_config->save();
	g_config->getOption("SDL.NTSCpalette", &c);
	g_config->getOption("SDL.Hue", &h);

	if (fceuWrapperTryLock())
	{
		FCEUI_SetNTSCTH(c, v, h);
		fceuWrapperUnLock();
	}
}
//----------------------------------------------------
void PaletteConfDialog_t::use_Custom_Changed(int state)
{
	int value = (state == Qt::Unchecked) ? 0 : 1;
	std::string filename;

	//printf("Use Custom:%i \n", state );

	g_config->getOption("SDL.Palette", &filename);

	if (fceuWrapperTryLock())
	{
		if (value && (filename.size() > 0))
		{
			LoadCPalette(filename.c_str());
		}
		else
		{
			FCEUI_SetUserPalette(NULL, 0);
		}
		fceuWrapperUnLock();
	}
}
//----------------------------------------------------
void PaletteConfDialog_t::force_GrayScale_Changed(int state)
{
	int value = (state == Qt::Unchecked) ? 0 : 1;

	if (fceuWrapperTryLock())
	{
		int e, h, t;
		g_config->getOption("SDL.NTSCpalette", &e);
		g_config->getOption("SDL.Hue", &h);
		g_config->getOption("SDL.Tint", &t);
		force_grayscale = value ? true : false;
		FCEUI_SetNTSCTH(e, t, h);
		fceuWrapperUnLock();

		g_config->setOption("SDL.ForceGrayScale", force_grayscale);
	}
}
//----------------------------------------------------
void PaletteConfDialog_t::deemphswap_Changed(int state)
{
	int value = (state == Qt::Unchecked) ? 0 : 1;

	if (fceuWrapperTryLock())
	{
		int e, h, t;
		g_config->getOption("SDL.NTSCpalette", &e);
		g_config->getOption("SDL.Hue", &h);
		g_config->getOption("SDL.Tint", &t);
		paldeemphswap = value ? true : false;
		FCEUI_SetNTSCTH(e, t, h);
		fceuWrapperUnLock();

		g_config->setOption("SDL.DeempBitSwap", paldeemphswap);
	}
}
//----------------------------------------------------
void PaletteConfDialog_t::use_NTSC_Changed(bool state)
{
	int h, t;
	int value = state;

	g_config->setOption("SDL.NTSCpalette", value);
	g_config->save();

	g_config->getOption("SDL.Hue", &h);
	g_config->getOption("SDL.Tint", &t);

	if (fceuWrapperTryLock())
	{
		FCEUI_SetNTSCTH(value, t, h);
		//UpdateEMUCore (g_config);
		fceuWrapperUnLock();
	}
}
//----------------------------------------------------
void PaletteConfDialog_t::clearPalette(void)
{
	g_config->setOption("SDL.Palette", "");
	custom_palette_path->setText("");

	if (fceuWrapperTryLock())
	{
		FCEUI_SetUserPalette(NULL, 0);
		fceuWrapperUnLock();
		useCustom->setChecked(false);
	}
}
//----------------------------------------------------
void PaletteConfDialog_t::openPaletteFile(void)
{
	int ret, useNativeFileDialogVal;
	QString filename;
	std::string last, iniPath;
	char dir[512];
	char exePath[512];
	QFileDialog dialog(this, tr("Open NES Palette"));
	QList<QUrl> urls;
	QDir d;

	fceuExecutablePath(exePath, sizeof(exePath));

	//urls = dialog.sidebarUrls();
	urls << QUrl::fromLocalFile(QDir::rootPath());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first());
	urls << QUrl::fromLocalFile(QDir(FCEUI_GetBaseDirectory()).absolutePath());

	if (exePath[0] != 0)
	{	// This is where the windows build expects the palettes to be
		d.setPath(QString(exePath) + "/../palettes");

		if (d.exists())
		{
			urls << QUrl::fromLocalFile(d.absolutePath());
			iniPath = d.absolutePath().toStdString();
		}

		#ifdef __APPLE__
		// Search for MacOSX DragNDrop Resources
		d.setPath(QString(exePath) + "/../Resources/palettes");

		//printf("Looking for: '%s'\n", d.path().toStdString().c_str());

		if (d.exists())
		{
			urls << QUrl::fromLocalFile(d.absolutePath());
			iniPath = d.absolutePath().toStdString();
		}
		#endif
	}
#ifdef WIN32

#else
	// Linux and MacOSX (homebrew) expect shared data folder to be relative to bin/fceux executable.
	if (exePath[0] != 0)
	{
		d.setPath(QString(exePath) + "/../share/fceux/palettes");
	}
	else
	{
		d.setPath(QString("/usr/local/share/fceux/palettes"));
	}
	if (!d.exists())
	{
		d.setPath(QString("/usr/local/share/fceux/palettes"));
	}
	if (!d.exists())
	{
		d.setPath(QString("/usr/share/fceux/palettes"));
	}

	//printf("Looking for: '%s'\n", d.path().toStdString().c_str());

	if (d.exists())
	{
		urls << QUrl::fromLocalFile(d.absolutePath());
		iniPath = d.absolutePath().toStdString();
	}
#endif

	dialog.setFileMode(QFileDialog::ExistingFile);

	dialog.setNameFilter(tr("NES Palettes (*.pal *.PAL) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter(QDir::AllEntries | QDir::AllDirs | QDir::Hidden);
	dialog.setLabelText(QFileDialog::Accept, tr("Load"));

	g_config->getOption("SDL.Palette", &last);

	if (last.size() == 0)
	{
		last.assign(iniPath);
	}

	getDirFromFile(last.c_str(), dir);

	dialog.setDirectory(tr(dir));

	// Check config option to use native file dialog or not
	g_config->getOption("SDL.UseNativeFileDialog", &useNativeFileDialogVal);

	dialog.setOption(QFileDialog::DontUseNativeDialog, !useNativeFileDialogVal);
	dialog.setSidebarUrls(urls);

	ret = dialog.exec();

	if (ret)
	{
		QStringList fileList;
		fileList = dialog.selectedFiles();

		if (fileList.size() > 0)
		{
			filename = fileList[0];
		}
	}

	if (filename.isNull())
	{
		return;
	}
	qDebug() << "selected file path : " << filename.toUtf8();

	if (fceuWrapperTryLock())
	{
		if (LoadCPalette(filename.toStdString().c_str()))
		{
			g_config->setOption("SDL.Palette", filename.toStdString().c_str());
			custom_palette_path->setText(filename.toStdString().c_str());
		}
		else
		{
			printf("Error: Failed to Load Palette File: %s \n", filename.toStdString().c_str());
		}
		fceuWrapperUnLock();

		useCustom->setChecked(FCEUI_GetUserPaletteAvail());
	}

	return;
}
//----------------------------------------------------
void PaletteConfDialog_t::palResetClicked(void)
{
	if (fceuWrapperTryLock())
	{
		palnotch      = 100;
		palsaturation = 100;
		palsharpness  = 0;
		palcontrast   = 100;
		palbrightness = 50;
		palupdate     = 1;

		fceuWrapperUnLock();

		notchSlider->setValue( palnotch );
		saturationSlider->setValue( palsaturation );
		sharpnessSlider->setValue( palsharpness );
		contrastSlider->setValue( palcontrast );
		brightnessSlider->setValue( palbrightness );
	}
}
//----------------------------------------------------
void PaletteConfDialog_t::palNotchChanged(int value)
{
	if (fceuWrapperTryLock())
	{
		char stmp[64];

		sprintf( stmp, "Notch: %3i%%", value );
		notchFrame->setTitle( tr(stmp) );

		palnotch  = value;
		palupdate = 1;

		fceuWrapperUnLock();

		g_config->setOption("SDL.PalNotch", palnotch);
	}
}
//----------------------------------------------------
void PaletteConfDialog_t::palSaturationChanged(int value)
{
	if (fceuWrapperTryLock())
	{
		char stmp[64];

		sprintf( stmp, "Saturation: %3i%%", value );
		saturationFrame->setTitle( tr(stmp) );

		palsaturation  = value;
		palupdate      = 1;

		fceuWrapperUnLock();

		g_config->setOption("SDL.PalSaturation", palsaturation);
	}
}
//----------------------------------------------------
void PaletteConfDialog_t::palSharpnessChanged(int value)
{
	if (fceuWrapperTryLock())
	{
		char stmp[64];

		sprintf( stmp, "Sharpness: %3i%%", value*2 );
		sharpnessFrame->setTitle( tr(stmp) );

		palsharpness   = value;
		palupdate      = 1;

		fceuWrapperUnLock();

		g_config->setOption("SDL.PalSharpness", palsharpness);
	}
}
//----------------------------------------------------
void PaletteConfDialog_t::palContrastChanged(int value)
{
	if (fceuWrapperTryLock())
	{
		char stmp[64];

		sprintf( stmp, "Contrast: %3i%%", value );
		contrastFrame->setTitle( tr(stmp) );

		palcontrast    = value;
		palupdate      = 1;

		fceuWrapperUnLock();

		g_config->setOption("SDL.PalContrast", palcontrast);
	}
}
//----------------------------------------------------
void PaletteConfDialog_t::palBrightnessChanged(int value)
{
	if (fceuWrapperTryLock())
	{
		char stmp[64];

		sprintf( stmp, "Brightness: %3i%%", value );
		brightnessFrame->setTitle( tr(stmp) );

		palbrightness  = value;
		palupdate      = 1;

		fceuWrapperUnLock();

		g_config->setOption("SDL.PalBrightness", palbrightness);
	}
}
//----------------------------------------------------
