#include<gtk/gtk.h>
#include<gdk/gdkx.h>

#include<SDL/SDL.h>

#include <fstream>
#include <iostream>
#include <cstdlib>

#include "../../types.h"
#include "../../fceu.h"
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

#ifdef _S9XLUA_H
#include "../../fceulua.h"
#endif

void toggleSound(GtkWidget* check, gpointer data);
void loadGame ();

extern Config *g_config;

GtkWidget* MainWindow = NULL;
GtkWidget* socket = NULL;
GtkWidget* padNoCombo;

// This function configures a single button on a gamepad
int configGamepadButton(GtkButton* button, gpointer p)
{
	gint x = ((gint)(glong)(p));
	//gint x = GPOINTER_TO_INT(p);
	int padNo = 0;
	char* padStr = gtk_combo_box_get_active_text(GTK_COMBO_BOX(padNoCombo));

	if(!strcmp(padStr, "1"))
		padNo = 0;
	if(!strcmp(padStr, "2"))
		padNo = 1;
	if(!strcmp(padStr, "3"))
		padNo = 2;
	if(!strcmp(padStr, "4"))
		padNo = 3;
		
    char buf[256];
    std::string prefix;
    
    ButtonConfigBegin();
    
    snprintf(buf, 256, "SDL.Input.GamePad.%d", padNo);
    prefix = buf;
    ConfigButton((char*)GamePadNames[x], &GamePadConfig[padNo][x]);

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

void toggleLowPass(GtkWidget* w, gpointer p)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)))
	{
		g_config->setOption("SDL.Sound.LowPass", 1);
		FCEUI_SetLowPass(1);
	}
	else
	{
		g_config->setOption("SDL.Sound.LowPass", 0);
		FCEUI_SetLowPass(0);
	}
	g_config->save();
	
}

// Wrapper for pushing GTK options into the config file
// p : pointer to the string that names the config option
// w : toggle widget
void toggleOption(GtkWidget* w, gpointer p)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)))
		g_config->setOption((char*)p, 1);
	else
		g_config->setOption((char*)p, 0);
	g_config->save();
}

int setTint(GtkWidget* w, gpointer p)
{
	int v = gtk_range_get_value(GTK_RANGE(w));
	g_config->setOption("SDL.Tint", v);
	g_config->save();
	int c, h;
	g_config->getOption("SDL.NTSCpalette", &c);
	g_config->getOption("SDL.Hue", &h);
	FCEUI_SetNTSCTH(c, v, h);
	
	return 0;
}
int setHue(GtkWidget* w, gpointer p)
{
	int v = gtk_range_get_value(GTK_RANGE(w));
	g_config->setOption("SDL.Hue", v);
	g_config->save();
	int c, t;
	g_config->getOption("SDL.Tint", &t);
	g_config->getOption("SDL.SDL.NTSCpalette", &c);
	FCEUI_SetNTSCTH(c, t, v);
	
	return 0;
}
void loadPalette (GtkWidget* w, gpointer p)
{
	GtkWidget* fileChooser;
	
	fileChooser = gtk_file_chooser_dialog_new ("Open NES Palette", GTK_WINDOW(MainWindow),
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	
	if (gtk_dialog_run (GTK_DIALOG (fileChooser)) ==GTK_RESPONSE_ACCEPT)
	{
		char* filename;
		
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (fileChooser));
		g_config->setOption("SDL.Palette", filename);
		g_config->setOption("SDL.SDL.NTSCpalette", 0);
		if(LoadCPalette(filename) == 0)
		{
			GtkWidget* msgbox;
			msgbox = gtk_message_dialog_new(GTK_WINDOW(MainWindow), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
			"Failed to load the palette.");
			
			gtk_dialog_run(GTK_DIALOG(msgbox));
			gtk_widget_hide_all(msgbox);
		}
			
		gtk_entry_set_text(GTK_ENTRY(p), filename);
		
	}
	gtk_widget_destroy (fileChooser);
}

void clearPalette(GtkWidget* w, gpointer p)
{
	g_config->setOption("SDL.Palette", 0);
	gtk_entry_set_text(GTK_ENTRY(p), "");
}

void openPaletteConfig()
{
	GtkWidget* win;
	GtkWidget* vbox;
	GtkWidget* paletteFrame;
	GtkWidget* paletteHbox;
	GtkWidget* paletteButton;
	GtkWidget* paletteEntry;
	GtkWidget* clearButton;
	GtkWidget* ntscColorChk;
	GtkWidget* slidersFrame;
	GtkWidget* slidersVbox;
	GtkWidget* tintFrame;
	GtkWidget* tintHscale;
	GtkWidget* hueFrame;
	GtkWidget* hueHscale;
	
	win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(win),"Palette Options");
	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(win), vbox);
	
	gtk_widget_set_size_request(win, 460, 275);
	
	paletteFrame = gtk_frame_new("Custom palette: ");
	paletteHbox = gtk_hbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(paletteHbox), 5);
	gtk_container_add(GTK_CONTAINER(paletteFrame), paletteHbox);
	paletteButton = gtk_button_new_from_stock(GTK_STOCK_OPEN);
	gtk_button_set_label(GTK_BUTTON(paletteButton), "Open palette");
	paletteEntry = gtk_entry_new();
	gtk_entry_set_editable(GTK_ENTRY(paletteEntry), FALSE);
	
	clearButton = gtk_button_new_from_stock(GTK_STOCK_CLEAR);
	
	gtk_box_pack_start(GTK_BOX(paletteHbox), paletteButton, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(paletteHbox), paletteEntry, TRUE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX(paletteHbox), clearButton, FALSE, FALSE, 0);
	
	g_signal_connect(paletteButton, "clicked", G_CALLBACK(loadPalette), paletteEntry);
	g_signal_connect(clearButton, "clicked", G_CALLBACK(clearPalette), paletteEntry);
	
	
	
	// sync with config
	std::string fn;
	g_config->getOption("SDL.Palette", &fn);
	gtk_entry_set_text(GTK_ENTRY(paletteEntry), fn.c_str());
	
	// ntsc color check
	
	ntscColorChk = gtk_check_button_new_with_label("Use NTSC palette");
	
	g_signal_connect(ntscColorChk, "clicked", G_CALLBACK(toggleOption), (gpointer)"SDL.NTSCpalette");
	
	int b;
	// sync with config
	g_config->getOption("SDL.NTSCpalette", &b);
	if(b)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ntscColorChk), 1);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ntscColorChk), 0);
	
	
	// color / tint / hue sliders
	slidersFrame = gtk_frame_new("NTSC palette controls");
	slidersVbox = gtk_vbox_new(FALSE, 2);
	tintFrame = gtk_frame_new("Tint");
	tintHscale = gtk_hscale_new_with_range(0, 128, 1);
	gtk_container_add(GTK_CONTAINER(tintFrame), tintHscale);
	hueFrame = gtk_frame_new("Hue");
	hueHscale = gtk_hscale_new_with_range(0, 128, 1);
	gtk_container_add(GTK_CONTAINER(hueFrame), hueHscale);
	
	g_signal_connect(tintHscale, "button-release-event", G_CALLBACK(setTint), NULL);
	g_signal_connect(hueHscale, "button-release-event", G_CALLBACK(setHue), NULL);
	
	// sync with config
	int h, t;
	g_config->getOption("SDL.Hue", &h);
	g_config->getOption("SDL.Tint", &t);

	gtk_range_set_value(GTK_RANGE(hueHscale), h);
	gtk_range_set_value(GTK_RANGE(tintHscale), t);
	
	gtk_container_add(GTK_CONTAINER(slidersFrame), slidersVbox);
	gtk_box_pack_start(GTK_BOX(slidersVbox), ntscColorChk, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(slidersVbox), tintFrame, FALSE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX(slidersVbox), hueFrame, FALSE, TRUE, 5);
	
	gtk_box_pack_start(GTK_BOX(vbox), paletteFrame, FALSE, TRUE, 5);
	
	gtk_box_pack_start(GTK_BOX(vbox), slidersFrame, FALSE, TRUE, 5);
	
	gtk_widget_show_all(win);
	
	return;
}

GtkWidget* ipEntry;
GtkWidget* portSpin;
GtkWidget* pwEntry;

void launchNet(GtkWidget* w, gpointer p)
{
	char* ip = (char*)gtk_entry_get_text(GTK_ENTRY(ipEntry));
	char* pw = (char*)gtk_entry_get_text(GTK_ENTRY(pwEntry));
	int port = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(portSpin));
	
	g_config->setOption("SDL.NetworkIP", ip);
	g_config->setOption("SDL.NetworkPassword", pw);
	g_config->setOption("SDL.NetworkPort", port);
	
	gtk_widget_destroy(GTK_WIDGET(p));
	
	loadGame();
}
void closeNet(GtkWidget* w, gpointer p)
{
	gtk_widget_destroy(GTK_WIDGET(p));
}

void setUsername(GtkWidget* w, gpointer p)
{
	char* s = (char*)gtk_entry_get_text(GTK_ENTRY(w));
	g_config->setOption("SDL.NetworkUsername", s);
}

void openNetworkConfig()
{
	GtkWidget* win;
	GtkWidget* box;
	GtkWidget* userBox;
	GtkWidget* userEntry;
	GtkWidget* userLbl;
	GtkWidget* frame;
	GtkWidget* vbox;
	GtkWidget* ipBox;
	GtkWidget* ipLbl;
	
	GtkWidget* portBox;
	GtkWidget* portLbl;
	
	//GtkWidget* localPlayersCbo;
	GtkWidget* pwBox;
	GtkWidget* pwLbl;
	
	GtkWidget* bb;
	GtkWidget* conBtn;
	GtkWidget* closeBtn;
	
	win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	box = gtk_vbox_new(FALSE, 3);
	
	userBox = gtk_hbox_new(FALSE, 3);
	userLbl = gtk_label_new("Username:");
	userEntry = gtk_entry_new();
	std::string s;
	g_config->getOption("SDL.NetworkUsername", &s);
	gtk_entry_set_text(GTK_ENTRY(userEntry), s.c_str());
	
	g_signal_connect(userEntry, "changed", G_CALLBACK(setUsername), NULL);

	
	frame = gtk_frame_new("Network options");
	vbox = gtk_vbox_new(FALSE, 5);
	ipBox = gtk_hbox_new(FALSE, 5);
	ipLbl = gtk_label_new("Server IP:");
	ipEntry = gtk_entry_new();
	portBox = gtk_hbox_new(FALSE, 5);
	portLbl = gtk_label_new("Server port:");
	portSpin = gtk_spin_button_new_with_range(0, 999999, 1);
	//localPlayersCbo = gtk_combo_box_new_text();
	pwBox = gtk_hbox_new(FALSE, 3);
	pwLbl = gtk_label_new("Server password:");
	pwEntry = gtk_entry_new();
	bb = gtk_hbox_new(FALSE, 5);
	conBtn = gtk_button_new_from_stock(GTK_STOCK_CONNECT);
	closeBtn = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(portSpin), 4046);
	
	gtk_box_pack_start(GTK_BOX(userBox), userLbl, FALSE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(userBox), userEntry, TRUE , TRUE, 3);
	
	gtk_box_pack_start(GTK_BOX(portBox), portLbl, FALSE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(portBox), portSpin, FALSE , FALSE, 3);
	
	gtk_box_pack_start(GTK_BOX(ipBox), ipLbl, FALSE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(ipBox), ipEntry, TRUE , TRUE, 3);
	
	gtk_box_pack_start(GTK_BOX(pwBox), pwLbl, FALSE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(pwBox), pwEntry, TRUE , TRUE, 3);
	
	gtk_box_pack_start_defaults(GTK_BOX(vbox), ipBox);
	gtk_box_pack_start_defaults(GTK_BOX(vbox), portBox);
	//gtk_box_pack_start_defaults(GTK_BOX(vbox), localPlayersCbo);
	gtk_box_pack_start_defaults(GTK_BOX(vbox), pwBox);
	
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	
	gtk_box_pack_start_defaults(GTK_BOX(box), userBox);
	gtk_box_pack_start_defaults(GTK_BOX(box), frame);
	gtk_box_pack_start(GTK_BOX(bb), closeBtn, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(bb), conBtn, FALSE, FALSE, 5);
	
	gtk_box_pack_start_defaults(GTK_BOX(box), bb);
	gtk_container_add(GTK_CONTAINER(win), box);
	
	gtk_widget_show_all(win);
	
	g_signal_connect(closeBtn, "clicked", G_CALLBACK(closeNet), win);
	g_signal_connect(conBtn, "clicked", G_CALLBACK(launchNet), win);
}

// creates and opens hotkey config window
/*void openHotkeyConfig()
{
	std::string prefix = "SDL.Hotkeys.";
	GtkWidget* win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	enum
	{
	COMMAND_COLUMN,
	KEY_COLUMN,
	N_COLUMNS
	};
	GtkListStore* store = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);
	GtkTreeIter iter;
	
	gtk_list_store_append(store, &iter); // aquire iter
	
	
		int buf;
	for(int i=0; i<HK_MAX; i++)
	{
		//g_config->getOption(prefix + HotkeyStrings[i], &buf);
		gtk_list_store_set(store, &iter, 
				COMMAND_COLUMN, prefix + HotkeyStrings[i], 
				KEY_COLUMN, "TODO", -1);
	}
	GtkWidget* tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Command", renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	
	gtk_container_add(GTK_CONTAINER(win),tree);
	gtk_widget_show_all(win);
}*/
GtkWidget* 	typeCombo;

// TODO: finish this
int setInputDevice(GtkWidget* w, gpointer p)
{
	std::string s = "SDL.Input.";
	s = s + (char*)p;
	printf("%s", s.c_str());
	g_config->setOption(s, gtk_combo_box_get_active_text(GTK_COMBO_BOX(typeCombo)));
	g_config->save();
	
	return 1;
}

// creates and opens the gamepad config window
void openGamepadConfig()
{
	GtkWidget* win;
	GtkWidget* vbox;
	GtkWidget* hboxPadNo;
	GtkWidget* padNoLabel;
	GtkWidget* fourScoreChk;
	
	GtkWidget* buttons[10];
	
	win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(win), "Gamepad Config");
	gtk_widget_set_size_request(win, 250, 500);
	vbox = gtk_vbox_new(TRUE, 4);
	hboxPadNo = gtk_hbox_new(FALSE, 5);
	padNoLabel = gtk_label_new("Gamepad Number:");
	fourScoreChk = gtk_check_button_new_with_label("Enable four score");
	
	typeCombo = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(typeCombo), "gamepad");
	gtk_combo_box_append_text(GTK_COMBO_BOX(typeCombo), "zapper");
	gtk_combo_box_append_text(GTK_COMBO_BOX(typeCombo), "powerpad.0");
	gtk_combo_box_append_text(GTK_COMBO_BOX(typeCombo), "powerpad.1");
	gtk_combo_box_append_text(GTK_COMBO_BOX(typeCombo), "arkanoid");
	
	gtk_combo_box_set_active(GTK_COMBO_BOX(typeCombo), 0);
	
	
	padNoCombo = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(padNoCombo), "1");
	gtk_combo_box_append_text(GTK_COMBO_BOX(padNoCombo), "2");
	gtk_combo_box_append_text(GTK_COMBO_BOX(padNoCombo), "3");
	gtk_combo_box_append_text(GTK_COMBO_BOX(padNoCombo), "4");
	
	gtk_combo_box_set_active(GTK_COMBO_BOX(padNoCombo), 0);
	
	g_signal_connect(GTK_OBJECT(typeCombo), "changed", G_CALLBACK(setInputDevice), 
		gtk_combo_box_get_active_text(GTK_COMBO_BOX(typeCombo)));
	
	// sync with config
	int buf = 0;
	g_config->getOption("SDL.FourScore", &buf);
	if(buf)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fourScoreChk), 1);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fourScoreChk), 0);
	
	g_signal_connect(GTK_OBJECT(fourScoreChk), "clicked", G_CALLBACK(toggleOption), (gpointer)"SDL.FourScore");
	
	gtk_box_pack_start(GTK_BOX(hboxPadNo), padNoLabel, TRUE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX(hboxPadNo), padNoCombo, TRUE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hboxPadNo, TRUE, TRUE, 5);
	//gtk_box_pack_start_defaults(GTK_BOX(vbox), typeCombo);
	
	gtk_box_pack_start(GTK_BOX(vbox), fourScoreChk, TRUE, TRUE, 5);
	// create gamepad buttons
	for(int i=0; i<10; i++)
	{
		buttons[i] = gtk_button_new_with_label(GamePadNames[i]);
		gtk_box_pack_start(GTK_BOX(vbox), buttons[i], TRUE, TRUE, 3);
		gtk_signal_connect(GTK_OBJECT(buttons[i]), "clicked", G_CALLBACK(configGamepadButton), GINT_TO_POINTER(i));	
	}
	
	gtk_container_add(GTK_CONTAINER(win), vbox);
	
	gtk_widget_show_all(win);
	
	return;
}

int setBufSize(GtkWidget* w, gpointer p)
{
	int x = gtk_range_get_value(GTK_RANGE(w));
	g_config->setOption("SDL.Sound.BufSize", x);
	// reset sound subsystem for changes to take effect
	KillSound();
	InitSound();
	g_config->save();
	return false;
}

void setRate(GtkWidget* w, gpointer p)
{
	char* str = gtk_combo_box_get_active_text(GTK_COMBO_BOX(w));
	g_config->setOption("SDL.Sound.Rate", atoi(str));
	// reset sound subsystem for changes to take effect
	KillSound();
	InitSound();
	g_config->save();	
	return;
}

void setQuality(GtkWidget* w, gpointer p)
{
	char* str = gtk_combo_box_get_active_text(GTK_COMBO_BOX(w));
	if(!strcmp(str, "Very High"))
		g_config->setOption("SDL.Sound.Quality", 2);
	if(!strcmp(str, "High"))
		g_config->setOption("SDL.Sound.Quality", 1);
	if(!strcmp(str, "Low"))
		g_config->setOption("SDL.Sound.Quality", 0);
	// reset sound subsystem for changes to take effect
	KillSound();
	InitSound();
	g_config->save();	
	return;
}

void resizeGtkWindow()
{
	if(GameInfo == 0)
	{
		double xscale, yscale;
		g_config->getOption("SDL.XScale", &xscale);
		g_config->getOption("SDL.YScale", &yscale);
		gtk_widget_set_size_request(socket, 256*xscale, 224*yscale);
		GtkRequisition req;
		gtk_widget_size_request(GTK_WIDGET(MainWindow), &req);
		gtk_window_resize(GTK_WINDOW(MainWindow), req.width, req.height);
	}
	return;
}

void setScaler(GtkWidget* w, gpointer p)
{
	int x = gtk_combo_box_get_active(GTK_COMBO_BOX(w));
	g_config->setOption("SDL.SpecialFilter", x);
	g_config->save();
	
	// 1 - hq2x 2 - Scale2x 3 - NTSC2x 4 - hq3x  5 - Scale3x
	if (x >= 1 && x <= 3)
	{
		g_config->setOption("SDL.XScale", 2.0);
		g_config->setOption("SDL.YScale", 2.0);
		g_config->save();
		resizeGtkWindow();
	}
	if (x >= 4 && x < 6)
	{
		g_config->setOption("SDL.XScale", 3.0);
		g_config->setOption("SDL.YScale", 3.0);
		g_config->save();
		resizeGtkWindow();
	}
	
}


int setXscale(GtkWidget* w, gpointer p)
{
	double v = gtk_spin_button_get_value(GTK_SPIN_BUTTON(w));
	g_config->setOption("SDL.XScale", v);
	g_config->save();
	resizeGtkWindow();
	return 0;
}

int setYscale(GtkWidget* w, gpointer p)
{
	double v = gtk_spin_button_get_value(GTK_SPIN_BUTTON(w));
	g_config->setOption("SDL.YScale", v);
	g_config->save();
	resizeGtkWindow();
	return 0;
}

void setGl(GtkWidget* w, gpointer p)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)))
		g_config->setOption("SDL.OpenGL", 1);
	else
		g_config->setOption("SDL.OpenGL", 0);
	g_config->save();
}
	

void openVideoConfig()
{
	GtkWidget* win;
	GtkWidget* vbox;
	GtkWidget* lbl;
	GtkWidget* hbox1;
	GtkWidget* scalerLbl;
	GtkWidget* scalerCombo;
	GtkWidget* glChk;
	GtkWidget* palChk;
	GtkWidget* ppuChk;
	GtkWidget* xscaleSpin;
	GtkWidget* yscaleSpin;
	GtkWidget* xscaleLbl;
	GtkWidget* yscaleLbl;
	GtkWidget* xscaleHbox;
	GtkWidget* yscaleHbox;
	
	
	win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(win), "Video Preferences");
	//gtk_widget_set_size_request(win, 250, 250);
	
	vbox = gtk_vbox_new(FALSE, 5);
	
	lbl = gtk_label_new("Video options will not take\neffect until the emulator is restarted.");
	
	// scalar widgets
	hbox1 = gtk_hbox_new(FALSE, 3);
	scalerLbl = gtk_label_new("Special Scaler: ");
	scalerCombo = gtk_combo_box_new_text();
	// -Video Modes Tag-
	gtk_combo_box_append_text(GTK_COMBO_BOX(scalerCombo), "none");
	gtk_combo_box_append_text(GTK_COMBO_BOX(scalerCombo), "hq2x");
	gtk_combo_box_append_text(GTK_COMBO_BOX(scalerCombo), "scale2x");
	gtk_combo_box_append_text(GTK_COMBO_BOX(scalerCombo), "NTSC 2x");
	gtk_combo_box_append_text(GTK_COMBO_BOX(scalerCombo), "hq3x");
	gtk_combo_box_append_text(GTK_COMBO_BOX(scalerCombo), "scale3x");
	
	// sync with cfg
	int buf;
	g_config->getOption("SDL.SpecialFilter", &buf);
	gtk_combo_box_set_active(GTK_COMBO_BOX(scalerCombo), buf);
	
	g_signal_connect(GTK_OBJECT(scalerCombo), "changed", G_CALLBACK(setScaler), NULL);
	gtk_box_pack_start(GTK_BOX(hbox1), scalerLbl, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hbox1), scalerCombo, FALSE, FALSE, 5);
	
	// openGL check
	glChk = gtk_check_button_new_with_label("Enable OpenGL");
	g_signal_connect(GTK_OBJECT(glChk), "clicked", G_CALLBACK(setGl), (gpointer)scalerCombo);
	
	// sync with config
	g_config->getOption("SDL.OpenGL", &buf);
	if(buf)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glChk), 1);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glChk), 0);
		
	// PAL check
	palChk = gtk_check_button_new_with_label("Enable PAL mode");
	g_signal_connect(GTK_OBJECT(palChk), "clicked", G_CALLBACK(toggleOption), (gpointer)"SDL.PAL");
	
	// sync with config
	g_config->getOption("SDL.PAL", &buf);
	if(buf)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(palChk), 1);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(palChk), 0);
		
	// New PPU check
	ppuChk = gtk_check_button_new_with_label("Enable new PPU");
	g_signal_connect(GTK_OBJECT(ppuChk), "clicked", G_CALLBACK(toggleOption), (gpointer)"SDL.NewPPU");
	
	// sync with config
	buf = 0;
	g_config->getOption("SDL.NewPPU", &buf);
	if(buf)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ppuChk), 1);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ppuChk), 0);
		
	// xscale / yscale
	xscaleHbox = gtk_hbox_new(FALSE, 5);
	xscaleLbl = gtk_label_new("X scaling factor");
	xscaleSpin = gtk_spin_button_new_with_range(1.0, 10.0, .1);
	yscaleHbox = gtk_hbox_new(FALSE, 5);
	yscaleLbl = gtk_label_new("Y scaling factor");
	yscaleSpin = gtk_spin_button_new_with_range(1.0, 10.0, .1);
	
	gtk_box_pack_start(GTK_BOX(xscaleHbox), xscaleLbl, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(xscaleHbox), xscaleSpin, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(yscaleHbox), yscaleLbl, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(yscaleHbox), yscaleSpin, FALSE, FALSE, 2);
	
	g_signal_connect(xscaleSpin, "value-changed", G_CALLBACK(setXscale), NULL);
	g_signal_connect(yscaleSpin, "value-changed", G_CALLBACK(setYscale), NULL);
	
	double f;
	// sync with config
	g_config->getOption("SDL.XScale", &f);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(xscaleSpin), f);
	g_config->getOption("SDL.YScale", &f);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(yscaleSpin), f);
	

	
	gtk_box_pack_start(GTK_BOX(vbox), lbl, FALSE, FALSE, 5);	
	gtk_box_pack_start(GTK_BOX(vbox), hbox1, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), glChk, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), palChk, FALSE, FALSE,5);
	gtk_box_pack_start(GTK_BOX(vbox), ppuChk, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), xscaleHbox, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), yscaleHbox, FALSE, FALSE, 5);
	
	
	gtk_container_add(GTK_CONTAINER(win), vbox);
	gtk_widget_show_all(win);
	
	return;
}
const char* mixerStrings[6] = {"Volume", "Triangle", "Square1", "Square2", "Noise", "PCM"};

int mixerChanged(GtkWidget* w, gpointer p)
{
	int v = gtk_range_get_value(GTK_RANGE(w));
	GtkWidget* parent = gtk_widget_get_parent(w);
	char* lbl = (char*)gtk_frame_get_label(GTK_FRAME(parent));
	if(strcmp(lbl, "Volume") == 0)
	{
		g_config->setOption("SDL.Sound.Volume", v);
		FCEUI_SetSoundVolume(v);
	}
	if(strcmp(lbl, "Triangle") == 0)
	{
		g_config->setOption("SDL.Sound.TriangleVolume", v);
		FCEUI_SetTriangleVolume(v);
	}
	if(strcmp(lbl, "Square1") == 0)
	{
		g_config->setOption("SDL.Sound.Square1Volume", v);
		FCEUI_SetSquare1Volume(v);
	}
	if(strcmp(lbl, "Square2") == 0)
	{
		g_config->setOption("SDL.Sound.Square2Volume", v);
		FCEUI_SetSquare2Volume(v);
	}
	if(strcmp(lbl, "Noise") == 0)
	{
		g_config->setOption("SDL.Sound.NoiseVolume", v);
		FCEUI_SetNoiseVolume(v);
	}
	if(strcmp(lbl, "PCM") == 0)
	{
		g_config->setOption("SDL.Sound.PCMVolume", v);
		FCEUI_SetPCMVolume(v);
	}
	
	return 0;
}
	

void openSoundConfig()
{
	GtkWidget* win;
	GtkWidget* main_hbox;
	GtkWidget* vbox;
	GtkWidget* soundChk;
	GtkWidget* lowpassChk;
	GtkWidget* hbox1;
	GtkWidget* qualityCombo;
	GtkWidget* qualityLbl;
	GtkWidget* hbox2;
	GtkWidget* rateCombo;
	GtkWidget* rateLbl;
	GtkWidget* hbox3;
	GtkWidget* bufferLbl;
	GtkWidget* bufferHscale;
	GtkWidget* mixerFrame;
	GtkWidget* mixerHbox;
	GtkWidget* mixers[6];
	GtkWidget* mixerFrames[6];
	
	
	win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(win), "Sound Preferences");
	main_hbox = gtk_hbox_new(FALSE, 15);
	vbox = gtk_vbox_new(False, 5);
	//gtk_widget_set_size_request(win, 300, 200);
	
	
	// sound enable check
	soundChk = gtk_check_button_new_with_label("Enable sound");
	
	// sync with cfg
	int cfgBuf;
	g_config->getOption("SDL.Sound", &cfgBuf);
	if(cfgBuf)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(soundChk), TRUE);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(soundChk), FALSE);
	
	gtk_signal_connect(GTK_OBJECT(soundChk), "clicked",
	 G_CALLBACK(toggleSound), NULL);
	 

	// low pass filter check
	lowpassChk = gtk_check_button_new_with_label("Enable low pass filter");
	
	// sync with cfg
	g_config->getOption("SDL.Sound.LowPass", &cfgBuf);
	if(cfgBuf)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lowpassChk), TRUE);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lowpassChk), FALSE);
	
	gtk_signal_connect(GTK_OBJECT(lowpassChk), "clicked", G_CALLBACK(toggleLowPass), NULL);
	
	// sound quality combo box
	hbox1 = gtk_hbox_new(FALSE, 3);
	qualityCombo = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(qualityCombo), "Low");
	gtk_combo_box_append_text(GTK_COMBO_BOX(qualityCombo), "High");
	gtk_combo_box_append_text(GTK_COMBO_BOX(qualityCombo), "Very High");

	// sync widget with cfg 
	g_config->getOption("SDL.Sound.Quality", &cfgBuf);
	if(cfgBuf == 2)
		gtk_combo_box_set_active(GTK_COMBO_BOX(qualityCombo), 2);
	else if(cfgBuf == 1)
		gtk_combo_box_set_active(GTK_COMBO_BOX(qualityCombo), 1);
	else
		gtk_combo_box_set_active(GTK_COMBO_BOX(qualityCombo), 0);
		
	g_signal_connect(qualityCombo, "changed", G_CALLBACK(setQuality), NULL);
	
	qualityLbl = gtk_label_new("Quality: ");
	
	gtk_box_pack_start(GTK_BOX(hbox1), qualityLbl, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hbox1), qualityCombo, FALSE, FALSE, 5);
	
	// sound rate widgets
	hbox2 = gtk_hbox_new(FALSE, 3);
	rateCombo = gtk_combo_box_new_text();
	
	const int rates[5] = {11025, 22050, 44100, 48000, 96000};
	
	char buf[8];
	for(int i=0; i<5;i++)
	{
		sprintf(buf, "%d", rates[i]);
		gtk_combo_box_append_text(GTK_COMBO_BOX(rateCombo), buf);	
	}
	
	// sync widget with cfg 
	g_config->getOption("SDL.Sound.Rate", &cfgBuf);
	for(int i=0; i<5; i++)
		if(cfgBuf == rates[i])
			gtk_combo_box_set_active(GTK_COMBO_BOX(rateCombo), i);
		
	g_signal_connect(rateCombo, "changed", G_CALLBACK(setRate), NULL);
	
	
	// sound rate widgets
	rateLbl = gtk_label_new("Rate (Hz): ");
	
	gtk_box_pack_start(GTK_BOX(hbox2), rateLbl, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hbox2), rateCombo, FALSE, FALSE, 5);
	
	hbox3 = gtk_hbox_new(FALSE, 2);
	bufferHscale = gtk_hscale_new_with_range(15, 200, 2);
	bufferLbl = gtk_label_new("Buffer size (in ms)");
	
	// sync widget with cfg 
	g_config->getOption("SDL.Sound.BufSize", &cfgBuf);
	gtk_range_set_value(GTK_RANGE(bufferHscale), cfgBuf);
	
	g_signal_connect(bufferHscale, "button-release-event", G_CALLBACK(setBufSize), NULL);
	
	
	// mixer
	mixerFrame = gtk_frame_new("Mixer:");
	mixerHbox = gtk_hbox_new(TRUE, 5);
	for(int i=0; i<6; i++)
	{
		mixers[i] = gtk_vscale_new_with_range(0, 256, 1);
		gtk_range_set_inverted(GTK_RANGE(mixers[i]), TRUE);
		mixerFrames[i] = gtk_frame_new(mixerStrings[i]);
		gtk_container_add(GTK_CONTAINER(mixerFrames[i]), mixers[i]);
		gtk_box_pack_start(GTK_BOX(mixerHbox), mixerFrames[i], FALSE, TRUE, 5);
		g_signal_connect(mixers[i], "button-release-event", G_CALLBACK(mixerChanged), (gpointer)i); 
	}
	
	// sync with cfg
	int v;
	g_config->getOption("SDL.Sound.Volume", &v);
	gtk_range_set_value(GTK_RANGE(mixers[0]), v);
	g_config->getOption("SDL.Sound.TriangleVolume", &v);
	gtk_range_set_value(GTK_RANGE(mixers[1]), v);
	g_config->getOption("SDL.Sound.Square1Volume", &v);
	gtk_range_set_value(GTK_RANGE(mixers[2]), v);
	g_config->getOption("SDL.Sound.Square2Volume", &v);
	gtk_range_set_value(GTK_RANGE(mixers[3]), v);
	g_config->getOption("SDL.Sound.NoiseVolume", &v);
	gtk_range_set_value(GTK_RANGE(mixers[4]), v);
	g_config->getOption("SDL.Sound.PCMVolume", &v);
	gtk_range_set_value(GTK_RANGE(mixers[5]), v);

	
	// packing some boxes
	
	gtk_box_pack_start(GTK_BOX(main_hbox), vbox, FALSE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), soundChk, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), lowpassChk, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox1, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), bufferLbl, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), bufferHscale, FALSE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX(main_hbox), mixerFrame, TRUE, TRUE, 5);
	gtk_container_add(GTK_CONTAINER(mixerFrame), mixerHbox);
	
	gtk_container_add(GTK_CONTAINER(win), main_hbox);
	
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

void showAbout ()
{
	GtkWidget* aboutDialog;
	
	aboutDialog = gtk_about_dialog_new ();
	gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(aboutDialog), "About fceuX");
	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(aboutDialog), "fceuX");
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(aboutDialog), FCEU_VERSION_STRING);
	gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(aboutDialog), "GPL-2; See COPYING");
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(aboutDialog), "http://fceux.com");
	
	
	gtk_dialog_run(GTK_DIALOG(aboutDialog));
	gtk_widget_hide_all(aboutDialog);
	
}

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

void emuReset ()
{
	if(isloaded)
		ResetNES();
}

void emuPause ()
{
	if(isloaded)
		FCEUI_SetEmulationPaused(1);
}
void emuResume ()
{
	if(isloaded)
		FCEUI_SetEmulationPaused(0);
}

void enableFullscreen ()
{
	if(isloaded)
		ToggleFS();
}
void recordMovie()
{
	char* movie_fname = const_cast<char*>(FCEU_MakeFName(FCEUMKF_MOVIE, 0, 0).c_str());
	FCEUI_printf("Recording movie to %s\n", movie_fname);
    FCEUI_SaveMovie(movie_fname, MOVIE_FLAG_NONE, L"");
    
    return;
}
void recordMovieAs ()
{
	GtkWidget* fileChooser;
	
	GtkFileFilter* filterFm2;
	GtkFileFilter* filterAll;
	
	filterFm2 = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filterFm2, "*.fm2");
	gtk_file_filter_set_name(filterFm2, "FM2 Movies");
	
	filterAll = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filterAll, "*");
	gtk_file_filter_set_name(filterAll, "All Files");
	
	fileChooser = gtk_file_chooser_dialog_new ("Save FM2 movie for recording", GTK_WINDOW(MainWindow),
			GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER(fileChooser), ".fm2");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fileChooser), filterFm2);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fileChooser), filterAll);
	
	if (gtk_dialog_run (GTK_DIALOG (fileChooser)) ==GTK_RESPONSE_ACCEPT)
	{
		std::string fname;
		
		fname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (fileChooser));
		if (!fname.size())
			return; // no filename selected, quit the whole thing
		
		std::string s = GetUserText("Author name");
		std::wstring author(s.begin(), s.end());

		
		FCEUI_SaveMovie(fname.c_str(), MOVIE_FLAG_FROM_POWERON, author);
	}
	gtk_widget_destroy (fileChooser);
}

void loadMovie ()
{
	GtkWidget* fileChooser;
	
	GtkFileFilter* filterFm2;
	GtkFileFilter* filterAll;
	
	filterFm2 = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filterFm2, "*.fm2");
	gtk_file_filter_add_pattern(filterFm2, "*.FM2f");
	gtk_file_filter_set_name(filterFm2, "FM2 Movies");
	
	filterAll = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filterAll, "*");
	gtk_file_filter_set_name(filterAll, "All Files");
	
	fileChooser = gtk_file_chooser_dialog_new ("Open FM2 Movie", GTK_WINDOW(MainWindow),
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
			
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fileChooser), filterFm2);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fileChooser), filterAll);
	
	if (gtk_dialog_run (GTK_DIALOG (fileChooser)) ==GTK_RESPONSE_ACCEPT)
	{
		char* fname;
		
		fname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (fileChooser));
		static int pauseframe;
        g_config->getOption("SDL.PauseFrame", &pauseframe);
        g_config->setOption("SDL.PauseFrame", 0);
        FCEUI_printf("Playing back movie located at %s\n", fname);
        if(FCEUI_LoadMovie(fname, false, false, pauseframe ? pauseframe : false) == FALSE)
        {
			GtkWidget* d;
			d = gtk_message_dialog_new(GTK_WINDOW(MainWindow), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, 
				"Could not open the selected FM2 file.");
			gtk_dialog_run(GTK_DIALOG(d));
			gtk_widget_destroy(d);
		}
	}
	gtk_widget_destroy (fileChooser);
}

#ifdef _S9XLUA_H
void loadLua ()
{
	GtkWidget* fileChooser;
	GtkFileFilter* filterLua;
	GtkFileFilter* filterAll;
	
	filterLua = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filterLua, "*.lua");
	gtk_file_filter_add_pattern(filterLua, "*.LUA");
	gtk_file_filter_set_name(filterLua, "Lua scripts");
	
	filterAll = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filterAll, "*");
	gtk_file_filter_set_name(filterAll, "All Files");
	
	fileChooser = gtk_file_chooser_dialog_new ("Open LUA Script", GTK_WINDOW(MainWindow),
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fileChooser), filterLua);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fileChooser), filterAll);
	
	if (gtk_dialog_run (GTK_DIALOG (fileChooser)) ==GTK_RESPONSE_ACCEPT)
	{
		char* filename;
		
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (fileChooser));
		gtk_widget_destroy(fileChooser);
		if(FCEU_LoadLuaCode(filename) == 0)
		{
			GtkWidget* d;
			d = gtk_message_dialog_new(GTK_WINDOW(MainWindow), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, 
				"Could not open the selected lua script.");
			gtk_dialog_run(GTK_DIALOG(d));
			gtk_widget_destroy(d);
		}
		g_free(filename);
	}
	else
		gtk_widget_destroy (fileChooser);
}
#endif


void loadFdsBios ()
{
	GtkWidget* fileChooser;
	GtkFileFilter* filterDiskSys;
	GtkFileFilter* filterRom;
	GtkFileFilter* filterAll;
	
	
	filterDiskSys = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filterDiskSys, "disksys.rom");
	gtk_file_filter_set_name(filterDiskSys, "FDS BIOS");
	
	filterRom = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filterRom, "*.rom");
	gtk_file_filter_set_name(filterRom, "*.rom");
	
	filterAll = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filterAll, "*");
	gtk_file_filter_set_name(filterAll, "All Files");
	
	
	fileChooser = gtk_file_chooser_dialog_new ("Load FDS BIOS (disksys.rom)", GTK_WINDOW(MainWindow),
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fileChooser), filterDiskSys);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fileChooser), filterRom);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fileChooser), filterAll);
	
	if (gtk_dialog_run (GTK_DIALOG (fileChooser)) ==GTK_RESPONSE_ACCEPT)
	{
		char* filename;
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (fileChooser));
		// copy BIOS file to proper place (~/.fceux/disksys.rom)
		std::ifstream f1 (filename,std::fstream::binary);
		std::string fn_out = FCEU_MakeFName(FCEUMKF_FDSROM, 0, "");
		std::ofstream f2 (fn_out.c_str(),std::fstream::trunc|std::fstream::binary);
		gtk_widget_destroy (fileChooser);
		GtkWidget* d;
		d = gtk_message_dialog_new(GTK_WINDOW(MainWindow), GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, 
			"Famicom Disk System BIOS loaded.  If you are you having issues, make sure your BIOS file is 8KB in size.");
		gtk_dialog_run(GTK_DIALOG(d));
		gtk_widget_destroy(d);
	
		f2<<f1.rdbuf();
		g_free(filename);
	}
	else
		gtk_widget_destroy (fileChooser);

}

void loadNSF ()
{
	GtkWidget* fileChooser;
	GtkFileFilter* filterNSF;
	GtkFileFilter* filterZip;
	GtkFileFilter* filterAll;
	
	filterNSF = gtk_file_filter_new();
	filterZip = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filterNSF, "*.nsf");
	gtk_file_filter_add_pattern(filterNSF, "*.NSF");
	gtk_file_filter_add_pattern(filterZip, "*.zip");
	gtk_file_filter_add_pattern(filterZip, "*.ZIP");
	gtk_file_filter_set_name(filterNSF, "NSF sound files");
	gtk_file_filter_set_name(filterZip, "Zip archives");
	
	filterAll = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filterAll, "*");
	gtk_file_filter_set_name(filterAll, "All Files");
	
	fileChooser = gtk_file_chooser_dialog_new ("Open NSF File", GTK_WINDOW(MainWindow),
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	
	const char* last_dir;
	g_config->getOption("SDL.LastOpenNSF", &last_dir);
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(fileChooser), last_dir);
	
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fileChooser), filterNSF);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fileChooser), filterZip);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fileChooser), filterAll);
	
	if (gtk_dialog_run (GTK_DIALOG (fileChooser)) ==GTK_RESPONSE_ACCEPT)
	{
		char* filename;
		
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (fileChooser));
		gtk_widget_destroy (fileChooser);
		if(LoadGame(filename) == 0)
		{
			
			GtkWidget* d;
			d = gtk_message_dialog_new(GTK_WINDOW(MainWindow), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, 
				"Could not open the selected NSF file.");
			gtk_dialog_run(GTK_DIALOG(d));
			gtk_widget_destroy(d);
		}
		g_config->setOption("SDL.LastOpenNSF", filename);
		g_config->save();
		g_free(filename);
	}
	else
		gtk_widget_destroy (fileChooser);
}

void closeGame()
{
	GdkColor bg = {0, 0, 0, 0};
	gtk_widget_modify_bg(socket, GTK_STATE_NORMAL, &bg);
	CloseGame();
}

void loadGame ()
{
	GtkWidget* fileChooser;
	GtkFileFilter* filterFCEU;
	GtkFileFilter* filterNes;
	GtkFileFilter* filterFds;
	GtkFileFilter* filterNSF;
	GtkFileFilter* filterZip;
	GtkFileFilter* filterAll;
	
	filterFCEU = gtk_file_filter_new();
	filterNes = gtk_file_filter_new();
	filterFds = gtk_file_filter_new();
	filterNSF = gtk_file_filter_new();
	filterZip = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filterFCEU, "*.nes");
	gtk_file_filter_add_pattern(filterFCEU, "*.NES");
	gtk_file_filter_add_pattern(filterFCEU, "*.fds");
	gtk_file_filter_add_pattern(filterFCEU, "*.FDS");
	gtk_file_filter_add_pattern(filterFCEU, "*.zip");
	gtk_file_filter_add_pattern(filterFCEU, "*.ZIP");
	gtk_file_filter_add_pattern(filterFCEU, "*.Nes");
	gtk_file_filter_add_pattern(filterFCEU, "*.Fds");
	gtk_file_filter_add_pattern(filterFCEU, "*.Zip");
	gtk_file_filter_add_pattern(filterFCEU, "*.nsf");
	gtk_file_filter_add_pattern(filterFCEU, "*.NSF");
	gtk_file_filter_add_pattern(filterNes, "*.nes");
	gtk_file_filter_add_pattern(filterNes, "*.NES");
	gtk_file_filter_add_pattern(filterFds, "*.fds");
	gtk_file_filter_add_pattern(filterFds, "*.FDS");
	gtk_file_filter_add_pattern(filterNSF, "*.nsf");
	gtk_file_filter_add_pattern(filterNSF, "*.NSF");
	gtk_file_filter_add_pattern(filterZip, "*.zip");
	gtk_file_filter_add_pattern(filterZip, "*.zip");
	gtk_file_filter_set_name(filterFCEU, "*.nes;*.fds;*.nsf;*.zip");
	gtk_file_filter_set_name(filterNes, "NES ROM files");
	gtk_file_filter_set_name(filterFds, "FDS ROM files");
	gtk_file_filter_set_name(filterNSF, "NSF sound files");
	gtk_file_filter_set_name(filterZip, "Zip archives");
	
	filterAll = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filterAll, "*");
	gtk_file_filter_set_name(filterAll, "All Files");
	
	
	
	fileChooser = gtk_file_chooser_dialog_new ("Open ROM", GTK_WINDOW(MainWindow),
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	const char* last_dir;
	g_config->getOption("SDL.LastOpenFile", &last_dir);
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(fileChooser), last_dir);
	
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fileChooser), filterFCEU);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fileChooser), filterNes);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fileChooser), filterFds);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fileChooser), filterNSF);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fileChooser), filterZip);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fileChooser), filterAll);
	
	if (gtk_dialog_run (GTK_DIALOG (fileChooser)) ==GTK_RESPONSE_ACCEPT)
	{
		char* filename;
		
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (fileChooser));
		gtk_widget_destroy (fileChooser);
		g_config->setOption("SDL.LastOpenFile", filename);
		g_config->save();
		closeGame();
		if(LoadGame(filename) == 0)
		{
			
			GtkWidget* d;
			d = gtk_message_dialog_new(GTK_WINDOW(MainWindow), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, 
				"Could not open the selected ROM file.");
			gtk_dialog_run(GTK_DIALOG(d));
			gtk_widget_destroy(d);
		}
		resizeGtkWindow();
		g_free(filename);
	}
	else
		gtk_widget_destroy (fileChooser);
}

void saveStateAs()
{
	GtkWidget* fileChooser;
	GtkFileFilter* filterSav;
	GtkFileFilter* filterAll;
	
	filterSav = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filterSav, "*.sav");
	gtk_file_filter_add_pattern(filterSav, "*.SAV");
	gtk_file_filter_set_name(filterSav, "SAV files");
	
	filterAll = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filterAll, "*");
	gtk_file_filter_set_name(filterAll, "All Files");
	
	const char* last_dir;
	g_config->getOption("SDL.LastSaveStateAs", &last_dir);
	
	fileChooser = gtk_file_chooser_dialog_new ("Save State As", GTK_WINDOW(MainWindow),
			GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
			
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(fileChooser), last_dir);
	
	gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER(fileChooser), ".sav");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fileChooser), filterSav);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fileChooser), filterAll);
	
	if (gtk_dialog_run (GTK_DIALOG (fileChooser)) ==GTK_RESPONSE_ACCEPT)
	{
		char* filename;
		
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (fileChooser));
		FCEUI_SaveState(filename);
		g_config->setOption("SDL.LastSaveStateAs", filename);
		g_config->save();
		g_free(filename);
	}
	gtk_widget_destroy (fileChooser);
	
	
}

void loadStateFrom()
{
	GtkWidget* fileChooser;
	GtkFileFilter* filterSav;
	GtkFileFilter* filterAll;
	
	filterSav = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filterSav, "*.sav");
	gtk_file_filter_add_pattern(filterSav, "*.SAV");
	gtk_file_filter_set_name(filterSav, "SAV files");
	
	filterAll = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filterAll, "*");
	gtk_file_filter_set_name(filterAll, "All Files");
	
	fileChooser = gtk_file_chooser_dialog_new ("Load State From", GTK_WINDOW(MainWindow),
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
			
	const char* last_dir;
	g_config->getOption("SDL.LastLoadStateFrom", &last_dir);
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(fileChooser), last_dir);

	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fileChooser), filterSav);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fileChooser), filterAll);
	
	if (gtk_dialog_run (GTK_DIALOG (fileChooser)) ==GTK_RESPONSE_ACCEPT)
	{
		char* filename;
		
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (fileChooser));
		FCEUI_LoadState(filename);
		g_config->setOption("SDL.LastLoadStateFrom", filename);
		g_config->save();
		g_free(filename);
	}
	
	gtk_widget_destroy (fileChooser);
	
	
}


// Adapted from Gens/GS.  Converts a GDK key value into an SDL key value.
unsigned short GDKToSDLKeyval(int gdk_key)
{
	if (!(gdk_key & 0xFF00))
	{
		// ASCII symbol.
		// SDL and GDK use the same values for these keys.
		
		// Make sure the key value is lowercase.
		gdk_key = tolower(gdk_key);
		
		// Return the key value.
		return gdk_key;
	}
	
	if (gdk_key & 0xFFFF0000)
	{
		// Extended X11 key. Not supported by SDL.
#ifdef GDK_WINDOWING_X11
		fprintf(stderr, "Unhandled extended X11 key: 0x%08X (%s)", gdk_key, XKeysymToString(gdk_key));
#else
		fprintf(stderr, "Unhandled extended key: 0x%08X\n", gdk_key);
#endif
		return 0;
	}
	
	// Non-ASCII symbol.
	static const uint16_t gdk_to_sdl_table[0x100] =
	{
		// 0x00 - 0x0F
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		SDLK_BACKSPACE, SDLK_TAB, SDLK_RETURN, SDLK_CLEAR,
		0x0000, SDLK_RETURN, 0x0000, 0x0000,
		
		// 0x10 - 0x1F
		0x0000, 0x0000, 0x0000, SDLK_PAUSE,
		SDLK_SCROLLOCK, SDLK_SYSREQ, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, SDLK_ESCAPE,
		0x0000, 0x0000, 0x0000, 0x0000,
		
		// 0x20 - 0x2F
		SDLK_COMPOSE, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		
		// 0x30 - 0x3F [Japanese keys]
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		
		// 0x40 - 0x4F [unused]
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		
		// 0x50 - 0x5F
		SDLK_HOME, SDLK_LEFT, SDLK_UP, SDLK_RIGHT,
		SDLK_DOWN, SDLK_PAGEUP, SDLK_PAGEDOWN, SDLK_END,
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		
		// 0x60 - 0x6F
		0x0000, SDLK_PRINT, 0x0000, SDLK_INSERT,
		SDLK_UNDO, 0x0000, 0x0000, SDLK_MENU,
		0x0000, SDLK_HELP, SDLK_BREAK, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		
		// 0x70 - 0x7F [mostly unused, except for Alt Gr and Num Lock]
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, SDLK_MODE, SDLK_NUMLOCK,
		
		// 0x80 - 0x8F [mostly unused, except for some numeric keypad keys]
		SDLK_KP5, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, SDLK_KP_ENTER, 0x0000, 0x0000,
		
		// 0x90 - 0x9F
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, SDLK_KP7, SDLK_KP4, SDLK_KP8,
		SDLK_KP6, SDLK_KP2, SDLK_KP9, SDLK_KP3,
		SDLK_KP1, SDLK_KP5, SDLK_KP0, SDLK_KP_PERIOD,
		
		// 0xA0 - 0xAF
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, SDLK_KP_MULTIPLY, SDLK_KP_PLUS,
		0x0000, SDLK_KP_MINUS, SDLK_KP_PERIOD, SDLK_KP_DIVIDE,
		
		// 0xB0 - 0xBF
		SDLK_KP0, SDLK_KP1, SDLK_KP2, SDLK_KP3,
		SDLK_KP4, SDLK_KP5, SDLK_KP6, SDLK_KP7,
		SDLK_KP8, SDLK_KP9, 0x0000, 0x0000,
		0x0000, SDLK_KP_EQUALS, SDLK_F1, SDLK_F2,
		
		// 0xC0 - 0xCF
		SDLK_F3, SDLK_F4, SDLK_F5, SDLK_F6,
		SDLK_F7, SDLK_F8, SDLK_F9, SDLK_F10,
		SDLK_F11, SDLK_F12, SDLK_F13, SDLK_F14,
		SDLK_F15, 0x0000, 0x0000, 0x0000,
		
		// 0xD0 - 0xDF [L* and R* function keys]
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		
		// 0xE0 - 0xEF
		0x0000, SDLK_LSHIFT, SDLK_RSHIFT, SDLK_LCTRL,
		SDLK_RCTRL, SDLK_CAPSLOCK, 0x0000, SDLK_LMETA,
		SDLK_RMETA, SDLK_LALT, SDLK_RALT, SDLK_LSUPER,
		SDLK_RSUPER, 0x0000, 0x0000, 0x0000,
		
		// 0xF0 - 0xFF [mostly unused, except for Delete]
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, SDLK_DELETE,		
	};
	
	unsigned short sdl_key = gdk_to_sdl_table[gdk_key & 0xFF];
	if (sdl_key == 0)
	{
		// Unhandled GDK key.
		fprintf(stderr, "Unhandled GDK key: 0x%04X (%s)", gdk_key, gdk_keyval_name(gdk_key));
		return 0;
	}
	
	return sdl_key;
}


// Function adapted from Gens/GS (source/gens/input/input_sdl.c)
gint convertKeypress(GtkWidget *grab, GdkEventKey *event, gpointer user_data)
{
	SDL_Event sdlev;
	SDLKey sdlkey;
	int keystate;

	// Only grab keys from the main window.
	if (grab != MainWindow)
	{
		// Don't push this key onto the SDL event stack.
		return FALSE;
	}
	
	switch (event->type)
	{
		case GDK_KEY_PRESS:
			sdlev.type = SDL_KEYDOWN;
			sdlev.key.state = SDL_PRESSED;
			keystate = 1;
			break;
		
		case GDK_KEY_RELEASE:
			sdlev.type = SDL_KEYUP;
			sdlev.key.state = SDL_RELEASED;
			keystate = 0;
			break;
		
		default:
			fprintf(stderr, "Unhandled GDK event type: %d", event->type);
			return FALSE;
	}
	
	// Convert this keypress from GDK to SDL.
	sdlkey = (SDLKey)GDKToSDLKeyval(event->keyval);
	
	// Create an SDL event from the keypress.
	sdlev.key.keysym.sym = sdlkey;
	if (sdlkey != 0)
	{
		SDL_PushEvent(&sdlev);
		#if SDL_VERSION_ATLEAST(1, 3, 0)
		SDL_GetKeyboardState(NULL)[SDL_GetScancodeFromKey(sdlkey)] = keystate;
		#else
		SDL_GetKeyState(NULL)[sdlkey] = keystate;
		#endif
	}
	
	// Allow GTK+ to process this key.
	return FALSE;
}


/* Our menu, an array of GtkItemFactoryEntry structures that defines each menu item */
static GtkItemFactoryEntry menu_items[] = {
  { "/_File",         NULL,         NULL,           0, "<Branch>" },
  //{ "/File/_New",     "<control>N", NULL,    0, "<StockItem>", GTK_STOCK_NEW },
  { "/File/_Open ROM",    "<control>O", loadGame,    0, "<StockItem>", GTK_STOCK_OPEN },
  { "/File/_Close ROM",    "<control>C", closeGame,    0, "<StockItem>", GTK_STOCK_CLOSE },
 // { "/File/Save _As", NULL,         NULL,           0, "<Item>" },
  { "/File/sep1",     NULL,         NULL,           0, "<Separator>" },
  { "/File/_Play NSF",    "<control>N", loadNSF,    0, "<StockItem>", GTK_STOCK_OPEN },
  { "/File/sep2",     NULL,         NULL,           0, "<Separator>" },
  { "/File/Savestate", NULL, NULL, 0, "<Branch>" },
  { "/File/Savestate/Load State _From", NULL, loadStateFrom, 0, "<Item>"},
  { "/File/Savestate/Save State _As", NULL, saveStateAs, 0, "<Item>"},
#ifdef _S9XLUA_H  
  { "/File/Load _Lua Script", NULL, loadLua, 0, "<Item>"},
#endif
  { "/File/sep3",     NULL,         NULL,           0, "<Separator>" },
  { "/File/_Screenshot", "F12", FCEUI_SaveSnapshot, 0, "<Item>"},
  { "/File/sep2",     NULL,         NULL,           0, "<Separator>" },
  { "/File/_Quit",    "<CTRL>Q", quit, 0, "<StockItem>", GTK_STOCK_QUIT },
  { "/_Options",      NULL,         NULL,           0, "<Branch>" },
  { "/_Emulator",	NULL,			NULL,			0, "<Branch>"  },
  { "/Emulator/P_ower", 	NULL, FCEUI_PowerNES, 0, "<Item>"},
  { "/Emulator/_Reset", 	NULL, emuReset, 0, "<Item>"},
  { "/Emulator/_Pause", NULL, emuPause, 0, "<Item>"},
  { "/Emulator/R_esume", NULL, emuResume, 0, "<Item>"},
  { "/Emulator/_FDS", NULL, NULL, 0, "<Branch>"},
  { "/Emulator/_FDS/_Switch Disk", NULL, FCEU_FDSSelect, 0, "<Item>"},
  { "/Emulator/_FDS/_Eject Disk", NULL, FCEU_FDSInsert, 0, "<Item>"},
  { "/Emulator/_FDS/Load _BIOS File", NULL, loadFdsBios, 0, "<Item>"},
  { "/Emulator/_Insert coin", NULL, FCEUI_VSUniCoin, 0, "<Item>"},
  //{ "/Emulator/GTKterm (DEV)", NULL, openGTKterm, 0, "<Item>"},
  { "/_Movie",	NULL,			NULL,			0, "<Branch>"  },
  { "/Movie/_Open", NULL, loadMovie, 0, "<Item>"},
  { "/Movie/S_top", NULL, FCEUI_StopMovie, 0, "<Item>"},
  { "/Movie/_Pause", NULL, emuPause, 0, "<Item>"},
  { "/Movie/R_esume", NULL, emuResume, 0, "<Item>"},
  { "/Movie/sep2",     NULL,         NULL,           0, "<Separator>" },
  { "/Movie/_Record", NULL, recordMovie, 0, "<Item>"},
  { "/Movie/Record _as", NULL, recordMovieAs, 0, "<Item>"},
  { "/Options/_Gamepad Config", NULL , openGamepadConfig, 0, "<StockItem>", GTK_STOCK_PREFERENCES },
  { "/Options/_Sound Config", NULL , openSoundConfig, 0, "<Item>" },
  { "/Options/_Video Config", NULL , openVideoConfig, 0, "<Item>" },
  { "/Options/_Palette Config", NULL , openPaletteConfig, 0, "<Item>" },
  { "/Options/_Network Config", NULL , openNetworkConfig, 0, "<Item>" },
  //{ "/Options/Map _Hotkeys", NULL , openHotkeyConfig, 0, "<Item>" },
  { "/Options/sep1",  NULL,         NULL,           0, "<Separator>" },
  { "/Options/_Fullscreen", "<alt>Return",         enableFullscreen,	   0, "<Item>" },
  { "/_Help",         NULL,         NULL,           0, "<LastBranch>" },
  { "/Help/About",   NULL,         showAbout,           0, "<Item>" },
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
//GtkTextBuffer* gtkConsoleBuf;
//GtkWidget* consoleOutput;
//GtkTextIter iter;
char* buf;
//GtkWidget* term;

void pushOutputToGTK(const char* str)
{
	
	//printf(str);
	//gtk_text_buffer_insert(GTK_TEXT_BUFFER(gtkConsoleBuf), &iter, str, -1);
	//gtk_text_buffer_set_text(gtkConsoleBuf, str, -1);
	
	//vte_terminal_feed_child(VTE_TERMINAL(term), str, -1);
	return;
}

void showGui(bool b)
{
	if(b)
		gtk_widget_show_all(MainWindow);
	else
		gtk_widget_hide_all(MainWindow);
}

int InitGTKSubsystem(int argc, char** argv)
{
	GtkWidget* Menubar;
	GtkWidget* vbox;
	
	MainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_policy (GTK_WINDOW (MainWindow), FALSE, FALSE, TRUE);
	gtk_window_set_title(GTK_WINDOW(MainWindow), FCEU_NAME_AND_VERSION);
	gtk_window_set_default_size(GTK_WINDOW(MainWindow), 256, 224);
	
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(MainWindow), vbox);
	
	Menubar = CreateMenubar(MainWindow);
		
	
	gtk_box_pack_start (GTK_BOX(vbox), Menubar, FALSE, TRUE, 0);
		
	// PRG: this code here is the the windowID "hack" to render SDL
	// in a GTK window.	however, I can't get it to work right now
	// so i'm commenting it out and haivng a seperate GTK2 window with 
	// controls
	// 12/21/09
	// ---
	// uncommented and fixed by Bryan Cain
	// 1/24/11
	//
	// prg - Bryan Cain, you are the man!
	GtkWidget* hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_box_pack_end (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	
	socket = gtk_event_box_new();
	gtk_box_pack_start (GTK_BOX(hbox), socket, TRUE, FALSE, 0);
	
	double xscale, yscale;
	g_config->getOption("SDL.XScale", &xscale);
	g_config->getOption("SDL.YScale", &yscale);
	gtk_widget_set_size_request(socket, 256*xscale, 224*yscale);
	gtk_widget_realize(socket);
	gtk_widget_show(socket);
	
	GdkColor bg = {0, 0, 0, 0};
	gtk_widget_modify_bg(socket, GTK_STATE_NORMAL, &bg);
	
	// set up keypress "snooper" to convert GDK keypress events into SDL keypresses
	gtk_key_snooper_install(convertKeypress, NULL);
	
	g_signal_connect(MainWindow, "destroy-event", quit, NULL);
	
	// signal handlers
	g_signal_connect(MainWindow, "delete-event", quit, NULL);
	
	gtk_widget_show_all(MainWindow);
	
	GtkRequisition req;
	gtk_widget_size_request(GTK_WIDGET(MainWindow), &req);
	gtk_window_resize(GTK_WINDOW(MainWindow), req.width, req.height);
	 
	return 0;
}

