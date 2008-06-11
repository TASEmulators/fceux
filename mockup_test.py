#!/usr/bin/env python
# displays mockup of gamepad config
import gtk, gtk.glade

class WidgetsWrapper:
    def __init__(self):
        self.widgets = gtk.glade.XML ('gfceu.glade', "gamepad_config_window")
        self.widgets.signal_autoconnect(GladeHandlers.__dict__)
    # Gives us the ability to do: widgets['widget_name'].action()
    def __getitem__(self, key):
        return self.widgets.get_widget(key)

widgets = WidgetsWrapper()

widgets['gamepad_config_window'].show_all()
gtk.main ()

