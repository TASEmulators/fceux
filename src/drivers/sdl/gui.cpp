#include<gtk/gtk.h>
#include<gdk/gdkx.h>

#include<SDL/SDL.h>

#include "../../driver.h"
#include "../../version.h"

#include "../common/configSys.h"
#include "sdl.h"
#include "gui.h"
#include "dface.h"
#include "input.h"

#ifdef _S9XLUA_H
#include "../../fceulua.h"
#endif

extern Config *g_config;

GtkWidget* MainWindow = NULL;

int configGamepadButton(GtkButton* button, gpointer p)
{
	int x = GPOINTER_TO_INT(p);
	int padNo = 0;
    char buf[256];
    std::string prefix;
    
    ButtonConfigBegin();
    
    snprintf(buf, 256, "SDL.Input.GamePad.%d", padNo);
    prefix = buf;
    ConfigButton("Press key twice to bind...", &GamePadConfig[padNo][x]);

    g_config->setOption(prefix + GamePadNames[x], GamePadConfig[padNo][x].ButtonNum[0]);

    if(GamePadConfig[padNo][x].ButtType[0] == BUTTC_KEYBOARD)
    {
		g_config->setOption(prefix + "DeviceType", "Keyboard");
    } else if(GamePadConfig[padNo][x].ButtType[0] == BUTTC_JOYSTICK) {
        g_config->setOption(prefix + "DeviceType", "Joystick");
    } else {
        g_config->setOption(prefix + "DeviceType", "Unknown");
    }
    g_config->setOption(prefix + "DeviceNum", GamePadConfig[padNo][0].DeviceNum[0]);

    ButtonConfigEnd();
    
    return 0;
}

// TODO:  Implement something for gamepads 1 - 4
// shouldnt be hard but im lazy right now
void openGamepadConfig()
{
	GtkWidget* win;
	GtkWidget* vbox;
	GtkWidget* buttons[10];
	
	win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(win), "Gamepad 1 Config");
	gtk_widget_set_size_request(win, 250, 600);
	vbox = gtk_vbox_new(TRUE, 2);
	for(int i=0; i<10; i++)
	{
		buttons[i] = gtk_button_new_with_label(GamePadNames[i]);
		gtk_box_pack_start(GTK_BOX(vbox), buttons[i], TRUE, TRUE, 5);
		gtk_signal_connect(GTK_OBJECT(buttons[i]), "clicked", G_CALLBACK(configGamepadButton), GINT_TO_POINTER(i));	
	}
	
	gtk_container_add(GTK_CONTAINER(win), vbox);
	
	gtk_widget_show_all(win);
	
	return;
}

void quit ()
{
	FCEUI_Kill();
	SDL_Quit();
	gtk_main_quit();
	exit(0);
}


GtkWidget* aboutDialog;

inline void quitAbout(void) { gtk_widget_hide_all(aboutDialog);}

void showAbout ()
{
	
	aboutDialog = gtk_about_dialog_new ();
	gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(aboutDialog), "About fceuX");
	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(aboutDialog), "fceuX");
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(aboutDialog), FCEU_VERSION_STRING);
	gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(aboutDialog), "GPL-2; See COPYING");
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(aboutDialog), "http://fceux.com");
	
	
	gtk_widget_show_all(GTK_WIDGET(aboutDialog));
	
	g_signal_connect(G_OBJECT(aboutDialog), "delete-event", quitAbout, NULL);

	

}
GtkWidget* prefsWin;

void toggleSound(GtkWidget* check, gpointer data)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check)))
	{
		g_config->setOption("SDL.Sound", 1);
		InitSound();
	}
	else
	{
		g_config->setOption("SDL.Sound", 0);
		KillSound();
	}
}

void closePrefs(void)
{
	gtk_widget_hide_all(prefsWin);
	//g_free(prefsWin);
}

void openPrefs(void)
{
	
	GtkWidget* vbox;
	GtkWidget* hbox;
	GtkWidget* someLabel;
	GtkWidget* someCheck;
	GtkWidget* closeButton;
	
	prefsWin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(prefsWin), "Preferences");
	vbox = gtk_vbox_new(TRUE, 5);
	hbox = gtk_hbox_new(TRUE, 5);
	someLabel = gtk_label_new("Enable something: ");
	someCheck = gtk_check_button_new();
	closeButton = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	
	gtk_container_add(GTK_CONTAINER(prefsWin), vbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX(hbox), someLabel, TRUE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX(hbox), someCheck, TRUE, TRUE, 5);
	gtk_box_pack_end(GTK_BOX(vbox), closeButton, TRUE, TRUE, 5);
	
	gtk_widget_show_all(prefsWin);
	
	gtk_signal_connect(GTK_OBJECT(prefsWin), "delete-event", G_CALLBACK(closePrefs), NULL);
	gtk_signal_connect(GTK_OBJECT(closeButton), "clicked", closePrefs, NULL);
	
	
}

void emuReset ()
{
	
}

void emuPause ()
{
	FCEUI_SetEmulationPaused(1);
}
void emuResume ()
{
	FCEUI_SetEmulationPaused(0);
}

void enableFullscreen ()
{
	ToggleFS();
}

#ifdef _S9XLUA_H
void loadLua ()
{
	GtkWidget* fileChooser;
	
	fileChooser = gtk_file_chooser_dialog_new ("Open LUA Script", GTK_WINDOW(MainWindow),
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	
	if (gtk_dialog_run (GTK_DIALOG (fileChooser)) ==GTK_RESPONSE_ACCEPT)
	{
		char* filename;
		
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (fileChooser));
		FCEU_LoadLuaCode(filename);
		g_free(filename);
	}
	gtk_widget_destroy (fileChooser);
}
#endif

void loadGame ()
{
	GtkWidget* fileChooser;
	
	fileChooser = gtk_file_chooser_dialog_new ("Open ROM", GTK_WINDOW(MainWindow),
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	
	if (gtk_dialog_run (GTK_DIALOG (fileChooser)) ==GTK_RESPONSE_ACCEPT)
	{
		char* filename;
		
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (fileChooser));
		LoadGame(filename);
		g_free(filename);
	}
	gtk_widget_destroy (fileChooser);

}

void closeGame() { CloseGame(); }

// this is not used currently; it is used in rendering sdl in
// the gtk window which is broken
gint configureEvent (GtkWidget* widget, GdkEventConfigure* event)
{
	//neccessary for SDL rendering on gtk win (i think?)
	//s_screen = SDL_SetVideoMode(event->width, event->height, 0, 0);
	
	return TRUE;
}

void saveStateAs()
{
	GtkWidget* fileChooser;
	
	fileChooser = gtk_file_chooser_dialog_new ("Save State As", GTK_WINDOW(MainWindow),
			GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	
	if (gtk_dialog_run (GTK_DIALOG (fileChooser)) ==GTK_RESPONSE_ACCEPT)
	{
		char* filename;
		
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (fileChooser));
		FCEUI_SaveState(filename);
		g_free(filename);
	}
	gtk_widget_destroy (fileChooser);
	
	
}

void loadStateFrom()
{
	GtkWidget* fileChooser;
	
	fileChooser = gtk_file_chooser_dialog_new ("Load State From", GTK_WINDOW(MainWindow),
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	
	if (gtk_dialog_run (GTK_DIALOG (fileChooser)) ==GTK_RESPONSE_ACCEPT)
	{
		char* filename;
		
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (fileChooser));
		FCEUI_LoadState(filename);
		g_free(filename);
	}
	gtk_widget_destroy (fileChooser);
	
	
}
	

/* Our menu, an array of GtkItemFactoryEntry structures that defines each menu item */
static GtkItemFactoryEntry menu_items[] = {
  { "/_File",         NULL,         NULL,           0, "<Branch>" },
  //{ "/File/_New",     "<control>N", NULL,    0, "<StockItem>", GTK_STOCK_NEW },
  { "/File/_Open ROM",    "<control>O", loadGame,    0, "<StockItem>", GTK_STOCK_OPEN },
  { "/File/_Close ROM",    "<control>C", closeGame,    0, "<StockItem>", GTK_STOCK_CLOSE },
 // { "/File/Save _As", NULL,         NULL,           0, "<Item>" },
  { "/File/sep1",     NULL,         NULL,           0, "<Separator>" },
  { "/File/Savestate", NULL, NULL, 0, "<Branch>" },
  { "/File/Savestate/Load State _From", NULL, loadStateFrom, 0, "<Item>"},
  { "/File/Savestate/Save State _As", NULL, saveStateAs, 0, "<Item>"},
#ifdef _S9XLUA_H  
  { "/File/Load _Lua Script", NULL, loadLua, 0, "<Item>"},
#endif
  { "/File/_Screenshot", "F12", FCEUI_SaveSnapshot, 0, "<Item>"},

  { "/File/_Quit",    "<CTRL>Q", quit, 0, "<StockItem>", GTK_STOCK_QUIT },
  { "/_Options",      NULL,         NULL,           0, "<Branch>" },
  { "/_Emulator",	NULL,			NULL,			0, "<Branch>"  },
  { "/Emulator/_Reset", 	NULL, emuReset, 0, "<Item>"},
  { "/Emulator/_Pause", NULL, emuPause, 0, "<Item>"},
  { "/Emulator/R_esume", NULL, emuResume, 0, "<Item>"},
  { "/Options/_Preferences", "<CTRL>P" , openPrefs, 0, "<StockItem>", GTK_STOCK_PREFERENCES },
  { "/Options/_Gamepad Config", NULL , openGamepadConfig, 0, "<StockItem>", GTK_STOCK_PREFERENCES },
  { "/Options/tear",  NULL,         NULL,           0, "<Tearoff>" },
  { "/Options/_Fullscreen", NULL,         enableFullscreen,	   0, "<Item>" },
 // { "/Options/sep",   NULL,         NULL,           0, "<Separator>" },
 // { "/Options/Rad1",  NULL,         NULL,			1, "<RadioItem>" },
 // { "/Options/Rad2",  NULL,         NULL,			2, "/Options/Rad1" },
 // { "/Options/Rad3",  NULL,         NULL,			3, "/Options/Rad1" },
  { "/_Help",         NULL,         NULL,           0, "<LastBranch>" },
  { "/_Help/About",   NULL,         showAbout,           0, "<Item>" },
};

static gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);

static GtkWidget* CreateMenubar( GtkWidget* window)
{
	GtkItemFactory *item_factory;
	GtkAccelGroup *accel_group;

	/* Make an accelerator group (shortcut keys) */
	accel_group = gtk_accel_group_new ();

	/* Make an ItemFactory (that makes a menubar) */
	item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", accel_group);

	/* This function generates the menu items. Pass the item factory,
		 the number of items in the array, the array itself, and any
		 callback data for the the menu items. */
	gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, NULL);

	/* Attach the new accelerator group to the window. */
	gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

	/* Finally, return the actual menu bar created by the item factory. */
	return gtk_item_factory_get_widget (item_factory, "<main>");
}


int InitGTKSubsystem(int argc, char** argv)
{
	//GtkWidget* MainWindow;
	GtkWidget* Menubar;
	GtkWidget* vbox;
	GtkWidget* soundLabel;
	GtkWidget* soundCheck;
	GtkWidget* soundHbox;
	int xres, yres;
	
	g_config->getOption("SDL.XResolution", &xres);
		g_config->getOption("SDL.YResolution", &yres);
	
	gtk_init(&argc, &argv);
	
	MainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(MainWindow), "FceuX");
	gtk_window_set_default_size(GTK_WINDOW(MainWindow), 359, 200);
	
	vbox = gtk_vbox_new(FALSE, 3);
	soundHbox = gtk_hbox_new(FALSE, 5);
	soundLabel = gtk_label_new("Enable sound: ");
	gtk_container_add(GTK_CONTAINER(MainWindow), vbox);
	
	Menubar = CreateMenubar(MainWindow);
	
	soundCheck = gtk_check_button_new();
	
	int s;
	g_config->getOption("SDL.Sound", &s);
	if(s)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(soundCheck), TRUE);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(soundCheck), FALSE);
		
	//gtk_container_add(GTK_CONTAINER(vbox), Menubar);
	gtk_box_pack_start (GTK_BOX(vbox), Menubar, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), soundHbox, TRUE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(soundHbox), soundLabel, FALSE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX(soundHbox), soundCheck, FALSE, TRUE, 5);

	
	
	// broken SDL embedding code
	//gtk_widget_set_usize(MainWindow, xres, yres);
	//gtk_widget_realize(MainWindow);
	
	// event handlers
	gtk_widget_add_events(MainWindow, GDK_BUTTON_PRESS_MASK);
	//gtk_signal_connect(GTK_OBJECT(MainWindow), "configure_event",
	//	GTK_SIGNAL_FUNC(configureEvent), 0);
	
	
	// PRG: this code here is the the windowID "hack" to render SDL
	// in a GTK window.	however, I can't get it to work right now
	// so i'm commenting it out and haivng a seperate GTK2 window with 
	// controls
	// 12/21/09
	/*
	GtkWidget* socket = gtk_socket_new();
	gtk_widget_show (socket) ; 
	gtk_container_add (GTK_CONTAINER(MainWindow), socket);
	
	gtk_widget_realize (socket);

	char SDL_windowhack[24];
	sprintf(SDL_windowhack, "SDL_WINDOWID=%ld", (long int)gtk_socket_get_id (GTK_SOCKET(socket)));
	putenv(SDL_windowhack); 
	
	
	// init SDL
	if ( SDL_Init(SDL_INIT_VIDEO) < 0 )
	{
		fprintf(stderr, "Couldn't init SDL: %s\n", SDL_GetError());
		gtk_main_quit();
	}
	
	
	
	// test rendering
	//screen = SDL_SetVideoMode(xres, yres, 0, 0);
	//hello = SDL_LoadBMP( "hello.bmp" );
	*/
	
	// signal handlers
	g_signal_connect(G_OBJECT(MainWindow), "delete-event", quit, NULL);
	
	gtk_signal_connect(GTK_OBJECT(soundCheck), "clicked", G_CALLBACK(toggleSound), NULL);
	//gtk_idle_add(mainLoop, MainWindow);
	gtk_widget_set_size_request (GTK_WIDGET(MainWindow), 300, 200);

	gtk_widget_show_all(MainWindow);
	
	
	return 0;
}

