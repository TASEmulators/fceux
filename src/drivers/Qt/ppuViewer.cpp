// ppuViewer.cpp
//
#include <stdio.h>

#include <QDir>
#include <QInputDialog>
#include <QMessageBox>

#include "Qt/ppuViewer.h"
#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/fceuWrapper.h"

static ppuViewerDialog_t *ppuViewWindow = NULL;
//----------------------------------------------------
int openPPUViewWindow( QWidget *parent )
{
	if ( ppuViewWindow != NULL )
	{
		return -1;
	}
	ppuViewWindow = new ppuViewerDialog_t(parent);

	ppuViewWindow->show();

	return 0;
}
//----------------------------------------------------
ppuViewerDialog_t::ppuViewerDialog_t(QWidget *parent)
	: QDialog( parent )
{
	ppuViewWindow = this;

   setWindowTitle( tr("PPU Viewer") );

	//setLayout( mainLayout );
}

//----------------------------------------------------
ppuViewerDialog_t::~ppuViewerDialog_t(void)
{
	ppuViewWindow = NULL;

	printf("PPU Viewer Window Deleted\n");
}
//----------------------------------------------------
void ppuViewerDialog_t::closeEvent(QCloseEvent *event)
{
   printf("PPU Viewer Close Window Event\n");
   done(0);
	deleteLater();
   event->accept();
}
//----------------------------------------------------
void ppuViewerDialog_t::closeWindow(void)
{
   printf("Close Window\n");
   done(0);
	deleteLater();
}
//----------------------------------------------------
