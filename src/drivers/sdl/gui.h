//      gui.h
//      
//      Copyright 2009 Lukas <lukas@LTx3-64>
//      
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; either version 2 of the License, or
//      (at your option) any later version.
//      
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//      
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
//      MA 02110-1301, USA.

#ifndef FCEUX_GUI_H
#define FCEUX_GUI_H

#define GTK

#ifdef _GTK
#include <gtk/gtk.h>
#endif
extern GtkWidget* MainWindow;
extern GtkWidget* evbox;
extern int GtkMouseData[3];
extern bool gtkIsStarted;

int InitGTKSubsystem(int argc, char** argv);
void pushOutputToGTK(const char* str);
void showGui(bool b);
void toggleMenuVis(void);

gint convertKeypress (GtkWidget * grab, GdkEventKey * event, gpointer user_data);
void toggleOption (GtkWidget * w, gpointer p);
void setCheckbox (GtkWidget * w, const char *configName);

bool checkGTKVersion(int major_required, int minor_required);

int configHotkey(char* hotkeyString);

void resetVideo();
void openPaletteConfig();

void openNetworkConfig();
void flushGtkEvents();

void openHotkeyConfig();
void openGamepadConfig();

void resizeGtkWindow();

void setStateMenuItem( int i );

void openVideoConfig();
void openSoundConfig();
void openAbout ();

void emuReset ();
void hardReset ();
void enableFullscreen ();

void recordMovie();
void recordMovieAs ();
void loadMovie ();
#ifdef _S9XLUA_H
void loadLua ();
#endif
void loadFdsBios ();

void enableGameGenie(int enabled);
void loadGameGenie ();

void loadNSF ();
void closeGame();
void loadGame ();
void saveStateAs();
void loadStateFrom();
void quickLoad();
void quickSave();
unsigned int GDKToSDLKeyval(int gdk_key);
int InitGTKSubsystem(int argc, char** argv);

uint32_t *getGuiPixelBuffer( int *w, int *h, int *s );
int  guiPixelBufferReDraw(void);

enum videoDriver_t
{
	VIDEO_NONE = -1,
	VIDEO_OPENGL_GLX,
	VIDEO_SDL,
	VIDEO_CAIRO
};
extern enum videoDriver_t  videoDriver;

int init_gui_video( videoDriver_t vd );
int destroy_gui_video( void );
void init_cairo_screen(void);
void destroy_cairo_screen(void);

#endif // ifndef FCEUX_GUI_H
