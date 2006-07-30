#!/usr/bin/env python
# A cool thing that i don't feel like explaining
import gtk
import gobject
import os
class shit:
  def __init__(self, command, title="Status Window"):
    self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
    self.window.set_default_size(400,300)
    self.window.set_title(title)

    self.scroll = gtk.ScrolledWindow()
    self.scroll.set_property("hscrollbar-policy", gtk.POLICY_NEVER)

    self.buffer = gtk.TextBuffer()
    
    self.text_view = gtk.TextView(self.buffer)
    self.text_view.set_editable(False)
    self.window.add(self.scroll)
    self.scroll.add(self.text_view)

    gobject.timeout_add(10, self.get_output)
    
    self.handle = os.popen(command, "r",1)
    self.add(self.handle.readlines())
  def get_output(self):
    self.add(self.handle.read())

  def show(self):
    self.window.show_all()

  def hide(self):
    self.window.hide_all()
  
  def add(self, string):
    self.buffer.insert_at_cursor(string)

myshit = shit("fceu-server", "THE StatUZ")
myshit.show()
gtk.main()
