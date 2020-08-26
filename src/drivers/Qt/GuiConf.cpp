// PaletteConf.cpp
//
#include <QTextEdit>

#include "Qt/GuiConf.h"
#include "Qt/main.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/fceuWrapper.h"
#include "Qt/ConsoleWindow.h"

//----------------------------------------------------
GuiConfDialog_t::GuiConfDialog_t(QWidget *parent)
	: QDialog( parent )
{
	int useNativeFileDialogVal;
	int useNativeMenuBarVal;
	QVBoxLayout *mainLayout;

	//resize( 512, 600 );

	// sync with config
	g_config->getOption ("SDL.UseNativeFileDialog", &useNativeFileDialogVal);
	g_config->getOption ("SDL.UseNativeMenuBar", &useNativeMenuBarVal);

   setWindowTitle( tr("GUI Config") );

	mainLayout = new QVBoxLayout();

	useNativeFileDialog  = new QCheckBox( tr("Use Native OS File Dialog") );
	useNativeMenuBar     = new QCheckBox( tr("Use Native OS Menu Bar") );

	useNativeFileDialog->setChecked( useNativeFileDialogVal );
	useNativeMenuBar->setChecked( useNativeMenuBarVal );

	connect(useNativeFileDialog , SIGNAL(stateChanged(int)), this, SLOT(useNativeFileDialogChanged(int)) );
	connect(useNativeMenuBar    , SIGNAL(stateChanged(int)), this, SLOT(useNativeMenuBarChanged(int)) );

	mainLayout->addWidget( useNativeFileDialog );
	mainLayout->addWidget( useNativeMenuBar );

	setLayout( mainLayout );
}

//----------------------------------------------------
GuiConfDialog_t::~GuiConfDialog_t(void)
{
   printf("Destroy GUI Config Close Window\n");
}
//----------------------------------------------------------------------------
void GuiConfDialog_t::closeEvent(QCloseEvent *event)
{
   printf("GUI Config Close Window Event\n");
   done(0);
	deleteLater();
   event->accept();
}
//----------------------------------------------------
void GuiConfDialog_t::closeWindow(void)
{
   //printf("Close Window\n");
   done(0);
	deleteLater();
}
//----------------------------------------------------
void GuiConfDialog_t::useNativeFileDialogChanged(int state)
{
	int value = (state == Qt::Unchecked) ? 0 : 1;

	g_config->setOption ("SDL.UseNativeFileDialog", value);
}
//----------------------------------------------------
void GuiConfDialog_t::useNativeMenuBarChanged(int state)
{
	int value = (state == Qt::Unchecked) ? 0 : 1;

	g_config->setOption ("SDL.UseNativeMenuBar", value);

	consoleWindow->menuBar()->setNativeMenuBar( value );
}
//----------------------------------------------------
