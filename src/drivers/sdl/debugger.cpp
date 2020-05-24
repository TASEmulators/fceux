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
#include "debugger.h"

extern Config *g_config;

//*******************************************************************************************************
// Debugger Window
//*******************************************************************************************************
//

struct debuggerWin_t
{
	GtkWidget *win;
	GtkTextView *textview;
	GtkTextBuffer *textbuf;
	GtkTextView *stackview;
	GtkTextBuffer *stackTextBuf;
	GtkTreeStore *bp_store;
	GtkTreeStore *bkm_store;
	GtkWidget *bp_tree;
	GtkWidget *bkm_tree;
	GtkWidget *pc_entry;
	GtkWidget *A_entry;
	GtkWidget *X_entry;
	GtkWidget *Y_entry;
	GtkWidget *ppu_label;
	GtkWidget *sprite_label;
	GtkWidget *scanline_label;
	GtkWidget *pixel_label;
	GtkWidget *cpu_label1;
	GtkWidget *cpu_label2;
	GtkWidget *instr_label1;
	GtkWidget *instr_label2;

	debuggerWin_t(void)
	{
		win = NULL;
		textview = NULL;
		textbuf = NULL;
		bp_store = NULL;
		bp_tree = NULL;
		stackview = NULL;
		stackTextBuf = NULL;
		ppu_label = NULL;
		sprite_label = NULL;
		scanline_label = NULL;
		pixel_label = NULL;
		cpu_label1 = NULL;
		cpu_label2 = NULL;
		instr_label1 = NULL;
		instr_label2 = NULL;
	}
	
	~debuggerWin_t(void)
	{
	
	}
};

static std::list <debuggerWin_t*> debuggerWinList;

static void closeDebuggerWindow (GtkWidget * w, GdkEvent * e, debuggerWin_t * dw)
{
	std::list < debuggerWin_t * >::iterator it;

	for (it = debuggerWinList.begin (); it != debuggerWinList.end (); it++)
	{
		if (dw == *it)
		{
			//printf("Removing MemView Window %p from List\n", cw);
			debuggerWinList.erase (it);
			break;
		}
	}

	delete dw;

	gtk_widget_destroy (w);
}

void openDebuggerWindow (void)
{
	GtkWidget *main_hbox;
	GtkWidget *vbox1, *vbox2, *vbox3;
	GtkWidget *hbox1, *hbox2;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *button;
	GtkWidget *scroll;
	GtkWidget *frame;
	GtkWidget *entry;
	GtkWidget *grid;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	debuggerWin_t *dw;

	dw = new debuggerWin_t;

	debuggerWinList.push_back (dw);

	dw->win = gtk_dialog_new_with_buttons ("6502 Debugger",
						GTK_WINDOW (MainWindow),
						(GtkDialogFlags)
						(GTK_DIALOG_DESTROY_WITH_PARENT),
						"_Close", GTK_RESPONSE_OK,
						NULL);

	gtk_window_set_default_size (GTK_WINDOW (dw->win), 800, 800);

	main_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);

	dw->textview = (GtkTextView*) gtk_text_view_new();

	gtk_text_view_set_monospace( dw->textview, TRUE );
	gtk_text_view_set_overwrite( dw->textview, TRUE );
	gtk_text_view_set_editable( dw->textview, FALSE );
	gtk_text_view_set_wrap_mode( dw->textview, GTK_WRAP_NONE );
	gtk_text_view_set_cursor_visible( dw->textview, TRUE );

	dw->textbuf = gtk_text_view_get_buffer( dw->textview );

	scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
					GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
	gtk_container_add (GTK_CONTAINER (scroll), GTK_WIDGET(dw->textview) );

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 1);
	gtk_box_pack_start (GTK_BOX (vbox), scroll, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (main_hbox), vbox, TRUE, TRUE, 2);

	vbox1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 3);
	hbox1 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);

	gtk_box_pack_start (GTK_BOX (vbox1), hbox1, TRUE, TRUE, 2);

	/*
	 *  Vertical box 2 contains:
	 *     1. "Run" and "Step Into" Buttons
	 *     2. "Step Out" and "Step Over" Buttons
	 *     3. "Run Line" and "128 Lines" Buttons
	 *     4. "Seek To:" button and Address Entry Field
	 *     5. PC Entry Field and "Seek PC" Button
	 *     6. A, X, Y Register Entry Fields
	 */
	vbox2  = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
	hbox   = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);

	gtk_box_pack_start (GTK_BOX (hbox1), vbox2, TRUE, TRUE, 2);

	grid = gtk_grid_new();

	gtk_grid_set_row_homogeneous( GTK_GRID(grid), TRUE );
	gtk_grid_set_column_homogeneous( GTK_GRID(grid), TRUE );
	gtk_grid_set_row_spacing( GTK_GRID(grid), 5 );
	gtk_grid_set_column_spacing( GTK_GRID(grid), 10 );

	gtk_grid_insert_column( GTK_GRID(grid), 0 );
	gtk_grid_insert_column( GTK_GRID(grid), 0 );

	for (int i=0; i<5;i++)
	{
		gtk_grid_insert_row( GTK_GRID(grid), 0 );
	}
	gtk_box_pack_start (GTK_BOX (hbox ), grid, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 2);

	//     Row 1
	button = gtk_button_new_with_label ("Run");

	gtk_grid_attach( GTK_GRID(grid), button, 0, 0, 1, 1 );

	button = gtk_button_new_with_label ("Step Into");

	gtk_grid_attach( GTK_GRID(grid), button, 1, 0, 1, 1 );

	//     Row 2
	button = gtk_button_new_with_label ("Step Out");

	gtk_grid_attach( GTK_GRID(grid), button, 0, 1, 1, 1 );

	button = gtk_button_new_with_label ("Step Over");

	gtk_grid_attach( GTK_GRID(grid), button, 1, 1, 1, 1 );

	//     Row 3
	button = gtk_button_new_with_label ("Run Line");

	gtk_grid_attach( GTK_GRID(grid), button, 0, 2, 1, 1 );

	button = gtk_button_new_with_label ("128 Lines");

	gtk_grid_attach( GTK_GRID(grid), button, 1, 2, 1, 1 );

	//     Row 4
	button = gtk_button_new_with_label ("Seek To:");

	gtk_grid_attach( GTK_GRID(grid), button, 0, 3, 1, 1 );

	entry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (entry), 4);
	gtk_entry_set_width_chars (GTK_ENTRY (entry), 4);

	gtk_grid_attach( GTK_GRID(grid), entry, 1, 3, 1, 1 );

	//     Row 5
	hbox  = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);

	label = gtk_label_new("PC:");
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);

	dw->pc_entry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (dw->pc_entry), 4);
	gtk_entry_set_width_chars (GTK_ENTRY (dw->pc_entry), 4);

	gtk_box_pack_start (GTK_BOX (hbox), dw->pc_entry, FALSE, FALSE, 2);

	gtk_grid_attach( GTK_GRID(grid), hbox, 0, 4, 1, 1 );

	button = gtk_button_new_with_label ("Seek PC");

	gtk_grid_attach( GTK_GRID(grid), button, 1, 4, 1, 1 );

	//     Row 6
	grid = gtk_grid_new();

	gtk_grid_set_row_homogeneous( GTK_GRID(grid), TRUE );
	gtk_grid_set_column_homogeneous( GTK_GRID(grid), TRUE );
	gtk_grid_set_row_spacing( GTK_GRID(grid), 5 );
	gtk_grid_set_column_spacing( GTK_GRID(grid), 10 );

	hbox  = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);

	label = gtk_label_new("A:");
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);

	dw->A_entry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (dw->A_entry), 2);
	gtk_entry_set_width_chars (GTK_ENTRY (dw->A_entry), 2);

	gtk_box_pack_start (GTK_BOX (hbox), dw->A_entry, FALSE, FALSE, 2);

	gtk_grid_attach( GTK_GRID(grid), hbox, 0, 0, 1, 1 );

	hbox  = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);

	label = gtk_label_new("X:");
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);

	dw->X_entry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (dw->X_entry), 2);
	gtk_entry_set_width_chars (GTK_ENTRY (dw->X_entry), 2);

	gtk_box_pack_start (GTK_BOX (hbox), dw->X_entry, FALSE, FALSE, 2);

	gtk_grid_attach( GTK_GRID(grid), hbox, 1, 0, 1, 1 );

	hbox  = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);

	label = gtk_label_new("Y:");
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);

	dw->Y_entry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (dw->Y_entry), 2);
	gtk_entry_set_width_chars (GTK_ENTRY (dw->Y_entry), 2);

	gtk_box_pack_start (GTK_BOX (hbox), dw->Y_entry, FALSE, FALSE, 2);

	gtk_grid_attach( GTK_GRID(grid), hbox, 2, 0, 1, 1 );

	gtk_box_pack_start (GTK_BOX (vbox2), grid, FALSE, FALSE, 2);

	// Stack Frame
	frame = gtk_frame_new ("Stack");

	dw->stackview = (GtkTextView*) gtk_text_view_new();

	gtk_text_view_set_monospace( dw->stackview, TRUE );
	gtk_text_view_set_overwrite( dw->stackview, TRUE );
	gtk_text_view_set_editable( dw->stackview, FALSE );
	gtk_text_view_set_wrap_mode( dw->stackview, GTK_WRAP_NONE );
	gtk_text_view_set_cursor_visible( dw->stackview, TRUE );

	dw->stackTextBuf = gtk_text_view_get_buffer( dw->stackview );

	scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
					GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
	gtk_container_add (GTK_CONTAINER (scroll), GTK_WIDGET(dw->stackview) );

	gtk_container_add (GTK_CONTAINER (frame), scroll);

	gtk_box_pack_start (GTK_BOX (vbox2), frame, TRUE, TRUE, 2);
	/*
	 *  Vertical box 3 contains:
	 *     1. Breakpoints Frame
	 *     2. Status Flags Frame
	 */
	vbox3  = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

	gtk_box_pack_start (GTK_BOX (hbox1), vbox3, TRUE, TRUE, 2);

	/*
	 *  Breakpoints Frame contains:
	 *     1. Breakpoints tree view
	 *     2. Add, delete, edit breakpoint button row.
	 *     3. Break on bad opcodes checkbox/label
	 */
	frame = gtk_frame_new ("Breakpoints");

	vbox  = gtk_box_new (GTK_ORIENTATION_VERTICAL, 3);

	gtk_container_add (GTK_CONTAINER (frame), vbox);

	gtk_box_pack_start (GTK_BOX (vbox3), frame, TRUE, TRUE, 2);

	dw->bp_store =
		gtk_tree_store_new (1, G_TYPE_STRING );

	dw->bp_tree =
		gtk_tree_view_new_with_model (GTK_TREE_MODEL
					      (dw->bp_store));

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer, "family", "MonoSpace", NULL);
	column = gtk_tree_view_column_new_with_attributes ("Breakpoint", renderer,
							   "text", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (dw->bp_tree), column);

	scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
					GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (scroll), dw->bp_tree);
	gtk_box_pack_start (GTK_BOX (vbox), scroll, TRUE, TRUE, 1);

	hbox  = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 3);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 1);

	button = gtk_button_new_with_label ("Add");

	gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 2);

	button = gtk_button_new_with_label ("Delete");

	gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 2);

	button = gtk_button_new_with_label ("Edit");

	gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 2);

	button = gtk_check_button_new_with_label("Break on Bad Opcodes");

	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 1);

	// Status Flags Frame
	frame = gtk_frame_new ("Status Flags");

	grid = gtk_grid_new();

	gtk_grid_set_row_homogeneous( GTK_GRID(grid), TRUE );
	gtk_grid_set_column_homogeneous( GTK_GRID(grid), TRUE );
	gtk_grid_set_row_spacing( GTK_GRID(grid), 5 );
	gtk_grid_set_column_spacing( GTK_GRID(grid), 10 );

	button = gtk_check_button_new_with_label("N");
	gtk_grid_attach( GTK_GRID(grid), button, 0, 0, 1, 1 );

	button = gtk_check_button_new_with_label("V");
	gtk_grid_attach( GTK_GRID(grid), button, 1, 0, 1, 1 );

	button = gtk_check_button_new_with_label("U");
	gtk_grid_attach( GTK_GRID(grid), button, 2, 0, 1, 1 );

	button = gtk_check_button_new_with_label("B");
	gtk_grid_attach( GTK_GRID(grid), button, 3, 0, 1, 1 );

	button = gtk_check_button_new_with_label("D");
	gtk_grid_attach( GTK_GRID(grid), button, 0, 1, 1, 1 );

	button = gtk_check_button_new_with_label("I");
	gtk_grid_attach( GTK_GRID(grid), button, 1, 1, 1, 1 );

	button = gtk_check_button_new_with_label("Z");
	gtk_grid_attach( GTK_GRID(grid), button, 2, 1, 1, 1 );

	button = gtk_check_button_new_with_label("C");
	gtk_grid_attach( GTK_GRID(grid), button, 3, 1, 1, 1 );

	gtk_container_add (GTK_CONTAINER (frame), grid);

	gtk_box_pack_start (GTK_BOX (vbox3), frame, FALSE, FALSE, 2);

	/*
	 *  End Vertical Box 3
	 */

	/*
	 *  Start PPU Frame
	 */
	grid = gtk_grid_new();

	gtk_grid_set_row_homogeneous( GTK_GRID(grid), TRUE );
	gtk_grid_set_column_homogeneous( GTK_GRID(grid), TRUE );
	gtk_grid_set_row_spacing( GTK_GRID(grid), 5 );
	gtk_grid_set_column_spacing( GTK_GRID(grid), 10 );

	frame = gtk_frame_new ("");

	hbox  = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
	vbox  = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);

	dw->ppu_label      = gtk_label_new("PPU:");
	dw->sprite_label   = gtk_label_new("Sprite:");
	dw->scanline_label = gtk_label_new("Scanline:");
	dw->pixel_label    = gtk_label_new("Pixel:");

	gtk_box_pack_start (GTK_BOX (vbox), dw->ppu_label     , FALSE, FALSE, 2);
	gtk_box_pack_start (GTK_BOX (vbox), dw->sprite_label  , FALSE, FALSE, 2);
	gtk_box_pack_start (GTK_BOX (vbox), dw->scanline_label, FALSE, FALSE, 2);
	gtk_box_pack_start (GTK_BOX (vbox), dw->pixel_label   , FALSE, FALSE, 2);

	gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 2);
	gtk_container_add (GTK_CONTAINER (frame), hbox);

	gtk_grid_attach( GTK_GRID(grid), frame, 0, 0, 1, 4 );
	/*
	 *  End PPU Frame
	 */

	/*
	 *  Start Cycle Break Block
	 */
	dw->cpu_label1      = gtk_label_new("CPU Cycles:");
	dw->cpu_label2      = gtk_label_new("+(0)");

	dw->instr_label1      = gtk_label_new("Instructions:");
	dw->instr_label2      = gtk_label_new("+(0)");

	gtk_grid_attach( GTK_GRID(grid), dw->cpu_label1, 1, 0, 1, 1 );
	gtk_grid_attach( GTK_GRID(grid), dw->cpu_label2, 2, 0, 1, 1 );
	gtk_grid_attach( GTK_GRID(grid), dw->instr_label1, 1, 2, 1, 1 );
	gtk_grid_attach( GTK_GRID(grid), dw->instr_label2, 2, 2, 1, 1 );

	button = gtk_check_button_new_with_label("Break when exceed");
	gtk_grid_attach( GTK_GRID(grid), button, 1, 1, 1, 1 );
	entry = gtk_entry_new ();
	gtk_grid_attach( GTK_GRID(grid), entry, 2, 1, 1, 1 );

	button = gtk_check_button_new_with_label("Break when exceed");
	gtk_grid_attach( GTK_GRID(grid), button, 1, 3, 1, 1 );
	entry = gtk_entry_new ();
	gtk_grid_attach( GTK_GRID(grid), entry, 2, 3, 1, 1 );

	gtk_box_pack_start (GTK_BOX (vbox1), grid, FALSE, FALSE, 2);
	/*
	 *  End Cycle Break Block
	 */

	/*
	 *  Start Address Bookmarks
	 */
	hbox2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);

	frame = gtk_frame_new ("Address Bookmarks");

	vbox  = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
	hbox  = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 1);

	gtk_container_add (GTK_CONTAINER (frame), vbox);

	dw->bkm_store =
		gtk_tree_store_new (1, G_TYPE_STRING );

	dw->bkm_tree =
		gtk_tree_view_new_with_model (GTK_TREE_MODEL
					      (dw->bkm_store));

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer, "family", "MonoSpace", NULL);
	column = gtk_tree_view_column_new_with_attributes ("Bookmarks", renderer,
							   "text", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (dw->bkm_tree), column);

	scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
					GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (scroll), dw->bkm_tree);
	gtk_box_pack_start (GTK_BOX (hbox), scroll, TRUE, TRUE, 1);

	vbox  = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 1);
	entry = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 2);
	button = gtk_button_new_with_label ("Add");
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 2);
	button = gtk_button_new_with_label ("Delete");
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 2);
	button = gtk_button_new_with_label ("Name");
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 2);

	gtk_box_pack_start (GTK_BOX (hbox2), frame, TRUE, TRUE, 2);
	
	vbox  = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
	gtk_box_pack_start (GTK_BOX (hbox2), vbox, FALSE, FALSE, 2);

	button = gtk_button_new_with_label ("Reset Counters");
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 2);
	button = gtk_check_button_new_with_label("ROM Offsets");
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 2);
	button = gtk_check_button_new_with_label("Symbolic Debug");
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 2);
	button = gtk_check_button_new_with_label("Register Names");
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 2);
	button = gtk_button_new_with_label ("Reload Symbols");
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 2);
	button = gtk_button_new_with_label ("ROM Patcher");
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 2);

	gtk_box_pack_start (GTK_BOX (vbox1), hbox2, TRUE, TRUE, 2);
	/*
	 *  End Address Bookmarks
	 */
	gtk_box_pack_start (GTK_BOX (main_hbox), vbox1, TRUE, TRUE, 2);

	gtk_box_pack_start (GTK_BOX
			    (gtk_dialog_get_content_area
			     (GTK_DIALOG (dw->win))), main_hbox, TRUE, TRUE,
			    0);

	g_signal_connect (dw->win, "delete-event",
			  G_CALLBACK (closeDebuggerWindow), dw);
	g_signal_connect (dw->win, "response",
			  G_CALLBACK (closeDebuggerWindow), dw);

	gtk_widget_show_all (dw->win);

   return;
}
