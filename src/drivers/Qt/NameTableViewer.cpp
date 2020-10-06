// NameTableViewer.cpp
#include <stdio.h>
#include <stdint.h>

#include <QDir>
#include <QPainter>
#include <QInputDialog>
#include <QMessageBox>

#include "../../types.h"
#include "../../fceu.h"
#include "../../cart.h"
#include "../../ppu.h"
#include "../../debug.h"
#include "../../palette.h"

#include "Qt/NameTableViewer.h"
#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/fceuWrapper.h"

static ppuNameTableViewerDialog_t *nameTableViewWindow = NULL;

//----------------------------------------------------
int openNameTableViewWindow( QWidget *parent )
{
	if ( nameTableViewWindow != NULL )
	{
		return -1;
	}

	nameTableViewWindow = new ppuNameTableViewerDialog_t(parent);

	nameTableViewWindow->show();

	return 0;
}
//----------------------------------------------------
ppuNameTableViewerDialog_t::ppuNameTableViewerDialog_t(QWidget *parent)
	: QDialog( parent )
{
	QVBoxLayout *mainLayout, *vbox;
	QHBoxLayout *hbox;
	QGridLayout *grid;
	char stmp[64];

	nameTableViewWindow = this;

   setWindowTitle( tr("Name Table Viewer") );

	mainLayout = new QVBoxLayout();

	setLayout( mainLayout );
}
//----------------------------------------------------
ppuNameTableViewerDialog_t::~ppuNameTableViewerDialog_t(void)
{
	nameTableViewWindow = NULL;

	printf("Name Table Viewer Window Deleted\n");
}
//----------------------------------------------------
void ppuNameTableViewerDialog_t::closeEvent(QCloseEvent *event)
{
   printf("Name Table Viewer Close Window Event\n");
   done(0);
	deleteLater();
   event->accept();
}
//----------------------------------------------------
void ppuNameTableViewerDialog_t::closeWindow(void)
{
   printf("Close Window\n");
   done(0);
	deleteLater();
}
//----------------------------------------------------
ppuNameTableView_t::ppuNameTableView_t(QWidget *parent)
	: QWidget(parent)
{
	this->setFocusPolicy(Qt::StrongFocus);
	this->setMouseTracking(true);
	setMinimumWidth( 256 );
	setMinimumHeight( 256 );
	viewWidth = 256;
	viewHeight = 256;
}
//----------------------------------------------------
ppuNameTableView_t::~ppuNameTableView_t(void)
{

}
//----------------------------------------------------
void ppuNameTableView_t::resizeEvent(QResizeEvent *event)
{
	viewWidth  = event->size().width();
	viewHeight = event->size().height();
}
//----------------------------------------------------
void ppuNameTableView_t::mouseMoveEvent(QMouseEvent *event)
{
	//QPoint tile = convPixToTile( event->pos() );

	//if ( (tile.x() < 16) && (tile.y() < 16) )
	//{
	//	char stmp[64];
	//	sprintf( stmp, "Tile: $%X%X", tile.y(), tile.x() );
	//	tileLabel->setText( tr(stmp) );
	//}
}
//----------------------------------------------------------------------------
void ppuNameTableView_t::mousePressEvent(QMouseEvent * event)
{
	//QPoint tile = convPixToTile( event->pos() );

	if ( event->button() == Qt::LeftButton )
	{
	}
	else if ( event->button() == Qt::RightButton )
	{
	}
}
//----------------------------------------------------
void ppuNameTableView_t::paintEvent(QPaintEvent *event)
{
	QPainter painter(this);
	viewWidth  = event->rect().width();
	viewHeight = event->rect().height();

}
//----------------------------------------------------
