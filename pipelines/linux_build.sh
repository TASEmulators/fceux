#!/bin/bash

id
pwd
uname -a
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
echo '****************************************'
echo 'Install Dependency Updates'
echo '****************************************'
sudo apt-get --assume-yes update
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

# GTK3 was retired in favor of cross platform QT
## Install GTK+-3 
#echo '****************************************'
#echo 'Install Dependency libgtk-3-dev'
#echo '****************************************'
sudo apt-get --assume-yes  install libgtk-3-dev
pkg-config --cflags --libs  gtk+-3.0
#
## Install GTK+-3 Sourceview
#sudo apt-get --assume-yes  install libgtksourceview-3.0-dev
#pkg-config --cflags --libs  gtksourceview-3.0

# Install QT5 
echo '****************************************'
echo 'Install Dependency Qt5'
echo '****************************************'
sudo apt-get --assume-yes  install qt5-default

# Install scons
#echo '****************************************'
#echo 'Install Build Dependency scons'
#echo '****************************************'
#sudo apt-get --assume-yes  install scons

# Install cppcheck
echo '****************************************'
echo 'Install Check Dependency cppcheck'
echo '****************************************'
sudo apt-get --assume-yes  install cppcheck

echo '**************************'
echo '***  Building Project  ***'
echo '**************************'
mkdir -p $INSTALL_PREFIX/usr;

echo "Num CPU: `nproc`";
mkdir buildQT; cd buildQT;
cmake  \
   -DCMAKE_BUILD_TYPE=Release  \
   -DCMAKE_INSTALL_PREFIX=/usr \
   -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON \
	..
make -j `nproc`
make  install  DESTDIR=$INSTALL_PREFIX

cd ..;
mkdir buildGTK; cd buildGTK;
cmake  \
   -DGTK=1 \
   -DCMAKE_BUILD_TYPE=Release  \
   -DCMAKE_INSTALL_PREFIX=/usr \
   -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON \
	..
make -j `nproc`
make  install  DESTDIR=$INSTALL_PREFIX

# Install Files
#cd .. # cd out of build
#mkdir -p $INSTALL_PREFIX/usr/bin/.
#mkdir -p $INSTALL_PREFIX/usr/share/fceux
#mkdir -p $INSTALL_PREFIX/usr/share/pixmaps
#mkdir -p $INSTALL_PREFIX/usr/share/applications
#mkdir -p $INSTALL_PREFIX/usr/man/man6
#
#cp -f ./build/fceux                       $INSTALL_PREFIX/usr/bin/.
#cp -a ./output/*                          $INSTALL_PREFIX/usr/share/fceux/.
#cp -a ./src/auxlib.lua                    $INSTALL_PREFIX/usr/share/fceux/.
#cp -a ./fceux.png                         $INSTALL_PREFIX/usr/share/pixmaps/.
#cp -a ./fceux.desktop                     $INSTALL_PREFIX/usr/share/applications/.
#cp -a ./documentation/fceux.6             $INSTALL_PREFIX/usr/man/man6/.
#cp -a ./documentation/fceux-net-server.6  $INSTALL_PREFIX/usr/man/man6/.


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

if [ -e $INSTALL_PREFIX/usr/bin/fceux-gtk ]; then
   echo '**************************************************************'
   echo 'Printing Shared Object Dependencies for fceux-gtk Executable'
   echo '**************************************************************'
   ldd  $INSTALL_PREFIX/usr/bin/fceux-gtk
else
   echo "Error: Executable Failed to build: $INSTALL_PREFIX/usr/bin/fceux-gtk";
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
