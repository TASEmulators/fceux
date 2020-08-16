// HotKeyConf.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <SDL.h>
#include <QHeaderView>

#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/fceuWrapper.h"
#include "Qt/HexEditor.h"

//----------------------------------------------------------------------------
HexEditorDialog_t::HexEditorDialog_t(QWidget *parent)
	: QDialog( parent )
{
	QVBoxLayout *mainLayout;

	setWindowTitle("Hex Editor");

	resize( 512, 512 );

	mainLayout = new QVBoxLayout();

	txt = new QTextEdit();

	txt->moveCursor(QTextCursor::Start);
	txt->setReadOnly(true);

	mainLayout->addWidget( txt );

	setLayout( mainLayout );
}
//----------------------------------------------------------------------------
HexEditorDialog_t::~HexEditorDialog_t(void)
{

}
//----------------------------------------------------------------------------
void HexEditorDialog_t::closeWindow(void)
{
   //printf("Close Window\n");
   done(0);
}
//----------------------------------------------------------------------------
void HexEditorDialog_t::keyPressEvent(QKeyEvent *event)
{
   printf("Hex Window Key Press: 0x%x \n", event->key() );
	//assignHotkey( event );
}
//----------------------------------------------------------------------------
void HexEditorDialog_t::keyReleaseEvent(QKeyEvent *event)
{
   printf("Hex Window Key Release: 0x%x \n", event->key() );
	//assignHotkey( event );
}
//----------------------------------------------------------------------------
