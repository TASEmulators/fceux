// TraceLogger.cpp
//
#include <QDir>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QPainter>

#include "../../types.h"
#include "../../fceu.h"
#include "../../cart.h"
#include "../../x6502.h"
#include "../../debug.h"
#include "../../asm.h"
#include "../../ppu.h"
#include "../../ines.h"
#include "../../nsf.h"
#include "../../movie.h"

#include "Qt/ConsoleUtilities.h"
#include "Qt/TraceLogger.h"
#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/fceuWrapper.h"

#define LOG_REGISTERS           0x00000001
#define LOG_PROCESSOR_STATUS    0x00000002
#define LOG_NEW_INSTRUCTIONS    0x00000004
#define LOG_NEW_DATA            0x00000008
#define LOG_TO_THE_LEFT         0x00000010
#define LOG_FRAMES_COUNT        0x00000020
#define LOG_MESSAGES            0x00000040
#define LOG_BREAKPOINTS         0x00000080
#define LOG_SYMBOLIC            0x00000100
#define LOG_CODE_TABBING        0x00000200
#define LOG_CYCLES_COUNT        0x00000400
#define LOG_INSTRUCTIONS_COUNT  0x00000800
#define LOG_BANK_NUMBER         0x00001000

#define LOG_LINE_MAX_LEN 160
// Frames count - 1+6+1 symbols
// Cycles count - 1+11+1 symbols
// Instructions count - 1+11+1 symbols
// AXYS state - 20
// Processor status - 11
// Tabs - 31
// Address - 6
// Data - 10
// Disassembly - 45
// EOL (/0) - 1
// ------------------------
// 148 symbols total
#define LOG_AXYSTATE_MAX_LEN 21
#define LOG_PROCSTATUS_MAX_LEN 12
#define LOG_TABS_MASK 31
#define LOG_ADDRESS_MAX_LEN 13
#define LOG_DATA_MAX_LEN 11
#define LOG_DISASSEMBLY_MAX_LEN 46
#define NL_MAX_MULTILINE_COMMENT_LEN 1000

static int logging = 0;
static int logging_options = LOG_REGISTERS | LOG_PROCESSOR_STATUS | LOG_TO_THE_LEFT | LOG_MESSAGES | LOG_BREAKPOINTS | LOG_CODE_TABBING;
static char str_axystate[LOG_AXYSTATE_MAX_LEN] = {0}, str_procstatus[LOG_PROCSTATUS_MAX_LEN] = {0};
static char str_tabs[LOG_TABS_MASK+1] = {0}, str_address[LOG_ADDRESS_MAX_LEN] = {0}, str_data[LOG_DATA_MAX_LEN] = {0}, str_disassembly[LOG_DISASSEMBLY_MAX_LEN] = {0};
static char str_result[LOG_LINE_MAX_LEN] = {0};
static char str_temp[LOG_LINE_MAX_LEN] = {0};
//static char str_decoration[NL_MAX_MULTILINE_COMMENT_LEN + 10] = {0};
//static char str_decoration_comment[NL_MAX_MULTILINE_COMMENT_LEN + 10] = {0};
//static char* tracer_decoration_comment = 0;
//static char* tracer_decoration_comment_end_pos = 0;
static int oldcodecount = 0, olddatacount = 0;

//----------------------------------------------------
TraceLoggerDialog_t::TraceLoggerDialog_t(QWidget *parent)
	: QDialog( parent )
{
	QVBoxLayout *mainLayout;
	QHBoxLayout *hbox;
	QGridLayout *grid;
	QGroupBox *frame;
	QLabel    *lbl;

   setWindowTitle( tr("Trace Logger") );

	mainLayout   = new QVBoxLayout();
	grid         = new QGridLayout();
	mainLayout->addLayout( grid, 100 );

	traceView = new QTraceLogView(this);
	vbar      = new QScrollBar( Qt::Vertical, this );
	hbar      = new QScrollBar( Qt::Horizontal, this );

	traceView->setScrollBars( hbar, vbar );
	hbar->setMinimum(0);
	hbar->setMaximum(100);
	vbar->setMinimum(0);
	vbar->setMaximum(10000);

	grid->addWidget( traceView, 0, 0);
	grid->addWidget( vbar     , 0, 1 );
	grid->addWidget( hbar     , 1, 0 );

	grid         = new QGridLayout();
	mainLayout->addLayout( grid, 1 );

	lbl = new QLabel( tr("Lines") );
	logLastCbox  = new QCheckBox( tr("Log Last") );
	logMaxLinesComboBox = new QComboBox();

	logFileCbox  = new QCheckBox( tr("Log to File") );
	selLogFileButton = new QPushButton( tr("Browse...") );
	startStopButton = new QPushButton( tr("Start Logging") );
	autoUpdateCbox = new QCheckBox( tr("Automatically update this window while logging") );

	if ( logging )
	{
		startStopButton->setText( tr("Stop Logging") );
	}
	connect( startStopButton, SIGNAL(clicked(void)), this, SLOT(toggleLoggingOnOff(void)) );

	hbox = new QHBoxLayout();
	hbox->addWidget( logLastCbox );
	hbox->addWidget( logMaxLinesComboBox );
	hbox->addWidget( lbl );

	grid->addLayout( hbox, 0, 0, Qt::AlignLeft );
	grid->addWidget( startStopButton, 0, 1, Qt::AlignCenter );

	hbox = new QHBoxLayout();
	hbox->addWidget( logFileCbox );
	hbox->addWidget( selLogFileButton );

	grid->addLayout( hbox, 1, 0, Qt::AlignLeft );
	grid->addWidget( autoUpdateCbox, 1, 1, Qt::AlignCenter );

	grid  = new QGridLayout();
	frame = new QGroupBox(tr("Log Options"));
	frame->setLayout( grid );

	logRegCbox            = new QCheckBox( tr("Log State of Registers") );
	logFrameCbox          = new QCheckBox( tr("Log Frames Count") );
	logEmuMsgCbox         = new QCheckBox( tr("Log Emulator Messages") );
	symTraceEnaCbox       = new QCheckBox( tr("Symbolic Trace") );
	logProcStatFlagCbox   = new QCheckBox( tr("Log Processor Status Flags") );
	logCyclesCountCbox    = new QCheckBox( tr("Log Cycles Count") );
	logBreakpointCbox     = new QCheckBox( tr("Log Breakpoint Hits") );
	useStackPointerCbox   = new QCheckBox( tr("Use Stack Pointer for Code Tabbing (Nesting Visualization)") );
	toLeftDisassemblyCbox = new QCheckBox( tr("To the Left from Disassembly") );
	logInstrCountCbox     = new QCheckBox( tr("Log Instructions Count") );
	logBankNumCbox        = new QCheckBox( tr("Log Bank Number") );

	grid->addWidget( logRegCbox     , 0, 0, Qt::AlignLeft );
	grid->addWidget( logFrameCbox   , 1, 0, Qt::AlignLeft );
	grid->addWidget( logEmuMsgCbox  , 2, 0, Qt::AlignLeft );
	grid->addWidget( symTraceEnaCbox, 3, 0, Qt::AlignLeft );
	grid->addWidget( logProcStatFlagCbox, 0, 1, Qt::AlignLeft );
	grid->addWidget( logCyclesCountCbox , 1, 1, Qt::AlignLeft );
	grid->addWidget( logBreakpointCbox  , 2, 1, Qt::AlignLeft );
	grid->addWidget( useStackPointerCbox, 3, 1, 1, 2, Qt::AlignLeft );
	grid->addWidget( toLeftDisassemblyCbox, 0, 2, Qt::AlignLeft );
	grid->addWidget( logInstrCountCbox    , 1, 2, Qt::AlignLeft );
	grid->addWidget( logBankNumCbox       , 2, 2, Qt::AlignLeft );

	mainLayout->addWidget( frame, 1 );

	grid  = new QGridLayout();
	frame = new QGroupBox(tr("Extra Log Options that work with the Code/Data Logger"));
	frame->setLayout( grid );

	logNewMapCodeCbox = new QCheckBox( tr("Only Log Newly Mapped Code") );
	logNewMapDataCbox = new QCheckBox( tr("Only Log that Accesses Newly Mapped Data") );

	grid->addWidget( logNewMapCodeCbox, 0, 0, Qt::AlignLeft );
	grid->addWidget( logNewMapDataCbox, 0, 1, Qt::AlignLeft );

	mainLayout->addWidget( frame, 1 );

	setLayout( mainLayout );

}
//----------------------------------------------------
TraceLoggerDialog_t::~TraceLoggerDialog_t(void)
{
	printf("Trace Logger Window Deleted\n");
}
//----------------------------------------------------
void TraceLoggerDialog_t::closeEvent(QCloseEvent *event)
{
   printf("Trace Logger Close Window Event\n");
   done(0);
	deleteLater();
   event->accept();
}
//----------------------------------------------------
void TraceLoggerDialog_t::closeWindow(void)
{
   printf("Trace Logger Close Window\n");
   done(0);
	deleteLater();
}
//----------------------------------------------------
void TraceLoggerDialog_t::toggleLoggingOnOff(void)
{
	logging = !logging;

	if ( logging )
	{
		startStopButton->setText( tr("Stop Logging") );
	}
	else
	{
		startStopButton->setText( tr("Start Logging") );
	}
}
//----------------------------------------------------
traceRecord_t::traceRecord_t(void)
{
	cpu.PC = 0;
	cpu.A = 0;
	cpu.X = 0;
	cpu.Y = 0;
	cpu.S = 0;
	cpu.P = 0;

	opCode[0] = 0;
	opCode[1] = 0;
	opCode[2] = 0;
	opSize = 0;
	asmTxtSize = 0;
	asmTxt[0] = 0;

	cycleCount = 0;
	instrCount = 0;
	flags = 0;

	callAddr = -1;
	romAddr = -1;
	bank = -1;
	skippedLines = 0;
}
//----------------------------------------------------
int traceRecord_t::appendAsmText( const char *txt )
{
	int i=0;

	while ( txt[i] != 0 )
	{
		asmTxt[ asmTxtSize ] = txt[i]; i++; asmTxtSize++;
	}
	asmTxt[ asmTxtSize ] = 0;

	return 0;
}
//----------------------------------------------------
//todo: really speed this up
void FCEUD_TraceInstruction(uint8 *opcode, int size)
{
	if (!logging)
		return;

	traceRecord_t  rec;

	unsigned int addr = X.PC;
	uint8 tmp;
	static int unloggedlines = 0;

	rec.cpu.PC = X.PC;
	rec.cpu.A  = X.A;
	rec.cpu.X  = X.X;
	rec.cpu.Y  = X.Y;
	rec.cpu.S  = X.S;
	rec.cpu.P  = X.P;

	for (int i=0; i<size; i++)
	{
		rec.opCode[i] = opcode[i];
	}
	rec.opSize  = size;
	rec.romAddr = GetPRGAddress(addr);
	rec.bank    = getBank(addr);

	rec.frameCount = currFrameCounter;
	rec.instrCount = total_instructions;

	int64 counter_value = timestampbase + (uint64)timestamp - total_cycles_base;
	if (counter_value < 0)	// sanity check
	{
		ResetDebugStatisticsCounters();
		counter_value = 0;
	}
	rec.cycleCount = counter_value;

	// if instruction executed from the RAM, skip this, log all instead
	// TODO: loops folding mame-lyke style
	if (rec.romAddr != -1)
	{
		if (((logging_options & LOG_NEW_INSTRUCTIONS) && (oldcodecount != codecount)) ||
		    ((logging_options & LOG_NEW_DATA) && (olddatacount != datacount)))
		{
			//something new was logged
			oldcodecount = codecount;
			olddatacount = datacount;
			if (unloggedlines > 0)
			{
				//sprintf(str_result, "(%d lines skipped)", unloggedlines);
				//OutputLogLine(str_result);
				rec.skippedLines = unloggedlines;
				unloggedlines = 0;
			}
		} 
		else
		{
			if ((logging_options & LOG_NEW_INSTRUCTIONS) ||
				 (logging_options & LOG_NEW_DATA))
			{
				if (FCEUI_GetLoggingCD())
				{
					unloggedlines++;
				}
				return;
			}
		}
	}

	if ((addr + size) > 0xFFFF)
	{
		//sprintf(str_data, "%02X        ", opcode[0]);
		//sprintf(str_disassembly, "OVERFLOW");
		rec.flags |= 0x01;
	} 
	else
	{
		char* a = 0;
		switch (size)
		{
			case 0:
				//sprintf(str_data, "%02X        ", opcode[0]);
				//sprintf(str_disassembly,"UNDEFINED");
				rec.flags |= 0x02;
				break;
			case 1:
			{
				//sprintf(str_data, "%02X        ", opcode[0]);
				a = Disassemble(addr + 1, opcode);
				// special case: an RTS opcode
				if (opcode[0] == 0x60)
				{
					// add the beginning address of the subroutine that we exit from
					unsigned int caller_addr = GetMem(((X.S) + 1)|0x0100) + (GetMem(((X.S) + 2)|0x0100) << 8) - 0x2;
					if (GetMem(caller_addr) == 0x20)
					{
						// this was a JSR instruction - take the subroutine address from it
						unsigned int call_addr = GetMem(caller_addr + 1) + (GetMem(caller_addr + 2) << 8);
						//sprintf(str_decoration, " (from $%04X)", call_addr);
						//strcat(a, str_decoration);
						rec.callAddr = call_addr;
					}
				}
				break;
			}
			case 2:
				//sprintf(str_data, "%02X %02X     ", opcode[0],opcode[1]);
				a = Disassemble(addr + 2, opcode);
				break;
			case 3:
				//sprintf(str_data, "%02X %02X %02X  ", opcode[0],opcode[1],opcode[2]);
				a = Disassemble(addr + 3, opcode);
				break;
		}

		if (a)
		{
			//if (logging_options & LOG_SYMBOLIC)
			//{
			//	loadNameFiles();
			//	tempAddressesLog.resize(0);
			//	// Insert Name and Comment lines if needed
			//	Name* node = findNode(getNamesPointerForAddress(addr), addr);
			//	if (node)
			//	{
			//		if (node->name)
			//		{
			//			strcpy(str_decoration, node->name);
			//			strcat(str_decoration, ":");
			//			tempAddressesLog.push_back(addr);
			//			//OutputLogLine(str_decoration, &tempAddressesLog);
			//		}
			//		if (node->comment)
			//		{
			//			// make a copy
			//			strcpy(str_decoration_comment, node->comment);
			//			strcat(str_decoration_comment, "\r\n");
			//			tracer_decoration_comment = str_decoration_comment;
			//			// divide the str_decoration_comment into strings (Comment1, Comment2, ...)
			//			char* tracer_decoration_comment_end_pos = strstr(tracer_decoration_comment, "\r\n");
			//			while (tracer_decoration_comment_end_pos)
			//			{
			//				tracer_decoration_comment_end_pos[0] = 0;		// set \0 instead of \r
			//				strcpy(str_decoration, "; ");
			//				strcat(str_decoration, tracer_decoration_comment);
			//				//OutputLogLine(str_decoration, &tempAddressesLog);
			//				tracer_decoration_comment_end_pos += 2;
			//				tracer_decoration_comment = tracer_decoration_comment_end_pos;
			//				tracer_decoration_comment_end_pos = strstr(tracer_decoration_comment_end_pos, "\r\n");
			//			}
			//		}
			//	}
			//	
			//	//replaceNames(ramBankNames, a, &tempAddressesLog);
			//	//for(int i=0;i<ARRAY_SIZE(pageNames);i++)
			//	//{
			//	//	replaceNames(pageNames[i], a, &tempAddressesLog);
			//	//}
			//}
			//strncpy(str_disassembly, a, LOG_DISASSEMBLY_MAX_LEN);
			//str_disassembly[LOG_DISASSEMBLY_MAX_LEN - 1] = 0;

			rec.appendAsmText(a);
		}
	}
	return; // TEST
	// All of the following log text creation is very cpu intensive, to keep emulation 
	// running realtime save data and have a separate thread do this translation.

	if (size == 1 && GetMem(addr) == 0x60)
	{
		// special case: an RTS opcode
		// add "----------" to emphasize the end of subroutine
		static const char* emphasize = " -------------------------------------------------------------------------------------------------------------------------";
		strncat(str_disassembly, emphasize, LOG_DISASSEMBLY_MAX_LEN - strlen(str_disassembly) - 1);
	}
	// stretch the disassembly string out if we have to output other stuff.
	if ((logging_options & (LOG_REGISTERS|LOG_PROCESSOR_STATUS)) && !(logging_options & LOG_TO_THE_LEFT))
	{
		for (int i = strlen(str_disassembly); i < (LOG_DISASSEMBLY_MAX_LEN - 1); ++i)
			str_disassembly[i] = ' ';
		str_disassembly[LOG_DISASSEMBLY_MAX_LEN - 1] = 0;
	}

	// Start filling the str_temp line: Frame count, Cycles count, Instructions count, AXYS state, Processor status, Tabs, Address, Data, Disassembly
	if (logging_options & LOG_FRAMES_COUNT)
	{
		sprintf(str_result, "f%-6u ", currFrameCounter);
	} else
	{
		str_result[0] = 0;
	}
	if (logging_options & LOG_CYCLES_COUNT)
	{
		int64 counter_value = timestampbase + (uint64)timestamp - total_cycles_base;
		if (counter_value < 0)	// sanity check
		{
			ResetDebugStatisticsCounters();
			counter_value = 0;
		}
		sprintf(str_temp, "c%-11llu ", counter_value);
		strcat(str_result, str_temp);
	}
	if (logging_options & LOG_INSTRUCTIONS_COUNT)
	{
		sprintf(str_temp, "i%-11llu ", total_instructions);
		strcat(str_result, str_temp);
	}
	
	if (logging_options & LOG_REGISTERS)
	{
		sprintf(str_axystate,"A:%02X X:%02X Y:%02X S:%02X ",(X.A),(X.X),(X.Y),(X.S));
	}
	
	if (logging_options & LOG_PROCESSOR_STATUS)
	{
		tmp = X.P^0xFF;
		sprintf(str_procstatus,"P:%c%c%c%c%c%c%c%c ",
			'N'|(tmp&0x80)>>2,
			'V'|(tmp&0x40)>>1,
			'U'|(tmp&0x20),
			'B'|(tmp&0x10)<<1,
			'D'|(tmp&0x08)<<2,
			'I'|(tmp&0x04)<<3,
			'Z'|(tmp&0x02)<<4,
			'C'|(tmp&0x01)<<5
			);
	}

	if (logging_options & LOG_TO_THE_LEFT)
	{
		if (logging_options & LOG_REGISTERS)
		{
			strcat(str_result, str_axystate);
		}
		if (logging_options & LOG_PROCESSOR_STATUS)
		{
			strcat(str_result, str_procstatus);
		}
	}

	if (logging_options & LOG_CODE_TABBING)
	{
		// add spaces at the beginning of the line according to stack pointer
		int spaces = (0xFF - X.S) & LOG_TABS_MASK;
		for (int i = 0; i < spaces; i++)
		{
			str_tabs[i] = ' ';
		}
		str_tabs[spaces] = 0;
		strcat(str_result, str_tabs);
	} 
	else if (logging_options & LOG_TO_THE_LEFT)
	{
		strcat(str_result, " ");
	}

	if (logging_options & LOG_BANK_NUMBER)
	{
		if (addr >= 0x8000)
		{
			sprintf(str_address, "$%02X:%04X: ", getBank(addr), addr);
		}
		else
		{
			sprintf(str_address, "  $%04X: ", addr);
		}
	} 
	else
	{
		sprintf(str_address, "$%04X: ", addr);
	}

	strcat(str_result, str_address);
	strcat(str_result, str_data);
	strcat(str_result, str_disassembly);

	if (!(logging_options & LOG_TO_THE_LEFT))
	{
		if (logging_options & LOG_REGISTERS)
		{
			strcat(str_result, str_axystate);
		}
		if (logging_options & LOG_PROCESSOR_STATUS)
		{
			strcat(str_result, str_procstatus);
		}
	}

	//OutputLogLine(str_result, &tempAddressesLog);
	
	return;
}
//----------------------------------------------------
QTraceLogView::QTraceLogView(QWidget *parent)
	: QWidget(parent)
{
	font.setFamily("Courier New");
	font.setStyle( QFont::StyleNormal );
	font.setStyleHint( QFont::Monospace );

	calcFontData();

	vbar = NULL;
	hbar = NULL;
}
//----------------------------------------------------
QTraceLogView::~QTraceLogView(void)
{

}
//----------------------------------------------------
void QTraceLogView::calcFontData(void)
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
	 pxLineWidth    = pxCharWidth * LOG_LINE_MAX_LEN;

	 viewLines   = (viewHeight / pxLineSpacing) + 1;
}
//----------------------------------------------------------------------------
void QTraceLogView::setScrollBars( QScrollBar *h, QScrollBar *v )
{
	hbar = h; vbar = v;
}
//----------------------------------------------------
void QTraceLogView::resizeEvent(QResizeEvent *event)
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
//----------------------------------------------------
void QTraceLogView::paintEvent(QPaintEvent *event)
{
	QPainter painter(this);

	painter.setFont(font);
	viewWidth  = event->rect().width();
	viewHeight = event->rect().height();

}
//----------------------------------------------------
