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

//*******************************************************************************************************
// Ram Watch Window
//*******************************************************************************************************
//
struct ramWatch_t
{
	std::string name;
	int addr;
	int type;
	int size;

	union
	{
		int8_t i8;
		uint8_t u8;
		int16_t i16;
		uint16_t u16;
	} val;

	  ramWatch_t (void)
	{
		addr = 0;
		type = 0;
		size = 0;
		val.u16 = 0;
	};

	void updateMem (void)
	{
		if (size == 1)
		{
			val.u8 = GetMem (addr);
		}
		else if (size == 2)
		{
			val.u16 = GetMem (addr) | (GetMem (addr + 1) << 8);
		}
		else
		{
			val.u8 = GetMem (addr);
		}
	}
};

struct ramWatchList_t
{

	std::list < ramWatch_t * >ls;

	ramWatchList_t (void)
	{

	}

	 ~ramWatchList_t (void)
	{
		ramWatch_t *rw;

		while (!ls.empty ())
		{
			rw = ls.front ();

			delete rw;

			ls.pop_front ();
		}
	}

	size_t size (void)
	{
		return ls.size ();
	};

	void add_entry (const char *name, int addr, int type, int size)
	{
		ramWatch_t *rw = new ramWatch_t;

		rw->name.assign (name);
		rw->addr = addr;
		rw->type = type;
		rw->size = size;
		ls.push_back (rw);
	}

	void updateMemoryValues (void)
	{
		ramWatch_t *rw;
		std::list < ramWatch_t * >::iterator it;

		for (it = ls.begin (); it != ls.end (); it++)
		{
			rw = *it;

			rw->updateMem ();
		}
	}

	ramWatch_t *getIndex (size_t idx)
	{
		size_t i = 0;
		std::list < ramWatch_t * >::iterator it;

		for (it = ls.begin (); it != ls.end (); it++)
		{
			if (i == idx)
			{
				return *it;
			}
			i++;
		}
		return NULL;
	}

	int deleteIndex (size_t idx)
	{
		size_t i = 0;
		std::list < ramWatch_t * >::iterator it;

		for (it = ls.begin (); it != ls.end (); it++)
		{
			if (i == idx)
			{
				delete *it;
				ls.erase (it);
				return 0;
			}
			i++;
		}
		return -1;
	}
};

static ramWatchList_t ramWatchList;

struct ramWatchWin_t;

struct ramWatchEntryWin_t
{
	GtkWidget *win;
	GtkWidget *chkbox;
	GtkWidget *button1, *button2, *button4;
	GtkWidget *txt_entry_name;
	GtkWidget *txt_entry_addr;
	ramWatchWin_t *rww;
	int idx;

	  ramWatchEntryWin_t (void)
	{
		win = NULL;
		chkbox = NULL;
		button1 = NULL;
		button2 = NULL;
		button4 = NULL;
		txt_entry_name = NULL;
		txt_entry_addr = NULL;
		rww = NULL;
		idx = -1;
	}
};

struct ramWatchWin_t
{

	GtkWidget *win;
	GtkWidget *tree;
	GtkTreeStore *ram_watch_store;
	bool ramWatchWinOpen;
	int ramWatchEditRowIdx;
	int ramWatchEditColIdx;

	  ramWatchWin_t (void)
	{
		win = NULL;
		tree = NULL;
		ram_watch_store = NULL;
		ramWatchWinOpen = false;
		ramWatchEditRowIdx = -1;
		ramWatchEditColIdx = -1;
	}

	void showRamWatchResults (int reset);
};

static gint ramWatchEvntSrcID = 0;

void ramWatchWin_t::showRamWatchResults (int reset)
{
	int row = 0;
	std::list < ramWatch_t * >::iterator it;
	GtkTreeIter iter;
	char addrStr[32], valStr1[16], valStr2[16];
	ramWatch_t *rw;

	//if ( !reset )
	//{
	//   if ( gtk_tree_model_get_iter_first( GTK_TREE_MODEL(ram_watch_store), &iter ) )
	//   {
	//      size_t treeSize = 1;

	//      while ( gtk_tree_model_iter_next( GTK_TREE_MODEL(ram_watch_store), &iter ) )
	//      {
	//         treeSize++;
	//      }
	//      if ( treeSize != ramWatchList.size() )
	//      {
	//         reset = 1;
	//      }
	//      //printf("  TreeSize:  %zi   RamWatchList.size: %zi \n", treeSize, ramWatchList.size() );
	//   }
	//}

	if (reset)
	{
		gtk_tree_store_clear (ram_watch_store);
	}
	else
	{
		if (!gtk_tree_model_get_iter_first
		    (GTK_TREE_MODEL (ram_watch_store), &iter))
		{
			gtk_tree_store_append (ram_watch_store, &iter, NULL);	// aquire iter
		}
	}

	for (it = ramWatchList.ls.begin (); it != ramWatchList.ls.end (); it++)
	{
		rw = *it;
		sprintf (addrStr, "0x%04X", rw->addr);

		if (reset)
		{
			gtk_tree_store_append (ram_watch_store, &iter, NULL);	// aquire iter
		}

		rw->updateMem ();

		if (rw->size == 2)
		{
			if (rw->type)
			{
				sprintf (valStr1, "%6u", rw->val.u16);
			}
			else
			{
				sprintf (valStr1, "%6i", rw->val.i16);
			}
			sprintf (valStr2, "0x%04X", rw->val.u16);
		}
		else
		{
			if (rw->type)
			{
				sprintf (valStr1, "%6u", rw->val.u8);
			}
			else
			{
				sprintf (valStr1, "%6i", rw->val.i8);
			}
			sprintf (valStr2, "0x%02X", rw->val.u8);
		}

		if (row != ramWatchEditRowIdx)
		{
			gtk_tree_store_set (ram_watch_store, &iter,
					    0, addrStr, 1, valStr1, 2, valStr2,
					    3, rw->name.c_str (), -1);
		}
		else
		{
			//if ( ramWatchEditColIdx != 0 )
			//{
			//   gtk_tree_store_set(ram_watch_store, &iter, 0, addrStr, -1 );
			//}

			//if ( ramWatchEditColIdx != 1 )
			//{
			//   gtk_tree_store_set(ram_watch_store, &iter, 1, valStr1, -1 );
			//}

			//if ( ramWatchEditColIdx != 2 )
			//{
			//   gtk_tree_store_set(ram_watch_store, &iter, 2, valStr2, -1 );
			//}

			//if ( ramWatchEditColIdx != 3 )
			//{
			//   gtk_tree_store_set(ram_watch_store, &iter, 3, rw->name.c_str(), -1 );
			//}
		}

		if (!reset)
		{
			if (!gtk_tree_model_iter_next
			    (GTK_TREE_MODEL (ram_watch_store), &iter))
			{
				gtk_tree_store_append (ram_watch_store, &iter, NULL);	// aquire iter
			}
		}
		row++;
	}
}

static std::list < ramWatchWin_t * >ramWatchWinList;

void showAllRamWatchResults (int reset)
{
	std::list < ramWatchWin_t * >::iterator it;

	for (it = ramWatchWinList.begin (); it != ramWatchWinList.end (); it++)
	{
		(*it)->showRamWatchResults (reset);
	}
}

static void saveWatchFile (const char *filename)
{
	int i;
	FILE *fp;
	const char *c;
	std::list < ramWatch_t * >::iterator it;
	ramWatch_t *rw;

	fp = fopen (filename, "w");

	if (fp == NULL)
	{
		printf ("Error: Failed to open file: %s\n", filename);
		return;
	}

	for (it = ramWatchList.ls.begin (); it != ramWatchList.ls.end (); it++)
	{
		rw = *it;

		c = rw->name.c_str ();

		fprintf (fp, "0x%04x   %c%i   ", rw->addr, rw->type ? 'U' : 'S',
			 rw->size);

		i = 0;
		fprintf (fp, "\"");
		while (c[i])
		{
			if (c[i] == '"')
			{
				fprintf (fp, "\\%c", c[i]);
			}
			else
			{
				fprintf (fp, "%c", c[i]);
			}
			i++;
		}
		fprintf (fp, "\"\n");
	}
	fclose (fp);

}

static void loadWatchFile (const char *filename)
{
	FILE *fp;
	int i, j, a, t, s, literal;
	char line[512], stmp[512];
	ramWatch_t *rw;

	fp = fopen (filename, "r");

	if (fp == NULL)
	{
		printf ("Error: Failed to open file: %s\n", filename);
		return;
	}

	while (fgets (line, sizeof (line) - 1, fp) > 0)
	{
		a = -1;
		t = -1;
		s = -1;
		// Check for Comments
		i = 0;
		while (line[i] != 0)
		{
			if (literal)
			{
				literal = 0;
			}
			else
			{
				if (line[i] == '#')
				{
					line[i] = 0;
					break;
				}
				else if (line[i] == '\\')
				{
					literal = 1;
				}
			}
			i++;
		}

		i = 0;
		j = 0;
		while (isspace (line[i])) i++;

		if ((line[i] == '0') && (tolower (line[i + 1]) == 'x'))
		{
			stmp[j] = '0';
			j++;
			i++;
			stmp[j] = 'x';
			j++;
			i++;

			while (isxdigit (line[i]))
			{
				stmp[j] = line[i];
				i++;
				j++;
			}
		}
		else
		{
			while (isxdigit (line[i]))
			{
				stmp[j] = line[i];
				i++;
				j++;
			}
		}
		stmp[j] = 0;

		if (j == 0) continue;

		a = strtol (stmp, NULL, 0);

		while (isspace (line[i])) i++;

		t = line[i];
		i++;
		s = line[i];
		i++;

		if ((t != 'U') && (t != 'S'))
		{
			printf ("Error: Invalid RAM Watch Byte Type: %c", t);
			continue;
		}
		if (!isdigit (s))
		{
			printf ("Error: Invalid RAM Watch Byte Size: %c", s);
			continue;
		}
		s = s - '0';

		if ((s != 1) && (s != 2) && (s != 4))
		{
			printf ("Error: Invalid RAM Watch Byte Size: %i", s);
			continue;
		}

		while (isspace (line[i])) i++;

		if (line[i] == '"')
		{
			i++;
			j = 0;
			literal = 0;
			while ((line[i] != 0))
			{
				if (literal)
				{
					literal = 0;
				}
				else
				{
					if (line[i] == '"')
					{
						break;
					}
					else if (line[i] == '\\')
					{
						literal = 1;
					}
				}
				if (!literal)
				{
					stmp[j] = line[i];
					j++;
				}
				i++;
			}
			stmp[j] = 0;
		}
		rw = new ramWatch_t;

		rw->addr = a;
		rw->type = (t == 'U') ? 1 : 0;
		rw->size = s;
		rw->name.assign (stmp);

		ramWatchList.ls.push_back (rw);
	}

	fclose (fp);

	showAllRamWatchResults (1);
}

static void openWatchFile (int mode)
{
	GtkWidget *fileChooser;
	GtkFileFilter *filterWch;
	GtkFileFilter *filterAll;

	filterWch = gtk_file_filter_new ();
	gtk_file_filter_add_pattern (filterWch, "*.wch");
	gtk_file_filter_add_pattern (filterWch, "*.WCH");
	gtk_file_filter_set_name (filterWch, "Watch files");

	filterAll = gtk_file_filter_new ();
	gtk_file_filter_add_pattern (filterAll, "*");
	gtk_file_filter_set_name (filterAll, "All Files");

	//const char* last_dir;
	//g_config->getOption("SDL.LastSaveStateAs", &last_dir);


	if (mode)
	{
		fileChooser =
			gtk_file_chooser_dialog_new ("Save Watch File",
						     GTK_WINDOW (MainWindow),
						     GTK_FILE_CHOOSER_ACTION_SAVE,
						     "_Cancel",
						     GTK_RESPONSE_CANCEL,
						     "_Save",
						     GTK_RESPONSE_ACCEPT, NULL);
	}
	else
	{
		fileChooser =
			gtk_file_chooser_dialog_new ("Load Watch File",
						     GTK_WINDOW (MainWindow),
						     GTK_FILE_CHOOSER_ACTION_OPEN,
						     "_Cancel",
						     GTK_RESPONSE_CANCEL,
						     "_Open",
						     GTK_RESPONSE_ACCEPT, NULL);
	}

	//gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fileChooser), last_dir);

	//gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER(fileChooser), ".wch");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (fileChooser), filterWch);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (fileChooser), filterAll);

	if (gtk_dialog_run (GTK_DIALOG (fileChooser)) == GTK_RESPONSE_ACCEPT)
	{
		char *filename;

		filename =
			gtk_file_chooser_get_filename (GTK_FILE_CHOOSER
						       (fileChooser));
		//FCEUI_SaveState(filename);
		//g_config->setOption("SDL.LastSaveStateAs", filename);
		if (filename)
		{
			if (mode)
			{
				saveWatchFile (filename);
			}
			else
			{
				loadWatchFile (filename);
			}
			g_free (filename);
		}
	}
	gtk_widget_destroy (fileChooser);
}

static void loadRamWatchCB (GtkMenuItem * menuitem, gpointer user_data)
{
	openWatchFile (0);
}

static void saveRamWatchCB (GtkMenuItem * menuitem, gpointer user_data)
{
	openWatchFile (1);
}

static GtkWidget *CreateRamWatchMenubar (GtkWidget * window)
{
	GtkWidget *menubar, *menu, *item;

	menubar = gtk_menu_bar_new ();

	item = gtk_menu_item_new_with_label ("File");

	gtk_menu_shell_append (GTK_MENU_SHELL (menubar), item);

	menu = gtk_menu_new ();

	gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);

	item = gtk_menu_item_new_with_label ("Load Watch");

	g_signal_connect (item, "activate", G_CALLBACK (loadRamWatchCB), NULL);

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

	item = gtk_menu_item_new_with_label ("Save Watch");

	g_signal_connect (item, "activate", G_CALLBACK (saveRamWatchCB), NULL);

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

	// Finally, return the actual menu bar created
	return menubar;
}

//void closeMemoryWatchEntryWindow(GtkWidget* w, GdkEvent* e, ramWatchEntryWin_t *ew)
//{
//
//// strcpy( name, gtk_entry_get_text ( GTK_ENTRY(ew->txt_entry_name) ) );
//
////*addr = strtol( gtk_entry_get_text ( GTK_ENTRY(ew->txt_entry_addr) ), NULL, 16 );
//
////*type = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(ew->chkbox) );
//
//// if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(ew->button4) ) )
//// {
////   *size = 4;
//// }
//// else if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(ew->button2) ) )
//// {
////   *size = 2;
//// }
//// else
//// {
////   *size = 1;
//// }
//
//   delete ew;
//
//      gtk_widget_destroy(w);
//}

static int openRamWatchEntryDialog (ramWatchWin_t * rww, std::string * name,
				    int *addr, int *type, int *size, int idx)
{
	int retval;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *label;
	ramWatchEntryWin_t *ew;
	char stmp[32];

	ew = new ramWatchEntryWin_t;

	//printf("RAM Entry At Index %i \n", idx );

	ew->rww = rww;
	ew->idx = idx;

	ew->win = gtk_dialog_new_with_buttons ("RAM Watch Entry",
					       GTK_WINDOW (rww->win),
					       (GtkDialogFlags)
					       (GTK_DIALOG_DESTROY_WITH_PARENT),
					       "_Close", GTK_RESPONSE_OK, NULL);
	gtk_window_set_default_size (GTK_WINDOW (ew->win), 400, 200);

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);

	label = gtk_label_new ("Name:");
	ew->txt_entry_name = gtk_entry_new ();

	if (name->size () > 0)
	{
		gtk_entry_set_text (GTK_ENTRY (ew->txt_entry_name),
				    name->c_str ());
	}

	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), ew->txt_entry_name, FALSE, TRUE, 0);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	label = gtk_label_new ("Hex Address:");
	ew->txt_entry_addr = gtk_entry_new ();

	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

	label = gtk_label_new ("0x");
	sprintf (stmp, "%04x", *addr);
	gtk_entry_set_max_length (GTK_ENTRY (ew->txt_entry_addr), 4);
	gtk_entry_set_width_chars (GTK_ENTRY (ew->txt_entry_addr), 4);
	gtk_entry_set_text (GTK_ENTRY (ew->txt_entry_addr), stmp);

	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), ew->txt_entry_addr, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

	ew->chkbox = gtk_check_button_new_with_label ("Value is Unsigned");
	gtk_box_pack_start (GTK_BOX (vbox), ew->chkbox, FALSE, FALSE, 0);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
	label = gtk_label_new ("Size in Bytes:");
	ew->button1 = gtk_radio_button_new_with_label (NULL, "1");
	//gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 1);
	gtk_box_pack_start (GTK_BOX (hbox), ew->button1, TRUE, FALSE, 1);

	ew->button2 =
		gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON
							     (ew->button1),
							     "2");
	gtk_box_pack_start (GTK_BOX (hbox), ew->button2, TRUE, FALSE, 1);

	ew->button4 =
		gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON
							     (ew->button1),
							     "4");
	gtk_box_pack_start (GTK_BOX (hbox), ew->button4, TRUE, FALSE, 1);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ew->button1), TRUE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ew->button2), FALSE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ew->button4), FALSE);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 1);

	gtk_box_pack_start (GTK_BOX
			    (gtk_dialog_get_content_area
			     (GTK_DIALOG (ew->win))), vbox, TRUE, TRUE, 1);

	gtk_widget_show_all (ew->win);

	retval = gtk_dialog_run (GTK_DIALOG (ew->win));

	printf ("retval %i\n", retval);
	//
	if (retval == GTK_RESPONSE_OK)
	{
		// FIXME - what error checking should be done here

		name->assign (gtk_entry_get_text
			      (GTK_ENTRY (ew->txt_entry_name)));

		*addr = strtol (gtk_entry_get_text
				(GTK_ENTRY (ew->txt_entry_addr)), NULL, 16);

		*type = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
						      (ew->chkbox));

		if (gtk_toggle_button_get_active
		    (GTK_TOGGLE_BUTTON (ew->button4)))
		{
			*size = 4;
		}
		else if (gtk_toggle_button_get_active
			 (GTK_TOGGLE_BUTTON (ew->button2)))
		{
			*size = 2;
		}
		else
		{
			*size = 1;
		}
	}

	//g_signal_connect(ew->win, "delete-event", G_CALLBACK(closeMemoryWatchEntryWindow), ew);
	//g_signal_connect(ew->win, "response", G_CALLBACK(closeMemoryWatchEntryWindow), ew);

	gtk_widget_destroy (ew->win);

	delete ew;

	return (retval == GTK_RESPONSE_OK);
}

static void editRamWatch (GtkButton * button, ramWatchWin_t * rww)
{
	GtkTreeModel *model = NULL;
	GtkTreeSelection *treeSel;
	int numListRows;
	GList *selListRows, *tmpList;

	treeSel = gtk_tree_view_get_selection (GTK_TREE_VIEW (rww->tree));

	numListRows = gtk_tree_selection_count_selected_rows (treeSel);

	if (numListRows == 0)
	{
		return;
	}

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
			ramWatch_t *rw = ramWatchList.getIndex (indexArray[0]);

			if (rw != NULL)
			{
				openRamWatchEntryDialog (rww, &rw->name,
							 &rw->addr, &rw->type,
							 &rw->size,
							 indexArray[0]);
				showAllRamWatchResults (0);
			}
		}
		tmpList = tmpList->next;
	}

	g_list_free_full (selListRows, (GDestroyNotify) gtk_tree_path_free);

}

static void removeRamWatch (GtkButton * button, ramWatchWin_t * rww)
{
	GtkTreeModel *model = NULL;
	GtkTreeSelection *treeSel;
	int numListRows;
	GList *selListRows, *tmpList;

	treeSel = gtk_tree_view_get_selection (GTK_TREE_VIEW (rww->tree));

	numListRows = gtk_tree_selection_count_selected_rows (treeSel);

	if (numListRows == 0)
	{
		return;
	}

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
			ramWatchList.deleteIndex (indexArray[0]);
			showAllRamWatchResults (1);
		}
		tmpList = tmpList->next;
	}

	g_list_free_full (selListRows, (GDestroyNotify) gtk_tree_path_free);
}

static void newRamWatch (GtkButton * button, ramWatchWin_t * rww)
{
	std::string name;
	int addr = 0, type = 0, size = 0;

	if (openRamWatchEntryDialog (rww, &name, &addr, &type, &size, -1))
	{
		ramWatchList.add_entry (name.c_str (), addr, type, size);
	}

	showAllRamWatchResults (1);
}

static gint updateRamWatchTree (void *userData)
{
	//static uint32_t c = 0;
	//printf("RamWatch: %u\n", c++ );
	showAllRamWatchResults (0);
	return 1;
}

static void ramWatch_cell_edited_cb (GtkCellRendererText * cell,
				     gchar * path_string,
				     gchar * new_text, ramWatchWin_t * rww)
{
	ramWatch_t *rw;
	//printf("Ram Watch Edited: %i:%i\n", ramWatchEditRowIdx, ramWatchEditColIdx);

	rw = ramWatchList.getIndex (rww->ramWatchEditRowIdx);

	if ( rw == NULL )
	{
		rww->ramWatchEditRowIdx = -1;
		rww->ramWatchEditColIdx = -1;
		return;
	}

	switch (rww->ramWatchEditColIdx)
	{
		case 0:
			rw->addr = strtol (new_text, NULL, 0);
			break;
		case 1:
		case 2:
		{
			if ( (rw->addr >= 0) && (rw->addr < 0x8000) )
			{
				writefunc wfunc;

				if (rw->size == 2)
				{
					if (rw->type)
					{
						rw->val.u16 =
							strtol (new_text, NULL, 0);
					}
					else
					{
						rw->val.i16 =
							strtol (new_text, NULL, 0);
					}
					wfunc = GetWriteHandler (rw->addr);

					if (wfunc)
					{
						wfunc ((uint32) rw->addr,
						       (uint8) (rw->val.u16 & 0x00ff));
					}

					wfunc = GetWriteHandler (rw->addr + 1);

					if (wfunc)
					{
						wfunc ((uint32) rw->addr + 1,
						       (uint8) ((rw->val.
								 u16 & 0xff00) >> 8));
					}
				}
				else
				{
					if (rw->type)
					{
						rw->val.u8 = strtol (new_text, NULL, 0);
					}
					else
					{
						rw->val.i8 = strtol (new_text, NULL, 0);
					}
					wfunc = GetWriteHandler (rw->addr);

					if (wfunc)
					{
						wfunc ((uint32) rw->addr,
						       (uint8) rw->val.u8);
					}
				}
			}
		}
			break;
		case 3:
			rw->name.assign (new_text);
			break;
		default:

			break;
	}

	rww->ramWatchEditRowIdx = -1;
	rww->ramWatchEditColIdx = -1;
}

static void ramWatch_cell_edited_start_cb0 (GtkCellRenderer * renderer,
					    GtkCellEditable * editable,
					    gchar * path, ramWatchWin_t * rww)
{
	//printf("Ram Watch Edit Start: '%s':%li\n", path,  (long)user_data);
	rww->ramWatchEditRowIdx = atoi (path);
	rww->ramWatchEditColIdx = 0;
}

static void ramWatch_cell_edited_start_cb1 (GtkCellRenderer * renderer,
					    GtkCellEditable * editable,
					    gchar * path, ramWatchWin_t * rww)
{
	//printf("Ram Watch Edit Start: '%s':%li\n", path,  (long)user_data);
	rww->ramWatchEditRowIdx = atoi (path);
	rww->ramWatchEditColIdx = 1;
}

static void ramWatch_cell_edited_start_cb2 (GtkCellRenderer * renderer,
					    GtkCellEditable * editable,
					    gchar * path, ramWatchWin_t * rww)
{
	//printf("Ram Watch Edit Start: '%s':%li\n", path,  (long)user_data);
	rww->ramWatchEditRowIdx = atoi (path);
	rww->ramWatchEditColIdx = 2;
}

static void ramWatch_cell_edited_start_cb3 (GtkCellRenderer * renderer,
					    GtkCellEditable * editable,
					    gchar * path, ramWatchWin_t * rww)
{
	//printf("Ram Watch Edit Start: '%s':%li\n", path,  (long)user_data);
	rww->ramWatchEditRowIdx = atoi (path);
	rww->ramWatchEditColIdx = 3;
}

static void ramWatch_cell_edited_cancel_cb (GtkCellRenderer * renderer,
					    ramWatchWin_t * rww)
{
	//printf("Ram Watch Edit Cancel:%li\n", (long)user_data);
	rww->ramWatchEditRowIdx = -1;
	rww->ramWatchEditColIdx = -1;
}

void closeMemoryWatchWindow (GtkWidget * w, GdkEvent * e, ramWatchWin_t * rww)
{
	std::list < ramWatchWin_t * >::iterator it;

	for (it = ramWatchWinList.begin (); it != ramWatchWinList.end (); it++)
	{
		if (rww == *it)
		{
			//printf("Removing RamWatch Window %p from List\n", cw);
			ramWatchWinList.erase (it);
			break;
		}
	}

	delete rww;

	gtk_widget_destroy (w);
}

// creates and opens cheats window
void openMemoryWatchWindow (void)
{
	GtkWidget *main_vbox;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *button;
	GtkWidget *scroll;
	GtkWidget *menubar;
	ramWatchWin_t *rww;

	rww = new ramWatchWin_t;

	ramWatchWinList.push_back (rww);

	rww->win = gtk_dialog_new_with_buttons ("RAM Watch",
						GTK_WINDOW (MainWindow),
						(GtkDialogFlags)
						(GTK_DIALOG_DESTROY_WITH_PARENT),
						"_Close", GTK_RESPONSE_OK,
						NULL);

	gtk_window_set_default_size (GTK_WINDOW (rww->win), 600, 600);

	main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 1);

	menubar = CreateRamWatchMenubar (rww->win);

	gtk_box_pack_start (GTK_BOX (main_vbox), menubar, FALSE, TRUE, 0);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);

	rww->ram_watch_store =
		gtk_tree_store_new (4, G_TYPE_STRING, G_TYPE_STRING,
				    G_TYPE_STRING, G_TYPE_STRING);

	rww->tree =
		gtk_tree_view_new_with_model (GTK_TREE_MODEL
					      (rww->ram_watch_store));

	gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (rww->tree),
				      GTK_TREE_VIEW_GRID_LINES_VERTICAL);

	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer, "family", "MonoSpace", NULL);
	g_object_set (renderer, "editable", TRUE, NULL);
	g_signal_connect (renderer, "edited",
			  (GCallback) ramWatch_cell_edited_cb, (gpointer) rww);
	g_signal_connect (renderer, "editing-started",
			  (GCallback) ramWatch_cell_edited_start_cb0,
			  (gpointer) rww);
	g_signal_connect (renderer, "editing-canceled",
			  (GCallback) ramWatch_cell_edited_cancel_cb,
			  (gpointer) rww);
	column = gtk_tree_view_column_new_with_attributes ("Addr", renderer,
							   "text", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (rww->tree), column);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer, "family", "MonoSpace", NULL);
	g_object_set (renderer, "editable", TRUE, NULL);
	g_signal_connect (renderer, "edited",
			  (GCallback) ramWatch_cell_edited_cb, (gpointer) rww);
	g_signal_connect (renderer, "editing-started",
			  (GCallback) ramWatch_cell_edited_start_cb1,
			  (gpointer) rww);
	g_signal_connect (renderer, "editing-canceled",
			  (GCallback) ramWatch_cell_edited_cancel_cb,
			  (gpointer) rww);
	column = gtk_tree_view_column_new_with_attributes ("Value Dec",
							   renderer, "text", 1,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (rww->tree), column);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer, "family", "MonoSpace", NULL);
	g_object_set (renderer, "editable", TRUE, NULL);
	g_signal_connect (renderer, "edited",
			  (GCallback) ramWatch_cell_edited_cb, (gpointer) rww);
	g_signal_connect (renderer, "editing-started",
			  (GCallback) ramWatch_cell_edited_start_cb2,
			  (gpointer) rww);
	g_signal_connect (renderer, "editing-canceled",
			  (GCallback) ramWatch_cell_edited_cancel_cb,
			  (gpointer) rww);
	column = gtk_tree_view_column_new_with_attributes ("Value Hex",
							   renderer, "text", 2,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (rww->tree), column);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer, "editable", TRUE, NULL);
	g_signal_connect (renderer, "edited",
			  (GCallback) ramWatch_cell_edited_cb, (gpointer) rww);
	g_signal_connect (renderer, "editing-started",
			  (GCallback) ramWatch_cell_edited_start_cb3,
			  (gpointer) rww);
	g_signal_connect (renderer, "editing-canceled",
			  (GCallback) ramWatch_cell_edited_cancel_cb,
			  (gpointer) rww);
	column = gtk_tree_view_column_new_with_attributes ("Notes", renderer,
							   "text", 3, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (rww->tree), column);

	scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
					GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (scroll), rww->tree);
	gtk_box_pack_start (GTK_BOX (hbox), scroll, TRUE, TRUE, 5);

	gtk_box_pack_start (GTK_BOX (main_vbox), hbox, TRUE, TRUE, 5);

	showAllRamWatchResults (1);

	if (ramWatchEvntSrcID == 0)
	{
		ramWatchEvntSrcID =
			g_timeout_add (100, updateRamWatchTree, NULL);
	}

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 3);

	button = gtk_button_new_with_label ("Edit");
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 5);
	g_signal_connect (button, "clicked",
			  G_CALLBACK (editRamWatch), (gpointer) rww);

	button = gtk_button_new_with_label ("Remove");
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 5);
	g_signal_connect (button, "clicked",
			  G_CALLBACK (removeRamWatch), (gpointer) rww);

	button = gtk_button_new_with_label ("New");
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 5);
	g_signal_connect (button, "clicked",
			  G_CALLBACK (newRamWatch), (gpointer) rww);

	gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 5);

	gtk_box_pack_start (GTK_BOX
			    (gtk_dialog_get_content_area
			     (GTK_DIALOG (rww->win))), main_vbox, TRUE, TRUE,
			    0);

	g_signal_connect (rww->win, "delete-event",
			  G_CALLBACK (closeMemoryWatchWindow), rww);
	g_signal_connect (rww->win, "response",
			  G_CALLBACK (closeMemoryWatchWindow), rww);

	gtk_widget_show_all (rww->win);

}
