#!/usr/bin/env python
# displays mockup of gamepad config
import gtk

glade_file = 'gfceux.xml'
print "Using: " + glade_file
widgets = gtk.Builder()
widgets.add_from_file(glade_file)

widgets.get_object('gamepad_config_window').show_all()
gtk.main ()

