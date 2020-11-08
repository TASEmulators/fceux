// HotKeyConf.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <SDL.h>
#include <QHeaderView>
#include <QFileDialog>
#include <QScreen>
#include <QGuiApplication>

#include "../../types.h"
#include "../../fceu.h"
#include "../../cheat.h"
#include "../../debug.h"
#include "../../driver.h"
#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/fceuWrapper.h"
#include "Qt/CheatsConf.h"
#include "Qt/ConsoleUtilities.h"

static GuiCheatsDialog_t *win = NULL;
//----------------------------------------------------------------------------
void openCheatDialog(QWidget *parent)
{
   if ( win != NULL )
   {
      return;
   }
   win = new GuiCheatsDialog_t(parent);
	
   win->show();
}
//----------------------------------------------------------------------------
void updateCheatDialog(void)
{
   if ( win == NULL )
   {
      return;
   }
   win->showActiveCheatList( true );
}
//----------------------------------------------------------------------------
GuiCheatsDialog_t::GuiCheatsDialog_t(QWidget *parent)
	: QDialog( parent, Qt::Window )
{
	QHBoxLayout *mainLayout, *hbox, *hbox1;
	QVBoxLayout *vbox, *vbox1, *vbox2, *vbox3;
	QTreeWidgetItem *item;
	QLabel *lbl;
	QGroupBox *groupBox;
	QFrame *frame;

	font.setFamily("Courier New");
	font.setStyle( QFont::StyleNormal );
	font.setStyleHint( QFont::Monospace );

	QFontMetrics fm(font);

#if QT_VERSION > QT_VERSION_CHECK(5, 11, 0)
    fontCharWidth = 2 * fm.horizontalAdvance(QLatin1Char('2'));
#else
    fontCharWidth = 2 * fm.width(QLatin1Char('2'));
#endif

	setWindowTitle("Cheat Search");

	pauseWhileActive  = false;
	wasPausedByCheats = false;

	//resize( 512, 512 );

	// Window Layout Box
	mainLayout = new QHBoxLayout();

	//-------------------------------------------------------
	// Left Side Active Cheats Frame
	actCheatFrame = new QGroupBox( tr("Active Cheats") );

	vbox1 = new QVBoxLayout();

	mainLayout->addWidget( actCheatFrame );
	
	actvCheatList = new QTreeWidget();

	actvCheatList->setColumnCount(2);

	item = new QTreeWidgetItem();
	item->setFont( 0, font );
	item->setFont( 1, font );
	item->setText( 0, QString::fromStdString( "Code" ) );
	item->setText( 1, QString::fromStdString( "Name" ) );
	item->setTextAlignment( 0, Qt::AlignCenter);
	item->setTextAlignment( 1, Qt::AlignCenter);

	actvCheatList->setHeaderItem( item );

	actvCheatList->header()->setSectionResizeMode( QHeaderView::ResizeToContents );

	connect( actvCheatList, SIGNAL(itemClicked(QTreeWidgetItem*, int)),
			   this, SLOT(actvCheatItemClicked( QTreeWidgetItem*, int)) );

	hbox = new QHBoxLayout();

	enaCheats = new QCheckBox( tr("Enable Cheats") );
	autoSave  = new QCheckBox( tr("Auto Load / Save with Game") );

	enaCheats->setChecked( !globalCheatDisabled );
	autoSave->setChecked( !disableAutoLSCheats );

	hbox->addWidget( enaCheats );
	hbox->addWidget( autoSave  );

	vbox1->addLayout( hbox );

	vbox1->addWidget( actvCheatList );

	hbox = new QHBoxLayout();

	lbl = new QLabel( tr("Name:") );
	cheatNameEntry = new QLineEdit();
	cheatNameEntry->setFont( font );
	//cheatNameEntry->setMaxLength(64);

	hbox->addWidget( lbl );
	hbox->addWidget( cheatNameEntry );

	vbox1->addLayout( hbox );

	hbox = new QHBoxLayout();

	lbl = new QLabel( tr("Address:") );
	cheatAddrEntry = new QLineEdit();
	cheatAddrEntry->setMaxLength(4);
	cheatAddrEntry->setInputMask( ">HHHH;0" );
	cheatAddrEntry->setFont( font );
	cheatAddrEntry->setCursorPosition(0);
	cheatAddrEntry->setAlignment(Qt::AlignCenter);
	cheatAddrEntry->setMaximumWidth( 5 * fontCharWidth );

	hbox->addWidget( lbl );
	hbox->addWidget( cheatAddrEntry );

	lbl = new QLabel( tr("Value:") );
	cheatValEntry = new QLineEdit();
	cheatValEntry->setMaxLength(2);
	cheatValEntry->setInputMask( ">HH;0" );
	cheatValEntry->setFont( font );
	cheatValEntry->setCursorPosition(0);
	cheatValEntry->setAlignment(Qt::AlignCenter);
	cheatValEntry->setMaximumWidth( 3 * fontCharWidth );

	hbox->addWidget( lbl );
	hbox->addWidget( cheatValEntry );

	lbl = new QLabel( tr("Compare:") );
	cheatCmpEntry = new QLineEdit();
	cheatCmpEntry->setMaxLength(2);
	cheatCmpEntry->setInputMask( ">HH;X" );
	cheatCmpEntry->setFont( font );
	cheatCmpEntry->setCursorPosition(0);
	cheatCmpEntry->setAlignment(Qt::AlignCenter);
	cheatCmpEntry->setMaximumWidth( 3 * fontCharWidth );

	hbox->addWidget( lbl );
	hbox->addWidget( cheatCmpEntry );

	vbox1->addLayout( hbox );

	hbox = new QHBoxLayout();

	addCheatBtn = new QPushButton( tr("Add") );
	delCheatBtn = new QPushButton( tr("Delete") );
	modCheatBtn = new QPushButton( tr("Update") );

	hbox->addWidget( addCheatBtn );
	hbox->addWidget( delCheatBtn );
	hbox->addWidget( modCheatBtn );

	vbox1->addLayout( hbox );

	hbox = new QHBoxLayout();

	importCheatFileBtn = new QPushButton( tr("Import") );
	exportCheatFileBtn = new QPushButton( tr("Export") );
	importCheatFileBtn->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	exportCheatFileBtn->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );

	hbox->addWidget( importCheatFileBtn );
	hbox->addWidget( exportCheatFileBtn );

	vbox1->addLayout( hbox );

	actCheatFrame->setLayout( vbox1 );

	cheatSearchFrame = new QGroupBox( tr("Cheat Search") );
	cheatResultFrame = new QGroupBox( tr("Possibilities") );

	srchResults = new QTreeWidget();
	srchResults->setColumnCount(3);

	item = new QTreeWidgetItem();
	item->setFont( 0, font );
	item->setFont( 1, font );
	item->setFont( 2, font );
	item->setText( 0, QString::fromStdString( "Addr" ) );
	item->setText( 1, QString::fromStdString( "Pre"  ) );
	item->setText( 2, QString::fromStdString( "Cur"  ) );
	item->setTextAlignment( 0, Qt::AlignCenter);
	item->setTextAlignment( 1, Qt::AlignCenter);
	item->setTextAlignment( 2, Qt::AlignCenter);

	srchResults->setHeaderItem( item );

	//srchResults->header()->setSectionResizeMode( QHeaderView::ResizeToContents );
	srchResults->header()->setSectionResizeMode( QHeaderView::Interactive );
	//srchResults->header()->setSectionResizeMode( QHeaderView::Fixed );
	srchResults->header()->resizeSection( 0, 10 * fontCharWidth );
	srchResults->header()->resizeSection( 1,  6 * fontCharWidth );
	srchResults->header()->resizeSection( 2,  6 * fontCharWidth );
	//srchResults->header()->setSectionResizeMode( QHeaderView::Stretch );
	//srchResults->header()->setDefaultSectionSize( 200 );
	//srchResults->header()->setDefaultSectionSize( 200 );
	//srchResults->setReadOnly(true);

	vbox = new QVBoxLayout();
	vbox->addWidget( srchResults );
	cheatResultFrame->setLayout( vbox );

	hbox1 = new QHBoxLayout();

	vbox2 = new QVBoxLayout();

	hbox1->addLayout( vbox2 );
	hbox1->addWidget( cheatResultFrame );

	cheatSearchFrame->setLayout( hbox1 );

	srchResetBtn = new QPushButton( tr("Reset") );
	srchResetBtn->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );

	vbox2->addWidget( srchResetBtn );

	frame = new QFrame();
	frame->setFrameShape( QFrame::Box );
	frame->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	vbox2->addWidget( frame );
	vbox = new QVBoxLayout();

	frame->setLayout( vbox );

	knownValBtn  = new QPushButton( tr("Known Value:") );
	knownValBtn->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	knownValBtn->setEnabled(false);

	vbox->addWidget( knownValBtn  );

	hbox1 = new QHBoxLayout();
	vbox->addLayout( hbox1 );

	lbl = new QLabel( tr("0x") );
	lbl->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	knownValEntry = new QLineEdit();
	knownValEntry->setMaxLength(2);
	knownValEntry->setInputMask( ">HH;0" );
	knownValEntry->setFont( font );
	knownValEntry->setCursorPosition(0);
	knownValEntry->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	knownValEntry->setMaximumWidth( 3 * fontCharWidth );
	knownValEntry->setAlignment(Qt::AlignCenter);
	knownValEntry->setEnabled(false);
	hbox1->addWidget( lbl, 0, Qt::AlignRight );
	hbox1->addWidget( knownValEntry, 0, Qt::AlignLeft );

	groupBox = new QGroupBox( tr("Previous Compare") );
	vbox2->addWidget( groupBox );
	groupBox->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );

	vbox3 = new QVBoxLayout();

	frame = new QFrame();
	frame->setFrameShape( QFrame::Box );
	frame->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	vbox3->addWidget( frame );
	vbox = new QVBoxLayout();

	eqValBtn = new QPushButton( tr("Equal") );
	eqValBtn->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	eqValBtn->setEnabled(false);
	vbox->addWidget( eqValBtn );

	frame->setLayout( vbox );

	frame = new QFrame();
	frame->setFrameShape( QFrame::Box );
	frame->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	vbox3->addWidget( frame );
	vbox = new QVBoxLayout();

	neValBtn = new QPushButton( tr("Not Equal") );
	neValBtn->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	neValBtn->setEnabled(false);

	hbox = new QHBoxLayout();
	useNeVal = new QCheckBox( tr("By:") );
	useNeVal->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	useNeVal->setEnabled(false);
	neValEntry = new QLineEdit();
	neValEntry->setMaxLength(2);
	neValEntry->setInputMask( ">HH;0" );
	neValEntry->setFont( font );
	neValEntry->setCursorPosition(0);
	neValEntry->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	neValEntry->setMaximumWidth( 3 * fontCharWidth );
	neValEntry->setAlignment(Qt::AlignCenter);
	neValEntry->setEnabled(false);

	hbox->addWidget( useNeVal, 0, Qt::AlignRight );
	hbox->addWidget( neValEntry, 0, Qt::AlignLeft );

	vbox->addWidget( neValBtn );
	vbox->addLayout( hbox );
	frame->setLayout( vbox );

	frame = new QFrame();
	frame->setFrameShape( QFrame::Box );
	frame->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	vbox3->addWidget( frame );
	vbox = new QVBoxLayout();

	grValBtn = new QPushButton( tr("Greater Than") );
	grValBtn->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	grValBtn->setEnabled(false);

	hbox = new QHBoxLayout();
	useGrVal = new QCheckBox( tr("By:") );
	useGrVal->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	useGrVal->setEnabled(false);
	grValEntry = new QLineEdit();
	grValEntry->setMaxLength(2);
	grValEntry->setInputMask( ">HH;0" );
	grValEntry->setFont( font );
	grValEntry->setCursorPosition(0);
	grValEntry->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	grValEntry->setMaximumWidth( 3 * fontCharWidth );
	grValEntry->setAlignment(Qt::AlignCenter);
	grValEntry->setEnabled(false);

	hbox->addWidget( useGrVal, 0, Qt::AlignRight );
	hbox->addWidget( grValEntry, 0, Qt::AlignLeft );

	vbox->addWidget( grValBtn );
	vbox->addLayout( hbox );
	frame->setLayout( vbox );

	frame = new QFrame();
	frame->setFrameShape( QFrame::Box );
	frame->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	vbox3->addWidget( frame );
	vbox = new QVBoxLayout();

	ltValBtn = new QPushButton( tr("Less Than") );
	ltValBtn->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	ltValBtn->setEnabled(false);

	hbox = new QHBoxLayout();
	useLtVal = new QCheckBox( tr("By:") );
	useLtVal->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	useLtVal->setEnabled(false);
	ltValEntry = new QLineEdit();
	ltValEntry->setMaxLength(2);
	ltValEntry->setInputMask( ">HH;0" );
	ltValEntry->setFont( font );
	ltValEntry->setCursorPosition(0);
	ltValEntry->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	ltValEntry->setMaximumWidth( 3 * fontCharWidth );
	ltValEntry->setAlignment(Qt::AlignCenter);
	ltValEntry->setEnabled(false);

	hbox->addWidget( useLtVal, 0, Qt::AlignRight );
	hbox->addWidget( ltValEntry, 0, Qt::AlignLeft );

	vbox->addWidget( ltValBtn );
	vbox->addLayout( hbox );
	frame->setLayout( vbox );

	groupBox->setLayout( vbox3 );

	vbox = new QVBoxLayout();

	pauseBox = new QCheckBox( tr("Pause emulation when this window is active") );

	vbox->addWidget( cheatSearchFrame );
	vbox->addWidget( pauseBox         );

	mainLayout->addLayout( vbox );

	setLayout( mainLayout );

	connect( srchResetBtn, SIGNAL(clicked(void)), this, SLOT(resetSearchCallback(void)) );
	connect( knownValBtn , SIGNAL(clicked(void)), this, SLOT(knownValueCallback(void)) );
	connect( eqValBtn    , SIGNAL(clicked(void)), this, SLOT(equalValueCallback(void)) );
	connect( neValBtn    , SIGNAL(clicked(void)), this, SLOT(notEqualValueCallback(void)) );
	connect( ltValBtn    , SIGNAL(clicked(void)), this, SLOT(lessThanValueCallback(void)) );
	connect( grValBtn    , SIGNAL(clicked(void)), this, SLOT(greaterThanValueCallback(void)) );
	connect( addCheatBtn , SIGNAL(clicked(void)), this, SLOT(addActvCheat(void)) );
	connect( delCheatBtn , SIGNAL(clicked(void)), this, SLOT(deleteActvCheat(void)) );
	connect( modCheatBtn , SIGNAL(clicked(void)), this, SLOT(updateCheatParameters(void)) );

	connect( enaCheats, SIGNAL(stateChanged(int)), this, SLOT(globalEnableCheats(int)) );
	connect( autoSave , SIGNAL(stateChanged(int)), this, SLOT(autoLoadSaveCheats(int)) );
	connect( pauseBox , SIGNAL(stateChanged(int)), this, SLOT(pauseWindowState(int)) );

	connect( importCheatFileBtn, SIGNAL(clicked(void)), this, SLOT(openCheatFile(void)) );
	connect( exportCheatFileBtn, SIGNAL(clicked(void)), this, SLOT(saveCheatFile(void)) );

	showActiveCheatList(true);
}
//----------------------------------------------------------------------------
GuiCheatsDialog_t::~GuiCheatsDialog_t(void)
{
	if (EmulationPaused && wasPausedByCheats)
	{
		EmulationPaused = 0;
		FCEU_printf ("Emulation paused: %d\n", EmulationPaused);
	}
	wasPausedByCheats = false;

   win = NULL;
   printf("Destroy Cheat Window Event\n");
}
//----------------------------------------------------------------------------
void GuiCheatsDialog_t::closeEvent(QCloseEvent *event)
{
   printf("Cheat Close Window Event\n");
   done(0);
	deleteLater();
   event->accept();
}
//----------------------------------------------------------------------------
void GuiCheatsDialog_t::closeWindow(void)
{
   //printf("Close Window\n");
   done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
int GuiCheatsDialog_t::addSearchResult (uint32_t a, uint8_t last, uint8_t current)
{
	QTreeWidgetItem *item;
	char addrStr[8], lastStr[8], curStr[8];

	item = new QTreeWidgetItem();

	sprintf (addrStr, "$%04X", a);
	sprintf (lastStr, "%02X", last);
	sprintf (curStr , "%02X", current);

	//item->setFont( 0, font );
	//item->setFont( 1, font );
	//item->setFont( 2, font );

	item->setText( 0, tr(addrStr) );
	item->setText( 1, tr(lastStr) );
	item->setText( 2, tr(curStr)  );

	item->setTextAlignment( 0, Qt::AlignCenter);
	item->setTextAlignment( 1, Qt::AlignCenter);
	item->setTextAlignment( 2, Qt::AlignCenter);

	srchResults->addTopLevelItem( item );

	return 1;
}
//----------------------------------------------------------------------------
static int ShowCheatSearchResultsCallB (uint32 a, uint8 last, uint8 current)
{
	return win->addSearchResult( a, last, current );
}
//----------------------------------------------------------------------------
void GuiCheatsDialog_t::showCheatSearchResults(void)
{
	int total_matches;

	win = this;

	srchResults->clear();

	total_matches = FCEUI_CheatSearchGetCount ();

	FCEUI_CheatSearchGetRange (0, total_matches,
				   ShowCheatSearchResultsCallB);

	printf("Num Matches: %i \n", total_matches );
}
//----------------------------------------------------------------------------
void GuiCheatsDialog_t::resetSearchCallback(void)
{
	fceuWrapperLock();

	FCEUI_CheatSearchBegin ();

	showCheatSearchResults();

	fceuWrapperUnLock();

	knownValBtn->setEnabled(true);
	eqValBtn->setEnabled(true);
	neValBtn->setEnabled(true);
	grValBtn->setEnabled(true);
	ltValBtn->setEnabled(true);

	useNeVal->setEnabled(true);
	useGrVal->setEnabled(true);
	useLtVal->setEnabled(true);

	knownValEntry->setEnabled(true);
	neValEntry->setEnabled(true);
	grValEntry->setEnabled(true);
	ltValEntry->setEnabled(true);
}
//----------------------------------------------------------------------------
void GuiCheatsDialog_t::knownValueCallback(void)
{
	int value;
	//printf("Cheat Search Known!\n");
	fceuWrapperLock();

	//printf("%s\n", knownValEntry->text().toStdString().c_str() );

	value = strtol( knownValEntry->text().toStdString().c_str(), NULL, 16 );

	FCEUI_CheatSearchEnd (FCEU_SEARCH_NEWVAL_KNOWN, value, 0);

	showCheatSearchResults();

	fceuWrapperUnLock();
}
//----------------------------------------------------------------------------
void GuiCheatsDialog_t::equalValueCallback(void)
{
	//printf("Cheat Search Equal!\n");
	fceuWrapperLock();

	FCEUI_CheatSearchEnd (FCEU_SEARCH_PUERLY_RELATIVE_CHANGE, 0, 0);

	showCheatSearchResults();

	fceuWrapperUnLock();
}
//----------------------------------------------------------------------------
void GuiCheatsDialog_t::notEqualValueCallback(void)
{
	//printf("Cheat Search Not Equal!\n");
	int value;
	int checked = useNeVal->checkState() != Qt::Unchecked;

	fceuWrapperLock();

	if (checked)
	{
		value = strtol( neValEntry->text().toStdString().c_str(), NULL, 16 );

		FCEUI_CheatSearchEnd (FCEU_SEARCH_PUERLY_RELATIVE_CHANGE, 0, value);
	}
	else
	{
		FCEUI_CheatSearchEnd (FCEU_SEARCH_ANY_CHANGE, 0, 0);
	}

	showCheatSearchResults();

	fceuWrapperUnLock();
}
//----------------------------------------------------------------------------
void GuiCheatsDialog_t::greaterThanValueCallback(void)
{
	//printf("Cheat Search Greater Than!\n");
	int value;
	int checked = useGrVal->checkState() != Qt::Unchecked;

	fceuWrapperLock();

	if (checked)
	{
		value = strtol( grValEntry->text().toStdString().c_str(), NULL, 16 );

		FCEUI_CheatSearchEnd (FCEU_SEARCH_NEWVAL_GT_KNOWN, 0, value);
	}
	else
	{
		FCEUI_CheatSearchEnd (FCEU_SEARCH_NEWVAL_GT, 0, 0);
	}

	showCheatSearchResults();

	fceuWrapperUnLock();
}
//----------------------------------------------------------------------------
void GuiCheatsDialog_t::lessThanValueCallback(void)
{
	//printf("Cheat Search Less Than!\n");
	int value;
	int checked = useLtVal->checkState() != Qt::Unchecked;

	fceuWrapperLock();

	if (checked)
	{
		value = strtol( ltValEntry->text().toStdString().c_str(), NULL, 16 );

		FCEUI_CheatSearchEnd (FCEU_SEARCH_NEWVAL_LT_KNOWN, 0, value);
	}
	else
	{
		FCEUI_CheatSearchEnd (FCEU_SEARCH_NEWVAL_LT, 0, 0);
	}

	showCheatSearchResults();

	fceuWrapperUnLock();
}
//----------------------------------------------------------------------------
int GuiCheatsDialog_t::activeCheatListCB (char *name, uint32 a, uint8 v, int c, int s, int type, void *data)
{
	QTreeWidgetItem *item;
	char codeStr[32];

	if (c >= 0)
	{
		sprintf (codeStr, "$%04X?%02X:%02X", a,c,v);
	}
	else
	{
		sprintf (codeStr, "$%04X:%02X   ", a,v);
	}

	item = actvCheatList->topLevelItem(actvCheatIdx);

	if ( item == NULL )
	{
		item = new QTreeWidgetItem();

		actvCheatList->addTopLevelItem( item );
	}

	//item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsUserCheckable );
	item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemNeverHasChildren );

	item->setCheckState( 0, s ? Qt::Checked : Qt::Unchecked );

	item->setText( 0, tr(codeStr));
	item->setText( 1, tr(name)   );

	item->setTextAlignment( 0, Qt::AlignLeft);
	item->setTextAlignment( 1, Qt::AlignLeft);
	item->setTextAlignment( 2, Qt::AlignLeft);

	actvCheatIdx++;

	return 1;
}
//----------------------------------------------------------------------------
static int activeCheatListCB (char *name, uint32 a, uint8 v, int c, int s, int type, void *data)
{
	return win->activeCheatListCB( name, a, v, c, s, type, data );
}
//----------------------------------------------------------------------------
void GuiCheatsDialog_t::showActiveCheatList(bool redraw)
{
	win = this;

	actvCheatRedraw = redraw;

	if ( redraw )
	{
		actvCheatList->clear();
	}
	actvCheatIdx = 0;

	FCEUI_ListCheats (::activeCheatListCB, (void *) this);
}
//----------------------------------------------------------------------------
void GuiCheatsDialog_t::openCheatFile(void)
{
	FILE *fp;
	int ret, useNativeFileDialogVal;
	QString filename;
	std::string last;
	char dir[512];
	QFileDialog  dialog(this, tr("Open Cheat File") );

	dialog.setFileMode(QFileDialog::ExistingFile);

	dialog.setNameFilter(tr("Cheat files (*.cht *.CHT) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::AllDirs | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Open") );

	g_config->getOption ("SDL.LastOpenFile", &last );

	getDirFromFile( last.c_str(), dir );

	dialog.setDirectory( tr(dir) );

	// Check config option to use native file dialog or not
	g_config->getOption ("SDL.UseNativeFileDialog", &useNativeFileDialogVal);

	dialog.setOption(QFileDialog::DontUseNativeDialog, !useNativeFileDialogVal);

	dialog.show();
	ret = dialog.exec();

	if ( ret )
	{
		QStringList fileList;
		fileList = dialog.selectedFiles();

		if ( fileList.size() > 0 )
		{
			filename = fileList[0];
		}
	}

   if ( filename.isNull() )
   {
      return;
   }
	qDebug() << "selected file path : " << filename.toUtf8();

	g_config->setOption ("SDL.LastOpenFile", filename.toStdString().c_str() );

	fceuWrapperLock();

	fp = fopen (filename.toStdString().c_str(), "r");

	if (fp != NULL)
	{
		FCEU_LoadGameCheats (fp, 0);
		fclose (fp);
	}
	fceuWrapperUnLock();

	showActiveCheatList(true);

   return;
}
//----------------------------------------------------------------------------
void GuiCheatsDialog_t::saveCheatFile(void)
{
	FILE *fp;
	int ret, useNativeFileDialogVal;
	QString filename;
	char dir[512];
	QFileDialog  dialog(this, tr("Save Cheat File") );

	dialog.setFileMode(QFileDialog::AnyFile);

	dialog.setNameFilter(tr("Cheat files (*.cht *.CHT) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::AllDirs | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Save") );

	if ( GameInfo )
	{
		getFileBaseName( GameInfo->filename, dir );

		strcat( dir, ".cht");

		dialog.selectFile( dir );
	}

	sprintf( dir, "%s/cheats", FCEUI_GetBaseDirectory() );

	dialog.setDirectory( tr(dir) );

	// Check config option to use native file dialog or not
	g_config->getOption ("SDL.UseNativeFileDialog", &useNativeFileDialogVal);

	dialog.setOption(QFileDialog::DontUseNativeDialog, !useNativeFileDialogVal);

	dialog.show();
	ret = dialog.exec();

	if ( ret )
	{
		QStringList fileList;
		fileList = dialog.selectedFiles();

		if ( fileList.size() > 0 )
		{
			filename = fileList[0];
		}
	}

   if ( filename.isNull() )
   {
      return;
   }
	qDebug() << "selected file path : " << filename.toUtf8();

	//g_config->setOption ("SDL.LastOpenFile", filename.toStdString().c_str() );

	fceuWrapperLock();

	fp = FCEUD_UTF8fopen (filename.toStdString().c_str(), "wb");

	if (fp != NULL)
	{
		FCEU_SaveGameCheats (fp);
		fclose (fp);
	}
	fceuWrapperUnLock();

	showActiveCheatList(true);

   return;
}
//----------------------------------------------------------------------------
void  GuiCheatsDialog_t::addActvCheat(void)
{
	uint32 a = 0;
	uint8 v = 0;
	int c = -1;
	std::string name, cmpStr;

	a = strtoul( cheatAddrEntry->text().toStdString().c_str(), NULL, 16 );

	v = strtoul( cheatValEntry->text().toStdString().c_str(), NULL, 16 );

	cmpStr = cheatCmpEntry->text().toStdString();

	if ( isdigit( cmpStr[0] ) )
	{
		c = strtoul( cmpStr.c_str(), NULL, 16 );
	}
	else
	{
		c = -1;
	}

	name = cheatNameEntry->text().toStdString();

	fceuWrapperLock();
	FCEUI_AddCheat( name.c_str(), a, v, c, 1 );
	fceuWrapperUnLock();

	showActiveCheatList(true);
}
//----------------------------------------------------------------------------
void  GuiCheatsDialog_t::deleteActvCheat(void)
{
	QTreeWidgetItem *item;

	item = actvCheatList->currentItem();

	if ( item == NULL )
	{
		printf( "No Item Selected\n");
		return;
	}

	int row = actvCheatList->indexOfTopLevelItem(item);

	fceuWrapperLock();
	FCEUI_DelCheat (row);
	fceuWrapperUnLock();

	showActiveCheatList(true);

	cheatNameEntry->setText( tr("") );
	cheatAddrEntry->setText( tr("") );
	cheatValEntry->setText( tr("") );
	cheatCmpEntry->setText( tr("") );
}
//----------------------------------------------------------------------------
void  GuiCheatsDialog_t::updateCheatParameters(void)
{
	uint32 a = 0;
	uint8 v = 0;
	int c = -1, s = 0, type = 0;
	std::string name, cmpStr;
	QTreeWidgetItem *item;

	item = actvCheatList->currentItem();

	if ( item == NULL )
	{
		printf( "No Item Selected\n");
		return;
	}

	int row = actvCheatList->indexOfTopLevelItem(item);

	if ( FCEUI_GetCheat( row, NULL, &a, &v, &c, &s, &type) == 0 )
	{
		return;
	}
	//printf("Row: %i \n", row );

	a = strtoul( cheatAddrEntry->text().toStdString().c_str(), NULL, 16 );

	v = strtoul( cheatValEntry->text().toStdString().c_str(), NULL, 16 );

	cmpStr = cheatCmpEntry->text().toStdString();

	if ( isdigit( cmpStr[0] ) )
	{
		c = strtoul( cmpStr.c_str(), NULL, 16 );
	}
	else
	{
		c = -1;
	}

	name = cheatNameEntry->text().toStdString();

	//printf("Name: %s \n", name.c_str() );

	fceuWrapperLock();

	FCEUI_SetCheat( row, name.c_str(), a, v, c, s, type);

	fceuWrapperUnLock();

	showActiveCheatList(false);
}
//----------------------------------------------------------------------------
void 	GuiCheatsDialog_t::actvCheatItemClicked( QTreeWidgetItem *item, int column)
{
	uint32 a = 0;
	uint8 v = 0;
	int c = -1, s = 0, type = 0;
	char *name = NULL;
	char stmp[64];

	int row = actvCheatList->indexOfTopLevelItem(item);

	//printf("Row: %i Column: %i \n", row, column );

	if ( FCEUI_GetCheat( row, &name, &a, &v, &c, &s, &type) == 0 )
	{
		return;
	}

	if ( column == 0 )
	{
		int isChecked = item->checkState( column ) != Qt::Unchecked;

		if ( isChecked != s )
		{
			//printf("Toggle Cheat: %i\n", isChecked);
			FCEUI_ToggleCheat( row );
		}
	}
	sprintf( stmp, "%04X", a );
	cheatAddrEntry->setText( tr(stmp) );

	sprintf( stmp, "%02X", v );
	cheatValEntry->setText( tr(stmp) );

	if ( c >= 0 )
	{
		sprintf( stmp, "%02X", c );
		cheatCmpEntry->setText( tr(stmp) );
	}
	else
	{
		cheatCmpEntry->setText( tr("") );
	}

	if ( name != NULL )
	{
		cheatNameEntry->setText( tr(name) );
	}
	else
	{
		cheatNameEntry->setText( tr("") );
	}
}
//----------------------------------------------------------------------------
void GuiCheatsDialog_t::globalEnableCheats(int state)
{
	fceuWrapperLock();
	FCEUI_GlobalToggleCheat( state != Qt::Unchecked );
	fceuWrapperUnLock();
}
//----------------------------------------------------------------------------
void GuiCheatsDialog_t::autoLoadSaveCheats(int state)
{
	if ( state == Qt::Unchecked )
	{
		printf("If this option is unchecked, you must manually save the cheats by yourself, or all the changes you made to the cheat list would be discarded silently without any asking once you close the game!\nDo you really want to do it in this way?");
		disableAutoLSCheats = 2;
	}
	else
	{
		disableAutoLSCheats = 0;
	}
}
//----------------------------------------------------------------------------
void GuiCheatsDialog_t::pauseWindowState(int state)
{
	pauseWhileActive = (state != Qt::Unchecked);

	if (pauseWhileActive)
	{
		if (EmulationPaused == 0)
		{
			EmulationPaused = 1;
			wasPausedByCheats = true;
		}
	}
	else
	{
		if (EmulationPaused && wasPausedByCheats)
		{
			EmulationPaused = 0;
		}
		wasPausedByCheats = false;
	}
	FCEU_printf ("Emulation paused: %d\n", EmulationPaused);
}
//----------------------------------------------------------------------------
