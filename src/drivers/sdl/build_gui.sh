#!/bin/sh
# builds the gtk2 gui test app
# requires gtk2 libraries
g++ `pkg-config --cflags --libs gtk+-2.0` -lSDL gui.cpp -o gui
