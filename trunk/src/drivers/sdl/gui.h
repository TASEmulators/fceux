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
#include <gtk/gtk.h>
extern GtkWidget* MainWindow;
extern GtkWidget* evbox;
extern GtkRadioAction* stateSlot;
extern int GtkMouseData[3];
extern bool gtkIsStarted;
int InitGTKSubsystem(int argc, char** argv);
void pushOutputToGTK(const char* str);
void showGui(bool b);
#endif
