// HotKeyConf.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <SDL.h>
#include <QHeaderView>

#include "../../types.h"
#include "../../fceu.h"
#include "../../cheat.h"
#include "../../debug.h"
#include "../../driver.h"
#include "../../version.h"
#include "../../movie.h"
#include "../../palette.h"
#include "../../fds.h"
#include "../../cart.h"
#include "../../ines.h"
#include "../common/configSys.h"

#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/fceuWrapper.h"
#include "Qt/HexEditor.h"

//----------------------------------------------------------------------------
static int getRAM( unsigned int i )
{
	return GetMem(i);
}
static int getPPU( unsigned int i )
{
	i &= 0x3FFF;
	if (i < 0x2000)return VPage[(i) >> 10][(i)];
	//NSF PPU Viewer crash here (UGETAB) (Also disabled by 'MaxSize = 0x2000')
	if (GameInfo->type == GIT_NSF)
		return 0;
	else
	{
		if (i < 0x3F00)
			return vnapage[(i >> 10) & 0x3][i & 0x3FF];
		return READPAL_MOTHEROFALL(i & 0x1F);
	}
	return 0;
}
static int getOAM( unsigned int i )
{
	return SPRAM[i & 0xFF];
}
static int getROM( unsigned int offset)
{
	if (offset < 16)
	{
		return *((unsigned char *)&head+offset);
	}
	else if (offset < (16+PRGsize[0]) )
	{
		return PRGptr[0][offset-16];
	}
	else if (offset < (16+PRGsize[0]+CHRsize[0]) )
	{
		return CHRptr[0][offset-16-PRGsize[0]];
	}
	return -1;
}
static int conv2xchar( int i )
{
   int c = 0;

	if ( (i >= 0) && (i < 10) )
	{
      c = i + '0';
	}
	else if ( i < 16 )
	{
		c = (i - 10) + 'A';
	}
	return c;
}
//----------------------------------------------------------------------------
HexEditorDialog_t::HexEditorDialog_t(QWidget *parent)
	: QDialog( parent )
{
	QVBoxLayout *mainLayout;

	font.setFamily("Courier New");
	font.setStyle( QFont::StyleNormal );
	font.setStyleHint( QFont::Monospace );

	setWindowTitle("Hex Editor");

	resize( 512, 512 );

	mainLayout = new QVBoxLayout();

	txtBuf = new QPlainTextEdit();

	txtBuf->setFont( font );
	txtBuf->moveCursor(QTextCursor::Start);
	txtBuf->setReadOnly(true);
	txtBuf->setLineWrapMode( QPlainTextEdit::NoWrap );

	mainLayout->addWidget( txtBuf );

	setLayout( mainLayout );

	mode = MODE_NES_RAM;
	numLines = 0;
	numCharsPerLine = 90;
	mbuf = NULL;
	memSize = 0;
	mbuf_size = 0;
	memAccessFunc = getRAM;
	redraw = false;
	total_instructions_lp = 0;

	showMemViewResults(true);

	periodicTimer  = new QTimer( this );

   connect( periodicTimer, &QTimer::timeout, this, &HexEditorDialog_t::updatePeriodic );

	periodicTimer->start( 1000 ); // 10hz
}
//----------------------------------------------------------------------------
HexEditorDialog_t::~HexEditorDialog_t(void)
{
	periodicTimer->stop();

	if ( mbuf != NULL )
	{
		::free( mbuf ); mbuf = NULL;
	}
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
int HexEditorDialog_t::calcVisibleRange( int *start_out, int *end_out, int *center_out )
{
	int start_pos, end_pos;
	QPoint bottom_left( 0, txtBuf->viewport()->height() - 1);
  
	start_pos = txtBuf->cursorForPosition(QPoint(0, 0)).position();
	end_pos   = txtBuf->cursorForPosition(bottom_left).position();

	start_pos = (start_pos / numCharsPerLine) - 1;
	end_pos   = (end_pos   / numCharsPerLine) + 1;

	if ( start_pos < 0 )
	{
		start_pos = 0;
	}

	printf("Start: %i   End: %i\n", start_pos, end_pos );

	if ( start_out )
	{
		*start_out = start_pos;
	}
	if ( end_out )
	{
		*end_out = end_pos;
	}
	if ( center_out )
	{
		*center_out = (start_pos + end_pos) / 2;
	}
	return 0;
}
//----------------------------------------------------------------------------
void HexEditorDialog_t::setMode(int new_mode)
{
	if ( mode != new_mode )
	{
		mode = new_mode;
		showMemViewResults(true);
	}
}
//----------------------------------------------------------------------------
void HexEditorDialog_t::initMem(void)
{

	for (int i=0; i<mbuf_size; i++)
	{
		mbuf[i].data  = memAccessFunc(i);
		mbuf[i].color = 0;
		mbuf[i].actv  = 0;
		mbuf[i].draw  = 1;
	}
}
//----------------------------------------------------------------------------
void HexEditorDialog_t::showMemViewResults (bool reset)
{
	int i, row, row_start, row_end;
	int addr = 0, lineAddr = 0, c, un, ln;
	char addrStr[128], valStr[16][8], ascii[18];
	char valChg[16], activityColoringOn, colorEnable;

	switch ( mode )
	{
		default:
		case MODE_NES_RAM:
			memAccessFunc = getRAM;
			memSize       = 0x10000;
		break;
		case MODE_NES_PPU:
			memAccessFunc = getPPU;
			memSize       = (GameInfo->type == GIT_NSF ? 0x2000 : 0x4000);
		break;
		case MODE_NES_OAM:
			memAccessFunc = getOAM;
			memSize       = 0x100;
		break;
		case MODE_NES_ROM:

			if ( GameInfo != NULL )
			{
				memAccessFunc = getROM;
				memSize       = 16 + CHRsize[0] + PRGsize[0];
			}
			else
			{  // No Game Loaded!!! Get out of Function
				memAccessFunc = NULL;
				memSize = 0;
				if ( mbuf )
				{
  		    	   free(mbuf); mbuf = NULL;
				}
				mbuf_size = 0;
				redraw = true;
				txtBuf->setPlainText("No ROM Loaded");
				return;
			}
		break;
	}
	numLines = memSize / 16;

	if ( (mbuf == NULL) || (mbuf_size != memSize) )
	{
		printf("Mode: %i  MemSize:%i   0x%08x\n", mode, memSize, (unsigned int)memSize );
		reset = 1;

		if ( mbuf )
		{
         free(mbuf); mbuf = NULL;
		}
		mbuf = (struct memByte_t *)malloc( memSize * sizeof(struct memByte_t) );

		if ( mbuf )
		{
         mbuf_size = memSize;
			initMem();
		}
		else
		{
			printf("Error: Failed to allocate memview buffer size\n");
			mbuf_size = 0;
			return;
		}
	}

	if ( reset )
	{
		txtBuf->clear();
		row_start = 0;
		row_end   = numLines;
	}
	else
	{
		calcVisibleRange( &row_start, &row_end, NULL );

		if ( row_start < 0 )
		{
			row_start = 0;
		}
		if ( row_end > numLines )
		{
			row_end = numLines;
		}
	}

	for (row=row_start; row<row_end; row++)
	{
		lineAddr = (row*16);

		if ( !reset )
		{
			txtBuf->textCursor().setPosition( row * numCharsPerLine );
		}

		if ( reset )
		{
			//next_iter = iter;
			//gtk_text_iter_forward_chars( &next_iter, 9 );

		   sprintf( addrStr, "%08X ", lineAddr );

			txtBuf->insertPlainText( addrStr );
		}
		else
		{
			txtBuf->textCursor().movePosition( QTextCursor::NextCharacter, QTextCursor::MoveAnchor, 9 );
		}

		for (i=0; i<16; i++)
		{
			valChg[i] = reset;

			addr = lineAddr+i;

			c = memAccessFunc(addr);

			if ( mbuf[addr].draw )
			{
				mbuf[addr].draw = 0;
				valChg[i] = 1;
			}

			un = ( c & 0x00f0 ) >> 4;
			ln = ( c & 0x000f );

			valStr[i][0] = ' ';
			valStr[i][1] = conv2xchar(un);
			valStr[i][2] = conv2xchar(ln);
			valStr[i][3] = ' ';
			valStr[i][4] = 0;

			if ( isprint(c) )
			{
            ascii[i] = c;
			}
			else
			{
            ascii[i] = '.';
			}

			if ( valChg[i] )
			{
				if ( !reset )
				{
					for (int j=0; j<4; j++)
					{
						txtBuf->textCursor().deleteChar();
					}
				}
				txtBuf->insertPlainText( valStr[i] );
			}
			else
			{
				txtBuf->textCursor().movePosition( QTextCursor::NextCharacter, QTextCursor::MoveAnchor, 4 );
			}
		}
		ascii[16] = 0;

		if ( reset )
		{
			txtBuf->insertPlainText( ascii );
		}

		if ( reset )
		{
			txtBuf->insertPlainText("\n");
		}

	}
}
//----------------------------------------------------------------------------
void HexEditorDialog_t::updatePeriodic(void)
{
	printf("Update Periodic\n");

	checkMemActivity();

	showMemViewResults(false);
}
//----------------------------------------------------------------------------
int HexEditorDialog_t::checkMemActivity(void)
{
	int c;

	// Don't perform memory activity checks when:
	// 1. In ROM View Mode
	// 2. The simulation is not cycling (paused)

	if ( ( mode == MODE_NES_ROM ) ||
	      ( total_instructions_lp == total_instructions ) )
	{
		return -1;
	}

	for (int i=0; i<mbuf_size; i++)
	{
		c = memAccessFunc(i);

		if ( c != mbuf[i].data )
		{
			mbuf[i].actv  = 15;
			mbuf[i].data  = c;
			mbuf[i].draw  = 1;
		}
		else
		{
			if ( mbuf[i].actv > 0 )
			{
				mbuf[i].draw = 1;
				mbuf[i].actv--;
			}
		}
	}
	total_instructions_lp = total_instructions;

   return 0;
}
//----------------------------------------------------------------------------
