// CodeDataLogger.cpp
//
#include <QDir>
#include <QInputDialog>
#include <QMessageBox>

#include "Qt/CodeDataLogger.h"
#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/fceuWrapper.h"

//----------------------------------------------------
CodeDataLoggerDialog_t::CodeDataLoggerDialog_t(QWidget *parent)
	: QDialog( parent )
{
	QVBoxLayout *mainLayout, *vbox1, *vbox;
	QHBoxLayout *hbox;
	QGridLayout *grid;
	QGroupBox *frame, *subframe;
	QPushButton *btn;
	QLabel *lbl;

	updateTimer  = new QTimer( this );

   connect( updateTimer, &QTimer::timeout, this, &CodeDataLoggerDialog_t::updatePeriodic );

   setWindowTitle( tr("Code Data Logger") );

	mainLayout   = new QVBoxLayout();
	vbox1        = new QVBoxLayout();
	grid         = new QGridLayout();
	lbl          = new QLabel( tr("Press Start to Run Logger") );
	cdlFileLabel = new QLabel( tr("CDL File:") );

	vbox1->addLayout( grid );
	vbox1->addWidget( lbl  );
	vbox1->addWidget( cdlFileLabel );

	frame = new QGroupBox(tr("Code/Data Log Status"));
	frame->setLayout( vbox1 );

	prgLoggedCodeLabel = new QLabel( tr("0x00000 0.00%") );
	prgLoggedDataLabel = new QLabel( tr("0x00000 0.00%") );
	prgUnloggedLabel   = new QLabel( tr("0x00000 0.00%") );
	chrLoggedCodeLabel = new QLabel( tr("0x00000 0.00%") );
	chrLoggedDataLabel = new QLabel( tr("0x00000 0.00%") );
	chrUnloggedLabel   = new QLabel( tr("0x00000 0.00%") );
	autoSaveCdlCbox    = new QCheckBox( tr("Auto-save .CDL when closing ROMs") );
	autoLoadCdlCbox    = new QCheckBox( tr("Auto-load .CDL when opening this window") );
	autoResumeLogCbox  = new QCheckBox( tr("Auto-resume logging when loading ROMs") );

	subframe = new QGroupBox(tr("PRG Logged as Code"));
	vbox     = new QVBoxLayout();
	vbox->addWidget( prgLoggedCodeLabel );
	subframe->setLayout( vbox );

	grid->addWidget( subframe, 0, 0, Qt::AlignCenter );

	subframe = new QGroupBox(tr("PRG Logged as Data"));
	vbox     = new QVBoxLayout();
	vbox->addWidget( prgLoggedDataLabel );
	subframe->setLayout( vbox );

	grid->addWidget( subframe, 0, 1, Qt::AlignCenter );

	subframe = new QGroupBox(tr("PRG not Logged"));
	vbox     = new QVBoxLayout();
	vbox->addWidget( prgUnloggedLabel );
	subframe->setLayout( vbox );

	grid->addWidget( subframe, 0, 2, Qt::AlignCenter );

	subframe = new QGroupBox(tr("CHR Logged as Code"));
	vbox     = new QVBoxLayout();
	vbox->addWidget( chrLoggedCodeLabel );
	subframe->setLayout( vbox );

	grid->addWidget( subframe, 1, 0, Qt::AlignCenter );

	subframe = new QGroupBox(tr("CHR Logged as Data"));
	vbox     = new QVBoxLayout();
	vbox->addWidget( chrLoggedDataLabel );
	subframe->setLayout( vbox );

	grid->addWidget( subframe, 1, 1, Qt::AlignCenter );

	subframe = new QGroupBox(tr("CHR not Logged"));
	vbox     = new QVBoxLayout();
	vbox->addWidget( chrUnloggedLabel );
	subframe->setLayout( vbox );

	grid->addWidget( subframe, 1, 2, Qt::AlignCenter );

	grid = new QGridLayout();
	vbox1->addLayout( grid );
	btn = new QPushButton( tr("Reset Log") );
	grid->addWidget( btn, 0, 0, Qt::AlignCenter );

	btn = new QPushButton( tr("Start") );
	grid->addWidget( btn, 0, 1, Qt::AlignCenter );

	btn = new QPushButton( tr("Save") );
	grid->addWidget( btn, 0, 2, Qt::AlignCenter );

	btn = new QPushButton( tr("Load") );
	grid->addWidget( btn, 1, 0, Qt::AlignCenter );

	btn = new QPushButton( tr("Save As") );
	grid->addWidget( btn, 1, 2, Qt::AlignCenter );

	hbox = new QHBoxLayout();
	vbox1->addLayout( hbox );

	subframe = new QGroupBox(tr("Logging Workflow Options"));
	vbox     = new QVBoxLayout();
	vbox->addWidget( autoSaveCdlCbox );
	vbox->addWidget( autoLoadCdlCbox );
	vbox->addWidget( autoResumeLogCbox );
	subframe->setLayout( vbox );
	hbox->addWidget( subframe );

	subframe = new QGroupBox(tr("Generate ROM"));
	vbox     = new QVBoxLayout();

	btn = new QPushButton( tr("Save Stripped Data") );
	vbox->addWidget( btn );
	btn = new QPushButton( tr("Save Unused Data") );
	vbox->addWidget( btn );
	subframe->setLayout( vbox );
	hbox->addWidget( subframe );

	mainLayout->addWidget( frame );

	setLayout( mainLayout );

   updateTimer->start( 100 ); // 10hz

}
//----------------------------------------------------
CodeDataLoggerDialog_t::~CodeDataLoggerDialog_t(void)
{
   updateTimer->stop();

	printf("Code Data Logger Window Deleted\n");
}
//----------------------------------------------------
void CodeDataLoggerDialog_t::closeEvent(QCloseEvent *event)
{
   printf("Code Data Logger Close Window Event\n");
   done(0);
	deleteLater();
   event->accept();
}
//----------------------------------------------------
void CodeDataLoggerDialog_t::closeWindow(void)
{
   printf("Code Data Logger Close Window\n");
   done(0);
	deleteLater();
}
//----------------------------------------------------
void CodeDataLoggerDialog_t::updatePeriodic(void)
{

}
//----------------------------------------------------
