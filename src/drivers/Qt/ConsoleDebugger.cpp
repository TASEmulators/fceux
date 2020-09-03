// ConsoleDebugger.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <list>

#include <SDL.h>
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

	asmText = new QPlainTextEdit(this);
	vbox1   = new QVBoxLayout();
	vbox2   = new QVBoxLayout();
	hbox1   = new QHBoxLayout();
	grid    = new QGridLayout();

	asmText->setFont(font);
	asmText->setReadOnly(true);
	asmText->setOverwriteMode(true);
	asmText->setMinimumWidth( 20 * fontCharWidth );
	asmText->setLineWrapMode( QPlainTextEdit::NoWrap );

	vbox1->addLayout( hbox1 );
	hbox1->addLayout( vbox2 );
	vbox2->addLayout( grid  );

	mainLayout->addWidget( asmText, 10 );
	mainLayout->addLayout( vbox1, 1 );

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

	displayROMoffsets = false;
	windowUpdateReq   = true;
	asmPC = NULL;

	dbgWinList.push_back( this );

	periodicTimer  = new QTimer( this );

   connect( periodicTimer, &QTimer::timeout, this, &ConsoleDebugger::updatePeriodic );

	periodicTimer->start( 100 ); // 10hz
}
//----------------------------------------------------------------------------
ConsoleDebugger::~ConsoleDebugger(void)
{
	printf("Destroy Debugger Window\n");
	periodicTimer->stop();
	asmClear();
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
void ConsoleDebugger::asmClear(void)
{
	for (size_t i=0; i<asmEntry.size(); i++)
	{
		delete asmEntry[i];
	}
	asmEntry.clear();
}
//----------------------------------------------------------------------------
int  ConsoleDebugger::getAsmLineFromAddr(int addr)
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
void  ConsoleDebugger::updateAssemblyView(void)
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

	asmText->clear();

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

		asmText->insertPlainText( tr(line.c_str()) );

		asmEntry.push_back(a);
	}

}
//----------------------------------------------------------------------------
void ConsoleDebugger::updateWindowData(void)
{
	updateAssemblyView();
	
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
}
//----------------------------------------------------------------------------
void ConsoleDebugger::breakPointNotify( int addr )
{
	windowUpdateReq = true;
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
