#include<gtk/gtk.h>
#include<gdk/gdkx.h>

#include<SDL/SDL.h>

#include "../common/configSys.h"
#include "sdl.h"
#include "gui.h"

//#define WIDTH 600
//#define HEIGHT 600


extern Config *g_config;

// test rendering
//SDL_Surface* screen = NULL;
//SDL_Surface* hello = NULL;



// we're not using this loop right now since integrated sdl is broken 
gint mainLoop(gpointer data)
{
	// test render
	/*
	SDL_UpdateRect(screen, 0, 0, xres, yres);
	
	
	SDL_BlitSurface (hello, NULL, screen, NULL);
	
	SDL_Flip( screen );
	*/
	DoFun(0);
	
	return TRUE;
}

// this is not used currently; it is used in rendering sdl in
// the gtk window which is broken
gint configureEvent (GtkWidget* widget, GdkEventConfigure* event)
{
	//neccessary for SDL rendering on gtk win (i think?)
	//s_screen = SDL_SetVideoMode(event->width, event->height, 0, 0);
	
	return TRUE;
}

int InitGTKSubsystem(int argc, char** argv)
{
	GtkWidget* MainWindow;
	int xres, yres;
	
	g_config->getOption("SDL.XResolution", &xres);
    g_config->getOption("SDL.YResolution", &yres);
	
	gtk_init(&argc, &argv);
	
	MainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(MainWindow), "fceuX GTK GUI - WIP");
	gtk_widget_set_usize(MainWindow, xres, yres);
	gtk_widget_realize(MainWindow);
	
	// event handlers
	gtk_widget_add_events(MainWindow, GDK_BUTTON_PRESS_MASK);
	//gtk_signal_connect(GTK_OBJECT(MainWindow), "configure_event",
	//	GTK_SIGNAL_FUNC(configureEvent), 0);
	
	
	// PRG: this code here is the the windowID "hack" to render SDL
	// in a GTK window.  however, I can't get it to work right now
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
	g_signal_connect(G_OBJECT(MainWindow), "delete-event", gtk_main_quit, NULL);
	//gtk_idle_add(mainLoop, MainWindow);
	
	gtk_widget_show_all(MainWindow);
	
	
	return 0;
}
