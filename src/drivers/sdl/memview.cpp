#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <list>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

#ifdef _GTK3
#include <gdk/gdkkeysyms-compat.h>
#endif

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

#include "sdl.h"
#include "gui.h"
#include "dface.h"
#include "input.h"
#include "config.h"
#include "memview.h"

extern Config *g_config;

#define HIGHLIGHT_ACTIVITY_NUM_COLORS 16

static unsigned int  highlightActivityColors[HIGHLIGHT_ACTIVITY_NUM_COLORS] = 
{ 
   0x000000, 0x004035, 0x185218, 0x5e5c34, 
   0x804c00, 0xba0300, 0xd10038, 0xb21272, 
   0xba00ab, 0x6f00b0, 0x3700c2, 0x000cba,
   0x002cc9, 0x0053bf, 0x0072cf, 0x3c8bc7
};
//*******************************************************************************************************
// Memory View (Hex Editor) Window
//*******************************************************************************************************
//
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

static void PalettePoke(uint32 addr, uint8 data)
{
	data = data & 0x3F;
	addr = addr & 0x1F;
	if ((addr & 3) == 0)
	{
		addr = (addr & 0xC) >> 2;
		if (addr == 0)
		{
			PALRAM[0x00] = PALRAM[0x04] = PALRAM[0x08] = PALRAM[0x0C] = data;
		}
		else
		{
			UPALRAM[addr-1] = UPALRAM[0x10|(addr-1)] = data;
		}
	}
	else
	{
		PALRAM[addr] = data;
	}
}

//
struct memViewWin_t
{
	GtkWidget *win;
	GtkWidget *ivbar;
	GtkWidget *selCellLabel;
	GtkWidget *addr_entry;
	GtkWidget *memSelRadioItem[4];
	GtkTextView *textview;
	GtkTextBuffer *textbuf;
	GtkTextTag *highlight[ HIGHLIGHT_ACTIVITY_NUM_COLORS ];
	int selAddr;
	int selRomAddr;
	int jumpAddr;
	int jumpDelay;
	int dialog_op;
	int mode;
	int evntSrcID;
	int numLines;
	int numCharsPerLine;
	unsigned char *mbuf;
	unsigned char *colorbuf;
	int mbuf_size;
	GtkCellRenderer *hexByte_renderer[16];
	bool redraw;
   int (*memAccessFunc)( unsigned int offset);

	enum {
		MODE_NES_RAM = 0,
		MODE_NES_PPU,
		MODE_NES_OAM,
		MODE_NES_ROM
	};

	memViewWin_t(void)
	{
		win = NULL;
		textview = NULL;
		textbuf = NULL;
		addr_entry = NULL;
		selCellLabel = NULL;
		ivbar = NULL;
		selAddr = 0;
		selRomAddr = -1;
		jumpAddr = -1;
		jumpDelay = 0;
		dialog_op = 0;
		mode = MODE_NES_RAM;
		mbuf = NULL;
		colorbuf = NULL;
		mbuf_size = 0;
		numLines = 0;
		evntSrcID = 0;
		numCharsPerLine = 90;
		redraw = 1;
		memAccessFunc = getRAM;

		for (int i=0; i<4; i++)
		{
			memSelRadioItem[i] = NULL;
		}
		for (int i=0; i<16; i++)
		{
			hexByte_renderer[i] = NULL;
		}
		for (int i=0; i<HIGHLIGHT_ACTIVITY_NUM_COLORS; i++)
      {
         highlight[i] = NULL;
      }
	}

	~memViewWin_t(void)
	{
		if ( mbuf != NULL )
		{
			free(mbuf); mbuf = NULL;
		}
		if ( colorbuf != NULL )
		{
			free(colorbuf); colorbuf = NULL;
		}
	}

	void setMode(int new_mode)
	{
		if ( mode != new_mode )
		{
			showMemViewResults(1);
		}
		mode = new_mode;
	}

	int writeMem( unsigned int addr, int value )
	{
		value = value & 0x000000ff;

		switch ( mode )
		{
			default:
			case MODE_NES_RAM:
			{
				if ( addr < 0x8000 )
				{
					writefunc wfunc;
               
					wfunc = GetWriteHandler (addr);
               
					if (wfunc)
					{
						wfunc ((uint32) addr,
						       (uint8) (value & 0x000000ff));
					}
				}
				else
				{
					fprintf( stdout, "Error: Writing into RAM addresses >= 0x8000 is unsafe. Operation Denied.\n");
				}
			}
			break;
			case MODE_NES_PPU:
			{
				addr &= 0x3FFF;
				if (addr < 0x2000)
				{
					VPage[addr >> 10][addr] = value; //todo: detect if this is vrom and turn it red if so
				}
				if ((addr >= 0x2000) && (addr < 0x3F00))
				{
					vnapage[(addr >> 10) & 0x3][addr & 0x3FF] = value; //todo: this causes 0x3000-0x3f00 to mirror 0x2000-0x2f00, is this correct?
				}
				if ((addr >= 0x3F00) && (addr < 0x3FFF))
				{
					PalettePoke(addr, value);
				}
			}
			break;
			case MODE_NES_OAM:
			{
				addr &= 0xFF;
				SPRAM[addr] = value;
			}
			break;
			case MODE_NES_ROM:
			{
				if (addr < 16)
				{
					fprintf( stdout, "You can't edit ROM header here, however you can use iNES Header Editor to edit the header if it's an iNES format file.");
				}
				else if ( (addr >= 16) && (addr < PRGsize[0]+16) )
				{
				  	*(uint8 *)(GetNesPRGPointer(addr-16)) = value;
				}
				else if ( (addr >= PRGsize[0]+16) && (addr < CHRsize[0]+PRGsize[0]+16) )
				{
					*(uint8 *)(GetNesCHRPointer(addr-16-PRGsize[0])) = value;
				}
			}
			break;
		}
      return 0;
	}

	int gotoLocation( int addr );
	void showMemViewResults (int reset);
	int  calcVisibleRange( int *start_out, int *end_out, int *center_out );
	int  getAddrFromCursor( int CursorTextOffset = -1 );

};

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

static void initMem( unsigned char *c, int size )
{
	for (int i=0; i<size; i++)
	{
		c[i] = (i%2) ? 0xA5 : 0x5A;
	}
}

int memViewWin_t::getAddrFromCursor( int CursorTextOffset )
{
	int line, offs, byte0, byte, bcol, addr = -1;

	if ( CursorTextOffset < 0 )
	{
		gint cpos;
		g_object_get( textbuf, "cursor-position", &cpos, NULL );
		CursorTextOffset = cpos;
	}
	line = CursorTextOffset / numCharsPerLine;
	offs = CursorTextOffset % numCharsPerLine;

	if ( offs < 10 )
	{
		return addr;
	}
	else if ( offs < 73 )
	{
		byte0 = (offs - 10);

		byte = byte0 / 4;
		bcol = byte0 % 4;

		if ( byte < 16 )
		{
			if (bcol < 2)
			{
				addr = (line*16) + byte;
			}
		}
	}
	else
	{
		if ( (offs >= 73) && (offs < 89) )
		{
			addr = (line*16) + (offs-73);
		}
	}

	return addr;
}

void memViewWin_t::showMemViewResults (int reset)
{
	int addr, memSize = 0;
	int lineAddr = 0, c, un, ln;
	int i, row, row_start, row_end, totalChars;
	gint cpos;
	char addrStr[128], valStr[16][8], ascii[18];
	char addrChg, valChg[16];
	GtkTextIter iter, next_iter, start_iter, end_iter;

	if ( redraw )
	{
		reset = 1;
		redraw = 0;
	}

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
			memAccessFunc = getROM;
			memSize       = 16 + CHRsize[0] + PRGsize[0];
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
      mbuf = (unsigned char *)malloc( memSize );

		if ( colorbuf )
		{
         free(colorbuf); colorbuf = NULL;
		}
      colorbuf = (unsigned char *)malloc( memSize );

		if ( colorbuf != NULL )
		{
			memset( colorbuf, 0, memSize );
		}
		if ( mbuf )
		{
         mbuf_size = memSize;
			initMem( mbuf, memSize );
		}
		else
		{
			printf("Error: Failed to allocate memview buffer size\n");
			mbuf_size = 0;
			return;
		}
	}

	g_object_get( textbuf, "cursor-position", &cpos, NULL );

	selAddr = getAddrFromCursor( cpos );

	//printf("CPOS: %i \n", cpos );

	gtk_text_buffer_get_start_iter( textbuf, &start_iter );
	gtk_text_buffer_get_end_iter( textbuf, &end_iter   );

	if ( reset )
	{
		gtk_text_buffer_delete( textbuf, &start_iter, &end_iter );

		row_start = 0;
		row_end   = numLines;

	   gtk_text_buffer_get_iter_at_offset( textbuf, &iter, 0 );
	}
	else
	{
		calcVisibleRange( &row_start, &row_end, NULL );

	   gtk_text_buffer_get_iter_at_line( textbuf, &iter, row_start );
	}

	//gtk_text_buffer_get_iter_at_offset( textbuf, &iter, 0 );

	totalChars = row_start * numCharsPerLine;

	for (row=row_start; row<row_end; row++)
	{
		//gtk_text_buffer_get_iter_at_offset( textbuf, &iter, totalChars );

      addrChg = reset;

		lineAddr = (row*16);

		if ( addrChg )
		{
			next_iter = iter;
			gtk_text_iter_forward_chars( &next_iter, 9 );

		   sprintf( addrStr, "%08X ", lineAddr );

			if ( !reset )
			{
				gtk_text_buffer_delete( textbuf, &iter, &next_iter );
			}
			gtk_text_buffer_insert( textbuf, &iter, addrStr, -1 );
		}
		else
		{
			gtk_text_iter_forward_chars( &iter, 9 );
		}

		for (i=0; i<16; i++)
		{
			valChg[i] = reset;

			addr = lineAddr+i;

			c = memAccessFunc(addr);

			if ( c != mbuf[addr] )
			{
				valChg[i] = 1;
				mbuf[addr] = c;
				colorbuf[addr] = 15;
				valChg[i] = 1;
			}
			else
			{
				if ( colorbuf[addr] > 0 )
				{
				   colorbuf[addr]--;
					valChg[i] = 1;
				}
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
				next_iter = iter;
				gtk_text_iter_forward_chars( &next_iter, 4 );

				if ( !reset )
				{
					gtk_text_buffer_delete( textbuf, &iter, &next_iter );
				}
				//gtk_text_buffer_insert( textbuf, &iter, valStr[i], -1 );
				gtk_text_buffer_insert_with_tags( textbuf, &iter, valStr[i], -1, highlight[ colorbuf[addr] ], NULL );
			}
			else
			{
				gtk_text_iter_forward_chars( &iter, 4 );
			}
		}
		ascii[16] = 0;

		for (i=0; i<16; i++)
		{
			if ( valChg[i] )
			{
				next_iter = iter;
				gtk_text_iter_forward_chars( &next_iter, 1 );

				if ( !reset )
				{
					gtk_text_buffer_delete( textbuf, &iter, &next_iter );
				}
				gtk_text_buffer_insert( textbuf, &iter, &ascii[i], 1 );
			}
			else
			{
				gtk_text_iter_forward_chars( &iter, 1 );
			}
		}

		numCharsPerLine = 9 + (4*16) + 16 + 1;

		if ( reset )
		{
			gtk_text_buffer_insert ( textbuf, &iter, "\n", -1 );
		}
		else
		{
			gtk_text_iter_forward_chars( &iter, 1 );
		}
		//if ( row_changed )
		//{
		//	if ( reset )
		//	{
		//		gtk_text_buffer_get_iter_at_offset( textbuf, &iter, totalChars );
		//		gtk_text_buffer_insert ( textbuf, &iter, line.c_str(), -1 );
		//	}
		//	else
		//	{
		//		GtkTextIter next_iter;

		//		gtk_text_buffer_get_iter_at_offset( textbuf, &next_iter, totalChars + numCharsPerLine - 1 );
		//		gtk_text_buffer_delete ( textbuf, &iter, &next_iter );

		//		gtk_text_buffer_get_iter_at_offset( textbuf, &iter, totalChars );
		//		gtk_text_buffer_insert ( textbuf, &iter, line.c_str(), line.size()-1 );
		//	}
		//}

		totalChars += numCharsPerLine;
	}

	// Put cursor back where it was
	gtk_text_buffer_get_iter_at_offset( textbuf, &iter, cpos );
	gtk_text_buffer_place_cursor( textbuf, &iter );

	// Check if a Jump Delay is set.
	if ( jumpDelay <= 0 )
	{
		if ( jumpAddr >= 0 )
		{
         gotoLocation( jumpAddr );
			jumpAddr = -1;
		}
		jumpDelay = 0;
	}
	else
	{
		jumpDelay--;
	}

	if ( selAddr >= 0 )
	{
		sprintf( addrStr, "Selected Addr: 0x%08X", selAddr );
	}
	else
	{
		addrStr[0] = 0;
	}
	gtk_label_set_text( GTK_LABEL(selCellLabel), addrStr );

}

int memViewWin_t::calcVisibleRange( int *start_out, int *end_out, int *center_out )
{
	//GtkAdjustment *ivadj;
	//double v, l, u, r;
	int start, end, center;
   GdkRectangle rect;
   GtkTextIter iter;

   gtk_text_view_get_visible_rect( textview, &rect );

   gtk_text_view_get_line_at_y( textview, &iter, rect.y, NULL );
   start = gtk_text_iter_get_line( &iter ) - 1;

   gtk_text_view_get_line_at_y( textview, &iter, rect.y+rect.height, NULL );
   end  = gtk_text_iter_get_line( &iter ) + 1;

   if ( start < 0 ) start = 0;

   if ( end > numLines )
   {
      end = numLines;
   }

   //printf("Line Iter: %i -> %i\n", start, end );

	//ivadj = gtk_range_get_adjustment( GTK_RANGE(ivbar) );


	//v = gtk_range_get_value( GTK_RANGE(ivbar) );
	//l = gtk_adjustment_get_lower( ivadj );
	//u = gtk_adjustment_get_upper( ivadj );

	//r = (v - l) / (u - l);

	//start = ((int)( r * (double)numLines )) - 16;

	//if ( start < 0 )
	//{
	//	start = 0;
	//}
	//end = start + 64;

	//if ( end > numLines )
	//{
	//	end = numLines;
	//}

	center = start + (end - start)/2;

	//printf(" Start:%i   End:%i      0x%08x -> 0x%08x \n", start, end, start * 16, end * 16 );

	if ( start_out  ) *start_out  = start;
	if ( end_out    ) *end_out    = end;
	if ( center_out ) *center_out = center;

   //printf("Start:%i  End:%i   StartADDR:%08X  EndADDR:%08X  Rect:  X:%i   Y:%i   W:%i  H:%i \n", 
   //      start, end, start*16, end*16, rect.x, rect.y, rect.width, rect.height );

   return 0;
}

int memViewWin_t::gotoLocation( int addr )
{
	int linenum, bytenum, cpos;
	GtkTextIter iter;

	linenum = addr / 16;
	bytenum = addr % 16;

	cpos = (linenum*numCharsPerLine) + (bytenum*4) + 10;

	//printf("Line:%i  Byte:%i   CPOS:%i  \n", linenum, bytenum, cpos );

	gtk_text_buffer_get_iter_at_offset( textbuf, &iter, cpos );

	gtk_text_view_scroll_to_iter ( textview, &iter, 0.0, 1, 0.0, 0.25 );
	gtk_text_buffer_place_cursor( textbuf, &iter );
                               
   return 0;
}

static int memViewEvntSrcID = 0;
static std::list <memViewWin_t*> memViewWinList;

static void changeModeRAM (GtkRadioMenuItem * radiomenuitem, memViewWin_t *mv)
{
	printf("Changing Mode RAM \n");
	mv->setMode( memViewWin_t::MODE_NES_RAM );
}
static void changeModePPU (GtkRadioMenuItem * radiomenuitem, memViewWin_t *mv)
{
	printf("Changing Mode PPU \n");
	mv->setMode( memViewWin_t::MODE_NES_PPU );
}
static void changeModeOAM (GtkRadioMenuItem * radiomenuitem, memViewWin_t *mv)
{
	printf("Changing Mode OAM \n");
	mv->setMode( memViewWin_t::MODE_NES_OAM );
}
static void changeModeROM (GtkRadioMenuItem * radiomenuitem, memViewWin_t *mv)
{
	printf("Changing Mode ROM \n");
	mv->setMode( memViewWin_t::MODE_NES_ROM );
}

static GtkWidget *CreateMemViewMenubar (memViewWin_t * mv)
{
	GtkWidget *menubar, *menu, *item;
	GSList *radioGroup;

	menubar = gtk_menu_bar_new ();

	item = gtk_menu_item_new_with_label ("View");

	gtk_menu_shell_append (GTK_MENU_SHELL (menubar), item);

	menu = gtk_menu_new ();

	gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);

	//-View --> RAM ------------------
	radioGroup = NULL;

	mv->memSelRadioItem[0] = gtk_radio_menu_item_new_with_label (radioGroup, "RAM");

	radioGroup =
			gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM(mv->memSelRadioItem[0]));

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), mv->memSelRadioItem[0]);

	g_signal_connect (mv->memSelRadioItem[0], "activate", G_CALLBACK (changeModeRAM),
				  (gpointer) mv);

	//-View --> PPU ------------------
	mv->memSelRadioItem[1] = gtk_radio_menu_item_new_with_label (radioGroup, "PPU");

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), mv->memSelRadioItem[1]);

	g_signal_connect (mv->memSelRadioItem[1], "activate", G_CALLBACK (changeModePPU),
				  (gpointer) mv);

	//-View --> OAM ------------------
	mv->memSelRadioItem[2] = gtk_radio_menu_item_new_with_label (radioGroup, "OAM");

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), mv->memSelRadioItem[2]);

	g_signal_connect (mv->memSelRadioItem[2], "activate", G_CALLBACK (changeModeOAM),
				  (gpointer) mv);

	//-View --> ROM ------------------
	mv->memSelRadioItem[3] = gtk_radio_menu_item_new_with_label (radioGroup, "ROM");

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), mv->memSelRadioItem[3]);

	g_signal_connect (mv->memSelRadioItem[3], "activate", G_CALLBACK (changeModeROM),
				  (gpointer) mv);

	// Finally, return the actual menu bar created
	return menubar;
}


static void closeMemoryViewWindow (GtkWidget * w, GdkEvent * e, memViewWin_t * mv)
{
	std::list < memViewWin_t * >::iterator it;

	for (it = memViewWinList.begin (); it != memViewWinList.end (); it++)
	{
		if (mv == *it)
		{
			//printf("Removing MemView Window %p from List\n", cw);
			memViewWinList.erase (it);
			break;
		}
	}

	delete mv;

	gtk_widget_destroy (w);
}

static void
textview_string_insert (GtkTextView *text_view,
               gchar       *string,
               memViewWin_t * mv )
{
	printf("String: '%s'\n", string );

}

static gboolean textbuffer_string_insert (GtkWidget * grab, GdkEventKey * event,  memViewWin_t * mv)
{
	int addr, line, offs, byte0, byte, bcol, c, d;
	gint cpos;
	gboolean stopKeyPropagate = TRUE;

	g_object_get( mv->textbuf, "cursor-position", &cpos, NULL );

	line = cpos / mv->numCharsPerLine;
	offs = cpos % mv->numCharsPerLine;

	byte0 = (offs - 10);

	byte = byte0 / 4;
	bcol = byte0 % 4;

	addr = (line*16) + byte;

	//printf("Line: %i   Offset: %i   Byte:%i   Bcol:%i\n", line, offs, byte, bcol );
	
	d = event->keyval;

	//printf("Key: %i  '%c' \n", d, d );

	stopKeyPropagate = (d != GDK_KEY_Up       ) &&
		                (d != GDK_KEY_Down     ) &&
		                (d != GDK_KEY_Left     ) &&
		                (d != GDK_KEY_Right    ) &&
		                (d != GDK_KEY_Page_Up  ) &&
		                (d != GDK_KEY_Page_Down);

	if ( !isascii( d ) )
	{
		return stopKeyPropagate;
	}

	if ( offs > (9 + (16*4)) )
	{  // ASCII Text Area
		byte = (offs - 73);

		if ( (byte < 0) || (byte >= 16) )
		{
			return stopKeyPropagate;
		}
		addr = (line*16) + byte;

		c = d;
	}
	else
	{  // Hex Text Area
		if ( !isxdigit( d ) )
		{
			return stopKeyPropagate;
		}
		byte0 = (offs - 10);

		byte = byte0 / 4;
		bcol = byte0 % 4;

		addr = (line*16) + byte;

		if ( (d >= '0') && (d <= '9') )
		{
			d = d - '0';
		}
		else if ( (d >= 'a') && (d <= 'f') )
		{
			d = d - 'a' + 10;
		}
  		else 
		{
			d = d - 'A' + 10;
		}

		c = mv->memAccessFunc( addr );

		//printf("Changing Addr:0x%08x  from:0x%02x ", addr, c );

		if ( bcol == 0 )
		{
			c = c & 0x0f;
			c = c | (d << 4);
		}
		else if ( bcol == 1 )
		{
			c = c & 0xf0;
			c = c | d;
		}
	}
	//printf(" to:0x%02x \n", c );

	mv->writeMem( addr, c );

	// Return wether to allow GTK+ to process this key.
	return stopKeyPropagate;
}

static void
textview_backspace_cb (GtkTextView *text_view,
				      	memViewWin_t * mv)
{
	//printf("BackSpace:\n");
	mv->redraw = 1;
}

static void handleDialogResponse (GtkWidget * w, gint response_id, memViewWin_t * mv)
{
	//printf("Response %i\n", response_id ); 

	if ( response_id == GTK_RESPONSE_OK )
	{
		const char *txt;

		//printf("Reponse OK\n");

		txt = gtk_entry_get_text( GTK_ENTRY( mv->addr_entry ) );

		if ( txt != NULL )
		{
			//printf("Text: '%s'\n", txt );
			switch (mv->dialog_op)
			{
				case 1:
					if ( isxdigit(txt[0]) )
					{  // Address is always treated as hex number
						mv->gotoLocation( strtol( txt, NULL, 16 ) );
					}
				break;
				case 2:
					if ( isdigit(txt[0]) )
					{  // Value is numerical
						mv->writeMem( mv->selAddr, strtol( txt, NULL, 0 ) );
					}
					else if ( (txt[0] == '\'') && (txt[2] == '\'') )
					{  // Byte to be written is expressed as ASCII character
						mv->writeMem( mv->selAddr, txt[1] );
					}
				break;
			}
		}
	}
	//else if ( response_id == GTK_RESPONSE_CANCEL )
	//{
	//	printf("Reponse Cancel\n");
	//}
	gtk_widget_destroy (w);

	mv->addr_entry = NULL;
}

static void closeDialogWindow (GtkWidget * w, GdkEvent * e, memViewWin_t * mv)
{
	gtk_widget_destroy (w);

	mv->addr_entry = NULL;
}

static void
gotoLocationCB (GtkMenuItem *menuitem,
				      	memViewWin_t * mv)
{
	GtkWidget *win;
	GtkWidget *vbox;

	mv->dialog_op = 1;

	win = gtk_dialog_new_with_buttons ("Goto Address",
					       GTK_WINDOW (mv->win),
					       (GtkDialogFlags)
					       (GTK_DIALOG_DESTROY_WITH_PARENT),
					       "_Cancel", GTK_RESPONSE_CANCEL,
					       "_Goto", GTK_RESPONSE_OK, NULL);

	gtk_dialog_set_default_response( GTK_DIALOG(win), GTK_RESPONSE_OK );

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 1);

	mv->addr_entry = gtk_entry_new ();

	gtk_entry_set_activates_default( GTK_ENTRY (mv->addr_entry), TRUE );

	gtk_entry_set_text (GTK_ENTRY (mv->addr_entry), "0x0");

	gtk_box_pack_start (GTK_BOX (vbox), mv->addr_entry, TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX
			    (gtk_dialog_get_content_area
			     (GTK_DIALOG (win))), vbox, TRUE, TRUE, 1);

	gtk_widget_show_all (win);

	g_signal_connect (win, "delete-event",
			  G_CALLBACK (closeDialogWindow), mv);
	g_signal_connect (win, "response",
			  G_CALLBACK (handleDialogResponse), mv);

}

static void
setValueCB (GtkMenuItem *menuitem,
				      	memViewWin_t * mv)
{
	GtkWidget *win;
	GtkWidget *vbox;
	char stmp[256];

	mv->dialog_op = 2;

	sprintf( stmp, "Poke Address 0x%08x", mv->selAddr );

	win = gtk_dialog_new_with_buttons ( stmp,
					       GTK_WINDOW (mv->win),
					       (GtkDialogFlags)
					       (GTK_DIALOG_DESTROY_WITH_PARENT),
					       "_Cancel", GTK_RESPONSE_CANCEL,
					       "_Write", GTK_RESPONSE_OK, NULL);

	gtk_dialog_set_default_response( GTK_DIALOG(win), GTK_RESPONSE_OK );

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 1);

	mv->addr_entry = gtk_entry_new ();

	gtk_entry_set_text (GTK_ENTRY (mv->addr_entry), "0x0");

	gtk_entry_set_activates_default( GTK_ENTRY (mv->addr_entry), TRUE );

	gtk_box_pack_start (GTK_BOX (vbox), mv->addr_entry, TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX
			    (gtk_dialog_get_content_area
			     (GTK_DIALOG (win))), vbox, TRUE, TRUE, 1);

	gtk_widget_show_all (win);

	g_signal_connect (win, "delete-event",
			  G_CALLBACK (closeDialogWindow), mv);
	g_signal_connect (win, "response",
			  G_CALLBACK (handleDialogResponse), mv);

}

static void
gotoRomLocationCB (GtkMenuItem *menuitem,
				      	memViewWin_t * mv)
{
	gtk_menu_item_activate( GTK_MENU_ITEM(mv->memSelRadioItem[3]) );

	mv->setMode( memViewWin_t::MODE_NES_ROM );

	mv->showMemViewResults(1);

	mv->jumpAddr  = mv->selRomAddr;
	mv->jumpDelay = 10;

}

static gboolean
populate_context_menu (GtkWidget *popup,
               memViewWin_t * mv )
{
   GtkWidget *menu = NULL;
   GtkWidget *item;
	char stmp[256];

   //printf("Context Menu\n");
	
	menu = gtk_menu_new ();
	
	item = gtk_menu_item_new_with_label("Goto Address...");

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

	g_signal_connect (item, "activate",
  			G_CALLBACK (gotoLocationCB), mv);

	switch ( mv->mode )
	{
		case memViewWin_t::MODE_NES_RAM:
		{
			if ( mv->selAddr >= 0x0000 )
			{
				sprintf( stmp, "Poke RAM 0x%04X", mv->selAddr );

   			item = gtk_menu_item_new_with_label(stmp);

   			gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

				g_signal_connect (item, "activate",
  					G_CALLBACK (setValueCB), mv);
			}
			if ( mv->selAddr >= 0x6000 )
			{
				mv->selRomAddr = GetNesFileAddress(mv->selAddr);

				if ( mv->selRomAddr >= 0 )
				{
					sprintf( stmp, "Goto ROM 0x%08X", mv->selRomAddr );

					item = gtk_menu_item_new_with_label(stmp);

					gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

					g_signal_connect (item, "activate",
			  			G_CALLBACK (gotoRomLocationCB), mv);
				}
			}
		}
		break;
		case memViewWin_t::MODE_NES_PPU:
		{
			if ( mv->selAddr >= 0x0000 )
			{
				sprintf( stmp, "Poke PPU 0x%08X", mv->selAddr );

   			item = gtk_menu_item_new_with_label(stmp);

   			gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

				g_signal_connect (item, "activate",
  					G_CALLBACK (setValueCB), mv);
			}
		}
		break;
		case memViewWin_t::MODE_NES_OAM:
		{
			if ( mv->selAddr >= 0x0000 )
			{
				sprintf( stmp, "Poke OAM 0x%08X", mv->selAddr );

   			item = gtk_menu_item_new_with_label(stmp);

   			gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

				g_signal_connect (item, "activate",
  					G_CALLBACK (setValueCB), mv);
			}
		}
		break;
		case memViewWin_t::MODE_NES_ROM:
		{
			if ( mv->selAddr >= 0x0000 )
			{
				sprintf( stmp, "Poke ROM 0x%08X", mv->selAddr );

   			item = gtk_menu_item_new_with_label(stmp);

   			gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

				g_signal_connect (item, "activate",
  					G_CALLBACK (setValueCB), mv);
			}
		}
		break;
	}

   gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL );
	//gtk_widget_show_all (popup);
	gtk_widget_show_all (menu);

   return TRUE;
}

static gboolean
textview_button_press_cb (GtkWidget *widget,
               GdkEventButton  *event,
               memViewWin_t * mv )
{
   gboolean  ret = FALSE;
   //printf("Press Button  %i   %u\n", event->type, event->button );

   if ( event->button == 3 )
   {
      ret = populate_context_menu( widget, mv );
   }

   return ret;
}

static gint updateMemViewTree (void *userData)
{
	std::list <memViewWin_t*>::iterator it;
  
	for (it=memViewWinList.begin(); it != memViewWinList.end(); it++)
	{
      (*it)->showMemViewResults(0);
	}
	return 1;
}

void openMemoryViewWindow (void)
{
	GtkWidget *main_vbox;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *scroll;
	GtkWidget *menubar;
	memViewWin_t *mv;
	GdkRGBA  color;

	mv = new memViewWin_t;

	memViewWinList.push_back (mv);

	mv->win = gtk_dialog_new_with_buttons ("Memory View",
						GTK_WINDOW (MainWindow),
						(GtkDialogFlags)
						(GTK_DIALOG_DESTROY_WITH_PARENT),
						"_Close", GTK_RESPONSE_OK,
						NULL);

	gtk_window_set_default_size (GTK_WINDOW (mv->win), 800, 600);

	main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 3);

	menubar = CreateMemViewMenubar(mv);

	gtk_box_pack_start (GTK_BOX (main_vbox), menubar, FALSE, TRUE, 0);

	mv->textview = (GtkTextView*) gtk_text_view_new();

	gtk_text_view_set_monospace( mv->textview, TRUE );
	gtk_text_view_set_overwrite( mv->textview, TRUE );
	gtk_text_view_set_editable( mv->textview, TRUE );
	gtk_text_view_set_wrap_mode( mv->textview, GTK_WRAP_NONE );
	gtk_text_view_set_cursor_visible( mv->textview, TRUE );

	g_signal_connect (mv->textview, "insert-at-cursor",
			  G_CALLBACK (textview_string_insert), mv);
	g_signal_connect (mv->textview, "preedit-changed",
			  G_CALLBACK (textview_string_insert), mv);
	g_signal_connect (mv->textview, "backspace",
			  G_CALLBACK (textview_backspace_cb), mv);
	g_signal_connect (mv->textview, "popup-menu",
			  G_CALLBACK (populate_context_menu), mv);
	g_signal_connect (mv->textview, "button-press-event",
			  G_CALLBACK (textview_button_press_cb), mv);

	mv->textbuf = gtk_text_view_get_buffer( mv->textview );

	//g_signal_connect (mv->textbuf, "insert-text",
	//		  G_CALLBACK (textbuffer_string_insert), mv);
	g_signal_connect (G_OBJECT (mv->textview), "key-press-event",
			  G_CALLBACK (textbuffer_string_insert), mv);

	for (int i=0; i<HIGHLIGHT_ACTIVITY_NUM_COLORS; i++)
	{
		int r,g,b;
		char stmp[64];

		sprintf( stmp, "hightlight%i\n", i );

		r = (highlightActivityColors[i] & 0x000000ff);
		g = (highlightActivityColors[i] & 0x0000ff00) >> 8;
		b = (highlightActivityColors[i] & 0x00ff0000) >> 16;

		color.red   = (double)r / 255.0f;
		color.green = (double)g / 255.0f;
		color.blue  = (double)b / 255.0f;
		color.alpha = 1.0;

		printf("%i  R:%f  G:%f  B:%f \n", i, color.red, color.green, color.blue );

      mv->highlight[i] = gtk_text_buffer_create_tag( mv->textbuf, stmp, "foreground-rgba", &color, NULL );
	}

	scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
					GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
	gtk_container_add (GTK_CONTAINER (scroll), GTK_WIDGET(mv->textview) );

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 1);
	gtk_box_pack_start (GTK_BOX (vbox), scroll, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 2);

	gtk_box_pack_start (GTK_BOX (main_vbox), hbox, TRUE, TRUE, 5);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);

	mv->selCellLabel = gtk_label_new("");
	//g_object_set (mv->selCellLabel, "family", "MonoSpace", NULL);
	gtk_box_pack_start (GTK_BOX (hbox), mv->selCellLabel, FALSE, FALSE, 5);
	gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 5);

	gtk_box_pack_start (GTK_BOX
			    (gtk_dialog_get_content_area
			     (GTK_DIALOG (mv->win))), main_vbox, TRUE, TRUE,
			    0);

   mv->ivbar = gtk_scrolled_window_get_vscrollbar( GTK_SCROLLED_WINDOW(scroll) );

	g_signal_connect (mv->win, "delete-event",
			  G_CALLBACK (closeMemoryViewWindow), mv);
	g_signal_connect (mv->win, "response",
			  G_CALLBACK (closeMemoryViewWindow), mv);

	gtk_widget_show_all (mv->win);

	mv->showMemViewResults(1);

	if (memViewEvntSrcID == 0)
	{
		memViewEvntSrcID =
			g_timeout_add (100, updateMemViewTree, mv);
	}

}
