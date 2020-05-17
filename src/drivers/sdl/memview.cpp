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
//
struct memViewWin_t
{
	GtkWidget *win;
	GtkWidget *tree;
	GtkWidget *vbar;
	GtkTreeStore *memview_store;
	int editRowIdx;
	int editColIdx;
	int row_vis_start;
	int row_vis_end;
	GtkCellRenderer *hexByte_renderer[16];

	memViewWin_t(void)
	{
		win = NULL;
		tree = NULL;
		vbar = NULL;
		memview_store = NULL;
	   editRowIdx = -1;
	   editColIdx = -1;
		row_vis_start = 0;
		row_vis_end   = 0;

		for (int i=0; i<16; i++)
		{
			hexByte_renderer[i] = NULL;
		}
	}

	void showMemViewResults (int reset);

};

void memViewWin_t::showMemViewResults (int reset)
{
	int lineAddr = 0, line_addr_start, line_addr_end, i;
	unsigned int c;
	GtkTreeIter iter;
	char addrStr[16], valStr[16][16], ascii[18];
	int *indexArray;
	GtkTreePath *start_path = NULL, *end_path = NULL;

	if ( gtk_tree_view_get_visible_range ( GTK_TREE_VIEW(tree), &start_path, &end_path ) )
	{
		indexArray = gtk_tree_path_get_indices (start_path);

		row_vis_start = indexArray[0];

		indexArray = gtk_tree_path_get_indices (end_path);

		row_vis_end = indexArray[0];

		if ( !gtk_tree_model_get_iter( GTK_TREE_MODEL (memview_store), &iter, start_path ) )
		{
			return;
		}
		gtk_tree_path_free( start_path );
		gtk_tree_path_free( end_path );

		//printf("Tree View Start: %i   End: %i  \n", row_vis_start, row_vis_end );
	}

	if (reset)
	{
		line_addr_start = 0;
		line_addr_end   = 0x10000;

		gtk_tree_store_clear (memview_store);
	}
	else
	{
		line_addr_start = row_vis_start * 16;
		line_addr_end   = row_vis_end   * 16;
	}

	for ( lineAddr=line_addr_start; lineAddr < line_addr_end; lineAddr += 16 )
	{
		if (reset)
		{
			gtk_tree_store_append (memview_store, &iter, NULL);	// aquire iter
		}

		sprintf( addrStr, "$%06X", lineAddr );

		for (i=0; i<16; i++)
		{
			c = GetMem(lineAddr+i);

			sprintf( valStr[i], "%02X", c );

			if ( isprint(c) )
			{
            ascii[i] = c;
			}
			else
			{
            ascii[i] = '.';
			}
		}
		ascii[16] = 0;

		gtk_tree_store_set( memview_store, &iter, 0, addrStr, 
				1 , valStr[0] ,  2, valStr[1] ,  3, valStr[2] ,  4, valStr[3], 
				5 , valStr[4] ,  6, valStr[5] ,  7, valStr[6] ,  8, valStr[7], 
				9 , valStr[8] , 10, valStr[9] , 11, valStr[10], 12, valStr[11], 
				13, valStr[12], 14, valStr[13], 15, valStr[14], 16, valStr[15], 
				17, ascii,
				-1 );

		if (!reset)
		{
			if (!gtk_tree_model_iter_next
			    (GTK_TREE_MODEL (memview_store), &iter))
			{
				gtk_tree_store_append (memview_store, &iter, NULL);	// aquire iter
			}
		}
	}
}

static int memViewEvntSrcID = 0;
static std::list <memViewWin_t*> memViewWinList;


static GtkWidget *CreateMemViewMenubar (GtkWidget * window)
{
	GtkWidget *menubar, *menu, *item;

	menubar = gtk_menu_bar_new ();

	item = gtk_menu_item_new_with_label ("File");

	gtk_menu_shell_append (GTK_MENU_SHELL (menubar), item);

	menu = gtk_menu_new ();

	gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);

	item = gtk_menu_item_new_with_label ("Load Watch");

	//g_signal_connect (item, "activate", G_CALLBACK (loadRamWatchCB), NULL);

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

	item = gtk_menu_item_new_with_label ("Save Watch");

	//g_signal_connect (item, "activate", G_CALLBACK (saveRamWatchCB), NULL);

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

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

//static void memview_cell_edit_cb( int row, int col

static void memview_cell_edited_start_cb1 (GtkCellRenderer * renderer, GtkCellEditable * editable, gchar * path, memViewWin_t * mv)
{
	//printf("MemView Edit Start: '%s':%li\n", path,  (long)user_data);
	mv->editRowIdx = atoi (path);
	mv->editColIdx = 1;
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

	main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

	menubar = CreateMemViewMenubar (mv->win);

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

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer, "family", "MonoSpace", NULL);
	column = gtk_tree_view_column_new_with_attributes ("Addr", renderer,
							   "text", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (mv->tree), column);

	for (int i=0; i<16; i++)
	{
		char title[16];

		sprintf( title, "%02X", i );

		renderer = gtk_cell_renderer_text_new ();
		g_object_set (renderer, "family", "MonoSpace", NULL);
		g_object_set (renderer, "editable", TRUE, NULL);
		//g_signal_connect (renderer, "edited",
		//		  (GCallback) ramWatch_cell_edited_cb, (gpointer) mv);
		g_signal_connect (renderer, "editing-started",
				  (GCallback) memview_cell_edited_start_cb1,
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

	gtk_box_pack_start (GTK_BOX
			    (gtk_dialog_get_content_area
			     (GTK_DIALOG (mv->win))), main_vbox, TRUE, TRUE,
			    0);

   mv->vbar = gtk_scrolled_window_get_vscrollbar( GTK_SCROLLED_WINDOW(scroll) );

	if (memViewEvntSrcID == 0)
	{
		memViewEvntSrcID =
			g_timeout_add (100, updateMemViewTree, NULL);
	}

	g_signal_connect (mv->win, "delete-event",
			  G_CALLBACK (closeMemoryViewWindow), mv);
	g_signal_connect (mv->win, "response",
			  G_CALLBACK (closeMemoryViewWindow), mv);

	gtk_widget_show_all (mv->win);

	mv->showMemViewResults(1);

	printf("VBAR: %p \n", mv->vbar );
}
