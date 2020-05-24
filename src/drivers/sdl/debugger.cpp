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
#include "../../x6502.h"
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
	GtkWidget *P_N_chkbox;
	GtkWidget *P_V_chkbox;
	GtkWidget *P_U_chkbox;
	GtkWidget *P_B_chkbox;
	GtkWidget *P_D_chkbox;
	GtkWidget *P_I_chkbox;
	GtkWidget *P_Z_chkbox;
	GtkWidget *P_C_chkbox;
	GtkWidget *stack_frame;
	GtkWidget *ppu_label;
	GtkWidget *sprite_label;
	GtkWidget *scanline_label;
	GtkWidget *pixel_label;
	GtkWidget *cpu_label1;
	GtkWidget *cpu_label2;
	GtkWidget *instr_label1;
	GtkWidget *instr_label2;
	GtkWidget *bp_start_entry;
	GtkWidget *bp_end_entry;
	GtkWidget *bp_read_chkbox;
	GtkWidget *bp_write_chkbox;
	GtkWidget *bp_execute_chkbox;
	GtkWidget *bp_enable_chkbox;
	GtkWidget *bp_forbid_chkbox;
	GtkWidget *bp_cond_entry;
	GtkWidget *bp_name_entry;
	GtkWidget *cpu_radio_btn;
	GtkWidget *ppu_radio_btn;
	GtkWidget *sprite_radio_btn;

	int  dialog_op;

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
		bp_start_entry = NULL;
		bp_end_entry = NULL;
		bp_cond_entry = NULL;
		bp_name_entry = NULL;
		bp_read_chkbox = NULL;
		bp_write_chkbox = NULL;
		bp_execute_chkbox = NULL;
		bp_enable_chkbox = NULL;
		bp_forbid_chkbox = NULL;
	   cpu_radio_btn = NULL;
	   ppu_radio_btn = NULL;
	   sprite_radio_btn = NULL;
		stack_frame = NULL;
		P_N_chkbox = NULL;
		P_V_chkbox = NULL;
		P_U_chkbox = NULL;
		P_B_chkbox = NULL;
		P_D_chkbox = NULL;
		P_I_chkbox = NULL;
		P_Z_chkbox = NULL;
		P_C_chkbox = NULL;
		dialog_op = 0;
	}
	
	~debuggerWin_t(void)
	{
	
	}

	void  bpListUpdate(void);
	void  updateViewPort(void);
	void  updateRegisterView(void);
};

static std::list <debuggerWin_t*> debuggerWinList;


void debuggerWin_t::bpListUpdate(void)
{
	GtkTreeIter iter;
	char line[256], addrStr[32], flags[16];

	gtk_tree_store_clear( bp_store );

	for (int i=0; i<numWPs; i++)
	{
		gtk_tree_store_append (bp_store, &iter, NULL);	// aquire iter

		if ( watchpoint[i].endaddress > 0 )
		{
			sprintf( addrStr, "$%04X-%04X:", watchpoint[i].address, watchpoint[i].endaddress );
		}
		else
		{
			sprintf( addrStr, "$%04X:", watchpoint[i].address );
		}

		flags[0] = (watchpoint[i].flags & WP_E) ? 'E' : '-';

		if ( watchpoint[i].flags & BT_P )
		{
			flags[1] = 'P';
		}
		else if ( watchpoint[i].flags & BT_S )
		{
			flags[1] = 'S';
		}
		else
		{
			flags[1] = 'C';
		}

		flags[2] = (watchpoint[i].flags & WP_R) ? 'R' : '-';
		flags[3] = (watchpoint[i].flags & WP_W) ? 'W' : '-';
		flags[4] = (watchpoint[i].flags & WP_X) ? 'X' : '-';
		flags[5] = (watchpoint[i].flags & WP_F) ? 'F' : '-';
		flags[6] = 0;

		strcpy( line, addrStr );
		strcat( line, flags   );

		if (watchpoint[i].desc )
		{
			strcat( line, " ");
			strcat( line, watchpoint[i].desc);
			strcat( line, " ");
		}

		if (watchpoint[i].condText )
		{
			strcat( line, " Condition:");
			strcat( line, watchpoint[i].condText);
			strcat( line, " ");
		}
		gtk_tree_store_set( bp_store, &iter, 0, line, -1 );
	}

}

void  debuggerWin_t::updateRegisterView(void)
{
	int stackPtr;
	char stmp[32];
	std::string  stackLine;

	sprintf( stmp, "%04X", X.PC );

	gtk_entry_set_text( GTK_ENTRY(pc_entry), stmp );

	sprintf( stmp, "%02X", X.A );

	gtk_entry_set_text( GTK_ENTRY(A_entry), stmp );

	sprintf( stmp, "%02X", X.X );

	gtk_entry_set_text( GTK_ENTRY(X_entry), stmp );

	sprintf( stmp, "%02X", X.Y );

	gtk_entry_set_text( GTK_ENTRY(Y_entry), stmp );

	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( P_N_chkbox ), (X.P & N_FLAG) ? TRUE : FALSE );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( P_V_chkbox ), (X.P & V_FLAG) ? TRUE : FALSE );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( P_U_chkbox ), (X.P & U_FLAG) ? TRUE : FALSE );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( P_B_chkbox ), (X.P & B_FLAG) ? TRUE : FALSE );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( P_D_chkbox ), (X.P & D_FLAG) ? TRUE : FALSE );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( P_I_chkbox ), (X.P & I_FLAG) ? TRUE : FALSE );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( P_Z_chkbox ), (X.P & Z_FLAG) ? TRUE : FALSE );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( P_C_chkbox ), (X.P & C_FLAG) ? TRUE : FALSE );

	stackPtr = X.S | 0x0100;

	sprintf( stmp, "Stack: %04X", stackPtr );
	gtk_frame_set_label( GTK_FRAME(stack_frame), stmp );

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

	gtk_text_buffer_set_text( stackTextBuf, stackLine.c_str(), -1 ) ;

}

void  debuggerWin_t::updateViewPort(void)
{
	updateRegisterView();

}

static void handleDialogResponse (GtkWidget * w, gint response_id, debuggerWin_t * dw)
{
	//printf("Response %i\n", response_id ); 

	if ( response_id == GTK_RESPONSE_OK )
	{
		const char *txt;

		////printf("Reponse OK\n");
		switch ( dw->dialog_op )
		{
			case 1: // Breakpoint Add
			{
				int  start_addr = -1, end_addr = -1, type = 0, enable = 1;;
				const char *name; 
				const char *cond;

				txt = gtk_entry_get_text( GTK_ENTRY( dw->bp_start_entry ) );

				if ( txt[0] != 0 )
				{
					start_addr = strtol( txt, NULL, 16 );
				}

				txt = gtk_entry_get_text( GTK_ENTRY( dw->bp_end_entry ) );

				if ( txt[0] != 0 )
				{
					end_addr = strtol( txt, NULL, 16 );
				}

				if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( dw->cpu_radio_btn ) ) )
				{
					type |= BT_C;
				}
				else if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( dw->ppu_radio_btn ) ) )
				{
					type |= BT_P;
				}
				else if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( dw->sprite_radio_btn ) ) )
				{
					type |= BT_S;
				}

				if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( dw->bp_read_chkbox ) ) )
				{
					type |= WP_R;
				}

				if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( dw->bp_write_chkbox ) ) )
				{
					type |= WP_W;
				}

				if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( dw->bp_execute_chkbox ) ) )
				{
					type |= WP_X;
				}

				//this overrides all
				if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( dw->bp_forbid_chkbox ) ) )
				{
					type  = WP_F;
				}

				enable = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( dw->bp_enable_chkbox ) );

				name = gtk_entry_get_text( GTK_ENTRY( dw->bp_name_entry ) );
				cond = gtk_entry_get_text( GTK_ENTRY( dw->bp_cond_entry ) );

				if ( (start_addr >= 0) && (numWPs < 64) )
				{
					unsigned int retval;

					retval = NewBreak( name, start_addr, end_addr, type, cond, numWPs, enable);

					if ( (retval == 1) || (retval == 2) )
					{
						printf("Breakpoint Add Failed\n");
					}
					else
					{
						numWPs++;

						dw->bpListUpdate();
					}
				}
			}
			break;
		}


		//if ( txt != NULL )
		//{
		//	//printf("Text: '%s'\n", txt );
		//	switch (mv->dialog_op)
		//	{
		//		case 1:
		//			if ( isxdigit(txt[0]) )
		//			{  // Address is always treated as hex number
		//				mv->gotoLocation( strtol( txt, NULL, 16 ) );
		//			}
		//		break;
		//		case 2:
		//			if ( isdigit(txt[0]) )
		//			{  // Value is numerical
		//				mv->writeMem( mv->selAddr, strtol( txt, NULL, 0 ) );
		//			}
		//			else if ( (txt[0] == '\'') && (txt[2] == '\'') )
		//			{  // Byte to be written is expressed as ASCII character
		//				mv->writeMem( mv->selAddr, txt[1] );
		//			}
		//		break;
		//	}
		//}
	}
	//else if ( response_id == GTK_RESPONSE_CANCEL )
	//{
	//	printf("Reponse Cancel\n");
	//}
	gtk_widget_destroy (w);

}

static void closeDialogWindow (GtkWidget * w, GdkEvent * e, debuggerWin_t * dw)
{
	gtk_widget_destroy (w);
}

static void create_breakpoint_dialog( int index, debuggerWin_t * dw )
{
	GtkWidget *win;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *vbox1;
	GtkWidget *label;
	GtkWidget *frame;
	GtkWidget *grid;

	if ( index < 0 )
	{
		win = gtk_dialog_new_with_buttons ("Add Breakpoint",
						       GTK_WINDOW (dw->win),
						       (GtkDialogFlags)
						       (GTK_DIALOG_DESTROY_WITH_PARENT),
						       "_Cancel", GTK_RESPONSE_CANCEL,
						       "_Add", GTK_RESPONSE_OK, NULL);
	}
	else
	{
		win = gtk_dialog_new_with_buttons ("Edit Breakpoint",
						       GTK_WINDOW (dw->win),
						       (GtkDialogFlags)
						       (GTK_DIALOG_DESTROY_WITH_PARENT),
						       "_Cancel", GTK_RESPONSE_CANCEL,
						       "_Edit", GTK_RESPONSE_OK, NULL);
	}

	gtk_dialog_set_default_response( GTK_DIALOG(win), GTK_RESPONSE_OK );

	vbox1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
	hbox  = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);

	// Adress entry fields
	label = gtk_label_new("Address:");
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);

	dw->bp_start_entry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (dw->bp_start_entry), 4);
	gtk_entry_set_width_chars (GTK_ENTRY (dw->bp_start_entry), 4);

	gtk_box_pack_start (GTK_BOX (hbox), dw->bp_start_entry, FALSE, FALSE, 2);

	label = gtk_label_new("-");
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);

	dw->bp_end_entry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (dw->bp_end_entry), 4);
	gtk_entry_set_width_chars (GTK_ENTRY (dw->bp_end_entry), 4);

	gtk_box_pack_start (GTK_BOX (hbox), dw->bp_end_entry, FALSE, FALSE, 2);

	dw->bp_forbid_chkbox = gtk_check_button_new_with_label("Forbid");
	gtk_box_pack_start (GTK_BOX (hbox), dw->bp_forbid_chkbox, FALSE, FALSE, 2);

	gtk_box_pack_start (GTK_BOX (vbox1), hbox, FALSE, FALSE, 2);

	// flags frame
	frame = gtk_frame_new ("");
	gtk_box_pack_start (GTK_BOX (vbox1), frame, FALSE, FALSE, 2);

	vbox  = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
	hbox  = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);

	dw->bp_read_chkbox    = gtk_check_button_new_with_label("Read");
	dw->bp_write_chkbox   = gtk_check_button_new_with_label("Write");
	dw->bp_execute_chkbox = gtk_check_button_new_with_label("Execute");
	dw->bp_enable_chkbox  = gtk_check_button_new_with_label("Enable");

	gtk_box_pack_start (GTK_BOX (hbox), dw->bp_read_chkbox   , FALSE, FALSE, 2);
	gtk_box_pack_start (GTK_BOX (hbox), dw->bp_write_chkbox  , FALSE, FALSE, 2);
	gtk_box_pack_start (GTK_BOX (hbox), dw->bp_execute_chkbox, FALSE, FALSE, 2);
	gtk_box_pack_start (GTK_BOX (hbox), dw->bp_enable_chkbox , FALSE, FALSE, 2);

	gtk_box_pack_start (GTK_BOX (vbox), hbox , FALSE, FALSE, 2);

	gtk_container_add (GTK_CONTAINER (frame), vbox);

	frame = gtk_frame_new ("Memory");
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 2);

	hbox  = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 3);

	dw->cpu_radio_btn    = gtk_radio_button_new_with_label (NULL, "CPU");
	dw->ppu_radio_btn    = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON(dw->cpu_radio_btn), "PPU");
	dw->sprite_radio_btn = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON(dw->cpu_radio_btn), "Sprite");

	gtk_box_pack_start (GTK_BOX (hbox), dw->cpu_radio_btn   , TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (hbox), dw->ppu_radio_btn   , TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (hbox), dw->sprite_radio_btn, TRUE, TRUE, 2);

	gtk_container_add (GTK_CONTAINER (frame), hbox);

	grid = gtk_grid_new();

	gtk_grid_set_row_homogeneous( GTK_GRID(grid), TRUE );
	gtk_grid_set_column_homogeneous( GTK_GRID(grid), TRUE );
	gtk_grid_set_row_spacing( GTK_GRID(grid), 5 );
	gtk_grid_set_column_spacing( GTK_GRID(grid), 10 );

	label = gtk_label_new("Condition:");
	gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1 );

	label = gtk_label_new("Name:");
	gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1 );

	dw->bp_cond_entry = gtk_entry_new ();
	dw->bp_name_entry = gtk_entry_new ();

	gtk_grid_attach (GTK_GRID (grid), dw->bp_cond_entry, 1, 0, 3, 1 );
	gtk_grid_attach (GTK_GRID (grid), dw->bp_name_entry, 1, 1, 3, 1 );

	gtk_box_pack_start (GTK_BOX (vbox1), grid, FALSE, FALSE, 2);

	gtk_box_pack_start (GTK_BOX
			    (gtk_dialog_get_content_area
			     (GTK_DIALOG (win))), vbox1, TRUE, TRUE, 1);

	gtk_widget_show_all (win);

	g_signal_connect (win, "delete-event",
			  G_CALLBACK (closeDialogWindow), dw);
	g_signal_connect (win, "response",
			  G_CALLBACK (handleDialogResponse), dw);

}

static void addBreakpointCB (GtkButton * button, debuggerWin_t * dw)
{
	dw->dialog_op = 1;

	create_breakpoint_dialog( -1, dw );
}

//this code enters the debugger when a breakpoint was hit
void FCEUD_DebugBreakpoint(int bp_num)
{

	//std::list < debuggerWin_t * >::iterator it;

	//for (it = debuggerWinList.begin (); it != debuggerWinList.end (); it++)
	//{

	//}

}

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

	gtk_box_pack_start (GTK_BOX (hbox), dw->pc_entry, TRUE, TRUE, 2);

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
	dw->stack_frame = gtk_frame_new ("Stack");

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

	gtk_container_add (GTK_CONTAINER (dw->stack_frame), scroll);

	gtk_box_pack_start (GTK_BOX (vbox2), dw->stack_frame, TRUE, TRUE, 2);
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

	g_signal_connect (button, "clicked",
			  G_CALLBACK (addBreakpointCB), (gpointer) dw);

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

	dw->P_N_chkbox = gtk_check_button_new_with_label("N");
	gtk_grid_attach( GTK_GRID(grid), dw->P_N_chkbox, 0, 0, 1, 1 );

	dw->P_V_chkbox = gtk_check_button_new_with_label("V");
	gtk_grid_attach( GTK_GRID(grid), dw->P_V_chkbox, 1, 0, 1, 1 );

	dw->P_U_chkbox = gtk_check_button_new_with_label("U");
	gtk_grid_attach( GTK_GRID(grid), dw->P_U_chkbox, 2, 0, 1, 1 );

	dw->P_B_chkbox = gtk_check_button_new_with_label("B");
	gtk_grid_attach( GTK_GRID(grid), dw->P_B_chkbox, 3, 0, 1, 1 );

	dw->P_D_chkbox = gtk_check_button_new_with_label("D");
	gtk_grid_attach( GTK_GRID(grid), dw->P_D_chkbox, 0, 1, 1, 1 );

	dw->P_I_chkbox = gtk_check_button_new_with_label("I");
	gtk_grid_attach( GTK_GRID(grid), dw->P_I_chkbox, 1, 1, 1, 1 );

	dw->P_Z_chkbox = gtk_check_button_new_with_label("Z");
	gtk_grid_attach( GTK_GRID(grid), dw->P_Z_chkbox, 2, 1, 1, 1 );

	dw->P_C_chkbox = gtk_check_button_new_with_label("C");
	gtk_grid_attach( GTK_GRID(grid), dw->P_C_chkbox, 3, 1, 1, 1 );

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

	dw->updateViewPort();

   return;
}
