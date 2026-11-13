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
// ConsoleVideoConf.cpp
//
#include <QCloseEvent>
#include <QGuiApplication>
#include <QMessageBox>
#include <QSignalBlocker>

#include "../../fceu.h"
#include "../../input.h"
#include "../../video.h"
#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/config.h"
#include "Qt/throttle.h"
#include "Qt/fceuWrapper.h"
#include "Qt/ConsoleWindow.h"
#include "Qt/ConsoleUtilities.h"
#include "Qt/ConsoleVideoConf.h"
#include "Qt/ConsoleViewerInterface.h"
#include "Qt/nes_shm.h"

#if defined(WIN32) && (QT_VERSION_MAJOR < 6)
#include <QtPlatformHeaders/QWindowsWindowFunctions>
#endif

extern int input_display;
extern int frame_display;
extern int rerecord_display;
//----------------------------------------------------
ConsoleVideoConfDialog_t::ConsoleVideoConfDialog_t(QWidget *parent)
	: QDialog( parent )
{
	QVBoxLayout *main_vbox, *vbox1, *vbox2, *vbox3, *vbox4, *vbox5, *vbox;
	QHBoxLayout *main_hbox, *hbox1, *hbox;
	QLabel *lbl;
	QPushButton *button;
	QStyle *style;
	QGroupBox *gbox;
	QGridLayout *grid;
	QFont font;
	int opt, fontCharWidth;
	char stmp[128];

	// One short line of explanation under a group title that is always visible
	auto descriptionLabel = [](const QString &text, const QString &tip) -> QLabel *
	{
		QLabel *l = new QLabel( text );
		QFont f = l->font();
		f.setItalic( true );
		l->setFont( f );
		l->setToolTip( tip );
		return l;
	};

	// Label with a tooltip, so hovering the text beside a control explains it too
	auto tipLabel = [](const QString &text, const QString &tip) -> QLabel *
	{
		QLabel *l = new QLabel( text );
		l->setToolTip( tip );
		return l;
	};

	font.setFamily("Courier New");
	font.setStyle( QFont::StyleNormal );
	font.setStyleHint( QFont::Monospace );
	QFontMetrics fm(font);

#if QT_VERSION > QT_VERSION_CHECK(5, 11, 0)
    fontCharWidth = fm.horizontalAdvance(QLatin1Char('2'));
#else
    fontCharWidth = fm.width(QLatin1Char('2'));
#endif

	g_config->getOption("SDL.ShowFrameCount", &frame_display);
	g_config->getOption("SDL.ShowLagCount", &lagCounterDisplay);
	g_config->getOption("SDL.ShowRerecordCount", &rerecord_display);
	g_config->getOption("SDL.ShowGuiMessages", &vidGuiMsgEna);
	
	style = this->style();

	setWindowTitle( tr("Video Config") );

	main_vbox = new QVBoxLayout();
	main_hbox = new QHBoxLayout();
	vbox1     = new QVBoxLayout();

	main_vbox->addLayout( main_hbox );
	main_hbox->addLayout( vbox1 );

	// Video Driver Select
	lbl = tipLabel( tr("Driver:"), tr("The renderer that draws the game image.") );

	driverSelect = new QComboBox();

	driverSelect->addItem( tr("OpenGL"), ConsoleViewerBase::VIDEO_DRIVER_OPENGL );
	driverSelect->addItem( tr("SDL"), ConsoleViewerBase::VIDEO_DRIVER_SDL );
	driverSelect->addItem( tr("QPainter"), ConsoleViewerBase::VIDEO_DRIVER_QPAINTER );
#ifdef FCEUX_HAS_QT_VULKAN
	driverSelect->addItem( tr("Vulkan"), ConsoleViewerBase::VIDEO_DRIVER_VULKAN );
#endif
	
	hbox1 = new QHBoxLayout();

	hbox1->addWidget( lbl );
	hbox1->addWidget( driverSelect );

	vbox1->addLayout( hbox1 );

	// Scaler Select
	lbl = tipLabel( tr("Scaler:"), tr("The filter applied to the game image before it is scaled to the window.") );

	scalerSelect = new QComboBox();

	scalerSelect->addItem( tr("None"), 0 );
	scalerSelect->addItem( tr("hq2x"), 1 );
	scalerSelect->addItem( tr("scale2x"), 2 );
	scalerSelect->addItem( tr("NTSC 2x"), 3 );
	scalerSelect->addItem( tr("hq3x"), 4 );
	scalerSelect->addItem( tr("scale3x"), 5 );
	scalerSelect->addItem( tr("Prescale 2x"), 6 );
	scalerSelect->addItem( tr("Prescale 3x"), 7 );
	scalerSelect->addItem( tr("Prescale 4x"), 8 );
	scalerSelect->addItem( tr("PAL 3x"), 9 );
	
	hbox1 = new QHBoxLayout();

	hbox1->addWidget( lbl );
	hbox1->addWidget( scalerSelect );

	vbox1->addLayout( hbox1 );

	// Enable Linear Filter Checkbox
	gl_LF_chkBox  = new QCheckBox( tr("Enable Linear Filter") );

	setCheckBoxFromProperty( gl_LF_chkBox  , "SDL.OpenGLip");

	connect(gl_LF_chkBox , SIGNAL(toggled(bool)), this, SLOT(openGL_linearFilterChanged(bool)) );

	vbox1->addWidget( gl_LF_chkBox );

#if defined(WIN32) && (QT_VERSION_MAJOR < 6)
	// 1px full screen border - hack fix for QOpenGLWidget fullscreen issues
	winFullScreenBorderCbx  = new QCheckBox( tr("Fullscreen Border (1px)") );
	winFullScreenBorderCbx->setToolTip(tr("Hack fix for QOpenGLWidget fullscreen issue. May not be needed."));

	setCheckBoxFromProperty( winFullScreenBorderCbx  , "SDL.winFullScreenBorder");

	connect(winFullScreenBorderCbx, SIGNAL(toggled(bool)), this, SLOT(winFullScreenBorderChanged(bool)) );

	vbox1->addWidget(winFullScreenBorderCbx);
#endif

	// Region Select
	lbl = tipLabel( tr("Region:"), tr("The video system the emulator runs as. This sets the timing of the game.") );

	regionSelect = new QComboBox();

	regionSelect->addItem( tr("NTSC") , 0 );
	regionSelect->addItem( tr("PAL")  , 1 );
	regionSelect->addItem( tr("Dendy"), 2 );

	setComboBoxFromProperty( regionSelect, "SDL.PAL");
	setComboBoxFromProperty( driverSelect, "SDL.VideoDriver");
	setComboBoxFromProperty( scalerSelect, "SDL.SpecialFilter");

	connect(regionSelect, SIGNAL(currentIndexChanged(int)), this, SLOT(regionChanged(int)) );
	connect(driverSelect, SIGNAL(currentIndexChanged(int)), this, SLOT(driverChanged(int)) );
	connect(scalerSelect, SIGNAL(currentIndexChanged(int)), this, SLOT(scalerChanged(int)) );

	hbox1 = new QHBoxLayout();

	hbox1->addWidget( lbl );
	hbox1->addWidget( regionSelect );

	vbox1->addLayout( hbox1 );

	// Enable Region Auto Detection Logic
	autoRegion  = new QCheckBox( tr("Region Auto Detect") );

	// Enable New PPU Checkbox
	new_PPU_ena  = new QCheckBox( tr("Enable New PPU") );

	// Enable Frameskip
	frmskipcbx  = new QCheckBox( tr("Enable Frameskip") );

	// Enable Vertical Sync
	vsync_ena  = new QCheckBox( tr("Enable Vertical Sync") );

	// Use Integer Frame Rate Checkbox
	intFrameRateCbx  = new QCheckBox( tr("Use Integer Frame Rate") );

	// Disable Sprite Limit Checkbox
	sprtLimCbx  = new QCheckBox( tr("Disable Sprite Limit") );

	// Clip Sides Checkbox
	clipSidesCbx  = new QCheckBox( tr("Clip Left/Right Sides (8 px on each)") );

	// Show GUI Messages Checkbox
	showGuiMsgs_cbx  = new QCheckBox( tr("GUI Messages") );
	showGuiMsgs_cbx->setChecked( vidGuiMsgEna );

	// Show FPS Checkbox
	showFPS_cbx  = new QCheckBox( tr("FPS") );

	// Show Frame Count Checkbox
	showFrameCount_cbx  = new QCheckBox( tr("Frame Counter") );
	showFrameCount_cbx->setChecked( frame_display );

	// Show Lag Count Checkbox
	showLagCount_cbx  = new QCheckBox( tr("Lag Counter") );
	showLagCount_cbx->setChecked( lagCounterDisplay );

	// Show Re-Record Count Checkbox
	showRerecordCount_cbx  = new QCheckBox( tr("Rerecord Count") );
	showRerecordCount_cbx->setChecked( rerecord_display );

	// Text overlay items and update rate, independent of the video overlay checkboxes
	for (int i = 0; i < TextOverlayDisplay_t::ITEM_COUNT; i++)
	{
		const TextOverlayDisplay_t::ItemInfo &info = TextOverlayDisplay_t::itemInfo( static_cast<TextOverlayDisplay_t::Item>(i) );

		textOverlayCbx[i] = new QCheckBox( tr(info.text) );

		QString tip = tr(info.tip);

		// Movie State has no counterpart in the legacy overlay
		if ( i != TextOverlayDisplay_t::ITEM_MOVIE )
		{
			tip += tr("\nThe same value is available in Video Overlay (Legacy). Turning it on in both shows it twice.");
		}
		textOverlayCbx[i]->setToolTip( tip );
	}
	textOverlayRateSel = new QComboBox();
	textOverlayRateSel->setToolTip( tr("How often the text overlay values are refreshed. Lower rates use less CPU.") );

	for (int hz : TextOverlayDisplay_t::UPDATE_RATES_HZ)
	{
		textOverlayRateSel->addItem( tr("%1 Hz").arg(hz), hz );
	}

	// Auto Scale on Resize
	autoScaleCbx  = new QCheckBox( tr("Auto Scale on Resize") );

	// Force Aspect Ratio
	aspectCbx  = new QCheckBox( tr("Force Aspect Ratio") );

	// Draw Input Aids
	drawInputAidsCbx = new QCheckBox( tr("Draw Input Aids") );

	// Input Display Select
	inputDisplaySel = new QComboBox();

	inputDisplaySel->addItem( tr("None"), 0 );
	inputDisplaySel->addItem( tr("1"), 1 );
	inputDisplaySel->addItem( tr("1 & 2"), 2 );
	inputDisplaySel->addItem( tr("1, 2, 3 & 4"), 4 );

	setComboBoxFromProperty( inputDisplaySel , "SDL.InputDisplay");

	// Input Display Select
	videoTest = new QComboBox();

	videoTest->addItem( tr("None")         , 0 );
	videoTest->addItem( tr("Vertical Sync"), 1 );

	videoTest->setCurrentIndex( nes_shm->video.test );

	connect(videoTest, SIGNAL(currentIndexChanged(int)), this, SLOT(testPatternChanged(int)) );

	setCheckBoxFromProperty( autoRegion      , "SDL.AutoDetectPAL");
	setCheckBoxFromProperty( new_PPU_ena     , "SDL.NewPPU");
	setCheckBoxFromProperty( frmskipcbx      , "SDL.Frameskip");
	setCheckBoxFromProperty( vsync_ena       , "SDL.VideoVsync");
	setCheckBoxFromProperty( intFrameRateCbx , "SDL.IntFrameRate");
	setCheckBoxFromProperty( sprtLimCbx      , "SDL.DisableSpriteLimit");
	setCheckBoxFromProperty( clipSidesCbx    , "SDL.ClipSides");
	setCheckBoxFromProperty( showFPS_cbx     , "SDL.ShowFPS");
	setCheckBoxFromProperty( drawInputAidsCbx, "SDL.DrawInputAids" );
	
	if ( consoleWindow )
	{
		if ( consoleWindow->viewport_Interface )
		{
			autoScaleCbx->setChecked( consoleWindow->viewport_Interface->getAutoScaleOpt() );
			aspectCbx->setChecked( consoleWindow->viewport_Interface->getForceAspectOpt() );
		}
	}

	connect(new_PPU_ena     , SIGNAL(clicked(bool))    , this, SLOT(use_new_PPU_changed(bool)) );
	connect(autoRegion      , SIGNAL(toggled(bool)), this, SLOT(autoRegionChanged(bool)) );
	connect(frmskipcbx      , SIGNAL(toggled(bool)), this, SLOT(frameskip_changed(bool)) );
	connect(vsync_ena       , SIGNAL(toggled(bool)), this, SLOT(vsync_changed(bool)) );
	connect(intFrameRateCbx , SIGNAL(toggled(bool)), this, SLOT(intFrameRate_changed(bool)) );
	connect(sprtLimCbx      , SIGNAL(toggled(bool)), this, SLOT(useSpriteLimitChanged(bool)) );
	connect(clipSidesCbx    , SIGNAL(toggled(bool)), this, SLOT(clipSidesChanged(bool)) );
	connect(showFPS_cbx     , SIGNAL(toggled(bool)), this, SLOT(showFPSChanged(bool)) );
	connect(showGuiMsgs_cbx , SIGNAL(toggled(bool)), this, SLOT(showGuiMsgsChanged(bool)) );
	connect(aspectCbx       , SIGNAL(toggled(bool)), this, SLOT(aspectEnableChanged(bool)) );
	connect(autoScaleCbx    , SIGNAL(toggled(bool)), this, SLOT(autoScaleChanged(bool)) );
	connect(drawInputAidsCbx, SIGNAL(toggled(bool)), this, SLOT(drawInputAidsChanged(bool)) );

	connect(   showFrameCount_cbx, SIGNAL(toggled(bool)), this, SLOT(showFrameCountChanged(bool))   );
	connect(     showLagCount_cbx, SIGNAL(toggled(bool)), this, SLOT(showLagCountChanged(bool))     );
	connect(showRerecordCount_cbx, SIGNAL(toggled(bool)), this, SLOT(showRerecordCountChanged(bool)));

	connect(inputDisplaySel, SIGNAL(currentIndexChanged(int)), this, SLOT(inputDisplayChanged(int)) );

	for (int i = 0; i < TextOverlayDisplay_t::ITEM_COUNT; i++)
	{
		connect(textOverlayCbx[i], &QCheckBox::toggled, this, [this, i](bool checked){ textOverlayItemChanged(i, checked); });
	}
	connect(textOverlayRateSel, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ConsoleVideoConfDialog_t::textOverlayRateChanged);

	if ( consoleWindow && consoleWindow->getTextOverlayDisplay() )
	{
		// The Options menu and the text overlay can change these while the dialog is open
		connect(consoleWindow->getTextOverlayDisplay(), &TextOverlayDisplay_t::changed, this, &ConsoleVideoConfDialog_t::updateTextOverlayControls);
	}
	updateTextOverlayControls();

	vbox1->addWidget( autoRegion  );
	vbox1->addWidget( new_PPU_ena );
	vbox1->addWidget( frmskipcbx  );
	vbox1->addWidget( vsync_ena   );
	vbox1->addWidget( intFrameRateCbx  );
	vbox1->addWidget( sprtLimCbx  );
	//vbox1->addWidget( drawInputAidsCbx );
	//vbox1->addWidget( showFPS_cbx );
	vbox1->addWidget( autoScaleCbx);
	vbox1->addWidget( aspectCbx   );

	aspectSelect = new QComboBox();

	aspectSelect->addItem( tr("Square Pixels (1:1)"), 0 );
	aspectSelect->addItem( tr("NTSC (8:7)"), 1 );
	aspectSelect->addItem( tr("PAL (11:8)"), 2 );
	aspectSelect->addItem( tr("Standard (4:3)"), 3 );
	aspectSelect->addItem( tr("Widescreen (16:9)"), 4 );
	//aspectSelect->addItem( tr("Custom"), 5 ); TODO

	setComboBoxFromProperty( aspectSelect, "SDL.AspectSelect");

	connect(aspectSelect, SIGNAL(currentIndexChanged(int)), this, SLOT(aspectChanged(int)) );

	aspectSelectLabel = new QLabel( tr("Aspect:") );

	hbox1 = new QHBoxLayout();
	hbox1->addWidget( aspectSelectLabel );
	hbox1->addWidget( aspectSelect      );
	vbox1->addLayout( hbox1 );

	xScaleBox = new QDoubleSpinBox(this);
	yScaleBox = new QDoubleSpinBox(this);

	xScaleBox->setRange( 1.0, 16.0 );
	yScaleBox->setRange( 1.0, 16.0 );

	xScaleBox->setSingleStep( 0.10 );
	yScaleBox->setSingleStep( 0.10 );

	if ( consoleWindow )
	{
		if ( consoleWindow->viewport_Interface )
		{
			xScaleBox->setValue( consoleWindow->viewport_Interface->getScaleX() );
			yScaleBox->setValue( consoleWindow->viewport_Interface->getScaleY() );
		}
	}

	if ( aspectCbx->isChecked() )
	{
		xScaleLabel = new QLabel( tr("Scale:") );
	}
	else
	{
		xScaleLabel = new QLabel( tr("X Scale:") );
	}
	yScaleLabel = new QLabel( tr("Y Scale:") );

	hbox1 = new QHBoxLayout();
	hbox1->addWidget( xScaleLabel );
	hbox1->addWidget( xScaleBox );
	vbox1->addLayout( hbox1 );

	hbox1 = new QHBoxLayout();
	hbox1->addWidget( yScaleLabel );
	hbox1->addWidget( yScaleBox );
	vbox1->addLayout( hbox1 );

	if ( aspectCbx->isChecked() )
	{
		yScaleLabel->hide();
		yScaleBox->hide();
	}
	else
	{
		aspectSelectLabel->hide();
		aspectSelect->hide();
	}

	hbox1 = new QHBoxLayout();

	button = new QPushButton( tr("Apply") );
	hbox1->addWidget( button );
	connect(button, SIGNAL(clicked()), this, SLOT(applyChanges(void)) );
	button->setIcon( style->standardIcon( QStyle::SP_DialogApplyButton ) );

	button = new QPushButton( tr("Close") );
	hbox1->addWidget( button );
	button->setIcon( style->standardIcon( QStyle::SP_DialogCloseButton ) );
	connect(button, SIGNAL(clicked()), this, SLOT(closeWindow(void)) );

	main_vbox->addLayout( hbox1 );

	gbox  = new QGroupBox( tr("Video Overlay (Legacy)") );
	gbox->setToolTip( tr("Overlays drawn by the emulator core into the game image. They scale and blur with the picture, and are affected by the scaler and the linear filter.\n"
		"The hotkeys for the frame counter, lag counter and controller display control these items.\n"
		"The Window Text Overlay below shows the same values as sharp text over the whole window.") );
	vbox3 = new QVBoxLayout();
	vbox4 = new QVBoxLayout();
	vbox5 = new QVBoxLayout();
	vbox  = new QVBoxLayout();

	vbox3->addWidget( gbox, 1 );

	{
		QGroupBox *textGbox = new QGroupBox( tr("Window Text Overlay") );
		textGbox->setToolTip( tr("Status text drawn by the video driver over the whole window, not into the game image. It stays sharp at any window size, "
			"and it can use the empty bars beside or above the image. It is drawn inside the image when there is no room outside.\n"
			"The frame counter and lag counter hotkeys do not change these items, only the ones in Video Overlay (Legacy).\n"
			"If the same value is turned on in both overlays it is shown twice.") );
		QVBoxLayout *textVbox = new QVBoxLayout();
		QHBoxLayout *rateBox = new QHBoxLayout();

		textGbox->setLayout( textVbox );

		textVbox->addWidget( descriptionLabel( tr("Over the whole window."), textGbox->toolTip() ) );

		for (int i = 0; i < TextOverlayDisplay_t::ITEM_COUNT; i++)
		{
			textVbox->addWidget( textOverlayCbx[i] );
		}
		rateBox->addWidget( tipLabel( tr("Update Rate:"), tr("How often the text overlay values are refreshed. Lower rates use less CPU.") ) );
		rateBox->addWidget( textOverlayRateSel );
		rateBox->addStretch( 1 );
		textVbox->addLayout( rateBox );

		vbox3->addWidget( textGbox );
	}
	vbox3->addStretch(5);
	 gbox->setLayout( vbox );

	vbox->addWidget( descriptionLabel( tr("Inside the game image."), gbox->toolTip() ) );
	vbox->addWidget( showGuiMsgs_cbx );
	vbox->addWidget( drawInputAidsCbx );
	vbox->addWidget( showFrameCount_cbx );
	vbox->addWidget( showLagCount_cbx );
	vbox->addWidget( showRerecordCount_cbx );
	vbox->addWidget( showFPS_cbx );

	gbox  = new QGroupBox( tr("Show Controllers:") );
	gbox->setToolTip( tr("Draw the buttons pressed on the game controllers near the bottom of the game image.") );
	gbox->setLayout( vbox4 );
	vbox->addWidget( gbox  );
	vbox4->addWidget( inputDisplaySel );

	gbox  = new QGroupBox( tr("Drawing Area") );
	gbox->setToolTip( tr("Which part of the NES picture is shown.") );
	vbox2 = new QVBoxLayout();
	grid  = new QGridLayout();

	main_hbox->addLayout( vbox3 );
	main_hbox->addLayout( vbox2 );
	vbox2->addWidget( gbox, 1 );
	gbox->setLayout(grid);

	ntsc_start = new QSpinBox();
	ntsc_end   = new QSpinBox();
	pal_start  = new QSpinBox();
	pal_end    = new QSpinBox();

	ntsc_start->setRange( 0, 239 );
	  ntsc_end->setRange( 0, 239 );
	 pal_start->setRange( 0, 239 );
	   pal_end->setRange( 0, 239 );

	ntsc_start->setFont( font );
	  ntsc_end->setFont( font );
	 pal_start->setFont( font );
	   pal_end->setFont( font );

	ntsc_start->setMinimumWidth( fontCharWidth * 8 );
	  ntsc_end->setMinimumWidth( fontCharWidth * 8 );
	 pal_start->setMinimumWidth( fontCharWidth * 8 );
	   pal_end->setMinimumWidth( fontCharWidth * 8 );

	ntsc_start->setMaximumWidth( fontCharWidth * 8 );
	  ntsc_end->setMaximumWidth( fontCharWidth * 8 );
	 pal_start->setMaximumWidth( fontCharWidth * 8 );
	   pal_end->setMaximumWidth( fontCharWidth * 8 );

	g_config->getOption("SDL.ScanLineStartNTSC", &opt);
	ntsc_start->setValue( opt );

	g_config->getOption("SDL.ScanLineEndNTSC", &opt);
	ntsc_end->setValue( opt );

	g_config->getOption("SDL.ScanLineStartPAL", &opt);
	pal_start->setValue( opt );

	g_config->getOption("SDL.ScanLineEndPAL", &opt);
	pal_end->setValue( opt );

	ntsc_start->setRange( 0, ntsc_end->value() );
	  ntsc_end->setRange( ntsc_start->value(), 239 );
	 pal_start->setRange( 0, pal_end->value() );
	   pal_end->setRange( pal_start->value(), 239 );

	connect( ntsc_start, SIGNAL(valueChanged(int)), this, SLOT(ntscStartScanLineChanged(int)));
	connect( ntsc_end  , SIGNAL(valueChanged(int)), this, SLOT(ntscEndScanLineChanged(int)));
	connect( pal_start , SIGNAL(valueChanged(int)), this, SLOT(palStartScanLineChanged(int)));
	connect( pal_end   , SIGNAL(valueChanged(int)), this, SLOT(palEndScanLineChanged(int)));

	grid->addWidget( tipLabel( tr("NTSC")      , tr("Scanline range used while the region is NTSC.") )                   , 0, 1, Qt::AlignLeft);
	grid->addWidget( tipLabel( tr("PAL/Dendy") , tr("Scanline range used while the region is PAL or Dendy.") )           , 0, 2, Qt::AlignLeft);
	grid->addWidget( tipLabel( tr("First Line:"), tr("The first scanline of the picture that is shown. Lines above it are cut off.") ), 1, 0, Qt::AlignLeft);
	grid->addWidget( tipLabel( tr("Last Line:") , tr("The last scanline of the picture that is shown. Lines below it are cut off.") ), 2, 0, Qt::AlignLeft);
	grid->addWidget( ntsc_start, 1, 1, Qt::AlignLeft);
	grid->addWidget( pal_start , 1, 2, Qt::AlignLeft);
	grid->addWidget( ntsc_end  , 2, 1, Qt::AlignLeft);
	grid->addWidget( pal_end   , 2, 2, Qt::AlignLeft);
	grid->addWidget( clipSidesCbx, 3, 0, 1, 3);

	gbox  = new QGroupBox( tr("Current Dimensions") );
	gbox->setToolTip( tr("The current size of the window and of the game area. Read only.") );
	grid  = new QGridLayout();

	vbox2->addWidget( gbox, 1 );
	gbox->setLayout(grid);

	winSizeReadout = new QLineEdit();
	winSizeReadout->setFont( font );
	winSizeReadout->setReadOnly(true);
	winSizeReadout->setAlignment(Qt::AlignCenter);

	vpSizeReadout = new QLineEdit();
	vpSizeReadout->setFont( font );
	vpSizeReadout->setReadOnly(true);
	vpSizeReadout->setAlignment(Qt::AlignCenter);

	grid->addWidget( tipLabel( tr("Window:")  , tr("Size of the whole main window, including the menu and any docked widgets, in pixels.") ), 0, 0, Qt::AlignLeft);
	grid->addWidget( tipLabel( tr("Viewport:"), tr("Size of the area that shows the game, in pixels.") ), 1, 0, Qt::AlignLeft);
	grid->addWidget( winSizeReadout, 0, 1, Qt::AlignLeft);
	grid->addWidget( vpSizeReadout, 1, 1, Qt::AlignLeft);

	gbox  = new QGroupBox( tr("Viewport Cursor") );
	gbox->setToolTip( tr("How the mouse cursor looks over the game area. Useful for light gun games.") );
	grid  = new QGridLayout();
	cursorSelect = new QComboBox();

	cursorSelect->addItem( tr("Arrow")  , 0 );
	cursorSelect->addItem( tr("Cross")  , 1 );
	cursorSelect->addItem( tr("Blank")  , 2 );
	cursorSelect->addItem( tr("Reticle 1x"), 3 );
	cursorSelect->addItem( tr("Reticle 2x"), 4 );

	setComboBoxFromProperty( cursorSelect, "SDL.CursorType" );

	connect(cursorSelect, SIGNAL(currentIndexChanged(int)), this, SLOT(cursorShapeChanged(int)) );

	vbox2->addWidget( gbox, 1 );
	gbox->setLayout(grid);

	grid->addWidget( tipLabel( tr("Shape:"), tr("The shape of the mouse cursor while it is over the game.") ), 0, 0, Qt::AlignLeft);
	grid->addWidget( cursorSelect, 0, 1, Qt::AlignLeft);

	cursorVisCbx = new QCheckBox( tr("Visible") );
	setCheckBoxFromProperty( cursorVisCbx, "SDL.CursorVis" );
	grid->addWidget( cursorVisCbx, 1, 0, 2, 1, Qt::AlignLeft);

	connect(cursorVisCbx    , SIGNAL(toggled(bool)), this, SLOT(cursorVisChanged(bool)) );

	vbox  = new QVBoxLayout();
	hbox  = new QHBoxLayout();
	gbox  = new QGroupBox( tr("Screen") );
	gbox->setToolTip( tr("Information about the screen that the window is on.") );
	gbox->setLayout( vbox );

	scrRateReadout = new QLineEdit();
	scrRateReadout->setFont( font );
	scrRateReadout->setReadOnly(true);
	scrRateReadout->setAlignment(Qt::AlignCenter);
	snprintf( stmp, sizeof(stmp), "%.3f", consoleWindow->getRefreshRate() );
	scrRateReadout->setText( tr(stmp) );

	hbox->addWidget( tipLabel( tr("Refresh Rate (Hz):"), tr("The refresh rate of the screen that the window is on.") ) );
	hbox->addWidget( scrRateReadout );
	vbox->addLayout( hbox );

	vbox2->addWidget( gbox );

	// Test Pattern
	gbox  = new QGroupBox( tr("Test Pattern:") );
	gbox->setToolTip( tr("Replace the game image with a pattern for checking the video output.") );
	gbox->setLayout( vbox5 );
	vbox5->addWidget( videoTest );
	vbox2->addWidget( gbox );

	vbox2->addStretch( 5 );

	setLayout( main_vbox );

	setupToolTips();

	updateReadouts();

	updateTimer  = new QTimer( this );

	connect( updateTimer, &QTimer::timeout, this, &ConsoleVideoConfDialog_t::periodicUpdate );

	updateTimer->start( 500 ); // 2Hz
}
//----------------------------------------------------
ConsoleVideoConfDialog_t::~ConsoleVideoConfDialog_t(void)
{
	//printf("Destroy Video Config Window\n");

	updateTimer->stop();
}
//----------------------------------------------------------------------------
void ConsoleVideoConfDialog_t::setupToolTips(void)
{
	driverSelect->setToolTip( tr("The renderer that draws the game image.\n"
		"OpenGL and Vulkan draw on the graphics card, SDL uses the SDL renderer and QPainter draws in software.\n"
		"SDL cannot be used on Wayland. A change takes effect right away.") );

	scalerSelect->setToolTip( tr("The filter applied to the game image before it is scaled to the window.\n"
		"hq2x, hq3x, Scale2x and Scale3x smooth the pixel art, NTSC 2x imitates composite video, "
		"Prescale 2x, 3x and 4x enlarge the image by whole numbers, and PAL 3x uses 3 times the horizontal "
		"resolution with the original vertical resolution.\n"
		"Takes effect when you press Apply.") );

	gl_LF_chkBox->setToolTip( tr("Smooth the image with a linear filter when it is scaled to the window, instead of keeping sharp pixel edges.") );

	regionSelect->setToolTip( tr("The video system the emulator runs as.\n"
		"NTSC runs at about 60 Hz, PAL at about 50 Hz, and Dendy is a PAL timed clone of the NES. "
		"Applies right away.") );

	autoRegion->setToolTip( tr("Choose NTSC or PAL from the game when it is loaded, instead of always using the selected Region.\n"
		"With Dendy selected, PAL games still run as PAL and NTSC games run as Dendy.") );

	new_PPU_ena->setToolTip( tr("Use the newer and more accurate PPU emulation. It is slower and is needed by a few games.\n"
		"The new PPU does not support overclocking, so overclocking is turned off when this is enabled.") );

	frmskipcbx->setToolTip( tr("Skip drawing some frames to keep up on a slow computer.\n"
		"Note: frame skipping is not compiled into this build, so this option currently has no effect.") );

	vsync_ena->setToolTip( tr("Wait for the screen refresh before showing each frame, which prevents tearing. "
		"It can add a little input delay. The OpenGL driver restarts when this changes.") );

	intFrameRateCbx->setToolTip( tr("Run at the nearest whole frame rate, for example 60 Hz instead of 60.0988 Hz. "
		"This can match a 60 Hz screen more smoothly with vertical sync. The sound is restarted when this changes.") );

	sprtLimCbx->setToolTip( tr("Remove the hardware limit of 8 sprites per scanline. This reduces sprite flicker, "
		"but a few games depend on the limit and may look different.") );

	autoScaleCbx->setToolTip( tr("Stretch the image to fill the window when the window is resized.\n"
		"When off, the image stays at the X and Y Scale below and the rest of the window shows the background color.") );

	aspectCbx->setToolTip( tr("Keep the aspect ratio of the image when it is scaled, adding bars at the sides or the top and bottom instead of stretching.\n"
		"The Aspect list picks the pixel shape, and one Scale is used for both directions.") );

	aspectSelect->setToolTip( tr("The shape of the image when Force Aspect Ratio is on.\n"
		"Square Pixels is 1:1, NTSC is 8:7 and PAL is 11:8, Standard is 4:3 and Widescreen is 16:9.") );
	aspectSelectLabel->setToolTip( aspectSelect->toolTip() );

	xScaleBox->setToolTip( tr("The size of the image as a multiple of the original NES resolution. "
		"With Force Aspect Ratio on, this one value is used for both directions.\n"
		"Takes effect when you press Apply, and also resizes the window.") );
	yScaleBox->setToolTip( tr("The vertical size of the image as a multiple of the original NES resolution. "
		"Only used when Force Aspect Ratio is off.\n"
		"Takes effect when you press Apply, and also resizes the window.") );
	xScaleLabel->setToolTip( xScaleBox->toolTip() );
	yScaleLabel->setToolTip( yScaleBox->toolTip() );

	// Legacy overlay: drawn into the game image by the emulator core
	showGuiMsgs_cbx->setToolTip( tr("Show emulator messages, such as saving or loading a state, over the game image. "
		"The messages go to the status bar instead when it is visible.") );

	drawInputAidsCbx->setToolTip( tr("Draw aids for light guns and other pointer devices, such as a crosshair where the cursor is.") );

	showFPS_cbx->setToolTip( tr("Draw the emulation speed in frames per second in the upper right of the game image.\n"
		"The same value is available in Window Text Overlay. Turning it on in both shows it twice.") );

	showFrameCount_cbx->setToolTip( tr("Draw the frame counter near the bottom of the game image. "
		"During a movie it also shows the movie length, the record mode and the read only state.\n"
		"The same value is available in Window Text Overlay. Turning it on in both shows it twice.") );

	showLagCount_cbx->setToolTip( tr("Draw the number of lag frames near the bottom of the game image. It is red while the game is lagging.\n"
		"The same value is available in Window Text Overlay. Turning it on in both shows it twice.") );

	showRerecordCount_cbx->setToolTip( tr("Draw the movie rerecord count near the bottom of the game image while a movie is loaded.\n"
		"The same value is available in Window Text Overlay. Turning it on in both shows it twice.") );

	inputDisplaySel->setToolTip( tr("Which controllers to show the pressed buttons of, drawn near the bottom of the game image.") );

	videoTest->setToolTip( tr("Vertical Sync shows a pattern for checking whether the screen tears. None shows the game.") );

	// Drawing area
	ntsc_start->setToolTip( tr("The first scanline of the picture that is shown for NTSC. The picture has 240 lines, numbered 0 to 239.") );
	ntsc_end->setToolTip( tr("The last scanline of the picture that is shown for NTSC. The picture has 240 lines, numbered 0 to 239.") );
	pal_start->setToolTip( tr("The first scanline of the picture that is shown for PAL and Dendy. The picture has 240 lines, numbered 0 to 239.") );
	pal_end->setToolTip( tr("The last scanline of the picture that is shown for PAL and Dendy. The picture has 240 lines, numbered 0 to 239.") );

	clipSidesCbx->setToolTip( tr("Hide the leftmost and the rightmost 8 pixels of the picture. "
		"Many games draw scrolling glitches there, and a real TV would have cut them off.") );

	winSizeReadout->setToolTip( tr("The size of the whole main window, in pixels.") );
	vpSizeReadout->setToolTip( tr("The size of the area that shows the game, in pixels.") );

	cursorSelect->setToolTip( tr("The shape of the mouse cursor over the game area. The reticles are useful for light gun games.") );
	cursorVisCbx->setToolTip( tr("Show the mouse cursor while it is over the game area.") );

	scrRateReadout->setToolTip( tr("The refresh rate of the screen that the window is on, in Hz.") );

	for ( QPushButton *btn : findChildren<QPushButton*>() )
	{
		if ( btn->text() == tr("Apply") )
		{
			btn->setToolTip( tr("Rebuild the video with the current settings, apply the scaler and the scale values, and resize the window.") );
		}
		else if ( btn->text() == tr("Close") )
		{
			btn->setToolTip( tr("Close this window. Options that are not marked as needing Apply are already in effect.") );
		}
	}
}
//----------------------------------------------------------------------------
void ConsoleVideoConfDialog_t::closeEvent(QCloseEvent *event)
{
	//printf("Video Config Close Window Event\n");
	done(0);
	deleteLater();
	event->accept();
}
//----------------------------------------------------------------------------
void ConsoleVideoConfDialog_t::periodicUpdate(void)
{
	int actRegion, selRegion;
       
	// Keep region menu selection sync'd to actual state
	actRegion = FCEUI_GetRegion();
	selRegion = regionSelect->currentIndex(); 

	if ( actRegion != selRegion )
	{
		regionSelect->setCurrentIndex(actRegion); 
	}

	setComboBoxFromValue( inputDisplaySel, input_display );

	   showFrameCount_cbx->setChecked( frame_display );
	     showLagCount_cbx->setChecked( lagCounterDisplay );
	showRerecordCount_cbx->setChecked( rerecord_display );

	// Update Window Size Readouts
	updateReadouts();
}
//----------------------------------------------------------------------------
void ConsoleVideoConfDialog_t::closeWindow(void)
{
	//printf("Video Config Close Window\n");
	done(0);
	deleteLater();
}
//----------------------------------------------------
void  ConsoleVideoConfDialog_t::resetVideo(void)
{
	FCEU_WRAPPER_SCOPED_LOCK();
	KillVideo ();
	InitVideo (GameInfo);
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::updateReadouts(void)
{
	if ( consoleWindow )
	{
		QSize w, v;
		char stmp[128];

		w = consoleWindow->size();

		if ( consoleWindow->viewport_Interface )
		{
			v = consoleWindow->viewport_Interface->size();
		}

		snprintf( stmp, sizeof(stmp), "%i x %i ", w.width(), w.height() );

		winSizeReadout->setText( tr(stmp) );

		snprintf( stmp, sizeof(stmp), "%i x %i ", v.width(), v.height() );

		vpSizeReadout->setText( tr(stmp) );
	}
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::ntscStartScanLineChanged(int value)
{
	int opt, opt2;

	opt = value;

	g_config->getOption("SDL.ScanLineEndNTSC", &opt2);

	if ( opt > opt2 )
	{
		opt = opt2;
	}
	g_config->setOption("SDL.ScanLineStartNTSC", opt);

	ntsc_start->setRange(  0, opt2 );
	  ntsc_end->setRange( opt, 239 );
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::ntscEndScanLineChanged(int value)
{
	int opt, opt2;

	opt = value;

	g_config->getOption("SDL.ScanLineStartNTSC", &opt2);

	if ( opt < opt2 )
	{
		opt = opt2;
	}
	g_config->setOption("SDL.ScanLineEndNTSC", opt);

	ntsc_start->setRange(    0, opt );
	  ntsc_end->setRange( opt2, 239 );
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::palStartScanLineChanged(int value)
{
	int opt, opt2;

	opt = value;

	g_config->getOption("SDL.ScanLineEndPAL", &opt2);

	if ( opt > opt2 )
	{
		opt = opt2;
	}
	g_config->setOption("SDL.ScanLineStartPAL", opt);

	pal_start->setRange(  0, opt2 );
	  pal_end->setRange( opt, 239 );
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::palEndScanLineChanged(int value)
{
	int opt, opt2;

	opt = value;

	g_config->getOption("SDL.ScanLineStartPAL", &opt2);

	if ( opt < opt2 )
	{
		opt = opt2;
	}
	g_config->setOption("SDL.ScanLineEndPAL", opt);

	pal_start->setRange(    0, opt );
	  pal_end->setRange( opt2, 239 );
}
//----------------------------------------------------
void  ConsoleVideoConfDialog_t::setCheckBoxFromProperty( QCheckBox *cbx, const char *property )
{
	int  pval;
	g_config->getOption (property, &pval);

	cbx->setCheckState( pval ? Qt::Checked : Qt::Unchecked );
}
//----------------------------------------------------
void  ConsoleVideoConfDialog_t::setComboBoxFromProperty( QComboBox *cbx, const char *property )
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
void  ConsoleVideoConfDialog_t::setComboBoxFromValue( QComboBox *cbx, int pval )
{
	for (int i=0; i<cbx->count(); i++)
	{
		if ( pval == cbx->itemData(i).toInt() )
		{
			cbx->setCurrentIndex(i); break;
		}
	}
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::openGL_linearFilterChanged( bool value )
{
   bool opt =  (value);
   g_config->setOption("SDL.OpenGLip", opt );
   g_config->save();

   if ( consoleWindow != NULL )
   {
      if ( consoleWindow->viewport_Interface )
      {
         consoleWindow->viewport_Interface->setLinearFilterEnable( opt );
      }
   }
}
//----------------------------------------------------
#if defined(WIN32) && (QT_VERSION_MAJOR < 6)
void ConsoleVideoConfDialog_t::winFullScreenBorderChanged(bool value)
{
	bool opt = (value);
	// This function is needed to fix the issue referenced below. It adds a 1-pixel border
	// around the fullscreen window due to some limitation in windows.
	// https://doc.qt.io/qt-5/windows-issues.html#fullscreen-opengl-based-windows
	QWindowsWindowFunctions::setHasBorderInFullScreen( consoleWindow->windowHandle(), opt);

	g_config->setOption("SDL.winFullScreenBorder", opt );
	g_config->save();
}
#endif
//----------------------------------------------------
void ConsoleVideoConfDialog_t::autoScaleChanged( bool value )
{
   bool opt =  (value);
   g_config->setOption("SDL.AutoScale", opt );
   g_config->save ();

   if ( consoleWindow != NULL )
   {
      if ( consoleWindow->viewport_Interface )
      {
         consoleWindow->viewport_Interface->setAutoScaleOpt( opt );
      }
   }
}

//----------------------------------------------------
void ConsoleVideoConfDialog_t::autoRegionChanged( bool value )
{
	//printf("Value:%i \n", value );
	g_config->setOption("SDL.AutoDetectPAL", (value) );
	g_config->save ();
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::use_new_PPU_changed( bool value )
{
	bool reqNewPPU;

	reqNewPPU = value;

	if ( reqNewPPU )
	{
		if ( overclock_enabled )
		{
			QMessageBox::StandardButton ret;
			const char *msg = "The new PPU does not support overclocking. This will be disabled. Do you wish to continue?";

			ret =  QMessageBox::question( this, tr("Overclocking"), tr(msg) );

			if ( ret == QMessageBox::No )
			{
				//printf("Skipping New PPU Activation\n");
				new_PPU_ena->setChecked(false);
				return;
			}
		}
	}

	//printf("NEW PPU Value:%i \n", reqNewPPU );
	FCEU_WRAPPER_SCOPED_LOCK();

	newppu = reqNewPPU;

	if ( newppu )
	{	
		overclock_enabled = 0;
	}

	g_config->setOption("SDL.NewPPU", newppu );
	g_config->setOption("SDL.OverClockEnable", overclock_enabled );
	g_config->save ();

	UpdateEMUCore (g_config);
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::frameskip_changed( bool value )
{
	//printf("Value:%i \n", value );
	g_config->setOption("SDL.Frameskip", (value) );
	g_config->save ();

	FCEU_WRAPPER_SCOPED_LOCK();
	UpdateEMUCore (g_config);
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::vsync_changed( bool value )
{
	//printf("Value:%i \n", value );
	bool opt =  (value);
	g_config->setOption("SDL.VideoVsync", opt );
	g_config->save ();

	if ( consoleWindow != NULL )
	{
		if ( consoleWindow->viewport_GL )
		{
			// QOpenGLWidget required full driver reset
			//consoleWindow->viewport_GL->setVsyncEnable( opt );
			consoleWindow->loadVideoDriver( 0, true );
		}
		else if ( consoleWindow->viewport_Interface )
		{
			consoleWindow->viewport_Interface->setVsyncEnable( opt );
		}
	}
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::intFrameRate_changed( bool value )
{
	//printf("Value:%i \n", value );
	useIntFrameRate = (value);
	g_config->setOption("SDL.IntFrameRate", useIntFrameRate );
	g_config->save ();

	FCEU_WRAPPER_SCOPED_LOCK();
	RefreshThrottleFPS();
	KillSound();
	InitSound();
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::useSpriteLimitChanged( bool value )
{
	//printf("Value:%i \n", value );
	g_config->setOption("SDL.DisableSpriteLimit", (value) );
	g_config->save ();

	FCEU_WRAPPER_SCOPED_LOCK();
	UpdateEMUCore (g_config);
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::clipSidesChanged( bool value )
{
	//printf("Value:%i \n", value );
	g_config->setOption("SDL.ClipSides", (value) );
	g_config->save ();

	FCEU_WRAPPER_SCOPED_LOCK();
	UpdateEMUCore (g_config);
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::showFPSChanged( bool value )
{
	//printf("Value:%i \n", value );
	g_config->setOption("SDL.ShowFPS", (value) );
	g_config->save ();

	FCEU_WRAPPER_SCOPED_LOCK();
	FCEUI_SetShowFPS( (value) );
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::showGuiMsgsChanged( bool value )
{
	//printf("Value:%i \n", value );
	vidGuiMsgEna = (value);
	g_config->setOption("SDL.ShowGuiMessages", vidGuiMsgEna );
	g_config->save ();
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::aspectEnableChanged( bool value )
{
	//printf("Value:%i \n", value );
	int forceAspect = (value);

	if ( forceAspect )
	{
		xScaleLabel->setText( tr("Scale:") );
		yScaleLabel->hide();
		yScaleBox->hide();

		aspectSelectLabel->show();
		aspectSelect->show();
	}
	else
	{
		xScaleLabel->setText( tr("X Scale:") );
		yScaleLabel->show();
		yScaleBox->show();

		aspectSelectLabel->hide();
		aspectSelect->hide();
	}
	g_config->setOption ("SDL.ForceAspect", forceAspect);

}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::driverChanged(int index)
{
	int driver;
	//printf("Driver: %i : %i \n", index, driverSelect->itemData(index).toInt() );

	driver = driverSelect->itemData(index).toInt();

#if (defined(__linux__) || defined(__unix__)) && !defined(__APPLE__)
	if (driver == ConsoleViewerBase::VIDEO_DRIVER_SDL &&
	    QGuiApplication::platformName() == QLatin1String("wayland"))
	{
		QMessageBox::warning(
			this,
			tr("SDL video driver"),
			tr("The SDL video driver cannot embed into the Qt window on Wayland "
			   "(SDL uses X11 here, but this window is not an X11 window). "
			   "FCEUX will use QPainter instead.\n\n"
			   "To use the SDL driver you can run FCEUX with the X11 Qt platform "
			   "plugin, for example:\n"
			   "QT_QPA_PLATFORM=xcb fceux"));

		driver = ConsoleViewerBase::VIDEO_DRIVER_QPAINTER;
		driverSelect->blockSignals(true);
		for (int i = 0; i < driverSelect->count(); i++)
		{
			if (driverSelect->itemData(i).toInt() == driver)
			{
				driverSelect->setCurrentIndex(i);
				break;
			}
		}
		driverSelect->blockSignals(false);
	}
#endif

	g_config->setOption ("SDL.VideoDriver", driver);

	g_config->save ();

	printf("Note: A restart of the application is needed for video driver change to take effect...\n");

	if ( consoleWindow )
	{
		consoleWindow->loadVideoDriver( driver );
	}
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::scalerChanged(int index)
{
	int scaler;
	//printf("Scaler: %i : %i \n", index, scalerSelect->itemData(index).toInt() );

	scaler = scalerSelect->itemData(index).toInt();

	g_config->setOption ("SDL.SpecialFilter", scaler);

	g_config->save ();
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::regionChanged(int index)
{
	int region;
	int actRegion = FCEUI_GetRegion();
	//printf("Region: %i : %i \n", index, regionSelect->itemData(index).toInt() );

	region = regionSelect->itemData(index).toInt();

	g_config->setOption ("SDL.PAL", region);
	g_config->save ();

	// reset sound subsystem for changes to take effect
	if ( actRegion != region )
	{
		FCEU_WRAPPER_SCOPED_LOCK();
		FCEUI_SetRegion (region, true);
	}
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::inputDisplayChanged(int index)
{
	//printf("Scaler: %i : %i \n", index, scalerSelect->itemData(index).toInt() );
	{
		FCEU_WRAPPER_SCOPED_LOCK();
		input_display = inputDisplaySel->itemData(index).toInt();
	}

	g_config->setOption ("SDL.InputDisplay", input_display);
	g_config->save ();
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::testPatternChanged(int index)
{
	nes_shm->video.test = videoTest->itemData(index).toInt();
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::aspectChanged(int index)
{
	int aspectID;

	aspectID = aspectSelect->itemData(index).toInt();

	g_config->setOption ("SDL.AspectSelect", aspectID);
	g_config->save ();

	consoleWindow->setViewportAspect();
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::cursorShapeChanged(int index)
{
	int cursorSel;
	//printf("Scaler: %i : %i \n", index, scalerSelect->itemData(index).toInt() );

	cursorSel = cursorSelect->itemData(index).toInt();

	g_config->setOption ("SDL.CursorType", cursorSel);

	g_config->save ();

	consoleWindow->loadCursor();
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::cursorVisChanged( bool value )
{
	int vis;

	vis = (value);

	//printf("Value:%i \n", value );
	g_config->setOption("SDL.CursorVis", vis );
	g_config->save ();

	consoleWindow->loadCursor();
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::drawInputAidsChanged( bool value )
{
	int draw;

	draw = (value);

	//printf("Value:%i \n", value );
	g_config->setOption("SDL.DrawInputAids", draw );
	g_config->save ();

	FCEU_WRAPPER_SCOPED_LOCK();
	drawInputAidsEnable = draw;
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::showFrameCountChanged( bool value )
{
	{
		FCEU_WRAPPER_SCOPED_LOCK();
		frame_display = (value);
	}

	//printf("Value:%i \n", value );
	g_config->setOption("SDL.ShowFrameCount", frame_display );
	g_config->save ();

}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::textOverlayItemChanged( int item, bool checked )
{
	if ( consoleWindow && consoleWindow->getTextOverlayDisplay() )
	{
		consoleWindow->getTextOverlayDisplay()->setEnabled( static_cast<TextOverlayDisplay_t::Item>(item), checked );
	}
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::textOverlayRateChanged( int index )
{
	if ( consoleWindow && consoleWindow->getTextOverlayDisplay() && (index >= 0) )
	{
		consoleWindow->getTextOverlayDisplay()->setUpdateRate( textOverlayRateSel->itemData(index).toInt() );
	}
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::updateTextOverlayControls(void)
{
	TextOverlayDisplay_t *display = consoleWindow ? consoleWindow->getTextOverlayDisplay() : nullptr;

	for (int i = 0; i < TextOverlayDisplay_t::ITEM_COUNT; i++)
	{
		QSignalBlocker blocker( textOverlayCbx[i] );
		textOverlayCbx[i]->setChecked( display && display->isEnabled( static_cast<TextOverlayDisplay_t::Item>(i) ) );
		textOverlayCbx[i]->setEnabled( display != nullptr );
	}
	QSignalBlocker blocker( textOverlayRateSel );
	setComboBoxFromValue( textOverlayRateSel, display ? display->getUpdateRate() : TextOverlayDisplay_t::DEFAULT_UPDATE_RATE_HZ );
	textOverlayRateSel->setEnabled( display != nullptr );
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::showLagCountChanged( bool value )
{
	{
		FCEU_WRAPPER_SCOPED_LOCK();
		lagCounterDisplay = (value);
	}

	//printf("Value:%i \n", value );
	g_config->setOption("SDL.ShowLagCount", lagCounterDisplay );
	g_config->save ();

}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::showRerecordCountChanged( bool value )
{
	{
		FCEU_WRAPPER_SCOPED_LOCK();
		rerecord_display = (value);
	}

	//printf("Value:%i \n", value );
	g_config->setOption("SDL.ShowRerecordCount", rerecord_display );
	g_config->save ();

}
//----------------------------------------------------
QSize ConsoleVideoConfDialog_t::calcNewScreenSize(void)
{
	QSize out( GL_NES_WIDTH, GL_NES_HEIGHT );

	if ( consoleWindow )
	{
		QSize w, v;
		double xscale = 1.0, yscale = 1.0, aspectRatio = 1.0;
		int texture_width  = nes_shm->video.ncol;
		int texture_height = nes_shm->video.nrow;
		int l=0, r=texture_width;
		int t=0, b=texture_height;
		int dw=0, dh=0, rw, rh;

		w = consoleWindow->size();

		if ( consoleWindow->viewport_Interface )
		{
			v = consoleWindow->viewport_Interface->size();
			aspectRatio = consoleWindow->viewport_Interface->getAspectRatio();
		}

		dw = w.width()  - v.width();
		dh = w.height() - v.height();

		if ( aspectCbx->isChecked() )
		{
			xscale = xScaleBox->value() / nes_shm->video.xscale;
			yscale = xscale * (double)nes_shm->video.xyRatio;
		}
		else
		{
			xscale = xScaleBox->value() / nes_shm->video.xscale;
			yscale = yScaleBox->value() / nes_shm->video.yscale;
		}
		rw=(int)((r-l)*xscale);
		rh=(int)((b-t)*yscale);

		if ( aspectCbx->isChecked() )
		{
			double rr;

			rr = (double)rh / (double)rw;

			if ( rr > aspectRatio )
			{
				rw = (int)( (((double)rh) / aspectRatio) + 0.50);
			}
			else
			{
				rh = (int)( (((double)rw) * aspectRatio) + 0.50);
			}
		}
		out.setWidth( rw + dw );
		out.setHeight( rh + dh );
	}
	return out;
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::applyChanges( void )
{ 
	resetVideo();

	if ( consoleWindow )
	{
		float xscale, yscale;
		QSize s = calcNewScreenSize();

		if ( aspectCbx->isChecked() )
		{
			yscale = xscale = xScaleBox->value();
		}
		else
		{
			xscale = xScaleBox->value();
			yscale = yScaleBox->value();
		}

		// Save desired scaling and window size to config.
		g_config->setOption("SDL.XScale", xscale );
		g_config->setOption("SDL.YScale", yscale );
		g_config->setOption("SDL.WinSizeX", s.width() );
		g_config->setOption("SDL.WinSizeY", s.height() );

		if ( consoleWindow->viewport_Interface )
		{
         		consoleWindow->viewport_Interface->setLinearFilterEnable( gl_LF_chkBox->isChecked() );
			consoleWindow->viewport_Interface->setForceAspectOpt( aspectCbx->isChecked() );
			consoleWindow->viewport_Interface->setAutoScaleOpt( autoScaleCbx->isChecked() );
			consoleWindow->viewport_Interface->setScaleXY( xscale, yscale );
			consoleWindow->viewport_Interface->reset();
		}

		if ( !consoleWindow->isFullScreen() && !consoleWindow->isMaximized() )
		{
			consoleWindow->resize( s );
		}

		updateReadouts();
	}

}
//----------------------------------------------------
