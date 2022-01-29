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
#include <QMessageBox>

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
#include "Qt/nes_shm.h"

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
	lbl = new QLabel( tr("Driver:") );

	driverSelect = new QComboBox();

	driverSelect->addItem( tr("OpenGL"), 0 );
	driverSelect->addItem( tr("SDL"), 1 );
	
	hbox1 = new QHBoxLayout();

	hbox1->addWidget( lbl );
	hbox1->addWidget( driverSelect );

	vbox1->addLayout( hbox1 );

	// Video Driver Select
	lbl = new QLabel( tr("Scaler:") );

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

	// Enable OpenGL Linear Filter Checkbox
	gl_LF_chkBox  = new QCheckBox( tr("Enable OpenGL Linear Filter") );

	setCheckBoxFromProperty( gl_LF_chkBox  , "SDL.OpenGLip");

	connect(gl_LF_chkBox , SIGNAL(stateChanged(int)), this, SLOT(openGL_linearFilterChanged(int)) );

	vbox1->addWidget( gl_LF_chkBox );

	// Region Select
	lbl = new QLabel( tr("Region:") );

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
	showFPS_cbx  = new QCheckBox( tr("Frames Per Second") );

	// Show Frame Count Checkbox
	showFrameCount_cbx  = new QCheckBox( tr("Frame Count") );
	showFrameCount_cbx->setChecked( frame_display );

	// Show Lag Count Checkbox
	showLagCount_cbx  = new QCheckBox( tr("Lag Count") );
	showLagCount_cbx->setChecked( lagCounterDisplay );

	// Show Re-Record Count Checkbox
	showRerecordCount_cbx  = new QCheckBox( tr("Re-Record Count") );
	showRerecordCount_cbx->setChecked( rerecord_display );

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
		if ( consoleWindow->viewport_GL )
		{
			autoScaleCbx->setChecked( consoleWindow->viewport_GL->getAutoScaleOpt() );
			aspectCbx->setChecked( consoleWindow->viewport_GL->getForceAspectOpt() );
		}
		else if ( consoleWindow->viewport_SDL )
		{
			autoScaleCbx->setChecked( consoleWindow->viewport_SDL->getAutoScaleOpt() );
			aspectCbx->setChecked( consoleWindow->viewport_SDL->getForceAspectOpt() );
		}
	}

	connect(new_PPU_ena     , SIGNAL(clicked(bool))    , this, SLOT(use_new_PPU_changed(bool)) );
	connect(autoRegion      , SIGNAL(stateChanged(int)), this, SLOT(autoRegionChanged(int)) );
	connect(frmskipcbx      , SIGNAL(stateChanged(int)), this, SLOT(frameskip_changed(int)) );
	connect(vsync_ena       , SIGNAL(stateChanged(int)), this, SLOT(vsync_changed(int)) );
	connect(intFrameRateCbx , SIGNAL(stateChanged(int)), this, SLOT(intFrameRate_changed(int)) );
	connect(sprtLimCbx      , SIGNAL(stateChanged(int)), this, SLOT(useSpriteLimitChanged(int)) );
	connect(clipSidesCbx    , SIGNAL(stateChanged(int)), this, SLOT(clipSidesChanged(int)) );
	connect(showFPS_cbx     , SIGNAL(stateChanged(int)), this, SLOT(showFPSChanged(int)) );
	connect(showGuiMsgs_cbx , SIGNAL(stateChanged(int)), this, SLOT(showGuiMsgsChanged(int)) );
	connect(aspectCbx       , SIGNAL(stateChanged(int)), this, SLOT(aspectEnableChanged(int)) );
	connect(autoScaleCbx    , SIGNAL(stateChanged(int)), this, SLOT(autoScaleChanged(int)) );
	connect(drawInputAidsCbx, SIGNAL(stateChanged(int)), this, SLOT(drawInputAidsChanged(int)) );

	connect(   showFrameCount_cbx, SIGNAL(stateChanged(int)), this, SLOT(showFrameCountChanged(int))   );
	connect(     showLagCount_cbx, SIGNAL(stateChanged(int)), this, SLOT(showLagCountChanged(int))     );
	connect(showRerecordCount_cbx, SIGNAL(stateChanged(int)), this, SLOT(showRerecordCountChanged(int)));

	connect(inputDisplaySel, SIGNAL(currentIndexChanged(int)), this, SLOT(inputDisplayChanged(int)) );

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
		if ( consoleWindow->viewport_GL )
		{
			xScaleBox->setValue( consoleWindow->viewport_GL->getScaleX() );
			yScaleBox->setValue( consoleWindow->viewport_GL->getScaleY() );
		}
		else if ( consoleWindow->viewport_SDL )
		{
			xScaleBox->setValue( consoleWindow->viewport_SDL->getScaleX() );
			yScaleBox->setValue( consoleWindow->viewport_SDL->getScaleY() );
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

	gbox  = new QGroupBox( tr("Overlay Options") );
	vbox3 = new QVBoxLayout();
	vbox4 = new QVBoxLayout();
	vbox5 = new QVBoxLayout();
	vbox  = new QVBoxLayout();

	vbox3->addWidget( gbox, 1 );
	vbox3->addStretch(5);
	 gbox->setLayout( vbox );

	vbox->addWidget( showGuiMsgs_cbx );
	vbox->addWidget( drawInputAidsCbx );
	vbox->addWidget( showFPS_cbx );
	vbox->addWidget( showFrameCount_cbx );
	vbox->addWidget( showLagCount_cbx );
	vbox->addWidget( showRerecordCount_cbx );

	gbox  = new QGroupBox( tr("Show Controllers:") );
	gbox->setLayout( vbox4 );
	vbox->addWidget( gbox  );
	vbox4->addWidget( inputDisplaySel );

	gbox  = new QGroupBox( tr("Test Pattern:") );
	gbox->setLayout( vbox5 );
	vbox->addWidget( gbox  );
	vbox5->addWidget( videoTest );

	gbox  = new QGroupBox( tr("Drawing Area") );
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

	grid->addWidget( new QLabel( tr("NTSC") )      , 0, 1, Qt::AlignLeft);
	grid->addWidget( new QLabel( tr("PAL/Dendy") ) , 0, 2, Qt::AlignLeft);
	grid->addWidget( new QLabel( tr("First Line:") ), 1, 0, Qt::AlignLeft);
	grid->addWidget( new QLabel( tr("Last Line:")  ), 2, 0, Qt::AlignLeft);
	grid->addWidget( ntsc_start, 1, 1, Qt::AlignLeft);
	grid->addWidget( pal_start , 1, 2, Qt::AlignLeft);
	grid->addWidget( ntsc_end  , 2, 1, Qt::AlignLeft);
	grid->addWidget( pal_end   , 2, 2, Qt::AlignLeft);
	grid->addWidget( clipSidesCbx, 3, 0, 1, 3);

	gbox  = new QGroupBox( tr("Current Dimensions") );
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

	grid->addWidget( new QLabel( tr("Window:") ), 0, 0, Qt::AlignLeft);
	grid->addWidget( new QLabel( tr("Viewport:") ), 1, 0, Qt::AlignLeft);
	grid->addWidget( winSizeReadout, 0, 1, Qt::AlignLeft);
	grid->addWidget( vpSizeReadout, 1, 1, Qt::AlignLeft);

	gbox  = new QGroupBox( tr("Viewport Cursor") );
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

	grid->addWidget( new QLabel( tr("Shape:") ), 0, 0, Qt::AlignLeft);
	grid->addWidget( cursorSelect, 0, 1, Qt::AlignLeft);

	cursorVisCbx = new QCheckBox( tr("Visible") );
	setCheckBoxFromProperty( cursorVisCbx, "SDL.CursorVis" );
	grid->addWidget( cursorVisCbx, 1, 0, 2, 1, Qt::AlignLeft);

	connect(cursorVisCbx    , SIGNAL(stateChanged(int)), this, SLOT(cursorVisChanged(int)) );

	vbox  = new QVBoxLayout();
	hbox  = new QHBoxLayout();
	gbox  = new QGroupBox( tr("Screen") );
	gbox->setLayout( vbox );

	scrRateReadout = new QLineEdit();
	scrRateReadout->setFont( font );
	scrRateReadout->setReadOnly(true);
	scrRateReadout->setAlignment(Qt::AlignCenter);
	sprintf( stmp, "%.3f", consoleWindow->getRefreshRate() );
	scrRateReadout->setText( tr(stmp) );

	hbox->addWidget( new QLabel( tr("Refresh Rate (Hz):") ) );
	hbox->addWidget( scrRateReadout );
	vbox->addLayout( hbox );

	vbox2->addWidget( gbox );
	vbox2->addStretch( 5 );

	setLayout( main_vbox );

	updateReadouts();

	updateTimer  = new QTimer( this );

	connect( updateTimer, &QTimer::timeout, this, &ConsoleVideoConfDialog_t::periodicUpdate );

	updateTimer->start( 500 ); // 2Hz
}
//----------------------------------------------------
ConsoleVideoConfDialog_t::~ConsoleVideoConfDialog_t(void)
{
	printf("Destroy Video Config Window\n");

	updateTimer->stop();
}
//----------------------------------------------------------------------------
void ConsoleVideoConfDialog_t::closeEvent(QCloseEvent *event)
{
	printf("Video Config Close Window Event\n");
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
	FCEU_WRAPPER_LOCK();
	KillVideo ();
	InitVideo (GameInfo);
	FCEU_WRAPPER_UNLOCK();
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::updateReadouts(void)
{
	if ( consoleWindow )
	{
		QSize w, v;
		char stmp[128];

		w = consoleWindow->size();

		if ( consoleWindow->viewport_GL )
		{
			v = consoleWindow->viewport_GL->size();
		}
		else if ( consoleWindow->viewport_SDL )
		{
			v = consoleWindow->viewport_SDL->size();
		}

		sprintf( stmp, "%i x %i ", w.width(), w.height() );

		winSizeReadout->setText( tr(stmp) );

		sprintf( stmp, "%i x %i ", v.width(), v.height() );

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
void ConsoleVideoConfDialog_t::openGL_linearFilterChanged( int value )
{
   bool opt =  (value != Qt::Unchecked);
   g_config->setOption("SDL.OpenGLip", opt );
	g_config->save ();

   if ( consoleWindow != NULL )
   {
      if ( consoleWindow->viewport_GL )
      {
         consoleWindow->viewport_GL->setLinearFilterEnable( opt );
      }
      if ( consoleWindow->viewport_SDL )
      {
         consoleWindow->viewport_SDL->setLinearFilterEnable( opt );
      }
   }
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::autoScaleChanged( int value )
{
   bool opt =  (value != Qt::Unchecked);
   g_config->setOption("SDL.AutoScale", opt );
   g_config->save ();

   if ( consoleWindow != NULL )
   {
      if ( consoleWindow->viewport_GL )
      {
         consoleWindow->viewport_GL->setAutoScaleOpt( opt );
      }
      if ( consoleWindow->viewport_SDL )
      {
         consoleWindow->viewport_SDL->setAutoScaleOpt( opt );
      }
   }
}

//----------------------------------------------------
void ConsoleVideoConfDialog_t::autoRegionChanged( int value )
{
	//printf("Value:%i \n", value );
	g_config->setOption("SDL.AutoDetectPAL", (value == Qt::Checked) );
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
	FCEU_WRAPPER_LOCK();

	newppu = reqNewPPU;

	if ( newppu )
	{	
		overclock_enabled = 0;
	}

	g_config->setOption("SDL.NewPPU", newppu );
	g_config->setOption("SDL.OverClockEnable", overclock_enabled );
	g_config->save ();

	UpdateEMUCore (g_config);
	FCEU_WRAPPER_UNLOCK();
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::frameskip_changed( int value )
{
	//printf("Value:%i \n", value );
	g_config->setOption("SDL.Frameskip", (value == Qt::Checked) );
	g_config->save ();

	FCEU_WRAPPER_LOCK();
	UpdateEMUCore (g_config);
	FCEU_WRAPPER_UNLOCK();
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::vsync_changed( int value )
{
	//printf("Value:%i \n", value );
	bool opt =  (value != Qt::Unchecked);
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
		if ( consoleWindow->viewport_SDL )
		{
			consoleWindow->viewport_SDL->setVsyncEnable( opt );
		}
	}
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::intFrameRate_changed( int value )
{
	//printf("Value:%i \n", value );
	useIntFrameRate = (value != Qt::Unchecked);
	g_config->setOption("SDL.IntFrameRate", useIntFrameRate );
	g_config->save ();

	FCEU_WRAPPER_LOCK();
	RefreshThrottleFPS();
	KillSound();
	InitSound();
	FCEU_WRAPPER_UNLOCK();
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::useSpriteLimitChanged( int value )
{
	//printf("Value:%i \n", value );
	g_config->setOption("SDL.DisableSpriteLimit", (value == Qt::Checked) );
	g_config->save ();

	FCEU_WRAPPER_LOCK();
	UpdateEMUCore (g_config);
	FCEU_WRAPPER_UNLOCK();
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::clipSidesChanged( int value )
{
	//printf("Value:%i \n", value );
	g_config->setOption("SDL.ClipSides", (value == Qt::Checked) );
	g_config->save ();

	FCEU_WRAPPER_LOCK();
	UpdateEMUCore (g_config);
	FCEU_WRAPPER_UNLOCK();
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::showFPSChanged( int value )
{
	//printf("Value:%i \n", value );
	g_config->setOption("SDL.ShowFPS", (value == Qt::Checked) );
	g_config->save ();

	FCEU_WRAPPER_LOCK();
	FCEUI_SetShowFPS( (value == Qt::Checked) );
	FCEU_WRAPPER_UNLOCK();
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::showGuiMsgsChanged( int value )
{
	//printf("Value:%i \n", value );
	vidGuiMsgEna = (value == Qt::Checked);
	g_config->setOption("SDL.ShowGuiMessages", vidGuiMsgEna );
	g_config->save ();
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::aspectEnableChanged( int value )
{
	//printf("Value:%i \n", value );
	int forceAspect = (value != Qt::Unchecked);

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
		FCEU_WRAPPER_LOCK();
		FCEUI_SetRegion (region, true);
		FCEU_WRAPPER_UNLOCK();
	}
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::inputDisplayChanged(int index)
{
	//printf("Scaler: %i : %i \n", index, scalerSelect->itemData(index).toInt() );
	FCEU_WRAPPER_LOCK();
	input_display = inputDisplaySel->itemData(index).toInt();
	FCEU_WRAPPER_UNLOCK();

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
void ConsoleVideoConfDialog_t::cursorVisChanged( int value )
{
	int vis;

	vis = (value != Qt::Unchecked);

	//printf("Value:%i \n", value );
	g_config->setOption("SDL.CursorVis", vis );
	g_config->save ();

	consoleWindow->loadCursor();
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::drawInputAidsChanged( int value )
{
	int draw;

	draw = (value != Qt::Unchecked);

	//printf("Value:%i \n", value );
	g_config->setOption("SDL.DrawInputAids", draw );
	g_config->save ();

	FCEU_WRAPPER_LOCK();
	drawInputAidsEnable = draw;
	FCEU_WRAPPER_UNLOCK();
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::showFrameCountChanged( int value )
{
	FCEU_WRAPPER_LOCK();
	frame_display = (value != Qt::Unchecked);
	FCEU_WRAPPER_UNLOCK();

	//printf("Value:%i \n", value );
	g_config->setOption("SDL.ShowFrameCount", frame_display );
	g_config->save ();

}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::showLagCountChanged( int value )
{
	FCEU_WRAPPER_LOCK();
	lagCounterDisplay = (value != Qt::Unchecked);
	FCEU_WRAPPER_UNLOCK();

	//printf("Value:%i \n", value );
	g_config->setOption("SDL.ShowLagCount", lagCounterDisplay );
	g_config->save ();

}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::showRerecordCountChanged( int value )
{
	FCEU_WRAPPER_LOCK();
	rerecord_display = (value != Qt::Unchecked);
	FCEU_WRAPPER_UNLOCK();

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

		if ( consoleWindow->viewport_GL )
		{
			v = consoleWindow->viewport_GL->size();
			aspectRatio = consoleWindow->viewport_GL->getAspectRatio();
		}
		else if ( consoleWindow->viewport_SDL )
		{
			v = consoleWindow->viewport_SDL->size();
			aspectRatio = consoleWindow->viewport_SDL->getAspectRatio();
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

		if ( consoleWindow->viewport_GL )
		{
         		consoleWindow->viewport_GL->setLinearFilterEnable( gl_LF_chkBox->isChecked() );
			consoleWindow->viewport_GL->setForceAspectOpt( aspectCbx->isChecked() );
			consoleWindow->viewport_GL->setAutoScaleOpt( autoScaleCbx->isChecked() );
			consoleWindow->viewport_GL->setScaleXY( xscale, yscale );
			consoleWindow->viewport_GL->reset();
		}
		if ( consoleWindow->viewport_SDL )
		{
         		consoleWindow->viewport_SDL->setLinearFilterEnable( gl_LF_chkBox->isChecked() );
			consoleWindow->viewport_SDL->setForceAspectOpt( aspectCbx->isChecked() );
			consoleWindow->viewport_SDL->setAutoScaleOpt( autoScaleCbx->isChecked() );
			consoleWindow->viewport_SDL->setScaleXY( xscale, yscale );
			consoleWindow->viewport_SDL->reset();
		}

		if ( !consoleWindow->isFullScreen() && !consoleWindow->isMaximized() )
		{
			consoleWindow->resize( s );
		}

		updateReadouts();
	}

}
//----------------------------------------------------
