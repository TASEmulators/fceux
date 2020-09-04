// ConsoleDebugger.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <list>

#include <SDL.h>
#include <QPainter>
#include <QHeaderView>
#include <QCloseEvent>
#include <QGridLayout>

#include "../../types.h"
#include "../../fceu.h"
#include "../../cheat.h"
#include "../../debug.h"
#include "../../driver.h"
#include "../../version.h"
#include "../../video.h"
#include "../../movie.h"
#include "../../palette.h"
#include "../../fds.h"
#include "../../cart.h"
#include "../../ines.h"
#include "../../asm.h"
#include "../../ppu.h"
#include "../../x6502.h"
#include "common/configSys.h"

#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/config.h"
#include "Qt/fceuWrapper.h"
#include "Qt/ConsoleDebugger.h"

// Where are these defined?
extern int vblankScanLines;
extern int vblankPixel;

static std::list <ConsoleDebugger*> dbgWinList;
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

	grid    = new QGridLayout();
	asmView = new QAsmView(this);
	vbar    = new QScrollBar( Qt::Vertical, this );
	hbar    = new QScrollBar( Qt::Horizontal, this );

   asmView->setScrollBars( hbar, vbar );

	grid->addWidget( asmView, 0, 0 );
	grid->addWidget( vbar   , 0, 1 );
	grid->addWidget( hbar   , 1, 0 );

	vbox1   = new QVBoxLayout();
	vbox2   = new QVBoxLayout();
	hbox1   = new QHBoxLayout();

	hbar->setMinimum(0);
	hbar->setMaximum(100);
	vbar->setMinimum(0);
	vbar->setMaximum( 0x10000 );

	//asmText->setFont(font);
	//asmText->setReadOnly(true);
	//asmText->setOverwriteMode(true);
	//asmText->setMinimumWidth( 20 * fontCharWidth );
	//asmText->setLineWrapMode( QPlainTextEdit::NoWrap );

	mainLayout->addLayout( grid, 10 );
	mainLayout->addLayout( vbox1, 1 );

	grid    = new QGridLayout();

	vbox1->addLayout( hbox1 );
	hbox1->addLayout( vbox2 );
	vbox2->addLayout( grid  );

	button = new QPushButton( tr("Run") );
	grid->addWidget( button, 0, 0, Qt::AlignLeft );
   connect( button, SIGNAL(clicked(void)), this, SLOT(debugRunCB(void)) );

	button = new QPushButton( tr("Step Into") );
	grid->addWidget( button, 0, 1, Qt::AlignLeft );
   connect( button, SIGNAL(clicked(void)), this, SLOT(debugStepIntoCB(void)) );

	button = new QPushButton( tr("Step Out") );
	grid->addWidget( button, 1, 0, Qt::AlignLeft );
   connect( button, SIGNAL(clicked(void)), this, SLOT(debugStepOutCB(void)) );

	button = new QPushButton( tr("Step Over") );
	grid->addWidget( button, 1, 1, Qt::AlignLeft );
   connect( button, SIGNAL(clicked(void)), this, SLOT(debugStepOverCB(void)) );

	button = new QPushButton( tr("Run Line") );
	grid->addWidget( button, 2, 0, Qt::AlignLeft );
   connect( button, SIGNAL(clicked(void)), this, SLOT(debugRunLineCB(void)) );

	button = new QPushButton( tr("128 Lines") );
	grid->addWidget( button, 2, 1, Qt::AlignLeft );
   connect( button, SIGNAL(clicked(void)), this, SLOT(debugRunLine128CB(void)) );

	button = new QPushButton( tr("Seek To:") );
	grid->addWidget( button, 3, 0, Qt::AlignLeft );
   //connect( button, SIGNAL(clicked(void)), this, SLOT(seekPCCB(void)) );

	seekEntry = new QLineEdit();
	seekEntry->setFont( font );
	seekEntry->setMaxLength( 4 );
	seekEntry->setInputMask( ">HHHH;" );
	seekEntry->setAlignment(Qt::AlignCenter);
	seekEntry->setMaximumWidth( 6 * fontCharWidth );
	grid->addWidget( seekEntry, 3, 1, Qt::AlignLeft );

	hbox = new QHBoxLayout();
	lbl  = new QLabel( tr("PC:") );
	pcEntry = new QLineEdit();
	pcEntry->setFont( font );
	pcEntry->setMaxLength( 4 );
	pcEntry->setInputMask( ">HHHH;" );
	pcEntry->setAlignment(Qt::AlignCenter);
	pcEntry->setMaximumWidth( 6 * fontCharWidth );
	hbox->addWidget( lbl );
	hbox->addWidget( pcEntry, 1, Qt::AlignLeft );
	grid->addLayout( hbox, 4, 0, Qt::AlignLeft );

	button = new QPushButton( tr("Seek PC") );
	grid->addWidget( button, 4, 1, Qt::AlignLeft );
   connect( button, SIGNAL(clicked(void)), this, SLOT(seekPCCB(void)) );

	hbox = new QHBoxLayout();
	lbl  = new QLabel( tr("A:") );
	regAEntry = new QLineEdit();
	regAEntry->setFont( font );
	regAEntry->setMaxLength( 2 );
	regAEntry->setInputMask( ">HH;" );
	regAEntry->setAlignment(Qt::AlignCenter);
	regAEntry->setMaximumWidth( 4 * fontCharWidth );
	hbox->addWidget( lbl );
	hbox->addWidget( regAEntry, 1, Qt::AlignLeft );
	lbl  = new QLabel( tr("X:") );
	regXEntry = new QLineEdit();
	regXEntry->setFont( font );
	regXEntry->setMaxLength( 2 );
	regXEntry->setInputMask( ">HH;" );
	regXEntry->setAlignment(Qt::AlignCenter);
	regXEntry->setMaximumWidth( 4 * fontCharWidth );
	hbox->addWidget( lbl );
	hbox->addWidget( regXEntry, 1, Qt::AlignLeft );
	lbl  = new QLabel( tr("Y:") );
	regYEntry = new QLineEdit();
	regYEntry->setFont( font );
	regYEntry->setMaxLength( 2 );
	regYEntry->setInputMask( ">HH;" );
	regYEntry->setAlignment(Qt::AlignCenter);
	regYEntry->setMaximumWidth( 4 * fontCharWidth );
	hbox->addWidget( lbl );
	hbox->addWidget( regYEntry, 1, Qt::AlignLeft );
	vbox2->addLayout( hbox );

	stackFrame = new QGroupBox(tr("Stack $0100"));
	stackText  = new QPlainTextEdit(this);
	hbox       = new QHBoxLayout();
	hbox->addWidget( stackText );
	vbox2->addWidget( stackFrame );
	stackFrame->setLayout( hbox );
	stackText->setFont(font);
	stackText->setReadOnly(true);
	//stackText->setMaximumWidth( 16 * fontCharWidth );

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

	vbox          = new QVBoxLayout();
	cpuCyclesLbl1 = new QLabel( tr("CPU Cycles:") );
	cpuCyclesLbl2 = new QLabel( tr("(+0):") );
	cpuInstrsLbl1 = new QLabel( tr("Instructions:") );
	cpuInstrsLbl2 = new QLabel( tr("(+0):") );
	brkCpuCycExd  = new QCheckBox( tr("Break when Exceed") );
	brkInstrsExd  = new QCheckBox( tr("Break when Exceed") );
	cpuCycExdVal  = new QLineEdit( tr("0") );
	instrExdVal   = new QLineEdit( tr("0") );
	hbox          = new QHBoxLayout();
	vbox->addLayout( hbox );
	hbox->addWidget( cpuCyclesLbl1 );
	hbox->addWidget( cpuCyclesLbl2 );
	hbox         = new QHBoxLayout();
	vbox->addLayout( hbox );
	hbox->addWidget( brkCpuCycExd );
	hbox->addWidget( cpuCycExdVal, 1, Qt::AlignLeft );

	hbox         = new QHBoxLayout();
	vbox->addLayout( hbox );
	hbox->addWidget( cpuInstrsLbl1 );
	hbox->addWidget( cpuInstrsLbl2 );
	hbox         = new QHBoxLayout();
	vbox->addLayout( hbox );
	hbox->addWidget( brkInstrsExd );
	hbox->addWidget( instrExdVal, 1, Qt::AlignLeft );
	hbox2->addLayout( vbox );

	cpuCycExdVal->setFont( font );
	cpuCycExdVal->setMaxLength( 16 );
	cpuCycExdVal->setInputMask( ">9000000000000000;" );
	cpuCycExdVal->setAlignment(Qt::AlignLeft);
	cpuCycExdVal->setMaximumWidth( 18 * fontCharWidth );

	instrExdVal->setFont( font );
	instrExdVal->setMaxLength( 16 );
	instrExdVal->setInputMask( ">9000000000000000;" );
	instrExdVal->setAlignment(Qt::AlignLeft);
	instrExdVal->setMaximumWidth( 18 * fontCharWidth );

	setLayout( mainLayout );

	windowUpdateReq   = true;

	dbgWinList.push_back( this );

	periodicTimer  = new QTimer( this );

   connect( periodicTimer, &QTimer::timeout, this, &ConsoleDebugger::updatePeriodic );
	//connect( hbar, SIGNAL(valueChanged(int)), this, SLOT(hbarChanged(int)) );
   connect( vbar, SIGNAL(valueChanged(int)), this, SLOT(vbarChanged(int)) );

	periodicTimer->start( 100 ); // 10hz
}
//----------------------------------------------------------------------------
ConsoleDebugger::~ConsoleDebugger(void)
{
	printf("Destroy Debugger Window\n");
	periodicTimer->stop();
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
void ConsoleDebugger::debugRunCB(void)
{
	if (FCEUI_EmulationPaused()) 
	{
		setRegsFromEntry();
		FCEUI_ToggleEmulationPause();
		//DebuggerWasUpdated = false done in above function;
	}
}
//----------------------------------------------------------------------------
void ConsoleDebugger::debugStepIntoCB(void)
{
	if (FCEUI_EmulationPaused())
	{
		setRegsFromEntry();
	}
	FCEUI_Debugger().step = true;
	FCEUI_SetEmulationPaused(0);
}
//----------------------------------------------------------------------------
void ConsoleDebugger::debugStepOutCB(void)
{
	if (FCEUI_EmulationPaused() > 0) 
	{
		DebuggerState &dbgstate = FCEUI_Debugger();
		setRegsFromEntry();
		if (dbgstate.stepout)
		{
			printf("Step Out is currently in process.\n");
			return;
		}
		if (GetMem(X.PC) == 0x20)
		{
			dbgstate.jsrcount = 1;
		}
		else 
		{
			dbgstate.jsrcount = 0;
		}
		dbgstate.stepout = 1;
		FCEUI_SetEmulationPaused(0);
	}
}
//----------------------------------------------------------------------------
void ConsoleDebugger::debugStepOverCB(void)
{
	if (FCEUI_EmulationPaused()) 
	{
		setRegsFromEntry();
		int tmp=X.PC;
		uint8 opcode = GetMem(X.PC);
		bool jsr = opcode==0x20;
		bool call = jsr;
		#ifdef BRK_3BYTE_HACK
		//with this hack, treat BRK similar to JSR
		if(opcode == 0x00)
		{
			call = true;
		}
		#endif
		if (call) 
		{
			if (watchpoint[64].flags)
			{
				printf("Step Over is currently in process.\n");
				return;
			}
			watchpoint[64].address = (tmp+3);
			watchpoint[64].flags = WP_E|WP_X;
		}
		else 
		{
			FCEUI_Debugger().step = true;
		}
		FCEUI_SetEmulationPaused(0);
	}
}
//----------------------------------------------------------------------------
void ConsoleDebugger::debugRunLineCB(void)
{
	if (FCEUI_EmulationPaused())
	{
		setRegsFromEntry();
	}
	uint64 ts=timestampbase;
	ts+=timestamp;
	ts+=341/3;
	//if (scanline == 240) vblankScanLines++;
	//else vblankScanLines = 0;
	FCEUI_Debugger().runline = true;
	FCEUI_Debugger().runline_end_time=ts;
	FCEUI_SetEmulationPaused(0);
}
//----------------------------------------------------------------------------
void ConsoleDebugger::debugRunLine128CB(void)
{
	if (FCEUI_EmulationPaused())
	{
		setRegsFromEntry();
	}
	FCEUI_Debugger().runline = true;
	{
		uint64 ts=timestampbase;
		ts+=timestamp;
		ts+=128*341/3;
		FCEUI_Debugger().runline_end_time=ts;
		//if (scanline+128 >= 240 && scanline+128 <= 257) vblankScanLines = (scanline+128)-240;
		//else vblankScanLines = 0;
	}
	FCEUI_SetEmulationPaused(0);
}
//----------------------------------------------------------------------------
void ConsoleDebugger::seekPCCB (void)
{
	if (FCEUI_EmulationPaused())
	{
		setRegsFromEntry();
		//updateAllDebugWindows();
	}
	windowUpdateReq = true;
	//asmView->scrollToPC();
}
//----------------------------------------------------------------------------
int  QAsmView::getAsmLineFromAddr(int addr)
{
	int  line = -1;
	int  incr, nextLine;
	int  run = 1;

	if ( asmEntry.size() <= 0 )
	{
      return -1;
	}
	incr = asmEntry.size() / 2;

	if ( addr < asmEntry[0]->addr )
	{
		return 0;
	}
	else if ( addr > asmEntry[ asmEntry.size() - 1 ]->addr )
	{
		return asmEntry.size() - 1;
	}

	if ( incr < 1 ) incr = 1;

	nextLine = line = incr;

	// algorithm to efficiently find line from address. Starts in middle of list and 
	// keeps dividing the list in 2 until it arrives at an answer.
	while ( run )
	{
		//printf("incr:%i   line:%i  addr:%04X   delta:%i\n", incr, line, asmEntry[line]->addr, addr - asmEntry[line]->addr);

		if ( incr == 1 )
		{
			if ( asmEntry[line]->addr < addr )
			{
				nextLine = line + 1;
				if ( asmEntry[line]->addr > nextLine )
				{
					break;
				}
				line = nextLine;
			}
			else if ( asmEntry[line]->addr > addr )
			{
				nextLine = line - 1;
				if ( asmEntry[line]->addr < nextLine )
				{
					break;
				}
				line = nextLine;
			}
			else 
			{
				run = 0; break;
			}
		} 
		else
		{
			incr = incr / 2; 
			if ( incr < 1 ) incr = 1;

			if ( asmEntry[line]->addr < addr )
			{
				nextLine = line + incr;
			}
			else if ( asmEntry[line]->addr > addr )
			{
				nextLine = line - incr;
			}
			else
			{
				run = 0; break;
			}
			line = nextLine;
		}
	}

	//for (size_t i=0; i<asmEntry.size(); i++)
	//{
	//	if ( asmEntry[i]->addr >= addr )
	//	{
   //      line = i; break;
	//	}
	//}

	return line;
}
//----------------------------------------------------------------------------
// This function is for "smart" scrolling...
// it attempts to scroll up one line by a whole instruction
static int InstructionUp(int from)
{
	int i = std::min(16, from), j;

	while (i > 0)
	{
		j = i;
		while (j > 0)
		{
			if (GetMem(from - j) == 0x00)
				break;	// BRK usually signifies data
			if (opsize[GetMem(from - j)] == 0)
				break;	// invalid instruction!
			if (opsize[GetMem(from - j)] > j)
				break;	// instruction is too long!
			if (opsize[GetMem(from - j)] == j)
				return (from - j);	// instruction is just right! :D
			j -= opsize[GetMem(from - j)];
		}
		i--;
	}

	// if we get here, no suitable instruction was found
	if ((from >= 2) && (GetMem(from - 2) == 0x00))
		return (from - 2);	// if a BRK instruction is possible, use that
	if (from)
		return (from - 1);	// else, scroll up one byte
	return 0;	// of course, if we can't scroll up, just return 0!
}
//static int InstructionDown(int from)
//{
//	int tmp = opsize[GetMem(from)];
//	if ((tmp))
//		return from + tmp;
//	else
//		return from + 1;		// this is data or undefined instruction
//}
//----------------------------------------------------------------------------
void  QAsmView::updateAssemblyView(void)
{
	int starting_address, start_address_lp, addr, size;
	int instruction_addr;
	std::string line;
	char chr[64];
	uint8 opcode[3];
	const char *disassemblyText = NULL;
	dbg_asm_entry_t *a;
	//GtkTextIter iter, next_iter;
	char pc_found = 0;

	start_address_lp = starting_address = X.PC;

	for (int i=0; i < 0xFFFF; i++)
	{
		//printf("%i: Start Address: 0x%04X \n", i, start_address_lp );

		starting_address = InstructionUp( start_address_lp );

		if ( starting_address == start_address_lp )
		{
			break;
		}
		if ( starting_address < 0 )
		{
			starting_address = start_address_lp;
			break;
		}	
		start_address_lp = starting_address;
	}

	asmClear();

	addr  = starting_address;
	asmPC = NULL;

	//asmText->clear();

	//gtk_text_buffer_get_start_iter( textbuf, &iter );

	//textview_lines_allocated = gtk_text_buffer_get_line_count( textbuf ) - 1;

	//printf("Num Lines: %i\n", textview_lines_allocated );

	for (int i=0; i < 0xFFFF; i++)
	{
		line.clear();

		// PC pointer
		if (addr > 0xFFFF) break;

		a = new dbg_asm_entry_t;

		instruction_addr = addr;

		if ( !pc_found )
		{
			if (addr > X.PC)
			{
				asmPC = a;
				line.assign(">");
				pc_found = 1;
			}
			else if (addr == X.PC)
			{
				asmPC = a;
				line.assign(">");
				pc_found = 1;
			} 
			else
			{
				line.assign(" ");
			}
		}
		else 
		{
			line.assign(" ");
		}
		a->addr = addr;

		if (addr >= 0x8000)
		{
			a->bank = getBank(addr);
			a->rom  = GetNesFileAddress(addr);

			if (displayROMoffsets && (a->rom != -1) )
			{
				sprintf(chr, " %06X: ", a->rom);
			} 
			else
			{
				sprintf(chr, "%02X:%04X: ", a->bank, addr);
			}
		} 
		else
		{
			sprintf(chr, "  :%04X: ", addr);
		}
		line.append(chr);

		size = opsize[GetMem(addr)];
		if (size == 0)
		{
			sprintf(chr, "%02X        UNDEFINED", GetMem(addr++));
			line.append(chr);
		} else
		{
			if ((addr + size) > 0xFFFF)
			{
				while (addr < 0xFFFF)
				{
					sprintf(chr, "%02X        OVERFLOW\n", GetMem(addr++));
					line.append(chr);
				}
				break;
			}
			for (int j = 0; j < size; j++)
			{
				sprintf(chr, "%02X ", opcode[j] = GetMem(addr++));
				line.append(chr);
			}
			while (size < 3)
			{
				line.append("   ");  //pad output to align ASM
				size++;
			}

			disassemblyText = Disassemble(addr, opcode);

			if ( disassemblyText )
			{
				line.append( disassemblyText );
			}
		}
		for (int j=0; j<size; j++)
		{
			a->opcode[j] = opcode[j];
		}
		a->size = size;

		// special case: an RTS opcode
		if (GetMem(instruction_addr) == 0x60)
		{
			line.append("-------------------------");
		}

		a->text.assign( line );

		a->line = asmEntry.size();

		line.append("\n");

		//asmText->insertPlainText( tr(line.c_str()) );

		asmEntry.push_back(a);
	}

	vbar->setMaximum( asmEntry.size() );
}
//----------------------------------------------------------------------------
void ConsoleDebugger::setRegsFromEntry(void)
{
	std::string s;
	long int i;

	s = pcEntry->text().toStdString();
	if ( s.size() > 0 )
	{
		i = strtol( s.c_str(), NULL, 16 );
	}
	else
	{
		i = 0;
	}
	X.PC = i;
	//printf("Set PC: '%s'  %04X\n", s.c_str(), X.PC );

	s = regAEntry->text().toStdString();
	if ( s.size() > 0 )
	{
		i = strtol( s.c_str(), NULL, 16 );
	}
	else
	{
		i = 0;
	}
	X.A  = i;
	//printf("Set A: '%s'  %02X\n", s.c_str(), X.A );

	s = regXEntry->text().toStdString();
	if ( s.size() > 0 )
	{
		i = strtol( s.c_str(), NULL, 16 );
	}
	else
	{
		i = 0;
	}
	X.X  = i;
	//printf("Set X: '%s'  %02X\n", s.c_str(), X.X );

	s = regYEntry->text().toStdString();
	if ( s.size() > 0 )
	{
		i = strtol( s.c_str(), NULL, 16 );
	}
	else
	{
		i = 0;
	}
	X.Y  = i;
	//printf("Set Y: '%s'  %02X\n", s.c_str(), X.Y );
}
//----------------------------------------------------------------------------
void  ConsoleDebugger::updateRegisterView(void)
{
	int stackPtr;
	char stmp[64];
	char str[32], str2[32];
	std::string  stackLine;

	sprintf( stmp, "%04X", X.PC );

	pcEntry->setText( tr(stmp) );

	sprintf( stmp, "%02X", X.A );

	regAEntry->setText( tr(stmp) );

	sprintf( stmp, "%02X", X.X );

	regXEntry->setText( tr(stmp) );

	sprintf( stmp, "%02X", X.Y );

	regYEntry->setText( tr(stmp) );

	N_chkbox->setChecked( (X.P & N_FLAG) ? true : false );
	V_chkbox->setChecked( (X.P & V_FLAG) ? true : false );
	U_chkbox->setChecked( (X.P & U_FLAG) ? true : false );
	B_chkbox->setChecked( (X.P & B_FLAG) ? true : false );
	D_chkbox->setChecked( (X.P & D_FLAG) ? true : false );
	I_chkbox->setChecked( (X.P & I_FLAG) ? true : false );
	Z_chkbox->setChecked( (X.P & Z_FLAG) ? true : false );
	C_chkbox->setChecked( (X.P & C_FLAG) ? true : false );

	stackPtr = X.S | 0x0100;

	sprintf( stmp, "Stack: $%04X", stackPtr );
	stackFrame->setTitle( tr(stmp) );

	stackPtr++;

	if ( stackPtr <= 0x01FF )
	{
		sprintf( stmp, "%02X", GetMem(stackPtr) );

		stackLine.assign( stmp );

		for (int i = 1; i < 128; i++)
		{
			//tmp = ((tmp+1)|0x0100)&0x01FF;  //increment and fix pointer to $0100-$01FF range
			stackPtr++;
			if (stackPtr > 0x1FF)
				break;
			if ((i & 7) == 0)
				sprintf( stmp, ",\r\n%02X", GetMem(stackPtr) );
			else
				sprintf( stmp, ",%02X", GetMem(stackPtr) );

			stackLine.append( stmp );
		}
	}

	stackText->setPlainText( tr(stackLine.c_str()) );

	// update counters
	int64 counter_value = timestampbase + (uint64)timestamp - total_cycles_base;
	if (counter_value < 0)	// sanity check
	{
		ResetDebugStatisticsCounters();
		counter_value = 0;
	}
	sprintf( stmp, "CPU Cycles: %llu", counter_value);

	cpuCyclesLbl1->setText( tr(stmp) );

	counter_value = timestampbase + (uint64)timestamp - delta_cycles_base;
	if (counter_value < 0)	// sanity check
	{
		ResetDebugStatisticsCounters();
		counter_value = 0;
	}
	sprintf(stmp, "(+%llu)", counter_value);

	cpuCyclesLbl2->setText( tr(stmp) );

	sprintf(stmp, "Instructions: %llu", total_instructions);
	cpuInstrsLbl1->setText( tr(stmp) );

	sprintf(stmp, "(+%llu)", delta_instructions);
	cpuInstrsLbl2->setText( tr(stmp) );

	// PPU Labels
	sprintf(stmp, "PPU: 0x%04X", (int)FCEUPPU_PeekAddress());
	ppuLbl->setText( tr(stmp) );

	sprintf(stmp, "Sprite: 0x%02X", PPU[3] );
	spriteLbl->setText( tr(stmp) );

	extern int linestartts;
	#define GETLASTPIXEL    (PAL?((timestamp*48-linestartts)/15) : ((timestamp*48-linestartts)/16) )
	
	int ppupixel = GETLASTPIXEL;

	if (ppupixel>341)	//maximum number of pixels per scanline
		ppupixel = 0;	//Currently pixel display is borked until Run 128 lines is clicked, this keeps garbage from displaying

	// If not in the 0-239 pixel range, make special cases for display
	if (scanline == 240 && vblankScanLines < (PAL?72:22))
	{
		if (!vblankScanLines)
		{
			// Idle scanline (240)
			sprintf(str, "%d", scanline);	// was "Idle %d"
		} else if (scanline + vblankScanLines == (PAL?311:261))
		{
			// Pre-render
			sprintf(str, "-1");	// was "Prerender -1"
		} else
		{
			// Vblank lines (241-260/310)
			sprintf(str, "%d", scanline + vblankScanLines);	// was "Vblank %d"
		}
		sprintf(str2, "%d", vblankPixel);
	} else
	{
		// Scanlines 0 - 239
		sprintf(str, "%d", scanline);
		sprintf(str2, "%d", ppupixel);
	}

	if(newppu)
	{
		sprintf(str ,"%d",newppu_get_scanline());
		sprintf(str2,"%d",newppu_get_dot());
	}

	sprintf( stmp, "Scanline: %s", str );
	scanLineLbl->setText( tr(stmp) );

	sprintf( stmp, "Pixel: %s", str2 );
	pixLbl->setText( tr(stmp) );

}
//----------------------------------------------------------------------------
void ConsoleDebugger::updateWindowData(void)
{
	asmView->updateAssemblyView();
	
	asmView->scrollToPC();

	updateRegisterView();

	windowUpdateReq = false;
}
//----------------------------------------------------------------------------
void ConsoleDebugger::updatePeriodic(void)
{
	//printf("Update Periodic\n");

	if ( windowUpdateReq )
	{
		fceuWrapperLock();
		updateWindowData();
		fceuWrapperUnLock();
	}
	asmView->update();
}
//----------------------------------------------------------------------------
void ConsoleDebugger::breakPointNotify( int addr )
{
	windowUpdateReq = true;
}
//----------------------------------------------------------------------------
void ConsoleDebugger::vbarChanged(int value)
{
	//printf("VBar Changed: %i\n", value);
	asmView->setLine( value );
}
//----------------------------------------------------------------------------
void FCEUD_DebugBreakpoint( int addr )
{
	std::list <ConsoleDebugger*>::iterator it;

	printf("Breakpoint Hit: 0x%04X \n", addr );

	fceuWrapperUnLock();
	
	for (it=dbgWinList.begin(); it!=dbgWinList.end(); it++)
	{
		(*it)->breakPointNotify( addr );
	}

	while (FCEUI_EmulationPaused() && !FCEUI_EmulationFrameStepped())
	{
		usleep(100000);
	}
	fceuWrapperLock();
}
//----------------------------------------------------------------------------
QAsmView::QAsmView(QWidget *parent)
	: QWidget( parent )
{
	QPalette pal;
	QColor fg("black"), bg("white");

	font.setFamily("Courier New");
	font.setStyle( QFont::StyleNormal );
	font.setStyleHint( QFont::Monospace );

	pal = this->palette();
	pal.setColor(QPalette::Base      , bg );
	pal.setColor(QPalette::Background, bg );
	pal.setColor(QPalette::WindowText, fg );

	this->setPalette(pal);

	calcFontData();

	vbar = NULL;
	hbar = NULL;
	asmPC = NULL;
	displayROMoffsets = false;
	lineOffset = 0;
	maxLineOffset = 0;
}
//----------------------------------------------------------------------------
QAsmView::~QAsmView(void)
{
	asmClear();
}
//----------------------------------------------------------------------------
void QAsmView::asmClear(void)
{
	for (size_t i=0; i<asmEntry.size(); i++)
	{
		delete asmEntry[i];
	}
	asmEntry.clear();
}
//----------------------------------------------------------------------------
void QAsmView::setLine(int lineNum)
{
	lineOffset = lineNum;
}
//----------------------------------------------------------------------------
void QAsmView::scrollToPC(void)
{
	if ( asmPC != NULL )
	{
		lineOffset = asmPC->line;
	}
}
//----------------------------------------------------------------------------
void QAsmView::calcFontData(void)
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

	 viewLines   = (viewHeight / pxLineSpacing) + 1;
}
//----------------------------------------------------------------------------
void QAsmView::setScrollBars( QScrollBar *h, QScrollBar *v )
{
	hbar = h; vbar = v;
}
//----------------------------------------------------------------------------
void QAsmView::resizeEvent(QResizeEvent *event)
{
	viewWidth  = event->size().width();
	viewHeight = event->size().height();

	//printf("QAsmView Resize: %ix%i\n", viewWidth, viewHeight );

	viewLines = (viewHeight / pxLineSpacing) + 1;

	maxLineOffset = 0; // mb.numLines() - viewLines + 1;

	//if ( viewWidth >= pxLineWidth )
	//{
	//	pxLineXScroll = 0;
	//}
	//else
	//{
	//	pxLineXScroll = (int)(0.010f * (float)hbar->value() * (float)(pxLineWidth - viewWidth) );
	//}

}
//----------------------------------------------------------------------------
void QAsmView::keyPressEvent(QKeyEvent *event)
{
	printf("Debug ASM Window Key Press: 0x%x \n", event->key() );

}
//----------------------------------------------------------------------------
void QAsmView::keyReleaseEvent(QKeyEvent *event)
{
   printf("Debug ASM Window Key Release: 0x%x \n", event->key() );
	//assignHotkey( event );
}
//----------------------------------------------------------------------------
void QAsmView::mousePressEvent(QMouseEvent * event)
{
	// TODO QPoint c = convPixToCursor( event->pos() );

}
//----------------------------------------------------------------------------
void QAsmView::contextMenuEvent(QContextMenuEvent *event)
{
	// TODO
}
//----------------------------------------------------------------------------
void QAsmView::paintEvent(QPaintEvent *event)
{
	int x,y,l, row, nrow;
	QPainter painter(this);

	painter.setFont(font);
	viewWidth  = event->rect().width();
	viewHeight = event->rect().height();

	nrow = (viewHeight / pxLineSpacing) + 1;

	if ( nrow < 1 ) nrow = 1;

	viewLines = nrow;

	if ( cursorPosY >= viewLines )
	{
		cursorPosY = viewLines-1;
	}
	maxLineOffset = asmEntry.size() - nrow + 1;

	if ( maxLineOffset < 0 )
	{
		maxLineOffset = 0;
	}

	if ( lineOffset < 0 )
	{
		lineOffset = 0;
	}
	if ( lineOffset > maxLineOffset )
	{
		lineOffset = maxLineOffset;
	}

	painter.fillRect( 0, 0, viewWidth, viewHeight, this->palette().color(QPalette::Background) );

	y = pxLineSpacing;

	for (row=0; row < nrow; row++)
	{
		x=0;
		l = lineOffset + row;
		painter.setPen( this->palette().color(QPalette::WindowText));

		if ( l < asmEntry.size() )
		{
			painter.drawText( x, y, tr(asmEntry[l]->text.c_str()) );
		}
		y += pxLineSpacing;
	}
}
//----------------------------------------------------------------------------
