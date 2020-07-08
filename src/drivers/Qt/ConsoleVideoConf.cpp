// ConsoleVideoConf.cpp
//
#include "../../fceu.h"
#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/config.h"
#include "Qt/fceuWrapper.h"
#include "Qt/ConsoleVideoConf.h"

//----------------------------------------------------
ConsoleVideoConfDialog_t::ConsoleVideoConfDialog_t(QWidget *parent)
	: QDialog( parent )
{
	QVBoxLayout *main_vbox;
	QHBoxLayout *hbox1;
	QLabel *lbl;
	QPushButton *button;

	setWindowTitle( tr("Video Config") );

	main_vbox = new QVBoxLayout();

	// Video Driver Select
	lbl = new QLabel( tr("Driver:") );

	driverSelect = new QComboBox();

	driverSelect->addItem( tr("OpenGL"), 0 );
	//driverSelect->addItem( tr("SDL"), 1 );
	
	hbox1 = new QHBoxLayout();

	hbox1->addWidget( lbl );
	hbox1->addWidget( driverSelect );

	main_vbox->addLayout( hbox1 );

	// Enable OpenGL Linear Filter Checkbox
	gl_LF_chkBox  = new QCheckBox( tr("Enable OpenGL Linear Filter") );

	setCheckBoxFromProperty( gl_LF_chkBox  , "SDL.OpenGLip");

	main_vbox->addWidget( gl_LF_chkBox );

	// Region Select
	lbl = new QLabel( tr("Region:") );

	regionSelect = new QComboBox();

	regionSelect->addItem( tr("NTSC") , 0 );
	regionSelect->addItem( tr("PAL")  , 1 );
	regionSelect->addItem( tr("Dendy"), 2 );

	hbox1 = new QHBoxLayout();

	hbox1->addWidget( lbl );
	hbox1->addWidget( regionSelect );

	main_vbox->addLayout( hbox1 );

	// Enable New PPU Checkbox
	new_PPU_ena  = new QCheckBox( tr("Enable New PPU") );

	// Enable New PPU Checkbox
	frmskipcbx  = new QCheckBox( tr("Enable Frameskip") );

	// Disable Sprite Limit Checkbox
	sprtLimCbx  = new QCheckBox( tr("Disable Sprite Limit") );

	// Clip Sides Checkbox
	clipSidesCbx  = new QCheckBox( tr("Clip Sides") );

	// Show FPS Checkbox
	showFPS_cbx  = new QCheckBox( tr("Show FPS") );

	setCheckBoxFromProperty( new_PPU_ena  , "SDL.NewPPU");
	setCheckBoxFromProperty( frmskipcbx   , "SDL.Frameskip");
	setCheckBoxFromProperty( sprtLimCbx   , "SDL.DisableSpriteLimit");
	setCheckBoxFromProperty( clipSidesCbx , "SDL.ClipSides");
	setCheckBoxFromProperty( showFPS_cbx  , "SDL.ShowFPS");

	connect(new_PPU_ena , SIGNAL(stateChanged(int)), this, SLOT(use_new_PPU_changed(int)) );
	connect(frmskipcbx  , SIGNAL(stateChanged(int)), this, SLOT(frameskip_changed(int)) );
	connect(sprtLimCbx  , SIGNAL(stateChanged(int)), this, SLOT(useSpriteLimitChanged(int)) );
	connect(clipSidesCbx, SIGNAL(stateChanged(int)), this, SLOT(clipSidesChanged(int)) );
	connect(showFPS_cbx , SIGNAL(stateChanged(int)), this, SLOT(showFPSChanged(int)) );

	main_vbox->addWidget( new_PPU_ena );
	main_vbox->addWidget( frmskipcbx  );
	main_vbox->addWidget( sprtLimCbx  );
	main_vbox->addWidget( clipSidesCbx);
	main_vbox->addWidget( showFPS_cbx );

	hbox1 = new QHBoxLayout();

	button = new QPushButton( tr("Apply") );
	hbox1->addWidget( button );
	connect(button, SIGNAL(clicked()), this, SLOT(applyChanges(void)) );

	button = new QPushButton( tr("Close") );
	hbox1->addWidget( button );
	connect(button, SIGNAL(clicked()), this, SLOT(closewindow(void)) );

	main_vbox->addLayout( hbox1 );

	setLayout( main_vbox );

}
//----------------------------------------------------
ConsoleVideoConfDialog_t::~ConsoleVideoConfDialog_t(void)
{

}
//----------------------------------------------------
void  ConsoleVideoConfDialog_t::resetVideo(void)
{
	KillVideo ();
	InitVideo (GameInfo);
}
//----------------------------------------------------
void  ConsoleVideoConfDialog_t::setCheckBoxFromProperty( QCheckBox *cbx, const char *property )
{
	int  pval;
	g_config->getOption (property, &pval);

	cbx->setCheckState( pval ? Qt::Checked : Qt::Unchecked );
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
void ConsoleVideoConfDialog_t::applyChanges( void )
{ 
	resetVideo();
}
//----------------------------------------------------
void ConsoleVideoConfDialog_t::closewindow( void )
{ 
	done(0);
}
//----------------------------------------------------
