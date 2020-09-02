// ConsoleDebugger.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <SDL.h>
#include <QHeaderView>
#include <QCloseEvent>
#include <QGridLayout>

#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/config.h"
#include "Qt/fceuWrapper.h"
#include "Qt/ConsoleDebugger.h"

//----------------------------------------------------------------------------
ConsoleDebugger::ConsoleDebugger(QWidget *parent)
	: QDialog( parent )
{
	QHBoxLayout *mainLayout;
	QVBoxLayout *vbox, *vbox1, *vbox2, *vbox3;
	QHBoxLayout *hbox, *hbox1, *hbox2;
	QGridLayout *grid;
	QPushButton *button;
	QFrame      *frame;
	QLabel      *lbl;
	float fontCharWidth;

	font.setFamily("Courier New");
	font.setStyle( QFont::StyleNormal );
	font.setStyleHint( QFont::Monospace );
	QFontMetrics fm(font);

	fontCharWidth = 1.00 * fm.averageCharWidth();

	setWindowTitle("6502 Debugger");

	//resize( 512, 512 );

	mainLayout = new QHBoxLayout();

	asmText = new QTextEdit(this);
	vbox1   = new QVBoxLayout();
	vbox2   = new QVBoxLayout();
	hbox1   = new QHBoxLayout();
	grid    = new QGridLayout();

	asmText->setFont(font);
	asmText->setMinimumWidth( 20 * fontCharWidth );

	vbox1->addLayout( hbox1 );
	hbox1->addLayout( vbox2 );
	vbox2->addLayout( grid  );

	mainLayout->addWidget( asmText );
	mainLayout->addLayout( vbox1 );

	button = new QPushButton( tr("Run") );
	grid->addWidget( button, 0, 0, Qt::AlignLeft );

	button = new QPushButton( tr("Step Into") );
	grid->addWidget( button, 0, 1, Qt::AlignLeft );

	button = new QPushButton( tr("Step Out") );
	grid->addWidget( button, 1, 0, Qt::AlignLeft );

	button = new QPushButton( tr("Step Over") );
	grid->addWidget( button, 1, 1, Qt::AlignLeft );

	button = new QPushButton( tr("Run Line") );
	grid->addWidget( button, 2, 0, Qt::AlignLeft );

	button = new QPushButton( tr("128 Lines") );
	grid->addWidget( button, 2, 1, Qt::AlignLeft );

	button = new QPushButton( tr("Seek To:") );
	grid->addWidget( button, 3, 0, Qt::AlignLeft );

	seekEntry = new QLineEdit();
	seekEntry->setFont( font );
	seekEntry->setMaxLength( 4 );
	seekEntry->setInputMask( ">HHHH;0" );
	seekEntry->setAlignment(Qt::AlignCenter);
	seekEntry->setMaximumWidth( 6 * fontCharWidth );
	grid->addWidget( seekEntry, 3, 1, Qt::AlignLeft );

	hbox = new QHBoxLayout();
	lbl  = new QLabel( tr("PC:") );
	pcEntry = new QLineEdit();
	pcEntry->setFont( font );
	pcEntry->setMaxLength( 4 );
	pcEntry->setInputMask( ">HHHH;0" );
	pcEntry->setAlignment(Qt::AlignCenter);
	pcEntry->setMaximumWidth( 6 * fontCharWidth );
	hbox->addWidget( lbl );
	hbox->addWidget( pcEntry, 1, Qt::AlignLeft );
	grid->addLayout( hbox, 4, 0, Qt::AlignLeft );

	button = new QPushButton( tr("Seek PC") );
	grid->addWidget( button, 4, 1, Qt::AlignLeft );

	hbox = new QHBoxLayout();
	lbl  = new QLabel( tr("A:") );
	regAEntry = new QLineEdit();
	regAEntry->setFont( font );
	regAEntry->setMaxLength( 2 );
	regAEntry->setInputMask( ">HH;0" );
	regAEntry->setAlignment(Qt::AlignCenter);
	regAEntry->setMaximumWidth( 4 * fontCharWidth );
	hbox->addWidget( lbl );
	hbox->addWidget( regAEntry, 1, Qt::AlignLeft );
	lbl  = new QLabel( tr("X:") );
	regXEntry = new QLineEdit();
	regXEntry->setFont( font );
	regXEntry->setMaxLength( 2 );
	regXEntry->setInputMask( ">HH;0" );
	regXEntry->setAlignment(Qt::AlignCenter);
	regXEntry->setMaximumWidth( 4 * fontCharWidth );
	hbox->addWidget( lbl );
	hbox->addWidget( regXEntry, 1, Qt::AlignLeft );
	lbl  = new QLabel( tr("Y:") );
	regYEntry = new QLineEdit();
	regYEntry->setFont( font );
	regYEntry->setMaxLength( 2 );
	regYEntry->setInputMask( ">HH;0" );
	regYEntry->setAlignment(Qt::AlignCenter);
	regYEntry->setMaximumWidth( 4 * fontCharWidth );
	hbox->addWidget( lbl );
	hbox->addWidget( regYEntry, 1, Qt::AlignLeft );
	vbox2->addLayout( hbox );

	stackFrame = new QGroupBox(tr("Stack $0100"));
	stackText  = new QTextEdit(this);
	hbox       = new QHBoxLayout();
	hbox->addWidget( stackText );
	vbox2->addWidget( stackFrame );
	stackFrame->setLayout( hbox );
	stackText->setFont(font);
	stackText->setReadOnly(true);
	stackText->setMaximumWidth( 16 * fontCharWidth );

	bpFrame = new QGroupBox(tr("Breakpoints"));
	vbox3   = new QVBoxLayout();
	vbox    = new QVBoxLayout();
	hbox    = new QHBoxLayout();
	bpTree  = new QTreeWidget();

	bpTree->setColumnCount(1);

	hbox->addWidget( bpTree );

	hbox    = new QHBoxLayout();
	button = new QPushButton( tr("Add") );
	hbox->addWidget( button );

	button = new QPushButton( tr("Delete") );
	hbox->addWidget( button );

	button = new QPushButton( tr("Edit") );
	hbox->addWidget( button );

	brkBadOpsCbox = new QCheckBox( tr("Break on Bad Opcodes") );

	vbox->addWidget( bpTree );
	vbox->addLayout( hbox   );
	vbox->addWidget( brkBadOpsCbox );
	bpFrame->setLayout( vbox );

	sfFrame = new QGroupBox(tr("Status Flags"));
	grid    = new QGridLayout();
	sfFrame->setLayout( grid );

	N_chkbox = new QCheckBox( tr("N") );
	V_chkbox = new QCheckBox( tr("V") );
	U_chkbox = new QCheckBox( tr("U") );
	B_chkbox = new QCheckBox( tr("B") );
	D_chkbox = new QCheckBox( tr("D") );
	I_chkbox = new QCheckBox( tr("I") );
	Z_chkbox = new QCheckBox( tr("Z") );
	C_chkbox = new QCheckBox( tr("C") );

	grid->addWidget( N_chkbox, 0, 0, Qt::AlignCenter );
	grid->addWidget( V_chkbox, 0, 1, Qt::AlignCenter );
	grid->addWidget( U_chkbox, 0, 2, Qt::AlignCenter );
	grid->addWidget( B_chkbox, 0, 3, Qt::AlignCenter );
	grid->addWidget( D_chkbox, 1, 0, Qt::AlignCenter );
	grid->addWidget( I_chkbox, 1, 1, Qt::AlignCenter );
	grid->addWidget( Z_chkbox, 1, 2, Qt::AlignCenter );
	grid->addWidget( C_chkbox, 1, 3, Qt::AlignCenter );

	vbox3->addWidget( bpFrame);
	vbox3->addWidget( sfFrame);
	hbox1->addLayout( vbox3  );

	hbox2       = new QHBoxLayout();
	vbox        = new QVBoxLayout();
	frame       = new QFrame();
	ppuLbl      = new QLabel( tr("PPU:") );
	spriteLbl   = new QLabel( tr("Sprite:") );
	scanLineLbl = new QLabel( tr("Scanline:") );
	pixLbl      = new QLabel( tr("Pixel:") );
	vbox->addWidget( ppuLbl );
	vbox->addWidget( spriteLbl );
	vbox->addWidget( scanLineLbl );
	vbox->addWidget( pixLbl );
	vbox1->addLayout( hbox2 );
	hbox2->addWidget( frame );
	frame->setLayout( vbox  );
	frame->setFrameShape( QFrame::Box );

	vbox         = new QVBoxLayout();
	hbox         = new QHBoxLayout();
	cpuCyclesLbl = new QLabel( tr("CPU Cycles:") );
	cpuInstrsLbl = new QLabel( tr("Instructions:") );
	brkCpuCycExd = new QCheckBox( tr("Break when Exceed") );
	brkInstrsExd = new QCheckBox( tr("Break when Exceed") );
	cpuCycExdVal = new QLineEdit( tr("0") );
	instrExdVal  = new QLineEdit( tr("0") );
	vbox->addWidget( cpuCyclesLbl );
	vbox->addLayout( hbox );
	hbox->addWidget( brkCpuCycExd );
	hbox->addWidget( cpuCycExdVal, 1, Qt::AlignLeft );

	hbox         = new QHBoxLayout();
	vbox->addWidget( cpuInstrsLbl );
	vbox->addLayout( hbox );
	hbox->addWidget( brkInstrsExd );
	hbox->addWidget( instrExdVal, 1, Qt::AlignLeft );
	hbox2->addLayout( vbox );

	cpuCycExdVal->setFont( font );
	cpuCycExdVal->setMaxLength( 10 );
	cpuCycExdVal->setInputMask( ">DDDD;0" );
	cpuCycExdVal->setAlignment(Qt::AlignCenter);
	cpuCycExdVal->setMaximumWidth( 12 * fontCharWidth );

	instrExdVal->setFont( font );
	instrExdVal->setMaxLength( 10 );
	instrExdVal->setInputMask( ">DDDD;0" );
	instrExdVal->setAlignment(Qt::AlignCenter);
	instrExdVal->setMaximumWidth( 12 * fontCharWidth );

	setLayout( mainLayout );
}
//----------------------------------------------------------------------------
ConsoleDebugger::~ConsoleDebugger(void)
{
	printf("Destroy Debugger Window\n");
}
//----------------------------------------------------------------------------
void ConsoleDebugger::closeEvent(QCloseEvent *event)
{
   printf("Debugger Close Window Event\n");
   done(0);
	deleteLater();
   event->accept();
}
//----------------------------------------------------------------------------
void ConsoleDebugger::closeWindow(void)
{
   //printf("Close Window\n");
   done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
