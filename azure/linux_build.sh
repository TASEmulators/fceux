#!/bin/bash

id
pwd
cat /etc/os-release

gcc --version
python2 --version
python3 --version

sudo apt-get install lua5.1-dev
pkg-config --cflags --libs  lua5.1
sudo apt-get install libsdl1.2-dev
sdl-config --cflags --libs
sudo apt-get install libsdl2-dev
sdl2-config --cflags --libs
sudo apt-get install libminizip-dev
pkg-config --cflags --libs  minizip
#sudo apt-get install libxml2-dev
#sudo apt-get install libgtk2.0-dev
sudo apt-get install libgtk-3-dev
pkg-config --cflags --libs  gtk+-3.0
sudo apt-get install libgtksourceview-3.0-dev
pkg-config --cflags --libs  gtksourceview-3.0
sudo apt-get install scons

scons   --clean
scons   GTK3=1   SYSTEM_LUA=1   SYSTEM_MINIZIP=1   CREATE_AVI=1

ls -ltr ./bin/*
