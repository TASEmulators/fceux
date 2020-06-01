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

extern Config *g_config;

static void updateAllActvCheatLists (bool redraw);

//-----------------------------------------
class cheat_win_t
{
      public:

	GtkTreeStore * actv_cheats_store;
	GtkTreeStore *ram_match_store;
	GtkTreeIter actv_cheats_iter;
	GtkTreeIter ram_match_iter;
	int cheat_search_known_value;
	int cheat_search_neq_value;
	int cheat_search_gt_value;
	int cheat_search_lt_value;
	int new_cheat_addr;
	int new_cheat_val;
	int new_cheat_cmp;
	  std::string new_cheat_name;
	bool wasPausedByCheats;
	bool pauseWhileCheatsActv;
	bool actv_cheat_redraw;

	GtkWidget *win;
	GtkWidget *actv_cheat_tree;
	GtkWidget *search_cheat_tree;
	GtkWidget *neq_chkbox;
	GtkWidget *lt_chkbox;
	GtkWidget *gt_chkbox;
	GtkWidget *cheat_name_entry;
	GtkWidget *cheat_addr_entry;
	GtkWidget *cheat_val_entry;
	GtkWidget *cheat_cmp_entry;
	GtkWidget *cheat_del_button;
	GtkWidget *cheat_edit_button;
	GtkWidget *cheat_search_known_btn;
	GtkWidget *cheat_search_eq_btn;
	GtkWidget *cheat_search_neq_btn;
	GtkWidget *cheat_search_gr_btn;
	GtkWidget *cheat_search_lt_btn;

	  cheat_win_t (void)
	{
		win = NULL;
		actv_cheats_store = NULL;
		ram_match_store = NULL;
		cheat_search_known_value = 0;
		cheat_search_neq_value = 0;
		cheat_search_gt_value = 0;
		cheat_search_lt_value = 0;
		new_cheat_addr = -1;
		new_cheat_val = -1;
		new_cheat_cmp = -1;
		wasPausedByCheats = false;
		pauseWhileCheatsActv = false;
		actv_cheat_redraw = true;
		actv_cheat_tree = NULL;
		search_cheat_tree = NULL;
		neq_chkbox = NULL;
		lt_chkbox = NULL;
		gt_chkbox = NULL;
		cheat_name_entry = NULL;
		cheat_addr_entry = NULL;
		cheat_val_entry = NULL;
		cheat_cmp_entry = NULL;
		cheat_del_button = NULL;
		cheat_edit_button = NULL;
		cheat_search_known_btn = NULL;
		cheat_search_eq_btn = NULL;
		cheat_search_neq_btn = NULL;
		cheat_search_gr_btn = NULL;
		cheat_search_lt_btn = NULL;
	}

	void showActiveCheatList (bool reset);
	void showCheatSearchResults (void);
	int  getSelCheatRow(void);
};

static cheat_win_t *curr_cw = NULL;
static std::list < cheat_win_t * >cheatWinList;

//*******************************************************************************************************
// Cheat Window
//*******************************************************************************************************

static int ShowCheatSearchResultsCallB (uint32 a, uint8 last, uint8 current)
{
	char addrStr[32], lastStr[32], curStr[32];

	sprintf (addrStr, "0x%04X ", a);
	sprintf (lastStr, " 0x%02X ", last);
	sprintf (curStr, " 0x%02X ", current);

	gtk_tree_store_append (curr_cw->ram_match_store, &curr_cw->ram_match_iter, NULL);	// aquire iter

	gtk_tree_store_set (curr_cw->ram_match_store, &curr_cw->ram_match_iter,
			    0, addrStr, 1, lastStr, 2, curStr, -1);

	return 1;
}

void cheat_win_t::showCheatSearchResults (void)
{
	int total_matches = 0;

	curr_cw = this;

	gtk_tree_store_clear (ram_match_store);

	total_matches = FCEUI_CheatSearchGetCount ();

	//printf("Cheat Search Matches: %i \n", total_matches );

	FCEUI_CheatSearchGetRange (0, total_matches,
				   ShowCheatSearchResultsCallB);
}

static void cheatSearchReset (GtkButton * button, cheat_win_t * cw)
{
	//printf("Cheat Search Reset!\n");

	//cheat_search_known_value = 0;
	//cheat_search_neq_value = 0;
	//cheat_search_gt_value = 0;
	//cheat_search_lt_value = 0;

	gtk_widget_set_sensitive( cw->cheat_search_known_btn , TRUE );
	gtk_widget_set_sensitive( cw->cheat_search_eq_btn    , TRUE );
	gtk_widget_set_sensitive( cw->cheat_search_neq_btn   , TRUE );
	gtk_widget_set_sensitive( cw->cheat_search_gr_btn    , TRUE );
	gtk_widget_set_sensitive( cw->cheat_search_lt_btn    , TRUE );

	FCEUI_CheatSearchBegin ();
	cw->showCheatSearchResults ();
	// Enable Cheat Search Buttons - Change Sensitivity
}

static void cheatSearchKnown (GtkButton * button, cheat_win_t * cw)
{
	//printf("Cheat Search Known!\n");

	FCEUI_CheatSearchEnd (FCEU_SEARCH_NEWVAL_KNOWN,
			      cw->cheat_search_known_value, 0);
	cw->showCheatSearchResults ();
}

static void cheatSearchEqual (GtkButton * button, cheat_win_t * cw)
{

	//printf("Cheat Search Equal !\n");

	FCEUI_CheatSearchEnd (FCEU_SEARCH_PUERLY_RELATIVE_CHANGE, 0, 0);
	cw->showCheatSearchResults ();
}

static void cheatSearchNotEqual (GtkButton * button, cheat_win_t * cw)
{
	int checked =
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
					      (cw->neq_chkbox));

	//printf("Cheat Search NotEqual %i!\n", checked);

	if (checked)
	{
		FCEUI_CheatSearchEnd (FCEU_SEARCH_PUERLY_RELATIVE_CHANGE, 0,
				      cw->cheat_search_neq_value);
	}
	else
	{
		FCEUI_CheatSearchEnd (FCEU_SEARCH_ANY_CHANGE, 0, 0);
	}
	cw->showCheatSearchResults ();
}

static void cheatSearchGreaterThan (GtkButton * button, cheat_win_t * cw)
{
	int checked =
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
					      (cw->gt_chkbox));

	//printf("Cheat Search GreaterThan %i!\n", checked);

	if (checked)
	{
		FCEUI_CheatSearchEnd (FCEU_SEARCH_NEWVAL_GT_KNOWN, 0,
				      cw->cheat_search_gt_value);
	}
	else
	{
		FCEUI_CheatSearchEnd (FCEU_SEARCH_NEWVAL_GT, 0, 0);
	}
	cw->showCheatSearchResults ();
}

static void cheatSearchLessThan (GtkButton * button, cheat_win_t * cw)
{
	int checked =
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
					      (cw->lt_chkbox));

	//printf("Cheat Search LessThan %i!\n", checked);

	if (checked)
	{
		FCEUI_CheatSearchEnd (FCEU_SEARCH_NEWVAL_LT_KNOWN, 0,
				      cw->cheat_search_lt_value);
	}
	else
	{
		FCEUI_CheatSearchEnd (FCEU_SEARCH_NEWVAL_LT, 0, 0);
	}
	cw->showCheatSearchResults ();
}

static void pauseDuringCheatWinActvCB (GtkToggleButton * button,
				       cheat_win_t * cw)
{
	cw->pauseWhileCheatsActv = gtk_toggle_button_get_active (button);

	if (cw->pauseWhileCheatsActv)
	{
		if (EmulationPaused == 0)
		{
			EmulationPaused = 1;
			cw->wasPausedByCheats = true;
		}
	}
	else
	{
		if (EmulationPaused && cw->wasPausedByCheats)
		{
			EmulationPaused = 0;
		}
		cw->wasPausedByCheats = false;
	}
	FCEU_printf ("Emulation paused: %d\n", EmulationPaused);
}

static void cheatSearchValueEntryCB1 (GtkWidget * widget, cheat_win_t * cw)
{
	long value;
	const gchar *entry_text;
	entry_text = gtk_entry_get_text (GTK_ENTRY (widget));

	value = strtol (entry_text, NULL, 16);

	cw->cheat_search_known_value = value;

	//printf("Cheat Value Entry contents: '%s' Value: 0x%02lx\n", entry_text, value);
}

static void cheatSearchValueEntryCB2 (GtkWidget * widget, cheat_win_t * cw)
{
	long value;
	const gchar *entry_text;
	entry_text = gtk_entry_get_text (GTK_ENTRY (widget));

	value = strtol (entry_text, NULL, 16);

	cw->cheat_search_neq_value = value;

	//printf("Cheat Value Entry contents: '%s' Value: 0x%02lx\n", entry_text, value);
}

static void cheatSearchValueEntryCB3 (GtkWidget * widget, cheat_win_t * cw)
{
	long value;
	const gchar *entry_text;
	entry_text = gtk_entry_get_text (GTK_ENTRY (widget));

	value = strtol (entry_text, NULL, 16);

	cw->cheat_search_gt_value = value;

	//printf("Cheat Value Entry contents: '%s' Value: 0x%02lx\n", entry_text, value);
}

static void cheatSearchValueEntryCB4 (GtkWidget * widget, cheat_win_t * cw)
{
	long value;
	const gchar *entry_text;
	entry_text = gtk_entry_get_text (GTK_ENTRY (widget));

	value = strtol (entry_text, NULL, 16);

	cw->cheat_search_lt_value = value;

	//printf("Cheat Value Entry contents: '%s' Value: 0x%02lx\n", entry_text, value);
}

static int activeCheatListCB (char *name, uint32 a, uint8 v, int c, int s,
			      int type, void *data)
{
	char addrStr[32], valStr[16], cmpStr[16];
	//printf("Cheat Name:'%s'   Addr:0x%04x   Val:0x%02x \n", name, a, v );
	//
	cheat_win_t *cw = (cheat_win_t *) data;

	if (cw->actv_cheat_redraw)
	{
		gtk_tree_store_append (cw->actv_cheats_store, &cw->actv_cheats_iter, NULL);	// aquire iter
	}

	sprintf (addrStr, "0x%04X ", a);
	sprintf (valStr, " 0x%02X ", v);

	if (c >= 0)
	{
		sprintf (cmpStr, " 0x%02X ", c);
	}
	else
	{
		strcpy (cmpStr, " 0xFF ");
	}

	gtk_tree_store_set (cw->actv_cheats_store, &cw->actv_cheats_iter,
			    0, s, 1, addrStr, 2, valStr, 3, cmpStr, 4, name,
			    -1);

	if (!cw->actv_cheat_redraw)
	{
		if (!gtk_tree_model_iter_next
		    (GTK_TREE_MODEL (cw->actv_cheats_store),
		     &cw->actv_cheats_iter))
		{
			gtk_tree_store_append (cw->actv_cheats_store, &cw->actv_cheats_iter, NULL);	// aquire iter
		}
	}

	return 1;
}

void cheat_win_t::showActiveCheatList (bool reset)
{
	actv_cheat_redraw = reset;

	if (actv_cheat_redraw)
	{
		gtk_tree_store_clear (actv_cheats_store);
	}
	else
	{
		if (!gtk_tree_model_get_iter_first
		    (GTK_TREE_MODEL (actv_cheats_store), &actv_cheats_iter))
		{
			//printf("No Tree Entries Loaded.\n");
			actv_cheat_redraw = 1;
		}
	}

	FCEUI_ListCheats (activeCheatListCB, (void *) this);

	actv_cheat_redraw = false;
}

int cheat_win_t::getSelCheatRow(void)
{
	int numListRows, retval = -1;
	GList *selListRows, *tmpList;
	GtkTreeModel *model = NULL;
	GtkTreeSelection *treeSel;

	treeSel =
		gtk_tree_view_get_selection (GTK_TREE_VIEW
					     (actv_cheat_tree));

	numListRows = gtk_tree_selection_count_selected_rows (treeSel);

	if (numListRows == 0)
	{
		return -1;
	}
	//printf("Number of Rows Selected: %i\n", numListRows );

	selListRows = gtk_tree_selection_get_selected_rows (treeSel, &model);

	tmpList = selListRows;

	while (tmpList)
	{
		int depth;
		int *indexArray;
		GtkTreePath *path = (GtkTreePath *) tmpList->data;

		depth = gtk_tree_path_get_depth (path);
		indexArray = gtk_tree_path_get_indices (path);

		if (depth > 0)
		{
			retval = indexArray[0];
		}

		tmpList = tmpList->next;
	}

	g_list_free_full (selListRows, (GDestroyNotify) gtk_tree_path_free);

	return retval;
}

static void cheatListEnableToggle (GtkCellRendererToggle * renderer,
				   gchar * pathStr, cheat_win_t * cw)
{
	GtkTreePath *path;
	int depth;
	int *indexArray;

	path = gtk_tree_path_new_from_string (pathStr);

	if (path == NULL)
	{
		printf ("Error: gtk_tree_path_new_from_string failed\n");
		return;
	}

	depth = gtk_tree_path_get_depth (path);
	indexArray = gtk_tree_path_get_indices (path);

	if (depth > 0)
	{
		//printf("Toggle: %i\n", indexArray[0] );
		FCEUI_ToggleCheat (indexArray[0]);
	}

	gtk_tree_path_free (path);

	updateAllActvCheatLists (0);

}

static void
cheat_select_rowCB (GtkTreeView *treeview,
							cheat_win_t * cw )
{
	int row, row_is_selected;

	row = cw->getSelCheatRow();

	row_is_selected = (row >= 0);

	//printf("Selected row = %i\n", row);
	//
	gtk_widget_set_sensitive( cw->cheat_del_button , row_is_selected );
	gtk_widget_set_sensitive( cw->cheat_edit_button, row_is_selected );

	if ( !row_is_selected )
	{
		return;
	}
	uint32 a;
	uint8 v;
	int c, s, type;
	char *name = NULL;

	if (FCEUI_GetCheat (row, &name, &a, &v, &c, &s, &type))
	{
		char txt[64];

		if ( name )
		{
			gtk_entry_set_text( GTK_ENTRY(cw->cheat_name_entry), name );
		}

		sprintf( txt, "%04X", a );
		gtk_entry_set_text( GTK_ENTRY(cw->cheat_addr_entry), txt );

		sprintf( txt, "%02X", v );
		gtk_entry_set_text( GTK_ENTRY(cw->cheat_val_entry), txt );

		if ( c >= 0 )
		{
			sprintf( txt, "%02X", c );
		}
		else
		{
			txt[0] = 0;
		}
		gtk_entry_set_text( GTK_ENTRY(cw->cheat_cmp_entry), txt );
	}

	//gtk_widget_set_sensitive( dw->del_bp_button , row_is_selected );
	//gtk_widget_set_sensitive( dw->edit_bp_button, row_is_selected );
}
static void refreshCheatListCB(GtkWidget * widget, cheat_win_t * cw)
{
	updateAllActvCheatLists (1);
}

static void openCheatFile (GtkWidget * widget, cheat_win_t * cw)
{
	GtkWidget *fileChooser;
	GtkFileFilter *filterCht;
	GtkFileFilter *filterAll;

	filterCht = gtk_file_filter_new ();
	filterAll = gtk_file_filter_new ();

	gtk_file_filter_add_pattern (filterCht, "*.cht");
	gtk_file_filter_add_pattern (filterAll, "*");

	gtk_file_filter_set_name (filterCht, "*.cht");
	gtk_file_filter_set_name (filterAll, "All Files");

	fileChooser =
		gtk_file_chooser_dialog_new ("Open Cheat",
					     GTK_WINDOW (cw->win),
					     GTK_FILE_CHOOSER_ACTION_OPEN,
					     "_Cancel", GTK_RESPONSE_CANCEL,
					     "_Open", GTK_RESPONSE_ACCEPT,
					     NULL);
	const char *last_dir;
	g_config->getOption ("SDL.LastOpenFile", &last_dir);
	gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (fileChooser),
				       last_dir);

	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (fileChooser), filterCht);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (fileChooser), filterAll);

	if (gtk_dialog_run (GTK_DIALOG (fileChooser)) == GTK_RESPONSE_ACCEPT)
	{
		FILE *fp;
		char *filename;

		filename =
			gtk_file_chooser_get_filename (GTK_FILE_CHOOSER
						       (fileChooser));
		gtk_widget_destroy (fileChooser);

		fp = fopen (filename, "r");

		if (fp != NULL)
		{
			FCEU_LoadGameCheats (fp, 0);
			fclose (fp);
		}
		//g_config->setOption("SDL.LastOpenFile", filename);
		// Error dialog no longer required with GTK implementation of FCEUD_PrintError()

		resizeGtkWindow ();
		g_free (filename);
	}
	else
	{
		gtk_widget_destroy (fileChooser);
	}

	updateAllActvCheatLists (1);
}

static void newCheatEntryCB1 (GtkWidget * widget, cheat_win_t * cw)
{
	const gchar *entry_text;
	entry_text = gtk_entry_get_text (GTK_ENTRY (widget));

	if (entry_text[0] == 0)
	{
		cw->new_cheat_addr = -1;
	}
	else
	{
		cw->new_cheat_addr = strtol (entry_text, NULL, 16);
	}
}

static void newCheatEntryCB2 (GtkWidget * widget, cheat_win_t * cw)
{
	const gchar *entry_text;
	entry_text = gtk_entry_get_text (GTK_ENTRY (widget));

	if (entry_text[0] == 0)
	{
		cw->new_cheat_val = -1;
	}
	else
	{
		cw->new_cheat_val = strtol (entry_text, NULL, 16);
	}
}

static void newCheatEntryCB3 (GtkWidget * widget, cheat_win_t * cw)
{
	const gchar *entry_text;
	entry_text = gtk_entry_get_text (GTK_ENTRY (widget));

	if (entry_text[0] == 0)
	{
		cw->new_cheat_cmp = -1;
	}
	else
	{
		cw->new_cheat_cmp = strtol (entry_text, NULL, 16);
	}
}

static void newCheatEntryCB4 (GtkWidget * widget, cheat_win_t * cw)
{
	const gchar *entry_text;
	entry_text = gtk_entry_get_text (GTK_ENTRY (widget));

	cw->new_cheat_name.assign (entry_text);
}

static void addCheat2Active (GtkWidget * widget, cheat_win_t * cw)
{

	if ((cw->new_cheat_addr >= 0) && (cw->new_cheat_val >= 0))
	{
		if (FCEUI_AddCheat
		    (cw->new_cheat_name.c_str (), cw->new_cheat_addr,
		     cw->new_cheat_val, cw->new_cheat_cmp, 1))
		{
			updateAllActvCheatLists (1);
		}
	}
}

static void removeCheatFromActive (GtkWidget * widget, cheat_win_t * cw)
{
	int numListRows;
	GList *selListRows, *tmpList;
	GtkTreeModel *model = NULL;
	GtkTreeSelection *treeSel;

	treeSel =
		gtk_tree_view_get_selection (GTK_TREE_VIEW
					     (cw->actv_cheat_tree));

	numListRows = gtk_tree_selection_count_selected_rows (treeSel);

	if (numListRows == 0)
	{
		return;
	}
	//printf("Number of Rows Selected: %i\n", numListRows );

	selListRows = gtk_tree_selection_get_selected_rows (treeSel, &model);

	tmpList = selListRows;

	while (tmpList)
	{
		int depth;
		int *indexArray;
		GtkTreePath *path = (GtkTreePath *) tmpList->data;

		depth = gtk_tree_path_get_depth (path);
		indexArray = gtk_tree_path_get_indices (path);

		if (depth > 0)
		{
			//GtkTreeIter iter;
			FCEUI_DelCheat (indexArray[0]);

			//if ( gtk_tree_model_get_iter ( model, &iter, path ) )
			//{
			//   gtk_tree_store_remove( actv_cheats_store, &iter );
			//}
		}

		tmpList = tmpList->next;
	}

	g_list_free_full (selListRows, (GDestroyNotify) gtk_tree_path_free);

	updateAllActvCheatLists (1);
}

static void updateCheatList (GtkWidget * widget, cheat_win_t * cw)
{
	int numListRows;
	GList *selListRows, *tmpList;
	GtkTreeModel *model = NULL;
	GtkTreeSelection *treeSel;

	treeSel =
		gtk_tree_view_get_selection (GTK_TREE_VIEW
					     (cw->actv_cheat_tree));

	numListRows = gtk_tree_selection_count_selected_rows (treeSel);

	if (numListRows == 0)
	{
		return;
	}
	//printf("Number of Rows Selected: %i\n", numListRows );

	selListRows = gtk_tree_selection_get_selected_rows (treeSel, &model);

	tmpList = selListRows;

	while (tmpList)
	{
		int depth;
		int *indexArray;
		GtkTreePath *path = (GtkTreePath *) tmpList->data;

		depth = gtk_tree_path_get_depth (path);
		indexArray = gtk_tree_path_get_indices (path);

		if (depth > 0)
		{
			uint32 a;
			uint8 v;
			int c, s, type;
			const char *name = NULL;
			if (FCEUI_GetCheat
			    (indexArray[0], NULL, &a, &v, &c, &s, &type))
			{
				if (cw->new_cheat_addr >= 0)
				{
					a = cw->new_cheat_addr;
				}
				if (cw->new_cheat_val >= 0)
				{
					v = cw->new_cheat_val;
				}
				if (cw->new_cheat_cmp >= 0)
				{
					c = cw->new_cheat_cmp;
				}
				if (cw->new_cheat_name.size ())
				{
					name = cw->new_cheat_name.c_str ();
				}
				FCEUI_SetCheat (indexArray[0], name, a, v, c, s,
						type);
			}
		}
		//printf("Depth: %i \n", depth );

		//for (int i=0; i<depth; i++)
		//{
		//   printf("%i: %i\n", i, indexArray[i] );
		//}
		tmpList = tmpList->next;
	}

	g_list_free_full (selListRows, (GDestroyNotify) gtk_tree_path_free);

	updateAllActvCheatLists (0);
}

static void cheat_cell_edited_cb1 (GtkCellRendererText * cell,
				   gchar * path_string,
				   gchar * new_text, cheat_win_t * cw)
{
	long int row_idx = -1;

	//printf("'%s'\n", path_string );
	//printf("'%s'\n", new_text    );
	//printf("%p\n", user_data   );

	if (isdigit (path_string[0]))
	{
		row_idx = atoi (path_string);
	}

	if (row_idx >= 0)
	{
		uint32 a;
		uint8 v;
		int c, s, type;
		const char *name = NULL;
		if (FCEUI_GetCheat (row_idx, NULL, &a, &v, &c, &s, &type))
		{
			a = strtol (new_text, NULL, 0);
			FCEUI_SetCheat (row_idx, name, a, v, c, s, type);
			updateAllActvCheatLists (0);
		}
	}
}

static void cheat_cell_edited_cb2 (GtkCellRendererText * cell,
				   gchar * path_string,
				   gchar * new_text, cheat_win_t * cw)
{
	long int row_idx = -1;

	//printf("'%s'\n", path_string );
	//printf("'%s'\n", new_text    );
	//printf("%p\n", user_data   );

	if (isdigit (path_string[0]))
	{
		row_idx = atoi (path_string);
	}

	if (row_idx >= 0)
	{
		uint32 a;
		uint8 v;
		int c, s, type;
		const char *name = NULL;
		if (FCEUI_GetCheat (row_idx, NULL, &a, &v, &c, &s, &type))
		{
			v = strtol (new_text, NULL, 0);
			FCEUI_SetCheat (row_idx, name, a, v, c, s, type);
			updateAllActvCheatLists (0);
		}
	}
}

static void cheat_cell_edited_cb3 (GtkCellRendererText * cell,
				   gchar * path_string,
				   gchar * new_text, cheat_win_t * cw)
{
	long int row_idx = -1;

	//printf("'%s'\n", path_string );
	//printf("'%s'\n", new_text    );
	//printf("%p\n", user_data   );

	if (isdigit (path_string[0]))
	{
		row_idx = atoi (path_string);
	}

	if (row_idx >= 0)
	{
		uint32 a;
		uint8 v;
		int c, s, type;
		const char *name = NULL;
		if (FCEUI_GetCheat (row_idx, NULL, &a, &v, &c, &s, &type))
		{
			c = strtol (new_text, NULL, 0);
			FCEUI_SetCheat (row_idx, name, a, v, c, s, type);
			updateAllActvCheatLists (0);
		}
	}
}

static void cheat_cell_edited_cb4 (GtkCellRendererText * cell,
				   gchar * path_string,
				   gchar * new_text, cheat_win_t * cw)
{
	long int row_idx = -1;

	//printf("'%s'\n", path_string );
	//printf("'%s'\n", new_text    );
	//printf("%p\n", user_data   );

	if (isdigit (path_string[0]))
	{
		row_idx = atoi (path_string);
	}

	if (row_idx >= 0)
	{
		uint32 a;
		uint8 v;
		int c, s, type;
		const char *name = NULL;
		if (FCEUI_GetCheat (row_idx, NULL, &a, &v, &c, &s, &type))
		{
			name = new_text;
			FCEUI_SetCheat (row_idx, name, a, v, c, s, type);
			updateAllActvCheatLists (0);
		}
	}
}

static void updateAllActvCheatLists (bool redraw)
{
	std::list < cheat_win_t * >::iterator it;

	for (it = cheatWinList.begin (); it != cheatWinList.end (); it++)
	{
		(*it)->showActiveCheatList (redraw);
	}
}

static void closeCheatDialog (GtkWidget * w, GdkEvent * e, gpointer p)
{
	std::list < cheat_win_t * >::iterator it;
	cheat_win_t *cw = (cheat_win_t *) p;

	if (EmulationPaused && cw->wasPausedByCheats)
	{
		EmulationPaused = 0;
	}

	for (it = cheatWinList.begin (); it != cheatWinList.end (); it++)
	{
		if (cw == *it)
		{
			//printf("Removing Cheat Window %p from List\n", cw);
			cheatWinList.erase (it);
			break;
		}
	}
	//printf("Number of Cheat Windows Still Open: %zi\n", cheatWinList.size() );

	delete cw;

	gtk_widget_destroy (w);
}

// creates and opens cheats window
void openCheatsWindow (void)
{
	GtkWidget *win;
	GtkWidget *main_hbox;
	GtkWidget *hbox;
	GtkWidget *vbox, *prev_cmp_vbox;
	GtkWidget *frame;
	GtkWidget *label, *txt_entry;
	GtkWidget *button;
	GtkWidget *scroll;

	cheat_win_t *cw = new cheat_win_t;

	cheatWinList.push_back (cw);

	win = gtk_dialog_new_with_buttons ("Cheats",
					   GTK_WINDOW (MainWindow),
					   (GtkDialogFlags)
					   (GTK_DIALOG_DESTROY_WITH_PARENT),
					   "_Close", GTK_RESPONSE_OK, NULL);
	gtk_window_set_default_size (GTK_WINDOW (win), 600, 600);

	cw->win = win;

	main_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
	frame = gtk_frame_new ("Active Cheats");

	cw->actv_cheats_store = gtk_tree_store_new (5, G_TYPE_BOOLEAN,
						    G_TYPE_STRING,
						    G_TYPE_STRING,
						    G_TYPE_STRING,
						    G_TYPE_STRING);

	cw->actv_cheat_tree =
		gtk_tree_view_new_with_model (GTK_TREE_MODEL
					      (cw->actv_cheats_store));

	GtkCellRenderer *renderer;
	GtkCellRenderer *chkbox_renderer;
	GtkTreeViewColumn *column;

	g_signal_connect (cw->actv_cheat_tree, "cursor-changed",
			  G_CALLBACK (cheat_select_rowCB), (gpointer) cw);

	gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (cw->actv_cheat_tree),
				      GTK_TREE_VIEW_GRID_LINES_VERTICAL);

	chkbox_renderer = gtk_cell_renderer_toggle_new ();
	gtk_cell_renderer_toggle_set_activatable ((GtkCellRendererToggle *)
						  chkbox_renderer, TRUE);
	column = gtk_tree_view_column_new_with_attributes ("Ena",
							   chkbox_renderer,
							   "active", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (cw->actv_cheat_tree),
				     column);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer, "family", "MonoSpace", NULL);
	g_object_set (renderer, "editable", TRUE, NULL);
	g_signal_connect (renderer, "edited", (GCallback) cheat_cell_edited_cb1,
			  (gpointer) cw);
	column = gtk_tree_view_column_new_with_attributes ("Addr", renderer,
							   "text", 1, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (cw->actv_cheat_tree),
				     column);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer, "family", "MonoSpace", NULL);
	g_object_set (renderer, "editable", TRUE, NULL);
	g_signal_connect (renderer, "edited", (GCallback) cheat_cell_edited_cb2,
			  (gpointer) cw);
	column = gtk_tree_view_column_new_with_attributes ("Val", renderer,
							   "text", 2, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (cw->actv_cheat_tree),
				     column);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer, "family", "MonoSpace", NULL);
	g_object_set (renderer, "editable", TRUE, NULL);
	g_signal_connect (renderer, "edited", (GCallback) cheat_cell_edited_cb3,
			  (gpointer) cw);
	column = gtk_tree_view_column_new_with_attributes ("Cmp", renderer,
							   "text", 3, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (cw->actv_cheat_tree),
				     column);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer, "editable", TRUE, NULL);
	g_signal_connect (renderer, "edited", (GCallback) cheat_cell_edited_cb4,
			  (gpointer) cw);
	column = gtk_tree_view_column_new_with_attributes ("Desc", renderer,
							   "text", 4, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (cw->actv_cheat_tree),
				     column);

	g_signal_connect (chkbox_renderer, "toggled",
			  G_CALLBACK (cheatListEnableToggle), (void *) cw);

	updateAllActvCheatLists (1);

	scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
					GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (scroll), cw->actv_cheat_tree);
	gtk_box_pack_start (GTK_BOX (vbox), scroll, TRUE, TRUE, 1);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	label = gtk_label_new ("Name:");
	cw->cheat_name_entry = gtk_entry_new ();

	g_signal_connect (cw->cheat_name_entry, "activate",
			  G_CALLBACK (newCheatEntryCB4), (void *) cw);
	g_signal_connect (cw->cheat_name_entry, "changed",
			  G_CALLBACK (newCheatEntryCB4), (void *) cw);

	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 1);
	gtk_box_pack_start (GTK_BOX (hbox), cw->cheat_name_entry, TRUE, TRUE, 1);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 1);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	label = gtk_label_new ("Addr:");
	cw->cheat_addr_entry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (cw->cheat_addr_entry), 4);
	gtk_entry_set_width_chars (GTK_ENTRY (cw->cheat_addr_entry), 4);

	g_signal_connect (cw->cheat_addr_entry, "activate",
			  G_CALLBACK (newCheatEntryCB1), (void *) cw);
	g_signal_connect (cw->cheat_addr_entry, "changed",
			  G_CALLBACK (newCheatEntryCB1), (void *) cw);

	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 1);
	gtk_box_pack_start (GTK_BOX (hbox), cw->cheat_addr_entry, TRUE, TRUE, 1);

	label = gtk_label_new ("Val:");
	cw->cheat_val_entry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (cw->cheat_val_entry), 2);
	gtk_entry_set_width_chars (GTK_ENTRY (cw->cheat_val_entry), 2);

	g_signal_connect (cw->cheat_val_entry, "activate",
			  G_CALLBACK (newCheatEntryCB2), (void *) cw);
	g_signal_connect (cw->cheat_val_entry, "changed",
			  G_CALLBACK (newCheatEntryCB2), (void *) cw);

	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 1);
	gtk_box_pack_start (GTK_BOX (hbox), cw->cheat_val_entry, TRUE, TRUE, 1);

	label = gtk_label_new ("Cmp:");
	cw->cheat_cmp_entry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (cw->cheat_cmp_entry), 2);
	gtk_entry_set_width_chars (GTK_ENTRY (cw->cheat_cmp_entry), 2);

	g_signal_connect (cw->cheat_cmp_entry, "activate",
			  G_CALLBACK (newCheatEntryCB3), (void *) cw);
	g_signal_connect (cw->cheat_cmp_entry, "changed",
			  G_CALLBACK (newCheatEntryCB3), (void *) cw);

	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 1);
	gtk_box_pack_start (GTK_BOX (hbox), cw->cheat_cmp_entry, TRUE, TRUE, 1);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 1);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 3);
	button = gtk_button_new_with_label ("Add");
	gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, FALSE, 1);

	g_signal_connect (button, "clicked",
			  G_CALLBACK (addCheat2Active), (gpointer) cw);

	cw->cheat_del_button = gtk_button_new_with_label ("Delete");
	gtk_box_pack_start (GTK_BOX (hbox), cw->cheat_del_button, TRUE, FALSE, 1);

	gtk_widget_set_sensitive( cw->cheat_del_button, FALSE );

	g_signal_connect (cw->cheat_del_button, "clicked",
			  G_CALLBACK (removeCheatFromActive), (gpointer) cw);

	cw->cheat_edit_button = gtk_button_new_with_label ("Update");
	gtk_box_pack_start (GTK_BOX (hbox), cw->cheat_edit_button, TRUE, FALSE, 1);

	gtk_widget_set_sensitive( cw->cheat_edit_button, FALSE );

	g_signal_connect (cw->cheat_edit_button, "clicked",
			  G_CALLBACK (updateCheatList), (gpointer) cw);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 1);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	button = gtk_button_new_with_label ("Refresh List");
	gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, FALSE, 1);
	g_signal_connect (button, "clicked",
			  G_CALLBACK (refreshCheatListCB), (gpointer) cw);

	button = gtk_button_new_with_label ("Add from CHT file...");
	gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, FALSE, 1);
	g_signal_connect (button, "clicked",
			  G_CALLBACK (openCheatFile), (gpointer) cw);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 1);

	gtk_container_add (GTK_CONTAINER (frame), vbox);
	//
	gtk_box_pack_start (GTK_BOX (main_hbox), frame, TRUE, TRUE, 1);

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
	button = gtk_button_new_with_label ("Reset");
	g_signal_connect (button, "clicked",
			  G_CALLBACK (cheatSearchReset), (gpointer) cw);
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 5);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 5);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 3);
	button = gtk_button_new_with_label ("Known Value:");
	cw->cheat_search_known_btn = button;
	g_signal_connect (button, "clicked",
			  G_CALLBACK (cheatSearchKnown), (gpointer) cw);
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 5);
	label = gtk_label_new ("0x");
	txt_entry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (txt_entry), 2);
	gtk_entry_set_width_chars (GTK_ENTRY (txt_entry), 2);

	g_signal_connect (txt_entry, "activate",
			  G_CALLBACK (cheatSearchValueEntryCB1), (void *) cw);
	g_signal_connect (txt_entry, "changed",
			  G_CALLBACK (cheatSearchValueEntryCB1), (void *) cw);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 5);
	gtk_box_pack_start (GTK_BOX (hbox), txt_entry, FALSE, FALSE, 5);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 5);

	frame = gtk_frame_new ("Previous Compare");
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 5);
	button = gtk_check_button_new_with_label
		("Pause emulation when this window is active");
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 5);
	g_signal_connect (button, "clicked",
			  G_CALLBACK (pauseDuringCheatWinActvCB),
			  (gpointer) cw);

	prev_cmp_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
	gtk_container_add (GTK_CONTAINER (frame), prev_cmp_vbox);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
	button = gtk_button_new_with_label ("Equal");
	cw->cheat_search_eq_btn = button;
	//gtk_widget_set_halign( button, GTK_ALIGN_BASELINE);

	gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 5);
	gtk_box_pack_start (GTK_BOX (prev_cmp_vbox), hbox, FALSE, FALSE, 5);
	g_signal_connect (button, "clicked",
			  G_CALLBACK (cheatSearchEqual), (gpointer) cw);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
	button = gtk_button_new_with_label ("Not Equal");
	cw->cheat_search_neq_btn = button;
	//gtk_widget_set_halign( button, GTK_ALIGN_BASELINE);
	gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 5);
	cw->neq_chkbox = gtk_check_button_new ();
	gtk_box_pack_start (GTK_BOX (hbox), cw->neq_chkbox, FALSE, FALSE, 5);
	g_signal_connect (button, "clicked",
			  G_CALLBACK (cheatSearchNotEqual), (gpointer) cw);
	label = gtk_label_new ("By: 0x");
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 5);
	txt_entry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (txt_entry), 2);
	gtk_entry_set_width_chars (GTK_ENTRY (txt_entry), 2);
	g_signal_connect (txt_entry, "activate",
			  G_CALLBACK (cheatSearchValueEntryCB2), (void *) cw);
	g_signal_connect (txt_entry, "changed",
			  G_CALLBACK (cheatSearchValueEntryCB2), (void *) cw);
	gtk_box_pack_start (GTK_BOX (hbox), txt_entry, FALSE, FALSE, 5);

	gtk_box_pack_start (GTK_BOX (prev_cmp_vbox), hbox, FALSE, FALSE, 5);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
	button = gtk_button_new_with_label ("Greater Than");
	cw->cheat_search_gr_btn = button;
	//gtk_widget_set_halign( button, GTK_ALIGN_BASELINE);
	gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 5);
	cw->gt_chkbox = gtk_check_button_new ();
	g_signal_connect (button, "clicked",
			  G_CALLBACK (cheatSearchGreaterThan), (gpointer) cw);
	gtk_box_pack_start (GTK_BOX (hbox), cw->gt_chkbox, FALSE, FALSE, 5);
	label = gtk_label_new ("By: 0x");
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 5);
	txt_entry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (txt_entry), 2);
	gtk_entry_set_width_chars (GTK_ENTRY (txt_entry), 2);
	g_signal_connect (txt_entry, "activate",
			  G_CALLBACK (cheatSearchValueEntryCB3), (void *) cw);
	g_signal_connect (txt_entry, "changed",
			  G_CALLBACK (cheatSearchValueEntryCB3), (void *) cw);
	gtk_box_pack_start (GTK_BOX (hbox), txt_entry, FALSE, FALSE, 5);

	gtk_box_pack_start (GTK_BOX (prev_cmp_vbox), hbox, FALSE, FALSE, 5);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
	button = gtk_button_new_with_label ("Less Than");
	cw->cheat_search_lt_btn = button;
	//gtk_widget_set_halign( button, GTK_ALIGN_BASELINE);
	gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 5);
	cw->lt_chkbox = gtk_check_button_new ();
	g_signal_connect (button, "clicked",
			  G_CALLBACK (cheatSearchLessThan), (gpointer) cw);
	gtk_box_pack_start (GTK_BOX (hbox), cw->lt_chkbox, FALSE, FALSE, 5);
	label = gtk_label_new ("By: 0x");
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 5);
	txt_entry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (txt_entry), 2);
	gtk_entry_set_width_chars (GTK_ENTRY (txt_entry), 2);
	g_signal_connect (txt_entry, "activate",
			  G_CALLBACK (cheatSearchValueEntryCB4), (void *) cw);
	g_signal_connect (txt_entry, "changed",
			  G_CALLBACK (cheatSearchValueEntryCB4), (void *) cw);
	gtk_box_pack_start (GTK_BOX (hbox), txt_entry, FALSE, FALSE, 5);

	gtk_box_pack_start (GTK_BOX (prev_cmp_vbox), hbox, FALSE, FALSE, 1);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 1);

	gtk_widget_set_sensitive( cw->cheat_search_known_btn , FALSE );
	gtk_widget_set_sensitive( cw->cheat_search_eq_btn    , FALSE );
	gtk_widget_set_sensitive( cw->cheat_search_neq_btn   , FALSE );
	gtk_widget_set_sensitive( cw->cheat_search_gr_btn    , FALSE );
	gtk_widget_set_sensitive( cw->cheat_search_lt_btn    , FALSE );

	frame = gtk_frame_new ("Cheat Search");
	gtk_container_add (GTK_CONTAINER (frame), hbox);
	gtk_box_pack_start (GTK_BOX (main_hbox), frame, TRUE, TRUE, 5);

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
	//hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);

	cw->ram_match_store =
		gtk_tree_store_new (3, G_TYPE_STRING, G_TYPE_STRING,
				    G_TYPE_STRING);

	cw->search_cheat_tree =
		gtk_tree_view_new_with_model (GTK_TREE_MODEL
					      (cw->ram_match_store));

	gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (cw->search_cheat_tree),
				      GTK_TREE_VIEW_GRID_LINES_VERTICAL);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer, "family", "MonoSpace", NULL);
	column = gtk_tree_view_column_new_with_attributes ("Addr", renderer,
							   "text", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (cw->search_cheat_tree),
				     column);
	column = gtk_tree_view_column_new_with_attributes ("Last", renderer,
							   "text", 1, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (cw->search_cheat_tree),
				     column);
	column = gtk_tree_view_column_new_with_attributes ("Curr", renderer,
							   "text", 2, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (cw->search_cheat_tree),
				     column);

	scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
					GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (scroll), cw->search_cheat_tree);
	gtk_box_pack_start (GTK_BOX (vbox), scroll, TRUE, TRUE, 5);

	frame = gtk_frame_new ("");
	gtk_container_add (GTK_CONTAINER (frame), vbox);

	gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 5);

	gtk_box_pack_start (GTK_BOX
			    (gtk_dialog_get_content_area (GTK_DIALOG (win))),
			    main_hbox, TRUE, TRUE, 0);

	g_signal_connect (win, "delete-event", G_CALLBACK (closeCheatDialog),
			  cw);
	g_signal_connect (win, "response", G_CALLBACK (closeCheatDialog), cw);

	gtk_widget_show_all (win);

	//printf("Added Cheat Window %p. Number of Cheat Windows Open: %zi\n", cw, cheatWinList.size() );
}
