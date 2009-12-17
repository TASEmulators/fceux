#include<gtk/gtk.h>
#include<gdk/gdkx.h>

#include<SDL/SDL.h>

#define WIDTH 600
#define HEIGHT 600

GtkWidget* MainWindow;
SDL_Surface* screen = NULL;
SDL_Surface* hello = NULL;

gint mainLoop(gpointer data)
{
	SDL_UpdateRect(screen, 0, 0, WIDTH, HEIGHT);
	// TODO: integrate main SDL loop here
	
	
	SDL_BlitSurface (hello, NULL, screen, NULL);
	
	SDL_Flip( screen );
	return TRUE;
}

gint configureEvent (GtkWidget* widget, GdkEventConfigure* event)
{
	screen = SDL_SetVideoMode(event->width, event->height, 0, 0);
	
	return TRUE;
}

int main(int argc, char** argv)
{
	
	gtk_init(&argc, &argv);
	
	MainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(MainWindow), "fceuX GTK GUI - WIP");
	gtk_widget_set_usize(MainWindow, WIDTH, HEIGHT);
	gtk_widget_realize(MainWindow);
	
	// event handlers
	gtk_widget_add_events(MainWindow, GDK_BUTTON_PRESS_MASK);
	gtk_signal_connect(GTK_OBJECT(MainWindow), "configure_event",
		GTK_SIGNAL_FUNC(configureEvent), 0);
	
	
	
	GtkWidget* socket = gtk_socket_new();
	gtk_widget_show (socket) ; 
	gtk_container_add (GTK_CONTAINER(MainWindow), socket);
	
	gtk_widget_realize (socket);
	
	
	// Hack to get SDL to use GTK window
	// broken as fuck?
	char SDL_windowhack[24];
	sprintf(SDL_windowhack, "SDL_WINDOWID=%ld", (long int)gtk_socket_get_id (GTK_SOCKET(socket)));
	putenv(SDL_windowhack); 
	
	
	// init SDL
	if ( SDL_Init(SDL_INIT_VIDEO) < 0 )
	{
		fprintf(stderr, "Couldn't init SDL: %s\n", SDL_GetError());
		gtk_main_quit();
	}
	
	screen = SDL_SetVideoMode(WIDTH, HEIGHT, 0, 0);
	
	
	hello = SDL_LoadBMP( "hello.bmp" );
	
	SDL_BlitSurface (hello, NULL, screen, NULL);
	
	SDL_Flip( screen );
	
	
	
	
	
	
	// signal handlers
	g_signal_connect(G_OBJECT(MainWindow), "delete-event", gtk_main_quit, NULL);
	gtk_idle_add(mainLoop, MainWindow);
	
	gtk_widget_show_all(MainWindow);
	
	gtk_main();
	SDL_Quit();
	
	return 0;
}
