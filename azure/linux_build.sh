#!/bin/bash

id
pwd
cat /etc/os-release

gcc --version
python2 --version
python3 --version

sudo apt-get install lua5.1-dev
sudo apt-get install libsdl1.2-dev
sudo apt-get install libsdl2-dev
sudo apt-get install libminizip-dev
#sudo apt-get install libxml2-dev
#sudo apt-get install libgtk2.0-dev
sudo apt-get install libgtk-3-dev
sudo apt-get install libgtksourceview-3.0-dev
sudo apt-get install scons

scons   GTK3=1   SYSTEM_LUA=1   SYSTEM_MINIZIP=1   CREATE_AVI=1

ls -ltr ./bin/*
