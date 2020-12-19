// RamSearch.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <string>

#include <SDL.h>
#include <QMenuBar>
#include <QAction>
#include <QHeaderView>
#include <QCloseEvent>
#include <QGroupBox>
#include <QLineEdit>
#include <QRadioButton>
#include <QFileDialog>
#include <QValidator>
#include <QPainter>

#include "../../types.h"
#include "../../fceu.h"
#include "../../cheat.h"
#include "../../debug.h"
#include "../../movie.h"

#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/fceuWrapper.h"
#include "Qt/RamWatch.h"
#include "Qt/RamSearch.h"
#include "Qt/HexEditor.h"
#include "Qt/CheatsConf.h"
#include "Qt/ConsoleWindow.h"
#include "Qt/ConsoleUtilities.h"

static bool ShowROM = false;
static RamSearchDialog_t *ramSearchWin = NULL;

struct memoryState_t
{
	union {
	   int8_t    i;
	   uint8_t   u;
	} v8;
	union {
	   int16_t    i;
	   uint16_t   u;
	} v16;
	union {
	   int32_t    i;
	   uint32_t   u;
	} v32;
};

struct memoryLocation_t
{
	int  addr;

	memoryState_t  val;

	std::vector <memoryState_t> hist;

	uint32_t  chgCount;
	uint64_t  elimMask;

	memoryLocation_t(void)
	{
		addr = 0; val.v32.u = 0;
		chgCount = 0; elimMask = 0;
	}
};
static struct memoryLocation_t  memLoc[0x10000];

static std::list <struct memoryLocation_t*>   actvSrchList;
static std::list <struct memoryLocation_t*> deactvSrchList;
static std::vector <int> deactvFrameStack;

static int cmpOp = '=';
static int dpySize = 'b';
static int dpyType = 's';
static bool  chkMisAligned = false;

class ramSearchInputValidator : public QValidator
{ 
   public:
   ramSearchInputValidator(QObject *parent)
      : QValidator(parent)
   {
   }

   QValidator::State validate(QString &input, int &pos) const
   {
      int i;
      //printf("Validate: %i '%s'\n", input.size(), input.toStdString().c_str() );

      if ( input.size() == 0 )
      {
         return QValidator::Acceptable;
      }
      std::string s = input.toStdString();
      i=0;

      if ( (s[i] == '-') || (s[i] == '+') )
      {
         i++;
      }
      if ( s[i] == 0 )
      {
         return QValidator::Acceptable;
      }

      if ( (s[i] == '$') || ((s[i] == '0') && ( tolower(s[i+1]) == 'x')) )
      {
         if ( s[i] == '$' )
         {
            i++;
         }
         else
         {
            i += 2;
         }

         if ( s[i] == 0 )
         {
            return QValidator::Acceptable;
         }

         if ( !isxdigit(s[i]) )
         {
            return QValidator::Invalid;
         }
         while ( isxdigit(s[i]) ) i++;

         if ( s[i] == 0 )
         {
            return QValidator::Acceptable;
         }
      }
      else if ( isdigit(s[i]) )
      {
         while ( isdigit(s[i]) ) i++;

         if ( s[i] == 0 )
         {
            return QValidator::Acceptable;
         }
      }
      return QValidator::Invalid;
   }

};

//----------------------------------------------------------------------------
void openRamSearchWindow( QWidget *parent )
{
	if ( ramSearchWin != NULL )
	{
		return;
	}
   ramSearchWin = new RamSearchDialog_t(parent);
	
   ramSearchWin->show();
}

//----------------------------------------------------------------------------
RamSearchDialog_t::RamSearchDialog_t(QWidget *parent)
	: QDialog( parent, Qt::Window )
{
	QVBoxLayout *mainLayout;
	QHBoxLayout *hbox, *hbox1, *hbox2, *hbox3;
	QVBoxLayout *vbox, *vbox1, *vbox2;
	QGridLayout *grid;
	QGroupBox *frame;
   ramSearchInputValidator *inpValidator;

	setWindowTitle("RAM Search");

	resize( 512, 512 );

	mainLayout = new QVBoxLayout();
	hbox1      = new QHBoxLayout();

	mainLayout->addLayout( hbox1, 100 );

	grid    = new QGridLayout();
	ramView = new QRamSearchView(this);
	vbar    = new QScrollBar( Qt::Vertical, this );
	hbar    = new QScrollBar( Qt::Horizontal, this );
	grid->addWidget( ramView, 0, 0 );
	grid->addWidget( vbar   , 0, 1 );
	grid->addWidget( hbar   , 1, 0 );

	connect( hbar, SIGNAL(valueChanged(int)), this, SLOT(hbarChanged(int)) );
   connect( vbar, SIGNAL(valueChanged(int)), this, SLOT(vbarChanged(int)) );

	ramView->setScrollBars( hbar, vbar );
	hbar->setMinimum(0);
	hbar->setMaximum(100);
	vbar->setMinimum(0);
	vbar->setMaximum(ShowROM ? 0x10000 : 0x8000);
   vbar->setValue(0);

	vbox  = new QVBoxLayout();
	hbox1->addLayout( grid, 100);
	hbox1->addLayout( vbox, 1  );

	searchButton = new QPushButton( tr("Search") );
	vbox->addWidget( searchButton );
	connect( searchButton, SIGNAL(clicked(void)), this, SLOT(runSearch(void)));

	resetButton = new QPushButton( tr("Reset") );
	vbox->addWidget( resetButton );
	connect( resetButton, SIGNAL(clicked(void)), this, SLOT(resetSearch(void)));

	clearChangeButton = new QPushButton( tr("Clear Change") );
	vbox->addWidget( clearChangeButton );
	connect( clearChangeButton, SIGNAL(clicked(void)), this, SLOT(clearChangeCounts(void)));

	undoButton = new QPushButton( tr("Undo") );
	vbox->addWidget( undoButton );
	connect( undoButton, SIGNAL(clicked(void)), this, SLOT(undoSearch(void)));
	undoButton->setEnabled(false);
	
	searchROMCbox = new QCheckBox( tr("Search ROM") );
	vbox->addWidget( searchROMCbox );
	searchROMCbox->setChecked( ShowROM );
	connect( searchROMCbox, SIGNAL(stateChanged(int)), this, SLOT(searchROMChanged(int)));

	elimButton = new QPushButton( tr("Eliminate") );
	vbox->addWidget( elimButton );
	connect( elimButton, SIGNAL(clicked(void)), this, SLOT(eliminateSelAddr(void)));
	elimButton->setEnabled(false);

	watchButton = new QPushButton( tr("Watch") );
	vbox->addWidget( watchButton );
	connect( watchButton, SIGNAL(clicked(void)), this, SLOT(addRamWatchClicked(void)));
	watchButton->setEnabled(false);

	addCheatButton = new QPushButton( tr("Add Cheat") );
	vbox->addWidget( addCheatButton );
	connect( addCheatButton, SIGNAL(clicked(void)), this, SLOT(addCheatClicked(void)));
	addCheatButton->setEnabled(false);

	hexEditButton = new QPushButton( tr("Hex Editor") );
	vbox->addWidget( hexEditButton );
	connect( hexEditButton, SIGNAL(clicked(void)), this, SLOT(hexEditSelAddr(void)));
	hexEditButton->setEnabled(false);

	hbox2 = new QHBoxLayout();
	mainLayout->addLayout( hbox2, 1 );
	frame = new QGroupBox( tr("Comparison Operator") );
	vbox  = new QVBoxLayout();

	hbox2->addWidget( frame );
	frame->setLayout(vbox);

	lt_btn = new QRadioButton( tr("Less Than") );
	gt_btn = new QRadioButton( tr("Greater Than") );
	le_btn = new QRadioButton( tr("Less Than or Equal To") );
	ge_btn = new QRadioButton( tr("Greater Than or Equal To") );
	eq_btn = new QRadioButton( tr("Equal To") );
	ne_btn = new QRadioButton( tr("Not Equal To") );
	df_btn = new QRadioButton( tr("Different By:") );
	md_btn = new QRadioButton( tr("Modulo") );

	lt_btn->setChecked( cmpOp == '<' );
	gt_btn->setChecked( cmpOp == '>' );
	le_btn->setChecked( cmpOp == 'l' );
	ge_btn->setChecked( cmpOp == 'm' );
	eq_btn->setChecked( cmpOp == '=' );
	ne_btn->setChecked( cmpOp == '!' );
	df_btn->setChecked( cmpOp == 'd' );
	md_btn->setChecked( cmpOp == '%' );

   connect( lt_btn, SIGNAL(clicked(void)), this, SLOT(opLtClicked(void)) );
   connect( gt_btn, SIGNAL(clicked(void)), this, SLOT(opGtClicked(void)) );
   connect( le_btn, SIGNAL(clicked(void)), this, SLOT(opLeClicked(void)) );
   connect( ge_btn, SIGNAL(clicked(void)), this, SLOT(opGeClicked(void)) );
   connect( eq_btn, SIGNAL(clicked(void)), this, SLOT(opEqClicked(void)) );
   connect( ne_btn, SIGNAL(clicked(void)), this, SLOT(opNeClicked(void)) );
   connect( df_btn, SIGNAL(clicked(void)), this, SLOT(opDfClicked(void)) );
   connect( md_btn, SIGNAL(clicked(void)), this, SLOT(opMdClicked(void)) );

	diffByEdit = new QLineEdit();
	moduloEdit = new QLineEdit();

	diffByEdit->setEnabled( cmpOp == 'd' );
	moduloEdit->setEnabled( cmpOp == '%' );

   inpValidator = new ramSearchInputValidator(this);
   diffByEdit->setMaxLength( 16 );
   diffByEdit->setCursorPosition(0);
   diffByEdit->setValidator( inpValidator );

   moduloEdit->setMaxLength( 16 );
   moduloEdit->setCursorPosition(0);
   moduloEdit->setValidator( inpValidator );

	vbox->addWidget( lt_btn );
	vbox->addWidget( gt_btn );
	vbox->addWidget( le_btn );
	vbox->addWidget( ge_btn );
	vbox->addWidget( eq_btn );
	vbox->addWidget( ne_btn );

	hbox = new QHBoxLayout();
	vbox->addLayout( hbox );
	hbox->addWidget( df_btn );
	hbox->addWidget( diffByEdit );

	hbox = new QHBoxLayout();
	vbox->addLayout( hbox );
	hbox->addWidget( md_btn );
	hbox->addWidget( moduloEdit );

	vbox1 = new QVBoxLayout();
	grid  = new QGridLayout();
	hbox2->addLayout( vbox1 );
	frame = new QGroupBox( tr("Compare To/By") );
	frame->setLayout( grid );
	vbox1->addWidget( frame );

	pv_btn = new QRadioButton( tr("Previous Value") );
	sv_btn = new QRadioButton( tr("Specific Value:") );
	sa_btn = new QRadioButton( tr("Specific Address:") );
	nc_btn = new QRadioButton( tr("Number of Changes:") );

	pv_btn->setChecked(true);

   connect( pv_btn, SIGNAL(clicked(void)), this, SLOT(pvBtnClicked(void)) );
   connect( sv_btn, SIGNAL(clicked(void)), this, SLOT(svBtnClicked(void)) );
   connect( sa_btn, SIGNAL(clicked(void)), this, SLOT(saBtnClicked(void)) );
   connect( nc_btn, SIGNAL(clicked(void)), this, SLOT(ncBtnClicked(void)) );

	specValEdit   = new QLineEdit();
	specAddrEdit  = new QLineEdit();
	numChangeEdit = new QLineEdit();

   specValEdit->setValidator( inpValidator );
   specAddrEdit->setValidator( inpValidator );
   numChangeEdit->setValidator( inpValidator );

   specValEdit->setEnabled(false);
   specAddrEdit->setEnabled(false);
   numChangeEdit->setEnabled(false);

	grid->addWidget( pv_btn       , 0, 0, Qt::AlignLeft );
	grid->addWidget( sv_btn       , 1, 0, Qt::AlignLeft );
	grid->addWidget( specValEdit  , 1, 1, Qt::AlignLeft );
	grid->addWidget( sa_btn       , 2, 0, Qt::AlignLeft );
	grid->addWidget( specAddrEdit , 2, 1, Qt::AlignLeft );
	grid->addWidget( nc_btn       , 3, 0, Qt::AlignLeft );
	grid->addWidget( numChangeEdit, 3, 1, Qt::AlignLeft );

	vbox  = new QVBoxLayout();
	hbox3 = new QHBoxLayout();
	frame = new QGroupBox( tr("Data Size") );
	frame->setLayout( vbox );
	vbox1->addLayout( hbox3 );
	hbox3->addWidget( frame );

	ds1_btn = new QRadioButton( tr("1 Byte") );
	ds2_btn = new QRadioButton( tr("2 Byte") );
	ds4_btn = new QRadioButton( tr("4 Byte") );
	misalignedCbox = new QCheckBox( tr("Check Misaligned") );
	misalignedCbox->setEnabled(dpySize != 'b');
	misalignedCbox->setChecked(chkMisAligned);
	connect( misalignedCbox, SIGNAL(stateChanged(int)), this, SLOT(misalignedChanged(int)));

	ds1_btn->setChecked( dpySize == 'b' );
	ds2_btn->setChecked( dpySize == 'w' );
	ds4_btn->setChecked( dpySize == 'd' );

	connect( ds1_btn, SIGNAL(clicked(void)), this, SLOT(ds1Clicked(void)));
	connect( ds2_btn, SIGNAL(clicked(void)), this, SLOT(ds2Clicked(void)));
	connect( ds4_btn, SIGNAL(clicked(void)), this, SLOT(ds4Clicked(void)));

	vbox->addWidget( ds1_btn );
	vbox->addWidget( ds2_btn );
	vbox->addWidget( ds4_btn );
	vbox->addWidget( misalignedCbox );

	vbox  = new QVBoxLayout();
	vbox2 = new QVBoxLayout();
	frame = new QGroupBox( tr("Data Type / Display") );
	frame->setLayout( vbox );
	vbox2->addWidget( frame );
	hbox3->addLayout( vbox2 );

	signed_btn   = new QRadioButton( tr("Signed") );
	unsigned_btn = new QRadioButton( tr("Unsigned") );
	hex_btn      = new QRadioButton( tr("Hexadecimal") );

	vbox->addWidget( signed_btn );
	vbox->addWidget( unsigned_btn );
	vbox->addWidget( hex_btn );

	signed_btn->setChecked( dpyType == 's' );
	unsigned_btn->setChecked( dpyType == 'u' );
	hex_btn->setChecked( dpyType == 'h' );

	connect( signed_btn  , SIGNAL(clicked(void)), this, SLOT(signedTypeClicked(void)));
	connect( unsigned_btn, SIGNAL(clicked(void)), this, SLOT(unsignedTypeClicked(void)));
	connect( hex_btn     , SIGNAL(clicked(void)), this, SLOT(hexTypeClicked(void)));

	autoSearchCbox = new QCheckBox( tr("Auto-Search") );
	autoSearchCbox->setEnabled(true);
	vbox2->addWidget( autoSearchCbox );

	setLayout( mainLayout );

	cycleCounter = 0;
	frameCounterLastPass = currFrameCounter;

	resetSearch();

	updateTimer  = new QTimer( this );

   connect( updateTimer, &QTimer::timeout, this, &RamSearchDialog_t::periodicUpdate );

	updateTimer->start( 8 ); // ~120hz
}
//----------------------------------------------------------------------------
RamSearchDialog_t::~RamSearchDialog_t(void)
{
	updateTimer->stop();
	printf("Destroy RAM Search Window\n");
	ramSearchWin = NULL;

	actvSrchList.clear();
	deactvSrchList.clear();
	deactvFrameStack.clear();

	for (unsigned int addr=0; addr<0x08000; addr++)
	{
		memLoc[addr].hist.clear();
	}
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::closeEvent(QCloseEvent *event)
{
   printf("RAM Search Close Window Event\n");
   done(0);
	deleteLater();
   event->accept();
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::closeWindow(void)
{
   //printf("Close Window\n");
   done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::periodicUpdate(void)
{
   int selAddr = -1;

   fceuWrapperLock();
	if ( currFrameCounter != frameCounterLastPass )
	{
      //if ( currFrameCounter != (frameCounterLastPass+1) )
      //{
      //   printf("Warning: Ram Search Missed Frame: %i \n", currFrameCounter );
      //}  
		updateRamValues();

      if ( autoSearchCbox->isChecked() )
      {
         runSearch();
      }
		frameCounterLastPass = currFrameCounter;
	}
   fceuWrapperUnLock();

   undoButton->setEnabled( deactvFrameStack.size() > 0 );

   selAddr = ramView->getSelAddr();

   if ( selAddr >= 0 )
   {
      elimButton->setEnabled(true);
      watchButton->setEnabled(true);
      addCheatButton->setEnabled(true);
      hexEditButton->setEnabled(true);
   }
   else
   {
      elimButton->setEnabled(false);
      watchButton->setEnabled(false);
      addCheatButton->setEnabled(false);
      hexEditButton->setEnabled(false);
   }

	if ( (cycleCounter % 10) == 0)
	{
		ramView->update();
	}
	cycleCounter++;
}
//----------------------------------------------------
void RamSearchDialog_t::hbarChanged(int val)
{
   ramView->update();
}
//----------------------------------------------------
void RamSearchDialog_t::vbarChanged(int val)
{
   ramView->update();
}
//----------------------------------------------------
void RamSearchDialog_t::searchROMChanged(int state)
{
	ShowROM = (state != Qt::Unchecked);
}
//----------------------------------------------------
void RamSearchDialog_t::misalignedChanged(int state)
{
	chkMisAligned = (state != Qt::Unchecked);

	calcRamList();
}
//----------------------------------------------------------------------------
static bool memoryAddrCompare( memoryLocation_t *loc1, memoryLocation_t *loc2 )
{
	return loc1->addr < loc2->addr;
}
static void sortActvMemList(void)
{
	actvSrchList.sort( memoryAddrCompare );
}

// basic comparison functions:
static bool LessCmp (int64_t x, int64_t y, int64_t i)        { return x < y; }
static bool MoreCmp (int64_t x, int64_t y, int64_t i)        { return x > y; }
static bool LessEqualCmp (int64_t x, int64_t y, int64_t i)   { return x <= y; }
static bool MoreEqualCmp (int64_t x, int64_t y, int64_t i)   { return x >= y; }
static bool EqualCmp (int64_t x, int64_t y, int64_t i)       { return x == y; }
static bool UnequalCmp (int64_t x, int64_t y, int64_t i)     { return x != y; }
static bool DiffByCmp (int64_t x, int64_t y, int64_t p)      { return x - y == p || y - x == p; }
static bool ModIsCmp (int64_t x, int64_t y, int64_t p)       { return p && x % p == y; }

static int64_t getLineEditValue( QLineEdit *edit, bool forceHex = false )
{
   int64_t val=0;
   std::string s;

   s = edit->text().toStdString();

   if ( s.size() > 0 )
   {
      val = strtoll( s.c_str(), NULL, forceHex ? 16 : 0 );
   }
   return val;
}

//----------------------------------------------------------------------------
void RamSearchDialog_t::SearchRelative(void)
{
   int elimCount = 0;
	std::list <struct memoryLocation_t*>::iterator it;
  	memoryLocation_t *loc = NULL;
   int64_t x = 0, y = 0, p = 0;
   bool (*cmpFun)(int64_t x, int64_t y, int64_t p) = NULL;
   bool storeHistory = !autoSearchCbox->isChecked();

   switch ( cmpOp )
   {
      case '<':
         cmpFun = LessCmp;
      break;
      case '>':
         cmpFun = MoreCmp;
      break;
      case '=':
         cmpFun = EqualCmp;
      break;
      case '!':
         cmpFun = UnequalCmp;
      break;
      case 'l':
         cmpFun = LessEqualCmp;
      break;
      case 'm':
         cmpFun = MoreEqualCmp;
      break;
      case 'd':
         cmpFun = DiffByCmp;
         p      = getLineEditValue( diffByEdit );
      break;
      case '%':
         cmpFun = ModIsCmp;
         p      = getLineEditValue( moduloEdit );
      break;
      default:
         cmpFun = NULL;
      break;
   }

   if ( cmpFun == NULL )
   {
      return;
   }
   //printf("Performing Relative Search Operation %zi: '%c'  '%lli'  '0x%llx' \n", deactvFrameStack.size()+1, cmpOp, (long long int)p, (unsigned long long int)p );

	it = actvSrchList.begin();

	while (it != actvSrchList.end())
	{
      loc = *it;

      switch ( dpySize )
      {
         default:
         case 'b':
         {
            if ( dpyType == 's')
            {
               x = loc->val.v8.i;
				   y = loc->hist.back().v8.i;
            }
            else
            {
               x = loc->val.v8.u;
				   y = loc->hist.back().v8.u;
            }
         }
         break;
         case 'w':
         {
            if ( dpyType == 's')
            {
               x = loc->val.v16.i;
				   y = loc->hist.back().v16.i;
            }
            else
            {
               x = loc->val.v16.u;
				   y = loc->hist.back().v16.u;
            }
         }
         break;
         case 'd':
         {
            if ( dpyType == 's')
            {
               x = loc->val.v32.i;
				   y = loc->hist.back().v32.i;
            }
            else
            {
               x = loc->val.v32.u;
				   y = loc->hist.back().v32.u;
            }
         }
         break;
      }

      if ( cmpFun( x, y, p ) == false )
      {
         //printf("Eliminated Address: $%04X\n", loc->addr );
         it = actvSrchList.erase(it);

         if ( storeHistory )
         {
            deactvSrchList.push_back( loc ); elimCount++;
         }
      }
      else 
      {
         if ( storeHistory )
         {
			   loc->hist.push_back( loc->val );
         }
         it++;
      }
   }

   if ( storeHistory )
   {
      deactvFrameStack.push_back( elimCount );
   }
   
	vbar->setMaximum( actvSrchList.size() );
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::SearchSpecificValue(void)
{
   int elimCount = 0;
	std::list <struct memoryLocation_t*>::iterator it;
  	memoryLocation_t *loc = NULL;
   int64_t x = 0, y = 0, p = 0;
   bool (*cmpFun)(int64_t x, int64_t y, int64_t p) = NULL;
   bool storeHistory = !autoSearchCbox->isChecked();

   switch ( cmpOp )
   {
      case '<':
         cmpFun = LessCmp;
      break;
      case '>':
         cmpFun = MoreCmp;
      break;
      case '=':
         cmpFun = EqualCmp;
      break;
      case '!':
         cmpFun = UnequalCmp;
      break;
      case 'l':
         cmpFun = LessEqualCmp;
      break;
      case 'm':
         cmpFun = MoreEqualCmp;
      break;
      case 'd':
         cmpFun = DiffByCmp;
         p      = getLineEditValue( diffByEdit );
      break;
      case '%':
         cmpFun = ModIsCmp;
         p      = getLineEditValue( moduloEdit );
      break;
      default:
         cmpFun = NULL;
      break;
   }

   if ( cmpFun == NULL )
   {
      return;
   }
   y = getLineEditValue( specValEdit );

   //printf("Performing Specific Value Search Operation %zi: 'x %c %lli' '%lli'  '0x%llx' \n", deactvFrameStack.size()+1, cmpOp,
   //     (long long int)y, (long long int)p, (unsigned long long int)p );

	it = actvSrchList.begin();

	while (it != actvSrchList.end())
	{
      loc = *it;

      switch ( dpySize )
      {
         default:
         case 'b':
         {
            if ( dpyType == 's')
            {
               x = loc->val.v8.i;
            }
            else
            {
               x = loc->val.v8.u;
            }
         }
         break;
         case 'w':
         {
            if ( dpyType == 's')
            {
               x = loc->val.v16.i;
            }
            else
            {
               x = loc->val.v16.u;
            }
         }
         break;
         case 'd':
         {
            if ( dpyType == 's')
            {
               x = loc->val.v32.i;
            }
            else
            {
               x = loc->val.v32.u;
            }
         }
         break;
      }

      if ( cmpFun( x, y, p ) == false )
      {
         //printf("Eliminated Address: $%04X\n", loc->addr );
         it = actvSrchList.erase(it);

         if ( storeHistory )
         {
            deactvSrchList.push_back( loc ); elimCount++;
         }
      }
      else 
      {
         if ( storeHistory )
         {
			   loc->hist.push_back( loc->val );
         }
         it++;
      }
   }

   if ( storeHistory )
   {
      deactvFrameStack.push_back( elimCount );
   }
   
	vbar->setMaximum( actvSrchList.size() );
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::SearchSpecificAddress(void)
{
   int elimCount = 0;
	std::list <struct memoryLocation_t*>::iterator it;
  	memoryLocation_t *loc = NULL;
   int64_t x = 0, y = 0, p = 0;
   bool (*cmpFun)(int64_t x, int64_t y, int64_t p) = NULL;
   bool storeHistory = !autoSearchCbox->isChecked();

   switch ( cmpOp )
   {
      case '<':
         cmpFun = LessCmp;
      break;
      case '>':
         cmpFun = MoreCmp;
      break;
      case '=':
         cmpFun = EqualCmp;
      break;
      case '!':
         cmpFun = UnequalCmp;
      break;
      case 'l':
         cmpFun = LessEqualCmp;
      break;
      case 'm':
         cmpFun = MoreEqualCmp;
      break;
      case 'd':
         cmpFun = DiffByCmp;
         p      = getLineEditValue( diffByEdit );
      break;
      case '%':
         cmpFun = ModIsCmp;
         p      = getLineEditValue( moduloEdit );
      break;
      default:
         cmpFun = NULL;
      break;
   }

   if ( cmpFun == NULL )
   {
      return;
   }
   y = getLineEditValue( specAddrEdit );

   //printf("Performing Specific Address Search Operation %zi: 'x %c 0x%llx' '%lli'  '0x%llx' \n", deactvFrameStack.size()+1, cmpOp,
   //     (unsigned long long int)y, (long long int)p, (unsigned long long int)p );

	it = actvSrchList.begin();

	while (it != actvSrchList.end())
	{
      loc = *it;

      x = loc->addr;

      if ( cmpFun( x, y, p ) == false )
      {
         //printf("Eliminated Address: $%04X\n", loc->addr );
         it = actvSrchList.erase(it);

         if ( storeHistory )
         {
            deactvSrchList.push_back( loc ); elimCount++;
         }
      }
      else 
      {
         if ( storeHistory )
         {
			   loc->hist.push_back( loc->val );
         }
         it++;
      }
   }

   if ( storeHistory )
   {
      deactvFrameStack.push_back( elimCount );
   }
   
	vbar->setMaximum( actvSrchList.size() );
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::SearchNumberChanges(void)
{
   int elimCount = 0;
	std::list <struct memoryLocation_t*>::iterator it;
  	memoryLocation_t *loc = NULL;
   int64_t x = 0, y = 0, p = 0;
   bool (*cmpFun)(int64_t x, int64_t y, int64_t p) = NULL;
   bool storeHistory = !autoSearchCbox->isChecked();

   switch ( cmpOp )
   {
      case '<':
         cmpFun = LessCmp;
      break;
      case '>':
         cmpFun = MoreCmp;
      break;
      case '=':
         cmpFun = EqualCmp;
      break;
      case '!':
         cmpFun = UnequalCmp;
      break;
      case 'l':
         cmpFun = LessEqualCmp;
      break;
      case 'm':
         cmpFun = MoreEqualCmp;
      break;
      case 'd':
         cmpFun = DiffByCmp;
         p      = getLineEditValue( diffByEdit );
      break;
      case '%':
         cmpFun = ModIsCmp;
         p      = getLineEditValue( moduloEdit );
      break;
      default:
         cmpFun = NULL;
      break;
   }

   if ( cmpFun == NULL )
   {
      return;
   }
   y = getLineEditValue( numChangeEdit );

   //printf("Performing Number of Changes Search Operation %zi: 'x %c 0x%llx' '%lli'  '0x%llx' \n", deactvFrameStack.size()+1, cmpOp,
   //     (unsigned long long int)y, (long long int)p, (unsigned long long int)p );

	it = actvSrchList.begin();

	while (it != actvSrchList.end())
	{
      loc = *it;

      x = loc->chgCount;

      if ( cmpFun( x, y, p ) == false )
      {
         //printf("Eliminated Address: $%04X\n", loc->addr );
         it = actvSrchList.erase(it);

         if ( storeHistory )
         {
            deactvSrchList.push_back( loc ); elimCount++;
         }
      }
      else 
      {
         if ( storeHistory )
         {
			   loc->hist.push_back( loc->val );
         }
         it++;
      }
   }

   if ( storeHistory )
   {
      deactvFrameStack.push_back( elimCount );
   }
   
	vbar->setMaximum( actvSrchList.size() );
}
//----------------------------------------------------------------------------
static unsigned int ReadValueAtHardwareAddress(int address, unsigned int size)
{
	unsigned int value = 0;
	int maxAddr = ShowROM ? 0x10000 : 0x8000;

	// read as little endian
	for (unsigned int i = 0; i < size; i++)
	{
		if ( address < maxAddr )
		{
			value <<= 8;
			value |= GetMem(address);
			address++;
		}
	}
	return value;
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::runSearch(void)
{
   if ( pv_btn->isChecked() )
   {
      // Relative Value
      SearchRelative();
   }
   else if ( sv_btn->isChecked() )
   {
      // Specific Value
      SearchSpecificValue();
   }
   else if ( sa_btn->isChecked() )
   {
      // Specific Address
      SearchSpecificAddress();
   }
   else if ( nc_btn->isChecked() )
   {
      // Number of Changes
      SearchNumberChanges();
   }

   undoButton->setEnabled( deactvFrameStack.size() > 0 );
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::resetSearch(void)
{
	actvSrchList.clear();
	deactvSrchList.clear();
	deactvFrameStack.clear();

	for (unsigned int addr=0; addr<0x10000; addr++)
	{
		memLoc[addr].hist.clear();
		memLoc[addr].addr      = addr;
		memLoc[addr].val.v8.u  = GetMem(addr);
		memLoc[addr].val.v16.u = ReadValueAtHardwareAddress(addr, 2);
		memLoc[addr].val.v32.u = ReadValueAtHardwareAddress(addr, 4);
		memLoc[addr].elimMask  = 0;
		memLoc[addr].chgCount  = 0;
		memLoc[addr].hist.push_back( memLoc[addr].val );
	}

	calcRamList();

	undoButton->setEnabled(false);
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::undoSearch(void)
{
   int elimCount = 0;
  	memoryLocation_t *loc = NULL;
	std::list <struct memoryLocation_t*>::iterator it;

   if ( deactvFrameStack.empty() )
   {
      printf("Error: UNDO Stack is empty\n");
      return;
   }
   printf("UNDO Search Operation: %zi \n", deactvFrameStack.size() );
   // To Undo a search operation:
   // 1. Loop through all current active values and revert previous value back to what it was before the search
   // 2. Get the number of eliminated values from the deactivate search stack and tranfer those values back into the active search list.

	it = actvSrchList.begin();

   while (it != actvSrchList.end())
	{
      loc = *it;

      if ( !loc->hist.empty() )
      {
         loc->hist.pop_back();
      }
      it++;
   }

   elimCount = deactvFrameStack.back();
   deactvFrameStack.pop_back();

   while ( elimCount > 0 )
   {
      if ( deactvSrchList.empty() )
      {
         printf("Error: Something went wrong with UNDO operation\n");
         break;
      }
      loc = deactvSrchList.back();
      deactvSrchList.pop_back();
      actvSrchList.push_back( loc );
      elimCount--;
   }

   sortActvMemList();

	undoButton->setEnabled( deactvFrameStack.size() > 0 );
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::clearChangeCounts(void)
{
	for (unsigned int addr=0; addr<0x10000; addr++)
	{
		memLoc[addr].chgCount  = 0;
	}
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::eliminateSelAddr(void)
{
   int elimCount = 0, op = '!';
	std::list <struct memoryLocation_t*>::iterator it;
  	memoryLocation_t *loc = NULL;
   int64_t x = 0, y = 0, p = 0;
   bool (*cmpFun)(int64_t x, int64_t y, int64_t p) = NULL;

   switch ( op )
   {
      case '<':
         cmpFun = LessCmp;
      break;
      case '>':
         cmpFun = MoreCmp;
      break;
      case '=':
         cmpFun = EqualCmp;
      break;
      case '!':
         cmpFun = UnequalCmp;
      break;
      case 'l':
         cmpFun = LessEqualCmp;
      break;
      case 'm':
         cmpFun = MoreEqualCmp;
      break;
      case 'd':
         cmpFun = DiffByCmp;
         p      = getLineEditValue( diffByEdit );
      break;
      case '%':
         cmpFun = ModIsCmp;
         p      = getLineEditValue( moduloEdit );
      break;
      default:
         cmpFun = NULL;
      break;
   }

   if ( cmpFun == NULL )
   {
      return;
   }
   y = ramView->getSelAddr();

   if ( y < 0 )
   {
      return;
   }

   printf("Performing Eliminate Address Operation %zi: 'x %c 0x%llx' '%lli'  '0x%llx' \n", deactvFrameStack.size()+1, cmpOp,
        (unsigned long long int)y, (long long int)p, (unsigned long long int)p );

	it = actvSrchList.begin();

	while (it != actvSrchList.end())
	{
      loc = *it;

      x = loc->addr;

      if ( cmpFun( x, y, p ) == false )
      {
         //printf("Eliminated Address: $%04X\n", loc->addr );
         it = actvSrchList.erase(it);

         deactvSrchList.push_back( loc ); elimCount++;
      }
      else 
      {
			loc->hist.push_back( loc->val );
         it++;
      }
   }

   deactvFrameStack.push_back( elimCount );
   
	vbar->setMaximum( actvSrchList.size() );
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::addCheatClicked(void)
{
   int addr = ramView->getSelAddr();
   char desc[128];

   if ( addr < 0 )
   {
      return;
   }
   strcpy( desc, "Quick Cheat Add");

	FCEUI_AddCheat( desc, addr, GetMem(addr), -1, 1 );

   updateCheatDialog();
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::addRamWatchClicked(void)
{
   int addr = ramView->getSelAddr();
   char desc[128];

   if ( addr < 0 )
   {
      return;
   }
   strcpy( desc, "Quick Watch Add");

   ramWatchList.add_entry( desc, addr, dpyType, dpySize, 0 );

   openRamWatchWindow(consoleWindow);
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::hexEditSelAddr(void)
{
   int addr = ramView->getSelAddr();

   if ( addr < 0 )
   {
      return;
   }
   hexEditorOpenFromDebugger( QHexEdit::MODE_NES_RAM, addr );
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::opLtClicked(void)
{
	cmpOp = '<';
	diffByEdit->setEnabled(false);
	moduloEdit->setEnabled(false);
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::opGtClicked(void)
{
	cmpOp = '>';
	diffByEdit->setEnabled(false);
	moduloEdit->setEnabled(false);
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::opLeClicked(void)
{
	cmpOp = 'l';
	diffByEdit->setEnabled(false);
	moduloEdit->setEnabled(false);
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::opGeClicked(void)
{
	cmpOp = 'm';
	diffByEdit->setEnabled(false);
	moduloEdit->setEnabled(false);
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::opEqClicked(void)
{
	cmpOp = '=';
	diffByEdit->setEnabled(false);
	moduloEdit->setEnabled(false);
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::opNeClicked(void)
{
	cmpOp = '!';
	diffByEdit->setEnabled(false);
	moduloEdit->setEnabled(false);
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::opDfClicked(void)
{
	cmpOp = 'd';
	diffByEdit->setEnabled(true);
	moduloEdit->setEnabled(false);
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::opMdClicked(void)
{
	cmpOp = '%';
	diffByEdit->setEnabled(false);
	moduloEdit->setEnabled(true);
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::pvBtnClicked(void)
{
   specValEdit->setEnabled(false);
   specAddrEdit->setEnabled(false);
   numChangeEdit->setEnabled(false);
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::svBtnClicked(void)
{
   specValEdit->setEnabled(true);
   specAddrEdit->setEnabled(false);
   numChangeEdit->setEnabled(false);
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::saBtnClicked(void)
{
   specValEdit->setEnabled(false);
   specAddrEdit->setEnabled(true);
   numChangeEdit->setEnabled(false);
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::ncBtnClicked(void)
{
   specValEdit->setEnabled(false);
   specAddrEdit->setEnabled(false);
   numChangeEdit->setEnabled(true);
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::ds1Clicked(void)
{
	dpySize = 'b';
	misalignedCbox->setEnabled(false);
	calcRamList();
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::ds2Clicked(void)
{
	dpySize = 'w';
	misalignedCbox->setEnabled(true);
	calcRamList();
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::ds4Clicked(void)
{
	dpySize = 'd';
	misalignedCbox->setEnabled(true);
	calcRamList();
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::signedTypeClicked(void)
{
	dpyType = 's';
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::unsignedTypeClicked(void)
{
	dpyType = 'u';
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::hexTypeClicked(void)
{
	dpyType = 'h';
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::calcRamList(void)
{
	int addr;
	int dataSize = 1;
	int endAddr = ShowROM ? 0x10000 : 0x8000;

	if ( chkMisAligned )
	{
		dataSize = 1;
	}
	else if ( dpySize == 'd' )
	{
		dataSize = 4;
	}
	else if ( dpySize == 'w' )
	{
		dataSize = 2;
	}
	else 
	{
		dataSize = 1;
	}

	actvSrchList.clear();

	for (addr=0; addr<endAddr; addr += dataSize)
	{
		switch ( dpySize )
		{
			case 'd':
				if ( (addr+3) < endAddr )
				{
					if ( (memLoc[addr].elimMask == 0) &&
					     (memLoc[addr+1].elimMask == 0)	&&
					     (memLoc[addr+2].elimMask == 0)	&&
					     (memLoc[addr+3].elimMask == 0)	)
					{
						actvSrchList.push_back( &memLoc[addr] );
					}
				}
			break;
			case 'w':
				if ( (addr+1) < endAddr )
				{
					if ( (memLoc[addr].elimMask == 0) &&
					     (memLoc[addr+1].elimMask == 0)	)
					{
						actvSrchList.push_back( &memLoc[addr] );
					}
				}
			break;
			default:
			case 'b':
				if ( memLoc[addr].elimMask == 0 )
				{
					actvSrchList.push_back( &memLoc[addr] );
				}
			break;
		}
	}
	vbar->setMaximum( actvSrchList.size() );
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::updateRamValues(void)
{
	std::list <struct memoryLocation_t*>::iterator it;
  	memoryLocation_t *loc = NULL;
	memoryState_t  val;

	for (it = actvSrchList.begin(); it != actvSrchList.end(); it++)
	{
		loc = *it;

		val.v8.u  = GetMem(loc->addr);
		val.v16.u = ReadValueAtHardwareAddress(loc->addr, 2);
		val.v32.u = ReadValueAtHardwareAddress(loc->addr, 4);

		if ( dpySize == 'd' )
		{
			if ( memLoc[loc->addr].val.v32.u != val.v32.u )
			{
				memLoc[loc->addr].val = val;
				memLoc[loc->addr].chgCount++;
			}
		}
		else if ( dpySize == 'w' )
		{
			if ( memLoc[loc->addr].val.v16.u != val.v16.u )
			{
				memLoc[loc->addr].val = val;
				memLoc[loc->addr].chgCount++;
			}
		}
		else 
		{
			if ( memLoc[loc->addr].val.v8.u != val.v8.u )
			{
				memLoc[loc->addr].val = val;
				memLoc[loc->addr].chgCount++;
			}
		}
	}
}
//----------------------------------------------------------------------------
QRamSearchView::QRamSearchView(QWidget *parent)
	: QWidget(parent)
{
	QPalette pal;
	QColor c, fg(0,0,0), bg(255,255,255);
	bool useDarkTheme = false;

	pal = this->palette();

	// Figure out if we are using a light or dark theme by checking the 
	// default window text grayscale color. If more white, then we will
	// use white text on black background, else we do the opposite.
	c = pal.color(QPalette::WindowText);

	if ( qGray( c.red(), c.green(), c.blue() ) > 128 )
	{
		useDarkTheme = true;
	}

	if ( useDarkTheme )
	{
		pal.setColor(QPalette::Base      , fg );
		pal.setColor(QPalette::Background, fg );
		pal.setColor(QPalette::WindowText, bg );
	}
	else 
	{
		pal.setColor(QPalette::Base      , bg );
		pal.setColor(QPalette::Background, bg );
		pal.setColor(QPalette::WindowText, fg );
	}
	this->setPalette(pal);

	font.setFamily("Courier New");
	font.setStyle( QFont::StyleNormal );
	font.setStyleHint( QFont::Monospace );

	calcFontData();

	lineOffset = 0;
	maxLineOffset = 0;
   selAddr = -1;
   selLine = -1;

	wheelPixelCounter = 0;

   setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Expanding );
   setFocusPolicy(Qt::StrongFocus);
}
//----------------------------------------------------------------------------
QRamSearchView::~QRamSearchView(void)
{

}
//----------------------------------------------------------------------------
void QRamSearchView::calcFontData(void)
{
	this->setFont(font);
    QFontMetrics metrics(font);
#if QT_VERSION > QT_VERSION_CHECK(5, 11, 0)
    pxCharWidth = metrics.horizontalAdvance(QLatin1Char('2'));
#else
    pxCharWidth = metrics.width(QLatin1Char('2'));
#endif
    pxCharHeight   = metrics.height();
	 pxLineSpacing  = metrics.lineSpacing() * 1.25;
    pxLineLead     = pxLineSpacing - pxCharHeight;
    pxCursorHeight = pxCharHeight;
	 pxColWidth[0]  = pxCharWidth * 10;
	 pxColWidth[1]  = pxCharWidth * 15;
	 pxColWidth[2]  = pxCharWidth * 15;
	 pxColWidth[3]  = pxCharWidth * 15;
	 pxLineWidth    = pxColWidth[0] + pxColWidth[1] + pxColWidth[2] + pxColWidth[3];

	 viewLines   = (viewHeight / pxLineSpacing) + 1;

	 setMinimumWidth( pxLineWidth );
}
//----------------------------------------------------------------------------
void QRamSearchView::setScrollBars( QScrollBar *hbar, QScrollBar *vbar )
{
	this->hbar = hbar; this->vbar = vbar;
}
//----------------------------------------------------
void QRamSearchView::resizeEvent(QResizeEvent *event)
{
	viewWidth  = event->size().width();
	viewHeight = event->size().height();

	//printf("QAsmView Resize: %ix%i\n", viewWidth, viewHeight );

	viewLines = (viewHeight / pxLineSpacing) + 1;

	//maxLineOffset = 0; // mb.numLines() - viewLines + 1;

	if ( viewWidth >= pxLineWidth )
	{
		pxLineXScroll = 0;
	}
	else
	{
		pxLineXScroll = (int)(0.010f * (float)hbar->value() * (float)(pxLineWidth - viewWidth) );
	}

}
//----------------------------------------------------------------------------
int QRamSearchView::convPixToLine( QPoint p )
{
   int lineNum = 0;
	float ly = ( (float)pxLineLead / (float)pxLineSpacing );
	float py = ( (float)p.y() -  (float)pxLineSpacing) /  (float)pxLineSpacing;
	float ry = fmod( py, 1.0 );

   if ( ry < ly )
	{
		lineNum = ( ((int)py) - 1 );
	}
	else
	{
		lineNum = ( ((int)py) );
	}
	//printf("Pos: %ix%i = %i\n", p.x(), p.y(), lineNum );

	return lineNum;
}
//----------------------------------------------------------------------------
void QRamSearchView::keyPressEvent(QKeyEvent *event)
{
	//printf("Ram Search View Key Press: 0x%x \n", event->key() );

   if (event->matches(QKeySequence::MoveToPreviousLine))
   {
      selAddr = -1;
      if ( selLine > 0 )
      {
         selLine--;
      }
      if ( selLine < lineOffset )
      {
         lineOffset = selLine;
      }
      if ( lineOffset < 0 )
      {
         lineOffset = 0;
      }
      vbar->setValue( lineOffset );
   }
  	else if (event->matches(QKeySequence::MoveToNextLine))
   {
      selAddr = -1;
      selLine++;

      if ( selLine >= actvSrchList.size() )
      {
         selLine = actvSrchList.size() - 1;
      }

      if ( selLine >= (lineOffset+viewLines) )
      {
         lineOffset = selLine - viewLines + 1;
      }

      if ( lineOffset >= maxLineOffset )
      {
         lineOffset = maxLineOffset;
      }
      vbar->setValue( lineOffset );
   }
   else if (event->matches(QKeySequence::MoveToNextPage))
   {
      lineOffset += ( (3 * viewLines) / 4);

      if ( lineOffset >= maxLineOffset )
      {
         lineOffset = maxLineOffset;
      }
      vbar->setValue( lineOffset );
   }
   else if (event->matches(QKeySequence::MoveToPreviousPage))
   {
      lineOffset -= ( (3 * viewLines) / 4);

      if ( lineOffset < 0 )
      {
         lineOffset = 0;
      }
      vbar->setValue( lineOffset );
   }
}
//----------------------------------------------------------------------------
void QRamSearchView::mousePressEvent(QMouseEvent * event)
{
	int lineNum = convPixToLine( event->pos() );

	//printf("c: %ix%i \n", c.x(), c.y() );

	if ( event->button() == Qt::LeftButton )
	{
      selLine = lineOffset + lineNum;
	}
}
//----------------------------------------------------------------------------
void QRamSearchView::wheelEvent(QWheelEvent *event)
{

	QPoint numPixels  = event->pixelDelta();
	QPoint numDegrees = event->angleDelta();

	if (!numPixels.isNull()) 
	{
		wheelPixelCounter -= numPixels.y();
	   //printf("numPixels: (%i,%i) \n", numPixels.x(), numPixels.y() );
	} 
	else if (!numDegrees.isNull()) 
	{
		//QPoint numSteps = numDegrees / 15;
		//printf("numSteps: (%i,%i) \n", numSteps.x(), numSteps.y() );
		//printf("numDegrees: (%i,%i)  %i\n", numDegrees.x(), numDegrees.y(), pxLineSpacing );
		wheelPixelCounter -= (pxLineSpacing * numDegrees.y()) / (15*8);
	}
	//printf("Wheel Event: %i\n", wheelPixelCounter);

	if ( wheelPixelCounter >= pxLineSpacing )
	{
		lineOffset += (wheelPixelCounter / pxLineSpacing);

		if ( lineOffset > maxLineOffset )
		{
			lineOffset = maxLineOffset;
		}
		vbar->setValue( lineOffset );

		wheelPixelCounter = wheelPixelCounter % pxLineSpacing;
	}
	else if ( wheelPixelCounter <= -pxLineSpacing )
	{
		lineOffset += (wheelPixelCounter / pxLineSpacing);

		if ( lineOffset < 0 )
		{
			lineOffset = 0;
		}
		vbar->setValue( lineOffset );

		wheelPixelCounter = wheelPixelCounter % pxLineSpacing;
	}

	 event->accept();
}
//----------------------------------------------------------------------------
void QRamSearchView::paintEvent(QPaintEvent *event)
{
   int i,x,y,row,nrow;
	std::list <struct memoryLocation_t*>::iterator it;
	char addrStr[32], valStr[32], prevStr[32], chgStr[32];
	QPainter painter(this);
  	memoryLocation_t *loc = NULL;
	int fieldWidth, fieldPad[4], fieldLen[4], fieldStart[4];
	const char *fieldText[4];

	painter.setFont(font);
	viewWidth  = event->rect().width();
	viewHeight = event->rect().height();
	fieldWidth = viewWidth / 4;
	
	for (i=0; i<4; i++)
	{
		fieldStart[i] = fieldWidth * i;
	}

	nrow = (viewHeight / pxLineSpacing)-1;

	if (nrow < 1 ) nrow = 1;

   viewLines = nrow;

	maxLineOffset = actvSrchList.size() - nrow;

	if ( maxLineOffset < 1 ) maxLineOffset = 1;

	lineOffset = vbar->value();

	vbar->setMaximum( maxLineOffset );

	if ( lineOffset > maxLineOffset )
	{
		lineOffset = maxLineOffset;
		vbar->setValue( lineOffset );
	}
	if ( lineOffset < 0 )
	{
		lineOffset = 0;
		vbar->setValue( 0 );
	}

	i=0;
	it = actvSrchList.begin();
	while ( it != actvSrchList.end() )
	{
		if ( i == lineOffset )
		{
			break;
		}
		i++; it++;
	}

   painter.fillRect( 0, 0, viewWidth, viewHeight, this->palette().color(QPalette::Background) );

   painter.setPen( this->palette().color(QPalette::WindowText));

	pxLineXScroll = (int)(0.010f * (float)hbar->value() * (float)(pxLineWidth - viewWidth) );

	x = -pxLineXScroll;
	y =  pxLineSpacing;

	strcpy( addrStr, "Address");
	strcpy( valStr , "Value");
	strcpy( prevStr, "Previous");
	strcpy( chgStr , "Changes");

	fieldText[0] = addrStr;
	fieldText[1] = valStr;
	fieldText[2] = prevStr;
	fieldText[3] = chgStr;

	for (i=0; i<4; i++)
	{
		fieldLen[i] = strlen(fieldText[i]) * pxCharWidth;

		fieldPad[i] = (fieldWidth - fieldLen[i]) / 2;

		painter.drawText( x+fieldStart[i]+fieldPad[i], y, tr(fieldText[i]) );
	}

	y += pxLineSpacing;

	for (row=0; row<nrow; row++)
	{
		if ( it != actvSrchList.end() )
		{
			loc = *it;
		}
		else
		{
			loc = NULL;
		}

		if ( loc == NULL )
		{
			continue;
		}
      if ( selLine >= 0 )
      {
         if ( selLine == (lineOffset+row) )
         {
            selAddr = loc->addr;
         }
      }
		it++;

      if ( selAddr == loc->addr )
      {
         painter.fillRect( 0, y - pxLineSpacing + pxLineLead, viewWidth, pxLineSpacing, QColor("light blue") );
      }

		sprintf (addrStr, "$%04X", loc->addr);

		if ( dpySize == 'd' )
		{
			if ( dpyType == 'h' )
			{
				sprintf( valStr , "0x%08X", loc->val.v32.u );
				sprintf( prevStr, "0x%08X", loc->hist.back().v32.u );
			}
			else if ( dpyType == 'u' )
			{
				sprintf( valStr , "%u", loc->val.v32.u );
				sprintf( prevStr, "%u", loc->hist.back().v32.u );
			}
			else 
			{
				sprintf( valStr , "%i", loc->val.v32.i );
				sprintf( prevStr, "%i", loc->hist.back().v32.i );
			}
		}
		else if ( dpySize == 'w' )
		{
			if ( dpyType == 'h' )
			{
				sprintf( valStr , "0x%04X", loc->val.v16.u );
				sprintf( prevStr, "0x%04X", loc->hist.back().v16.u );
			}
			else if ( dpyType == 'u' )
			{
				sprintf( valStr , "%u", loc->val.v16.u );
				sprintf( prevStr, "%u", loc->hist.back().v16.u );
			}
			else 
			{
				sprintf( valStr , "%i", loc->val.v16.i );
				sprintf( prevStr, "%i", loc->hist.back().v16.i );
			}
		}
		else 
		{
			if ( dpyType == 'h' )
			{
				sprintf( valStr , "0x%02X", loc->val.v8.u );
				sprintf( prevStr, "0x%02X", loc->hist.back().v8.u );
			}
			else if ( dpyType == 'u' )
			{
				sprintf( valStr , "%u", loc->val.v8.u );
				sprintf( prevStr, "%u", loc->hist.back().v8.u );
			}
			else 
			{
				sprintf( valStr , "%i", loc->val.v8.i );
				sprintf( prevStr, "%i", loc->hist.back().v8.i );
			}
		}
		sprintf( chgStr, "%u", loc->chgCount );


		for (i=0; i<4; i++)
		{
			fieldLen[i] = strlen(fieldText[i]) * pxCharWidth;

			fieldPad[i] = (fieldWidth - fieldLen[i]) / 2;

			painter.drawText( x+fieldStart[i]+fieldPad[i], y, tr(fieldText[i]) );
		}

		y += pxLineSpacing;
	}

	painter.drawLine( 0, pxLineSpacing+pxLineLead, viewWidth, pxLineSpacing+pxLineLead );
	painter.drawLine( x+fieldStart[1], 0, x+fieldStart[1], viewHeight );
	painter.drawLine( x+fieldStart[2], 0, x+fieldStart[2], viewHeight );
	painter.drawLine( x+fieldStart[3], 0, x+fieldStart[3], viewHeight );

}
//----------------------------------------------------------------------------
