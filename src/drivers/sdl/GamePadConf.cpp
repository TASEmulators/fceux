// GamePadConf.cpp
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>

#include <gtk/gtk.h>

#include "sdl/GamePadConf.h"
#include "sdl/main.h"
#include "sdl/dface.h"
#include "sdl/input.h"
#include "sdl/config.h"
#include "sdl/keyscan.h"
#include "sdl/sdl-joystick.h"
#include "sdl/gui.h"

extern Config *g_config;
static GtkWidget *gamePadConfwin = NULL;
static GtkWidget *padNoCombo = NULL;
static GtkWidget *devSelCombo = NULL;
static GtkWidget *mapProfCombo = NULL;
static GtkWidget *buttonMappings[GAMEPAD_NUM_BUTTONS] = { NULL };
static GtkWidget *buttonState[GAMEPAD_NUM_BUTTONS] = { NULL };
static GtkWidget *guidLbl = NULL;
static GtkWidget *msgLbl = NULL;
static int buttonConfigStatus = 0;
static int padNo = 0;
static int numProfiles = 0;

struct GamePadConfigLocalData_t
{
	std::string  guid;
	std::string  profile;

	struct {

		char needsSave;

	} btn[GAMEPAD_NUM_BUTTONS];

	GamePadConfigLocalData_t(void)
	{
		for (int i=0; i<GAMEPAD_NUM_BUTTONS; i++)
		{
			btn[i].needsSave = 0;
		}
	}

};

static GamePadConfigLocalData_t lcl[GAMEPAD_NUM_DEVICES];

static int  getDeviceIndex( void )
{
	int devIdx = -1;
	const char *s;

	s = gtk_combo_box_text_get_active_text ( GTK_COMBO_BOX_TEXT(devSelCombo) );

	if ( s && isdigit( s[0] ) )
	{
		devIdx = atoi( s );
	}
	return devIdx;
}

static void updateCntrlrDpy(void)
{
	int i;
	char strBuf[128];

	if ( (padNoCombo == NULL) )
	{
		return;
	}

	for (i = 0; i < GAMEPAD_NUM_BUTTONS; i++)
	{
		GtkWidget *mappedKey = buttonMappings[i];
		if (GamePad[padNo].bmap[i].ButtType == BUTTC_KEYBOARD)
		{
			snprintf (strBuf, sizeof (strBuf), "<tt>%s</tt>",
				  SDL_GetKeyName (GamePad[padNo].bmap[i].
						  ButtonNum));
		}
		else		
			sprintf (strBuf, "<tt>%s</tt>", ButtonName( &GamePad[padNo].bmap[i] ) );

		if ( mappedKey != NULL )
		{
			gtk_label_set_text (GTK_LABEL (mappedKey), strBuf);
			gtk_label_set_use_markup (GTK_LABEL (mappedKey), TRUE);
		}
	}
}

static void loadMapList(void)
{
	const char *baseDir = FCEUI_GetBaseDirectory();
   const char *guid;
   std::string path;
	std::string prefix, mapName;
   int devIdx = -1;
   jsDev_t *js;
	size_t i,n=0;
	char stmp[256];
	struct dirent *d;
	DIR *dir;

	devIdx = getDeviceIndex();

   if ( devIdx < 0 )
   {
      guid = "keyboard";
   }
   else
   {
      js = getJoystickDevice( devIdx );

      guid = js->getGUID();
   }

   if ( guid == NULL )
   {
      return;
   }

   path = std::string(baseDir) + "/input/" + std::string(guid);

	sprintf( stmp, "SDL.Input.GamePad.%i.", padNo );
	prefix = stmp;

	g_config->getOption(prefix + "Profile", &mapName );

	gtk_combo_box_text_remove_all( GTK_COMBO_BOX_TEXT(mapProfCombo) );
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (mapProfCombo), "default");
	gtk_combo_box_set_active (GTK_COMBO_BOX (mapProfCombo), 0);

	numProfiles = n = 1;
	dir = ::opendir( path.c_str() );

	if ( dir == NULL ) return;

	d = ::readdir( dir );

	while  ( d != NULL ) 
	{
		i=0;
		while ( d->d_name[i] != 0 )
		{
			if ( d->d_name[i] == '.' )
			{
				break;
			}
			stmp[i] = d->d_name[i]; i++;
		}
		stmp[i] = 0;

		//printf("Directory: '%s'\n", stmp );

		if ( i > 0 )
		{
			if ( strcmp( stmp, "default" ) != 0 )
			{
				gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (mapProfCombo), stmp); 

				if ( mapName.compare(stmp) == 0 )
				{
					gtk_combo_box_set_active (GTK_COMBO_BOX (mapProfCombo), n);
				}
				n++;
			}
		}

		d = ::readdir( dir );
	}
	numProfiles = n;

	::closedir( dir );

}

static void selPortChanged( GtkWidget * w, gpointer p )
{
	const char *txt;

	txt = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT (padNoCombo));

	if ( txt == NULL )
	{
		return;
	}
	padNo = atoi(txt) - 1;

	GtkTreeModel *treeModel = gtk_combo_box_get_model( GTK_COMBO_BOX (devSelCombo) );
	GtkTreeIter iter;
	gboolean iterValid;

	iterValid = gtk_tree_model_get_iter_first( treeModel, &iter );

	while ( iterValid )
	{
		GValue value;

		memset( &value, 0, sizeof(value));

		gtk_tree_model_get_value (treeModel, &iter, 0, &value );

		if ( G_IS_VALUE(&value) )
		{
			if ( G_VALUE_TYPE(&value) == G_TYPE_STRING )
			{
				int devIdx = -1;
				const char *s = (const char *)g_value_peek_pointer( &value );

				if ( isdigit( s[0] ) )
				{
					devIdx = atoi(s);
				}
				if ( (devIdx >= 0) && (devIdx == GamePad[padNo].getDeviceIndex() ) )
				{
				 	gtk_combo_box_set_active_iter (GTK_COMBO_BOX (devSelCombo), &iter);
				}
			}
		}
		g_value_unset(&value);

		iterValid = gtk_tree_model_iter_next( treeModel, &iter );
	}
	gtk_label_set_text( GTK_LABEL(guidLbl), GamePad[padNo].getGUID() );

	loadMapList();

	updateCntrlrDpy();
}

static void selInputDevice (GtkWidget * w, gpointer p)
{
	//std::string s = "SDL.Input.";
	int devIdx = -1;
	jsDev_t *js;

	if ( (padNoCombo == NULL) )
	{
		return;
	}

	devIdx = getDeviceIndex();

	js = getJoystickDevice( devIdx );

	if ( js != NULL )
	{
		if ( js->isConnected() )
		{
			gtk_label_set_text( GTK_LABEL(guidLbl), js->getGUID() );
		}
	}
	else
	{
		gtk_label_set_text( GTK_LABEL(guidLbl), "keyboard" );
	}
	GamePad[padNo].setDeviceIndex( devIdx );

	lcl[padNo].guid.assign( GamePad[padNo].getGUID() );
	lcl[padNo].profile.assign("default");
			
	loadMapList();

	updateCntrlrDpy();

	return;
}

static void saveConfig(void)
{
	int i;
	char stmp[256];
	const char *txt;
	std::string prefix, mapName;

	sprintf( stmp, "SDL.Input.GamePad.%i.", padNo );
	prefix = stmp;

	txt = gtk_combo_box_text_get_active_text( GTK_COMBO_BOX_TEXT(mapProfCombo) );

	if ( txt == NULL )
	{
		return;
	}
   mapName.assign( txt );

	g_config->setOption(prefix + "DeviceGUID", GamePad[padNo].getGUID() );
	g_config->setOption(prefix + "Profile"   , mapName.c_str()            );

	for (i=0; i<GAMEPAD_NUM_BUTTONS; i++)
	{
		lcl[padNo].btn[i].needsSave = 0;
	}
	g_config->save();
}

static void createNewProfile( const char *name )
{
	char stmp[256];
   //printf("Creating: %s \n", name );

   GamePad[padNo].createProfile(name);

	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (mapProfCombo), name);
	numProfiles++;

	gtk_combo_box_set_active( GTK_COMBO_BOX (mapProfCombo), numProfiles - 1 );

	saveConfig();

   sprintf( stmp, "Mapping Created: %s/%s \n", GamePad[padNo].getGUID(), name );
   gtk_label_set_text( GTK_LABEL(msgLbl), stmp );

	loadMapList();
}

static void loadProfileCB (GtkButton * button, gpointer p)
{
	char stmp[256];
   int devIdx, ret;
   std::string mapName;
	const char *txt;

	devIdx = getDeviceIndex();

	txt = gtk_combo_box_text_get_active_text( GTK_COMBO_BOX_TEXT(mapProfCombo) );

	if ( txt == NULL )
	{
		return;
	}
   mapName.assign( txt );

	GamePad[padNo].setDeviceIndex( devIdx );

   if ( mapName.compare("default") == 0 )
   {
      ret = GamePad[padNo].loadDefaults();
   }
   else
   {
      ret = GamePad[padNo].loadProfile( mapName.c_str() );
   }
   if ( ret == 0 )
   {
		saveConfig();

      sprintf( stmp, "Mapping Loaded: %s/%s \n", GamePad[padNo].getGUID(), mapName.c_str() );
   }
   else
   {
      sprintf( stmp, "Error: Failed to Load Mapping: %s/%s \n", GamePad[padNo].getGUID(), mapName.c_str() );
   }
   gtk_label_set_text( GTK_LABEL(msgLbl), stmp );

	updateCntrlrDpy();
}

static void saveProfileCB (GtkButton * button, gpointer p)
{
	int ret;
   std::string mapName;
   char stmp[256];
	const char *txt;

	txt = gtk_combo_box_text_get_active_text( GTK_COMBO_BOX_TEXT(mapProfCombo) );

	if ( txt == NULL )
	{
		return;
	}
   mapName.assign( txt );

   ret = GamePad[padNo].saveCurrentMapToFile( mapName.c_str() );

   if ( ret == 0 )
   {
		saveConfig();

      sprintf( stmp, "Mapping Saved: %s/%s \n", GamePad[padNo].getGUID(), mapName.c_str() );
   }
   else
   {
      sprintf( stmp, "Error: Failed to Save Mapping: %s \n", mapName.c_str() );
   }
   gtk_label_set_text( GTK_LABEL(msgLbl), stmp );
}

static void newProfileCB (GtkButton * button, gpointer p)
{
	int ret;
	GtkWidget *vbox, *lbl, *entry;
	GtkWidget *dialog
		= gtk_dialog_new_with_buttons ("New Profile", GTK_WINDOW(gamePadConfwin),
                             (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
                             "_Cancel", GTK_RESPONSE_CANCEL, "_Create", GTK_RESPONSE_ACCEPT, NULL );

	vbox = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
	gtk_box_set_homogeneous (GTK_BOX (vbox), FALSE);

	lbl = gtk_label_new("Specify New Profile Name");
	entry = gtk_entry_new();

	gtk_box_pack_start (GTK_BOX (vbox), lbl, TRUE, TRUE, 5);
	gtk_box_pack_start (GTK_BOX (vbox), entry, TRUE, TRUE, 5);

	gtk_widget_show_all( dialog );

	ret = gtk_dialog_run( GTK_DIALOG(dialog) );

	if ( ret == GTK_RESPONSE_ACCEPT )
	{
		printf("Text: '%s'\n", gtk_entry_get_text( GTK_ENTRY(entry) ) );

		createNewProfile( gtk_entry_get_text( GTK_ENTRY(entry) ) );
	}

	gtk_widget_destroy( dialog );
}

static void deleteProfileCB (GtkButton * button, gpointer p)
{
	int ret;
   std::string mapName;
   char stmp[256];
	const char *txt;

	txt = gtk_combo_box_text_get_active_text( GTK_COMBO_BOX_TEXT(mapProfCombo) );

	if ( txt == NULL )
	{
		return;
	}
   mapName.assign( txt );

   ret = GamePad[padNo].deleteMapping( mapName.c_str() );

   if ( ret == 0 )
   {
      sprintf( stmp, "Mapping Deleted: %s/%s \n", GamePad[padNo].getGUID(), mapName.c_str() );
   }
   else
   {
      sprintf( stmp, "Error: Failed to Delete Mapping: %s \n", mapName.c_str() );
   }
   gtk_label_set_text( GTK_LABEL(msgLbl), stmp );

   loadMapList();
}

// This function configures a single button on a gamepad
static void clearGamepadButton (GtkButton * button, gpointer p)
{
	long int x = (long int)p;

	GamePad[padNo].bmap[x].ButtonNum = -1;

	gtk_label_set_text (GTK_LABEL (buttonMappings[x]), "");

	lcl[padNo].btn[x].needsSave = 1;

	updateCntrlrDpy();
}

// This function configures a single button on a gamepad
static void configGamepadButton (GtkButton * button, gpointer p)
{
	gint x = ((gint) (glong) (p));
	//gint x = GPOINTER_TO_INT(p);

	char buf[256];
	std::string prefix;

	// only configure when the "Change" button is pressed in, not when it is unpressed
	if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
		return;

	gtk_button_set_label (GTK_BUTTON (button), "Waiting");

	buttonConfigStatus = 2;

	ButtonConfigBegin ();

	snprintf (buf, sizeof(buf)-1, "SDL.Input.GamePad.%d.", padNo);
	prefix = buf;
	DWaitButton (NULL, &GamePad[padNo].bmap[x], &buttonConfigStatus );

//	g_config->setOption (prefix + GamePadNames[x],
//			     GamePadConfig[padNo][x].ButtonNum[configNo]);
//
//	if (GamePadConfig[padNo][x].ButtType[0] == BUTTC_KEYBOARD)
//	{
//		g_config->setOption (prefix + "DeviceType", "Keyboard");
//	}
//	else if (GamePadConfig[padNo][x].ButtType[0] == BUTTC_JOYSTICK)
//	{
//		g_config->setOption (prefix + "DeviceType", "Joystick");
//	}
//	else
//	{
//		g_config->setOption (prefix + "DeviceType", "Unknown");
//	}
//	g_config->setOption (prefix + "DeviceNum",
//			     GamePadConfig[padNo][x].DeviceNum[configNo]);

	snprintf (buf, sizeof (buf), "<tt>%s</tt>",
		  ButtonName (&GamePad[padNo].bmap[x]));

	if ( buttonMappings[x] != NULL )
	{
		gtk_label_set_markup (GTK_LABEL (buttonMappings[x]), buf);
	}
	lcl[padNo].btn[x].needsSave = 1;

	ButtonConfigEnd ();

	buttonConfigStatus = 1;

	gtk_button_set_label (GTK_BUTTON (button), "Change");

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), FALSE);

	return;
}

//static void updateGamepadConfig (GtkWidget * w, gpointer p)
//{
//	updateCntrlrDpy();
//}

static gint timeout_callback (gpointer data)
{
	if ( gamePadConfwin == NULL )
	{
		return FALSE;
	}

	for (int i=0; i<GAMEPAD_NUM_BUTTONS; i++)
   {
      const char *txt; 
      if ( GamePad[padNo].bmap[i].state )
      {
			txt = "<b><span background='green' foreground='white'> T </span></b>";
      }
      else
      {
			txt = "<b><span background='red' foreground='white'> F </span></b>";
      }

		gtk_label_set_markup( GTK_LABEL( buttonState[i] ), txt );

		//if ( lcl[portNum].btn[i].needsSave )
		//{
      //	keyName[i]->setStyleSheet("color: red;");
		//}
		//else
		//{
      //	keyName[i]->setStyleSheet("color: black;");
		//}
   }

	return TRUE;
}

static void closeGamepadConfig (GtkWidget * w, GdkEvent * e, gpointer p)
{
	gtk_widget_destroy (w);

	gamePadConfwin = NULL;
	padNoCombo     = NULL;

	for (int i = 0; i < GAMEPAD_NUM_BUTTONS; i++)
	{
		buttonMappings[i] = NULL;
		buttonState[i] = NULL;
	}
	buttonConfigStatus = 0;
}

void openGamepadConfig (void)
{
	GtkWidget *win;
	GtkWidget *mainVbox;
	GtkWidget *hbox, /* *vbox,*/ *lbl;
	GtkWidget *hboxPadNo;
	GtkWidget *padNoLabel;
	GtkWidget* devSelLabel, *devSelHbox;
	GtkWidget *fourScoreChk;
	GtkWidget *oppositeDirChk;
	GtkWidget *buttonFrame;
	GtkWidget *buttonTable;
	GtkWidget *button;
	GtkWidget *grid;
	GtkWidget *mappingFrame;
	char stmp[256];

	if ( gamePadConfwin != NULL )
	{
		return;
	}

	// Ensure that joysticks are enabled, no harm calling init again.
	InitJoysticks();

	padNo = 0;

	win = gtk_dialog_new_with_buttons ("Controller Configuration",
					   GTK_WINDOW (MainWindow),
					   (GtkDialogFlags)
					   (GTK_DIALOG_DESTROY_WITH_PARENT),
					   "_Close", GTK_RESPONSE_OK, NULL);
	gtk_window_set_title (GTK_WINDOW (win), "Controller Configuration");
	gtk_window_set_icon_name (GTK_WINDOW (win), "input-gaming");
	gtk_widget_set_size_request (win, 350, 500);

	mainVbox = gtk_dialog_get_content_area (GTK_DIALOG (win));
	gtk_box_set_homogeneous (GTK_BOX (mainVbox), FALSE);

	hboxPadNo = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_set_homogeneous( GTK_BOX(hboxPadNo), TRUE );
	padNoLabel = gtk_label_new ("Port:");
	//configNoLabel = gtk_label_new("Config Number:");
	fourScoreChk = gtk_check_button_new_with_label ("Enable Four Score");
	oppositeDirChk =
		gtk_check_button_new_with_label ("Allow Up+Down / Left+Right");

	//typeCombo = gtk_combo_box_text_new ();
	//gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (typeCombo),
	//				"gamepad");
	//gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (typeCombo),
	//				"zapper");
	//gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (typeCombo),
	//				"powerpad.0");
	//gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (typeCombo),
	//				"powerpad.1");
	//gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (typeCombo),
	//				"arkanoid");

	//gtk_combo_box_set_active (GTK_COMBO_BOX (typeCombo), 0);


	padNoCombo = gtk_combo_box_text_new ();
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (padNoCombo), "1");
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (padNoCombo), "2");
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (padNoCombo), "3");
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (padNoCombo), "4");
	gtk_combo_box_set_active (GTK_COMBO_BOX (padNoCombo), 0);
	g_signal_connect (padNoCombo, "changed",
			  G_CALLBACK (selPortChanged), NULL);

	devSelHbox  = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_set_homogeneous( GTK_BOX(devSelHbox), TRUE );
	devSelLabel = gtk_label_new ("Device:");
	devSelCombo = gtk_combo_box_text_new ();
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (devSelCombo), "Keyboard");

 	gtk_combo_box_set_active (GTK_COMBO_BOX (devSelCombo), 0);

	for (int i=0; i<MAX_JOYSTICKS; i++)
	{
		jsDev_t *js = getJoystickDevice( i );

		if ( js != NULL )
		{
			if ( js->isConnected() )
			{
				sprintf( stmp, "%i: %s", i, js->getName() );
   			gtk_combo_box_text_append_text( GTK_COMBO_BOX_TEXT (devSelCombo), stmp );
			}
		}
	}
 	gtk_combo_box_set_active (GTK_COMBO_BOX (devSelCombo), 0);

	{
		GtkTreeModel *treeModel = gtk_combo_box_get_model( GTK_COMBO_BOX (devSelCombo) );
		GtkTreeIter iter;
		gboolean iterValid;

		iterValid = gtk_tree_model_get_iter_first( treeModel, &iter );

		while ( iterValid )
		{
			GValue value;

			memset( &value, 0, sizeof(value));

			gtk_tree_model_get_value (treeModel, &iter, 0, &value );

			if ( G_IS_VALUE(&value) )
			{
				if ( G_VALUE_TYPE(&value) == G_TYPE_STRING )
				{
					int devIdx = -1;
					const char *s = (const char *)g_value_peek_pointer( &value );

					if ( isdigit( s[0] ) )
					{
						devIdx = atoi(s);
					}
					if ( (devIdx >= 0) && (devIdx == GamePad[padNo].getDeviceIndex() ) )
					{
					 	gtk_combo_box_set_active_iter (GTK_COMBO_BOX (devSelCombo), &iter);
					}
				}
			}
			g_value_unset(&value);

			iterValid = gtk_tree_model_iter_next( treeModel, &iter );
		}
	}
	g_signal_connect (devSelCombo, "changed", G_CALLBACK (selInputDevice), NULL );

	//g_signal_connect (typeCombo, "changed", G_CALLBACK (setInputDevice),
	//		  gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT
	//						      (typeCombo)));

	setCheckbox (fourScoreChk, "SDL.FourScore");
	g_signal_connect (fourScoreChk, "clicked", G_CALLBACK (toggleOption),
			  (gpointer) "SDL.FourScore");
	setCheckbox (oppositeDirChk, "SDL.Input.EnableOppositeDirectionals");
	g_signal_connect (oppositeDirChk, "clicked", G_CALLBACK (toggleOption),
			  (gpointer) "SDL.Input.EnableOppositeDirectionals");


	gtk_box_pack_start (GTK_BOX (hboxPadNo), padNoLabel, TRUE, TRUE, 5);
	gtk_box_pack_start (GTK_BOX (hboxPadNo), padNoCombo, TRUE, TRUE, 5);
	gtk_box_pack_start (GTK_BOX (mainVbox), hboxPadNo, FALSE, TRUE, 5);
	//gtk_box_pack_start_defaults(GTK_BOX(mainVbox), typeCombo);

	gtk_box_pack_start (GTK_BOX (devSelHbox), devSelLabel, TRUE, TRUE, 5);
	gtk_box_pack_start (GTK_BOX (devSelHbox), devSelCombo, TRUE, TRUE, 5);
	gtk_box_pack_start (GTK_BOX (mainVbox), devSelHbox, TRUE, TRUE, 5);

	hbox    = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_set_homogeneous( GTK_BOX(hbox), TRUE );
	lbl     = gtk_label_new ("GUID:");
	guidLbl = gtk_label_new ("");

	gtk_box_pack_start (GTK_BOX (hbox), lbl, TRUE, TRUE, 5);
	gtk_box_pack_start (GTK_BOX (hbox), guidLbl, TRUE, TRUE, 5);
	gtk_box_pack_start (GTK_BOX (mainVbox), hbox, TRUE, TRUE, 5);

	gtk_label_set_text( GTK_LABEL(guidLbl), GamePad[padNo].getGUID() );

	mappingFrame = gtk_frame_new ("<b><i>Mapping Profile:</i></b>");
	gtk_label_set_use_markup (GTK_LABEL
				  (gtk_frame_get_label_widget
				   (GTK_FRAME (mappingFrame))), TRUE);
	gtk_box_pack_start (GTK_BOX (mainVbox), mappingFrame, FALSE, TRUE, 5);
	grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (grid), 5);
	gtk_grid_set_column_spacing (GTK_GRID (grid), 5);
	gtk_grid_set_column_homogeneous (GTK_GRID (grid), TRUE);

	//vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add (GTK_CONTAINER (mappingFrame), grid);

	mapProfCombo = gtk_combo_box_text_new ();
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (mapProfCombo), "default");
 	gtk_combo_box_set_active (GTK_COMBO_BOX (mapProfCombo), 0);
	gtk_grid_attach (GTK_GRID (grid), mapProfCombo, 0, 0, 2, 1 );

	button = gtk_button_new_with_label ("Load");
	gtk_grid_attach (GTK_GRID (grid), button, 0, 1, 1, 1 );
	g_signal_connect (button, "clicked", G_CALLBACK (loadProfileCB), NULL );

	button = gtk_button_new_with_label ("Save");
	gtk_grid_attach (GTK_GRID (grid), button, 1, 1, 1, 1 );
	g_signal_connect (button, "clicked", G_CALLBACK (saveProfileCB), NULL );

	button = gtk_button_new_with_label ("New");
	gtk_grid_attach (GTK_GRID (grid), button, 0, 2, 1, 1 );
	g_signal_connect (button, "clicked", G_CALLBACK (newProfileCB), NULL );

	button = gtk_button_new_with_label ("Delete");
	gtk_grid_attach (GTK_GRID (grid), button, 1, 2, 1, 1 );
	g_signal_connect (button, "clicked", G_CALLBACK (deleteProfileCB), NULL );

	msgLbl = gtk_label_new("");
	gtk_grid_attach (GTK_GRID (grid), msgLbl, 0, 3, 2, 1 );

	gtk_box_pack_start (GTK_BOX (mainVbox), fourScoreChk, FALSE, TRUE, 5);
	gtk_box_pack_start (GTK_BOX (mainVbox), oppositeDirChk, FALSE, TRUE, 5);


	// create gamepad buttons
	buttonFrame = gtk_frame_new ("<b><i>Active Button Mappings:</i></b>");
	gtk_label_set_use_markup (GTK_LABEL
				  (gtk_frame_get_label_widget
				   (GTK_FRAME (buttonFrame))), TRUE);
	buttonTable = gtk_grid_new ();
	gtk_container_add (GTK_CONTAINER (buttonFrame), buttonTable);

	for (int i = 0; i < 3; i++)
	{
		gtk_grid_insert_column (GTK_GRID (buttonTable), i);
	}
	gtk_grid_set_column_spacing (GTK_GRID (buttonTable), 5);
	gtk_grid_set_column_homogeneous (GTK_GRID (buttonTable), TRUE);
	gtk_grid_set_row_spacing (GTK_GRID (buttonTable), 3);

	for (int i = 0; i < GAMEPAD_NUM_BUTTONS; i++)
	{
		GtkWidget *buttonName = gtk_label_new (GamePadNames[i]);
		GtkWidget *mappedKey = gtk_label_new (NULL);
		GtkWidget *changeButton = gtk_toggle_button_new ();
		GtkWidget *clearButton = gtk_button_new ();
		char strBuf[128];

		lbl = gtk_label_new ("State:");
		buttonState[i] = gtk_label_new (" F ");

		gtk_grid_insert_row (GTK_GRID (buttonTable), i);

		sprintf (strBuf, "%s:", GamePadNames[i]);
		gtk_label_set_text (GTK_LABEL (buttonName), strBuf);

		gtk_button_set_label (GTK_BUTTON (changeButton), "Change");
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (changeButton),
					      FALSE);

		gtk_button_set_label (GTK_BUTTON (clearButton), "Clear");

		gtk_grid_attach (GTK_GRID (buttonTable), buttonName, 0, i, 1, 1);
		gtk_grid_attach (GTK_GRID (buttonTable), mappedKey, 1, i, 1, 1);
		gtk_grid_attach (GTK_GRID (buttonTable), lbl, 2, i, 1, 1);
		gtk_grid_attach (GTK_GRID (buttonTable), buttonState[i], 3, i, 1, 1);
		gtk_grid_attach (GTK_GRID (buttonTable), changeButton, 4, i, 1, 1);
		gtk_grid_attach (GTK_GRID (buttonTable), clearButton, 5, i, 1, 1);

		g_signal_connect (changeButton, "clicked",
				  G_CALLBACK (configGamepadButton),
				  GINT_TO_POINTER (i));

		g_signal_connect (clearButton, "clicked",
				  G_CALLBACK (clearGamepadButton),
				  GINT_TO_POINTER (i));

		buttonMappings[i] = mappedKey;
	}

	gtk_box_pack_start (GTK_BOX (mainVbox), buttonFrame, TRUE, TRUE, 5);

	g_signal_connect (win, "delete-event", G_CALLBACK (closeGamepadConfig), NULL);
	g_signal_connect (win, "response", G_CALLBACK (closeGamepadConfig), NULL);

	gtk_widget_show_all (win);

	g_signal_connect (G_OBJECT (win), "key-press-event",
			  G_CALLBACK (convertKeypress), NULL);
	g_signal_connect (G_OBJECT (win), "key-release-event",
			  G_CALLBACK (convertKeypress), NULL);

	buttonConfigStatus = 1;

	gamePadConfwin = win;

	loadMapList();

	// display the button mappings for the currently selected configuration
	updateCntrlrDpy();

	g_timeout_add ( 100, timeout_callback, NULL );

	return;
}

