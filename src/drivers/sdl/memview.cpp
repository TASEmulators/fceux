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
	GtkWidget *tree;
	GtkWidget *vbar;
	GtkWidget *selCellLabel;
	GtkTreeStore *memview_store;
	int selAddr;
	int selRowIdx;
	int selColIdx;
	int editRowIdx;
	int editColIdx;
	int row_vis_start;
	int row_vis_end;
	int mode;
	int evntSrcID;
	unsigned char *mbuf;
	int mbuf_size;
	GtkCellRenderer *hexByte_renderer[16];

	enum {
		MODE_NES_RAM = 0,
		MODE_NES_PPU,
		MODE_NES_OAM,
		MODE_NES_ROM
	};

	memViewWin_t(void)
	{
		win = NULL;
		tree = NULL;
		vbar = NULL;
		selCellLabel = NULL;
		memview_store = NULL;
		selRowIdx  = -1;
		selColIdx  = -1;
	   editRowIdx = -1;
	   editColIdx = -1;
		selAddr = -1;
		row_vis_start = 0;
		row_vis_end   = 0;
		mode = MODE_NES_RAM;
		mbuf = NULL;
		evntSrcID = 0;

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

	void showMemViewResults (int reset);

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

void memViewWin_t::showMemViewResults (int reset)
{
	
	int lineAddr = 0, line_addr_start, line_addr_end, i, row;
	int addr, memSize = 0, un, ln;
	unsigned int c;
	GtkTreeIter iter;
	char addrStr[16], valStr[16][8], ascii[18], row_changed;
	int *indexArray;
	GtkTreePath *start_path = NULL, *end_path = NULL;
   int (*memAccessFunc)( unsigned int offset) = NULL;

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

	if (reset)
	{
		line_addr_start = 0;
		line_addr_end   = memSize;

		gtk_tree_store_clear (memview_store);

		for ( lineAddr=line_addr_start; lineAddr < (line_addr_end+1); lineAddr += 16 )
		{
			gtk_tree_store_append (memview_store, &iter, NULL);	// aquire iter
		}
		gtk_tree_model_get_iter_first( GTK_TREE_MODEL (memview_store), &iter );

		line_addr_start = 0;
		line_addr_end   = memSize;

		row_vis_start   = 0;
		row_vis_end     = 60;
	}

	if ( !reset )
	{
		if ( gtk_tree_view_get_visible_range ( GTK_TREE_VIEW(tree), &start_path, &end_path ) )
		{
			int iterValid;
			indexArray = gtk_tree_path_get_indices (start_path);

			if ( indexArray != NULL )
			{
				row_vis_start = indexArray[0];
			}

			indexArray = gtk_tree_path_get_indices (end_path);

			if ( indexArray != NULL )
			{
				row_vis_end = indexArray[0];
			}

			iterValid = gtk_tree_model_get_iter( GTK_TREE_MODEL (memview_store), &iter, start_path );

			gtk_tree_path_free( start_path );
			gtk_tree_path_free( end_path );

			if ( !iterValid )
			{
				printf("Error: Failed to get start iterator.\n");
				return;
			}
			//printf("Tree View Start: %i   End: %i  \n", row_vis_start, row_vis_end );
		}
	}
	line_addr_start = row_vis_start * 16;
	line_addr_end   = row_vis_end   * 16;

	row = row_vis_start;

	for ( lineAddr=line_addr_start; lineAddr < line_addr_end; lineAddr += 16 )
	{
		row_changed = reset;

		sprintf( addrStr, "%08X", lineAddr );

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

		if ( row_changed && (editRowIdx != row) )
		{
			gtk_tree_store_set( memview_store, &iter, 0, addrStr, 
					1 , valStr[0] ,  2, valStr[1] ,  3, valStr[2] ,  4, valStr[3], 
					5 , valStr[4] ,  6, valStr[5] ,  7, valStr[6] ,  8, valStr[7], 
					9 , valStr[8] , 10, valStr[9] , 11, valStr[10], 12, valStr[11], 
					13, valStr[12], 14, valStr[13], 15, valStr[14], 16, valStr[15], 
					17, ascii,
					-1 );
		}

		if (!gtk_tree_model_iter_next
		    (GTK_TREE_MODEL (memview_store), &iter))
		{
			return;
		}
		row++;
	}
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
treeRowActivated (GtkTreeView       *tree_view,
             		GtkTreePath       *path,
             		GtkTreeViewColumn *column,
             		memViewWin_t      * mv )
{
	char stmp[128];

	mv->selRowIdx = strtol( gtk_tree_path_to_string(path), NULL, 0 );
	mv->selColIdx = strtol( gtk_tree_view_column_get_title(column), NULL, 16 );

	mv->selAddr = (mv->selRowIdx*16) + mv->selColIdx;

	sprintf( stmp, "<span font_desc=\"mono 12\">Selected Cell Address: 0x%04X</span>", mv->selAddr );

	gtk_label_set_markup ( GTK_LABEL(mv->selCellLabel), stmp );

	//printf("Row:Col Active: %i:%i   Addr: 0x%04X \n", mv->selRowIdx, mv->selColIdx, mv->selAddr );
}

static void memview_cell_edited_cb (GtkCellRendererText * cell,
				     gchar * path_string,
				     gchar * new_text, memViewWin_t * mv)
{
	int addr, rowIdx;
	unsigned int value;
	writefunc wfunc;

	//printf("PATH: %s \n", path_string );

	rowIdx = atoi( path_string );

	addr = (rowIdx*16) + mv->selColIdx;

	wfunc = GetWriteHandler (addr);

	if (wfunc)
	{
		value = strtoul (new_text, NULL, 16);

		wfunc ((uint32) addr,
		       (uint8) (value & 0x000000ff));
	}

	mv->editRowIdx = -1;
	mv->editColIdx = -1;
}
static void memview_cell_edited_start_cb (GtkCellRenderer * renderer, GtkCellEditable * editable, gchar * path, memViewWin_t * mv)
{
	//printf("MemView Edit Start: '%s':%li\n", path,  (long)user_data);
	mv->editRowIdx = atoi (path);
	mv->editColIdx = mv->selColIdx;
}

static void memview_cell_edited_cancel_cb (GtkCellRenderer * renderer, memViewWin_t * mv)
{
	//printf("MemView Edit Cancel:%li\n", (long)user_data);
	mv->editRowIdx = -1;
	mv->editColIdx = -1;
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
	GtkWidget *scroll;
	GtkWidget *menubar;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	memViewWin_t *mv;
   //void (*memview_cell_edited_start_cbs)(GtkCellRenderer * renderer, GtkCellEditable * editable, gchar * path, memViewWin_t * mv)[16];

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

	mv->memview_store =
		gtk_tree_store_new ( 18, 
				G_TYPE_STRING, // Address Field
				   G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, // Bytes 0  -> 3
				   G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, // Bytes 4  -> 7
				   G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, // Bytes 8  -> 11
				   G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, // Bytes 12 -> 15
				G_TYPE_STRING ); // Ascii Byte Decoding

	mv->tree =
		gtk_tree_view_new_with_model (GTK_TREE_MODEL
					      (mv->memview_store));

	gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (mv->tree),
				      GTK_TREE_VIEW_GRID_LINES_VERTICAL);

	g_object_set( mv->tree, "activate-on-single-click", TRUE, NULL );

	g_signal_connect (mv->tree, "row-activated",
			  (GCallback) treeRowActivated,
			  (gpointer) mv);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer, "family", "MonoSpace", NULL);
	column = gtk_tree_view_column_new_with_attributes ("Addr", renderer,
							   "text", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (mv->tree), column);

	for (long int i=0; i<16; i++)
	{
		char title[16];

		sprintf( title, "%02lX", i );

		renderer = gtk_cell_renderer_text_new ();
		g_object_set (renderer, "family", "MonoSpace", NULL);
		g_object_set (renderer, "editable", TRUE, NULL);
		g_signal_connect (renderer, "edited",
				  (GCallback) memview_cell_edited_cb, (gpointer) mv);
		g_signal_connect (renderer, "editing-started",
				  (GCallback) memview_cell_edited_start_cb,
				  (gpointer) mv);
		g_signal_connect (renderer, "editing-canceled",
				  (GCallback) memview_cell_edited_cancel_cb,
				  (gpointer) mv);
		column = gtk_tree_view_column_new_with_attributes ( title,
								   renderer, "text", i+1,
								   NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (mv->tree), column);

		mv->hexByte_renderer[i] = renderer;
	}

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer, "family", "MonoSpace", NULL);
	column = gtk_tree_view_column_new_with_attributes ("ASCII", renderer,
							   "text", 17, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (mv->tree), column);

	scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
					GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (scroll), mv->tree);
	gtk_box_pack_start (GTK_BOX (main_vbox), scroll, TRUE, TRUE, 5);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);

	mv->selCellLabel = gtk_label_new("");
	//g_object_set (mv->selCellLabel, "family", "MonoSpace", NULL);
	gtk_box_pack_start (GTK_BOX (hbox), mv->selCellLabel, FALSE, FALSE, 5);
	gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 5);

	gtk_box_pack_start (GTK_BOX
			    (gtk_dialog_get_content_area
			     (GTK_DIALOG (mv->win))), main_vbox, TRUE, TRUE,
			    0);

   mv->vbar = gtk_scrolled_window_get_vscrollbar( GTK_SCROLLED_WINDOW(scroll) );

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
