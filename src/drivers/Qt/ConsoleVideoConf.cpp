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

#include "../../fceu.h"
#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/config.h"
#include "Qt/fceuWrapper.h"
#include "Qt/ConsoleWindow.h"
#include "Qt/ConsoleUtilities.h"
#include "Qt/ConsoleVideoConf.h"
#include "Qt/nes_shm.h"

//----------------------------------------------------
ConsoleVideoConfDialog_t::ConsoleVideoConfDialog_t(QWidget *parent)
	: QDialog( parent )
{
	QVBoxLayout *main_vbox, *vbox1, *vbox2;
	QHBoxLayout *main_hbox, *hbox1;
	QLabel *lbl;
	QPushButton *button;
	QStyle *style;
	QGroupBox *gbox;
	QGridLayout *grid;
	fceuDecIntValidtor *validator;
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
	scalerSelect->addItem( tr("PAL"), 9 );
	
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

	// Enable New PPU Checkbox
	frmskipcbx  = new QCheckBox( tr("Enable Frameskip") );

	// Disable Sprite Limit Checkbox
	sprtLimCbx  = new QCheckBox( tr("Disable Sprite Limit") );

	// Clip Sides Checkbox
	clipSidesCbx  = new QCheckBox( tr("Clip Left/Right Sides (8 px on each)") );

	// Show FPS Checkbox
	showFPS_cbx  = new QCheckBox( tr("Show FPS") );

	// Auto Scale on Resize
	autoScaleCbx  = new QCheckBox( tr("Auto Scale on Resize") );

	// Square Pixels
	sqrPixCbx  = new QCheckBox( tr("Square Pixels") );

	setCheckBoxFromProperty( autoRegion   , "SDL.AutoDetectPAL");
	setCheckBoxFromProperty( new_PPU_ena  , "SDL.NewPPU");
	setCheckBoxFromProperty( frmskipcbx   , "SDL.Frameskip");
	setCheckBoxFromProperty( sprtLimCbx   , "SDL.DisableSpriteLimit");
	setCheckBoxFromProperty( clipSidesCbx , "SDL.ClipSides");
	setCheckBoxFromProperty( showFPS_cbx  , "SDL.ShowFPS");
	
	if ( consoleWindow )
	{
		if ( consoleWindow->viewport_GL )
		{
			autoScaleCbx->setChecked( consoleWindow->viewport_GL->getAutoScaleOpt() );
			sqrPixCbx->setChecked( consoleWindow->viewport_GL->getSqrPixelOpt() );
		}
		else if ( consoleWindow->viewport_SDL )
		{
			autoScaleCbx->setChecked( consoleWindow->viewport_SDL->getAutoScaleOpt() );
			sqrPixCbx->setChecked( consoleWindow->viewport_SDL->getSqrPixelOpt() );
		}
	}

	connect(autoRegion  , SIGNAL(stateChanged(int)), this, SLOT(autoRegionChanged(int)) );
	connect(new_PPU_ena , SIGNAL(stateChanged(int)), this, SLOT(use_new_PPU_changed(int)) );
	connect(frmskipcbx  , SIGNAL(stateChanged(int)), this, SLOT(frameskip_changed(int)) );
	connect(sprtLimCbx  , SIGNAL(stateChanged(int)), this, SLOT(useSpriteLimitChanged(int)) );
	connect(clipSidesCbx, SIGNAL(stateChanged(int)), this, SLOT(clipSidesChanged(int)) );
	connect(showFPS_cbx , SIGNAL(stateChanged(int)), this, SLOT(showFPSChanged(int)) );
	connect(sqrPixCbx   , SIGNAL(stateChanged(int)), this, SLOT(sqrPixChanged(int)) );
	connect(autoScaleCbx, SIGNAL(stateChanged(int)), this, SLOT(autoScaleChanged(int)) );

	vbox1->addWidget( autoRegion  );
	vbox1->addWidget( new_PPU_ena );
	vbox1->addWidget( frmskipcbx  );
	vbox1->addWidget( sprtLimCbx  );
	//vbox1->addWidget( clipSidesCbx);
	vbox1->addWidget( showFPS_cbx );
	vbox1->addWidget( autoScaleCbx);
	vbox1->addWidget( sqrPixCbx   );

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

	if ( sqrPixCbx->isChecked() )
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

	if ( sqrPixCbx->isChecked() )
	{
		yScaleLabel->hide();
		yScaleBox->hide();
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

	gbox  = new QGroupBox( tr("Drawing Area") );
	vbox2 = new QVBoxLayout();
	grid  = new QGridLayout();

	main_hbox->addLayout( vbox2 );
	vbox2->addWidget( gbox, 1 );
	gbox->setLayout(grid);

	ntsc_start = new QLineEdit();
	ntsc_end   = new QLineEdit();
	pal_start  = new QLineEdit();
	pal_end    = new QLineEdit();

	validator = new fceuDecIntValidtor( 0, 239, this );
	ntsc_start->setFont( font );
	ntsc_start->setMaxLength( 3 );
	ntsc_start->setValidator( validator );
	ntsc_start->setAlignment(Qt::AlignCenter);
	ntsc_start->setMaximumWidth( 8 * fontCharWidth );
	ntsc_start->setCursorPosition(0);

	validator = new fceuDecIntValidtor( 0, 239, this );
	ntsc_end->setFont( font );
	ntsc_end->setMaxLength( 3 );
	ntsc_end->setValidator( validator );
	ntsc_end->setAlignment(Qt::AlignCenter);
	ntsc_end->setMaximumWidth( 8 * fontCharWidth );
	ntsc_end->setCursorPosition(0);

	validator = new fceuDecIntValidtor( 0, 239, this );
	pal_start->setFont( font );
	pal_start->setMaxLength( 3 );
	pal_start->setValidator( validator );
	pal_start->setAlignment(Qt::AlignCenter);
	pal_start->setMaximumWidth( 8 * fontCharWidth );
	pal_start->setCursorPosition(0);

	validator = new fceuDecIntValidtor( 0, 239, this );
	pal_end->setFont( font );
	pal_end->setMaxLength( 3 );
	pal_end->setValidator( validator );
	pal_end->setAlignment(Qt::AlignCenter);
	pal_end->setMaximumWidth( 8 * fontCharWidth );
	pal_end->setCursorPosition(0);

	g_config->getOption("SDL.ScanLineStartNTSC", &opt);
	sprintf( stmp, "%i", opt );
	ntsc_start->setText( tr(stmp) );

	g_config->getOption("SDL.ScanLineEndNTSC", &opt);
	sprintf( stmp, "%i", opt );
	ntsc_end->setText( tr(stmp) );

	g_config->getOption("SDL.ScanLineStartPAL", &opt);
	sprintf( stmp, "%i", opt );
	pal_start->setText( tr(stmp) );

	g_config->getOption("SDL.ScanLineEndPAL", &opt);
	sprintf( stmp, "%i", opt );
	pal_end->setText( tr(stmp) );

	connect( ntsc_start, SIGNAL(textEdited(const QString &)), this, SLOT(ntscStartScanLineChanged(const QString &)));
	connect( ntsc_end  , SIGNAL(textEdited(const QString &)), this, SLOT(ntscEndScanLineChanged(const QString &)));
	connect( pal_start , SIGNAL(textEdited(const QString &)), this, SLOT(palStartScanLineChanged(const QString &)));
	connect( pal_end   , SIGNAL(textEdited(const QString &)), this, SLOT(palEndScanLineChanged(const QString &)));

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
	fceuWrapperLock();
	KillVideo ();
	InitVideo (GameInfo);
	fceuWrapperUnLock();
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
void ConsoleVideoConfDialog_t::ntscStartScanLineChanged(const QString &txt)
{
	int opt, opt2;
	std::string s;

	s = txt.toStdString();

	if ( s.size() > 0 )
	{
		opt = strtoul( s.c_str(), NULL, 10 );

		g_config->getOption("SDL.ScanLineEndNTSC", &opt2);

		if ( opt > opt2 )
		{
			opt = opt2;
		}
		g_config->setOption("SDL.ScanLineStartNTSC", opt);
	}
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::ntscEndScanLineChanged(const QString &txt)
{
	int opt, opt2;
	std::string s;

	s = txt.toStdString();

	if ( s.size() > 0 )
	{
		opt = strtoul( s.c_str(), NULL, 10 );

		g_config->getOption("SDL.ScanLineStartNTSC", &opt2);

		if ( opt < opt2 )
		{
			opt = opt2;
		}
		g_config->setOption("SDL.ScanLineEndNTSC", opt);
	}
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::palStartScanLineChanged(const QString &txt)
{
	int opt, opt2;
	std::string s;

	s = txt.toStdString();

	if ( s.size() > 0 )
	{
		opt = strtoul( s.c_str(), NULL, 10 );

		g_config->getOption("SDL.ScanLineEndPAL", &opt2);

		if ( opt > opt2 )
		{
			opt = opt2;
		}
		g_config->setOption("SDL.ScanLineStartPAL", opt);
	}
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::palEndScanLineChanged(const QString &txt)
{
	int opt, opt2;
	std::string s;

	s = txt.toStdString();

	if ( s.size() > 0 )
	{
		opt = strtoul( s.c_str(), NULL, 10 );

		g_config->getOption("SDL.ScanLineStartPAL", &opt2);

		if ( opt < opt2 )
		{
			opt = opt2;
		}
		g_config->setOption("SDL.ScanLineEndPAL", opt);
	}
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
void ConsoleVideoConfDialog_t::use_new_PPU_changed( int value )
{
	//printf("Value:%i \n", value );
	g_config->setOption("SDL.NewPPU", (value == Qt::Checked) );
	g_config->save ();

	fceuWrapperLock();
	UpdateEMUCore (g_config);
	fceuWrapperUnLock();
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::frameskip_changed( int value )
{
	//printf("Value:%i \n", value );
	g_config->setOption("SDL.Frameskip", (value == Qt::Checked) );
	g_config->save ();

	fceuWrapperLock();
	UpdateEMUCore (g_config);
	fceuWrapperUnLock();
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::useSpriteLimitChanged( int value )
{
	//printf("Value:%i \n", value );
	g_config->setOption("SDL.DisableSpriteLimit", (value == Qt::Checked) );
	g_config->save ();

	fceuWrapperLock();
	UpdateEMUCore (g_config);
	fceuWrapperUnLock();
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::clipSidesChanged( int value )
{
	//printf("Value:%i \n", value );
	g_config->setOption("SDL.ClipSides", (value == Qt::Checked) );
	g_config->save ();

	fceuWrapperLock();
	UpdateEMUCore (g_config);
	fceuWrapperUnLock();
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::showFPSChanged( int value )
{
	//printf("Value:%i \n", value );
	g_config->setOption("SDL.ShowFPS", (value == Qt::Checked) );
	g_config->save ();

	fceuWrapperLock();
	UpdateEMUCore (g_config);
	fceuWrapperUnLock();
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::sqrPixChanged( int value )
{
	//printf("Value:%i \n", value );
	int useSqrPix = (value != Qt::Unchecked);

	if ( useSqrPix )
	{
		xScaleLabel->setText( tr("Scale:") );
		yScaleLabel->hide();
		yScaleBox->hide();
	}
	else
	{
		xScaleLabel->setText( tr("X Scale:") );
		yScaleLabel->show();
		yScaleBox->show();
	}

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
		fceuWrapperLock();
		FCEUI_SetRegion (region, true);
		fceuWrapperUnLock();
	}
}
//----------------------------------------------------
QSize ConsoleVideoConfDialog_t::calcNewScreenSize(void)
{
	QSize out( GL_NES_WIDTH, GL_NES_HEIGHT );

	if ( consoleWindow )
	{
		QSize w, v;
		double xscale, yscale;
		int texture_width  = nes_shm->video.ncol;
		int texture_height = nes_shm->video.nrow;
		int l=0, r=texture_width;
		int t=0, b=texture_height;
		int dw=0, dh=0, rw, rh;

		w = consoleWindow->size();

		if ( consoleWindow->viewport_GL )
		{
			v = consoleWindow->viewport_GL->size();
		}
		else if ( consoleWindow->viewport_SDL )
		{
			v = consoleWindow->viewport_SDL->size();
		}

		dw = w.width()  - v.width();
		dh = w.height() - v.height();

		if ( sqrPixCbx->isChecked() )
		{
			xscale = xScaleBox->value();

			yscale = xscale * (double)nes_shm->video.xyRatio;
		}
		else
		{
			xscale = xScaleBox->value();
			yscale = yScaleBox->value();
		}
		rw=(int)((r-l)*xscale);
		rh=(int)((b-t)*yscale);

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

		if ( sqrPixCbx->isChecked() )
		{
			yscale = xscale = xScaleBox->value();
		}
		else
		{
			xscale = xScaleBox->value();
			yscale = yScaleBox->value();
		}

		if ( consoleWindow->viewport_GL )
		{
		   consoleWindow->viewport_GL->setSqrPixelOpt( sqrPixCbx->isChecked() );
		   consoleWindow->viewport_GL->setAutoScaleOpt( autoScaleCbx->isChecked() );
		   consoleWindow->viewport_GL->setScaleXY( xscale, yscale );
		}
		if ( consoleWindow->viewport_SDL )
		{
		   consoleWindow->viewport_SDL->setSqrPixelOpt( sqrPixCbx->isChecked() );
		   consoleWindow->viewport_SDL->setAutoScaleOpt( autoScaleCbx->isChecked() );
		   consoleWindow->viewport_SDL->setScaleXY( xscale, yscale );
		}

		consoleWindow->resize( s );

		updateReadouts();
	}

}
//----------------------------------------------------
