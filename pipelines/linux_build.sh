#!/bin/bash

id
pwd
cat /etc/os-release

SCRIPT_DIR=$( cd $(dirname $BASH_SOURCE[0]); pwd );

#echo $SCRIPT_DIR;

gcc --version
python2 --version
python3 --version

INSTALL_PREFIX=/tmp/fceux

echo '****************************************'
echo "APPVEYOR_SSH_KEY=$APPVEYOR_SSH_KEY";
echo "APPVEYOR_SSH_BLOCK=$APPVEYOR_SSH_BLOCK";
echo '****************************************'

echo '****************************************'
echo '****************************************'
echo '***  Installing Package Dependencies ***'
echo '****************************************'
echo '****************************************'
# Install Lua-5.1 development package
echo '****************************************'
echo 'Install Dependency lua5.1-dev'
echo '****************************************'
sudo apt-get --assume-yes  install lua5.1-dev
pkg-config --cflags --libs  lua5.1

# Install libSDL-1.2 and libSDL-2
# libSDL-1.2 no long needed
#echo '****************************************'
#echo 'Install Dependency libsdl1.2-dev'
#echo '****************************************'
#sudo apt-get --assume-yes  install libsdl1.2-dev  
#sdl-config --cflags --libs
echo '****************************************'
echo 'Install Dependency libsdl2-dev'
echo '****************************************'
sudo apt-get --assume-yes  install libsdl2-dev  
sdl2-config --cflags --libs

# Install libminizip-dev
echo '****************************************'
echo 'Install Dependency libminizip-dev'
echo '****************************************'
sudo apt-get --assume-yes  install libminizip-dev
pkg-config --cflags --libs  minizip

# GTK+-2 is no longer needed
#sudo apt-get install libgtk2.0-dev

# Install GTK+-3 
echo '****************************************'
echo 'Install Dependency libgtk-3-dev'
echo '****************************************'
sudo apt-get --assume-yes  install libgtk-3-dev
pkg-config --cflags --libs  gtk+-3.0

# Install GTK+-3 Sourceview
sudo apt-get --assume-yes  install libgtksourceview-3.0-dev
pkg-config --cflags --libs  gtksourceview-3.0

# Install scons
echo '****************************************'
echo 'Install Build Dependency scons'
echo '****************************************'
sudo apt-get --assume-yes  install scons

# Install cppcheck
echo '****************************************'
echo 'Install Check Dependency cppcheck'
echo '****************************************'
sudo apt-get --assume-yes  install cppcheck

echo '**************************'
echo '***  Building Project  ***'
echo '**************************'
mkdir -p $INSTALL_PREFIX/usr;
scons   --clean
scons   GTK3=1   SYSTEM_LUA=1   SYSTEM_MINIZIP=1   CREATE_AVI=1  install  --prefix=$INSTALL_PREFIX/usr

# Debug via ssh if necessary
if [ ! -z $APPVEYOR_SSH_BLOCK ]; then
   curl -sflL 'https://raw.githubusercontent.com/appveyor/ci/master/scripts/enable-ssh.sh' | bash -e -
fi

if [ -e $INSTALL_PREFIX/usr/bin/fceux ]; then
   echo '**************************************************************'
   echo 'Printing Shared Object Dependencies for ./bin/fceux Executable'
   echo '**************************************************************'
   ldd  $INSTALL_PREFIX/usr/bin/fceux
else
   echo "Error: Executable Failed to build: $INSTALL_PREFIX/usr/bin/fceux";
   exit 1;
fi

echo '**************************************************************'
echo 'Printing To Be Packaged Files '
echo '**************************************************************'
find $INSTALL_PREFIX

echo '**************************************************************'
echo 'Creating Debian Package'
echo '**************************************************************'
$SCRIPT_DIR/debpkg.pl;

echo '**************************************************************'
echo 'Testing Install of Package'
echo '**************************************************************'
sudo dpkg -i /tmp/fceux-*.deb

echo 'Pushing Debian Package to Build Artifacts'
appveyor PushArtifact /tmp/fceux-*.deb
