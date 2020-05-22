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

//
struct memViewWin_t
{
	GtkWidget *win;
	GtkWidget *ivbar;
	GtkWidget *selCellLabel;
	GtkTextView *textview;
	GtkTextBuffer *textbuf;
	GtkTreeStore *memview_store;
	int selAddr;
	int row_vis_start;
	int row_vis_end;
	int row_vis_center;
	int mode;
	int evntSrcID;
	int numLines;
	int numCharsPerLine;
	unsigned char *mbuf;
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
		selCellLabel = NULL;
		memview_store = NULL;
		row_vis_start = 0;
		row_vis_end   = 64;
		row_vis_center= 32;
		selAddr = 0;
		mode = MODE_NES_RAM;
		mbuf = NULL;
		numLines = 0;
		evntSrcID = 0;
		numCharsPerLine = 90;
		redraw = 1;
		memAccessFunc = getRAM;

		for (int i=0; i<16; i++)
		{
			hexByte_renderer[i] = NULL;
		}
	}

	~memViewWin_t(void)
	{
		if ( mbuf != NULL )
		{
			free(mbuf); mbuf = NULL;
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
		switch ( mode )
		{
			default:
			case MODE_NES_RAM:
			{
				writefunc wfunc;

				wfunc = GetWriteHandler (addr);

				if (wfunc)
				{
					wfunc ((uint32) addr,
					       (uint8) (value & 0x000000ff));
				}
			}
			break;
			case MODE_NES_PPU:
			break;
			case MODE_NES_OAM:
			break;
			case MODE_NES_ROM:
			break;
		}
      return 0;
	}

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
	char row_changed;
	std::string line;
	GtkTextIter iter, start_iter, end_iter;

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
	}
	else
	{
		calcVisibleRange( &row_start, &row_end, NULL );
	}

	gtk_text_buffer_get_iter_at_offset( textbuf, &iter, 0 );

	totalChars = row_start * numCharsPerLine;

	for (row=row_start; row<row_end; row++)
	{
		gtk_text_buffer_get_iter_at_offset( textbuf, &iter, totalChars );

		row_changed = 1;

		line.clear();

		lineAddr = (row*16);

		sprintf( addrStr, "%08X ", lineAddr );

		for (i=0; i<16; i++)
		{
			addr = lineAddr+i;

			c = memAccessFunc(addr);

			un = ( c & 0x00f0 ) >> 4;
			ln = ( c & 0x000f );

			valStr[i][0] = conv2xchar(un);
			valStr[i][1] = conv2xchar(ln);
			valStr[i][2] = 0;

			if ( isprint(c) )
			{
            ascii[i] = c;
			}
			else
			{
            ascii[i] = '.';
			}
			if ( c != mbuf[addr] )
			{
				row_changed = 1;
				mbuf[addr] = c;
			}
		}
		ascii[16] = 0;

		line.assign( addrStr  );

		for (i=0; i<16; i++)
		{
			line.append( " ");
			line.append( valStr[i] );
			line.append( " ");
		}
		line.append( ascii );
		line.append( "\n");

		numCharsPerLine = line.size();

		if ( row_changed )
		{
			if ( reset )
			{
				gtk_text_buffer_get_iter_at_offset( textbuf, &iter, totalChars );
				gtk_text_buffer_insert ( textbuf, &iter, line.c_str(), -1 );
			}
			else
			{
				GtkTextIter next_iter;

				gtk_text_buffer_get_iter_at_offset( textbuf, &next_iter, totalChars + numCharsPerLine - 1 );
				gtk_text_buffer_delete ( textbuf, &iter, &next_iter );

				gtk_text_buffer_get_iter_at_offset( textbuf, &iter, totalChars );
				gtk_text_buffer_insert ( textbuf, &iter, line.c_str(), line.size()-1 );
			}
		}

		totalChars += numCharsPerLine;
	}

	// Put cursor back where it was
	gtk_text_buffer_get_iter_at_offset( textbuf, &iter, cpos );
	gtk_text_buffer_place_cursor( textbuf, &iter );

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
	GtkAdjustment *ivadj;
	double v, l, u, r;
	int start, end, center;

	ivadj = gtk_range_get_adjustment( GTK_RANGE(ivbar) );


	v = gtk_range_get_value( GTK_RANGE(ivbar) );
	l = gtk_adjustment_get_lower( ivadj );
	u = gtk_adjustment_get_upper( ivadj );

	r = (v - l) / (u - l);

	start = ((int)( r * (double)numLines )) - 16;

	if ( start < 0 )
	{
		start = 0;
	}
	end = start + 64;

	if ( end > numLines )
	{
		end = numLines;
	}

	center = start + (end - start)/2;

	//printf(" Start:%i   End:%i      0x%08x -> 0x%08x \n", start, end, start * 16, end * 16 );

	if ( start_out  ) *start_out  = start;
	if ( end_out    ) *end_out    = end;
	if ( center_out ) *center_out = center;

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

	item = gtk_radio_menu_item_new_with_label (radioGroup, "RAM");

	radioGroup =
			gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM(item));

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

	g_signal_connect (item, "activate", G_CALLBACK (changeModeRAM),
				  (gpointer) mv);

	//-View --> PPU ------------------
	item = gtk_radio_menu_item_new_with_label (radioGroup, "PPU");

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

	g_signal_connect (item, "activate", G_CALLBACK (changeModePPU),
				  (gpointer) mv);

	//-View --> OAM ------------------
	item = gtk_radio_menu_item_new_with_label (radioGroup, "OAM");

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

	g_signal_connect (item, "activate", G_CALLBACK (changeModeOAM),
				  (gpointer) mv);

	//-View --> ROM ------------------
	item = gtk_radio_menu_item_new_with_label (radioGroup, "ROM");

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

	g_signal_connect (item, "activate", G_CALLBACK (changeModeROM),
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

static void
textbuffer_string_insert (GtkTextBuffer *textbuffer,
               GtkTextIter   *location,
               gchar         *text,
               gint           len,
               memViewWin_t * mv )
{
	if ( len == 1 )
	{
		int addr, line, offs, byte0, byte, bcol, c, d;

		line = gtk_text_iter_get_line( location ),
		offs = gtk_text_iter_get_line_offset( location );

		byte0 = (offs - 10);

		byte = byte0 / 4;
		bcol = byte0 % 4;

		addr = (line*16) + byte;

		//printf("Line: %i   Offset: %i   Byte:%i   Bcol:%i\n", line, offs, byte, bcol );
		//printf("Text: '%s'   \n", text );

		if ( !isxdigit( text[0] ) )
		{
			return;
		}
		d = text[0];

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
		//printf(" to:0x%02x \n", c );

		mv->writeMem( addr, c );
	}
}

static void
inner_vbar_changed (GtkRange *range,
				      	memViewWin_t * mv)
{
	GtkAdjustment *ivadj;
	double v, l, u, r;

	ivadj = gtk_range_get_adjustment( range );


	v = gtk_range_get_value( range );
	l = gtk_adjustment_get_lower( ivadj );
	u = gtk_adjustment_get_upper( ivadj );

	r = (v - l) / (u - l);

	//printf("Inner VBAR: %f   %f   %f   %f   \n" , 
	//		v, l, u, r );
}

static void
textview_backspace_cb (GtkTextView *text_view,
				      	memViewWin_t * mv)
{
	//printf("BackSpace:\n");
	mv->redraw = 1;
}

static gboolean
populate_context_menu (GtkWidget *popup,
               memViewWin_t * mv )
{
   GtkWidget *menu;
   GtkWidget *item;

   //printf("Context Menu\n");

	menu = gtk_menu_new ();

   item = gtk_menu_item_new_with_label("Go to ROM");

   gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

   gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL );

	gtk_widget_show_all (popup);
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

	g_signal_connect (mv->textbuf, "insert-text",
			  G_CALLBACK (textbuffer_string_insert), mv);

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

	g_signal_connect (mv->ivbar, "value-changed",
			  G_CALLBACK (inner_vbar_changed), mv);

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
