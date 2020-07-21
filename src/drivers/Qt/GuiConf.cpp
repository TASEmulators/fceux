// PaletteConf.cpp
//
#include <QTextEdit>

#include "Qt/GuiConf.h"
#include "Qt/main.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/fceuWrapper.h"

//----------------------------------------------------
GuiConfDialog_t::GuiConfDialog_t(QWidget *parent)
	: QDialog( parent )
{
	int useNativeFileDialogVal;
	QVBoxLayout *mainLayout;

	//resize( 512, 600 );

	// sync with config
	g_config->getOption ("SDL.UseNativeFileDialog", &useNativeFileDialogVal);

   setWindowTitle( tr("GUI Config") );

	mainLayout = new QVBoxLayout();

	useNativeFileDialog  = new QCheckBox( tr("Use Native OS File Dialog") );

	useNativeFileDialog->setChecked( useNativeFileDialogVal );

	connect(useNativeFileDialog , SIGNAL(stateChanged(int)), this, SLOT(useNativeFileDialogChanged(int)) );

	mainLayout->addWidget( useNativeFileDialog );

	setLayout( mainLayout );
}

//----------------------------------------------------
GuiConfDialog_t::~GuiConfDialog_t(void)
{

}
//----------------------------------------------------
void GuiConfDialog_t::closeWindow(void)
{
   //printf("Close Window\n");
   done(0);
}
//----------------------------------------------------
void GuiConfDialog_t::useNativeFileDialogChanged(int state)
{
	int value = (state == Qt::Unchecked) ? 0 : 1;

	g_config->setOption ("SDL.UseNativeFileDialog", value);

}
//----------------------------------------------------
